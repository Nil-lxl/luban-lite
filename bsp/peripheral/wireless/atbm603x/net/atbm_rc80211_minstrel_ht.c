#if MINSTREL_RATE_CONTROL
#include "atbm_ratectrl.h"
#include "atbm_hal.h"
#define EWMA_LEVEL		75
struct rate_control_ops mac80211_ratectrl_minstrel_ht;

#define CONFIG_COMPAT_MAC80211_RC_DEFAULT "minstrel_ht"  //20M/40M b/g/n mode
#define CONFIG_COMPAT_MAC80211_RC_DEFAULT "minstrel"     //20M bg only
#define CONFIG_COMPAT_MAC80211_RC_DEFAULT "pid" 
#define atbm_swap(a, b) \
	do {         \
		int tmp; \
		tmp = (a); (a) = (b); (b) = tmp; \
	} while (0)
static atbm_uint8 sample_table[SAMPLE_COLUMNS][MCS_GROUP_RATES];
extern struct atbm_mcs_group atbm_minstrel_mcs_groups[3];

/*
 * Perform EWMA (Exponentially Weighted Moving Average) calculation
 */
static int
atbm_minstrel_ewma(int old, int new, int weight)
{
	return (new * (100 - weight) + old * weight) / 100;
}

/*
 * Look up an MCS group index based on mac80211 rate information
 */
static int
atbm_minstrel_ht_get_group_idx(struct atbmwifi_ieee80211_tx_rate *rate)
{
	int streams = (rate->idx / MCS_GROUP_RATES) + 1;
	atbm_uint32 flags = IEEE80211_TX_RC_SHORT_GI | IEEE80211_TX_RC_40_MHZ_WIDTH;
	int i;
	for (i = 0; i < ATBM_ARRAY_SIZE(atbm_minstrel_mcs_groups); i++) {
		if (atbm_minstrel_mcs_groups[i].streams != streams)
			continue;
		if (atbm_minstrel_mcs_groups[i].flags != (rate->flags & flags))
			continue;

		return i;
	}
	return 0;
}

struct atbm_minstrel_rate_stats *
atbm_minstrel_get_ratestats(struct atbm_minstrel_ht_sta *mi, int index)
{
	return &mi->groups[index / MCS_GROUP_RATES].rates[index % MCS_GROUP_RATES];
}
/*
 * Recalculate success probabilities and counters for a rate using EWMA
 */
static atbm_void
atbm_minstrel_calc_rate_ewma(struct atbmwifi_minstrel_priv *mp, struct atbm_minstrel_rate_stats *mr)
{
	if (atbm_unlikely(mr->attempts > 0)) {
		mr->sample_skipped = 0;
		mr->cur_prob = MINSTREL_FRAC(mr->success, mr->attempts);
		if (!mr->att_hist)
			mr->probability = mr->cur_prob;
		else
			mr->probability = atbm_minstrel_ewma(mr->probability,
				mr->cur_prob, EWMA_LEVEL);
		mr->att_hist += mr->attempts;
		mr->succ_hist += mr->success;
	} else {
		mr->sample_skipped++;
	}
	mr->last_success = mr->success;
	mr->last_attempts = mr->attempts;
	mr->success = 0;
	mr->attempts = 0;
}

/*
 * Calculate throughput based on the average A-MPDU length, taking into account
 * the expected number of retransmissions and their expected length
 */
static atbm_void
atbm_minstrel_ht_calc_tp(struct atbmwifi_minstrel_priv *mp, struct atbm_minstrel_ht_sta *mi,
                    int group, int rate)
{
	struct atbm_minstrel_rate_stats *mr;
	unsigned int usecs=0;

	mr = &mi->groups[group].rates[rate];

	if (mr->probability < MINSTREL_FRAC(1, 10)) {
		mr->cur_tp = 0;
		return;
	}
	if( MINSTREL_TRUNC(mi->avg_ampdu_len) !=0)	{
		usecs = mi->overhead / MINSTREL_TRUNC(mi->avg_ampdu_len);
	}
	usecs += atbm_minstrel_mcs_groups[group].duration[rate];
	
	if(usecs)
		mr->cur_tp = MINSTREL_TRUNC((1000000 / usecs) * mr->probability);
	else
		mr->cur_tp = 0;
}

/*
 * Update rate statistics and select new primary rates
 *
 * Rules for rate selection:
 *  - max_prob_rate must use only one stream, as a tradeoff between delivery
 *    probability and throughput during strong fluctuations
 *  - as long as the max prob rate has a probability of more than 3/4, pick
 *    higher throughput rates, even if the probablity is a bit lower
 */
