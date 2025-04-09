/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef ATBMWIFI_NET_H
#define ATBMWIFI_NET_H
#include "atbm_type.h"
enum atbm_nl80211_iftype {
	ATBM_NL80211_IFTYPE_STATION,                          
	ATBM_NL80211_IFTYPE_AP,                               
	ATBM_NL80211_IFTYPE_ADHOC,                            
	ATBM_NL80211_IFTYPE_MONITOR,                          
	ATBM_NL80211_IFTYPE_P2P_CLIENT,                       
	ATBM_NL80211_IFTYPE_P2P_GO,                           
	/* keep last */                                       
	ATBM_NUM_NL80211_IFTYPES,                             
	ATBM_NL80211_IFTYPE_MAX = ATBM_NUM_NL80211_IFTYPES - 1
};
/**
 * enum nl80211_band - Frequency band
 * @NL80211_BAND_2GHZ: 2.4 GHz ISM band
 * @NL80211_BAND_5GHZ: around 5 GHz band (4.9 - 5.7 GHz)
 */
enum atbm_nl80211_band {
	ATBM_NL80211_BAND_2GHZ,
	ATBM_NL80211_BAND_5GHZ,
};
/**
 * enum atbmwifi_ieee80211_band - supported frequency bands
 *
 * The bands are assigned this way because the supported
 * bitrates differ in these bands.
 *
 * @ATBM_IEEE80211_BAND_2GHZ: 2.4GHz ISM band
 * @IEEE80211_BAND_5GHZ: around 5GHz band (4.9-5.7)
 * @ATBM_IEEE80211_NUM_BANDS: number of defined bands
 */
enum atbmwifi_ieee80211_band {
	ATBM_IEEE80211_BAND_2GHZ = ATBM_NL80211_BAND_2GHZ,
	ATBM_IEEE80211_BAND_5GHZ =ATBM_NL80211_BAND_5GHZ,
	/* keep last */
	ATBM_IEEE80211_NUM_BANDS
};
/**
 * enum atbmwifi_ieee80211_ac_numbers - AC numbers as used in mac80211
 * @IEEE80211_AC_VO: voice
 * @IEEE80211_AC_VI: video
 * @IEEE80211_AC_BE: best effort
 * @IEEE80211_AC_BK: background
 */
enum atbmwifi_ieee80211_ac_numbers {
	ATBM_IEEE80211_AC_VO		= 0,
	ATBM_IEEE80211_AC_VI		= 1,
	ATBM_IEEE80211_AC_BE		= 2,
	ATBM_IEEE80211_AC_BK		= 3,
};
/* Access Catagory Indices*/
#define ATBM_D11_ACI_AC_BE		       0	/* Best Effort*/
#define ATBM_D11_ACI_AC_BK		       1	/* Background*/
#define ATBM_D11_ACI_AC_VI		       2	/* Video*/
#define ATBM_D11_ACI_AC_VO		       3	/* Voice*/

/**
 * enum ieee80211_frame_release_type - frame release reason
 * @IEEE80211_FRAME_RELEASE_PSPOLL: frame released for PS-Poll
 * @IEEE80211_FRAME_RELEASE_UAPSD: frame(s) released due to
 *	frame received on trigger-enabled AC
 */
enum atbm_ieee80211_frame_release_type {
	ATBM_IEEE80211_FRAME_RELEASE_PSPOLL,
	ATBM_IEEE80211_FRAME_RELEASE_UAPSD,
};
/*Ap ps*/
#define ATBM_TOTAL_MAX_TX_BUFFER 512
#define ATBM_STA_MAX_TX_BUFFER 64
#define ATBM_AP_MAX_BC_BUFFER 128
/* Minimum buffered frame expiry time. If STA uses listen interval that is
 * smaller than this value, the minimum value here is used instead. */
#define ATBM_STA_TX_BUFFER_EXPIRE (5 * 1000) //ms
#define ATBM_STA_INFO_CLEANUP_INTERVAL (4 * 1000) //ms
/* there are 40 bytes if you don't need the rateset to be kept */
#define ATBM_IEEE80211_TX_INFO_DRIVER_DATA_SIZE 40

/* if you do need the rateset, then you have less space */
#define ATBM_IEEE80211_TX_INFO_RATE_DRIVER_DATA_SIZE 24


/* maximum number of rate stages */
#define ATBM_IEEE80211_TX_MAX_RATES	5

#define ATBM_IEEE80211_TX_CTL_STBC_SHIFT		23
/* maximum number of rate stages */

#define ATBM_IEEE80211_NUM_ACS	4
struct atbmwifi_ieee80211_tx_rate {
	atbm_int8  idx;
	atbm_uint8  count;
	atbm_uint16 flags;
};

struct atbmwifi_ieee80211_tx_rate_control{
	struct atbmwifi_ieee80211_tx_rate rates[ATBM_IEEE80211_TX_MAX_RATES];
	atbm_uint8 ampdu_ack_len;
	atbm_uint8 ampdu_len;
	atbm_uint8 rts_cts_rate_idx;
	atbm_uint8 reserved;
	int ack_signal;
	/* 15 bytes free */
};
struct atbmwifi_ieee80211_tx_info{	 
	atbm_uint16	rate_11g:1,
	    ht:1,
		short_preamble:1,
		use_short_slot:1,
		ht_40M:1,	
		greenfield:1,
		b_net:1,
		b_multi,
		short_gi:1,
		b_eapol:1,
		offset:1;
	atbm_uint8	reserve[2];
	atbm_uint8	link_id;
	atbm_uint8  hw_rate_id;
	atbm_uint8  rate_policy;
	atbm_uint8  txpower;
	int flags;
	/* only needed before rate control */
	unsigned int m_jiffies;
	struct atbmwifi_ieee80211_tx_rate_control control;
};
#define TX_CONTINUE	0
#define TX_QUEUED	1

