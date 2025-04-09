/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"
#define ATBM_CHECK_AGRV_AVAILABLE(PARAM_1,PARAM_2) do{ \
		atbm_uint8 ret; \
		if(PARAM_1==ATBM_NULL){ \
			ret=-1; \
			goto EXIT;\
		} \
		if(PARAM_2==ATBM_NULL){\
			ret= -2; \
			goto EXIT;\
		} \
	}while(0)

struct wpa_supplicant *g_wpa_s;
atbm_void wpa_authen_assc_timeout(atbm_void * data1,atbm_void *data);
int wpa_wsc_rx_process(struct atbmwifi_vif *priv, atbm_uint8 *buf, atbm_size_t len);
int wpa_wsc_tx_process(atbm_void *ctx, int type, const atbm_uint8 *buf, atbm_size_t len);

#if CONFIG_SAE
static void wpa_sm_pmksa_free_cb(struct rsn_pmksa_cache_entry *entry,
			  void *ctx, enum pmksa_free_reason reason)
{
 struct atbmwifi_wpa_sm *sm = ctx;
 int deauth = 0;

 wpa_printf(MSG_DEBUG, "RSN: PMKSA cache entry free_cb: "
	 MACSTR " reason=%d", MAC2STR(entry->aa), reason);

 if (sm->cur_pmksa == entry) {
	 wpa_printf(MSG_DEBUG,
		 "RSN: %s current PMKSA entry",
		 reason == PMKSA_REPLACE ? "replaced" : "removed");
	 pmksa_cache_clear_current(sm);

	 /*
	  * If an entry is simply being replaced, there's no need to
	  * deauthenticate because it will be immediately re-added.
	  * This happens when EAP authentication is completed again
	  * (reauth or failed PMKSA caching attempt).
	  */
	 if (reason != PMKSA_REPLACE)
		 deauth = 1;
 }

 if (reason == PMKSA_EXPIRE &&
	 (sm->pmk_len == entry->pmk_len &&
	 atbm_memcmp(sm->pmk, entry->pmk, sm->pmk_len) == 0)) {
	 wpa_printf(MSG_DEBUG,
		 "RSN: deauthenticating due to expired PMK");
	 pmksa_cache_clear_current(sm);
	 deauth = 1;
 }

 if (deauth) {
	 sm->pmk_len = 0;
	 atbm_memset(sm->pmk, 0, sizeof(sm->pmk));
 }
}
#endif

 struct wpa_supplicant * init_wpa_supplicant(struct atbmwifi_vif *priv)
{
	struct wpa_supplicant *wpa_s;
#if CONFIG_SAE
	int groups[10] = { 19, 20, 21, 0 };
#endif

//	p_info("init_wpa_supplicant");
	wpa_s = (struct wpa_supplicant *)atbm_kzalloc(sizeof(struct wpa_supplicant)
		+sizeof(struct atbmwifi_wpa_sm ),GFP_KERNEL);
	wpa_s->wpa	 = (struct atbmwifi_wpa_sm *)((atbm_uint8 *)(wpa_s)+sizeof(struct wpa_supplicant));
	wpa_s->own_addr = priv->mac_addr;
	wpa_s->wpa->wpa_s = wpa_s;
	wpa_s->priv = priv;
	g_wpa_s = wpa_s;
#if CONFIG_SAE
	wpa_s->wpa->pmksa = pmksa_cache_init(wpa_sm_pmksa_free_cb, wpa_s->wpa, wpa_s->wpa);	
 	atbm_memcpy(wpa_s->sae_groups, groups, sizeof(groups));
#endif

	return wpa_s;
}

 atbm_void free_wpa_supplicant(struct atbmwifi_vif *priv)
{
	if(priv->appdata){
		struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(priv->appdata);
		struct atbmwifi_wpa_sm		   *wpa = wpa_s->wpa;
		wpa_s->wpa_state = ATBM_WPA_DISCONNECTED;
		if(wpa)	{
			if(wpa->ap_wpa_ie){
				atbm_kfree(wpa->ap_wpa_ie);
				wpa->ap_wpa_ie = ATBM_NULL;
				wpa->ap_wpa_ie_len = 0;
			}
			if(wpa->ap_rsn_ie){
				atbm_kfree(wpa->ap_rsn_ie);
				wpa->ap_rsn_ie = ATBM_NULL;
				wpa->ap_rsn_ie_len = 0;
			}

			if(wpa->assoc_wpa_ie){
				atbm_kfree(wpa->assoc_wpa_ie);
				wpa->assoc_wpa_ie = ATBM_NULL;
				wpa->assoc_wpa_ie_len = 0;
			}
		}
#if CONFIG_SAE
		wpabuf_free(wpa_s->sae_token);
		wpa_s->sae_token = ATBM_NULL;
		wpabuf_free(wpa_s->sae_data);
		wpa_s->sae_data = ATBM_NULL;
		sae_clear_data(&wpa_s->sae);
		pmksa_cache_deinit(wpa_s->wpa->pmksa);
#endif /* CONFIG_SAE */
		atbm_kfree(priv->appdata);
		priv->appdata = ATBM_NULL;
	}
}

#if CONFIG_SAE
atbm_void wpa_sm_set_pmk(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *pmk, atbm_size_t pmk_len,
	const atbm_uint8 *pmkid, const atbm_uint8 *bssid)
{
	if (sm == NULL)
		return;

	wpa_hexdump_key(MSG_DEBUG, "WPA: Set PMK based on external data",
		pmk, pmk_len);
	sm->pmk_len = pmk_len;
	atbm_memcpy(sm->pmk, pmk, pmk_len);

#if CONFIG_IEEE80211R
	/* Set XXKey to be PSK for FT key derivation */
	sm->xxkey_len = pmk_len;
	atbm_memcpy(sm->xxkey, pmk, pmk_len);
#endif /* CONFIG_IEEE80211R */
	if (bssid) {
		pmksa_cache_add(sm->pmksa, pmk, pmk_len, pmkid, ATBM_NULL, 0,
			bssid, sm->wpa_s->priv->mac_addr,
			&sm->wpa_s->priv->config, sm->key_mgmt, ATBM_NULL);
	}
}

/**
* wpa_sm_set_pmk_from_pmksa - Set PMK based on the current PMKSA
* @sm: Pointer to WPA state machine data from wpa_sm_init()
*
* Take the PMK from the current PMKSA into use. If no PMKSA is active, the PMK
* will be cleared.
*/
atbm_void wpa_sm_set_pmk_from_pmksa(struct atbmwifi_wpa_sm *sm)
{
	if (sm == NULL)
	return;

	if (sm->cur_pmksa) {
		wpa_hexdump_key(MSG_DEBUG,
			"WPA: Set PMK based on current PMKSA",
			sm->cur_pmksa->pmk, sm->cur_pmksa->pmk_len);
		sm->pmk_len = sm->cur_pmksa->pmk_len;
		atbm_memcpy(sm->pmk, sm->cur_pmksa->pmk, sm->pmk_len);
	} else {
		wpa_printf(MSG_DEBUG, "WPA: No current PMKSA - clear PMK");
		sm->pmk_len = 0;
		atbm_memset(sm->pmk, 0, ATBM_PMK_LEN);
	}
}

static int index_within_array(const int *array, int idx)
{
	int i;
	for (i = 0; i < idx; i++) {
		if (array[i] <= 0)
			return 0;
	}
	return 1;
}

int sme_set_sae_group(struct wpa_supplicant *wpa_s)
{
	int *groups = wpa_s->sae_groups;
	int default_groups[] = { 19, 20, 21, 0 };

	if (!groups || groups[0] <= 0)
		groups = default_groups;

	/* Configuration may have changed, so validate current index */
	if (!index_within_array(groups, wpa_s->sae_group_index))
		return -1;

	for (;;) {
		int group = groups[wpa_s->sae_group_index];
		if (group <= 0)
			break;
		if (sae_set_group(&wpa_s->sae, group) == 0) {
			wpa_printf(MSG_DEBUG, "SME: Selected SAE group %d",
				wpa_s->sae.group);
			return 0;
		}
		wpa_s->sae_group_index++;
	}

	return -1;
}


static struct wpabuf * sme_auth_build_sae_commit(struct wpa_supplicant *wpa_s,
						 struct atbmwifi_vif *priv,
						 const atbm_uint8 *bssid, int external,
						 int reuse)
{
	struct wpabuf *buf;
	atbm_size_t len;
	const char *password;
	struct atbmwifi_cfg *config = &priv->config;

#ifdef CONFIG_TESTING_OPTIONS
	if (wpa_s->sae_commit_override) {
		wpa_printf(MSG_DEBUG, "SAE: TESTING - commit override");
		buf = wpabuf_alloc(4 + wpabuf_len(wpa_s->sae_commit_override));
		if (!buf)
			return NULL;
		if (!external) {
			wpabuf_put_le16(buf, 1); /* Transaction seq# */
			wpabuf_put_le16(buf, ATBM_WLAN_STATUS_SUCCESS);
		}
		wpabuf_put_buf(buf, wpa_s->sae_commit_override);
		return buf;
	}
#endif /* CONFIG_TESTING_OPTIONS */

	password = config->password;

	if (!password) {
		wpa_printf(MSG_DEBUG, "SAE: No password available");
		return ATBM_NULL;
	}

	if (reuse && wpa_s->sae.tmp &&
	    atbm_memcmp(bssid, wpa_s->sae.tmp->bssid, ATBM_ETH_ALEN) == 0) {
		wpa_printf(MSG_DEBUG,
			   "SAE: Reuse previously generated PWE on a retry with the same AP");
		goto reuse_data;
	}

	if (sme_set_sae_group(wpa_s) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Failed to select group");
		return ATBM_NULL;
	}

	if (sae_prepare_commit(priv->mac_addr, bssid,
			       (atbm_uint8 *) password, strlen(password),
			       config->sae_password_id,
			       &wpa_s->sae) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not pick PWE");
		return NULL;
	}
	if (wpa_s->sae.tmp)
		atbm_memcpy(wpa_s->sae.tmp->bssid, bssid, ATBM_ETH_ALEN);

reuse_data:
	len = wpa_s->sae_token ? wpabuf_len(wpa_s->sae_token) : 0;
	if (config->sae_password_id)
		len += 4 + strlen(config->sae_password_id);
	buf = wpabuf_alloc(4 + SAE_COMMIT_MAX_LEN + len);
	if (buf == ATBM_NULL)
		return ATBM_NULL;
	if (!external) {
		wpabuf_put_le16(buf, 1); /* Transaction seq# */
		wpabuf_put_le16(buf, ATBM_WLAN_STATUS_SUCCESS);
	}
	sae_write_commit(&wpa_s->sae, buf, wpa_s->sae_token,
			 config->sae_password_id);

	return buf;
}


static struct wpabuf * sme_auth_build_sae_confirm(struct wpa_supplicant *wpa_s,
						  int external)
{
	struct wpabuf *buf;

	buf = wpabuf_alloc(4 + SAE_CONFIRM_MAX_LEN);
	if (buf == NULL)
		return NULL;

	if (!external) {
		wpabuf_put_le16(buf, 2); /* Transaction seq# */
		wpabuf_put_le16(buf, ATBM_WLAN_STATUS_SUCCESS);
	}
	sae_write_confirm(&wpa_s->sae, buf);

	return buf;
}
#endif /* CONFIG_SAE */

/*
static void wpa_sm_set_state(struct atbmwifi_wpa_sm *sm, enum wpa_states state)
{
	sm->wpa_s->wpa_state = state;
}


static enum wpa_states wpa_sm_get_state(struct atbmwifi_wpa_sm *sm)
{
	return sm->wpa_s->wpa_state;
}
*/
#if 0
atbm_void wpas_notify_state_changed(struct wpa_supplicant *wpa_s,
			       enum wpa_states new_state,
			       enum wpa_states old_state)
{
#if CONFIG_P2P
	if (new_state == ATBM_WPA_COMPLETED)
		wpas_p2p_notif_connected(wpa_s);
	else if (old_state >= ATBM_WPA_ASSOCIATED && new_state < ATBM_WPA_ASSOCIATED)
		wpas_p2p_notif_disconnected(wpa_s);
#endif /* CONFIG_P2P */
	
}
#endif
/*
void wpa_supplicant_set_state(struct wpa_supplicant *wpa_s,
			      enum wpa_states state)
{
	enum wpa_states old_state = wpa_s->wpa_state;

	wpa_s->wpa_state = state;

	if (wpa_s->wpa_state != old_state) {
		wpas_notify_state_changed(wpa_s, wpa_s->wpa_state, old_state);
	}

}

*/
 atbm_void atbmwifi_wpa_sm_notify_assoc(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *bssid,atbm_uint16 linkid)
{
	int clear_ptk = 1;

	struct atbmwifi_vif *priv = sm->wpa_s->priv;

	if (sm == ATBM_NULL)
	{
		wifi_printk(WIFI_DBG_ERROR,"sm err\n");
		return;
	}

	atbm_memcpy(sm->bssid, bssid, ATBM_ETH_ALEN);
	atbm_memcpy(sm->own_addr, priv->mac_addr, 6);
	atbm_memset(sm->rx_replay_counter, 0, ATBM_WPA_REPLAY_COUNTER_LEN);
	sm->rx_replay_counter_set = 0;
	sm->renew_snonce = 1;
	sm->linkid = linkid;


	if (clear_ptk) {
		/*
		 * IEEE 802.11, 8.4.10: Delete PTK SA on (re)association if
		 * this is not part of a Fast BSS Transition.
		 */
#if 0
		wpa_printf(MSG_DEBUG, "WPA: Clear old PTK");
#endif
		sm->ptk_set = 0;
		sm->tptk_set = 0;
	}
}


atbm_void wpa_supplicant_event_disauthen(struct atbmwifi_vif *priv,atbm_uint16 res)
{
	wifi_printk(WIFI_WPA, "authen fail(%d)\n\r",res);
	wpa_deauthen(priv);
}
static atbm_void __wpa_supplicant_event_assoc(struct atbmwifi_vif *priv,atbm_uint16 linkid)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(priv->appdata);
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	atbm_uint8 bssid[ATBM_ETH_ALEN];

	atbmwifi_eloop_cancel_timeout(wpa_authen_assc_timeout,(atbm_void *)priv,ATBM_NULL);
	if(atbmwifi_get_config(priv)->wpa)
		atbmwifi_eloop_register_timeout(ATBM_WIFI_AUTH_ASSOC_TIMEOUT,0,wpa_authen_assc_timeout,(atbm_void *)priv,ATBM_NULL);
	wpa_s->wpa_state = ATBM_WPA_ASSOCIATED;
	atbm_memcpy(bssid, priv->bssid, ATBM_ETH_ALEN);
	atbmwifi_wpa_sm_notify_assoc(wpa_s->wpa, bssid,linkid);
	wpa_s->connect_retry = 0;

	if((config->key_mgmt == ATBM_WPA_KEY_MGMT_NONE)||
		(config->key_mgmt ==ATBM_WPA_KEY_MGMT_WEP)){
		priv->connect_ok = 1;
		priv->scan_expire = 2;
		wpa_s->wpa_state = ATBM_WPA_COMPLETED;
		/*Use fase ps mode,the Time can adjust dymatic,default is 50*/
		atbmwifi_set_pm(priv,ATBM_TRUE,50);
		atbmwifi_event_uplayer(priv,ATBM_WIFI_ENABLE_NET_EVENT,priv->mac_addr);
	}
}

atbm_void atbmwifi_wpa_event_assocated(struct atbmwifi_vif *priv,atbm_uint16 linkid)
{
	__wpa_supplicant_event_assoc(priv,linkid);
}

//this is call in atbmwifi_assoc_success --> call in wpa event, must not call atbmwifi_wpa_event_queue again
/*管理包先调用的WPA_EVENT__RX_PKG 调度一次（这时还是正序的),再调用
//WPA_EVENT__SUPPLICANT_ASSOCIATED调度了一次（这时就是乱序了）
*/
atbm_void wpa_supplicant_event_assoc(struct atbmwifi_vif *priv,atbm_uint16 linkid)
{
	//atbmwifi_wpa_event_queue((atbm_void*)priv,(atbm_void*)&linkid,ATBM_NULL,WPA_EVENT__SUPPLICANT_ASSOCIATED,ATBM_WPA_EVENT_NOACK);
	if(priv==ATBM_NULL){
		wifi_printk(WIFI_WPA,"wpa_event_assoc err\n");
		return;
	}
	
	if(!atbmwifi_is_sta_mode(priv->iftype))	{				
		wifi_printk(WIFI_WPA,"wpa_event_assoc not sta mode\n");
		return;
	}
	atbmwifi_wpa_event_assocated(priv,linkid);
}

