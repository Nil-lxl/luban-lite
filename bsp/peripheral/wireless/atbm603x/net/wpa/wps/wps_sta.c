/**************************************************************************************************************
* altobeam IOT Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "wps_sta.h"
#include "wps_dev_attr.h"
#include "wpa_debug.h"

#define WPS_DEVICE_NAME "ALTOBEAM_RTOS"
#define WPS_MANUFACTURER "ALTOBEAM"
#define WPS_MODEL_NAME	"iot_wifi"
#define WPS_MODEL_NUMBER "123"
#define WPS_SERIAL_NUMBER "12345678"
#define WPS_OS_VERSION 1

 atbm_void wpas_wps_timeout(void *eloop_ctx, atbm_void *timeout_ctx);
 atbm_void wpas_clear_wps(struct wpa_supplicant *wpa_s)
{
	atbmwifi_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);
	wpa_s->wps_mode = WPS_MODE_UNKNOWN;
	wpa_s->wps_ap_cnt = 0;
	if(wpa_s->pin != NULL){
		atbm_kfree(wpa_s->pin);
		wpa_s->pin = NULL;
	}
	//wpas_wps_clear_ap_info(wpa_s);
	return;
}
/**
 * wpa_config_update_psk - Update WPA PSK based on passphrase and SSID
 * @ssid: Pointer to network configuration data
 *
 * This function must be called to update WPA PSK when either SSID or the
 * passphrase has changed for the network configuration.
 */
 static atbm_void wpa_config_update_psk(struct atbm_wpa_ssid *ssid)
{
	if (ssid->passphrase) {
		atbm_pbkdf2_sha1(ssid->passphrase,
			    (char *) ssid->ssid, ssid->ssid_len, 4096,
			    ssid->psk, ATBM_PMK_LEN);
		ssid->psk_set = 1;
	}
}
/**
 * wpa_config_set_network_defaults - Set network default values
 * @ssid: Pointer to network configuration data
 */
 static atbm_void wpa_config_set_network_defaults(struct atbm_wpa_ssid *ssid)
{
	ssid->proto = ATBM_DEFAULT_PROTO;
	ssid->pairwise_cipher = ATBM_DEFAULT_PAIRWISE;
	ssid->group_cipher = ATBM_DEFAULT_GROUP;
	ssid->key_mgmt = ATBM_DEFAULT_KEY_MGMT;
	ssid->bg_scan_period = -1;
#ifdef IEEE8021X_EAPOL
	ssid->eapol_flags = ATBM_DEFAULT_EAPOL_FLAGS;
	ssid->eap_workaround = ATBM_DEFAULT_EAP_WORKAROUND;
	ssid->eap.fragment_size = ATBM_DEFAULT_FRAGMENT_SIZE;
#endif /* IEEE8021X_EAPOL */
}


 atbm_void wpa_wps_done(atbm_void *_wpa_s,atbm_void *data)
{
	struct atbm_wpa_ssid *ssid = (struct atbm_wpa_ssid *)data;
	struct wpa_supplicant *wpa_s=(struct wpa_supplicant *)_wpa_s;
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)wpa_get_driver_priv(wpa_s->priv);
#if CONFIG_P2P
	if(!(priv->p2p_join || priv->p2p_ap))