#define ATBM_IEEE80211_TX_UNICAST		BIT(1)
#define ATBM_IEEE80211_TX_PS_BUFFERED	BIT(2)
struct atbmwifi_txinfo {
	struct atbm_buff *skb;
	unsigned queue;
	struct atbmwifi_ieee80211_tx_info *tx_info;
	//const struct atbmwifi_ieee80211_rate *rate;
	struct atbmwifi_ieee80211_hdr *hdr;
	atbm_size_t hdrlen;
	const atbm_uint8 *da;
	struct atbmwifi_sta_priv *sta_priv;
	struct atbmwifi_txpriv txpriv;
};

struct atbmwifi_ieee80211_rx_status {
	//u64 mactime;
	enum atbmwifi_ieee80211_band band;
	atbm_uint8 antenna;
	atbm_uint16 freq;

	
	atbm_int8 signal;
	atbm_uint8 rate_idx;
	/*
	 * Index into sequence numbers array, 0..16
	 * since the last (16) is used for non-QoS,
	 * will be 16 on non-QoS frames.
	 */
	atbm_uint8 seqno_idx;

	/*
	 * Index into the security IV/PN arrays, 0..16
	 * since the last (16) is used for CCMP-encrypted
	 * management frames, will be set to 16 on mgmt
	 * frames and 0 on non-QoS frames.
	 */
	//atbm_uint8 security_idx;
	//atbm_uint8 reserved[3];
	atbm_uint8 link_id;
	
	int flag;

};
enum mac80211_rx_flags {
	ATBM_RX_FLAG_MMIC_ERROR	= 1<<0,    
	ATBM_RX_FLAG_DECRYPTED	= 1<<1,      
	ATBM_RX_FLAG_MMIC_STRIPPED	= 1<<3,  
	ATBM_RX_FLAG_IV_STRIPPED	= 1<<4,    
	ATBM_RX_FLAG_FAILED_FCS_CRC	= 1<<5,
	ATBM_RX_FLAG_FAILED_PLCP_CRC = 1<<6,
	ATBM_RX_FLAG_MACTIME_MPDU	= 1<<7,  
	ATBM_RX_FLAG_SHORTPRE	= 1<<8,      
	ATBM_RX_FLAG_HT		= 1<<9,          
	ATBM_RX_FLAG_40MHZ		= 1<<10,       
	ATBM_RX_FLAG_SHORT_GI	= 1<<11,     
	ATBM_RX_FLAG_AMSDU	= 1<<12,         
};

#define ATBM_BASIC_RATE_MASK 0x80
enum rate_flags {
	ATBM_IEEE80211_RT_11B 			=BIT(0),
	ATBM_IEEE80211_RT_11B_SHORT		= BIT(1),
	ATBM_IEEE80211_RT_11G		= BIT(2),
	ATBM_IEEE80211_RT_11N		= BIT(3),
	ATBM_IEEE80211_RATE_ERP_G	= BIT(4),
	ATBM_IEEE80211_RT_BASIC		= ATBM_BASIC_RATE_MASK,/*must be BIT7*/
};
/**
 * struct atbmwifi_ieee80211_rate - bitrate definition
 *
 * This structure describes a bitrate that an 802.11 PHY can
 * operate with. The two values @hw_value and @hw_value_short
 * are only for driver use when pointers to this structure are
 * passed around.
 *
 * @flags: rate-specific flags
 * @bitrate: bitrate in units of 100 Kbps
 * @hw_value: driver/hardware value for this rate
 * @hw_value_short: driver/hardware value for this rate when
 *	short preamble is used
 */
struct atbmwifi_ieee80211_rate {
	//atbm_uint32 flags;
	atbm_uint16 bitrate;
	atbm_uint8 hw_value;
	atbm_uint8 rate_flag;
}atbm_packed;

enum mac80211_tx_control_flags {
	ATBM_IEEE80211_TX_CTL_NO_ACK			= BIT(0),
	ATBM_IEEE80211_TX_CTL_NO_CCK_RATE		= BIT(1),
	ATBM_IEEE80211_TX_CTL_ASSIGN_SEQ		= BIT(2),
	ATBM_IEEE80211_TX_CTL_USE_MINRATE		= BIT(3),
	ATBM_IEEE80211_TX_CTL_USE_FIXRATE		= BIT(4),	
	ATBM_IEEE80211_TX_INTFL_TKIP_MIC_FAILURE	= BIT(5),
	ATBM_IEEE80211_TX_CTL_AMPDU			= BIT(6),
	ATBM_IEEE80211_TX_STATUS_EOSP		= BIT(7),
	ATBM_IEEE80211_TX_STAT_ACK			= BIT(8),
	ATBM_IEEE80211_TX_STAT_AMPDU			= BIT(9),
	ATBM_IEEE80211_TX_STAT_AMPDU_NO_BACK		= BIT(10),
	ATBM_IEEE80211_TX_CTL_RATE_CTRL_PROBE	= BIT(11),
	ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT 	= BIT(12),
	ATBM_IEEE80211_TX_CTL_LDPC				= BIT(13),
	ATBM_IEEE80211_TX_CTL_POLL_RESPONSE		= BIT(14),
	ATBM_IEEE80211_TX_CTL_REQ_TX_STATUS		= BIT(15),
};
#define ATBM_IEEE80211_TX_CTL_STBC_SHIFT		23
/* maximum number of rate stages */
//#define ATBM_IEEE80211_TX_STAT_ACK	5