atbm_void atbmwifi_wpa_sm_notify_disassoc(struct atbmwifi_wpa_sm *sm)
{
//	rsn_preauth_deinit(sm);
//	if (wpa_sm_get_state(sm) == ATBM_WPA_4WAY_HANDSHAKE)
//		sm->dot11RSNA4WayHandshakeFailures++;
#ifdef CONFIG_TDLS
	wpa_tdls_disassoc(sm);
#endif /* CONFIG_TDLS */

}

atbm_void wpa_supplicant_mark_disassoc(struct wpa_supplicant *wpa_s)
{
	int bssid_changed;


#ifdef CONFIG_AP
	wpa_supplicant_ap_deinit(wpa_s);
#endif /* CONFIG_AP */

	if (wpa_s->wpa_state == ATBM_WPA_INTERFACE_DISABLED)
		return;

//	wpa_supplicant_set_state(wpa_s, ATBM_WPA_DISCONNECTED);
	wpa_s->wpa_state = ATBM_WPA_DISCONNECTED;

	bssid_changed = !atbm_is_zero_ether_addr(wpa_s->bssid);
	atbm_memset(wpa_s->bssid, 0, ATBM_ETH_ALEN);
//	atbm_memset(wpa_s->pending_bssid, 0, ATBM_ETH_ALEN);
#ifdef CONFIG_SME
	wpa_s->sme.prev_bssid_set = 0;
#endif /* CONFIG_SME */
#if CONFIG_P2P
	//atbm_memset(wpa_s->go_dev_addr, 0, ATBM_ETH_ALEN);
#endif /* CONFIG_P2P */
	wpa_s->current_bss = ATBM_NULL;
	wpa_s->assoc_freq = 0;
#if CONFIG_IEEE80211R
#ifdef CONFIG_SME
	if (wpa_s->sme.ft_ies)
		sme_update_ft_ies(wpa_s, ATBM_NULL, ATBM_NULL, 0);
#endif /* CONFIG_SME */
#endif /* CONFIG_IEEE80211R */
#if 0
//	eapol_sm_notify_portEnabled(wpa_s->eapol, FALSE);
//	eapol_sm_notify_portValid(wpa_s->eapol, FALSE);
	if (atbmwifi_wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt))
		eapol_sm_notify_eap_success(wpa_s->eapol, ATBM_FALSE);
	wpa_s->ap_ies_from_associnfo = 0;
	wpa_s->current_ssid = ATBM_NULL;
//	wpa_s->key_mgmt = 0;
#endif
}

#if 0
static atbm_void wpa_supplicant_clear_connection(struct wpa_supplicant *wpa_s,
					    const atbm_uint8 *addr)
{
	struct wpa_ssid *old_ssid;

	wpa_clear_keys(wpa_s, addr);
	old_ssid = wpa_s->current_ssid;
	wpa_supplicant_mark_disassoc(wpa_s);
//	wpa_sm_set_config(wpa_s->wpa, NULL);
//	wpa_sm_set_config(wpa_s->wpa);
#ifdef CONFIG_WAPI
	wapi_sm_init(wpa_s->wapi);
#endif
//	eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
	//if (old_ssid != wpa_s->current_ssid)
	//	wpas_notify_network_changed(wpa_s);
//	atbmwifi_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, NULL);
}
#endif

 atbm_void wpa_supplicant_event_disassoc(struct atbmwifi_vif *priv)
{
	wifi_printk(WIFI_WPA,"%s %d\n",__func__,__LINE__);
	wpa_deauthen(priv);
}
 static int wpa_supplicant_verify_eapol_key_mic(struct atbmwifi_wpa_sm *sm,
					       struct atbmwifi_wpa_eapol_key *key,
					       atbm_uint16 ver,
					       const atbm_uint8 *buf, atbm_size_t len)
{
	atbm_uint8 mic[16];
	int ok = 0;

	atbm_memcpy(mic, key->key_mic, 16);
	if (sm->tptk_set) {
		atbm_memset(key->key_mic, 0, 16);
		wpa_eapol_key_mic(sm->tptk.kck, ver, buf, len,
				  key->key_mic);
		if (atbm_memcmp(mic, key->key_mic, 16) != 0) {
			wpa_printf(MSG_WARNING, "WPA: Invalid EAPOL-Key MIC "
				   "when using TPTK - ignoring TPTK");
		} else {
			ok = 1;
			sm->tptk_set = 0;
			sm->ptk_set = 1;
			atbm_memcpy(&sm->ptk, &sm->tptk, sizeof(sm->ptk));
		}
	}

	if (!ok && sm->ptk_set) {
		atbm_memset(key->key_mic, 0, 16);
		wpa_eapol_key_mic(sm->ptk.kck, ver, buf, len,
				  key->key_mic);
		if (atbm_memcmp(mic, key->key_mic, 16) != 0) {
			wpa_printf(MSG_WARNING, "WPA: Invalid EAPOL-Key MIC "
				   "- dropping packet");
			return -1;
		}
		ok = 1;
	}

	if (!ok) {
		wpa_printf(MSG_WARNING, "WPA: Could not verify EAPOL-Key MIC "
			   "- dropping packet");
		return -1;
	}

	atbm_memcpy(sm->rx_replay_counter, key->replay_counter,
		  ATBM_WPA_REPLAY_COUNTER_LEN);
	sm->rx_replay_counter_set = 1;
	return 0;
}


 atbm_uint8 * wpa_alloc_eapol(atbm_uint8 type,
			    const atbm_void *data, atbm_uint16 data_len,
			    atbm_size_t *msg_len, atbm_void **data_pos)
{
	struct atbmwifi_ieee802_1x_hdr *hdr;

	*msg_len = sizeof(*hdr) + data_len;
	hdr = (struct atbmwifi_ieee802_1x_hdr *)atbm_kmalloc(*msg_len,GFP_KERNEL);
	if (hdr == ATBM_NULL)
		return ATBM_NULL;

	hdr->version =  EAPOL_VERSION;
	hdr->type = type;
	hdr->length = atbm_host_to_be16(data_len);

	if (data)
		atbm_memcpy(hdr + 1, data, data_len);
	else
		atbm_memset(hdr + 1, 0, data_len);

	if (data_pos)
		*data_pos = hdr + 1;

	return (atbm_uint8 *) hdr;
}

static atbm_uint8 * wpa_sm_alloc_eapol(struct atbmwifi_wpa_sm *sm, atbm_uint8 type,
				      const atbm_void *data, atbm_uint16 data_len,
				      atbm_size_t *msg_len, atbm_void **data_pos)
{
	return wpa_alloc_eapol(type, data, data_len,
				    msg_len, data_pos);
}


int wpa_supplicant_parse_ies(const atbm_uint8 *buf, atbm_size_t len,
			     struct wpa_eapol_ie_parse *ie)
{
	const atbm_uint8 *pos, *end;
	int ret = 0;

	atbm_memset(ie, 0, sizeof(*ie));
	for (pos = buf, end = pos + len; pos + 1 < end; pos += 2 + pos[1]) {
		if (pos[0] == 0xdd &&
		    ((pos == buf + len - 1) || pos[1] == 0)) {
			/* Ignore padding */
			break;
		}
		if (pos + 2 + pos[1] > end) {
			wpa_printf(MSG_DEBUG, "WPA: EAPOL-Key Key Data "
				   "underflow (ie=%d len=%d pos=%d)",
				   pos[0], pos[1], (int) (pos - buf));
			wpa_hexdump(MSG_DEBUG, "WPA: Key Data",
					(const atbm_uint8*)buf, len);
			ret = -1;
			break;
		}
		if (*pos == ATBM_WLAN_EID_RSN) {
			ie->rsn_ie = pos;
			ie->rsn_ie_len = pos[1] + 2;
			wpa_hexdump(MSG_DEBUG, "WPA: RSN IE in EAPOL-Key",
				    (const atbm_uint8*)ie->rsn_ie, ie->rsn_ie_len);
#if CONFIG_IEEE80211R
		} else if (*pos == WLAN_EID_MOBILITY_DOMAIN) {
			ie->mdie = pos;
			ie->mdie_len = pos[1] + 2;
			wpa_hexdump(MSG_DEBUG, "WPA: MDIE in EAPOL-Key",
				    (const atbm_uint8*)ie->mdie, ie->mdie_len);
		} else if (*pos == ATBM_WLAN_EID_FAST_BSS_TRANSITION) {
			ie->ftie = pos;
			ie->ftie_len = pos[1] + 2;
			wpa_hexdump(MSG_DEBUG, "WPA: FTIE in EAPOL-Key",
				    (const atbm_uint8*)ie->ftie, ie->ftie_len);
		} else if (*pos == WLAN_EID_TIMEOUT_INTERVAL && pos[1] >= 5) {
			if (pos[2] == WLAN_TIMEOUT_REASSOC_DEADLINE) {
				ie->reassoc_deadline = pos;
				wpa_hexdump(MSG_DEBUG, "WPA: Reassoc Deadline "
					    "in EAPOL-Key",
					    (const atbm_uint8*)ie->reassoc_deadline, pos[1] + 2);
			} else if (pos[2] == WLAN_TIMEOUT_KEY_LIFETIME) {
				ie->key_lifetime = pos;
				wpa_hexdump(MSG_DEBUG, "WPA: KeyLifetime "
					    "in EAPOL-Key",
					    (const atbm_uint8*)ie->key_lifetime, pos[1] + 2);
			} else {
				wpa_hexdump(MSG_DEBUG, "WPA: Unrecognized "
					    "EAPOL-Key Key Data IE",
					    (const atbm_uint8*)pos, 2 + pos[1]);
			}
#endif /* CONFIG_IEEE80211R */
		} else if (*pos == ATBM_WLAN_EID_VENDOR_SPECIFIC) {
			ret = wpa_parse_generic(pos, end, ie);
			if (ret < 0)
				break;
			if (ret > 0) {
				ret = 0;
				break;
			}
		} else {
			wpa_hexdump(MSG_DEBUG, "WPA: Unrecognized EAPOL-Key "
				    "Key Data IE", (const atbm_uint8*)pos, 2 + pos[1]);
		}
	}

	return ret;
}


/*static int wpa_supplicant_get_pmk(struct atbmwifi_wpa_sm *sm,
				  const unsigned char *src_addr,
				  const atbm_uint8 *pmkid)
{
	int i;
	#if 0 
	int abort_cached = 0;
	#endif
	for(i = 0;i< ATBM_PMK_CACHE_NUM ;i++)
	{
		if(atbm_memcmp(sm->pmksa[i].aa,src_addr,ATBM_ETH_ALEN) == 0)
		{
			atbm_memcpy(sm->pmk, sm->pmksa[i].pmk, sm->pmksa[i].pmk_len);
			sm->pmk_len = sm->pmksa[i].pmk_len;
			return 0;
		}
	}
	

	return -2;

}*/


atbm_void wpa_eapol_key_send(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *kck,
			int ver, const atbm_uint8 *dest, atbm_uint16 proto,
			atbm_uint8 *msg, atbm_size_t msg_len, atbm_uint8 *key_mic)
{
	if (atbm_is_zero_ether_addr(dest) && atbm_is_zero_ether_addr(sm->bssid)) {
		/*
		 * Association event was not yet received; try to fetch
		 * BSSID from the driver.
		 */
		if (/*wpa_sm_get_bssid(sm, sm->bssid)*/-1 < 0) {


//			wifi_printk(WIFI_DBG_MSG, "wpa_eapol_key_send: Failed to read BSSID "
//				   "\n\r");

		} else {
			dest = sm->bssid;

//			wifi_printk(WIFI_DBG_MSG, "wpa_eapol_key_send: Use BSSID (" MACSTR
//				   ") as the destination for EAPOL-Key\n\r",
//				   MAC2STR(dest));

		}
	}
	if (key_mic &&
	    wpa_eapol_key_mic(kck, ver, msg, msg_len, key_mic)) {


		wifi_printk(WIFI_WPA, "wpa_eapol_key_send: "
			   "version %d MIC\n\r", ver);


		goto out;
	}

	
	
//	wifi_printk(WIFI_DBG_MSG, "wpa kck mic:mic[1](%x),mic[5](%x),mic[10](%x),mic[15](%x)\n"
//			   ,key_mic[1],key_mic[5],key_mic[10],key_mic[15]);
//	wpa_sm_ether_send(sm, dest, proto, msg, msg_len);

	wpa_drv_send_eapol(sm->wpa_s->priv,dest,proto,msg,msg_len);
//	eapol_sm_notify_tx_eapol_key(sm->eapol);
out:
	atbm_kfree(msg);
}


 int wpa_supplicant_send_2_of_4(struct atbmwifi_wpa_sm *sm, const unsigned char *dst,
			       const struct atbmwifi_wpa_eapol_key *key,
			       int ver, const atbm_uint8 *nonce,
			       const atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len,
			       struct atbmwifi_wpa_ptk *ptk)
{
	atbm_size_t rlen;
	struct atbmwifi_wpa_eapol_key *reply;
	atbm_uint8 *rbuf;
	int ret = 0;

	if (wpa_ie == ATBM_NULL) {
		ret = -1;
		goto __err;
	}
	wifi_printk(WIFI_WPA, "WPA: Tx EAPOL-Key 2/4.....\n");


//	wpa_hexdump(MSG_DEBUG, "WPA: WPA IE for msg 2/4", (const atbm_uint8*)wpa_ie, wpa_ie_len);


//°2è?êí·?
	rbuf = wpa_sm_alloc_eapol(sm, ATBM_IEEE802_1X_TYPE_EAPOL_KEY,
				  ATBM_NULL, sizeof(*reply) + wpa_ie_len,
				  &rlen, (atbm_void *) &reply);
	if (rbuf == ATBM_NULL) {
		ret = -2;
		goto __err;
	}

	reply->type = sm->proto == ATBM_WPA_PROTO_RSN ?
		ATBM_EAPOL_KEY_TYPE_RSN : ATBM_EAPOL_KEY_TYPE_WPA;
	ATBM_WPA_PUT_BE16(reply->key_info,
		     ver | ATBM_WPA_KEY_INFO_KEY_TYPE | ATBM_WPA_KEY_INFO_MIC);
	if (sm->proto == ATBM_WPA_PROTO_RSN)
		ATBM_WPA_PUT_BE16(reply->key_length, 0);
	else
		atbm_memcpy(reply->key_length, key->key_length, 2);
	atbm_memcpy(reply->replay_counter, key->replay_counter,
		  ATBM_WPA_REPLAY_COUNTER_LEN);

	ATBM_WPA_PUT_BE16(reply->key_data_length, wpa_ie_len);
	atbm_memcpy(reply + 1, wpa_ie, wpa_ie_len);


	atbm_memcpy(reply->key_nonce, nonce, ATBM_WPA_NONCE_LEN);

	wifi_printk(WIFI_WPA, "WPA: Tx EAPOL-Key 2/4\n");

	wpa_eapol_key_send(sm, ptk->kck, ver, dst, ATBM_ETH_P_EAPOL,
			   rbuf, rlen, reply->key_mic);

	return 0;
__err:
	wifi_printk(WIFI_WPA, "%s fail ret=%d\n",__func__,ret);

	return ret;
}
 static int wpa_derive_ptk(struct atbmwifi_cfg *config,struct atbmwifi_wpa_sm *sm, const unsigned char *src_addr,
			  const struct atbmwifi_wpa_eapol_key *key,
			  struct atbmwifi_wpa_ptk *ptk)
{
	atbm_size_t ptk_len = sm->pairwise_cipher != ATBM_WPA_CIPHER_TKIP ? 48 : 64;
#if CONFIG_IEEE80211R
	if (atbmwifi_wpa_key_mgmt_ft(sm->key_mgmt))
		return wpa_derive_ptk_ft(sm, src_addr, key, ptk, ptk_len);
#endif /* CONFIG_IEEE80211R */

