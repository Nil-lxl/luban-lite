#if MINSTREL_RATE_CONTROL

#include "atbm_ratectrl.h"
struct rate_control_ops mac80211_ratectrl_minstrel;
#define SAMPLE_TBL(_mi, _idx, _col) \
		_mi->sample_table[(_idx * SAMPLE_COLUMNS) + _col]
/* convert mac80211 rate index to local array index */
int rix_to_ndx(struct atbmwifi_minstrel_sta_info *mi, int rix)
{
	int i = rix;
	for (i = rix; i >= 0; i--)
		if (mi->r[i].rix == rix)
			break;
	return i;
}
static atbm_void
atbm_minstrel_update_stats(struct atbmwifi_minstrel_priv *mp, struct atbmwifi_minstrel_sta_info *mi)
{
	atbm_uint32 max_tp = 0, index_max_tp = 0, index_max_tp2 = 0;
	atbm_uint32 max_prob = 0, index_max_prob = 0;
	atbm_uint32 usecs;
	atbm_uint32 p;
	int i;

	mi->stats_update = atbm_GetOsTimeMs();
	for (i = 0; i < mi->n_rates; i++) {
		struct minstrel_rate *mr = &mi->r[i];

		usecs = mr->perfect_tx_time;
		if (!usecs)
			usecs = 1000000;

		/* To avoid rounding issues, probabilities scale from 0 (0%)
		 * to 18000 (100%) */
		if (mr->attempts) {
			p = (mr->success * 18000) / mr->attempts;
			mr->succ_hist += mr->success;
			mr->att_hist += mr->attempts;
			mr->cur_prob = p;
			p = ((p * (100 - mp->ewma_level)) + (mr->probability *
				mp->ewma_level)) / 100;
			mr->probability = p;
			mr->cur_tp = p * (1000000 / usecs);
		}

		mr->last_success = mr->success;
		mr->last_attempts = mr->attempts;
		mr->success = 0;
		mr->attempts = 0;

		/* Sample less often below the 10% chance of success.
		 * Sample less often above the 95% chance of success. */
		if ((mr->probability > 17100) || (mr->probability < 1800)) {
			mr->adjusted_retry_count = mr->retry_count >> 1;
			if (mr->adjusted_retry_count > 2)
				mr->adjusted_retry_count = 2;
			mr->sample_limit = 4;
		} else {
			mr->sample_limit = -1;
			mr->adjusted_retry_count = mr->retry_count;
		}
		if (!mr->adjusted_retry_count)
			mr->adjusted_retry_count = 2;
	}

	for (i = 0; i < mi->n_rates; i++) {
		struct minstrel_rate *mr = &mi->r[i];
		if (max_tp < mr->cur_tp) {
			index_max_tp = i;
			max_tp = mr->cur_tp;
		}
		if (max_prob < mr->probability) {
			index_max_prob = i;
			max_prob = mr->probability;
		}
	}

	max_tp = 0;
	for (i = 0; i < mi->n_rates; i++) {
		struct minstrel_rate *mr = &mi->r[i];

		if (i == index_max_tp)
			continue;

		if (max_tp < mr->cur_tp) {
			index_max_tp2 = i;
			max_tp = mr->cur_tp;
		}
	}
	mi->max_tp_rate = index_max_tp;
	mi->max_tp_rate2 = index_max_tp2;
	mi->max_prob_rate = index_max_prob;
}

static atbm_void
atbm_minstrel_tx_status(struct atbmwifi_cfg80211_bss  *sta, atbm_void *priv_sta,struct atbm_buff *skb)
{
	struct atbmwifi_minstrel_sta_info *mi = priv_sta;
	struct atbmwifi_ieee80211_tx_info *info = ATBM_IEEE80211_SKB_TXCB(skb);
	struct atbmwifi_ieee80211_tx_rate *ar = info->control.rates;
	int i, ndx;
	int success;

	success = !!(info->flags & ATBM_IEEE80211_TX_STAT_ACK);

	for (i = 0; i < ATBM_IEEE80211_TX_STAT_ACK; i++) {
		if (ar[i].idx < 0)
			break;

		ndx = rix_to_ndx(mi, ar[i].idx);
		if (ndx < 0)
			continue;

		mi->r[ndx].attempts += ar[i].count;

		if ((i != ATBM_IEEE80211_TX_STAT_ACK - 1) && (ar[i + 1].idx < 0))
			mi->r[ndx].success += success;
	}

	if ((info->flags & ATBM_IEEE80211_TX_CTL_RATE_CTRL_PROBE) && (i >= 0))
		mi->sample_count++;

	if (mi->sample_deferred > 0)
		mi->sample_deferred--;
}