/**
 * enum mac80211_rate_control_flags - per-rate flags set by the
 *	Rate Control algorithm.
 *
 * These flags are set by the Rate control algorithm for each rate during tx,
 * in the @flags member of struct ieee80211_tx_rate.
 *
 * @ATBM_IEEE80211_TX_RC_USE_RTS_CTS: Use RTS/CTS exchange for this rate.
 * @ATBM_IEEE80211_TX_RC_USE_CTS_PROTECT: CTS-to-self protection is required.
 *	This is set if the current BSS requires ERP protection.
 * @IEEE80211_TX_RC_USE_SHORT_PREAMBLE: Use short preamble.
 * @IEEE80211_TX_RC_MCS: HT rate.
 * @IEEE80211_TX_RC_GREEN_FIELD: Indicates whether this rate should be used in
 *	Greenfield mode.
 * @IEEE80211_TX_RC_40_MHZ_WIDTH: Indicates if the Channel Width should be 40 MHz.
 * @IEEE80211_TX_RC_DUP_DATA: The frame should be transmitted on both of the
 *	adjacent 20 MHz channels, if the current channel type is
 *	ATBM_NL80211_CHAN_HT40MINUS or ATBM_NL80211_CHAN_HT40PLUS.
 * @IEEE80211_TX_RC_SHORT_GI: Short Guard interval should be used for this rate.
 */
enum mac80211_rate_control_flags {
	ATBM_IEEE80211_TX_RC_USE_RTS_CTS		= BIT(0),
	ATBM_IEEE80211_TX_RC_USE_CTS_PROTECT		= BIT(1),
	ATBM_IEEE80211_TX_RC_USE_SHORT_PREAMBLE	= BIT(2),

	/* rate index is an MCS rate number instead of an index */
	ATBM_IEEE80211_TX_RC_MCS			= BIT(3),
	ATBM_IEEE80211_TX_RC_GREEN_FIELD		= BIT(4),
	ATBM_IEEE80211_TX_RC_40_MHZ_WIDTH		= BIT(5),
	ATBM_IEEE80211_TX_RC_DUP_DATA		= BIT(6),
	ATBM_IEEE80211_TX_RC_SHORT_GI		= BIT(7),
};
#define ATBM_HT_CAP_INFO_LDPC_CODING_CAP		((atbm_uint16) BIT(0))
#define ATBM_HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET	((atbm_uint16) BIT(1))
#define ATBM_HT_CAP_INFO_SMPS_MASK			((atbm_uint16) (BIT(2) | BIT(3)))
#define ATBM_HT_CAP_INFO_SMPS_STATIC			((atbm_uint16) 0)
#define ATBM_HT_CAP_INFO_SMPS_DYNAMIC		((atbm_uint16) BIT(2))
#define ATBM_HT_CAP_INFO_SMPS_DISABLED		((atbm_uint16) (BIT(2) | BIT(3)))
#define ATBM_HT_CAP_INFO_GREEN_FIELD			((atbm_uint16) BIT(4))
#define ATBM_HT_CAP_INFO_SHORT_GI20MHZ		((atbm_uint16) BIT(5))
#define ATBM_HT_CAP_INFO_SHORT_GI40MHZ		((atbm_uint16) BIT(6))
#define ATBM_HT_CAP_INFO_TX_STBC			    ((atbm_uint16) BIT(7))
#define ATBM_HT_CAP_INFO_RX_STBC_MASK		((atbm_uint16) (BIT(8) | BIT(9)))
#define ATBM_HT_CAP_INFO_RX_STBC_1			((atbm_uint16) BIT(8))
#define ATBM_HT_CAP_INFO_RX_STBC_12			((atbm_uint16) BIT(9))
#define ATBM_HT_CAP_INFO_RX_STBC_123			((atbm_uint16) (BIT(8) | BIT(9)))
#define ATBM_HT_CAP_INFO_DELAYED_BA			((atbm_uint16) BIT(10))
#define ATBM_HT_CAP_INFO_MAX_AMSDU_SIZE		((atbm_uint16) BIT(11))
#define ATBM_HT_CAP_INFO_DSSS_CCK40MHZ		((atbm_uint16) BIT(12))
#define ATBM_HT_CAP_INFO_PSMP_SUPP			((atbm_uint16) BIT(13))
#define ATBM_HT_CAP_INFO_40MHZ_INTOLERANT		((atbm_uint16) BIT(14))
#define ATBM_HT_CAP_INFO_LSIG_TXOP_PROTECT_SUPPORT	((atbm_uint16) BIT(15))


#define ATBM_EXT_HT_CAP_INFO_PCO			((atbm_uint16) BIT(0))
#define ATBM_EXT_HT_CAP_INFO_TRANS_TIME_OFFSET	1
#define ATBM_EXT_HT_CAP_INFO_MCS_FEEDBACK_OFFSET	8
#define ATBM_EXT_HT_CAP_INFO_HTC_SUPPORTED		((atbm_uint16) BIT(10))
#define ATBM_EXT_HT_CAP_INFO_RD_RESPONDER		((atbm_uint16) BIT(11))