static atbm_void
atbm_minstrel_ht_update_stats(struct atbmwifi_minstrel_priv *mp, struct atbm_minstrel_ht_sta *mi)
{

	struct atbm_minstrel_mcs_group_data *mg;
	struct atbm_minstrel_rate_stats *mr;
	int cur_prob, cur_prob_tp, cur_tp, cur_tp2;
	int group, i, index;

	if (mi->ampdu_packets > 0) {
		mi->avg_ampdu_len = atbm_minstrel_ewma(mi->avg_ampdu_len,
			MINSTREL_FRAC(mi->ampdu_len, mi->ampdu_packets), EWMA_LEVEL);
		mi->ampdu_len = 0;
		mi->ampdu_packets = 0;
	}

	mi->sample_slow = 0;
	mi->sample_count = 0;
	mi->max_tp_rate = 0;
	mi->max_tp_rate2 = 0;
	mi->max_prob_rate = 0;

	for (group = 0; group < ATBM_ARRAY_SIZE(atbm_minstrel_mcs_groups); group++) {
		cur_prob = 0;
		cur_prob_tp = 0;
		cur_tp = 0;
		cur_tp2 = 0;

		mg = &mi->groups[group];
		if (!mg->supported)
			continue;

		mg->max_tp_rate = 0;
		mg->max_tp_rate2 = 0;
		mg->max_prob_rate = 0;
		mi->sample_count++;

		for (i = 0; i < MCS_GROUP_RATES; i++) {
			if (!(mg->supported & BIT(i)))
				continue;

			mr = &mg->rates[i];
			mr->retry_updated = ATBM_FALSE;
			index = MCS_GROUP_RATES * group + i;
			atbm_minstrel_calc_rate_ewma(mp, mr);
			atbm_minstrel_ht_calc_tp(mp, mi, group, i);

			if (!mr->cur_tp)
				continue;

			/* ignore the lowest rate of each single-stream group */
			if (!i && atbm_minstrel_mcs_groups[group].streams == 1)
				continue;

			if ((mr->cur_tp > cur_prob_tp && mr->probability >
			     MINSTREL_FRAC(3, 4)) || mr->probability > cur_prob) {
				mg->max_prob_rate = index;
				cur_prob = mr->probability;
				cur_prob_tp = mr->cur_tp;
			}

			if (mr->cur_tp > cur_tp) {
				atbm_swap(index, mg->max_tp_rate);
				cur_tp = mr->cur_tp;
				mr = atbm_minstrel_get_ratestats(mi, index);
			}

			if (index >= mg->max_tp_rate)
				continue;

			if (mr->cur_tp > cur_tp2) {
				mg->max_tp_rate2 = index;
				cur_tp2 = mr->cur_tp;
			}
		}
	}

	/* try to sample up to half of the available rates during each interval */
	mi->sample_count *= 4;

	cur_prob = 0;
	cur_prob_tp = 0;
	cur_tp = 0;
	cur_tp2 = 0;
	for (group = 0; group < ATBM_ARRAY_SIZE(atbm_minstrel_mcs_groups); group++) {
		mg = &mi->groups[group];
		if (!mg->supported)
			continue;

		mr = atbm_minstrel_get_ratestats(mi, mg->max_prob_rate);
		if (cur_prob_tp < mr->cur_tp &&
		    atbm_minstrel_mcs_groups[group].streams == 1) {
			mi->max_prob_rate = mg->max_prob_rate;
			cur_prob = mr->cur_prob;
			cur_prob_tp = mr->cur_tp;
		}

		mr = atbm_minstrel_get_ratestats(mi, mg->max_tp_rate);
		if (cur_tp < mr->cur_tp) {
			mi->max_tp_rate2 = mi->max_tp_rate;
			cur_tp2 = cur_tp;
			mi->max_tp_rate = mg->max_tp_rate;
			cur_tp = mr->cur_tp;
		}

		mr = atbm_minstrel_get_ratestats(mi, mg->max_tp_rate2);
		if (cur_tp2 < mr->cur_tp) {
			mi->max_tp_rate2 = mg->max_tp_rate2;
			cur_tp2 = mr->cur_tp;
		}
	}
	mi->stats_update = atbm_GetOsTimeMs();
}

static ATBM_BOOL
atbm_minstrel_ht_txstat_valid(struct atbmwifi_ieee80211_tx_rate *rate)
{
	if (!rate->count)
		return ATBM_FALSE;

	if (rate->idx < 0)
		return ATBM_FALSE;

	return !!(rate->flags & IEEE80211_TX_RC_MCS);
}

static atbm_void
atbm_minstrel_next_sample_idx(struct atbm_minstrel_ht_sta *mi)
{
	struct atbm_minstrel_mcs_group_data *mg;

	for (;;) {
		mi->sample_group++;
		mi->sample_group %= ATBM_ARRAY_SIZE(atbm_minstrel_mcs_groups);
		mg = &mi->groups[mi->sample_group];

		if (!mg->supported)
			continue;

		if (++mg->index >= MCS_GROUP_RATES) {
			mg->index = 0;
			if (++mg->column >= ATBM_ARRAY_SIZE(sample_table))
				mg->column = 0;
		}
		break;
	}
}

static atbm_void
atbm_minstrel_downgrade_rate(struct atbm_minstrel_ht_sta *mi, unsigned int *idx,
			ATBM_BOOL primary)
{
	int group, orig_group;

	orig_group = group = *idx / MCS_GROUP_RATES;
	while (group > 0) {
		group--;

		if (!mi->groups[group].supported)
			continue;

		if (atbm_minstrel_mcs_groups[group].streams >
		    atbm_minstrel_mcs_groups[orig_group].streams)
			continue;

		if (primary)
			*idx = mi->groups[group].max_tp_rate;
		else
			*idx = mi->groups[group].max_tp_rate2;
		break;
	}
}