	wpa_pmk_to_ptk(sm->pmk, sm->pmk_len, "Pairwise key expansion",
		       sm->own_addr, sm->bssid, sm->snonce, key->key_nonce,
		       (atbm_uint8 *) ptk, ptk_len,
		       atbmwifi_wpa_key_mgmt_sha256(sm->key_mgmt));
	return 0;
}

 static atbm_void wpa_supplicant_process_1_of_4(
					  struct atbmwifi_wpa_sm *sm,
					  const unsigned char *src_addr,
					  const struct atbmwifi_wpa_eapol_key *key,
					  atbm_uint16 ver)
{

	struct wpa_eapol_ie_parse ie;
	struct atbmwifi_wpa_ptk *ptk;
	atbm_uint8 buf[8];
//	int res;

	sm->wpa_s->wpa_state = ATBM_WPA_4WAY_HANDSHAKE;
//	wpa_sm_set_state(sm, ATBM_WPA_4WAY_HANDSHAKE);

	wifi_printk(WIFI_WPA, "WPA: RX M1/4-Way Handshake from "
		   MACSTR " (ver=%d)\n\r", MAC2STR(src_addr), ver);


	atbm_memset(&ie, 0, sizeof(ie));

	if (sm->proto == ATBM_WPA_PROTO_RSN) {
		/* RSN: msg 1/4 should contain PMKID for the selected PMK */
		const atbm_uint8 *_buf = (const atbm_uint8 *) (key + 1);
		atbm_size_t len = ATBM_WPA_GET_BE16(key->key_data_length);

		wifi_printk(WIFI_WPA, "RSN: M1/4 key data\n\r");

		wpa_supplicant_parse_ies(_buf, len, &ie);
		if (ie.pmkid) {
			wifi_printk(WIFI_WPA, "RSN: PMKID from Authenticator\n\r");
		}
	}

//	wifi_printk(WIFI_DBG_MSG, "snonce = %d\n",sm->renew_snonce);
	if (sm->renew_snonce) {
		if (atbmwifi_os_get_random(sm->snonce, ATBM_WPA_NONCE_LEN)) {
			wifi_printk(WIFI_WPA,
				"WPA: Failed to get atbm_os_random data for SNonce\n\r");
			goto failed;
		}
		sm->renew_snonce = 0;
		//wifi_printk(WIFI_WPA, "snonce ok\n");
	}

	/* Calculate PTK which will be stored as a temporary PTK until it has
	 * been verified when processing message 3/4. */
	ptk = &sm->tptk;
//	wifi_printk(WIFI_DBG_MSG, "ATBM_PMK_LEN(%d)\n",sm->ATBM_PMK_LEN);
//	wifi_printk(WIFI_DBG_MSG, "pmk[0](%d),pmk[10](%d),pmk[16](%d),pmk[20](%d)\n"
//							  "pmk[25](%d),pmk[28](%d),pmk[30](%d),pmk[31](%d)\n",
//							  sm->pmk[0],sm->pmk[10],sm->pmk[16],sm->pmk[20],
//							  sm->pmk[25],sm->pmk[28],sm->pmk[30],sm->pmk[31]);
	wpa_derive_ptk(ATBM_NULL,sm, src_addr, key, ptk);
	/* Supplicant: swap tx/rx Mic keys */
//	wifi_printk(WIFI_DBG_MSG, "wpa kck kck:kck[1](%x),kck[5](%x),kck[10](%x),kck[15](%x)\n"
//			   ,ptk->kck[1],ptk->kck[5],ptk->kck[10],ptk->kck[15]);
	atbm_memcpy(buf, ptk->u.auth.tx_mic_key, 8);
	atbm_memcpy(ptk->u.auth.tx_mic_key, ptk->u.auth.rx_mic_key, 8);
	atbm_memcpy(ptk->u.auth.rx_mic_key, buf, 8);
	sm->tptk_set = 1;

	if (wpa_supplicant_send_2_of_4(sm, sm->bssid, key, ver, sm->snonce,
				       sm->assoc_wpa_ie, sm->assoc_wpa_ie_len,
				       ptk))
		goto failed;

	atbm_memcpy(sm->anonce, key->key_nonce, ATBM_WPA_NONCE_LEN);
	return;
failed:
//	wpa_sm_deauthenticate(sm, WLAN_REASON_UNSPECIFIED);
	return;
}



 static int wpa_supplicant_validate_ie(struct atbmwifi_wpa_sm *sm,
				      const unsigned char *src_addr,
				      struct wpa_eapol_ie_parse *ie)
{
	if (sm->ap_wpa_ie == ATBM_NULL && sm->ap_rsn_ie == ATBM_NULL) {
#if 0
		wpa_printf(MSG_DEBUG, "WPA: No WPA/RSN IE for this AP known. "
			   "Trying to get from scan results");
//		if (/*wpa_sm_get_beacon_ie(sm)*/-1 < 0) {
//			wpa_printf(MSG_WARNING, "WPA: Could not find AP from "
//				   "the scan results");
//		} else {
			wpa_printf(MSG_DEBUG, "WPA: Found the current AP from "
				   "updated scan results");
#endif
//		}
	}

	if (ie->wpa_ie == ATBM_NULL && ie->rsn_ie == ATBM_NULL &&
	    (sm->ap_wpa_ie || sm->ap_rsn_ie)) {
/*		wpa_report_ie_mismatch(sm, "IE in 3/4 msg does not match "
				       "with IE in Beacon/ProbeResp (no IE?)",
				       src_addr, ie->wpa_ie, ie->wpa_ie_len,
				       ie->rsn_ie, ie->rsn_ie_len);*/
		return -1;
	}

	if ((ie->wpa_ie && sm->ap_wpa_ie &&
	     (ie->wpa_ie_len != sm->ap_wpa_ie_len ||
	      atbm_memcmp(ie->wpa_ie, sm->ap_wpa_ie, ie->wpa_ie_len) != 0)) ||
	    (ie->rsn_ie && sm->ap_rsn_ie &&
	     wpa_compare_rsn_ie(atbmwifi_wpa_key_mgmt_ft(sm->key_mgmt),
				sm->ap_rsn_ie, sm->ap_rsn_ie_len,
				ie->rsn_ie, ie->rsn_ie_len))) {
	/*	wpa_report_ie_mismatch(sm, "IE in 3/4 msg does not match "
				       "with IE in Beacon/ProbeResp",
				       src_addr, ie->wpa_ie, ie->wpa_ie_len,
				       ie->rsn_ie, ie->rsn_ie_len);*/
		return -1;
	}

	if (sm->proto == ATBM_WPA_PROTO_WPA &&
	    ie->rsn_ie && sm->ap_rsn_ie == ATBM_NULL && sm->rsn_enabled) {
	/*	wpa_report_ie_mismatch(sm, "Possible downgrade attack "
				       "detected - RSN was enabled and RSN IE "
				       "was in msg 3/4, but not in "
				       "Beacon/ProbeResp",
				       src_addr, ie->wpa_ie, ie->wpa_ie_len,
				       ie->rsn_ie, ie->rsn_ie_len);*/
		return -1;
	}

#if CONFIG_IEEE80211R
	if (atbmwifi_wpa_key_mgmt_ft(sm->key_mgmt) &&
	    wpa_supplicant_validate_ie_ft(sm, src_addr, ie) < 0)
		return -1;
#endif /* CONFIG_IEEE80211R */

	return 0;
}


 int wpa_supplicant_send_4_of_4(struct atbmwifi_wpa_sm *sm, const unsigned char *dst,
			       const struct atbmwifi_wpa_eapol_key *key,
			       atbm_uint16 ver, atbm_uint16 key_info,
			       const atbm_uint8 *kde, atbm_size_t kde_len,
			       struct atbmwifi_wpa_ptk *ptk)
{
	atbm_size_t rlen;
	struct atbmwifi_wpa_eapol_key *reply;
	atbm_uint8 *rbuf = ATBM_NULL;

	if (kde){
		wifi_printk(WIFI_WPA, "WPA: KDE for M4/4\n\r");
	}
//°2è?êí·?
	rbuf = wpa_sm_alloc_eapol(sm, ATBM_IEEE802_1X_TYPE_EAPOL_KEY, ATBM_NULL,
				  sizeof(*reply) + kde_len,
				  &rlen, (atbm_void *) &reply);
	if (rbuf == ATBM_NULL)
		return -1;

	reply->type = sm->proto == ATBM_WPA_PROTO_RSN ?
		ATBM_EAPOL_KEY_TYPE_RSN : ATBM_EAPOL_KEY_TYPE_WPA;
	key_info &= ATBM_WPA_KEY_INFO_SECURE;
	key_info |= ver | ATBM_WPA_KEY_INFO_KEY_TYPE | ATBM_WPA_KEY_INFO_MIC;
	ATBM_WPA_PUT_BE16(reply->key_info, key_info);
	if (sm->proto == ATBM_WPA_PROTO_RSN)
		ATBM_WPA_PUT_BE16(reply->key_length, 0);
	else
		atbm_memcpy(reply->key_length, key->key_length, 2);
	atbm_memcpy(reply->replay_counter, key->replay_counter,ATBM_WPA_REPLAY_COUNTER_LEN);

	ATBM_WPA_PUT_BE16(reply->key_data_length, kde_len);
	if (kde)
		atbm_memcpy(reply + 1, kde, kde_len);


	wifi_printk(WIFI_WPA, "WPA: Sending EAPOL-Key 4/4\n\r");


	wpa_eapol_key_send(sm, ptk->kck, ver, dst, ATBM_ETH_P_EAPOL,
			   rbuf, rlen, reply->key_mic);

	return 0;
}

atbm_void atbmwifi_wpa_event_connect_timeout(struct atbmwifi_vif *priv)
{
	wifi_printk(WIFI_DBG_ERROR,"%s %d\n",__func__,__LINE__);

	//when time out retry to send assoc req frame
	wpa_deauthen(priv);
	//fixme:reconnect here
	if(priv->auto_connect_when_lost){
		atbmwifi_autoconnect(priv, priv->scan_expire);
	}
}

static atbm_void wpa_supplicant_timeout(atbm_void *eloop_ctx, atbm_void *timeout_ctx)
{
	struct wpa_supplicant *wpas = (struct wpa_supplicant *)eloop_ctx;
	atbmwifi_wpa_event_queue((atbm_void*)wpas->priv,timeout_ctx,ATBM_NULL,
			WPA_EVENT__CONNECT_TIMEOUT,ATBM_WPA_EVENT_NOACK);
}
#if 0
static atbm_void wpa_supplicant_over_notice_app(atbm_void *data1,atbm_void *data2)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)data1;
	wifi_printk(WIFI_WPA,"notice app:net_enable\n");
	atbmwifi_event_uplayer(priv,ATBM_WIFI_ENABLE_NET_EVENT,priv->bssid);
	
}
static atbm_void wpa_supplicant_complete_timeout(atbm_void *data1,atbm_void *data2)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)data1;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	//tcp_opt->net_enable(g_vmac->ndev);
	if(wpa_s->wpa_state == ATBM_WPA_COMPLETED) {
		wpa_supplicant_eapol_notice_ack(priv);
	}
	else {
		wifi_printk(WIFI_WPA, "<WARNING>[TX] wpa_supp_timeout state=%d,\n",
			wpa_s->wpa_state);
	}
}
#endif

atbm_void wpa_supplicant_eapol_notice_ack(struct atbmwifi_vif *priv)
{
	atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,ATBM_NULL,WPA_EVENT__EAP_TX_STATUS,ATBM_WPA_EVENT_NOACK);	
}
atbm_void wpa_supplicant_rx_eap_status(struct atbmwifi_vif *priv)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	
	wifi_printk(WIFI_WPA, "[TX] wpa_state %d.assoc_ok %d\n",
		wpa_s->wpa_state,priv->assoc_ok);
	if(wpa_s == ATBM_NULL){
		wifi_printk(WIFI_WPA, "[TX] wpa_s is null\n");
		return;
	}
	if(wpa_s->wpa_state == ATBM_WPA_COMPLETED)
	{
		if((priv->assoc_ok)/*&&!b_uip_dev_enable*/)
		{
			/*MayBe It is enable to before the install key*/
			atbmwifi_event_uplayer(priv,ATBM_WIFI_ENABLE_NET_EVENT,priv->bssid);
			priv->connect.encrype = 1;
			priv->connect_ok = 1;
			priv->scan_expire = INITIAL_SCAN_EXPIRE;
			/*Use fase ps mode,the Time can adjust dymatic,default is 50*/
			atbmwifi_set_pm(priv,ATBM_TRUE,50);
		}
	}
	else if(wpa_s->wpa_state == ATBM_WPA_GROUP_HANDSHAKE){		

		/*MayBe It is enable to before the install key*/
		atbmwifi_event_uplayer(priv,ATBM_WIFI_ENABLE_NET_EVENT,priv->bssid);
	}
}
static atbm_void wpa_supplicant_key_neg_complete(struct atbmwifi_wpa_sm *sm,
					    const atbm_uint8 *addr, int secure)
{
	sm->wpa_s->wpa_state = ATBM_WPA_COMPLETED;
  	atbmwifi_eloop_cancel_timeout(wpa_supplicant_timeout, sm->wpa_s,ATBM_NULL);
#if CONFIG_IEEE80211R
	if (atbmwifi_wpa_key_mgmt_ft(sm->key_mgmt)) {
		/* Prepare for the next transition */
		wpa_ft_prepare_auth_request(sm, ATBM_NULL);
	}
#endif /* CONFIG_IEEE80211R */
	wifi_printk(WIFI_WPA, "4 and 2 way handshake done!!!! \n");
}
static int atbmwifi_ieee80211w_set_keys(struct atbmwifi_wpa_sm *sm,
			       struct wpa_eapol_ie_parse *ie)
{
#if CONFIG_IEEE80211W
	if (sm->mgmt_group_cipher != ATBM_WPA_CIPHER_AES_128_CMAC)
		return 0;

	if (ie->igtk) {
		const struct wpa_igtk_kde *igtk;
		atbm_uint16 keyidx;
	if (ie->igtk_len != ATBM_WPA_IGTK_KDE_PREFIX_LEN +
		(atbm_uint32) wpa_commom_key_len(sm->mgmt_group_cipher))
			return -1;
		igtk = (const struct wpa_igtk_kde *) ie->igtk;
		keyidx = ATBM_WPA_GET_LE16(igtk->keyid);
		wpa_printf(MSG_DEBUG, "WPA: IGTK keyid %d "
			   "pn %02x%02x%02x%02x%02x%02x",
			   keyidx, MAC2STR(igtk->pn));
		wpa_hexdump_key(MSG_DEBUG, "WPA: IGTK",
				igtk->igtk, ATBM_WPA_IGTK_LEN);
		if (keyidx > 4095) {
			wpa_printf(MSG_WARNING, "WPA: Invalid IGTK KeyID %d",
				   keyidx);
			return -1;
		}
		if(wpa_common_install_igtk(sm->wpa_s->priv, igtk->igtk, ATBM_WPA_CIPHER_AES_128_CMAC, keyidx)){
			wpa_printf(MSG_WARNING, "WPA: Failed to configure IGTK"
				   " to the driver");
			return -1;
		}
	}

	return 0;
#else /* CONFIG_IEEE80211W */
	return 0;
#endif /* CONFIG_IEEE80211W */
}