enum ieee80211_hw_flags {
	ATBM_IEEE80211_HW_HAS_RATE_CONTROL			= 1<<0,
	ATBM_IEEE80211_HW_RX_INCLUDES_FCS			= 1<<1,
	ATBM_IEEE80211_HW_HOST_BROADCAST_PS_BUFFERING	= 1<<2,
	ATBM_IEEE80211_HW_2GHZ_SHORT_SLOT_INCAPABLE		= 1<<3,
	ATBM_IEEE80211_HW_2GHZ_SHORT_PREAMBLE_INCAPABLE	= 1<<4,
	ATBM_IEEE80211_HW_SIGNAL_UNSPEC			= 1<<5,
	ATBM_IEEE80211_HW_SIGNAL_DBM				= 1<<6,
	ATBM_IEEE80211_HW_NEED_DTIM_PERIOD			= 1<<7,
	ATBM_IEEE80211_HW_SPECTRUM_MGMT			= 1<<8,
	ATBM_IEEE80211_HW_AMPDU_AGGREGATION			= 1<<9,
	ATBM_IEEE80211_HW_SUPPORTS_PS			= 1<<10,
	ATBM_IEEE80211_HW_PS_NULLFUNC_STACK			= 1<<11,
	ATBM_IEEE80211_HW_SUPPORTS_DYNAMIC_PS		= 1<<12,
	ATBM_IEEE80211_HW_MFP_CAPABLE			= 1<<13,
	ATBM_IEEE80211_HW_BEACON_FILTER			= 1<<14,
	ATBM_IEEE80211_HW_SUPPORTS_STATIC_SMPS		= 1<<15,
	ATBM_IEEE80211_HW_SUPPORTS_DYNAMIC_SMPS		= 1<<16,
	ATBM_IEEE80211_HW_SUPPORTS_UAPSD			= 1<<17,
	ATBM_IEEE80211_HW_REPORTS_TX_ACK_STATUS		= 1<<18,
	ATBM_IEEE80211_HW_CONNECTION_MONITOR			= 1<<19,
	ATBM_IEEE80211_HW_SUPPORTS_CQM_RSSI			= 1<<20,
	ATBM_IEEE80211_HW_SUPPORTS_PER_STA_GTK		= 1<<21,
	ATBM_IEEE80211_HW_AP_LINK_PS				= 1<<22,
	ATBM_IEEE80211_HW_TX_AMPDU_SETUP_IN_HW		= 1<<23,
	ATBM_IEEE80211_HW_SUPPORTS_CQM_BEACON_MISS		= 1<<24,
	ATBM_IEEE80211_HW_SUPPORTS_CQM_TX_FAIL		= 1<<25,
	ATBM_IEEE80211_HW_SUPPORTS_P2P_PS			= 1<<26,
	ATBM_IEEE80211_HW_SUPPORTS_MULTI_CHANNEL		= 1<<27,
	ATBM_IEEE80211_HW_QUEUE_CONTROL			= 1<<28,

};

/* Rx MCS bitmask is in the first 77 bits of supported_mcs_set */
#define ATBM_IEEE80211_HT_MCS_MASK_LEN 10

struct atbmwifi_ieee80211_ht_capabilities {
	atbm_uint16 ht_capabilities_info;
	atbm_uint8 a_mpdu_params;
	atbm_uint8 supported_mcs_set[16];
	atbm_uint16 ht_extended_capabilities;
	atbm_uint32 tx_bf_capability_info;
	atbm_uint8 asel_capabilities;
}atbm_packed;

struct atbmwifi_ieee80211_ht_operation {
	atbm_uint8 control_chan;
	atbm_uint8 ht_param;
	atbm_uint16 operation_mode;
	atbm_uint16 stbc_param;
	atbm_uint8 basic_set[16];
}atbm_packed;

static inline atbm_uint16 atbm_is_ht(int channleType)
{
	return channleType != ATBM_NL80211_CHAN_NO_HT;
}


/**
 * struct atbmwifi_ieee80211_channel - channel definition
 *
 * This structure describes a single channel for use
 * with cfg80211.
 *
 * @center_freq: center frequency in MHz
 * @hw_value: hardware-specific value for the channel
 * @flags: channel flags from &enum atbmwifi_ieee80211_channel_flags.
 * @orig_flags: channel flags at registration time, used by regulatory
 *	code to support devices with additional restrictions
 * @band: band this channel belongs to.
 * @max_antenna_gain: maximum antenna gain in dBi
 * @max_power: maximum transmission power (in dBm)
 * @beacon_found: helper to regulatory code to indicate when a beacon
 *	has been found on this channel. Use regulatory_hint_found_beacon()
 *	to enable this, this is useful only on 5 GHz band.
 * @orig_mag: internal use
 * @orig_mpwr: internal use
 */
struct atbmwifi_ieee80211_channel {
	//enum atbmwifi_ieee80211_band band;
	atbm_int8 max_power;
	atbm_uint8 hw_value;
	//ATBM_BOOL beacon_found;
	//atbm_uint16 center_freq;
	atbm_uint16 flags;
	atbm_uint16 center_freq;
	//int max_antenna_gain;
	//atbm_uint32 orig_flags;
	//int orig_mag, orig_mpwr;
};
enum atbm_ieee80211_channel_flags {
	ATBM_IEEE80211_CHAN_DISABLED		= 1<<0,
	ATBM_IEEE80211_CHAN_PASSIVE_SCAN	= 1<<1,
	ATBM_IEEE80211_CHAN_NO_IBSS		= 1<<2,
	ATBM_IEEE80211_CHAN_RADAR		= 1<<3,
	ATBM_IEEE80211_CHAN_NO_HT40PLUS	= 1<<4,
	ATBM_IEEE80211_CHAN_NO_HT40MINUS	= 1<<5,

};
/**
 * struct atbmwifi_ieee80211_sta_ht_cap - STA's HT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11n HT capabilities for an STA.
 *
 * @ht_supported: is HT supported by the STA
 * @cap: HT capabilities map as described in 802.11n spec
 * @ampdu_factor: Maximum A-MPDU length factor
 * @ampdu_density: Minimum A-MPDU spacing
 * @mcs: Supported MCS rates
 */
struct atbmwifi_ieee80211_sta_ht_cap {
	atbm_uint16 cap; /* use IEEE80211_HT_CAP_ */
	ATBM_BOOL ht_supported;
	atbm_uint8 ampdu_factor;

	atbm_uint8 ampdu_density;
	atbm_uint8 reserved[3];
	struct atbmwifi_ieee80211_mcs_info mcs;
}atbm_packed;
/**
 * struct atbmwifi_ieee80211_supported_band - frequency band definition
 *
 * This structure describes a frequency band a wiphy
 * is able to operate in.
 *
 * @channels: Array of channels the hardware can operate in
 *	in this band.
 * @band: the band this structure represents
 * @n_channels: Number of channels in @channels
 * @bitrates: Array of bitrates the hardware can operate with
 *	in this band. Must be sorted to give a valid "supported
 *	rates" IE, i.e. CCK rates first, then OFDM.
 * @n_bitrates: Number of bitrates in @bitrates
 * @ht_cap: HT capabilities in this band
 */