#endif
		wpas_deinit_wps(wpa_s);
	if(ssid != NULL){
		atbmwifi_event_uplayer(priv, ATBM_WIFI_WPS_SUCCESS, (atbm_uint8*)ssid);
		wpa_printf(MSG_ALWAYS, "WPS:wps_cred free %d ssid %s",__LINE__,ssid->ssid);
		if(ssid->ssid != NULL)
			atbm_kfree(ssid->ssid);

		if(ssid->passphrase != NULL)
			atbm_kfree(ssid->passphrase);
		
		atbm_kfree(ssid);
	}
}

 static int wpa_supplicant_wps_cred(void *ctx,
				   const struct wps_credential *cred)
{
	int ret = 0;
	struct wpa_supplicant *wpa_s = ctx;
	struct atbm_wpa_ssid *ssid=ATBM_NULL;
    struct atbmwifi_vif *priv = (struct atbmwifi_vif *)wpa_get_driver_priv(wpa_s->priv);

	//configure info
//	struct atbmwifi_vif *priv;
	//struct wifi_configure *config;
	atbm_uint8 key_mgmt=ATBM_KEY_NONE;
	
	//atbm_uint8 key_idx = 0;
	atbm_uint16 auth_type;
#ifdef CONFIG_WPS_REG_DISABLE_OPEN
	int registrar = 0;
#endif /* CONFIG_WPS_REG_DISABLE_OPEN */

	ssid = (struct atbm_wpa_ssid *)atbm_kzalloc(sizeof(*ssid), GFP_KERNEL);
	if(ssid == NULL){
		wpa_printf(MSG_ERROR, "WPS: WPS-CRED-RECEIVED ssid malloc failed.");
		return -1;
	}
	
	
	//wpa_printf(MSG_ALWAYS, "WPS: cred stack sz.s %d",atbm_getThreadStackFreesize());
	
	wpa_printf(MSG_ALWAYS, "WPS: cred, SSID %s len %d \n",cred->ssid, cred->ssid_len);
	if(cred->key_len == 64){
		wpa_hexdump(MSG_ALWAYS, "WPS: cred, Network Key \n", cred->key, cred->key_len);
	}else{
		wpa_printf(MSG_ALWAYS, "WPS: cred, Password %s len %d \n",cred->key, cred->key_len);
	}
	wpa_printf(MSG_ALWAYS, "WPS: cred, Authentication Type 0x%x \n",cred->auth_type);
	wpa_printf(MSG_ALWAYS, "WPS: cred, Encryption Type 0x%x \n", cred->encr_type);
	wpa_printf(MSG_ALWAYS, "WPS: cred, Network Key Index %d \n", cred->key_idx);
	wpa_printf(MSG_ALWAYS, "WPS: cred, MAC Address "MACSTR" \n",MAC2STR(cred->mac_addr));

	auth_type = cred->auth_type;
	if (auth_type == (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) {
		wpa_printf(MSG_DEBUG, "WPS: Workaround - convert mixed-mode auth_type into WPA2PSK \n");
		auth_type = WPS_AUTH_WPA2PSK;
	}

	if (auth_type != WPS_AUTH_OPEN &&
	    auth_type != WPS_AUTH_SHARED &&
	    auth_type != WPS_AUTH_WPAPSK &&
	    auth_type != WPS_AUTH_WPA2PSK) {
		wpa_printf(MSG_ERROR, "WPS: Ignored credentials for "
			   "unsupported authentication type 0x%x \n",
			   auth_type);
		ret = -1;
		goto __error_;
	}


	wpa_config_set_network_defaults(ssid);

	ssid->ssid = (atbm_uint8 *)atbm_kzalloc(cred->ssid_len+1, GFP_KERNEL);
	if (ssid->ssid) {
		atbm_memcpy(ssid->ssid, cred->ssid, cred->ssid_len);
		ssid->ssid_len = cred->ssid_len;
	}else{
		ret = -1;
        wpa_printf(MSG_ERROR, "WPS: WPS-CRED-RECEIVED ssid->ssid malloc failed. \n");
		goto __error_;
    }

	switch (cred->encr_type) {
	case WPS_ENCR_NONE:
		break;

//because wps certification wep mode can't used wps, so if wep mode just fail
#if 0
	case WPS_ENCR_WEP:
		if (cred->key_len <= 0)
			break;
		if (cred->key_len != 5 && cred->key_len != 13 &&
		    cred->key_len != 10 && cred->key_len != 26) {
			wpa_printf(MSG_ERROR, "WPS: Invalid WEP Key length "
				   "%lu", (unsigned long) cred->key_len);
			ret = -1;
			goto __error_;
		}
		if (cred->key_idx > ATBM_NUM_WEP_KEYS) {
			wpa_printf(MSG_ERROR, "WPS: Invalid WEP Key index %d",
				   cred->key_idx);
			ret = -1;
			goto __error_;
		}
		if (cred->key_idx)
			key_idx = cred->key_idx - 1;
		if (cred->key_len == 10 || cred->key_len == 26) {
			if (atbmwifi_hexstr2bin(ssid->wep_key[key_idx],
						(char *) cred->key,
				       	cred->key_len / 2) < 0) {
				wpa_printf(MSG_ERROR, "WPS: Invalid WEP Key "
					   "%d", key_idx);
				ret = -1;
				goto __error_;
			}
			ssid->wep_key_len[key_idx] = cred->key_len / 2;
		} else {
			atbm_memcpy(ssid->wep_key[key_idx], cred->key,
				  cred->key_len);
			ssid->wep_key_len[key_idx] = cred->key_len;
		}
		ssid->wep_tx_keyidx = key_idx;
		break;
#endif //0
	case WPS_ENCR_TKIP:
		ssid->pairwise_cipher = ATBM_WPA_CIPHER_TKIP;
		break;
	case WPS_ENCR_AES:
		ssid->pairwise_cipher = ATBM_WPA_CIPHER_CCMP;
		break;
	default:
		ret = -1;
		goto __error_;
	}

	switch (auth_type) {
	case WPS_AUTH_OPEN:
		ssid->auth_alg = ATBM_WPA_AUTH_ALG_OPEN;
		ssid->key_mgmt = ATBM_WPA_KEY_MGMT_NONE;
		ssid->proto = 0;
#ifdef CONFIG_WPS_REG_DISABLE_OPEN
		if (registrar) {
			wpa_msg(wpa_s, MSG_INFO, WPS_EVENT_OPEN_NETWORK
				"id=%d - Credentials for an open "
				"network disabled by default - use "
    				"'select_network %d' to enable \n",
				ssid->id, ssid->id);
			ssid->disabled = 1;
		}
#endif /* CONFIG_WPS_REG_DISABLE_OPEN */

		key_mgmt = ATBM_KEY_NONE;
		break;
//because wps certification wep mode can't used wps, so if wep mode just fail
#if 0
	case WPS_AUTH_SHARED:
		ssid->auth_alg = ATBM_WPA_AUTH_ALG_SHARED;
		ssid->key_mgmt = ATBM_WPA_KEY_MGMT_NONE;
		ssid->proto = 0;
		key_mgmt = ATBM_KEY_WEP_SHARE;
		break;
#endif //0
	case WPS_AUTH_WPAPSK:
		ssid->auth_alg = ATBM_WPA_AUTH_ALG_OPEN;
		ssid->key_mgmt = ATBM_WPA_KEY_MGMT_PSK;
		ssid->proto = ATBM_WPA_PROTO_WPA;
		key_mgmt = ATBM_KEY_WPA;
		break;
	case WPS_AUTH_WPA2PSK:
		ssid->auth_alg = ATBM_WPA_AUTH_ALG_OPEN;
		ssid->key_mgmt = ATBM_WPA_KEY_MGMT_PSK;
		ssid->proto = ATBM_WPA_PROTO_RSN;
		key_mgmt = ATBM_KEY_WPA2;
		break;
	default:
		ret = -1;
		goto __error_;
	}

	if (ssid->key_mgmt == ATBM_WPA_KEY_MGMT_PSK) {
		if (cred->key_len == 2 * ATBM_PMK_LEN) {
			if (atbmwifi_hexstr2bin(ssid->psk, (const char *) cred->key, ATBM_PMK_LEN)) {
				wpa_printf(MSG_ERROR, "WPS: Invalid Network Key");
				ret = -1;
				goto __error_;
			}
			ssid->passphrase = atbm_kmalloc(cred->key_len + 1, GFP_KERNEL);
			if (ssid->passphrase == NULL){
				wpa_printf(MSG_ERROR, "WPS: atbm_kmalloc passphrase fail");
				ret = -1;
				goto __error_;
			}
			//64 key has process in connect fn
			atbm_memcpy(ssid->passphrase, cred->key, cred->key_len);	
			ssid->passphrase[cred->key_len] = '\0';
			ssid->psk_set = 1;
			ssid->export_keys = 1;
		} else if (cred->key_len >= 8 && cred->key_len < 2 * ATBM_PMK_LEN) {
			//atbm_kfree(ssid->passphrase);
			ssid->passphrase = atbm_kmalloc(cred->key_len + 1, GFP_KERNEL);
			if (ssid->passphrase == NULL){
				wpa_printf(MSG_ERROR, "WPS: atbm_kmalloc passphrase fail");
				ret = -1;
				goto __error_;
			}
			
			atbm_memcpy(ssid->passphrase, cred->key, cred->key_len);
			ssid->passphrase[cred->key_len] = '\0';
			wpa_config_update_psk(ssid);
			ssid->export_keys = 1;
		} else {
			wpa_printf(MSG_ERROR, "WPS: Invalid Network Key "
				   "length %lu",
				   (unsigned long) cred->key_len);
			ret = -1;
			goto __error_;
		}
	}

	//wpas_wps_security_workaround(wpa_s, ssid, cred);
	//Console_SetPolling(1);
	#if 1
	//priv = wpa_s->priv;
//	atbm_wifi_disconnect();

//	 AT_WDisConnect(ATBM_NULL);
	
	wpas_clear_wps(wpa_s);

	atbmwifi_eloop_register_timeout(1, 0, wpa_wps_done,(atbm_uint8 *)wpa_s,(atbm_uint8 *)ssid);
	//priv->auto_connect_when_lost = 1;

	//connect to the AP
//	atbm_wifi_set_config(ssid->ssid, ssid->ssid_len, ssid->passphrase, cred->key_len, key_mgmt, ssid->wep_tx_keyidx, NULL);

//	atbm_wifi_connect_ap();

	//wpa_printf(MSG_ALWAYS, "WPS: cred stack sz.e %d", atbm_getThreadStackFreesize());

	//config = priv->hw_priv->config;
	//save into flash at here ???
	//atbmwifi_flash_param_wificonfig_change();
	#else
	atbm_wifi_set_config(ssid->ssid, ssid->ssid_len, ssid->passphrase, cred->key_len, key_mgmt, ssid->wep_tx_keyidx);
	#endif
    return ret;
    
__error_:
    wpas_clear_wps(wpa_s);
    atbmwifi_eloop_register_timeout(1, 0, wpa_wps_done,(atbm_uint8 *)wpa_s,(atbm_uint8 *)ssid);
    atbmwifi_event_uplayer(priv, ATBM_WIFI_DEASSOC_EVENT, (atbm_uint8*)ssid);

	return ret;
}
#if 0
static atbm_void wpa_supplicant_wps_event_success(struct wpa_supplicant *wpa_s)
{
	struct atbmwifi_vif *priv = wpa_s->priv;

	atbmwifi_ieee80211_connection_loss(priv);

	//atbm_wifi_disconnect();
	priv->auto_connect_when_lost = 1;
	sta_deauth(priv);

	atbm_SleepMs(50);
	sta_deauth(priv);

	wpas_clear_wps(wpa_s);

	wpas_deinit_wps(wpa_s);

	//peterjiang@20200404, compiled error, removed.
	//atbm_wifi_connect_ap();
	
	//save into flash at here ???
	//atbmwifi_flash_param_wificonfig_change();
	
#if CONFIG_P2P
#endif /* CONFIG_P2P */

	return;
}
#endif
 static atbm_void wpa_supplicant_wps_event(void *ctx, enum wps_event event,
				     union wps_event_data *data)
{
//	struct wpa_supplicant *wpa_s = ctx;
	wpa_printf(MSG_ALWAYS, "WPS: wpa_supplicant_wps_event(), event=%d", event);
	switch (event) {
	case WPS_EV_M2D:
		//wpa_supplicant_wps_event_m2d(wpa_s, &data->m2d);
		break;
	case WPS_EV_FAIL:
		//wpa_supplicant_wps_event_fail(wpa_s, &data->fail);
		break;
	case WPS_EV_SUCCESS:
		//wpa_supplicant_wps_event_success(wpa_s);
		break;
	case WPS_EV_PWD_AUTH_FAIL:
#ifdef CONFIG_AP
		//if (wpa_s->ap_iface && data->pwd_auth_fail.enrollee)
		//	wpa_supplicant_ap_pwd_auth_fail(wpa_s);
#endif /* CONFIG_AP */
		break;
	case WPS_EV_PBC_OVERLAP:
		break;
	case WPS_EV_PBC_TIMEOUT:
		break;
	case WPS_EV_ER_AP_ADD:
		//wpa_supplicant_wps_event_er_ap_add(wpa_s, &data->ap);
		break;
	case WPS_EV_ER_AP_REMOVE:
		//wpa_supplicant_wps_event_er_ap_remove(wpa_s, &data->ap);
		break;
	case WPS_EV_ER_ENROLLEE_ADD:
		//wpa_supplicant_wps_event_er_enrollee_add(wpa_s, &data->enrollee);
		break;
	case WPS_EV_ER_ENROLLEE_REMOVE:
		//wpa_supplicant_wps_event_er_enrollee_remove(wpa_s,&data->enrollee);
		break;
	case WPS_EV_ER_AP_SETTINGS:
		//wpa_supplicant_wps_event_er_ap_settings(wpa_s,&data->ap_settings);
		break;
	case WPS_EV_ER_SET_SELECTED_REGISTRAR:
		//wpa_supplicant_wps_event_er_set_sel_reg(wpa_s,&data->set_sel_reg);
		break;
	case WPS_EV_AP_PIN_SUCCESS:
		break;
	case WPS_EV_PBC_ACTIVE:
		break;
	case WPS_EV_PBC_DISABLE:
		break;
	default:
		break;
	}
}
/**
 * wpa_supplicant_cancel_scan - Cancel a scheduled scan request
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to cancel a scan request scheduled with
 * wpa_supplicant_req_scan().
 */
 static atbm_void wpa_supplicant_cancel_scan(struct wpa_supplicant *wpa_s)
{
	//wpa_printf(MSG_DEBUG, "WPS: Cancelling scan request\n");
	//atbmwifi_eloop_cancel_timeout(wpa_supplicant_scan, wpa_s, NULL);
}

/**
 * wps_deinit - Deinitialize WPS Registration protocol data
 * @data: WPS Registration protocol data from wps_init()
 */
  atbm_void wps_deinit(struct wps_data *data)
{
#ifdef CONFIG_WPS_NFC
	if (data->registrar && data->nfc_pw_token)
		wps_registrar_remove_nfc_pw_token(data->wps->registrar,
						  data->nfc_pw_token);
#endif /* CONFIG_WPS_NFC */
	if(data == NULL)
		return;

	if (data->wps_pin_revealed) {
		wpa_printf(MSG_ERROR, "WPS: Full PIN information revealed and "
			   "negotiation failed");
	} else if (data->registrar){
	}


	wpabuf_free(data->dh_pubkey);
	data->dh_pubkey=NULL;

	wpabuf_free(data->dh_privkey);
	data->dh_privkey=NULL;

	wpabuf_free(data->dh_pubkey_e);
	data->dh_pubkey_e=NULL;

	wpabuf_free(data->dh_pubkey_r);
	data->dh_pubkey_r=NULL;
	
	
	wpabuf_free(data->last_msg);
	data->last_msg=NULL;

	if(data->dev_password)
		atbm_kfree(data->dev_password);
	data->dev_password=NULL;

	if(data->new_psk)
		atbm_kfree(data->new_psk);
	data->new_psk=NULL;

	wps_device_data_free(&data->peer_dev);

	if(data->new_ap_settings)
		atbm_kfree(data->new_ap_settings);
	data->new_ap_settings=NULL;	

	dh5_free(data->dh_ctx);

#ifdef CONFIG_WPS_NFC
	atbm_kfree(data->nfc_pw_token);
#endif
	atbm_kfree(data);
}


/**
 * wps_init - Initialize WPS Registration protocol data
 * @cfg: WPS configuration
 * Returns: Pointer to allocated data or %NULL on failure
 *
 * This function is used to initialize WPS data for a registration protocol
 * instance (i.e., each run of registration protocol as a Registrar of
 * Enrollee. The caller is responsible for freeing this data after the
 * registration run has been completed by calling wps_deinit().
 */
 static struct wps_data * wps_init(struct wpa_supplicant *wpa_s)
{
	struct eap_wsc_data *wsc_data = wpa_s->wsc_data;
#if CONFIG_P2P
	struct atbmwifi_vif *priv = wpa_s->priv;
	struct wpabuf *privkey = (struct wpabuf *)priv->p2p_wps_privkey;
	struct wpabuf *pubkey = (struct wpabuf *)priv->p2p_wps_pubkey;
#endif

	struct wps_data *data = (struct wps_data *)atbm_kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL){
		wpa_printf(MSG_ERROR, "WPS: wps_init, wsc data malloc failed.");
		return NULL;
	}
	data->wsc_data = wpa_s->wsc_data;
	data->wps = wsc_data->wps_ctx;
	data->registrar = wsc_data->registrar;
	if (wsc_data->registrar) {
		atbm_memcpy(data->uuid_r, wsc_data->wps_ctx->uuid, WPS_UUID_LEN);
	} else {
		atbm_memcpy(data->mac_addr_e, wsc_data->wps_ctx->dev.mac_addr, ATBM_ETH_ALEN);
		atbm_memcpy(data->uuid_e, wsc_data->wps_ctx->uuid, WPS_UUID_LEN);
	}
	
	wpa_printf(MSG_DEBUG, "WPS: wps_init wpas mac addr="MACSTR"", MAC2STR(data->mac_addr_e));
	//sys_dump_mem(data->uuid_e, WPS_UUID_LEN);
	switch(wpa_s->wps_mode){
		case WPS_MODE_PIN:
			data->dev_pw_id = DEV_PW_DEFAULT;
			data->dev_password = (atbm_uint8 *) strdup((char *)wpa_s->pin);
			if (data->dev_password == NULL) {
				wpa_printf(MSG_ERROR, "WPS: wps_init, pin dev psw is NULL.");
				goto __error__;
			}

			data->dev_password_len = PIN_CODE_LENGTH;

			break;
		case WPS_MODE_PBC:
			/* Use special PIN '00000000' for PBC */
			data->dev_pw_id = DEV_PW_PUSHBUTTON;

			if(data->dev_password != NULL)
				atbm_kfree(data->dev_password);

			data->dev_password = (atbm_uint8 *) strdup("00000000");
			if (data->dev_password == NULL) {
				wpa_printf(MSG_ERROR, "WPS: wps_init, pbc dev psw is NULL.");
				goto __error__;
			}
			data->dev_password_len = 8;

			break;
		default:
			wpa_printf(MSG_ERROR, "WPS: wps_init, wps mode invalid(%d).", wpa_s->wps_mode);
			goto __error__;

	}
#if CONFIG_P2P
	if(priv->iftype == ATBM_NL80211_IFTYPE_P2P_CLIENT){
		if(privkey && pubkey){
			data->dh_privkey = wpabuf_alloc(privkey->size);
			if(data->dh_privkey){
				atbm_memcpy(data->dh_privkey->buf, privkey->buf, privkey->size);
				data->dh_privkey->size = privkey->size;
				data->dh_privkey->flags = privkey->flags;
				data->dh_privkey->used = privkey->used;
			}

			data->dh_pubkey = wpabuf_alloc(pubkey->size);
			if(data->dh_pubkey){
				atbm_memcpy(data->dh_pubkey->buf, pubkey->buf, pubkey->size);
				data->dh_pubkey->size = pubkey->size;
				data->dh_pubkey->flags = pubkey->flags;
				data->dh_pubkey->used = pubkey->used;
			}
			if(data->dh_privkey && data->dh_pubkey){
				data->dh_ctx = (void *)1;
			}
		}else{
			wpa_printf(MSG_ERROR, "WPS: dh5_init failed, dh_ctx is NULL");
		}
	}else
#endif
	{
		data->dh_ctx = dh5_init(&data->dh_privkey, &data->dh_pubkey);
		if(data->dh_ctx == NULL){
			wpa_printf(MSG_ERROR, "WPS: dh5_init failed, dh_ctx is NULL");
			goto __error__;
		}
	}

	data->state = data->registrar ? RECV_M1 : SEND_M1;

	return data;

__error__:
	if(data != NULL)
		atbm_kfree(data);
	if(data->dev_password != NULL)
		atbm_kfree(data->dev_password);

	return NULL;
}

 static atbm_void wpas_eap_wsc_deinit(struct wpa_supplicant *wpa_s)
{
	struct eap_wsc_data *data;

	data = wpa_s->wsc_data;

	if(data == NULL)
		return;
	wpabuf_free(data->in_buf);
	data->in_buf = NULL;

	wpabuf_free(data->out_buf);
	data->out_buf = NULL;
	
	wps_deinit(data->wps);
	if(data->wps_ctx->network_key)
		atbm_kfree(data->wps_ctx->network_key);	
	data->wps_ctx->network_key = NULL;

	if(data->wps_ctx)
		atbm_kfree(data->wps_ctx);
	data->wps_ctx = NULL;
	atbm_kfree(data);
	wpa_s->wsc_data = NULL;

	return;
}

