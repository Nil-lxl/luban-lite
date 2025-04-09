/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef __HMAC_RATE_CTROL__
#define __HMAC_RATE_CTROL__
#include "atbm_hal.h"
#include "atbm_rc80211_pid.h"
#include "atbm_rc80211_minstrel.h"
#include "atbm_rc80211_minstrel_ht.h"
#define SAMPLE_COLUMNS	4
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
struct rate_control_ops {
#if (RATE_CONTROL_MODE==2)
	atbm_void (*tx_status)(struct atbmwifi_cfg80211_rate	*sta, atbm_void *priv_sta,struct atbm_buff *skb);
#else
	atbm_void  (*tx_status)(struct atbmwifi_cfg80211_rate  *sta, atbm_void  *priv_sta,struct txrate_status  * txstatus);
#endif //#if (RATE_CONTROL_MODE==2)
	atbm_void (*get_rate)(struct atbmwifi_cfg80211_rate  *sta, atbm_void *priv_sta,struct atbmwifi_ieee80211_tx_info *tx_info ,struct atbm_buff *skb);
	atbm_void (*sta_rate_init)(struct atbmwifi_cfg80211_rate  *sta,atbm_void *priv_sta);
	atbm_void (*rate_update)(struct atbmwifi_cfg80211_rate *sta, atbm_void *priv_sta, enum atbm_nl80211_channel_type oper_chan_type);
	atbm_void *(*alloc_sta)(void);
	atbm_void (*free_sta)(atbm_void *priv_sta);
	atbm_void (*alloc)(atbm_void);
	atbm_void (*free)(atbm_void*);
};
extern struct rate_control_ops* mac80211_ratectrl;
 ATBM_BOOL atbmwifi_rate_control_send_low(struct atbmwifi_cfg80211_rate *sta,struct atbm_buff *skb);
int atbmwifi_ieee80211_frame_duration(atbm_size_t len,
			     int rate, int erp, int short_preamble);
atbm_void hmac_rc_init(struct atbmwifi_common *hw_priv,const char *rc_name);
#endif /*__HMAC_RATE_CTROL_*/
