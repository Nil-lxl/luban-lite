/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef WPA_SUPPLICANT_I_H
#define WPA_SUPPLICANT_I_H

#if CONFIG_WPS
#include "wps_main.h"
#include "wps.h"
#endif
/**
 * struct wpa_config - wpa_supplicant configuration data
 *
 * This data structure is presents the per-interface (radio) configuration
 * data. In many cases, there is only one struct wpa_config instance, but if
 * more than one network interface is being controlled, one instance is used
 * for each.
 */
struct wpa_config {
	/**
	 * ssid - Head of the global network list
	 *
	 * This is the head for the list of all the configured networks.
	 */
	struct wpa_ssid *ssid;
	atbm_uint8 		ap_scan;
	int    fast_reauth;
};

struct wpa_supplicant {
	struct atbmwifi_vif *priv;
	int countermeasures;
	atbm_uint8 *own_addr;
	atbm_uint8 bssid[ATBM_ETH_ALEN];
	atbm_uint8 reassociate; /* reassociation requested */
	atbm_uint8 disconnected; /* all connections disabled; i.e., do no reassociate
			   * before this has been cleared */
	atbm_uint8 connect_retry;
	atbm_uint8 ap_ies_from_associnfo;
	struct wpa_bss *current_bss;/*****Ô­À´±»ÆÁ±Îµôåå******/
	unsigned int assoc_freq;
	/* Selected configuration (based on Beacon/ProbeResp WPA IE) */
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	int mgmt_group_cipher;
	int scan_runs; /* number of scan runs since WPS was started */
	atbm_uint32 wps_pin_start_time;
#define WILDCARD_SSID_SCAN ((struct wpa_ssid *) 1)

	struct atbmwifi_wpa_sm *wpa;

	enum atbm_wpa_states wpa_state;
	int eapol_received; /* number of EAPOL packets received after the
			     * previous association event */
	unsigned char last_eapol_src[ATBM_ETH_ALEN];
	unsigned int drv_flags;
#if CONFIG_WPS
	struct eap_wsc_data *wsc_data;
	atbm_uint8 *pin;
	enum {
		WPS_MODE_UNKNOWN = 0,
		WPS_MODE_PBC,
		WPS_MODE_PIN,
	} wps_mode;
	atbm_uint8 wps_ap_cnt;
#endif
#if CONFIG_SAE
	struct sae_data sae;
	struct wpabuf *sae_token;
	int sae_group_index;
	struct wpabuf *sae_data;
	int sae_start;
	atbm_uint32 sae_pmksa_caching:1;
	int sae_pmk_set;
	int sae_groups[10];
#endif
#if CONFIG_IEEE80211W
	atbm_uint32 last_unprot_disconnect;
	atbm_uint32 sa_query_start;
	int sa_query_count;
	atbm_uint8 *sa_query_trans_id;
#endif
};
#endif /*WPA_SUPPLICANT_I_H*/