/**
 * wpas_eap_wsc_init -init eap wsc for STA.
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at program initialization of WPS STA. 
 */
#if 0
 static int wpas_eap_wsc_init(struct atbmwifi_vif *priv)
{
	int ret = 0;
	struct eap_wsc_data *wsc_data;
	struct wpa_supplicant *wpa_s;
	int registrar;

	if (priv == NULL) {
		wpa_printf(MSG_ERROR, "WPS: EAP-WSC-INIT: priv is not available");
		ret = -1;
		goto __error__;
	}

	wpa_s = (struct wpa_supplicant *)priv->appdata;
	if (wpa_s == NULL) {
		wpa_printf(MSG_ERROR, "WPS: EAP-WSC-INIT: wpa_s is not available");
		ret = -1;
		goto __error__;
	}

	if (priv->iftype == ATBM_NL80211_IFTYPE_AP || priv->iftype == ATBM_NL80211_IFTYPE_P2P_GO)
		registrar = 1; /* Supplicant is Registrar */
	else if (priv->iftype == ATBM_NL80211_IFTYPE_STATION || priv->iftype == ATBM_NL80211_IFTYPE_P2P_CLIENT)
		registrar = 0; /* Supplicant is Enrollee */
	else {
		wpa_printf(MSG_ERROR, "WPS: EAP-WSC-INIT: Unexpected iftype(%d)", priv->iftype);
		ret = -1;
		goto __error__;
	}

	wsc_data = (struct eap_wsc_data *)atbm_kzalloc(sizeof(*wsc_data), GFP_KERNEL);
	if (wsc_data == NULL){
		wpa_printf(MSG_ERROR, "WPS: EAP-WSC-INIT: eap wsc data memory is not alloc");
		ret = -1;
		goto __error__;
	}

	wpa_s->wsc_data = wsc_data; /*wpa_s point to the wsc data*/

	wsc_data->state = registrar ? MESG : WAIT_START;
	wsc_data->registrar = registrar;
	
	wsc_data->wps_ctx = atbm_kzalloc(sizeof(struct wps_context), GFP_KERNEL);
	if(wsc_data->wps_ctx == NULL){
		wpa_printf(MSG_ERROR, "WPS: EAP-WSC-INIT: wps ctx memory is not alloc");
		ret = -1;
		goto __error__;
	}

	wsc_data->wps = wps_init(wpa_s);
	if (wsc_data->wps == NULL) {
		atbm_kfree(wsc_data);
		ret = -1;
		goto __error__;
	}

	wsc_data->fragment_size = WSC_FRAGMENT_SIZE;
	wpa_printf(MSG_DEBUG, "WPS: EAP-WSC-INIT: Fragment size limit %u",
		   (atbm_uint32) wsc_data->fragment_size);

__error__:
	return ret;
}
#endif