static atbm_void wpa_supplicant_process_3_of_4(struct atbmwifi_vif *priv,
					  struct atbmwifi_wpa_sm *sm,
					  const struct atbmwifi_wpa_eapol_key *key,
					  atbm_uint16 ver)
{
	atbm_uint16 key_info, keylen, len;
	const atbm_uint8 *pos;
	struct wpa_eapol_ie_parse ie;

//	wpa_sm_set_state(sm, ATBM_WPA_4WAY_HANDSHAKE);
	sm->wpa_s->wpa_state = ATBM_WPA_4WAY_HANDSHAKE;
	atbm_memset(&ie,0,sizeof(struct wpa_eapol_ie_parse));
	wifi_printk(WIFI_WPA, "WPA: RX M3/4-Way Handshake from "
		   MACSTR " (ver=%d)\n\r", MAC2STR(sm->bssid), ver);
	key_info = ATBM_WPA_GET_BE16(key->key_info);

	pos = (const atbm_uint8 *) (key + 1);
	len = ATBM_WPA_GET_BE16(key->key_data_length);

	wpa_supplicant_parse_ies(pos, len, &ie);
//	wpa_supplicant_parse_ies(sm->ap_rsn_ie, sm->ap_rsn_ie_len, &ie);
	if (ie.gtk && !(key_info & ATBM_WPA_KEY_INFO_ENCR_KEY_DATA)) {
//		wifi_printk(WIFI_DBG_MSG, "WPA: GTK IE in unencrypted key data\n\r");
		goto failed;
	}
//	wifi_printk(WIFI_DBG_MSG, "WPA:gtk(%d),gtk_len(&d)",ie.gtk,ie.gtk_len);
#if CONFIG_IEEE80211W
	if (ie.igtk && !(key_info & ATBM_WPA_KEY_INFO_ENCR_KEY_DATA)) {
		goto failed;
	}

	if (ie.igtk && ie.igtk_len != ATBM_WPA_IGTK_KDE_PREFIX_LEN +
	    (atbm_uint32) wpa_commom_key_len(sm->mgmt_group_cipher)) {
		goto failed;
	}
#endif /* CONFIG_IEEE80211W */

	if (wpa_supplicant_validate_ie(sm, sm->bssid, &ie) < 0)
		goto failed;
	
#if  (CONFIG_WPA2_REINSTALL_CERTIFICATION==0)
	if (atbm_memcmp(sm->anonce, key->key_nonce, ATBM_WPA_NONCE_LEN) != 0) {
//		wifi_printk(WIFI_DBG_MSG, "WPA: ANonce from message 1 of 4-Way "
//			   "Handshake differs from 3 of 4-Way Handshake - drop"
//			   " packet (src=" MACSTR ")\n\r", MAC2STR(sm->bssid));
		goto failed;
	}
#endif //#if CONFIG_WPA2_REINSTALL_CERTIFICATION

	keylen = ATBM_WPA_GET_BE16(key->key_length);
	switch (sm->pairwise_cipher) {
	case ATBM_WPA_CIPHER_CCMP:
		if (keylen != 16) {


//			wifi_printk(WIFI_DBG_MSG, "WPA: Invalid CCMP key length "
//				   "%d (src=" MACSTR ")\n\r",
//				   keylen, MAC2STR(sm->bssid));

			goto failed;
		}
		break;
	case ATBM_WPA_CIPHER_TKIP:
		if (keylen != 32) {
//			wifi_printk(WIFI_DBG_MSG, "WPA: Invalid TKIP key length "
//				   "%d (src=" MACSTR ")\n\r",
//				   keylen, MAC2STR(sm->bssid));
			goto failed;
		}
		break;
	}
	//key can't be all zero
	/*
	if (key_info & ATBM_WPA_KEY_INFO_INSTALL) {
		atbm_memset(key_zero,0,32 );
		if((sm->pairwise_cipher==ATBM_WPA_CIPHER_CCMP)){
			// device installing an all zero CCMP TK, then FAIL else PASS.
			if(atbm_memcmp(sm->ptk.tk1,key_zero,16)==0){
				wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk all zero key drop\n\r");
				//dump_mem(connect->key,connect->key_len);
				goto failed;
			}
		}
		else if(sm->pairwise_cipher==ATBM_WPA_CIPHER_TKIP){
			// device installing an all zero TKIP TK, then FAIL else PASS.
			if(atbm_memcmp(sm->ptk.tk1,key_zero,32)==0){
				wifi_printk(WIFI_DBG_ERROR, "wpa_common_install_ptk all zero key drop\n\r");
				//dump_mem(connect->key,connect->key_len);
				goto failed;
			}
		}
	}
	*/
	if (wpa_supplicant_send_4_of_4(sm, sm->bssid, key, ver, key_info,
				       ATBM_NULL, 0, &sm->ptk)) {
		goto failed;
	}

	/* SNonce was successfully used in msg 3/4, so mark it to be renewed
	 * for the next 4-Way Handshake. If msg 3 is received again, the old
	 * SNonce will still be used to avoid changing PTK. */
	sm->renew_snonce = 1;

	if((sm->wpa_s->wpa_state < ATBM_WPA_GROUP_HANDSHAKE)){
		if (key_info & ATBM_WPA_KEY_INFO_INSTALL) {
			wifi_printk(WIFI_WPA, "WPA:install ptk\n\r");
			if(wpa_common_install_ptk(priv,&sm->ptk,sm->pairwise_cipher,1 | (sm->linkid << 8)))
				goto failed;
		}
	}

	sm->wpa_s->wpa_state = ATBM_WPA_GROUP_HANDSHAKE;

	if(!(ie.gtk_len&&ie.gtk&&(!wpa_common_install_gtk(priv,(atbm_uint8 *)ie.gtk+2,sm->group_cipher,ie.gtk[0] & 0x3))))
	{
		wifi_printk(WIFI_WPA, "no group key\n\r");
		goto failed;
	}
	wpa_supplicant_key_neg_complete(sm, sm->bssid,
					key_info & ATBM_WPA_KEY_INFO_SECURE);
	if (atbmwifi_ieee80211w_set_keys(sm, &ie) < 0) {
		wifi_printk(WIFI_WPA, "RSN: Failed to configure IGTK");
		goto failed;
	}

	return;

failed:
//	wpa_sm_deauthenticate(sm, WLAN_REASON_UNSPECIFIED);
	return;
}



static int wpa_supplicant_check_group_cipher(int group_cipher,
					     int keylen, int maxkeylen,
					     int *key_rsc_len,
					     enum atbm_wpa_alg *alg)
{
	int ret = 0;

	switch (group_cipher) {
	case ATBM_WPA_CIPHER_CCMP:
		if (keylen != 16 || maxkeylen < 16) {
			ret = -1;
			break;
		}
		*key_rsc_len = 6;
		*alg =ATBM_WPA_ALG_CCMP;
		break;
	case ATBM_WPA_CIPHER_TKIP:
		if (keylen != 32 || maxkeylen < 32) {
			ret = -1;
			break;
		}
		*key_rsc_len = 6;
		*alg = ATBM_WPA_ALG_TKIP;
		break;
	case ATBM_WPA_CIPHER_WEP104:
		if (keylen != 13 || maxkeylen < 13) {
			ret = -1;
			break;
		}
		*key_rsc_len = 0;
		*alg = ATBM_WPA_ALG_WEP;
		break;
	case ATBM_WPA_CIPHER_WEP40:
		if (keylen != 5 || maxkeylen < 5) {
			ret = -1;
			break;
		}
		*key_rsc_len = 0;
		*alg = ATBM_WPA_ALG_WEP;
		break;
	default:
#if 0
		wpa_printf(MSG_WARNING, "WPA: Unsupported Group Cipher %d",
			   group_cipher);
#endif
		return -1;
	}

	if (ret < 0 ) {
	/*	wpa_printf(MSG_WARNING, "WPA: Unsupported %s Group Cipher key "
			   "length %d (%d).",
			   wpa_cipher_txt(group_cipher), keylen, maxkeylen);*/
	}

	return ret;
}

static int wpa_supplicant_gtk_tx_bit_workaround(const struct atbmwifi_wpa_sm *sm,
						int tx)
{
	if (tx && sm->pairwise_cipher != ATBM_WPA_CIPHER_NONE) {
		/* Ignore Tx bit for GTK if a pairwise key is used. One AP
		 * seemed to set this bit (incorrectly, since Tx is only when
		 * doing Group Key only APs) and without this workaround, the
		 * data connection does not work because wpa_supplicant
		 * configured non-zero keyidx to be used for unicast. */
#if 0
		wpa_printf(MSG_INFO, "WPA: Tx bit set for GTK, but pairwise "
			   "keys are used - ignore Tx bit");
#endif
		return 0;
	}
	return tx;
}
static int wpa_supplicant_process_1_of_2_rsn(struct atbmwifi_wpa_sm *sm,
					     const atbm_uint8 *keydata,
					     atbm_size_t keydatalen,
					     atbm_uint16 key_info,
					     struct atbmwifi_wpa_gtk_data *gd)
{
	int maxkeylen;
	struct wpa_eapol_ie_parse ie;

	wpa_supplicant_parse_ies(keydata, keydatalen, &ie);
	if (ie.gtk && !(key_info & ATBM_WPA_KEY_INFO_ENCR_KEY_DATA)) {

		return -1;
	}
	if (ie.gtk == ATBM_NULL) {

		return -1;
	}
	maxkeylen = gd->gtk_len = ie.gtk_len - 2;

	if (wpa_supplicant_check_group_cipher(sm->group_cipher,
					      gd->gtk_len, maxkeylen,
					      &gd->key_rsc_len, &gd->alg))
		return -1;

	gd->keyidx = ie.gtk[0] & 0x3;
	gd->tx = wpa_supplicant_gtk_tx_bit_workaround(sm,
						      !!(ie.gtk[0] & BIT(2)));
	if (ie.gtk_len - 2 > sizeof(gd->gtk)) {

		return -1;
	}
	atbm_memcpy(gd->gtk, ie.gtk + 2, ie.gtk_len - 2);

	if (atbmwifi_ieee80211w_set_keys(sm, &ie) < 0)
	{
		return -1;
	}


	return 0;
}

/*
const atbm_uint8 test_ek[32]  = {
0x15 ,0x07 ,0x3f ,0x31 ,0x42 ,0x71 ,0xf3 ,0xeb ,0x35 ,0x55 ,0xed ,0x96 ,0xe5 ,0x1e ,0x28 ,0xeb ,0x7a ,0x4e ,0x87 ,0x46 ,0xbb ,0x64 ,0x01 ,0xa4 ,0xa8 ,0x26 ,0x0e ,0xa8 ,0x87 ,0x02 ,0xf0 ,0x31
//0x15 ,0x07 ,0x3f ,0x31 ,0x42 ,0x71 ,0xf3 ,0xeb ,0x35 ,0x55 ,0xed ,0x96 ,0xe5 ,0x1e ,0x29 ,0x13 ,0x76 ,0x3e ,0x63 ,0xf2 ,0x4e ,0x03 ,0x63 ,0x3b ,0xbd ,0xd1 ,0x2b ,0xc3 ,0x32 ,0xba ,0xf3 ,0x31
};
const atbm_uint8 test_gtk[32] = {
0x03 ,0x17 ,0x7f ,0xd1 ,0x74 ,0x9a ,0xb1 ,0x3f ,0xf2 ,0x72 ,0x2b ,0xdd ,0x6f ,0xc0 ,0xdf ,0xba ,0x5f ,0xee ,0xb4 ,0xf2 ,0x67 ,0xf3 ,0xf4 ,0xfe ,0x47 ,0x68 ,0x52 ,0xb8 ,0xc9 ,0x4f ,0x33 ,0x3e
//0xd2 ,0xcc ,0x9b ,0xe3 ,0x9a ,0xcd ,0x6f ,0xc4 ,0x57 ,0x4f ,0x63 ,0x0e ,0x36 ,0x5a ,0xbe ,0xad ,0x2b ,0x73 ,0x40 ,0xd7 ,0x94 ,0xd2 ,0xb4 ,0xc9 ,0xdd ,0xa8 ,0x36 ,0x8a ,0xb5 ,0x9e ,0xfb ,0x50
};

const atbm_uint8 test_kek[32] = {
0x43 ,0x42 ,0x8c ,0x89 ,0x3a ,0x78 ,0x1c ,0xfe ,0x72 ,0xef ,0x62 ,0xeb ,0x1a ,0xbf ,0xd7 ,0x1b
};

const atbm_uint8 test_cipher[32] = {
0x4c ,0xfb ,0xf0 ,0x93 ,0x1d ,0xb5 ,0xb2 ,0x6c
};
*/


static int wpa_supplicant_process_1_of_2_wpa(struct atbmwifi_wpa_sm *sm,
					     const struct atbmwifi_wpa_eapol_key *key,
					     atbm_size_t keydatalen, int key_info,
					     atbm_size_t extra_len, atbm_uint16 ver,
					     struct atbmwifi_wpa_gtk_data *gd)
{
	atbm_size_t maxkeylen;
	atbm_uint8 ek[32];

	gd->gtk_len = ATBM_WPA_GET_BE16(key->key_length);
	maxkeylen = keydatalen;
	if (keydatalen > extra_len) {
//		wifi_printk(WIFI_DBG_MSG, "wpa g keylen err\n");
		return -1;
	}
	if (ver == ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
		if (maxkeylen < 8) {
//			wifi_printk(WIFI_DBG_MSG, "wpa g maxkeylen err\n");
			return -1;
		}
		maxkeylen -= 8;
	}

	if (wpa_supplicant_check_group_cipher(sm->group_cipher,
					      gd->gtk_len, maxkeylen,
					      &gd->key_rsc_len, &gd->alg))
		{
//			wifi_printk(WIFI_DBG_MSG, "wpa group chpher err\n");
			return -1;
		}
	gd->keyidx = (key_info & ATBM_WPA_KEY_INFO_KEY_INDEX_MASK) >>
		ATBM_WPA_KEY_INFO_KEY_INDEX_SHIFT;
	if (ver == ATBM_WPA_KEY_INFO_TYPE_HMAC_MD5_RC4) {
		
//		wifi_printk(WIFI_DBG_MSG, "ATBM_WPA_KEY_INFO_TYPE_HMAC_MD5_RC4,keydatalen(%d)\n",keydatalen);
		atbm_memcpy(ek, key->key_iv, 16);
		atbm_memcpy(ek + 16, sm->ptk.kek, 16);
		if (keydatalen > sizeof(gd->gtk)) {
//			wifi_printk(WIFI_DBG_MSG, "wpa HMAC_MD5_RC4 err\n");
			return -1;
		}
		atbm_memcpy(gd->gtk, key + 1, keydatalen);

		if (atbmwifi_rc4_skip(ek, 32, 256, gd->gtk, keydatalen)) {

//			wifi_printk(WIFI_DBG_MSG, "atbmwifi_rc4_skip err\n");
			return -1;
		}

	} else if (ver == ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
		if (keydatalen % 8) {
//			wifi_printk(WIFI_DBG_MSG, "HMAC_SHA1_AES err\n");
			return -1;
		}
		if (maxkeylen > sizeof(gd->gtk)) {
			wifi_printk(WIFI_WPA, "AES-WRAP  err\n");
			return -1;
		}
		if (atbmwifi_aes_unwrap(sm->ptk.kek, maxkeylen / 8,
			       (const atbm_uint8 *) (key + 1), gd->gtk)) {

//			wifi_printk(WIFI_DBG_MSG, "AES unwrap err\n");
			return -1;
		}

	} else {

//		wifi_printk(WIFI_DBG_MSG, "Unsupported key_info err\n");
		return -1;
	}
	gd->tx = wpa_supplicant_gtk_tx_bit_workaround(
		sm, !!(key_info & ATBM_WPA_KEY_INFO_TXRX));
	return 0;
}


static int wpa_supplicant_send_2_of_2(struct atbmwifi_wpa_sm *sm,
				      const struct atbmwifi_wpa_eapol_key *key,
				      int ver, atbm_uint16 key_info)
{
	atbm_size_t rlen;
	struct atbmwifi_wpa_eapol_key *reply;
	atbm_uint8 *rbuf;
//安全释放
	rbuf = wpa_sm_alloc_eapol(sm, ATBM_IEEE802_1X_TYPE_EAPOL_KEY, ATBM_NULL,
				  sizeof(*reply), (atbm_size_t *)&rlen, (atbm_void *) &reply);
	if (rbuf == ATBM_NULL)
		return -1;

	reply->type = sm->proto == ATBM_WPA_PROTO_RSN ?
		ATBM_EAPOL_KEY_TYPE_RSN : ATBM_EAPOL_KEY_TYPE_WPA;
	key_info &= ATBM_WPA_KEY_INFO_KEY_INDEX_MASK;
	key_info |= ver | ATBM_WPA_KEY_INFO_MIC | ATBM_WPA_KEY_INFO_SECURE;
	ATBM_WPA_PUT_BE16(reply->key_info, key_info);
	if (sm->proto == ATBM_WPA_PROTO_RSN)
		ATBM_WPA_PUT_BE16(reply->key_length, 0);
	else
		atbm_memcpy(reply->key_length, key->key_length, 2);
	atbm_memcpy(reply->replay_counter, key->replay_counter,
		  ATBM_WPA_REPLAY_COUNTER_LEN);

	ATBM_WPA_PUT_BE16(reply->key_data_length, 0);
#if 0
	wpa_printf(MSG_DEBUG, "WPA: Sending EAPOL-Key 2/2");
#endif
	wifi_printk(WIFI_WPA, "wpa group 2/2  \n");
	wpa_eapol_key_send(sm, sm->ptk.kck, ver, sm->bssid, ATBM_ETH_P_EAPOL,
			   rbuf, rlen, reply->key_mic);

	return 0;
}


