#ifndef __RC_MINSTREL_H
#define __RC_MINSTREL_H
#include "atbm_type.h"

#define SAMPLE_COLUMNS_11BG	12

struct minstrel_rate {
	int bitrate;
	int rix;

	unsigned int perfect_tx_time;
	unsigned int ack_time;

	int sample_limit;
	unsigned int retry_count;
	unsigned int retry_count_cts;
	unsigned int retry_count_rtscts;
	unsigned int adjusted_retry_count;

	atbm_uint32 success;
	atbm_uint32 attempts;
	atbm_uint32 last_attempts;
	atbm_uint32 last_success;

	/* parts per thousand */
	atbm_uint32 cur_prob;
	atbm_uint32 probability;

	/* per-rate throughput */
	atbm_uint32 cur_tp;

	atbm_uint64 succ_hist;
	atbm_uint64 att_hist;
};

struct atbmwifi_minstrel_sta_info {
	unsigned long stats_update;
	unsigned int sp_ack_dur;
	unsigned int rate_avg;

	unsigned int lowest_rix;

	unsigned int max_tp_rate;
	unsigned int max_tp_rate2;
	unsigned int max_prob_rate;
	unsigned int packet_count;
	unsigned int sample_count;
	int sample_deferred;

	unsigned int sample_idx;
	unsigned int sample_column;

	int n_rates;
	struct minstrel_rate *r;
	ATBM_BOOL prev_sample;

	/* sampling table */
	atbm_uint8 *sample_table;
};

struct atbmwifi_minstrel_priv {
//	struct ieee80211_hw *hw;
	ATBM_BOOL has_mrr;
	unsigned int cw_min;
	unsigned int cw_max;
	unsigned int max_retry;
	unsigned int ewma_level;
	unsigned int segment_size;
	unsigned int update_interval;
	unsigned int lookaround_rate;
	unsigned int lookaround_rate_mrr;
	struct atbmwifi_minstrel_sta_info mi;

};

#endif