/**
 * wpas_wps_set_uuid - gennerate uuid according to the MAC Address.
 * Returns: NULL
 *
 * This function is called at program initialization of WPS STA. 
 */
 static atbm_void wpas_wps_set_uuid(struct wpa_supplicant *wpa_s,
			      struct wps_context *wps)
{

	uuid_gen_mac_addr(wpa_s->own_addr, wps->uuid);
	//wpa_hexdump(MSG_DEBUG, "WPS: UUID based on MAC address\n", wps->uuid, WPS_UUID_LEN);
}

 atbm_void wpas_wps_timeout(void *eloop_ctx, atbm_void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s;

	wpa_s = (struct wpa_supplicant *)eloop_ctx;

	wpa_printf(MSG_ALWAYS, "WPS: Requested operation timed out");
    //peterjiang@20200531,fix bug #35766
    if(wpa_s == ATBM_NULL){
        wpa_printf(MSG_ALWAYS, "WPS: wps has been cannelled \n");
        return;
    }
	wpas_clear_wps(wpa_s);
	wpas_deinit_wps(wpa_s);

	return;
}


/**
 * wpas_cancel_wps - cancel wps conenction
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at program cancel wps connection for STA mode.
 */
 int wpas_cancel_wps(struct wpa_supplicant *wpa_s)
{
	if((wpa_s->wsc_data == NULL) || (wpa_s->wsc_data->wps_ctx == NULL)){
		return -1;
	}
	if (wpa_s->wpa_state == ATBM_WPA_SCANNING ||
		wpa_s->wpa_state == ATBM_WPA_DISCONNECTED) {
		wpa_printf(MSG_DEBUG, "WPS: Cancel operation - cancel scan");
		wpa_supplicant_cancel_scan(wpa_s);
		wpas_clear_wps(wpa_s);
		wpas_deinit_wps(wpa_s);
	} else if (wpa_s->wpa_state >= ATBM_WPA_ASSOCIATED) {
		wpa_printf(MSG_DEBUG, "WPS: Cancel operation - deauthenticate");

		wpa_deauthen(wpa_s->priv);
		wpas_clear_wps(wpa_s);
		wpas_deinit_wps(wpa_s);
	}

	return 0;
}