int atbm_minstrel_get_retry_count(struct minstrel_rate *mr,
                         struct atbmwifi_ieee80211_tx_info *info)
{
	unsigned int retry = mr->adjusted_retry_count;

	if (info->control.rates[0].flags & ATBM_IEEE80211_TX_RC_USE_RTS_CTS)
		retry = atbm_max(2U, atbm_min(mr->retry_count_rtscts, retry));
	else if (info->control.rates[0].flags & ATBM_IEEE80211_TX_RC_USE_CTS_PROTECT)
		retry = atbm_max(2U, atbm_min(mr->retry_count_cts, retry));
	return retry;
}


static int
atbm_minstrel_get_next_sample(struct atbmwifi_minstrel_sta_info *mi)
{
	unsigned int sample_ndx;
	sample_ndx = SAMPLE_TBL(mi, mi->sample_idx, mi->sample_column);
	mi->sample_idx++;
	if ((int) mi->sample_idx > (mi->n_rates - 2)) {
		mi->sample_idx = 0;
		mi->sample_column++;
		if (mi->sample_column >= SAMPLE_COLUMNS)
			mi->sample_column = 0;
	}
	return sample_ndx;
}

static atbm_void
atbm_minstrel_get_rate(struct atbmwifi_cfg80211_rate  *sta,
			  atbm_void *priv_sta,struct atbmwifi_ieee80211_tx_info *info,struct atbm_buff *skb)
{
	//struct atbmwifi_ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct atbmwifi_minstrel_sta_info *mi = priv_sta;
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	struct atbmwifi_ieee80211_tx_rate *ar = info->control.rates;
	unsigned int ndx, sample_ndx = 0;
	ATBM_BOOL mrr;
	ATBM_BOOL sample_slower = ATBM_FALSE;
	ATBM_BOOL sample = ATBM_FALSE;
	int i, delta;
	int mrr_ndx[3];
	int sample_rate;

	if (atbmwifi_rate_control_send_low(sta,skb)){
		return;
	}
	mrr = mp->has_mrr;
	if(!atbm_TimeAfter(mi->stats_update, mp->update_interval*200/2)) {
		atbm_minstrel_update_stats(mp, mi);
	}
	ndx = mi->max_tp_rate;

	sample_rate = mp->lookaround_rate_mrr; //10%

	mi->packet_count++;
	delta = (mi->packet_count * sample_rate / 100) -
			(mi->sample_count + mi->sample_deferred / 2);

	/* delta > 0: sampling required */
	if ((delta > 0) && (mrr || !mi->prev_sample)) {
		struct minstrel_rate *msr;
		if (mi->packet_count >= 10000) {
			mi->sample_deferred = 0;
			mi->sample_count = 0;
			mi->packet_count = 0;
		} else if (delta > mi->n_rates * 2) {
			/* With multi-rate retry, not every planned sample
			 * attempt actually gets used, due to the way the retry
			 * chain is set up - [max_tp,sample,prob,lowest] for
			 * sample_rate < max_tp.
			 *
			 * If there's too much sampling backlog and the link
			 * starts getting worse, minstrel would start bursting
			 * out lots of sampling frames, which would result
			 * in a large throughput loss. */
			mi->sample_count += (delta - mi->n_rates * 2);  //sample_deferred, sample counter will not be changed,  will lead to burst sample if no count manual  change.
		}

		sample_ndx = atbm_minstrel_get_next_sample(mi);
		msr = &mi->r[sample_ndx];
		sample = ATBM_TRUE;
		sample_slower = mrr && (msr->perfect_tx_time >
			mi->r[ndx].perfect_tx_time);

		if (!sample_slower) {
			if (msr->sample_limit != 0) {
				ndx = sample_ndx;
				mi->sample_count++;
				if (msr->sample_limit > 0)
					msr->sample_limit--;
			} else {
				sample = ATBM_FALSE;
			}
		} else {
			/* Only use ATBM_IEEE80211_TX_CTL_RATE_CTRL_PROBE to mark
			 * packets that have the sampling rate deferred to the
			 * second MRR stage. Increase the sample counter only
			 * if the deferred sample rate was actually used.
			 * Use the sample_deferred counter to make sure that
			 * the sampling is not done in large bursts */
			info->flags |= ATBM_IEEE80211_TX_CTL_RATE_CTRL_PROBE;
			mi->sample_deferred++;
		}
	}
	mi->prev_sample = sample;

	/* If we're not using MRR and the sampling rate already
	 * has a probability of >95%, we shouldn't be attempting
	 * to use it, as this only wastes precious airtime */
	if (!mrr && sample && (mi->r[ndx].probability > 17100))
		ndx = mi->max_tp_rate;

	ar[0].idx = mi->r[ndx].rix;
	ar[0].count = atbm_minstrel_get_retry_count(&mi->r[ndx], info);

	if (!mrr) {
		if (!sample)
			ar[0].count = mp->max_retry;
		ar[1].idx = mi->lowest_rix;
		ar[1].count = mp->max_retry;
		return;
	}

	/* MRR setup */
	if (sample) {
		if (sample_slower)
			mrr_ndx[0] = sample_ndx;
		else
			mrr_ndx[0] = mi->max_tp_rate;
	} else {
		mrr_ndx[0] = mi->max_tp_rate2;
	}
	mrr_ndx[1] = mi->max_prob_rate;
	mrr_ndx[2] = 0;
	for (i = 1; i < 4; i++) {
		ar[i].idx = mi->r[mrr_ndx[i - 1]].rix;
		ar[i].count = mi->r[mrr_ndx[i - 1]].adjusted_retry_count;
	}
}