static atbm_void wpa_supplicant_process_1_of_2(struct atbmwifi_vif *priv,
				      struct atbmwifi_wpa_sm *sm,
					  const unsigned char *src_addr,
					  const struct atbmwifi_wpa_eapol_key *key,
					  int extra_len, atbm_uint16 ver)
{
	atbm_uint16 key_info, keydatalen;
	int rekey, ret;
	struct atbmwifi_wpa_gtk_data gd;
	atbm_uint8 *_gtk;
	atbm_memset(&gd, 0, sizeof(gd));

//	rekey = wpa_sm_get_state(sm) == WPA_COMPLETED;
	rekey = sm->wpa_s->wpa_state == ATBM_WPA_COMPLETED;
	wifi_printk(WIFI_WPA, "wpa group 1/2\n");
	key_info = ATBM_WPA_GET_BE16(key->key_info);
	keydatalen = ATBM_WPA_GET_BE16(key->key_data_length);

	if (sm->proto == ATBM_WPA_PROTO_RSN) {
		ret = wpa_supplicant_process_1_of_2_rsn(sm,
							(const atbm_uint8 *) (key + 1),
							keydatalen, key_info,
							&gd);
	} else {
		wifi_printk(WIFI_WPA, "ATBM_WPA_PROTO_WPA  \n");
		ret = wpa_supplicant_process_1_of_2_wpa(sm, key, keydatalen,
							key_info, extra_len,
							ver, &gd);
	}

//	wpa_sm_set_state(sm, ATBM_WPA_GROUP_HANDSHAKE);
	sm->wpa_s->wpa_state = ATBM_WPA_GROUP_HANDSHAKE;

	if (ret)
	{
//		wifi_printk(WIFI_DBG_MSG, "wpa group 1/2 err \n");
		goto failed;
	}
//	wifi_printk(WIFI_DBG_MSG, "gtk index(%d)\n",gd.keyidx);
	_gtk = gd.gtk;

	if(sm->group_cipher == ATBM_WPA_CIPHER_TKIP)
	{
		atbm_uint8 gtk_buff[32];
		atbm_memset(gtk_buff,0,sizeof(gtk_buff));
		/* Swap Tx/Rx keys for Michael MIC */
		atbm_memcpy(gtk_buff, gd.gtk, 16);
		atbm_memcpy(gtk_buff + 16, gd.gtk + 16, 8);
		atbm_memcpy(gtk_buff + 24, gd.gtk + 24, 8);
		_gtk = gtk_buff;
		wifi_printk(WIFI_WPA, "Swap Tx/Rx keys for MIC \n");
	}
	
	if (wpa_common_install_gtk(priv,_gtk,sm->group_cipher,gd.keyidx) ||
	    wpa_supplicant_send_2_of_2(sm, key, ver, key_info))
		goto failed;
	if (rekey) {

	} else {
		wpa_supplicant_key_neg_complete(sm, sm->bssid,
						key_info &
						ATBM_WPA_KEY_INFO_SECURE);
	}
	return;

failed:
//	wpa_sm_deauthenticate(sm, WLAN_REASON_UNSPECIFIED);
	return;
}

/* Decrypt RSN EAPOL-Key key data (RC4 or AES-WRAP) */
static int wpa_supplicant_decrypt_key_data(struct atbmwifi_wpa_sm *sm,
					   struct atbmwifi_wpa_eapol_key *key, atbm_uint16 ver)
{
	atbm_uint16 keydatalen = ATBM_WPA_GET_BE16(key->key_data_length);

	if (!sm->ptk_set) {

		return -1;
	}

	/* Decrypt key data here so that this operation does not need
	 * to be implemented separately for each message type. */
	if (ver == ATBM_WPA_KEY_INFO_TYPE_HMAC_MD5_RC4) {
		atbm_uint8 ek[32];
		atbm_memcpy(ek, key->key_iv, 16);
		atbm_memcpy(ek + 16, sm->ptk.kek, 16);
		if (atbmwifi_rc4_skip(ek, 32, 256, (atbm_uint8 *) (key + 1), keydatalen)) {

			return -1;
		}
	} else if (ver == ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES ||
			ver == ATBM_WPA_KEY_INFO_TYPE_AES_128_CMAC ||
			(sm->wpa_s->key_mgmt & (ATBM_WPA_KEY_MGMT_SAE | ATBM_WPA_KEY_MGMT_FT_SAE))) {
		atbm_uint8 *buf;
		if (keydatalen % 8) {

			return -1;
		}
		keydatalen -= 8; /* AES-WRAP adds 8 bytes */
		//release ok
		buf = (atbm_uint8*)atbm_kmalloc(keydatalen,GFP_KERNEL);
		if (buf == ATBM_NULL) {

			return -1;
		}
		if (atbmwifi_aes_unwrap(sm->ptk.kek, keydatalen / 8,
			       (atbm_uint8 *) (key + 1), buf)) {
			atbm_kfree(buf);
			return -1;
		}
		atbm_memcpy(key + 1, buf, keydatalen);
		atbm_kfree(buf);
		ATBM_WPA_PUT_BE16(key->key_data_length, keydatalen);
	} else {

		return -1;
	}

	return 0;
}


int atbmwifi_wpa_sm_rx_eapol(struct atbmwifi_vif *priv,struct atbmwifi_wpa_sm *sm, const atbm_uint8 *src_addr,
		    const atbm_uint8 *buf, atbm_size_t len)
{
	atbm_size_t plen, data_len, extra_len;
	struct atbmwifi_ieee802_1x_hdr *hdr;
	struct atbmwifi_wpa_eapol_key *key;
	atbm_uint16 key_info, ver;
	atbm_uint8 *tmp;
	int ret = -1;
	struct wpa_peerkey *peerkey = ATBM_NULL;
#if CONFIG_IEEE80211R
	sm->ft_completed = 0;
#endif /* CONFIG_IEEE80211R */

	if (len < sizeof(*hdr) + sizeof(*key)) {
		ret =  -2;
		goto out2;
	}
	tmp = (atbm_uint8*)atbm_kmalloc(len,GFP_KERNEL);
	if (tmp == ATBM_NULL)
	{
		ret =  -3;
		goto out2;
	}		
	atbm_memcpy(tmp, buf, len);

	hdr = (struct atbmwifi_ieee802_1x_hdr *) tmp;
	key = (struct atbmwifi_wpa_eapol_key *) (hdr + 1);
	plen = atbm_be_to_host16(hdr->length);
	data_len = plen + sizeof(*hdr);
	

	if (hdr->version < EAPOL_VERSION) {
		/* TODO: backwards compatibility */
	}
	if (hdr->type != ATBM_IEEE802_1X_TYPE_EAPOL_KEY) {

		ret =  -4;
		goto out;
	}
	if (plen > len - sizeof(*hdr) || plen < sizeof(*key)) {

		ret =  -5;
		goto out;
	}

	if (key->type != ATBM_EAPOL_KEY_TYPE_WPA && key->type != ATBM_EAPOL_KEY_TYPE_RSN)
	{
		ret =  -6;
		goto out;
	}
	key_info = ATBM_WPA_GET_BE16(key->key_info);
	ver = key_info & ATBM_WPA_KEY_INFO_TYPE_MASK;
	if (ver != ATBM_WPA_KEY_INFO_TYPE_HMAC_MD5_RC4 &&
#if CONFIG_IEEE80211R || CONFIG_IEEE80211W
	    ver != ATBM_WPA_KEY_INFO_TYPE_AES_128_CMAC &&
#endif /* CONFIG_IEEE80211R || CONFIG_IEEE80211W */
	    ver != ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES &&
		!(sm->wpa_s->key_mgmt & (ATBM_WPA_KEY_MGMT_SAE | ATBM_WPA_KEY_MGMT_FT_SAE)) ) {
		ret =  -7;
		goto out;

	}
#ifdef CONFIG_IEEE80211W
	if (atbmwifi_wpa_key_mgmt_sha256(sm->key_mgmt)) {
		if (ver != ATBM_WPA_KEY_INFO_TYPE_AES_128_CMAC &&
				!atbmwifi_wpa_key_mgmt_sae(sm->key_mgmt)) {
			wpa_printf(MSG_INFO,
				"WPA: AP did not use the "
				"negotiated AES-128-CMAC");
			goto out;
		}
	} else
#endif /* CONFIG_IEEE80211W */
	if (sm->pairwise_cipher == ATBM_WPA_CIPHER_CCMP &&
		ver != ATBM_WPA_KEY_INFO_TYPE_HMAC_SHA1_AES &&
		!atbmwifi_wpa_key_mgmt_sae(sm->key_mgmt)) {

		if (sm->group_cipher != ATBM_WPA_CIPHER_CCMP &&
		    !(key_info & ATBM_WPA_KEY_INFO_KEY_TYPE)) {
			/* Earlier versions of IEEE 802.11i did not explicitly
			 * require version 2 descriptor for all EAPOL-Key
			 * packets, so allow group keys to use version 1 if
			 * CCMP is not used for them. */

		} 
		else{
			ret =  -9;
			goto out;
		}
	}


	if (!peerkey && sm->rx_replay_counter_set &&
	    atbm_memcmp(key->replay_counter, sm->rx_replay_counter,
		      ATBM_WPA_REPLAY_COUNTER_LEN) <= 0) {
		ret =  -10;
		goto out;
	}

	if (!(key_info & (ATBM_WPA_KEY_INFO_ACK | ATBM_WPA_KEY_INFO_SMK_MESSAGE))) {
		ret =  -11;
		goto out;
	}

	if (key_info & ATBM_WPA_KEY_INFO_REQUEST) {
		ret =  -12;
		goto out;
	}

	if ((key_info & ATBM_WPA_KEY_INFO_MIC) && !peerkey &&
	    wpa_supplicant_verify_eapol_key_mic(sm, key, ver, tmp, data_len))	{
		ret =  -13;
		goto out;
	}


	extra_len = data_len - sizeof(*hdr) - sizeof(*key);

	if (ATBM_WPA_GET_BE16(key->key_data_length) > extra_len) {
		ret =  -14;
		goto out;
	}
	extra_len = ATBM_WPA_GET_BE16(key->key_data_length);


	if (sm->proto == ATBM_WPA_PROTO_RSN &&
	    (key_info & ATBM_WPA_KEY_INFO_ENCR_KEY_DATA)) {
		if (wpa_supplicant_decrypt_key_data(sm, key, ver))
		{			
			ret =  -15;
			goto out;
		}
		extra_len = ATBM_WPA_GET_BE16(key->key_data_length);
	}
	if (key_info & ATBM_WPA_KEY_INFO_KEY_TYPE) {
		if (key_info & ATBM_WPA_KEY_INFO_KEY_INDEX_MASK) {
			goto out;
		}
		if (peerkey) {
			/* PeerKey 4-Way Handshake */
//			p_err("no peerkey_rx_eapol_4way\n");
//			peerkey_rx_eapol_4way(sm, peerkey, key, key_info, ver);
		} 
		else if (key_info & ATBM_WPA_KEY_INFO_MIC) {
			/* 3/4 4-Way Handshake */
			wpa_supplicant_process_3_of_4(priv,sm, key, ver);
		} 
		else {
			/* 1/4 4-Way Handshake */
			wpa_supplicant_process_1_of_4(sm, src_addr, key, ver);
		}
	} 
	else if (key_info & ATBM_WPA_KEY_INFO_SMK_MESSAGE) {
		/* PeerKey SMK Handshake */
//		peerkey_rx_eapol_smk(sm, src_addr, key, extra_len, key_info,
//				     ver);
	} 
	else {
		if (key_info & ATBM_WPA_KEY_INFO_MIC) {
			/* 1/2 Group Key Handshake */
			wpa_supplicant_process_1_of_2(priv,sm, src_addr, key,
						      extra_len, ver);
		} else {
		}
	}
	ret = 1;
out:
	atbm_kfree(tmp);
	
out2:
	return ret;
}
/*static  int wpa_key_mgmt_wpa(int akm)
{
	return atbmwifi_wpa_key_mgmt_wpa_ieee8021x(akm) ||
		atbmwifi_wpa_key_mgmt_wpa_psk(akm)
#ifdef CONFIG_WAPI
	|| wpa_key_mgmt_wapi_psk(akm);
#else
	;
#endif
}

static int wpa_key_mgmt_wpa_any(int akm)
{
	return wpa_key_mgmt_wpa(akm) || (akm & ATBM_WPA_KEY_MGMT_WPA_NONE);
}*/


const atbm_uint8 * wpa_bss_get_ie(const atbm_uint8 *ie_buff, int ie_len, atbm_uint8 ie)
{
	const atbm_uint8 *end, *pos;
	
	if(!ie_buff || !ie_len)
			return 0;

	pos = ie_buff;
	end = pos + ie_len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == ie)
			return pos;
		pos += 2 + pos[1];
	}

	return ATBM_NULL;
}


const atbm_uint8 * wpa_bss_get_vendor_ie(const atbm_uint8 *ie_buff, int ie_len, atbm_uint32 vendor_type)
{
	const atbm_uint8 *end, *pos;

	if(!ie_buff || !ie_len)
		return 0;

	pos = ie_buff;
	end = pos + ie_len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == ATBM_WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == ATBM_WPA_GET_BE32(&pos[2]))
			return pos;
		pos += 2 + pos[1];
	}

	return ATBM_NULL;
}

int atbmwifi_wpa_parse_wpa_ie(const atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len,
		     struct atbmwifi_wpa_ie_data *data)
{
	if (wpa_ie_len >= 1 && wpa_ie[0] == ATBM_WLAN_EID_RSN)
		return atbmwifi_wpa_parse_wpa_ie_rsn(wpa_ie, wpa_ie_len, data);
	else
		return wpa_parse_wpa_ie_wpa(wpa_ie, wpa_ie_len, data);
}

int atbmwifi_wpa_sm_parse_own_wpa_ie(struct atbmwifi_wpa_sm *sm, struct atbmwifi_wpa_ie_data *data)
{
	if (sm == ATBM_NULL || sm->assoc_wpa_ie == ATBM_NULL) {
#if 0
		wpa_printf(MSG_DEBUG, "WPA: No WPA/RSN IE available from "
			   "association info");
#endif
		return -1;
	}
	if (atbmwifi_wpa_parse_wpa_ie(sm->assoc_wpa_ie, sm->assoc_wpa_ie_len, data))
		return -2;
	return 0;
}

int atbmwifi_wpa_sm_set_param(struct atbmwifi_wpa_sm *sm, enum atbmwifi_wpa_sm_conf_params param,
		     unsigned int value)
{
	int ret = 0;

	if (sm == ATBM_NULL)
		return -1;

	switch (param) {
	case ATBM_RSNA_PMK_LIFETIME:
		if (value > 0)
			sm->dot11RSNAConfigPMKLifetime = value;
		else
			ret = -1;
		break;
	case ATBM_RSNA_PMK_REAUTH_THRESHOLD:
		if (value > 0 && value <= 100)
			sm->dot11RSNAConfigPMKReauthThreshold = value;
		else
			ret = -1;
		break;
	case ATBM_RSNA_SA_TIMEOUT:
		if (value > 0)
			sm->dot11RSNAConfigSATimeout = value;
		else
			ret = -1;
		break;
	case ATBM_WPA_PARAM_PROTO:
		sm->proto = value;
		break;
	case ATBM_WPA_PARAM_PAIRWISE:
		sm->pairwise_cipher = value;
		break;
	case ATBM_WPA_PARAM_GROUP:
		sm->group_cipher = value;
		break;
	case ATBM_WPA_PARAM_KEY_MGMT:
		sm->key_mgmt = value;
		break;
#if CONFIG_IEEE80211W
	case ATBM_WPA_PARAM_MGMT_GROUP:
		sm->mgmt_group_cipher = value;
		break;
#endif /* CONFIG_IEEE80211W */
	case ATBM_WPA_PARAM_RSN_ENABLED:
		sm->rsn_enabled = value;
		break;
	case ATBM_WPA_PARAM_MFP:
		sm->mfp = value;
		break;
	default:
		break;
	}

	return ret;
}