/**
 * wpas_deinit_wps - destrory wps station structure
 * Returns: NULL
 *
 * This function is called at program over run to destroy wps STA.
 */
 atbm_void wpas_deinit_wps(struct wpa_supplicant *wpa_s)
{
	wpa_printf(MSG_ALWAYS, "wpas_deinit_wps ");
	wpas_eap_wsc_deinit(wpa_s);
	return;
}

/**
 * wpas_init_wps - init wps station structure
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at program initialization to enable wps station.
 */
 int wpas_init_wps(struct wpa_supplicant *wpa_s)
{
	int ret = 0;
	int registrar;
	struct wps_context *wps_ctx;
	struct atbmwifi_vif *priv;
	struct eap_wsc_data *wsc_data;
	//atbm_uint16 m;
	//WPS_DEV_OUI_WFA
	char wps_oui[] = {0x00, 0x50, 0xF2, 0x04};

	if(wpa_s == NULL){
		wpa_printf(MSG_ERROR, "WPS: wpas_init_wps, wpa_s is NULL.");
		ret = -1;
		goto __error__;
	}

	priv = wpa_s->priv;
	if(priv == NULL){
		wpa_printf(MSG_ERROR, "WPS: wpas_init_wps, priv is NULL.");
		ret = -1;
		goto __error__;
	}
	
	if (atbmwifi_is_ap_mode(priv->iftype))
		registrar = 1; /* Supplicant is Registrar */
	else if (atbmwifi_is_sta_mode(priv->iftype))
		registrar = 0; /* Supplicant is Enrollee */
	else {
		wpa_printf(MSG_ERROR, "WPS: wpas_init_wps, Unexpected iftype(%d).", priv->iftype);
		ret = -1;
		goto __error__;
	}

	//1. init eap wsc data
	wsc_data = (struct eap_wsc_data *)atbm_kzalloc(sizeof(*wsc_data), GFP_KERNEL);
	if (wsc_data == NULL){
		wpa_printf(MSG_ERROR, "WPS: wpas_init_wps, eap wsc data memory is not alloc.");
		ret = -1;
		goto __error__;
	}

	wpa_s->wsc_data = wsc_data; /*wpa_s point to the wsc data*/

	wsc_data->state = registrar ? MESG : WAIT_START;
	wsc_data->registrar = registrar;

	//2. init wps context
	wsc_data->wps_ctx = (struct wps_context *)atbm_kzalloc(sizeof(struct wps_context), GFP_KERNEL);
	if(wsc_data->wps_ctx == NULL){
		wpa_printf(MSG_ERROR, "WPS: wpas_init_wps, wps ctx memory is not alloc.");
		ret = -1;
		goto __error__;
	}

	wps_ctx = wsc_data->wps_ctx;

	wps_ctx->cred_cb = wpa_supplicant_wps_cred;
	wps_ctx->event_cb = wpa_supplicant_wps_event;
	wps_ctx->cb_ctx = wpa_s;
	wps_ctx->wps_state = WPS_STATE_NOT_CONFIGURED;

	wps_ctx->dev.device_name = WPS_DEVICE_NAME;
	wps_ctx->dev.manufacturer = WPS_MANUFACTURER;
	wps_ctx->dev.model_name = WPS_MODEL_NAME;
	wps_ctx->dev.model_number = WPS_MODEL_NUMBER;
	wps_ctx->dev.serial_number = WPS_SERIAL_NUMBER;
//	wps_ctx->config_methods = WPS_CONFIG_VIRT_PUSHBUTTON | WPS_CONFIG_LABEL;
	wps_ctx->config_methods = WPS_CONFIG_PHY_PUSHBUTTON | WPS_CONFIG_PHY_DISPLAY;
	if ((wps_ctx->config_methods & (WPS_CONFIG_DISPLAY | WPS_CONFIG_LABEL)) ==
		(WPS_CONFIG_DISPLAY | WPS_CONFIG_LABEL)) {
		wpa_printf(MSG_DEBUG, "WPS: Both Label and Display config "
			   "methods are not allowed at the same time");
		atbm_kfree(wps_ctx);
		ret = -1;
		goto __error__;
	}
	
	wps_ctx->dev.config_methods = wps_ctx->config_methods;

	/*primary device type*/
	if(priv->iftype == ATBM_NL80211_IFTYPE_AP){
		//Category(2 bytes)
		wps_ctx->dev.pri_dev_type[0] = 0x00;
		wps_ctx->dev.pri_dev_type[1] = Category_Network_Infrastructure;

		//Sub Category(2 bytes)
		wps_ctx->dev.pri_dev_type[6] = 0x00;
		wps_ctx->dev.pri_dev_type[7] = 0x01;//AP
	}else{
		//Category(2 bytes)
		wps_ctx->dev.pri_dev_type[0] = 0x00;
		wps_ctx->dev.pri_dev_type[1] = Category_Network_Infrastructure;

		//Sub Category(2 bytes)
		wps_ctx->dev.pri_dev_type[6] = 0x00;
		wps_ctx->dev.pri_dev_type[7] = 0x02;//Router
	}

	/*For the predefined values, the Wi-Fi Alliance OUI of 00 50 F2 04 is used.*/
	atbm_memcpy(&wps_ctx->dev.pri_dev_type[2], wps_oui, 4);

	//wps->dev.num_sec_dev_types = wpa_s->conf->num_sec_device_types;
	//atbm_memcpy(wps->dev.sec_dev_type, wpa_s->conf->sec_device_type,
	//	  WPS_DEV_TYPE_LEN * wps->dev.num_sec_dev_types);

	wps_ctx->dev.os_version = WPS_OS_VERSION;//ATBM_WPA_GET_BE32(wpa_s->conf->os_version);

	wps_ctx->dev.rf_bands |= WPS_RF_24GHZ;

	atbm_memcpy(wps_ctx->dev.mac_addr, wpa_s->priv->mac_addr, ATBM_ETH_ALEN);
	wpa_printf(MSG_DEBUG, "WPS: EAP-WSC-INIT wpas own addr="MACSTR"", MAC2STR(wpa_s->priv->mac_addr));
	wpas_wps_set_uuid(wpa_s, wps_ctx);

	wps_ctx->auth_types = WPS_AUTH_WPA2PSK | WPS_AUTH_WPAPSK;
	wps_ctx->encr_types = WPS_ENCR_AES | WPS_ENCR_TKIP;

	//3. init wps data
	wsc_data->wps = wps_init(wpa_s);
	if (wsc_data->wps == NULL) {
		atbm_kfree(wsc_data);
		ret = -1;
		goto __error__;
	}

	wsc_data->fragment_size = WSC_FRAGMENT_SIZE;
	wpa_printf(MSG_DEBUG, "WPS: EAP-WSC-INIT: Fragment size limit %u",
		   (atbm_uint32) wsc_data->fragment_size);

__error__:
	return ret;
}


 int wpas_wps_start_pbc(struct wpa_supplicant *wpa_s, const atbm_uint8 *bssid, int p2p_group)
{
	int ret = 0;
	struct atbmwifi_vif *priv = wpa_s->priv;

	sta_deauth(priv);
	//atbm_wifi_disconnect();

	wpas_clear_wps(wpa_s);

	wpa_s->wps_mode = WPS_MODE_PBC;
	ret = wpas_init_wps(wpa_s);
	if(ret < 0){
		wpa_printf(MSG_ERROR, "WPS: wpas_wps_start_pbc, wpas init wps failed.");
		goto __error__;
	}

	atbmwifi_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);
	atbmwifi_eloop_register_timeout(WPS_PBC_WALK_TIME, 0, wpas_wps_timeout, wpa_s, NULL);

