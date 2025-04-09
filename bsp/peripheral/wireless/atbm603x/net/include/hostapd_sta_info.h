
 /**************************************************************************************************************
  * altobeam RTOS WSM host interface (HI) implementation
  *
  * Copyright (c) 2018, altobeam.inc   All rights reserved.
  *
  *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
  *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
 *****************************************************************************************************************/

#ifndef STA_INFO_H
#define STA_INFO_H

/* STA flags */
#define WLAN_STA_AUTH BIT(0)
#define WLAN_STA_ASSOC BIT(1)
#define WLAN_STA_PS BIT(2)
#define WLAN_STA_TIM BIT(3)
#define WLAN_STA_PERM BIT(4)
#define WLAN_STA_AUTHORIZED BIT(5)
#define WLAN_STA_PENDING_POLL BIT(6) /* pending activity poll not ACKed */
#define WLAN_STA_SHORT_PREAMBLE BIT(7)
#define WLAN_STA_PREAUTH BIT(8)
#define WLAN_STA_WMM BIT(9)
#define WLAN_STA_MFP BIT(10)
#define WLAN_STA_HT BIT(11)
#define WLAN_STA_WPS BIT(12)
#define WLAN_STA_MAYBE_WPS BIT(13)
#define WLAN_STA_WDS BIT(14)
#define WLAN_STA_ASSOC_REQ_OK BIT(15)
#define WLAN_STA_BLOCK_BA BIT(16)
#define WLAN_STA_GAS BIT(17)
#define WLAN_STA_VHT BIT(18)
#define WLAN_STA_SP BIT(19)
#define WLAN_STA_UAPSD BIT(20)
#define WLAN_STA_PS_DRIVER BIT(21)
#define WLAN_STA_EOSP BIT(22)
#define WLAN_STA_PENDING_DISASSOC_CB BIT(29)
#define WLAN_STA_PENDING_DEAUTH_CB BIT(30)
#define WLAN_STA_NONERP BIT(31)

/* Maximum number of supported rates (from both Supported Rates and Extended
 * Supported Rates IEs). */
#define WLAN_SUPP_RATES_MAX 32


struct hostapd_sta_info {
//	struct hostapd_sta_info *next; /* next entry in sta list */
	//struct hostapd_sta_info *hnext; /* next entry in hash table list */
	atbm_uint8 addr[6];
	atbm_uint16 aid; /* STA's unique AID (1 .. 2007) or 0 if not yet assigned */
	atbm_uint32 flags; /* Bitfield of WLAN_STA_* */
	atbm_uint16 capability;
	atbm_uint16 listen_interval; /* or beacon_int for APs */
	atbm_uint8 supported_rates[WLAN_SUPP_RATES_MAX];
	int supported_rates_len;
	atbm_uint8 qosinfo; /* Valid when WLAN_STA_WMM is set */

	unsigned int nonerp_set:1, 
		no_short_slot_time_set:1,
		no_short_preamble_set:1,
		no_ht_gf_set:1,
		no_ht_set:1,
		ht_20mhz_set:1,
		no_p2p_set:1;

	atbm_uint16 auth_alg;

	enum {
		STA_START = 0, STA_DISASSOC,STA_HANDSHAKE
	} timeout;

	atbm_uint16 deauth_reason;
	atbm_uint16 disassoc_reason;

	/* IEEE 802.1X related data */
//	struct eapol_state_machine *eapol_sm;
     
	unsigned long last_rx_bytes;
	unsigned long last_tx_bytes;
 
	atbm_uint8 *challenge; /* IEEE 802.11 Shared Key Authentication Challenge */

	struct atbmwifi_wpa_state_machine *atbmwifi_wpa_sm;

//	struct hostapd_ssid *ssid; /* SSID selection based on (Re)AssocReq */
	//struct hostapd_ssid *ssid_probe; /* SSID selection based on ProbeReq */

	atbm_uint8 *psk; /* PSK from RADIUS authentication server */

	//char *identity; /* User-Name from RADIUS */

