/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef _HOSTAPD_MAIN_H
#define _HOSTAPD_MAIN_H
#include "wpa_auth_i.h"

#if CONFIG_WPS
#include "wps.h"
#endif

typedef enum {
	WPA_AUTH, WPA_ASSOC, WPA_DISASSOC, WPA_DEAUTH, WPA_REAUTH,
	WPA_REAUTH_EAPOL, WPA_ASSOC_FT
} wpa_event;

#define WPA_SEND_EAPOL_TIMEOUT 5 //second

#if CONFIG_SAE
struct sae_password_entry {
	struct sae_password_entry *next;
	char *password;
	char *identifier;
	atbm_uint8 peer_addr[ATBM_ETH_ALEN];
	int vlan_id;
};
#endif
/**
 * struct hostapd_data - hostapd per-BSS data structure
 */

struct hostapd_data {
    struct atbmwifi_vif *priv;
	atbm_uint8 own_addr[ATBM_ETH_ALEN];	
	struct atbmwifi_wpa_group group;
	int num_sta; /* number of entries in sta_list */
	struct hostapd_sta_info *sta_list[ATBMWIFI__MAX_STA_IN_AP_MODE]; /* STA info list head */
	int michael_mic_failures;
	int tkip_countermeasures;
#if CONFIG_WPS
	struct wps_context *wps;
	struct wpabuf *wps_beacon_ie;
	struct wpabuf *wps_probe_resp_ie;
	struct wps_data *wpsdata;
	struct wpabuf *wps_last_rx_data;
	struct atbmwifi_ieee802_1x_hdr *wps_tx_hdr;
	int wps_tx_hdr_len;
#endif
#if CONFIG_SAE
	//struct sae_data sae;
	//struct wpabuf *sae_token;
	//int sae_group_index;
	//struct wpabuf *sae_data;
	//int sae_start;
	//atbm_uint32 sae_pmksa_caching:1;
	//int sae_pmk_set;
	struct wpabuf *sae_data;
	char *wpa_passphrase;
	struct sae_password_entry *sae_passwords;
	atbm_uint8 sae_token_key[8];
	atbm_uint16 sae_token_idx;
	atbm_uint16 sae_pending_token_idx[256];
	atbm_uint32 last_sae_token_key_update;
	atbm_uint32 sae_anti_clogging_threshold;
	atbm_uint32 sae_sync;
	int sae_groups[10];
	struct rsn_pmksa_cache *pmksa; /* PMKSA cache */
#endif
};

atbm_void hostap_sta_del(struct atbmwifi_vif *priv,atbm_uint8 * staMacAddr);
#define hostapd_send_eapol(priv,da,proto,buf,len)  wpa_drv_send_eapol(priv,da,proto,buf,len)
#define hostapd_init_extra_ie(priv) wpa_comm_init_extra_ie(priv)
#if CONFIG_WPS
atbm_void hostapd_wps_handshake_process(struct atbmwifi_vif *priv, struct hostapd_sta_info *sta,
							struct atbmwifi_ieee802_1x_hdr *hdr, atbm_uint16 datalen);
#endif
#endif