int atbmwifi_wpa_sm_set_ap_wpa_ie(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *ie, atbm_size_t len)
{
	if (sm == ATBM_NULL)
		return -1;
	if(sm->ap_wpa_ie)
	{
		atbm_kfree(sm->ap_wpa_ie);
		sm->ap_wpa_ie = 0;
	}
	if (ie == ATBM_NULL || len == 0) {
#if 0
		wpa_printf(MSG_DEBUG, "WPA: clearing AP WPA IE");
		sm->ap_wpa_ie = ATBM_NULL;
		sm->ap_wpa_ie_len = 0;
#endif
	} else {
#if 0
		wpa_hexdump(MSG_DEBUG, "WPA: set AP WPA IE", ie, len);
#endif
		sm->ap_wpa_ie = (atbm_uint8*)atbm_kmalloc(len,GFP_KERNEL);
		if (sm->ap_wpa_ie == ATBM_NULL)
			return -1;

		atbm_memcpy(sm->ap_wpa_ie, ie, len);
		sm->ap_wpa_ie_len = len;
	}

	return 0;
}


int atbmwifi_wpa_sm_set_ap_rsn_ie(struct atbmwifi_wpa_sm *sm, const atbm_uint8 *ie, atbm_size_t len)
{
	if (sm == ATBM_NULL)
		return -1;
	if(sm->ap_rsn_ie)
	{
		atbm_kfree(sm->ap_rsn_ie);
		sm->ap_rsn_ie = 0;
	}
	if (ie == ATBM_NULL || len == 0) {
#if 0
		wpa_printf(MSG_DEBUG, "WPA: clearing AP RSN IE");
#endif
		sm->ap_rsn_ie = ATBM_NULL;
		sm->ap_rsn_ie_len = 0;
	} else {
#if 0
		wpa_hexdump(MSG_DEBUG, "WPA: set AP RSN IE", ie, len);
#endif
		sm->ap_rsn_ie = (atbm_uint8*)atbm_kmalloc(len,GFP_KERNEL);
		if (sm->ap_rsn_ie == ATBM_NULL)
			return -1;

		atbm_memcpy(sm->ap_rsn_ie, ie, len);
		sm->ap_rsn_ie_len = len;
	}

	return 0;
}


static int wpa_gen_wpa_ie_rsn(atbm_uint8 *rsn_ie, atbm_size_t rsn_ie_len,
			      int pairwise_cipher, int group_cipher,
			      int key_mgmt, int mgmt_group_cipher,
			      struct atbmwifi_wpa_sm *sm)
{
#ifndef CONFIG_NO_WPA2
	atbm_uint8 *pos;
	struct atbmwifi_rsn_ie_hdr *hdr;
	atbm_uint16 capab;

	if (rsn_ie_len < sizeof(*hdr) + ATBM_RSN_SELECTOR_LEN +
	    2 + ATBM_RSN_SELECTOR_LEN + 2 + ATBM_RSN_SELECTOR_LEN + 2 +
	    (/*sm->cur_pmksa ? 2 + ATBM_PMKID_LEN :*/ 0)) {
#if 0
		wpa_printf(MSG_DEBUG, "RSN: Too short IE buffer (%lu bytes)",
			   (unsigned long) rsn_ie_len);
#endif
		return -1;
	}

	hdr = (struct atbmwifi_rsn_ie_hdr *) rsn_ie;
	hdr->elem_id = ATBM_WLAN_EID_RSN;
	ATBM_WPA_PUT_LE16(hdr->version, ATBM_RSN_VERSION);
	pos = (atbm_uint8 *) (hdr + 1);

	if (group_cipher == ATBM_WPA_CIPHER_CCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_CCMP);
	} else if (group_cipher == ATBM_WPA_CIPHER_TKIP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_TKIP);
	} else if (group_cipher == ATBM_WPA_CIPHER_WEP104) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_WEP104);
	} else if (group_cipher == ATBM_WPA_CIPHER_WEP40) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_WEP40);
	} else {
#if 0
		wpa_printf(MSG_WARNING, "Invalid group cipher (%d).",
			   group_cipher);
#endif
		return -1;
	}
	pos += ATBM_RSN_SELECTOR_LEN;

	*pos++ = 1;
	*pos++ = 0;
	if (pairwise_cipher == ATBM_WPA_CIPHER_CCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_CCMP);
	} else if (pairwise_cipher == ATBM_WPA_CIPHER_TKIP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_TKIP);
	} else if (pairwise_cipher == ATBM_WPA_CIPHER_NONE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_NONE);
	} else {
		wpa_printf(MSG_WARNING, "Invalid pairwise cipher (%d).",
			   pairwise_cipher);
		return -1;
	}
	pos += ATBM_RSN_SELECTOR_LEN;

	*pos++ = 1;
	*pos++ = 0;
	if (key_mgmt == ATBM_WPA_KEY_MGMT_IEEE8021X) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_UNSPEC_802_1X);
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_PSK) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X);
#if CONFIG_IEEE80211R
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_FT_IEEE8021X) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_FT_802_1X);
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_FT_PSK) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_FT_PSK);
#endif /* CONFIG_IEEE80211R */
#if CONFIG_IEEE80211W
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_802_1X_SHA256);
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_PSK_SHA256) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_PSK_SHA256);
#endif /* CONFIG_IEEE80211W */
#if CONFIG_SAE
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_SAE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_SAE);
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_FT_SAE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_AUTH_KEY_MGMT_FT_SAE);
#endif /* CONFIG_SAE */
	} else {
		wpa_printf(MSG_WARNING, "Invalid key management type (%d).",
			   key_mgmt);
		return -1;
	}
	pos += ATBM_RSN_SELECTOR_LEN;

	/* RSN Capabilities */
	capab = 0;
#if CONFIG_IEEE80211W
	if (sm->mfp)
		capab |= ATBM_WPA_CAPABILITY_MFPC;
	if (sm->mfp == 2)
		capab |= ATBM_WPA_CAPABILITY_MFPR;
#endif /* CONFIG_IEEE80211W */
	ATBM_WPA_PUT_LE16(pos, capab);
	pos += 2;

#if CONFIG_SAE
	if(sm->cur_pmksa){
		/* PMKID Count (2 octets, little endian) */
		*pos++ = 1;
		*pos++ = 0;
		/* PMKID */
		atbm_memcpy(pos, sm->wpa_s->sae.pmkid, ATBM_PMKID_LEN);
		pos += ATBM_PMKID_LEN;
	}
#endif /* CONFIG_SAE */

#if CONFIG_IEEE80211W
	if (mgmt_group_cipher == ATBM_WPA_CIPHER_AES_128_CMAC) {
		if (!sm->cur_pmksa) {
			/* PMKID Count */
			ATBM_WPA_PUT_LE16(pos, 0);
			pos += 2;
		}

		/* Management Group Cipher Suite */
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_RSN_CIPHER_SUITE_AES_128_CMAC);
		pos += ATBM_RSN_SELECTOR_LEN;
	}
#endif /* CONFIG_IEEE80211W */

	hdr->len = (pos - rsn_ie) - 2;

//	WPA_ASSERT((atbm_size_t) (pos - rsn_ie) <= rsn_ie_len);

	return pos - rsn_ie;
#else /* CONFIG_NO_WPA2 */
	return -1;
#endif /* CONFIG_NO_WPA2 */
}



static int wpa_gen_wpa_ie_wpa(atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len,
			      int pairwise_cipher, int group_cipher,
			      int key_mgmt)
{
	atbm_uint8 *pos;
	struct atbmwifi_wpa_ie_hdr *hdr;

	if (wpa_ie_len < sizeof(*hdr) + ATBM_WPA_SELECTOR_LEN +
	    2 + ATBM_WPA_SELECTOR_LEN + 2 + ATBM_WPA_SELECTOR_LEN)
		return -1;

	hdr = (struct atbmwifi_wpa_ie_hdr *) wpa_ie;
	hdr->elem_id = ATBM_WLAN_EID_VENDOR_SPECIFIC;
	ATBM_RSN_SELECTOR_PUT(hdr->oui, ATBM_WPA_OUI_TYPE);
	ATBM_WPA_PUT_LE16(hdr->version, ATBM_WPA_VERSION);
	pos = (atbm_uint8 *) (hdr + 1);

	if (group_cipher == ATBM_WPA_CIPHER_CCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_CCMP);
	} else if (group_cipher == ATBM_WPA_CIPHER_TKIP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_TKIP);
	} else if (group_cipher == ATBM_WPA_CIPHER_WEP104) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_WEP104);
	} else if (group_cipher == ATBM_WPA_CIPHER_WEP40) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_WEP40);
	} else {
		wpa_printf(MSG_WARNING, "Invalid group cipher (%d).",
			   group_cipher);
		wifi_printk(WIFI_WPA, "Invalid group cipher (%d).",group_cipher);
		return -1;
	}
	pos += ATBM_WPA_SELECTOR_LEN;

	*pos++ = 1;
	*pos++ = 0;
	if (pairwise_cipher == ATBM_WPA_CIPHER_CCMP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_CCMP);
	} else if (pairwise_cipher == ATBM_WPA_CIPHER_TKIP) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_TKIP);
	} else if (pairwise_cipher == ATBM_WPA_CIPHER_NONE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_CIPHER_SUITE_NONE);
	} else {
		wpa_printf(MSG_WARNING, "Invalid pairwise cipher (%d).",
			   pairwise_cipher);
		wifi_printk(WIFI_WPA, "Invalid pairwise cipher (%d).",pairwise_cipher);
		return -1;
	}
	pos += ATBM_WPA_SELECTOR_LEN;

	*pos++ = 1;
	*pos++ = 0;
	if (key_mgmt == ATBM_WPA_KEY_MGMT_IEEE8021X) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_AUTH_KEY_MGMT_UNSPEC_802_1X);
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_PSK) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X);
	} else if (key_mgmt == ATBM_WPA_KEY_MGMT_WPA_NONE) {
		ATBM_RSN_SELECTOR_PUT(pos, ATBM_WPA_AUTH_KEY_MGMT_NONE);
	} else {
		wpa_printf(MSG_WARNING, "Invalid key management type (%d).",
			   key_mgmt);
		wifi_printk(WIFI_WPA, "Invalid key management type (%d).",key_mgmt);
		return -1;
	}
	pos += ATBM_WPA_SELECTOR_LEN;

	/* WPA Capabilities; use defaults, so no need to include it */

	hdr->len = (pos - wpa_ie) - 2;

//	WPA_ASSERT((atbm_size_t) (pos - wpa_ie) <= wpa_ie_len);

	return pos - wpa_ie;
}
int wpa_gen_wpa_ie(struct atbmwifi_wpa_sm *sm, atbm_uint8 *wpa_ie, atbm_size_t wpa_ie_len)
{
	if (sm->proto == ATBM_WPA_PROTO_RSN)
		return wpa_gen_wpa_ie_rsn(wpa_ie, wpa_ie_len,
					  sm->pairwise_cipher,
					  sm->group_cipher,
					  sm->key_mgmt, sm->mgmt_group_cipher,
					  sm);
	else
		return wpa_gen_wpa_ie_wpa(wpa_ie, wpa_ie_len,
					  sm->pairwise_cipher,
					  sm->group_cipher,
					  sm->key_mgmt);
}