	struct atbmwifi_ieee80211_ht_capabilities *ht_capabilities;
 
#if CONFIG_IEEE80211W
	int sa_query_count; /* number of pending SA Query requests;
			     * 0 = no SA Query in progress */
	int sa_query_timed_out;
	atbm_uint8 *sa_query_trans_id; /* buffer of WLAN_SA_QUERY_TR_ID_LEN *
				* sa_query_count octets of pending SA Query
				* transaction identifiers */
	//struct os_time sa_query_start;
#endif /* CONFIG_IEEE80211W */

#if 0
	struct wpabuf *wps_ie; /* WPS IE from (Re)Association Request */
	struct wpabuf *p2p_ie; /* P2P IE from (Re)Association Request */
#endif
#if CONFIG_SAE
	struct sae_data sae;
#endif
};


/* Default value for maximum station inactivity. After AP_MAX_INACTIVITY has
 * passed since last received frame from the station, a nullfunc data frame is
 * sent to the station. If this frame is not acknowledged and no other frames
 * have been received, the station will be disassociated after
 * AP_DISASSOC_DELAY seconds. Similarly, the station will be deauthenticated
 * after AP_DEAUTH_DELAY seconds has passed after disassociation. */
#define AP_MAX_INACTIVITY (5 * 60)
#define AP_DISASSOC_DELAY (1)
#define AP_DEAUTH_DELAY (1)
/* Number of seconds to keep STA entry with Authenticated flag after it has
 * been disassociated. */
#define AP_MAX_INACTIVITY_AFTER_DISASSOC (1 * 30)
/* Number of seconds to keep STA entry after it has been deauthenticated. */
#define AP_MAX_INACTIVITY_AFTER_DEAUTH (1 * 5)

atbm_void ap_sta_del(struct hostapd_data *hostapd,struct hostapd_sta_info *sta);
struct hostapd_sta_info * ap_get_sta(struct hostapd_data * hapd, const atbm_uint8 *sta);
struct hostapd_sta_info * ap_sta_add(struct hostapd_data *hapd, const atbm_uint8 *addr,atbm_uint8 linkid);
//struct hostapd_data;
#if 0
int ap_for_each_sta(struct hostapd_data *hapd,
		    int (*cb)(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
			      atbm_void *ctx),
		    atbm_void *ctx);
struct hostapd_sta_info * ap_get_sta(struct hostapd_data *hapd, const atbm_uint8 *sta);
atbm_void ap_free_sta(struct hostapd_data *hapd, struct hostapd_sta_info *sta);
atbm_void hostapd_free_stas(struct hostapd_data *hapd);
atbm_void ap_handle_timer(atbm_void *eloop_ctx, atbm_void *timeout_ctx);
atbm_void ap_sta_session_timeout(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
			    atbm_uint32 session_timeout);
atbm_void ap_sta_no_session_timeout(struct hostapd_data *hapd,
			       struct hostapd_sta_info *sta);
struct hostapd_sta_info * ap_sta_add(struct hostapd_data *hapd, const atbm_uint8 *addr);
atbm_void ap_sta_disassociate(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
			 atbm_uint16 reason);
atbm_void ap_sta_deauthenticate(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
			   atbm_uint16 reason);
#if CONFIG_WPS
int ap_sta_wps_cancel(struct hostapd_data *hapd,
		      struct hostapd_sta_info *sta, atbm_void *ctx);
#endif /* CONFIG_WPS */

atbm_void ap_sta_start_sa_query(struct hostapd_data *hapd, struct hostapd_sta_info *sta);
atbm_void ap_sta_stop_sa_query(struct hostapd_data *hapd, struct hostapd_sta_info *sta);
int ap_check_sa_query_timeout(struct hostapd_data *hapd, struct hostapd_sta_info *sta);
atbm_void ap_sta_disconnect(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
		       const atbm_uint8 *addr, atbm_uint16 reason);

atbm_void ap_sta_set_authorized(struct hostapd_data *hapd,
			   struct hostapd_sta_info *sta, int authorized);
 int ap_sta_is_authorized(struct hostapd_sta_info *sta)
{
	return sta->flags & WLAN_STA_AUTHORIZED;
}

atbm_void ap_sta_deauth_cb(struct hostapd_data *hapd, struct hostapd_sta_info *sta);
atbm_void ap_sta_disassoc_cb(struct hostapd_data *hapd, struct hostapd_sta_info *sta);
#endif
#endif /* STA_INFO_H */