static atbm_void
atbm_minstrel_ht_tx_status(struct atbmwifi_cfg80211_rate  *sta, atbm_void *priv_sta,struct atbm_buff *skb)
{
	struct atbm_minstrel_ht_sta_priv *msp = priv_sta;
	struct atbm_minstrel_ht_sta *mi = &msp->ht;
	struct atbmwifi_ieee80211_tx_info *tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	struct atbmwifi_ieee80211_tx_rate *ar = tx_info->control.rates;
	struct atbm_minstrel_rate_stats *rate, *rate2;
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	ATBM_BOOL last = ATBM_FALSE;
	int group=0;
	int i = 0;
	if (!msp->is_ht)
		return mac80211_ratectrl_minstrel.tx_status(sta, priv_sta,skb);

	/* This packet was aggregated but doesn't carry status info */
	if ((tx_info->flags & ATBM_IEEE80211_TX_CTL_AMPDU) &&
	    !(tx_info->flags & ATBM_IEEE80211_TX_STAT_AMPDU))
		return;
	/*because Ampdu is done by lmac,so ampdu_len is 1,as an mpdu */
	if (!(tx_info->flags & ATBM_IEEE80211_TX_STAT_AMPDU)) {
		tx_info->control.ampdu_ack_len =
			(tx_info->flags & ATBM_IEEE80211_TX_STAT_ACK ? 1 : 0);
		tx_info->control.ampdu_len = 1;
	}
	mi->ampdu_packets++;
	mi->ampdu_len += tx_info->control.ampdu_len;
	if (!mi->sample_wait && !mi->sample_tries && mi->sample_count > 0) {
		mi->sample_wait = 16 + 2 * MINSTREL_TRUNC(mi->avg_ampdu_len);
		mi->sample_tries = 2;
		mi->sample_count--;
	}

	if (tx_info->flags & ATBM_IEEE80211_TX_CTL_RATE_CTRL_PROBE)
		mi->sample_packets += tx_info->control.ampdu_len;

	for (i = 0; !last; i++) {
		last = (i == ATBM_IEEE80211_TX_STAT_ACK - 1) ||
		       !atbm_minstrel_ht_txstat_valid(&ar[i + 1]);
		if (!atbm_minstrel_ht_txstat_valid(&ar[i]))
			break;

		group = atbm_minstrel_ht_get_group_idx(&ar[i]);
		rate = &mi->groups[group].rates[ar[i].idx % MCS_GROUP_RATES];
		if (last){
			rate->success += tx_info->control.ampdu_ack_len;
		}
		/*ar[i].count will calc*/
		rate->attempts += ar[i].count * tx_info->control.ampdu_len;
		//wifi_printk(WIFI_CONNECT,"ar[%d].count=%d\n",i,ar[i].count);
	}
	/*
	 * check for sudden death of spatial multiplexing,
	 * downgrade to a lower number of streams if necessary.
	 */
	// wifi_printk(WIFI_CONNECT,"GetRate1----->max_tp_rate=%d\n",mi->max_tp_rate);
	rate = atbm_minstrel_get_ratestats(mi, mi->max_tp_rate);
	if (rate->attempts > 30 &&
	    MINSTREL_FRAC(rate->success, rate->attempts) <
	    MINSTREL_FRAC(20, 100)){
		atbm_minstrel_downgrade_rate(mi, &mi->max_tp_rate, ATBM_TRUE);
	}
	//wifi_printk(WIFI_CONNECT,"GetRate2----->max_tp_rate2=%d\n",mi->max_tp_rate2);
	rate2 = atbm_minstrel_get_ratestats(mi, mi->max_tp_rate2);
	if (rate2->attempts > 30 &&
	    MINSTREL_FRAC(rate2->success, rate2->attempts) <
	    MINSTREL_FRAC(20, 100))
		atbm_minstrel_downgrade_rate(mi, &mi->max_tp_rate2, ATBM_FALSE);
	/*update Rate stats after 10s*/
	if(!atbm_TimeAfter(mi->stats_update, mp->update_interval*200/2)) {
		atbm_minstrel_ht_update_stats(mp, mi);
	}
}
static atbm_void
atbm_minstrel_calc_retransmit(struct atbmwifi_minstrel_priv *mp, struct atbm_minstrel_ht_sta *mi,
                         int index)
{
	struct atbm_minstrel_rate_stats *mr;
	const struct atbm_mcs_group *group;
	unsigned int tx_time, tx_time_rtscts, tx_time_data;
	unsigned int cw = mp->cw_min;
	unsigned int ctime = 0;
	unsigned int t_slot = 9; /* FIXME */
	unsigned int ampdu_len = MINSTREL_TRUNC(mi->avg_ampdu_len);

	mr = atbm_minstrel_get_ratestats(mi, index);
	if (mr->probability < MINSTREL_FRAC(1, 10)) {
		mr->retry_count = 1;
		mr->retry_count_rtscts = 1;
		return;
	}

	mr->retry_count = 2;
	mr->retry_count_rtscts = 2;
	mr->retry_updated = ATBM_TRUE;

	group = &atbm_minstrel_mcs_groups[index / MCS_GROUP_RATES];
	tx_time_data = group->duration[index % MCS_GROUP_RATES] * ampdu_len;

	/* Contention time for first 2 tries */
	ctime = (t_slot * cw) >> 1;
	cw = atbm_min((cw << 1) | 1, mp->cw_max);
	ctime += (t_slot * cw) >> 1;
	cw = atbm_min((cw << 1) | 1, mp->cw_max);

	/* Total TX time for data and Contention after first 2 tries */
	tx_time = ctime + 2 * (mi->overhead + tx_time_data);
	tx_time_rtscts = ctime + 2 * (mi->overhead_rtscts + tx_time_data);

	/* See how many more tries we can fit inside segment size */
	do {
		/* Contention time for this try */
		ctime = (t_slot * cw) >> 1;
		cw = atbm_min((cw << 1) | 1, mp->cw_max);

		/* Total TX time after this try */
		tx_time += ctime + mi->overhead + tx_time_data;
		tx_time_rtscts += ctime + mi->overhead_rtscts + tx_time_data;

		if (tx_time_rtscts < mp->segment_size)
			mr->retry_count_rtscts++;
	} while ((tx_time < mp->segment_size) &&
	         (++mr->retry_count < mp->max_retry));
}


static atbm_void
atbm_minstrel_ht_set_rate(struct atbmwifi_minstrel_priv *mp, struct atbm_minstrel_ht_sta *mi,
                     struct atbmwifi_ieee80211_tx_rate *rate, int index,
					 ATBM_BOOL sample, ATBM_BOOL rtscts)
{
	const struct atbm_mcs_group *group = &atbm_minstrel_mcs_groups[index / MCS_GROUP_RATES];
	struct atbm_minstrel_rate_stats *mr;
	mr = atbm_minstrel_get_ratestats(mi, index);
	if (!mr->retry_updated)
		atbm_minstrel_calc_retransmit(mp, mi, index);

	if (sample)
		rate->count = 1;
	else if (mr->probability < MINSTREL_FRAC(20, 100))
		rate->count = 2;
	else if (rtscts)
		rate->count = mr->retry_count_rtscts;
	else
		rate->count = mr->retry_count;

	rate->flags = IEEE80211_TX_RC_MCS | group->flags;
	if (rtscts)
		rate->flags |= ATBM_IEEE80211_TX_RC_USE_RTS_CTS;
	rate->idx = index % MCS_GROUP_RATES + (group->streams - 1) * MCS_GROUP_RATES;
	//wifi_printk(WIFI_CONNECT,"rate->idx=%d,index=%d,streams=%d,Groupflags=%x,rateFlag=%x,RateCnt=%d\n",rate->idx,index,group->streams,group->flags,rate->flags,rate->count);
}
int atbm_minstrel_get_duration(int index)
{
	const struct atbm_mcs_group *group = &atbm_minstrel_mcs_groups[index / MCS_GROUP_RATES];
	return group->duration[index % MCS_GROUP_RATES];
}

static int
atbm_minstrel_get_sample_rate(struct atbmwifi_minstrel_priv *mp, struct atbm_minstrel_ht_sta *mi)
{
	struct atbm_minstrel_rate_stats *mr;
	struct atbm_minstrel_mcs_group_data *mg;
	int sample_idx = 0;

	if (mi->sample_wait > 0) {
		mi->sample_wait--;
		return -1;
	}
	if (!mi->sample_tries)
		return -1;
	mi->sample_tries--;
	mg = &mi->groups[mi->sample_group];
	/*Get current sample Rate*/
	sample_idx = sample_table[mg->column][mg->index];
	/*Cal mg->column & mg->index,in order to Get next sample Rate*/
	mr = &mg->rates[sample_idx];
	sample_idx += mi->sample_group * MCS_GROUP_RATES;
	atbm_minstrel_next_sample_idx(mi);

	/*
	 * When not using MRR, do not sample if the probability is already
	 * higher than 95% to avoid wasting airtime
	 */
	if (!mp->has_mrr && (mr->probability > MINSTREL_FRAC(95, 100)))
		return -1;

	/*
	 * Make sure that lower rates get sampled only occasionally,
	 * if the link is working perfectly.
	 */
	if (atbm_minstrel_get_duration(sample_idx) >
	    atbm_minstrel_get_duration(mi->max_tp_rate)) {
		if (mr->sample_skipped < 20)
			return -1;

		if (mi->sample_slow++ > 2)
			return -1;
	}
	return sample_idx;
}