static atbm_void
atbm_calc_rate_durations(
		    struct minstrel_rate *d,
		    struct atbmwifi_ieee80211_rate *rate)
{
	int erp = !!(rate->rate_flag & IEEE80211_RATE_ERP_G);

	d->perfect_tx_time = atbmwifi_ieee80211_frame_duration(1200,
			rate->bitrate, erp, 1);
	d->ack_time = atbmwifi_ieee80211_frame_duration(10,
			rate->bitrate, erp, 1);
}
extern int atbm_get_random_bytes();
static atbm_void
atbm_init_sample_table(struct atbmwifi_minstrel_sta_info *mi)
{
	unsigned int i, col, new_idx;
	unsigned int n_srates = mi->n_rates - 1;
	atbm_uint8 rnd[8];

	mi->sample_column = 0;
	mi->sample_idx = 0;
	atbm_memset(mi->sample_table, 0, SAMPLE_COLUMNS * mi->n_rates);

	for (col = 0; col < SAMPLE_COLUMNS; col++) {
		for (i = 0; i < n_srates; i++) {
			rnd[i&7]=atbm_get_random_bytes();
			new_idx = (i + rnd[i & 7]) % n_srates;

			while (SAMPLE_TBL(mi, new_idx, col) != 0)
				new_idx = (new_idx + 1) % n_srates;
			
			/* Don't sample the slowest rate (i.e. slowest base
			 * rate). We must presume that the slowest rate works
			 * fine, or else other management frames will also be
			 * failing and the link will break */
			SAMPLE_TBL(mi, new_idx, col) = i + 1;
		}
	}
}

static atbm_void
atbm_minstrel_rate_init(struct atbmwifi_cfg80211_rate *sta, atbm_void *priv_sta)
{
	struct atbmwifi_minstrel_sta_info *mi = priv_sta;
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	struct atbmwifi_ieee80211_rate *ctl_rate;
	unsigned int i, n = 0;
	unsigned int t_slot = 9; /* FIXME: get real slot time */
	struct atbmwifi_ieee80211_supported_band *sband;
	sband=&atbmwifi_band_2ghz;

	mi->lowest_rix = atbmwifi_rate_lowest_index(sband, sta);
	ctl_rate = &sband->bitrates[mi->lowest_rix];
	mi->sp_ack_dur = atbmwifi_ieee80211_frame_duration(10,
				ctl_rate->bitrate,
				!!(ctl_rate->rate_flag & IEEE80211_RATE_ERP_G), 1);

	for (i = 0; i < sband->n_bitrates; i++) {
		struct minstrel_rate *mr = &mi->r[n];
		unsigned int tx_time = 0, tx_time_cts = 0, tx_time_rtscts = 0;
		unsigned int tx_time_single;
		unsigned int cw = mp->cw_min;

		if (!atbmwifi_rate_supported(sta,i))
			continue;
		n++;
		atbm_memset(mr, 0, sizeof(*mr));

		mr->rix = i;
		mr->bitrate = sband->bitrates[i].bitrate / 5;
		atbm_calc_rate_durations(mr, &sband->bitrates[i]);

		/* calculate maximum number of retransmissions before
		 * fallback (based on maximum segment size) */
		mr->sample_limit = -1;
		mr->retry_count = 1;
		mr->retry_count_cts = 1;
		mr->retry_count_rtscts = 1;
		tx_time = mr->perfect_tx_time + mi->sp_ack_dur;
		do {
			/* add one retransmission */
			tx_time_single = mr->ack_time + mr->perfect_tx_time;

			/* contention window */
			tx_time_single += (t_slot * cw) >> 1;
			cw = atbm_min((cw << 1) | 1, mp->cw_max);

			tx_time += tx_time_single;
			tx_time_cts += tx_time_single + mi->sp_ack_dur;
			tx_time_rtscts += tx_time_single + 2 * mi->sp_ack_dur;
			if ((tx_time_cts < mp->segment_size) &&
				(mr->retry_count_cts < mp->max_retry))
				mr->retry_count_cts++;
			if ((tx_time_rtscts < mp->segment_size) &&
				(mr->retry_count_rtscts < mp->max_retry))
				mr->retry_count_rtscts++;
		} while ((tx_time < mp->segment_size) &&
				(++mr->retry_count < mp->max_retry));
		mr->adjusted_retry_count = mr->retry_count;
	}

	for (i = n; i <sband->n_bitrates; i++) {
		struct minstrel_rate *mr = &mi->r[i];
		mr->rix = -1;
	}

	mi->n_rates = n;
	mi->stats_update = atbm_GetOsTimeMs();

	atbm_init_sample_table(mi);
}