struct atbmwifi_ieee80211_supported_band {
	struct atbmwifi_ieee80211_channel *channels;
	struct atbmwifi_ieee80211_rate *bitrates;
	enum atbmwifi_ieee80211_band band;
	int n_channels;
	int n_bitrates;
	struct atbmwifi_ieee80211_sta_ht_cap ht_cap;
};

/* BACK (block-ack) parties */
enum ieee80211_back_parties {
	WLAN_BACK_RECIPIENT = 0,
	WLAN_BACK_INITIATOR = 1,
	WLAN_BACK_TIMER = 2,
};

#define ATBM_IEEE80211_MAX_RX_AGGR			128
#define ATBM_IEEE80211_MAX_TX_AGGR			128

#define STA_TID_NUM 16
#define ADDBA_RESP_INTERVAL HZ
#define HT_AGG_MAX_RETRIES		0x3

#define HT_AGG_STATE_DRV_READY		0
#define HT_AGG_STATE_RESPONSE_RECEIVED	1
#define HT_AGG_STATE_OPERATIONAL	2
#define HT_AGG_STATE_STOPPING		3
#define HT_AGG_STATE_WANT_START		4
#define HT_AGG_STATE_WANT_STOP		5

#define HT_AGG_TX_INTREVAL			10*HZ
#define HT_AGG_TX_BURST				5


/**
 * struct tid_ampdu_tx - TID aggregation information (Tx).
 *
 * @rcu_head: rcu head for freeing structure
 * @addba_resp_timer: timer for peer's response to addba request
 * @pending: pending frames queue -- use sta's spinlock to protect
 * @dialog_token: dialog token for aggregation session
 * @timeout: session timeout value to be filled in ADDBA requests
 * @state: session state (see above)
 * @stop_initiator: initiator of a session stop
 * @tx_stop: TX DelBA frame when stopping
 * @buf_size: reorder buffer size at receiver
 * @failed_bar_ssn: ssn of the last failed BAR tx attempt
 * @bar_pending: BAR needs to be re-sent
 *
 * This structure's lifetime is managed by RCU, assignments to
 * the array holding it must hold the aggregation mutex.
 *
 * The TX path can access it under RCU lock-free if, and
 * only if, the state has the flag %HT_AGG_STATE_OPERATIONAL
 * set. Otherwise, the TX path must also acquire the spinlock
 * and re-check the state, see comments in the tx code
 * touching it.
 */
struct tid_ampdu_tx {
	//struct wpa_timer addba_resp_timer;
	struct atbm_buff_head pending;
	unsigned long state;
	atbm_uint16 timeout;
	atbm_uint8 dialog_token;
	atbm_uint8 stop_initiator;
	ATBM_BOOL tx_stop;
	atbm_uint8 buf_size;

	atbm_uint16 failed_bar_ssn;
	ATBM_BOOL bar_pending;
	atbm_uint8 hw_token;
};

/**
 * struct tid_ampdu_rx - TID aggregation information (Rx).
 *
 * @reorder_buf: buffer to reorder incoming aggregated MPDUs
 * @reorder_time: jiffies when skb was added
 * @session_timer: check if peer keeps Tx-ing on the TID (by timeout value)
 * @reorder_timer: releases expired frames from the reorder buffer.
 * @head_seq_num: head sequence number in reordering buffer.
 * @stored_mpdu_num: number of MPDUs in reordering buffer
 * @ssn: Starting Sequence Number expected to be aggregated.
 * @buf_size: buffer size for incoming A-MPDUs
 * @timeout: reset timer value (in TUs).
 * @dialog_token: dialog token for aggregation session
 * @rcu_head: RCU head used for freeing this struct
 * @reorder_lock: serializes access to reorder buffer, see below.
 *
 * This structure's lifetime is managed by RCU, assignments to
 * the array holding it must hold the aggregation mutex.
 *
 * The @reorder_lock is used to protect the members of this
 * struct, except for @timeout, @buf_size and @dialog_token,
 * which are constant across the lifetime of the struct (the
 * dialog token being used only for debugging).
 */
struct tid_ampdu_rx {
	atbm_spinlock_t reorder_lock;
	struct atbm_buff **reorder_buf;
	unsigned long *reorder_time;
	//struct atbm_timer_list session_timer;
	//struct atbm_timer_list reorder_timer;
	atbm_uint16 head_seq_num;
	atbm_uint16 stored_mpdu_num;
	atbm_uint16 ssn;
	atbm_uint16 buf_size;
	atbm_uint16 timeout;
	atbm_uint8 dialog_token;
	atbm_uint8 hw_token;
};

/**
 * struct sta_ampdu_mlme - STA aggregation information.
 *
 * @tid_rx: aggregation info for Rx per TID -- RCU protected
 * @tid_tx: aggregation info for Tx per TID
 * @tid_start_tx: sessions where start was requested
 * @addba_req_num: number of times addBA request has been sent.
 * @dialog_token_allocator: dialog token enumerator for each new session;
 * @work: work struct for starting/stopping aggregation
 * @tid_rx_timer_expired: bitmap indicating on which TIDs the
 *	RX timer expired until the work for it runs
 * @tid_rx_stop_requested:  bitmap indicating which BA sessions per TID the
 *	driver requested to close until the work for it runs
 * @mtx: mutex to protect all TX data (except non-NULL assignments
 *	to tid_tx[idx], which are protected by the sta spinlock)
 */
struct sta_ampdu_mlme {
	int init;
	atbm_mutex mtx;
	/* rx */
	//struct tid_ampdu_rx *tid_rx[STA_TID_NUM];
	unsigned long tid_rx_timer_expired[BITS_TO_LONGS(STA_TID_NUM)];
	unsigned long tid_rx_stop_requested[BITS_TO_LONGS(STA_TID_NUM)];
	/* tx */
	atbm_work work;
	struct tid_ampdu_tx *tid_tx[STA_TID_NUM];
	struct tid_ampdu_tx *tid_start_tx[STA_TID_NUM];
	atbm_int8 addba_req_num[STA_TID_NUM];
	atbm_int8 dialog_token_allocator;
	int trylimit[STA_TID_NUM];
	atbm_uint32 timestamp[STA_TID_NUM];
};