static atbm_void
atbm_minstrel_ht_get_rate(struct atbmwifi_cfg80211_rate  *sta,
			  atbm_void *priv_sta,struct atbmwifi_ieee80211_tx_info *tx_info,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_tx_rate *ar = tx_info->control.rates;
	struct atbm_minstrel_ht_sta_priv *msp = priv_sta;
	struct atbm_minstrel_ht_sta *mi = &msp->ht;
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	int sample_idx;
	ATBM_BOOL sample = ATBM_FALSE;

	if (atbmwifi_rate_control_send_low(sta,skb)){
		return;
	}
	if (!msp->is_ht){
		return mac80211_ratectrl_minstrel.get_rate(sta,priv_sta,tx_info,skb);
	}
	//wifi_printk(WIFI_CONNECT,"GetRate--->mi->tx_flags=%x\n",mi->tx_flags);
	tx_info->flags |= mi->tx_flags;
	/*Get sample_idx */
	sample_idx = atbm_minstrel_get_sample_rate(mp, mi);
	/*if sample_idx >0,means it needs sample Rate,
		oherwise it needs control[1]& control[2]
	*/
	//wifi_printk(WIFI_CONNECT,"sample_idx=%d\n",sample_idx);
	if (sample_idx >= 0) {
		sample = ATBM_TRUE;
		atbm_minstrel_ht_set_rate(mp, mi, &ar[0], sample_idx,ATBM_TRUE, ATBM_FALSE);
		tx_info->flags |= ATBM_IEEE80211_TX_CTL_RATE_CTRL_PROBE;
	} else {
		atbm_minstrel_ht_set_rate(mp, mi, &ar[0], mi->max_tp_rate,ATBM_FALSE, ATBM_FALSE);
	}
	//wifi_printk(WIFI_CONNECT,"GetRate12--->mi->tx_flags=%x,mi->max_tp_rate=%d\n",mi->tx_flags,mi->max_tp_rate);
	/*
	 * At least 3 tx rates supported, use
	 * sample_rate -> max_tp_rate -> max_prob_rate for sampling and
	 * max_tp_rate -> max_tp_rate2 -> max_prob_rate by default.
	 */
	if (sample_idx >= 0){
		atbm_minstrel_ht_set_rate(mp, mi, &ar[1], mi->max_tp_rate,ATBM_FALSE, ATBM_FALSE);
	}else{
		atbm_minstrel_ht_set_rate(mp, mi, &ar[1], mi->max_tp_rate2,ATBM_FALSE, ATBM_TRUE);
	}
	//wifi_printk(WIFI_CONNECT,"GetRate13--->mi->tx_flags=%x,mi->max_tp_rate2=%d\n",mi->tx_flags,mi->max_tp_rate2);
	atbm_minstrel_ht_set_rate(mp, mi, &ar[2], mi->max_prob_rate,ATBM_FALSE, !sample);
	//wifi_printk(WIFI_CONNECT,"GetRate14--->mi->tx_flags=%x,mi->max_prob_rate=%d\n",mi->tx_flags,mi->max_prob_rate);

	ar[3].count = 0;
	ar[3].idx = -1;

	mi->total_packets++;

	/* wraparound */
	if (mi->total_packets == ~0) {
		mi->total_packets = 0;
		mi->sample_packets = 0;
	}
	//wifi_printk(WIFI_CONNECT,"GetRate13--->mi->tx_flags=%x\n",mi->tx_flags);
}
static atbm_void
atbm_minstrel_ht_update_caps(struct atbmwifi_cfg80211_rate *sta,atbm_void *priv_sta)
{
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	struct atbm_minstrel_ht_sta_priv *msp = priv_sta;
	struct atbm_minstrel_ht_sta *mi = &msp->ht;
	struct atbmwifi_ieee80211_mcs_info *mcs = &sta->ht_cap.mcs;
	struct atbmwifi_ieee80211_supported_band *sband=&atbmwifi_band_2ghz;
	atbm_uint16 sta_cap=sta->ht_cap.cap;
	int n_supported = 0;
	int ack_dur;
	int stbc;
	int i;
	/* fall back to the old minstrel for legacy stations */
	if (!sta->ht_cap.ht_supported){
		goto use_legacy;
	}
	//wifi_printk(WIFI_CONNECT,"update_caps-->ht_cap->cap=%x\n",sta_cap);
	msp->is_ht = ATBM_TRUE;
	atbm_memset(mi, 0, sizeof(*mi));
	mi->stats_update = atbm_GetOsTimeMs();

	ack_dur = atbmwifi_ieee80211_frame_duration(10, 60, 1, 1);
	mi->overhead = atbmwifi_ieee80211_frame_duration(0, 60, 1, 1) + ack_dur;
	mi->overhead_rtscts = mi->overhead + 2 * ack_dur;

	mi->avg_ampdu_len = MINSTREL_FRAC(1, 1);

	/* When using MRR, sample more on the first attempt, without delay */
	if (mp->has_mrr) {
		mi->sample_count = 16;
		mi->sample_wait = 0;
	} else {
		mi->sample_count = 8;
		mi->sample_wait = 8;
	}
	mi->sample_tries = 4;

	stbc = (sta_cap & IEEE80211_HT_CAP_RX_STBC) >>
		ATBM_IEEE80211_HT_CAP_RX_STBC_SHIFT;
	mi->tx_flags |= stbc << IEEE80211_TX_CTL_STBC_SHIFT;

	if (sta_cap & IEEE80211_HT_CAP_LDPC_CODING)
		mi->tx_flags |= IEEE80211_TX_CTL_LDPC;

	if (sta->channel_type != ATBM_NL80211_CHAN_HT40MINUS &&
	    sta->channel_type != ATBM_NL80211_CHAN_HT40PLUS)
	{
		sta_cap &= ~ATBM_IEEE80211_HT_CAP_SUP_WIDTH_20_40;
	}
	for (i = 0; i < ATBM_ARRAY_SIZE(mi->groups); i++) {
		atbm_uint16 req = 0;

		mi->groups[i].supported = 0;
		//wifi_printk(WIFI_CONNECT,"1-->atbm_minstrel_mcs_groups[%d].flags=%x,req=%x,sta_cap=%x\n",i,atbm_minstrel_mcs_groups[i].flags,req,sta_cap);
		if (atbm_minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_SHORT_GI) {
			if (atbm_minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH){
				req |= ATBM_IEEE80211_HT_CAP_SGI_40;
			}else{
				req |= ATBM_IEEE80211_HT_CAP_SGI_20;
			}
		}

		if (atbm_minstrel_mcs_groups[i].flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
			req |= ATBM_IEEE80211_HT_CAP_SUP_WIDTH_20_40;

		if ((sta_cap & req) != req){
			continue;
		}

		mi->groups[i].supported =
			mcs->rx_mask[atbm_minstrel_mcs_groups[i].streams - 1];
		if (mi->groups[i].supported)
			n_supported++;
	}
	if (!n_supported)
		goto use_legacy;
	return;
use_legacy:
	msp->is_ht = ATBM_FALSE;
	atbm_memset(&msp->legacy, 0, sizeof(msp->legacy));
	msp->legacy.r = msp->ratelist;
	msp->legacy.sample_table = msp->sample_table;
    mac80211_ratectrl_minstrel.sta_rate_init(sta,priv_sta);
	return;
}

static atbm_void
atbm_minstrel_ht_rate_init(struct atbmwifi_cfg80211_bss *sta,atbm_void *priv_sta)
{	
	atbm_minstrel_ht_update_caps(sta,priv_sta);
}
static atbm_void
atbm_minstrel_ht_rate_update(struct atbmwifi_cfg80211_bss  *sta, atbm_void *priv_sta)
{
	atbm_minstrel_ht_update_caps(sta,priv_sta);
}

static atbm_void *
atbm_minstrel_ht_alloc_sta(atbm_void)
{
	struct atbm_minstrel_ht_sta_priv *msp;
	int max_rates = 0;
	int i;
	max_rates = atbmwifi_band_2ghz.n_bitrates;

	msp = (struct atbm_minstrel_ht_sta_priv *)atbm_kmalloc(sizeof(struct atbm_minstrel_ht_sta_priv),GFP_KERNEL);
	if (!msp)
		return ATBM_NULL;

	msp->ratelist = (atbm_void *)atbm_kmalloc(sizeof(struct minstrel_rate) * max_rates,GFP_KERNEL);
	if (!msp->ratelist)
		goto error;
	msp->sample_table = (atbm_void *)atbm_kmalloc(SAMPLE_COLUMNS * max_rates,GFP_KERNEL);
	if (!msp->sample_table)
		goto error1;

	return msp;

error1:
	atbm_kfree(msp->ratelist);
error:
	atbm_kfree(msp);
	return ATBM_NULL;
}

static atbm_void
atbm_minstrel_ht_free_sta(atbm_void *priv_sta)
{
	struct atbm_minstrel_ht_sta_priv *msp = priv_sta;
	atbm_kfree(msp->sample_table);
	atbm_kfree(msp->ratelist);
	atbm_kfree(msp);
}

static atbm_void
atbm_minstrel_ht_alloc(atbm_void)
{
	mac80211_ratectrl_minstrel.alloc(atbm_void);
}

static atbm_void
atbm_minstrel_ht_free(atbm_void *priv)
{
	mac80211_ratectrl_minstrel.free(priv);
}
struct atbm_mcs_group atbm_minstrel_mcs_groups[4];
static atbm_void atbm_init_group_var()
{
	atbm_minstrel_mcs_groups[0].flags=0;//no shortGi & no Ht40
	atbm_minstrel_mcs_groups[0].streams=1;
	atbm_minstrel_mcs_groups[0].duration[0]=MCS_DURATION(1,0,26);
	atbm_minstrel_mcs_groups[0].duration[1]=MCS_DURATION(1,0,52);
	atbm_minstrel_mcs_groups[0].duration[2]=MCS_DURATION(1,0,78);
	atbm_minstrel_mcs_groups[0].duration[3]=MCS_DURATION(1,0,104);
	atbm_minstrel_mcs_groups[0].duration[4]=MCS_DURATION(1,0,156);
	atbm_minstrel_mcs_groups[0].duration[5]=MCS_DURATION(1,0,208);
	atbm_minstrel_mcs_groups[0].duration[6]=MCS_DURATION(1,0,234);
	atbm_minstrel_mcs_groups[0].duration[7]=MCS_DURATION(1,0,260);
	atbm_minstrel_mcs_groups[1].flags=IEEE80211_TX_RC_SHORT_GI; //shortGi
	atbm_minstrel_mcs_groups[1].streams=1;
	atbm_minstrel_mcs_groups[1].duration[0]=MCS_DURATION(1,1,26);
	atbm_minstrel_mcs_groups[1].duration[1]=MCS_DURATION(1,1,52);
	atbm_minstrel_mcs_groups[1].duration[2]=MCS_DURATION(1,1,78);
	atbm_minstrel_mcs_groups[1].duration[3]=MCS_DURATION(1,1,104);
	atbm_minstrel_mcs_groups[1].duration[4]=MCS_DURATION(1,1,156);
	atbm_minstrel_mcs_groups[1].duration[5]=MCS_DURATION(1,1,208);
	atbm_minstrel_mcs_groups[1].duration[6]=MCS_DURATION(1,1,234);
	atbm_minstrel_mcs_groups[1].duration[7]=MCS_DURATION(1,1,260);
	atbm_minstrel_mcs_groups[2].flags=IEEE80211_TX_RC_40_MHZ_WIDTH;//ht40
	atbm_minstrel_mcs_groups[2].streams=1;
	atbm_minstrel_mcs_groups[2].duration[0]=MCS_DURATION(1,0,54);
	atbm_minstrel_mcs_groups[2].duration[1]=MCS_DURATION(1,0,108);
	atbm_minstrel_mcs_groups[2].duration[2]=MCS_DURATION(1,0,162);
	atbm_minstrel_mcs_groups[2].duration[3]=MCS_DURATION(1,0,216);
	atbm_minstrel_mcs_groups[2].duration[4]=MCS_DURATION(1,0,324);
	atbm_minstrel_mcs_groups[2].duration[5]=MCS_DURATION(1,0,432);
	atbm_minstrel_mcs_groups[2].duration[6]=MCS_DURATION(1,0,486);
	atbm_minstrel_mcs_groups[2].duration[7]=MCS_DURATION(1,0,540);
	atbm_minstrel_mcs_groups[3].flags=IEEE80211_TX_RC_40_MHZ_WIDTH|IEEE80211_TX_RC_SHORT_GI;//shortGi&ht40
	atbm_minstrel_mcs_groups[3].streams=1;
	atbm_minstrel_mcs_groups[3].duration[0]=MCS_DURATION(1,1,54);
	atbm_minstrel_mcs_groups[3].duration[1]=MCS_DURATION(1,1,108);
	atbm_minstrel_mcs_groups[3].duration[2]=MCS_DURATION(1,1,162);
	atbm_minstrel_mcs_groups[3].duration[3]=MCS_DURATION(1,1,216);
	atbm_minstrel_mcs_groups[3].duration[4]=MCS_DURATION(1,1,324);
	atbm_minstrel_mcs_groups[3].duration[5]=MCS_DURATION(1,1,432);
	atbm_minstrel_mcs_groups[3].duration[6]=MCS_DURATION(1,1,486);
	atbm_minstrel_mcs_groups[3].duration[7]=MCS_DURATION(1,1,540);
}
int atbm_get_random_bytes()
{
	static int seed = 0x123456;

	seed =  seed*0x33333 + 1234 - seed;
	seed = seed/325 *seed;
	seed = seed>>2;
	seed &= 0x20;
	return seed;
}

atbm_void atbm_init_sample_ht_table(atbm_void)
{
	int col, i, new_idx;
	atbm_uint8 rnd[MCS_GROUP_RATES];

	atbm_memset(sample_table, 0xff, sizeof(sample_table));
	for (col = 0; col < SAMPLE_COLUMNS; col++) {
		for (i = 0; i < MCS_GROUP_RATES; i++) {
			rnd[i]=atbm_get_random_bytes();
			new_idx = (i + rnd[i]) % MCS_GROUP_RATES;

			while (sample_table[col][new_idx] != 0xff)
				new_idx = (new_idx + 1) % MCS_GROUP_RATES;//1,2,3,4,5,6,7,8

			sample_table[col][new_idx] = i;
			/*Here is an arrary which is s[0][0]=0....7,s[0][1]=0......7,,,,s[0][7]=0.....7
										 s[1][0]=0....7,s[1][1]=0......7,,,,s[1][7]=0.....7
										  .........
										  .........
								 		 s[3][0]=0....7,s[3][1]=0......7,,,,s[3][7]=0.....7
			*/
		}
	}
}

 struct rate_control_ops * atbm_rate_control_minstrel_ht_init(atbm_void)
{			

	atbm_init_group_var();
	/*minstrel Ht sample Rate init*/
	atbm_init_sample_ht_table();
	/*ministrel rateCtrol init*/
	atbm_rate_control_minstrel_init();

	
	mac80211_ratectrl_minstrel_ht.tx_status = atbm_minstrel_ht_tx_status;
	mac80211_ratectrl_minstrel_ht.get_rate =  atbm_minstrel_ht_get_rate;
	mac80211_ratectrl_minstrel_ht.sta_rate_init = atbm_minstrel_ht_rate_init;
	mac80211_ratectrl_minstrel_ht.rate_update = atbm_minstrel_ht_rate_update,
	mac80211_ratectrl_minstrel_ht.alloc_sta = 	 atbm_minstrel_ht_alloc_sta;
	mac80211_ratectrl_minstrel_ht.free_sta =    atbm_minstrel_ht_free_sta;
	mac80211_ratectrl_minstrel_ht.alloc = atbm_minstrel_ht_alloc;
	mac80211_ratectrl_minstrel_ht.free =  atbm_minstrel_ht_free;	
	//
	mac80211_ratectrl_minstrel_ht.alloc(atbm_void);
	return &mac80211_ratectrl_minstrel_ht;
}



atbm_void atbmwifi_rate_control_get_rate(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t)
{
	int i;
	struct atbmwifi_ieee80211_tx_info *tx_info = t->tx_info;

	for (i = 0; i < ATBM_IEEE80211_TX_STAT_ACK; i++) {
		tx_info->control.rates[i].idx = -1;
		tx_info->control.rates[i].flags = 0;
		tx_info->control.rates[i].count = 1;
	}

	if((priv->iftype == ATBM_NL80211_IFTYPE_AP)||(priv->iftype ==ATBM_NL80211_IFTYPE_P2P_GO)){
		if((t->sta_priv)&&(t->sta_priv->sta_rc_priv))
			mac80211_ratectrl_minstrel_ht.get_rate(&t->sta_priv->rate,t->sta_priv->sta_rc_priv,t->tx_info, t->skb);
		else{
			tx_info->flags |= IEEE80211_TX_CTL_USE_MINRATE;
		}
	}
	else {
		if(priv->bss.rc_priv)
			mac80211_ratectrl_minstrel_ht.get_rate(&priv->bss.sta_priv.rate,priv->bss.rc_priv,t->tx_info, t->skb);
		else {
			tx_info->flags |= IEEE80211_TX_CTL_USE_MINRATE;		
		}
	}

}
int atbmwifi_ieee80211_tx_h_rate_ctrl(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t)
{
	struct atbmwifi_ieee80211_tx_info *tx_info = t->tx_info;
	struct atbmwifi_ieee80211_supported_band *sband;
	struct atbmwifi_ieee80211_rate *rate;
	int i;
	atbm_uint32 len;
	ATBM_BOOL inval = ATBM_FALSE, rts = ATBM_FALSE, short_preamble = ATBM_FALSE;
	ATBM_BOOL assoc = ATBM_FALSE;
	sband = &atbmwifi_band_2ghz;
	/*
	 * If we're associated with the sta at this point we know we can at
	 * least send the frame at the lowest bit rate.
	 */
	atbmwifi_rate_control_get_rate(priv,t);
	if(tx_info->flags & IEEE80211_TX_CTL_USE_MINRATE){
		return 0;
	}
	if (atbm_unlikely(tx_info->control.rates[0].idx < 0)){
		wifi_printk(WIFI_CONNECT,"Drop =%d\n",__LINE__);
		return 0;
	}
	if ((!tx_info->control.rates[0].count)){
		tx_info->control.rates[0].count = 1;
	}
	if (((tx_info->control.rates[0].count > 1) &&
			 (tx_info->flags & IEEE80211_TX_CTL_NO_ACK)))
		tx_info->control.rates[0].count = 1;

	if (!(tx_info->control.rates[0].flags & IEEE80211_TX_RC_MCS)) {
		atbm_int8 baserate = 0;
		wifi_printk(WIFI_RATE,"NO 11n rate\n");
		rate = &sband->bitrates[tx_info->control.rates[0].idx];
		for (i = 0; i < sband->n_bitrates; i++) {
			/* must be a basic rate */
			if (!(priv->bss.sta_priv.rate.basic_rates & BIT(i)))
				continue;
			/* must not be faster than the data rate */
			if (sband->bitrates[i].bitrate > rate->bitrate)
				continue;
			/* maximum */
			if (sband->bitrates[baserate].bitrate <
			     sband->bitrates[i].bitrate)
				baserate = i;
		}
		tx_info->control.rts_cts_rate_idx = baserate;
	}

	for (i = 0; i < ATBM_IEEE80211_TX_STAT_ACK; i++) {
		if (inval) {
			tx_info->control.rates[i].idx = -1;
			continue;
		}
		if (tx_info->control.rates[i].idx < 0) {
			inval = ATBM_TRUE;
			continue;
		}
		//wifi_printk(WIFI_CONNECT,"txInfo_Idx[%d]=%d,count[%d]=%d,flags[%d]=%x\n",i,tx_info->control.rates[i].idx,i,tx_info->control.rates[i].count,i,tx_info->control.rates[i].flags);
		if (tx_info->control.rates[i].flags & IEEE80211_TX_RC_MCS) {
			ATBM_WARN_ON(tx_info->control.rates[i].idx > 76);
			continue;
		}

		/* set up RTS protection if desired */
		if (rts)
			tx_info->control.rates[i].flags |=
				ATBM_IEEE80211_TX_RC_USE_RTS_CTS;

		/* RC is busted */
		if ((tx_info->control.rates[i].idx >=
				 sband->n_bitrates)) {
			tx_info->control.rates[i].idx = -1;
			continue;
		}

		rate = &sband->bitrates[tx_info->control.rates[i].idx];

		/* set up short preamble */
		if (short_preamble &&
		    rate->rate_flag & IEEE80211_RT_11B_SHORT)
			tx_info->control.rates[i].flags |=
				IEEE80211_TX_RC_USE_SHORT_PREAMBLE;

		/* set up G protection */
		if (!rts && priv->bss.use_cts_prot &&
		    rate->rate_flag & IEEE80211_RATE_ERP_G)
			tx_info->control.rates[i].flags |=
				ATBM_IEEE80211_TX_RC_USE_CTS_PROTECT;
	}
	//atbmwifi_ieee80211_tx_h_rate_ctrl_cnt++;
	return 0;
	
}
int atbm_get_tx_hwrate(struct atbmwifi_common *hw_priv,struct atbmwifi_ieee80211_tx_rate *rate)
{
	struct atbmwifi_ieee80211_supported_band *sband=&atbmwifi_band_2ghz;
	if (rate->idx < 0)
		return ATBM_NULL;
	if (rate->flags & IEEE80211_TX_RC_MCS){
		return hw_priv->mcs_rates[rate->idx].hw_value;
	}else{
		return sband->bitrates[rate->idx].hw_value;
	}
}


static ATBM_BOOL atbm_tx_policy_is_equal(const struct tx_policy *wanted,
					const struct tx_policy *cached)
{
	int count = wanted->defined >> 1;
	if (wanted->defined > cached->defined)
		return ATBM_FALSE;
	if (count) {
		if (atbm_memcmp(wanted->raw, cached->raw, count))
			return ATBM_FALSE;
	}
	if (wanted->defined & 1) {
		if ((wanted->raw[count] & 0x0F) != (cached->raw[count] & 0x0F))
			return ATBM_FALSE;
	}
	return ATBM_TRUE;
}
static atbm_void tx_policy_dump(struct tx_policy *policy)
{
	wifi_printk(WIFI_CONNECT,"[TX policy] "
		"%.1X%.1X%.1X%.1X%.1X%.1X%.1X%.1X"
		"%.1X%.1X%.1X%.1X%.1X%.1X%.1X%.1X"
		"%.1X%.1X%.1X%.1X%.1X%.1X%.1X%.1X: %d\n",
		policy->raw[0] & 0x0F,  policy->raw[0] >> 4,
		policy->raw[1] & 0x0F,  policy->raw[1] >> 4,
		policy->raw[2] & 0x0F,  policy->raw[2] >> 4,
		policy->raw[3] & 0x0F,  policy->raw[3] >> 4,
		policy->raw[4] & 0x0F,  policy->raw[4] >> 4,
		policy->raw[5] & 0x0F,  policy->raw[5] >> 4,
		policy->raw[6] & 0x0F,  policy->raw[6] >> 4,
		policy->raw[7] & 0x0F,  policy->raw[7] >> 4,
		policy->raw[8] & 0x0F,  policy->raw[8] >> 4,
		policy->raw[9] & 0x0F,  policy->raw[9] >> 4,
		policy->raw[10] & 0x0F,  policy->raw[10] >> 4,
		policy->raw[11] & 0x0F,  policy->raw[11] >> 4,
		policy->defined);
}
static atbm_void atbm_tx_policy_build(const struct atbmwifi_common *hw_priv,
	/* [out] */ struct tx_policy *policy,
	struct atbmwifi_ieee80211_tx_rate *rates, int count)
{
	int i, j;
	unsigned limit = hw_priv->short_frame_max_tx_count;
	
	unsigned total = 0;
	if(rates[0].idx<0){
		wifi_printk(WIFI_CONNECT,"Invalid rateId\n");
	}
	
	atbm_memset(policy, 0, sizeof(*policy));
	/*I think It's no need to sort rates in descending order,
		because Ministrel/ministrel_ht will be sorted*/
	/* Sort rates in descending order. */
	for (i = 1; i < count; ++i) {
		if (rates[i].idx < 0) {
			count = i;
			break;
		}
		if (rates[i].idx > rates[i - 1].idx) {
			struct atbmwifi_ieee80211_tx_rate tmp = rates[i - 1];
			rates[i - 1] = rates[i];
			rates[i] = tmp;
		}
	}
	/* Eliminate duplicates. */
	total = rates[0].count;
	for (i = 0, j = 1; j < count; ++j) {
		if (rates[j].idx == rates[i].idx) {
			rates[i].count += rates[j].count;
		} else if (rates[j].idx > rates[i].idx) {
			break;
		} else {
			++i;
			if (i != j)
				rates[i] = rates[j];
		}
		total += rates[j].count;
	}
	count = i + 1;
	/* Re-fill policy trying to keep every requested rate and with
	 * respect to the global max tx retransmission count. */
	if (limit < count)
		limit = count;///
	if (total > limit) {
		for (i = 0; i < count; ++i) {
			int left = count - i - 1;
			if (rates[i].count > limit - left)
				rates[i].count = limit - left;
			limit -= rates[i].count;
		}
	}
	
	policy->defined = atbm_get_tx_hwrate(hw_priv, &rates[0])+ 1;	
	for (i = 0; i < count; ++i) {
		register unsigned rateid, off, shift, retries;

		rateid = atbm_get_tx_hwrate(hw_priv, &rates[i]);
		off = rateid >> 3;		/* eq. rateid / 8 */
		shift = (rateid & 0x07) << 2;	/* eq. (rateid % 8) * 4 */

		retries = rates[i].count;
		if (atbm_unlikely(retries > 0x0F))
			rates[i].count = retries = 0x0F;
		policy->tbl[off] |= atbm_cpu_to_le32(retries << shift);
		policy->retry_count += retries;
		//wifi_printk(WIFI_CONNECT,"policy->tbl[%d]=%x,rateid=%d\n",off,policy->tbl[off],rateid);
	}
}
static int atbm_tx_policy_find(struct tx_policy_cache *cache,
				const struct tx_policy *wanted)
{
	struct tx_policy_cache_entry *it;
	/* Search for policy in "used" list */	
	for (it = atbm_list_entry((&cache->used)->next, struct tx_policy_cache_entry, link); &(it)->link != (&cache->used);
		 it = atbm_list_entry((it)->link.next, struct tx_policy_cache_entry, link)){
		if (atbm_tx_policy_is_equal(wanted, &it->policy))
			return it - cache->cache;
	}
	/* Then - in "free list" */
	/* Search for policy in "used" list */	
	for (it = atbm_list_entry((&cache->free)->next, struct tx_policy_cache_entry, link); &(it)->link != (&cache->free);
		 it = atbm_list_entry((it)->link.next, struct tx_policy_cache_entry, link)){
		if (atbm_tx_policy_is_equal(wanted, &it->policy))
			return it - cache->cache;
	}
	return -1;

}
static atbm_void atbm_tx_policy_use(struct tx_policy_cache *cache,
				 struct tx_policy_cache_entry *entry)
{
	++entry->policy.usage_count;
	atbm_list_move(&entry->link, &cache->used);
}
int atbmwifi_tx_policy_get(struct atbmwifi_common *hw_priv,
		  struct atbmwifi_ieee80211_tx_rate *rates,
		  int count, ATBM_BOOL *renew)
{
	int idx;
	int i;
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	struct tx_policy wanted;
	atbm_tx_policy_build(hw_priv, &wanted, rates, count);
	atbm_spin_lock(&cache->lock);
	if (atbm_list_empty(&cache->free)){
		atbm_spin_unlock(&cache->lock);
		return ATBM_APOLLO_INVALID_RATE_ID;
	}
	idx = atbm_tx_policy_find(cache, &wanted);
	if (idx>=0){
		//wifi_printk(WIFI_CONNECT,"Used TX Policy:%d\n",idx);
		renew=ATBM_FALSE;
	}else{
		struct tx_policy_cache_entry *entry;
		*renew = ATBM_TRUE;
		entry = atbm_list_entry(cache->free.prev,
			struct tx_policy_cache_entry, link);
		entry->policy = wanted;
		idx = entry - cache->cache;
		//wifi_printk(WIFI_CONNECT,"NEW TX Policy:%d\n",idx);
		//tx_policy_dump(&entry->policy);
	}
	atbm_tx_policy_use(cache, &cache->cache[idx]);
	/*If the FreeRate policy link table is full,it Maybe disable txqueue,else continue txPacket*/
	if (atbm_unlikely(atbm_list_empty(&cache->free))) { 
		wifi_printk(WIFI_RATE,"Free RatePolicy list is full,need stop tx\n");
		/* Lock TX queues. */
		for(i=0;i<4;i++){
			atbmwifi_queue_lock(&hw_priv->tx_queue[i],g_vmac);
		}
	}
	atbm_spin_unlock(&cache->lock);
	return idx;
}
static int atbm_tx_policy_release(struct tx_policy_cache *cache,
				    struct tx_policy_cache_entry *entry)
{
	int ret = --entry->policy.usage_count;
	if (!ret)
		atbm_list_move(&entry->link, &cache->free);
	return ret;
}
atbm_void atbm_tx_policy_put(struct atbmwifi_common *hw_priv, int idx)
{
	int usage, locked;
	int i;
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	atbm_spin_lock(&cache->lock);
	locked = atbm_list_empty(&cache->free);/*If the cache->free is empty,It maybe unlocked*/
	usage = atbm_tx_policy_release(cache, &cache->cache[idx]);
	if (atbm_unlikely(locked) && !usage) {
		/* Unlock TX queues. */
		for(i=0;i<4;i++){
			atbmwifi_queue_unlock(&hw_priv->tx_queue[i],g_vmac);
		}
	}
	atbm_spin_unlock(&cache->lock);
}
int atbm_tx_policy_upload(struct atbmwifi_common *hw_priv)
{	
	struct tx_policy_cache *cache = &hw_priv->tx_policy_cache;
	int i;
	int if_id = 0;
	struct wsm_set_tx_rate_retry_policy arg;
	atbm_spin_lock(&cache->lock);
	arg.hdr.numTxRatePolicies=0;
	/* Upload only modified entries. */
	for (i = 0; i < TX_POLICY_CACHE_SIZE; ++i) {
		struct tx_policy *src = &cache->cache[i].policy;
		if (src->retry_count && !src->uploaded) {
			struct wsm_set_tx_rate_retry_policy_policy *dst =
				&arg.tbl[arg.hdr.numTxRatePolicies];
			dst->policyIndex = i;
			dst->shortRetryCount =
				hw_priv->short_frame_max_tx_count;
			dst->longRetryCount = hw_priv->long_frame_max_tx_count;

			/* BIT(2) - Terminate retries when Tx rate retry policy
			 *          finishes.
			 * BIT(3) - Count initial frame transmission as part of
			 *          rate retry counting but not as a retry
			 *          attempt */
			dst->policyFlags = BIT(2) | BIT(3);

			atbm_memcpy(dst->rateCountIndices, src->tbl,
					sizeof(dst->rateCountIndices));
			src->uploaded = 1;
			++arg.hdr.numTxRatePolicies;
		}
	}
	atbm_spin_unlock(&cache->lock);
	wifi_printk(WIFI_RATE,"Write Rate policy------\n");
	return wsm_set_tx_rate_retry_policy(hw_priv, &arg, if_id);

}
int atbm_policy_upload_work(struct atbm_work_struct *work)
{	
	int ret=0;
	struct atbmwifi_vif *priv=(struct atbmwifi_vif *)work;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	ret=atbm_tx_policy_upload(hw_priv);
	if(ret<0){
		wifi_printk(WIFI_RATE,"RatePolicy update Fail\n");
	}
	return ret;
}


#endif //0