int atbmwifi_wpa_sm_set_assoc_wpa_ie_default(struct atbmwifi_wpa_sm *sm, atbm_uint8 *wpa_ie,
				    atbm_size_t *wpa_ie_len)
{
	int res;

	if (sm == ATBM_NULL)
		return -1;
	if (wpa_ie == ATBM_NULL)
		return -1;
	res = wpa_gen_wpa_ie(sm, wpa_ie, *wpa_ie_len);
	if (res < 0)
		return -1;
	*wpa_ie_len = res;

	wpa_hexdump(MSG_DEBUG, "WPA: Set own WPA IE default",
		    wpa_ie, *wpa_ie_len);

	if (sm->assoc_wpa_ie == ATBM_NULL) {
		/*
		 * Make a copy of the WPA/RSN IE so that 4-Way Handshake gets
		 * the correct version of the IE even if PMKSA caching is
		 * aborted (which would remove PMKID from IE generation).
		 */
		sm->assoc_wpa_ie = (atbm_uint8*)atbm_kmalloc(*wpa_ie_len,GFP_KERNEL);
		if (sm->assoc_wpa_ie == ATBM_NULL)
			return -1;

		atbm_memcpy(sm->assoc_wpa_ie, wpa_ie, *wpa_ie_len);
		sm->assoc_wpa_ie_len = *wpa_ie_len;
	}

	return 0;
}
void wpa_supplicant_set_suites_to_config(struct atbmwifi_vif *priv)
{
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct atbmwifi_cfg80211_bss *bss = &priv->bss;

	if((config->privacy == 0)&&(bss->privacy == 0)){
		config->key_mgmt = ATBM_WPA_KEY_MGMT_NONE;
		config->wpa = 0;
		config->privacy = 0;
		config->auth_alg = ATBM_WLAN_AUTH_OPEN;
		config->pairwise_cipher = ATBM_WPA_CIPHER_NONE;
		config->group_cipher = ATBM_WPA_CIPHER_NONE;		
	}else if(config->privacy&&bss->privacy){

		if((!bss->sta_priv.wpa)/*&&(!bss->sta_priv.wps)*/){
			extern atbm_void atbmwifi_wep_key_work(struct atbmwifi_vif *priv);
			config->key_mgmt=ATBM_WPA_KEY_MGMT_WEP;
			config->privacy = 1;
			/*HEX OR ASCII*/
			
			wifi_printk(WIFI_DBG_MSG, "wpa_supplicant_set_suites_to_config:WEP\n\r");
			if((config->password_len != 5) && (config->password_len != 13) && (config->password_len != 10) && (config->password_len != 26)){
				wifi_printk(WIFI_DBG_ERROR, "wpa_supplicant_set_suites_to_config:failed ,false keylen for wep\n\r");
				return;
			}

			config->auth_alg = ATBM_WLAN_AUTH_OPEN;
			config->group_cipher = (config->password_len == 5 || config->password_len == 10) ? ATBM_WPA_CIPHER_WEP40 : ATBM_WPA_CIPHER_WEP104;
			config->pairwise_cipher = (config->password_len ==5 || config->password_len == 10)? ATBM_WPA_CIPHER_WEP40: ATBM_WPA_CIPHER_WEP104;
			if((config->password_len== 5)||(config->password_len == 13))
			{
				wpa_common_install_wepkey(priv,(char *)config->password,config->group_cipher,config->key_id,0/*Sta linkid == 0*/);
			}else{
				atbm_uint8 password_ascii[13];
				atbmwifi_hexstr2bin(password_ascii, (char *)config->password, config->password_len);
				wpa_common_install_wepkey(priv, password_ascii,config->group_cipher,config->key_id,0/*Sta linkid == 0*/);
			}
			atbmwifi_wep_key_work(priv);

			wifi_printk(WIFI_DBG_MSG,"wpa scan (%s),auth(%x),gcipher(%x)keyid=%d,key=%s\n",
										  priv->ssid,config->auth_alg,config->group_cipher,config->key_id,config->password);
		}else if(bss->sta_priv.wpa){
			struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(priv->appdata);
			struct atbmwifi_wpa_sm *sm = wpa_s->wpa;
			
			wifi_printk(WIFI_DBG_MSG, "wpa_supplicant_set_suites_to_config:WPA\n\r");
			config->key_mgmt = sm->key_mgmt;
			config->wpa = sm->proto;
#if CONFIG_SAE
			if (wpa_s->key_mgmt & (ATBM_WPA_KEY_MGMT_SAE | ATBM_WPA_KEY_MGMT_FT_SAE))
				config->auth_alg = ATBM_WLAN_AUTH_SAE;
			else
#endif
			config->auth_alg = ATBM_WLAN_AUTH_OPEN;
			config->pairwise_cipher = sm->pairwise_cipher;
			config->group_cipher = sm->group_cipher;
			config->privacy = 1;
		}else if(bss->sta_priv.wps){
			//add wps
		}else {
			wifi_printk(WIFI_DBG_MSG, "wpa_supplicant_set_suites_to_config: privacy err\n\r");
		}
	}
	
	wifi_printk(WIFI_ALWAYS, "wpa[%d],key_mgmt[%d],privacy[%d],auth_alg[%d]\n\r",
		config->wpa,config->key_mgmt,config->privacy,config->auth_alg);
}
int atbmwifi_wpa_supplicant_set_suites(struct wpa_supplicant *wpa_s,
			      struct atbmwifi_cfg80211_bss *bss, struct atbmwifi_cfg *ssid,
			      atbm_uint8 *wpa_ie, atbm_size_t *wpa_ie_len)
{
	struct atbmwifi_wpa_ie_data ie;
	int sel, proto;
	const atbm_uint8 *bss_wpa, *bss_rsn;

	if (bss) {
		bss_wpa = wpa_bss_get_vendor_ie(bss->information_elements, 
			bss->len_information_elements, 
			WPA_IE_VENDOR_TYPE);
		bss_rsn = wpa_bss_get_ie(bss->information_elements, 
			bss->len_information_elements, 
			ATBM_WLAN_EID_RSN);
	} 
	else {
		bss_wpa = bss_rsn = ATBM_NULL;
	}
	if (bss_rsn && atbmwifi_wpa_parse_wpa_ie(bss_rsn, 2 + bss_rsn[1], &ie) == 0) {

		wifi_printk(WIFI_WPA, "set_suites: rsn\n\r");

		proto = ATBM_WPA_PROTO_RSN;
	} else if (bss_wpa && atbmwifi_wpa_parse_wpa_ie(bss_wpa, 2 +bss_wpa[1], &ie) == 0) {

		wifi_printk(WIFI_WPA, "set_suites: wpa\n\r");

		proto = ATBM_WPA_PROTO_WPA;
	} else/* if (bss) */{

		wifi_printk(WIFI_WPA, "set_suites:none\n\r");

		return -1;
	}

#if CONFIG_IEEE80211W
	if (ssid->ieee80211w) {
		wpa_printf(MSG_DEBUG, "WPA: Selected mgmt group cipher %d",
			   ie.mgmt_group_cipher);
	}
#endif /* CONFIG_IEEE80211W */

	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_PROTO, proto);
	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_RSN_ENABLED,
			 /*!!(ssid->proto & ATBM_WPA_PROTO_RSN)*/0);

	if (bss || !wpa_s->ap_ies_from_associnfo) {
		if (atbmwifi_wpa_sm_set_ap_wpa_ie(wpa_s->wpa, bss_wpa,
					 bss_wpa ? 2 + bss_wpa[1] : 0) ||
		    atbmwifi_wpa_sm_set_ap_rsn_ie(wpa_s->wpa, bss_rsn,
					 bss_rsn ? 2 + bss_rsn[1] : 0))
			return -1;
	}

	sel = ie.group_cipher;
	if (sel & ATBM_WPA_CIPHER_CCMP) {
		wpa_s->group_cipher = ATBM_WPA_CIPHER_CCMP;
		wpa_printf( MSG_DEBUG, "WPA: using GTK CCMP");
	} else if (sel & ATBM_WPA_CIPHER_TKIP) {
		wpa_s->group_cipher = ATBM_WPA_CIPHER_TKIP;
		wpa_printf( MSG_DEBUG, "WPA: using GTK TKIP");
	} else if (sel & ATBM_WPA_CIPHER_WEP104) {
		wpa_s->group_cipher = ATBM_WPA_CIPHER_WEP104;
		wpa_printf( MSG_DEBUG, "WPA: using GTK WEP104");
	} else if (sel & ATBM_WPA_CIPHER_WEP40) {
		wpa_s->group_cipher = ATBM_WPA_CIPHER_WEP40;
		wpa_printf( MSG_DEBUG, "WPA: using GTK WEP40");
	} else {
		wpa_printf(MSG_WARNING, "WPA: Failed to select group cipher.");
		return -1;
	}

	sel = ie.pairwise_cipher;
	if (sel & ATBM_WPA_CIPHER_CCMP) {
		wpa_s->pairwise_cipher = ATBM_WPA_CIPHER_CCMP;
		wpa_printf( MSG_DEBUG, "WPA: using PTK CCMP");
	} else if (sel & ATBM_WPA_CIPHER_TKIP) {
		wpa_s->pairwise_cipher = ATBM_WPA_CIPHER_TKIP;
		wpa_printf( MSG_DEBUG, "WPA: using PTK TKIP");
	} else if (sel & ATBM_WPA_CIPHER_NONE) {
		wpa_s->pairwise_cipher = ATBM_WPA_CIPHER_NONE;
		wpa_printf( MSG_DEBUG, "WPA: using PTK NONE");
	} else {
		wpa_printf(MSG_WARNING, "WPA: Failed to select pairwise "
			   "cipher.");
		return -1;
	}

	sel = ie.key_mgmt;
	if (0) {
#if CONFIG_IEEE80211R
	} else if (sel & ATBM_WPA_KEY_MGMT_FT_IEEE8021X) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_FT_IEEE8021X;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT FT/802.1X");
	} else if (sel & ATBM_WPA_KEY_MGMT_FT_PSK) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_FT_PSK;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT FT/PSK");
#endif /* CONFIG_IEEE80211R */
#if CONFIG_IEEE80211W
	} else if (sel & ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256;
		wpa_printf( MSG_DEBUG,
			"WPA: using KEY_MGMT 802.1X with SHA256");
	} else if (sel & ATBM_WPA_KEY_MGMT_PSK_SHA256) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_PSK_SHA256;
		wpa_printf( MSG_DEBUG,
			"WPA: using KEY_MGMT PSK with SHA256");
#endif /* CONFIG_IEEE80211W */
#if CONFIG_SAE
	} else if (sel & ATBM_WPA_KEY_MGMT_FT_SAE) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_FT_SAE;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT FT/SAE");
	} else if (sel & ATBM_WPA_KEY_MGMT_SAE) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_SAE;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT SAE");
#endif /* CONFIG_SAE */
	} else if (sel & ATBM_WPA_KEY_MGMT_IEEE8021X) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_IEEE8021X;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT 802.1X");
	} else if (sel & ATBM_WPA_KEY_MGMT_PSK) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_PSK;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT WPA-PSK");
	} else if (sel & ATBM_WPA_KEY_MGMT_WPA_NONE) {
		wpa_s->key_mgmt = ATBM_WPA_KEY_MGMT_WPA_NONE;
		wpa_printf( MSG_DEBUG, "WPA: using KEY_MGMT WPA-NONE");
	} else {
		wpa_printf(MSG_WARNING, "WPA: Failed to select authenticated "
			   "key management type.");
		return -1;
	}

	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_KEY_MGMT, wpa_s->key_mgmt);
	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_PAIRWISE,
			 wpa_s->pairwise_cipher);
	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_GROUP, wpa_s->group_cipher);

#if CONFIG_IEEE80211W
	sel = ie.mgmt_group_cipher;
	if (ssid->ieee80211w == ATBM_NO_MGMT_FRAME_PROTECTION ||
	    !(ie.capabilities & ATBM_WPA_CAPABILITY_MFPC))
		sel = 0;
	if (sel & ATBM_WPA_CIPHER_AES_128_CMAC) {
		wpa_s->mgmt_group_cipher = ATBM_WPA_CIPHER_AES_128_CMAC;
		wpa_printf( MSG_DEBUG, "WPA: using MGMT group cipher "
			"AES-128-CMAC");
	} else {
		wpa_s->mgmt_group_cipher = 0;
		wpa_printf( MSG_DEBUG, "WPA: not using MGMT group cipher");
	}
	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_MGMT_GROUP,
			 wpa_s->mgmt_group_cipher);
	atbmwifi_wpa_sm_set_param(wpa_s->wpa, ATBM_WPA_PARAM_MFP, ssid->ieee80211w);
#endif /* CONFIG_IEEE80211W */
	wifi_printk(WIFI_WPA, "p(%d),g(%d),k(%d)\n\r",
	wpa_s->pairwise_cipher,wpa_s->group_cipher,wpa_s->key_mgmt);

//	atbmwifi_wpa_sm_set_assoc_wpa_ie_default(wpa_s->wpa, wpa_ie, wpa_ie_len);
/*
	if (ssid->key_mgmt &
	    (ATBM_WPA_KEY_MGMT_PSK | ATBM_WPA_KEY_MGMT_FT_PSK | ATBM_WPA_KEY_MGMT_PSK_SHA256))
		wpa_sm_set_pmk(wpa_s->wpa, ssid->psk, ATBM_PMK_LEN);
	else
		wpa_sm_set_pmk_from_pmksa(wpa_s->wpa);
*/
/*	wpa_printf(MSG_DEBUG, "WPA: wpa_sm_set_pmk:%02x %02x %02x %02x %02x %02x \n",
	ssid->psk[0],
	ssid->psk[1],
	ssid->psk[2],
	ssid->psk[3],
	ssid->psk[4],
	ssid->psk[5]
	);		*/
	return proto;
}




atbm_void atbmwifi_wpa_supplicant_req_auth_timeout(struct wpa_supplicant *wpa_s,
				     int sec, int usec)
{	
	atbmwifi_eloop_cancel_timeout(wpa_supplicant_timeout, (atbm_void*)wpa_s,ATBM_NULL);
	
	atbmwifi_eloop_register_timeout(sec,usec,wpa_supplicant_timeout, (atbm_void*)wpa_s,ATBM_NULL);

}

/**
 * atbmwifi_wpa_supplicant_cancel_auth_timeout - Cancel authentication timeout
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to cancel authentication timeout scheduled with
 * atbmwifi_wpa_supplicant_req_auth_timeout() and it is called when authentication has
 * been completed.
 */
atbm_void atbmwifi_wpa_supplicant_cancel_auth_timeout(struct wpa_supplicant *wpa_s)
{
#if 0
	wpa_dbg(wpa_s, MSG_DEBUG, "Cancelling authentication timeout");
#endif
	atbmwifi_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, ATBM_NULL);
}

/**
 * eapol_sm_rx_eapol - Process received EAPOL frames
 * @sm: Pointer to EAPOL state machine allocated with eapol_sm_init()
 * @src: Source MAC address of the EAPOL packet
 * @buf: Pointer to the beginning of the EAPOL data (EAPOL header)
 * @len: Length of the EAPOL frame
 * Returns: 1 = EAPOL frame processed, 0 = not for EAPOL state machine,
 * -1 failure
 */
int eapol_sm_rx_eapol(struct eapol_sm *sm, const atbm_uint8 *src, const atbm_uint8 *buf,
		      atbm_size_t len)
{
	int res = 1;
	return res;
}


 
atbm_void atbmwifi_wpa_supplicant_rx_eapol(atbm_void *ctx, atbm_uint8 *src_addr,
			     atbm_uint8 *buf, atbm_size_t len)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif*)ctx;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	
	//wifi_printk(WIFI_DBG_MSG,"atbmwifi_wpa_supplicant_rx_eapol:line=%d\n",__LINE__);
	
	if (wpa_s->wpa_state < ATBM_WPA_ASSOCIATED) {
		/*
		 * There is possible race condition between receiving the
		 * association event and the EAPOL frame since they are coming
		 * through different paths from the driver. In order to avoid
		 * issues in trying to process the EAPOL frame before receiving
		 * association information, lets queue it for processing until
		 * the association event is received.
		 */

		wifi_printk(WIFI_DBG_ERROR,"wpa_supplicant_rx_eapol:line=%d,state=%d\n",__LINE__,wpa_s->wpa_state);

/*		wpabuf_free(wpa_s->pending_eapol_rx);
		wpa_s->pending_eapol_rx = wpabuf_alloc_copy(buf, len);
		if (wpa_s->pending_eapol_rx) {
			os_get_time(&wpa_s->pending_eapol_rx_time);
			atbm_memcpy(wpa_s->pending_eapol_rx_src, src_addr,
				  ATBM_ETH_ALEN);
		}*/
		return;
	}


	if (config->key_mgmt == ATBM_WPA_KEY_MGMT_NONE) {
		wifi_printk(WIFI_DBG_ERROR,"wpa_supplicant_rx_eapol:line=%d,key_mgmt=%d\n",__LINE__,config->key_mgmt);
		return;
	}
	atbmwifi_eloop_cancel_timeout(wpa_authen_assc_timeout,(atbm_void*)priv,ATBM_NULL);

#if CONFIG_WPS
	if(wpa_s->wps_mode != WPS_MODE_UNKNOWN){
		/* PBC or Pin: wps mode not need to start 4-way handshake timeout.*/
	}else
#endif
	if (wpa_s->eapol_received == 0 &&
	    (!(wpa_s->drv_flags & ATBM_WPA_DRIVER_FLAGS_4WAY_HANDSHAKE) ||
	     !atbmwifi_wpa_key_mgmt_wpa_psk(config->key_mgmt) ||
	     wpa_s->wpa_state != ATBM_WPA_COMPLETED)/* &&
	    (wpa_s->current_ssid == NULL ||
	     wpa_s->current_ssid->mode != IEEE80211_MODE_IBSS)*/) {
		/* Timeout for completing IEEE 802.1X and WPA authentication */

		atbmwifi_wpa_supplicant_req_auth_timeout(
			wpa_s,
			(atbmwifi_wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt) ||
			 wpa_s->key_mgmt == ATBM_WPA_KEY_MGMT_IEEE8021X_NO_WPA ||
			 wpa_s->key_mgmt == ATBM_WPA_KEY_MGMT_WPS) ?
		70 : 10, 0);

	}
	wpa_s->eapol_received++;

	if (wpa_s->countermeasures) {

		return;
	}


	/* Source address of the incoming EAPOL frame could be compared to the
	 * current BSSID. However, it is possible that a centralized
	 * Authenticator could be using another MAC address than the BSSID of
	 * an AP, so just allow any address to be used for now. The replies are
	 * still sent to the current BSSID (if available), though. */
//	wifi_printk(WIFI_DBG_MSG,"rx_eapol:k=%d,flag=%d\n",config->key_mgmt,wpa_s->drv_flags);
	atbm_memcpy(wpa_s->last_eapol_src, src_addr, ATBM_ETH_ALEN);

#if (CONFIG_WPS==0)
	if (!atbmwifi_wpa_key_mgmt_wpa_psk(config->key_mgmt) &&
		!atbmwifi_wpa_key_mgmt_sae(config->key_mgmt)/*&&
	    eapol_sm_rx_eapol(wpa_s->eapol, src_addr, buf, len) > 0*/)
	{
		wifi_printk(WIFI_DBG_ERROR,"sm_rx_eapol:err(1)\n");
		return;
	}
#endif

#if CONFIG_WPS
	if(wpa_s->wps_mode != WPS_MODE_UNKNOWN){
		wifi_printk(WIFI_WPS, "WPS: rx eapol packet.\n");
		if(wpa_wsc_rx_process(priv, buf, len) < 0){
			wifi_printk(WIFI_WPS, "WPS: eap_wsc_process failed.\n");
		}
	}else
#endif
	{
		atbmwifi_wpa_sm_rx_eapol(priv,wpa_s->wpa, src_addr, buf, len);
	}
}
atbm_void wpa_authen_assc_timeout(atbm_void *data1,atbm_void *data)
{
	atbmwifi_wpa_event_queue((atbm_void*)data1,data,ATBM_NULL,
				WPA_EVENT__CONNECT_TIMEOUT,ATBM_WPA_EVENT_NOACK);
}