/**
 * enum ieee80211_ampdu_mlme_action - A-MPDU actions
 *
 * These flags are used with the ampdu_action() callback in
 * &struct ieee80211_ops to indicate which action is needed.
 *
 * Note that drivers MUST be able to deal with a TX aggregation
 * session being stopped even before they OK'ed starting it by
 * calling ieee80211_start_tx_ba_cb_irqsafe, because the peer
 * might receive the addBA frame and send a delBA right away!
 *
 * @IEEE80211_AMPDU_RX_START: start Rx aggregation
 * @IEEE80211_AMPDU_RX_STOP: stop Rx aggregation
 * @IEEE80211_AMPDU_TX_START: start Tx aggregation
 * @IEEE80211_AMPDU_TX_STOP: stop Tx aggregation
 * @IEEE80211_AMPDU_TX_OPERATIONAL: TX aggregation has become operational
 */
enum atbmwifi_ieee80211_ampdu_mlme_action {
	ATBM_IEEE80211_AMPDU_RX_START,
	ATBM_IEEE80211_AMPDU_RX_STOP,
	ATBM_IEEE80211_AMPDU_TX_START,
	ATBM_IEEE80211_AMPDU_TX_STOP,
	ATBM_IEEE80211_AMPDU_TX_OPERATIONAL,
};

enum ATBM_SCAN_TYPE{
	ATBM_SCAN_24G_ACTIVE,
	ATBM_SCAN_24G_PASSAVE,
	ATBM_SCAN_5G_PASSAVE_DFS,
	ATBM_SCAN_5G_PASSAVE,
	ATBM_SCAN_5G_ACTIVE,
	ATBM_SCAN_5G_ACTIVE_DFS,
	ATBM_SCAN_FAST_LINK,
	ATBM_SCAN_24G_WPS,
	ATBM_SCAN_5G_WPS,
	ATBM_SCAN_P2P,
	ATBM_SCAN_MAX_TYPE,
};

#define ATBM_MAX_SCAN_CHANNELS 14

struct atbm_scan_req{
	struct atbm_list_head list;
	struct wsm_scan scan;
	enum ATBM_SCAN_TYPE req_type;
	ATBM_BOOL scan_again;
	ATBM_BOOL scan_end;
};

/**/
struct atbm_cfg80211_connect_params {
	atbm_uint32 crypto_pairwise;
	atbm_uint32 crypto_group;
	atbm_uint32 crypto_igtkgroup;
	atbm_uint8 *bssid;
	atbm_uint8 key[32];//ap or sta 
	atbm_uint8 ptk[32];//sta just
	atbm_uint8 gtk[32];//sta just
	atbm_uint8 ptk_pn[8][8];//sta just
	atbm_uint8 ptk_pn_init[8];
	atbm_uint8 gtk_pn[8];//sta just
	atbm_uint8 gtk_pn_init;
	atbm_uint8 ptk_noqos_pn[8];//sta just
	atbm_uint8 ptk_noqos_pn_init;
	atbm_uint8 key_len;//ap or sta 
	atbm_uint8 key_idx;//ap or sta 
	atbm_uint8 key_idx_igtk;
	atbm_uint8 encrype;//ap or sta 
	atbm_uint8 update;
};

struct atbmwifi_cfg80211_rate {
	atbm_uint32 basic_rates;
	atbm_uint32 support_rates;
	struct atbmwifi_ieee80211_sta_ht_cap ht_cap;
	atbm_uint16 ht;
	atbm_uint16 channel_type; /*enum atbm_nl80211_channel_type*/
};

struct atbmwifi_sta_priv {
	int link_id;
	struct atbmwifi_vif *priv;
	
	atbm_uint8 mac[6];
	//struct ieee80211_sta_ht_cap ht_cap;
	ATBM_BOOL wme;
	atbm_uint8 uapsd_support_queues;
	atbm_uint8 max_sp;
	atbm_uint8 reserved2[2];
	atbm_uint32 driver_buffered_tids;
	atbm_uint32 flags;
	///

	atbm_uint16 beacon_interval;
	atbm_uint16 capability;
	
	int wpa:1,
		wps:1,
	    p2p:1,
	    bcm_ap:1,
	    uapsd_supported:1,
	    wmm_used:1,
	    has_erp_value:1,
	    rate_11g:1,
	    ht:1,
	    short_preamble:1,
	    sgi:1,
		ieee_80211w:1;
	
	struct atbmwifi_cfg80211_rate rate;
	
	atbm_void * sta_rc_priv;/*rate control priv for station link to my AP*/

	atbm_spinlock_t lock;
	atbm_uint16 sequence_number;
	atbm_uint8 timer_to_tid[STA_TID_NUM];
	int rx_token[STA_TID_NUM];
	atbm_uint16 tid_seq[ATBM_IEEE80211_QOS_CTL_TID_MASK + 1];
	struct sta_ampdu_mlme ampdu_mlme;

	enum atbmwifi_ieee80211_band band;
	atbm_void *reserved; //struct hostapd_sta_info * sta; initial in  hostapd_link_sta_sm()

};


struct atbmwifi_cfg80211_bss {
	atbm_uint16 channel_num;	
	atbm_uint8 channel_type/*enum atbm_nl80211_channel_type*/; //now used channel
	atbm_uint8 dtim_period;

	atbm_uint8 bssid[ATBM_ETH_ALEN];
	atbm_uint8 aid;	
	atbm_uint8 parameter_set_count; 
	/*
	 * Maximum number of buffered frames AP can deliver during a
	 * service period, IEEE80211_WMM_IE_STA_QOSINFO_SP_ALL or similar.
	 * Needs a new association to take effect.
	 */
	atbm_uint8 uapsd_max_sp_len; //IEEE80211_DEFAULT_MAX_SP_LEN;
	atbm_int8 rssi;