//	wpa_supplicant_set_state(wpa_s, ATBM_WPA_SCANNING);
	wpa_s->wpa_state = ATBM_WPA_SCANNING;
	wpa_comm_init_extra_ie(priv);
	priv->scan_expire = INITIAL_SCAN_EXPIRE;

	atbmwifi_sta_scan(priv);
	if(priv->extra_ie){
		atbm_kfree(priv->extra_ie);
		priv->extra_ie =NULL;
		priv->extra_ie_len = 0;
	}

__error__:
	return ret;
}
 int wpas_wps_start_pin(struct wpa_supplicant *wpa_s, const atbm_uint8 *bssid,
			   const char *pin, int p2p_group, atbm_uint16 dev_pw_id)
{
	int ret = 0;
	struct atbmwifi_vif *priv;
	atbm_uint32 rpin = 0;
	priv = (struct atbmwifi_vif *)wpa_get_driver_priv(wpa_s->priv);

	//char val[128];

	//sta_deauth(priv);
//	atbm_wifi_disconnect();
#if ATBM_PLATFORM == AK_RTOS_37D
	{
		char cmd[10];
		strcpy(cmd, "wifi disc");
		msh_exec(cmd, strlen(cmd));
	}
#else
	 AT_WDisConnect(ATBM_NULL);
#endif

	wpas_clear_wps(wpa_s);

	wpa_s->pin = (atbm_uint8 *)atbm_kzalloc(PIN_CODE_LENGTH+1, GFP_KERNEL);
	wpa_s->scan_runs = 0;
	wpa_s->wps_pin_start_time = atbm_GetOsTime();
	if(wpa_s->pin == NULL){
		wpa_printf(MSG_ERROR, "WPS: wpas_wps_start_pin, pin code malloc is NULL.");
		ret = -1;
		goto __error__;
	}
	
	if (pin){
		if(strlen(pin) != PIN_CODE_LENGTH){
			wpa_printf(MSG_ERROR, "WPS: wpas_wps_start_pin, pin code length invalid,[%s] len %d", pin,strlen(pin));
			ret = -1;
			goto __error__;
		}

		atbm_memcpy(wpa_s->pin, pin, PIN_CODE_LENGTH);
	}else{
		//Generate random pin code
		rpin = wps_generate_pin();
		sprintf((char *)wpa_s->pin, "%s", (char *)&rpin);
	}

	wpa_printf(MSG_DEBUG, "WPS: wpas_wps_start_pin, pin code is %s", wpa_s->pin);

	wpa_s->wps_mode = WPS_MODE_PIN;

	ret = wpas_init_wps(wpa_s);
	if(ret < 0){
		wpa_printf(MSG_ERROR, "WPS: wpas_wps_start_pin, wpas init wps failed.");
		goto __error__;
	}

	atbmwifi_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);
	atbmwifi_eloop_register_timeout(WPS_PBC_WALK_TIME, 0, wpas_wps_timeout, wpa_s, NULL);
	