static atbm_void __wpa_prepare_auth(struct atbmwifi_vif *priv)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(priv->appdata);
	struct atbmwifi_cfg *ssid = atbmwifi_get_config(priv);
	struct atbmwifi_cfg80211_bss *ap = &priv->bss; 
	atbm_uint16 transaction = 1;
	int i;

	wpa_s->wpa_state = ATBM_WPA_AUTHENTICATING;
	if(ap->sta_priv.wpa)
	{
		atbmwifi_wpa_supplicant_set_suites(wpa_s,ap, ssid,
						  ATBM_NULL, ATBM_NULL);
	}
	wpa_supplicant_set_suites_to_config(priv);
	/*If the key is wep or tkip ,Disable 11n*/
	ssid->flags &= ~ATBM_IEEE80211_STA_DISABLE_11N;
	for (i = 0; i < ssid->n_pairwise_cipher; i++){
		if (ssid->pairwise_cipher == ATBM_WPA_CIPHER_WEP40 ||
		    ssid->pairwise_cipher == ATBM_WPA_CIPHER_TKIP ||
		    ssid->pairwise_cipher == ATBM_WPA_CIPHER_WEP104)
			ssid->flags |= ATBM_IEEE80211_STA_DISABLE_11N;

	}

	if (ssid->key_mgmt & ATBM_WPA_KEY_MGMT_PSK
#if CONFIG_IEEE80211W
		 || ssid->key_mgmt & ATBM_WPA_KEY_MGMT_PSK_SHA256
#endif
		)
	{
		if(ssid->psk_set == 0){
			if(ssid->password_len == ATBM_PMK_LEN*2){
				atbmwifi_hexstr2bin(ssid->psk,(const char*)ssid->password, ATBM_PMK_LEN*2);
			}
			else {			
				atbm_pbkdf2_sha1((const char*)ssid->password, (const char*)ssid->ssid, ssid->ssid_len,4096,ssid->psk, ATBM_PMK_LEN);
			}
		}
		atbm_memcpy(wpa_s->wpa->pmk, ssid->psk,32 );
		wpa_s->wpa->pmk_len = 32;	
		ssid->psk_set=1;
	}

#if CONFIG_SAE
	wpa_s->sae_pmksa_caching = 0;
	if (atbmwifi_wpa_key_mgmt_sae(wpa_s->key_mgmt))
	{
		const atbm_uint8 *rsn;
		struct atbmwifi_wpa_ie_data ied;

		rsn = wpa_bss_get_ie(priv->bss.information_elements, priv->bss.len_information_elements, ATBM_WLAN_EID_RSN);
		if (!rsn) {
			wpa_printf(MSG_DEBUG,
				"SAE enabled, but target BSS does not advertise RSN");
#ifdef CONFIG_DPP
		} else if (atbmwifi_wpa_parse_wpa_ie(rsn, 2 + rsn[1], &ied) == 0 &&
			   (priv->config.key_mgmt & ATBM_WPA_KEY_MGMT_DPP) &&
			   (ied.key_mgmt & ATBM_WPA_KEY_MGMT_DPP)) {
			wpa_printf(MSG_DEBUG, "Prefer DPP over SAE when both are enabled");
#endif /* CONFIG_DPP */
		} else if (atbmwifi_wpa_parse_wpa_ie(rsn, 2 + rsn[1], &ied) == 0 &&
			   atbmwifi_wpa_key_mgmt_sae(ied.key_mgmt)) {
			wpa_printf(MSG_DEBUG, "Using SAE auth_alg");
			priv->config.auth_alg = ATBM_WLAN_AUTH_SAE;
		} else {
			wpa_printf(MSG_DEBUG,
				"SAE enabled, but target BSS does not advertise SAE AKM for RSN");
		}
	}
	if (priv->config.auth_alg == ATBM_WLAN_AUTH_SAE &&
	    pmksa_cache_set_current(wpa_s->wpa, NULL, priv->bssid, ssid, 0,
				    NULL,
				    wpa_s->key_mgmt == ATBM_WPA_KEY_MGMT_FT_SAE ?
				    ATBM_WPA_KEY_MGMT_FT_SAE :
				    ATBM_WPA_KEY_MGMT_SAE) == 0) {
		wpa_printf(MSG_DEBUG,
			"PMKSA cache entry found - try to use PMKSA caching instead of new SAE authentication");
		wpa_sm_set_pmk_from_pmksa(wpa_s->wpa);
		priv->config.auth_alg = ATBM_WLAN_AUTH_OPEN;
		wpa_s->sae_pmksa_caching = 1;
		if(wpa_s->sae_data){
			wpabuf_free(wpa_s->sae_data);
			wpa_s->sae_data = ATBM_NULL;
		}
	}

	if (priv->config.auth_alg  == ATBM_WLAN_AUTH_SAE) {
		if(wpa_s->sae_data){
			wpabuf_free(wpa_s->sae_data);
			wpa_s->sae_data = ATBM_NULL;
		}
		if (wpa_s->sae_start)
			wpa_s->sae_data = sme_auth_build_sae_commit(wpa_s, priv,
							 priv->bss.bssid, 0,
							 wpa_s->sae_start == 2);
		else
			wpa_s->sae_data = sme_auth_build_sae_confirm(wpa_s, 0);
		if (wpa_s->sae_data == ATBM_NULL) {
			wpa_deauthen(priv);
			//fixme:reconnect here
			if(priv->auto_connect_when_lost){
				atbmwifi_autoconnect(priv, priv->scan_expire);
			}
		}
		wpa_s->sae.state = wpa_s->sae_start ? SAE_COMMITTED : SAE_CONFIRMED;
		transaction = wpa_s->sae_start ? 1 : 2;
	}
#endif /* CONFIG_SAE */

	atbmwifi_eloop_cancel_timeout(wpa_authen_assc_timeout,(atbm_void*)priv,ATBM_NULL);
	atbmwifi_eloop_register_timeout(ATBM_WIFI_AUTH_ASSOC_TIMEOUT,0,wpa_authen_assc_timeout,(atbm_void*)priv,ATBM_NULL);
	atbmwifi_tx_sta_mgmtframe(priv,ATBM_IEEE80211_STYPE_AUTH,transaction);

}

atbm_void wpa_prepare_auth(struct atbmwifi_vif *priv)
{
	atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,ATBM_NULL,WPA_EVENT__SUPPLICANT_AUTHEN,ATBM_WPA_EVENT_NOACK);
}

atbm_void atbmwifi_wpa_event_authen(struct atbmwifi_vif *priv)
{
	__wpa_prepare_auth(priv);
}
static atbm_void __wpa_prepare_assciating(struct atbmwifi_vif *priv)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(priv->appdata);
	struct atbmwifi_cfg80211_bss *ap = &priv->bss;
	atbm_uint8 *wpa_ie=ATBM_NULL;
	atbm_size_t wpa_ie_len=0;

	if(wpa_s->wpa_state == ATBM_WPA_AUTHENTICATING)
	{
		wpa_s->connect_retry = 0;
	}
	else
	{
		return;
	}

	if(ap->sta_priv.wpa)
	{
		wpa_ie_len = 100;
		wpa_ie = (atbm_uint8 *)atbm_kmalloc(100,GFP_KERNEL);
		if(wpa_ie == ATBM_NULL)
		{
			return;
		}
		atbmwifi_wpa_sm_set_assoc_wpa_ie_default(wpa_s->wpa, wpa_ie, &wpa_ie_len);
	}
	
	if(ATBM_NULL != priv->extra_ie)
	{
		atbm_kfree(priv->extra_ie);
	}
	priv->extra_ie = wpa_ie;
	priv->extra_ie_len = wpa_ie_len;
	atbmwifi_tx_sta_mgmtframe(priv,ATBM_IEEE80211_STYPE_ASSOC_REQ,0);
	wpa_s->wpa_state = ATBM_WPA_ASSOCIATING;
	if(ATBM_NULL != priv->extra_ie)
	{
		atbm_kfree(priv->extra_ie);
		priv->extra_ie = ATBM_NULL;
		priv->extra_ie_len = 0;
	}
}
void atbmwifi_wpa_event_assciate(struct atbmwifi_vif *priv)
{
	__wpa_prepare_assciating(priv);
}
void wpa_prepare_assciating(struct atbmwifi_vif *priv)
{
	atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,ATBM_NULL,WPA_EVENT__SUPPLICANT_ASSOCIAT,ATBM_WPA_EVENT_NOACK);	 /*Already in HIF task*/
}
atbm_void wpa_disconnect(struct atbmwifi_vif *priv)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	wpa_s->wpa_state = ATBM_WPA_DISCONNECTED;
}
/*connect retry also call this function*/
atbm_void wpa_deauthen(struct atbmwifi_vif *priv)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(priv->appdata);
	struct atbmwifi_wpa_sm         *wpa = wpa_s->wpa;

	wifi_printk(WIFI_ALWAYS,"wpa_s->wpa_state =%d\n",wpa_s->wpa_state);
	if(wpa_s->wpa_state >= ATBM_WPA_AUTHENTICATING)
		atbmwifi_ieee80211_tx_mgmt_deauth(priv,priv->bssid,priv->bssid,ATBM_WLAN_REASON_DEAUTH_LEAVING);
	
	wpa_s->wpa_state = ATBM_WPA_DISCONNECTED;


	if(wpa)	{
		if(wpa->ap_wpa_ie){
			atbm_kfree(wpa->ap_wpa_ie);
			wpa->ap_wpa_ie = ATBM_NULL;
			wpa->ap_wpa_ie_len = 0;
		}
		if(wpa->ap_rsn_ie){
			atbm_kfree(wpa->ap_rsn_ie);
			wpa->ap_rsn_ie = ATBM_NULL;
			wpa->ap_rsn_ie_len = 0;
		}

		if(wpa->assoc_wpa_ie){
			atbm_kfree(wpa->assoc_wpa_ie);
			wpa->assoc_wpa_ie = ATBM_NULL;
			wpa->assoc_wpa_ie_len = 0;
		}
	}
	atbmwifi_eloop_cancel_timeout(wpa_authen_assc_timeout,priv,ATBM_NULL);
	atbmwifi_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, ATBM_NULL);

	if(priv->scan_expire< 60)
		priv->scan_expire++;
	wifi_printk((WIFI_WPA|WIFI_CONNECT),"wpa_deauthen\n");
}

#if CONFIG_WPS
 static int wpa_wsc_build_identity(atbm_uint8 *buf, atbm_uint8 identifier)
{
	atbm_uint8 *pos = buf;

	if(pos == ATBM_NULL){
		wifi_printk(WIFI_WPS, "WPS: wpa_wsc_build_identity error\n");
		return -1;
	}

	*pos++ = EAP_CODE_RESPONSE;
	*pos++ = identifier;

	*pos++ = 0;
	*pos++ = WSC_ID_ENROLLEE_IE_LEN;

	*pos++ = ATBM_EAP_TYPE_IDENTITY;
	atbm_memcpy(pos, WSC_ID_ENROLLEE, WSC_ID_ENROLLEE_LEN);

	return 0;
}

 int wpa_wsc_rx_process(struct atbmwifi_vif *priv, atbm_uint8 *buf, atbm_size_t len)
{
	int ret = 0;
	atbm_uint8 *pos;
	atbm_uint8 *id_buf;
	atbm_uint16 length;
	EapType eap_type;
	struct atbm_eap_hdr *hdrEap;
	struct atbmwifi_ieee802_1x_hdr *hdr8021x;
	struct wpa_supplicant *wpa_s;
	struct wpabuf *wpa_buf;
	struct wpabuf *sendbuf;
	struct wps_data *wps;

	if(len < sizeof(*hdr8021x) + sizeof(*hdrEap)){
		wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process pkt length	is invalid(%d)\n", len);
		ret = -1;
		goto __error;
	}

	//wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process len(%d)\n", len);

	wpa_s = (struct wpa_supplicant *)priv->appdata;
	wps = wpa_s->wsc_data->wps;
	pos = buf;

	hdr8021x = (struct atbmwifi_ieee802_1x_hdr *)buf;
	hdrEap = (struct atbm_eap_hdr *)(hdr8021x + 1);

	if(hdr8021x->version < EAPOL_VERSION){
		wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process version is invalid(%d).\n", hdr8021x->version);
	}

	if(hdr8021x->type != ATBM_IEEE802_1X_TYPE_EAP_PACKET){
		wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process 8021X type is invalid(%d).\n", hdr8021x->type);
		ret = -1;
		goto __error;
	}

	if(hdr8021x->length != hdrEap->length){
		wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process hdr len invalid(%d, %d).\n", hdr8021x->length, hdrEap->length);
		ret = -4;
		goto __error;
	}

	//Complete WPS protocol
	if(hdrEap->code == EAP_CODE_FAILURE && wps->state == WPS_FINISHED){
		wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process Recv EAP Faolure\n" );
		sta_deauth(priv);
		return ret;
	}else if(wps->state == WPS_FINISHED){
		wifi_printk(WIFI_WPS, "WPS: finished--no handle \n");
		return ret;
	}

	//point to the EAP Type
	pos += sizeof(*hdr8021x) + sizeof(*hdrEap);
	eap_type = *pos;
	switch(eap_type){
		case ATBM_EAP_TYPE_IDENTITY:
			id_buf = (atbm_uint8 *)atbm_kmalloc(WSC_ID_ENROLLEE_IE_LEN, GFP_KERNEL);
			if(id_buf == ATBM_NULL){
				wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process id_buf alloc failed.\n");
				ret = -5;
				goto __error;
			}
			atbm_memset(id_buf, 0, WSC_ID_ENROLLEE_IE_LEN);
			wpa_wsc_build_identity(id_buf, hdrEap->identifier);
			wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process identity\n");
			//sys_dump_mem(id_buf, WSC_ID_ENROLLEE_IE_LEN);
			if(wpa_wsc_tx_process(wpa_s, ATBM_IEEE802_1X_TYPE_EAP_PACKET, id_buf, WSC_ID_ENROLLEE_IE_LEN))
				wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process eap id resp failed.\n");

			if(id_buf != ATBM_NULL)
				atbm_kfree(id_buf);

			break;
		case ATBM_EAP_TYPE_EXPANDED:/*WPS*/
			length = atbm_be_to_host16(hdrEap->length);
			pos = buf + sizeof(*hdr8021x);
			wpa_buf = wpabuf_alloc_copy(pos, length);
			if(wpa_buf == ATBM_NULL){
				wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process wpabuf failed.\n");
				ret = -6;
				goto __error;
			}
			//wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process expanded\n");

			sendbuf = wpas_eap_wsc_process(wpa_s, wpa_buf);
			if(sendbuf == ATBM_NULL){
				wpabuf_free(wpa_buf);
				ret = -7;
				goto __error;
			}

			if(wpa_wsc_tx_process(wpa_s, ATBM_IEEE802_1X_TYPE_EAP_PACKET, sendbuf->buf, sendbuf->used) < 0)
				wifi_printk(WIFI_ALWAYS, "WPS: wpa_wsc_rx_process eap wsc resp failed.\n");

			if(wpa_buf != ATBM_NULL)
				wpabuf_free(wpa_buf);

			if(sendbuf != ATBM_NULL)
				wpabuf_free(sendbuf);

			break;
		default:
			wifi_printk(WIFI_WPS, "WPS: wpa_wsc_rx_process eap_type unknown(%d)\n", eap_type);
			break;
	}
__error:
	return ret;
}

 int wpa_wsc_tx_process(atbm_void *ctx, int type, const atbm_uint8 *buf, atbm_size_t len)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)ctx;
	struct atbmwifi_vif *priv = wpa_get_driver_priv(wpa_s->priv);
	atbm_uint8 *msg, *dst/*, bssid[ATBM_ETH_ALEN]*/;
	atbm_size_t msglen;
	int res;

	if(atbm_is_zero_ether_addr(wpa_s->bssid)){
		wifi_printk(WIFI_DBG_ERROR, "BSSID not set when trying to send an EAPOL frame\n");
		return -1;
	}
	dst = wpa_s->bssid;

	msg = wpa_alloc_eapol(type, buf, len, &msglen, ATBM_NULL);
	if(msg == ATBM_NULL)
		return -1;
	wifi_printk(WIFI_DBG_MSG, "TX EAPOL: dst = "MACSTR" \n", MAC2STR(dst));
	res = wpa_drv_send_eapol(priv, dst, ATBM_ETH_P_EAPOL, msg, msglen);

	atbm_kfree(msg);
	return res;
}
#endif