	atbm_uint32 use_cts_prot:1,
		use_short_slot:1,
	    ps_enabled:1,
	    arp_filter_enabled:1,
		ht_40M:1,	
		greenfield:1,
		b_eapol:1,
		privacy:1,
		qos:1;
	int dynamic_ps_timeout;
	int retry_short;
	int retry_long;
	struct atbmwifi_sta_priv sta_priv;
	int len_information_elements;
	atbm_uint8 *information_elements;
	atbm_void * rc_priv;/*ratecontrol priv*/
};

struct response{
		atbm_uint32 len;
		atbm_uint32 respbuff[31];
};
struct atbmwifi_mac80211 {
	const char *rate_control_algorithm;
	atbm_void *priv;
	atbm_uint32 flags;
	unsigned int extra_tx_headroom;
	int channel_change_time;
	int vif_data_size;
	int sta_data_size;
	int napi_weight;
	atbm_uint16 queues;
	atbm_uint16 max_listen_interval;
	atbm_int8 max_signal;
	atbm_uint8 max_rates;
	atbm_uint8 max_report_rates;
	atbm_uint8 max_rate_tries;
	atbm_uint8 max_rx_aggregation_subframes;
	atbm_uint8 max_tx_aggregation_subframes;
	atbm_uint8 offchannel_tx_hw_queue;
	atbm_uint8 vendcmd_nl80211;
	struct response vendreturn;
};
/* flags used in struct ieee80211_if_managed.flags */
enum ieee80211_sta_flags {
	ATBM_IEEE80211_STA_BEACON_POLL	= BIT(0),
	ATBM_IEEE80211_STA_CONNECTION_POLL	= BIT(1),
	ATBM_IEEE80211_STA_CONTROL_PORT	= BIT(2),
	ATBM_IEEE80211_STA_DISABLE_11N	= BIT(4),
	ATBM_IEEE80211_STA_CSA_RECEIVED	= BIT(5),
	ATBM_IEEE80211_STA_MFP_ENABLED	= BIT(6),
	ATBM_IEEE80211_STA_UAPSD_ENABLED	= BIT(7),
	ATBM_IEEE80211_STA_NULLFUNC_ACKED	= BIT(8),
	ATBM_IEEE80211_STA_RESET_SIGNAL_AVE	= BIT(9),
};
enum ieee80211_bss_change {
	ATBM_BSS_CHANGED_ASSOC		= 1<<0,
	ATBM_BSS_CHANGED_ERP_CTS_PROT	= 1<<1,
	ATBM_BSS_CHANGED_ERP_PREAMBLE	= 1<<2,
	ATBM_BSS_CHANGED_ERP_SLOT		= 1<<3,
	ATBM_BSS_CHANGED_HT 		= 1<<4,
	ATBM_BSS_CHANGED_BASIC_RATES		= 1<<5,
	ATBM_BSS_CHANGED_BEACON_INT 	= 1<<6,
	ATBM_BSS_CHANGED_BSSID		= 1<<7,
	ATBM_BSS_CHANGED_BEACON 	= 1<<8,
	ATBM_BSS_CHANGED_BEACON_ENABLED = 1<<9,
	ATBM_BSS_CHANGED_CQM			= 1<<10,
	ATBM_BSS_CHANGED_IBSS		= 1<<11,
	ATBM_BSS_CHANGED_ARP_FILTER 	= 1<<12,
	ATBM_BSS_CHANGED_QOS			= 1<<13,
	ATBM_BSS_CHANGED_IDLE		= 1<<14,
	ATBM_BSS_CHANGED_SSID		= 1<<15,
	ATBM_BSS_CHANGED_PS 		= 1<<16,
	ATBM_BSS_CHANGED_CHANNEL		= 1<<17, // XXX: COMBO: should this be merged with _HT?
	ATBM_BSS_CHANGED_RETRY_LIMITS	= 1<<18,
	ATBM_BSS_CHANGED_P2P_PS 	= 1<<19,
#ifdef IPV6_FILTERING
	ATBM_BSS_CHANGED_NDP_FILTER 	= 1<<20,

#endif /*IPV6_FILTERING*/

	/* when adding here, make sure to change ieee80211_reconfig */
};
atbm_uint8 * atbmwifi_ieee80211_add_wpa_ie(struct atbmwifi_vif *priv,atbm_uint8 * eid);
atbm_uint8 * atbmwifi_ieee80211_add_ht_ie(struct atbmwifi_vif *priv,struct atbmwifi_ieee80211_supported_band *band,atbm_uint8 * eid);
atbm_uint8 * atbmwifi_ieee80211_add_ht_operation(struct atbmwifi_vif *priv, atbm_uint8 *eid);
int atbmwifi_ieee80211_data_to_8023(struct atbm_buff *skb, const atbm_uint8 *addr,enum atbm_nl80211_iftype iftype);
unsigned int  atbmwifi_ieee80211_hdrlen(atbm_uint16 fc);
int atbmwifi_ieee80211_channel_to_frequency(int chan, enum atbmwifi_ieee80211_band band);
int atbmwifi_ieee80211_data_from_8023(struct atbm_buff *skb, const atbm_uint8 *addr,
			     enum atbm_nl80211_iftype iftype, atbm_uint8 *bssid, ATBM_BOOL qos,atbm_uint8 encrype);
atbm_void atbmwifi_ieee80211_connection_loss(struct atbmwifi_vif *priv);
 int atbmwifi_ap_deauth(struct atbmwifi_vif *priv,atbm_uint8 * StaMac);
int atbmwifi_ieee80211_rx_irqsafe(struct atbmwifi_vif *priv,struct atbm_buff *skb);
atbm_void atbmwifi_ieee80211_ht_cap_ie_to_sta_ht_cap(struct atbmwifi_ieee80211_supported_band *sband,
				       struct atbmwifi_ieee80211_ht_cap *ht_cap_ie,
				       struct atbmwifi_ieee80211_sta_ht_cap *ht_cap);
