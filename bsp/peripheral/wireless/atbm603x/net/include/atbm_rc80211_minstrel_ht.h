#ifndef __RC_MINSTREL_HT_H__
#define __RC_MINSTREL_HT_H__
#define MINSTREL_MAX_STREAMS	3
#define MINSTREL_STREAM_GROUPS	4

/* scaled fraction values */
#define MINSTREL_SCALE	16
#define MINSTREL_FRAC(val, div) (((val) << MINSTREL_SCALE) / div)
#define MINSTREL_TRUNC(val) ((val) >> MINSTREL_SCALE)
#define MCS_GROUP_RATES	8
struct atbm_mcs_group {
	atbm_uint32 flags;
	unsigned int streams;
	unsigned int duration[MCS_GROUP_RATES];
};
struct atbm_minstrel_rate_stats {
	/* current / last sampling period attempts/success counters */
	unsigned int attempts, last_attempts;
	unsigned int success, last_success;

	/* total attempts/success counters */
	atbm_uint64 att_hist, succ_hist;

	/* current throughput */
	unsigned int cur_tp;

	/* packet delivery probabilities */
	unsigned int cur_prob, probability;

	/* maximum retry counts */
	unsigned int retry_count;
	unsigned int retry_count_rtscts;

	ATBM_BOOL retry_updated;
	atbm_uint8 sample_skipped;
};
struct atbm_minstrel_mcs_group_data {
	atbm_uint8 index;
	atbm_uint8 column;
	/* bitfield of supported MCS rates of this group */
	atbm_uint8 supported;
	/* selected primary rates */
	unsigned int max_tp_rate;
	unsigned int max_tp_rate2;
	unsigned int max_prob_rate;
	/* MCS rate statistics */
	struct atbm_minstrel_rate_stats rates[MCS_GROUP_RATES];
};
struct atbm_minstrel_ht_sta {
	/* ampdu length (average, per sampling interval) */
	unsigned int ampdu_len;
	unsigned int ampdu_packets;

	/* ampdu length (EWMA) */
	unsigned int avg_ampdu_len;

	/* best throughput rate */
	unsigned int max_tp_rate;

	/* second best throughput rate */
	unsigned int max_tp_rate2;

	/* best probability rate */
	unsigned int max_prob_rate;

	/* time of last status update */
	unsigned long stats_update;

	/* overhead time in usec for each frame */
	unsigned int overhead;
	unsigned int overhead_rtscts;

	unsigned int total_packets;
	unsigned int sample_packets;

	/* tx flags to add for frames for this sta */
	atbm_uint32 tx_flags;

	atbm_uint8 sample_wait;
	atbm_uint8 sample_tries;
	atbm_uint8 sample_count;
	atbm_uint8 sample_slow;

	/* current MCS group to be sampled */
	atbm_uint8 sample_group;

	/* MCS rate group info and statistics */
	struct atbm_minstrel_mcs_group_data groups[4];
};
struct atbm_minstrel_ht_sta_priv {
	union {
		struct atbm_minstrel_ht_sta ht;
		struct atbmwifi_minstrel_sta_info legacy;
	};
	atbm_void *ratelist;
	atbm_void *sample_table;
	ATBM_BOOL is_ht;
};

int atbmwifi_ieee80211_tx_h_rate_ctrl(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t);
int atbm_get_tx_hwrate(struct atbmwifi_common *hw_priv,struct atbmwifi_ieee80211_tx_rate *rate);
int atbmwifi_tx_policy_get(struct atbmwifi_common *hw_priv,struct atbmwifi_ieee80211_tx_rate *rates,int count, ATBM_BOOL *renew);
atbm_void atbm_tx_policy_put(struct atbmwifi_common *hw_priv, int idx);

#endif //__RC_MINSTREL_HT_H
