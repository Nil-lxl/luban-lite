#ifndef __RC80211_PID_H__
#define __RC80211_PID_H__

#include "atbm_ratectrl.h"
#include "atbm_hal.h"
#define MCS_GROUP_RATES	8

/* Sampling period for measuring percentage of failed frames in ms. */
#define RC_PID_INTERVAL			125

/* Exponential averaging smoothness (used for I part of PID controller) */
#define RC_PID_SMOOTHING_SHIFT		3
#define RC_PID_SMOOTHING		(1 << RC_PID_SMOOTHING_SHIFT)

/* Sharpening factor (used for D part of PID controller) */
#define RC_PID_SHARPENING_FACTOR	0
#define RC_PID_SHARPENING_DURATION	0

/* Fixed point arithmetic shifting amount. */
#define RC_PID_ARITH_SHIFT		8

/* Proportional PID component coefficient. */
#define RC_PID_COEFF_P			15
/* Integral PID component coefficient. */
#define RC_PID_COEFF_I			9
/* Derivative PID component coefficient. */
#define RC_PID_COEFF_D			15

/* Target failed frames rate for the PID controller. NB: This effectively gives
 * maximum failed frames percentage we're willing to accept. If the wireless
 * link quality is good, the controller will fail to adjust failed frames
 * percentage to the target. This is intentional.
 */
#define RC_PID_TARGET_PF		14

/* Rate behaviour normalization quantity over time. */
#define RC_PID_NORM_OFFSET		3

/* Push high rates right after loading. */
#define RC_PID_FAST_START		0

/* Arithmetic right shift for positive and negative values for ISO C. */
#define RC_PID_DO_ARITH_RIGHT_SHIFT(x, y) \
	((x) < 0 ? -((-(x)) >> (y)) : (x) >> (y))

struct rc_pid_sta_info {
	//unsigned long last_change;
	unsigned long last_sample;
	atbm_uint32  pf[RATE_INDEX_MAX];
	atbm_uint32  pf_cnt[RATE_INDEX_MAX];

	atbm_uint32 tx_num_failed;
	atbm_uint32 tx_num_xmit;

	atbm_uint8 txrate_idx;	
	/* Sharpening needed. */
	atbm_uint8 sharp_cnt;	
	atbm_uint8 lowest_txrate_idx;
	atbm_uint8 b_ht_sta:1,
	   b_valid:1;	

	struct rc_pid_info * pinfo;
};

/* Algorithm parameters. We keep them on a per-algorithm approach, so they can
 * be tuned individually for each interface.
 */
struct rc_pid_rateinfo {

	/* Map sorted rates to rates in atbmwifi_ieee80211_hw_mode. */
	atbm_uint8 index;

	/* Map rates in atbmwifi_ieee80211_hw_mode to sorted rates. */
	atbm_uint8 rev_index;

	/* Did we do any measurement on this rate? */
	ATBM_BOOL valid;

	atbm_uint8  reserve;/*±£¡Ù*/
	//atbm_uint8   max_index;

};

struct rc_pid_info {

	/* Rate at which failed frames percentage is sampled in 0.001s. */
	unsigned short sampling_period;

	/* Rates information. */
	struct rc_pid_rateinfo *rinfo;
	/* Rates information. */
	struct rc_pid_rateinfo no_ht_rinfo[14];
	/* Rates information. */
	struct rc_pid_rateinfo ht_rinfo[MCS_GROUP_RATES*CONFIG_HT_MCS_STREAM_MAX_STREAMS];

	/* Index of the last used rate. */
	atbm_uint8  oldrate;
	/* Index of the mac no_ht rate. */
	atbm_uint8  max_no_ht_rate_index;
	/* Index of the mac ht rate. */
	atbm_uint8  max_ht_rate_index;	
	
	/* Index of the mac ht or noht rate. */
	atbm_uint8  max_rate_index;

	struct rc_pid_sta_info spinfo[ATBMWIFI__MAX_STA_IN_AP_MODE];
};

struct atbmwifi_cfg80211_bss;

/*
struct rate_control_ops {
	//struct module *module;
	void (*init)();
	void (*deinit)();
	void *(*alloc_sta)();
	void (*free_sta)(void *priv_sta);
	void (*sta_rate_init)(struct atbmwifi_cfg80211_bss  *sta, void *priv_sta);
	void (*tx_status)(struct atbmwifi_cfg80211_bss  *sta, void *priv_sta,
			  struct atbmwifi_ieee80211_tx_done_status *tx_status);
	void (*get_rate)(struct atbmwifi_cfg80211_bss  *sta, void *priv_sta,
			  struct atbmwifi_ieee80211_tx_info *txrc);

};*/
 struct rate_control_ops * rate_control_ops_pid_init(atbm_void);

#endif /* RC80211_PID_H */