atbm_uint8 *atbmwifi_find_ie(atbm_uint8 eid, const atbm_uint8 *ies, int len);
atbm_void atbmwifi_ieee80211_get_sta_rateinfo(struct atbmwifi_cfg80211_rate *rate,struct atbmwifi_ieee80211_supported_band *band,atbm_uint8 *supp_rates,int supp_rates_len);
atbm_uint8 * atbmwifi_ieee80211_add_rate_ie(atbm_uint8 * pos,ATBM_BOOL no_cck,atbm_uint32 mask);
atbm_uint8 * atbmwifi_ieee80211_add_rate_ie_from_ap(atbm_uint8 * pos,ATBM_BOOL no_cck,atbm_uint32 mask, atbm_uint32 basic_mask);
atbm_void atbmwifi_ieee80211_channel_country(struct atbmwifi_common *hw_priv,int country);
atbm_void atbmwifi_ieee80211_parse_qos(struct atbmwifi_vif *priv,struct atbm_buff *skb);
 atbm_void atbmwifi_ieee80211_deliver_skb(struct atbmwifi_vif *priv,struct atbm_buff *skb,atbm_uint16 *need_free);
struct atbmwifi_cfg *atbmwifi_get_config(struct atbmwifi_vif *priv);
atbm_void atbmwifi_start_wifimode(struct atbmwifi_vif *start_priv,enum atbm_nl80211_iftype start_type);
atbm_void atbmwifi_stop_wifimode(struct atbmwifi_vif *stop_priv,enum atbm_nl80211_iftype stop_type);
struct atbmwifi_vif *atbmwifi_config_get_priv(struct atbmwifi_cfg *config);
ATBM_BOOL atbmwifi_iee80211_check_combination(struct atbmwifi_vif *ignore_priv,atbm_uint8 combination_channel);
atbm_void atbmwifi_iee80211_unify_channel_type(struct atbmwifi_vif *ignore_priv, atbm_uint32 channel_type);
int atbmwifi_iee80211_peerif_channel_type(struct atbmwifi_vif *ignore_priv);
struct atbmwifi_vif * atbmwifi_iee80211_getvif_by_name
	(struct atbmwifi_common	*hw_priv,char *name);
ATBM_BOOL atbmwifi_ieee80211_check_alive_if(struct atbmwifi_common	*hw_priv,struct atbmwifi_vif *ignore_priv);

int atbmwifi_buffed_timeout(struct atbmwifi_vif *priv);
#if CONFIG_WPS
atbm_uint8 * atbmwifi_ieee80211_add_preq_wps_ie(struct atbmwifi_vif *priv,atbm_uint8 * pos);
atbm_uint8 * atbmwifi_ieee80211_add_assocreq_wps_ie(struct atbmwifi_vif *priv,atbm_uint8 * pos);
#endif
void ___ieee80211_stop_rx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				     atbm_uint16 initiator, atbm_uint16 reason, ATBM_BOOL stop);
void __ieee80211_stop_rx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				    atbm_uint16 initiator, atbm_uint16 reason, ATBM_BOOL stop);

void ieee80211_sta_tear_down_BA_sessions(struct atbmwifi_sta_priv *sta_priv, ATBM_BOOL tx);
void ieee80211_process_delba(struct atbmwifi_common *hw_priv,
			     struct atbmwifi_sta_priv *sta_priv,
			     struct atbmwifi_ieee80211_mgmt *mgmt, int len);
void ieee80211_process_addba_request(struct atbmwifi_common *hw_priv,
				     struct atbmwifi_sta_priv *sta_priv,
				     struct atbmwifi_ieee80211_mgmt *mgmt,
				     int len);
void ieee80211_process_addba_resp(struct atbmwifi_common *hw_priv,
				  struct atbmwifi_sta_priv *sta_priv,
				  struct atbmwifi_ieee80211_mgmt *mgmt,
				  atbm_size_t len);
int __ieee80211_stop_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				   enum ieee80211_back_parties initiator,
				   ATBM_BOOL tx);
int ___ieee80211_stop_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid,
				    enum ieee80211_back_parties initiator,
				    ATBM_BOOL tx);
int ieee80211_stop_tx_ba_session(struct atbmwifi_sta_priv *sta_priv, atbm_uint16 tid);
void ieee80211_tx_ba_session_handle_start(struct atbmwifi_sta_priv *sta_priv, int tid);
void ieee80211_ba_session_work(struct atbm_work_struct *work);
void sta_addba_resp_timer_expired(atbm_void *data1,atbm_void *data2);
void sta_rx_agg_session_timer_expired(atbm_void *data1,atbm_void *data2);

struct tcpip_opt{
	atbm_void (*net_init)(struct atbm_net_device *dev);
	atbm_void (*net_enable)(struct atbm_net_device *dev);
	atbm_void (*net_disable)(struct atbm_net_device *dev);
	atbm_uint32 (*net_rx)(struct atbm_net_device *dev, struct atbm_buff * skb);
	atbm_void (*net_tx_done)(struct atbm_net_device *dev);
	atbm_void (*net_start_queue)(struct atbm_net_device *dev,int);
	atbm_void (*net_stop_queue)(struct atbm_net_device *dev,int);
	atbm_void (*net_task_event)(struct atbm_net_device *dev);	
};
struct atbm_net_device_ops{
	int	(*ndo_open)(struct atbmwifi_vif *priv);
	int (*ndo_stop)(struct atbmwifi_vif *priv);
	int  (*ndo_start_xmit)(struct atbmwifi_vif *priv,struct atbm_buff *skb );
	int (*ndo_set_mac_address)(struct atbmwifi_vif *priv,atbm_uint8 *addr);
};
extern struct tcpip_opt * tcp_opt;

/**
 * ffs - find first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int atbm_ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}
static inline int atbm_fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

#endif   /*ATBMWIFI_NET_H*/