static atbm_void *
atbm_minstrel_alloc_sta()
{
	struct atbmwifi_minstrel_sta_info *mi;
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	//struct ieee80211_hw *hw = mp->hw;
	int max_rates = 0;
	int i;

	mi = (struct atbmwifi_minstrel_sta_info *)atbm_kmalloc(sizeof(struct atbmwifi_minstrel_sta_info),GFP_KERNEL);
	if (!mi)
		return ATBM_NULL;
	atbm_memset(mi,0,sizeof(struct atbmwifi_minstrel_sta_info));

	max_rates = atbmwifi_band_2ghz.n_bitrates;
	mi->r = (struct minstrel_rate *)atbm_kmalloc(sizeof(struct minstrel_rate) * max_rates,GFP_KERNEL);
	if (!mi->r)
		goto error;

	mi->sample_table = (atbm_uint8 *)atbm_kmalloc(SAMPLE_COLUMNS * max_rates,GFP_KERNEL);
	if (!mi->sample_table)
		goto error1;

	mi->stats_update = atbm_GetOsTimeMs();
	return mi;

error1:
	atbm_kfree(mi->r);
error:
	atbm_kfree(mi);
	return ATBM_NULL;
}

static atbm_void
atbm_minstrel_free_sta(atbm_void *priv_sta)
{
	struct atbmwifi_minstrel_sta_info *mi = (struct atbmwifi_minstrel_sta_info *)priv_sta;

	atbm_kfree(mi->sample_table);
	atbm_kfree(mi->r);
	atbm_kfree(mi);
}

static atbm_void *
atbm_minstrel_alloc(atbm_void)
{
	struct atbmwifi_minstrel_priv *mp = &mp_rc;
	/* contention window settings
	 * Just an approximation. Using the per-queue values would complicate
	 * the calculations and is probably unnecessary */
	mp->cw_min = 15;
	mp->cw_max = 1023;

	/* number of packets (in %) to use for sampling other rates
	 * sample less often for non-mrr packets, because the overhead
	 * is much higher than with mrr */
	mp->lookaround_rate = 5;
	mp->lookaround_rate_mrr = 10;

	/* moving average weight for EWMA */
	mp->ewma_level = 75;

	/* maximum time that the hw is allowed to stay in one MRR segment */
	mp->segment_size = 6000;

	mp->max_retry = 8;//hw->max_rate_tries;

	mp->has_mrr = ATBM_TRUE;

	mp->update_interval = 100;

	return mp;
}
static atbm_void
atbm_minstrel_free(atbm_void *priv)
{
	wifi_printk(WIFI_RATE,"atbm_minstrel_free!\n");
	//atbm_kfree(priv);
}
 atbm_void atbm_rate_control_minstrel_init(atbm_void)
{		
	mac80211_ratectrl_minstrel.tx_status = atbm_minstrel_tx_status;
	mac80211_ratectrl_minstrel.get_rate =  atbm_minstrel_get_rate;
	mac80211_ratectrl_minstrel.sta_rate_init = atbm_minstrel_rate_init;
	mac80211_ratectrl_minstrel.alloc_sta = atbm_minstrel_alloc_sta;
	mac80211_ratectrl_minstrel.free_sta =  atbm_minstrel_free_sta;	
	mac80211_ratectrl_minstrel.alloc = 	 atbm_minstrel_alloc;
	mac80211_ratectrl_minstrel.free =    atbm_minstrel_free;
}
#endif //MINSTREL_RATE_CONTROL
