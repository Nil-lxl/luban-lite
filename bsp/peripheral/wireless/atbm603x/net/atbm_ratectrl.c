/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"
#include "atbm_ratectrl.h"
//extern int atbmwifi_ieee80211_tx_h_rate_ctrl_cnt;
#define CONFIG_COMPAT_MAC80211_RC_MINSTREL_HT "minstrel_ht"  //20M/40M b/g/n mode
#define CONFIG_COMPAT_MAC80211_RC_MINSTREL "minstrel"     //20M bg only
#define CONFIG_COMPAT_MAC80211_RC_PID "pid" 
#define AVG_PKT_SIZE	1200
/*Define global rate varible*/
struct atbmwifi_minstrel_priv  mp_rc;
/* Number of bits for an average sized packet */
#define MCS_NBITS (AVG_PKT_SIZE << 3)

/* Number of symbols for a packet with (bps) bits per symbol */
#define MCS_NSYMS(bps) ((MCS_NBITS + (bps) - 1) / (bps))

/* Transmission time for a packet containing (syms) symbols */
#define MCS_SYMBOL_TIME(sgi, syms)					\
	(sgi ?								\
	  ((syms) * 18 + 4) / 5 :	/* syms * 3.6 us */		\
	  (syms) << 2			/* syms * 4 us */		\
	)

/* Transmit duration for the raw data part of an average sized packet */
#define MCS_DURATION(streams, sgi, bps) MCS_SYMBOL_TIME(sgi, MCS_NSYMS((streams) * (bps)))

struct rate_control_ops * mac80211_ratectrl;


int atbmwifi_ieee80211_frame_duration(atbm_size_t len,
			     int rate, int erp, int short_preamble)
{
	int dur;

	/*
	 * 802.11b or 802.11g with 802.11b compatibility:
	 * 18.3.4: TXTIME = PreambleLength + PLCPHeaderTime +
	 * Ceiling(((LENGTH+PBCC)x8)/DATARATE). PBCC=0.
	 *
	 * 802.11 (DS): 15.3.3, 802.11b: 18.3.4
	 * aSIFSTime = 10 usec
	 * aPreambleLength = 144 usec or 72 usec with short preamble
	 * aPLCPHeaderLength = 48 usec or 24 usec with short preamble
	 */
	dur = 10; /* aSIFSTime = 10 usec */
	dur += short_preamble ? (72 + 24) : (144 + 48);

	dur += DIV_ROUND_UP(8 * (len + 4) * 10, rate);
	return dur;
}
int atbmwifi_rate_supported(struct atbmwifi_cfg80211_rate *sta,int index)
{
	return (sta == ATBM_NULL || sta->support_rates & BIT(index));
}
int atbmwifi_rate_lowest_index(struct atbmwifi_ieee80211_supported_band *sband,
		  struct atbmwifi_cfg80211_rate *sta_rate)
{
	int i;

	for (i = 0; i < sband->n_bitrates; i++)
		if (atbmwifi_rate_supported(sta_rate,i)){
			return i;
		}
	/* and return 0 (the lowest index) */
	return 0;
}
#if 0
static atbm_void atbmwifi_rc_send_low_broadcast(atbm_int8 *idx, atbm_uint32 basic_rates,
				  struct atbmwifi_ieee80211_supported_band *sband)
{
	atbm_uint8 i;

	if (basic_rates == 0)
		return; /* assume basic rates unknown and accept rate */
	if (*idx < 0)
		return;
	if (basic_rates & (1 << *idx))
		return; /* selected rate is a basic rate */

	for (i = *idx + 1; i <= sband->n_bitrates; i++) {
		if (basic_rates & (1 << i)) {
			*idx = i;
			return;
		}
	}

	/* could not find a basic rate; use original selection */
}
#endif
static ATBM_BOOL atbmwifi_rc_no_data_or_no_ack_use_min(struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_tx_info *tx_info = ATBM_IEEE80211_SKB_TXCB(skb);

	return (tx_info->flags & (ATBM_IEEE80211_TX_CTL_NO_ACK |
			       ATBM_IEEE80211_TX_CTL_USE_MINRATE));
}
int atbmwifi_rate_lowest_non_cck_index(struct atbmwifi_ieee80211_supported_band *sband,
			  struct atbmwifi_cfg80211_rate *sta)
{
	int i;

	for (i = 0; i < sband->n_bitrates; i++) {
		struct atbmwifi_ieee80211_rate *srate = &sband->bitrates[i];
		if ((srate->bitrate == 10) || (srate->bitrate == 20) ||
		    (srate->bitrate == 55) || (srate->bitrate == 110))
			continue;

		if (atbmwifi_rate_supported(sta,i))
			return i;
	}

	/* No matching rate found */
	return 0;
}

ATBM_BOOL atbmwifi_rate_control_send_low(struct atbmwifi_cfg80211_rate *sta,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_supported_band *sband=&atbmwifi_band_2ghz;
	struct atbmwifi_ieee80211_tx_info *tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
//	int mcast_rate;
	if (atbmwifi_rc_no_data_or_no_ack_use_min(skb)) {
		if ((sband->band != ATBM_IEEE80211_BAND_2GHZ) ||
			!(tx_info->flags & ATBM_IEEE80211_TX_CTL_NO_CCK_RATE)){
				tx_info->control.rates[0].idx =
					atbmwifi_rate_lowest_index(sband, sta);
			}else{
				tx_info->control.rates[0].idx =
					atbmwifi_rate_lowest_non_cck_index(sband, sta);
			}
			tx_info->control.rates[0].count =
				(tx_info->flags & ATBM_IEEE80211_TX_CTL_NO_ACK) ?1 : 8;
		return ATBM_TRUE;
	}
	return ATBM_FALSE;
}



atbm_void hmac_rc_init(struct atbmwifi_common *hw_priv,const char *rc_name)
{
//#if ATBM_TX_SKB_NO_TXCONFIRM
	mac80211_ratectrl =rate_control_ops_pid_init();
//#else //ATBM_TX_SKB_NO_TXCONFIRM
	/*ministrel Ht rateCtrol init*/
//	mac80211_ratectrl = atbm_rate_control_minstrel_ht_init();
//#endif //ATBM_TX_SKB_NO_TXCONFIRM
}