//	wpa_supplicant_set_state(wpa_s, ATBM_WPA_SCANNING);
	wpa_s->wpa_state = ATBM_WPA_SCANNING;
	wpa_comm_init_extra_ie(priv);

	atbmwifi_sta_scan(priv);
	if(priv->extra_ie){
		atbm_kfree(priv->extra_ie);
		priv->extra_ie =NULL;
		priv->extra_ie_len = 0;
	}

	return ret;

__error__:
	if(wpa_s->pin != NULL)
		atbm_kfree(wpa_s->pin);
	
	if(priv->extra_ie != NULL){
		atbm_kfree(priv->extra_ie);
		priv->extra_ie =NULL;
		priv->extra_ie_len = 0;
	}

	return ret;
}

 int wpas_wps_p2p_init(struct wpa_supplicant *wpa_s)
{
	int ret = 0;
	
	wpas_clear_wps(wpa_s);
	wpa_s->wps_mode = WPS_MODE_PBC;
	ret = wpas_init_wps(wpa_s);
	if(ret < 0){
		wpa_printf(MSG_ERROR, "WPS: init wps failed.");
		return -1;
	}
	return 0;
}

 int wpas_wps_p2p_start_pbc(struct wpa_supplicant *wpa_s, const atbm_uint8 *bssid, int p2p_group)
{
	struct atbmwifi_vif *priv = wpa_s->priv;

	atbmwifi_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);
	atbmwifi_eloop_register_timeout(WPS_PBC_WALK_TIME, 0, wpas_wps_timeout, wpa_s, NULL);

//	wpa_supplicant_set_state(wpa_s, ATBM_WPA_SCANNING);
	wpa_s->wpa_state = ATBM_WPA_SCANNING;
	wpa_s->wps_mode = WPS_MODE_PBC;
	wpa_comm_init_extra_ie(priv);
	priv->scan_expire = 1;

	atbmwifi_sta_scan(priv);
	if(priv->extra_ie){
		atbm_kfree(priv->extra_ie);
		priv->extra_ie =NULL;
		priv->extra_ie_len = 0;
	}	
	return 0;
}


