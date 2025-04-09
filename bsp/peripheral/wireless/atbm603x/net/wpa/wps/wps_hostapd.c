/*
 * hostapd / WPS integration
 * Copyright (c) 2008-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"
#include "wpa_debug.h"

#if CONFIG_WPS_UPNP
#include "wps/wps_upnp.h"
static int hostapd_wps_upnp_init(struct hostapd_data *hapd,
				 struct wps_context *wps);
static atbm_void hostapd_wps_upnp_deinit(struct hostapd_data *hapd);
#endif /* CONFIG_WPS_UPNP */


static atbm_void hostapd_wps_ap_pin_timeout(void *eloop_data, atbm_void *user_ctx);

extern  struct wps_registrar *
wps_registrar_init(struct wps_context *wps,struct wps_registrar_config *cfg);
extern int atbmwifi_hexstr2bin(atbm_uint8 *buf, const char *hex, atbm_size_t len);
extern int uuid_str2bin(const char *str, atbm_uint8 *bin);
extern int  atbmwifi_ap_start_beacon(struct atbmwifi_vif *priv);
extern int wps_registrar_add_pin(struct wps_registrar *reg,const atbm_uint8 *addr);
/* static atbm_void hostapd_wps_event_cb(void *ctx, enum wps_event event,
				 union wps_event_data *data)
{
//	struct hostapd_data *hapd = ctx;

	switch (event) {
	case WPS_EV_M2D:
		//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_M2D);
		break;
	case WPS_EV_FAIL:
		//hostapd_wps_event_fail(hapd, &data->fail);
		break;
	case WPS_EV_SUCCESS:
		//hostapd_wps_event_success(hapd, &data->success);
		//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_SUCCESS);
		break;
	case WPS_EV_PWD_AUTH_FAIL:
		//hostapd_pwd_auth_fail(hapd, &data->pwd_auth_fail);
		break;
	case WPS_EV_PBC_OVERLAP:
		//hostapd_wps_event_pbc_overlap(hapd);
		//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_OVERLAP);
		break;
	case WPS_EV_PBC_TIMEOUT:
		//hostapd_wps_event_pbc_timeout(hapd);
		//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_TIMEOUT);
		break;
	case WPS_EV_PBC_ACTIVE:
		//hostapd_wps_event_pbc_active(hapd);
		//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_ACTIVE);
		break;
	case WPS_EV_PBC_DISABLE:
		//hostapd_wps_event_pbc_disable(hapd);
		//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_DISABLE);
		break;
	case WPS_EV_ER_AP_ADD:
		break;
	case WPS_EV_ER_AP_REMOVE:
		break;
	case WPS_EV_ER_ENROLLEE_ADD:
		break;
	case WPS_EV_ER_ENROLLEE_REMOVE:
		break;
	case WPS_EV_ER_AP_SETTINGS:
		break;
	case WPS_EV_ER_SET_SELECTED_REGISTRAR:
		break;
	case WPS_EV_AP_PIN_SUCCESS:
		//hostapd_wps_ap_pin_success(hapd);
		break;
	}
}*/


 static int hostapd_wps_rf_band_cb(void *ctx)
{
	return WPS_RF_24GHZ; /* FIX: dualband AP */
}

 atbm_void hostapd_wps_timeout(void *eloop_ctx, atbm_void *timeout_ctx)
{
//	struct hostapd_data *hapd;

//	hapd = (struct hostapd_data *)eloop_ctx;

	return;
}

 static atbm_void hostapd_wps_clear_ies(struct hostapd_data *hapd, int deinit_only)
{
	wpabuf_free(hapd->wps_beacon_ie);
	hapd->wps_beacon_ie = NULL;
	hapd->priv->wps_beacon_ie = NULL;
	hapd->priv->wps_beacon_ie_len = 0;
	wpabuf_free(hapd->wps_probe_resp_ie);
	hapd->wps_probe_resp_ie = NULL;
	hapd->priv->wps_probe_resp_ie = NULL;
	hapd->priv->wps_probe_resp_ie_len = 0;
	if (deinit_only)
		return;

	atbmwifi_ap_start_beacon(hapd->priv);
}





 static atbm_void hostapd_free_wps(struct wps_context *wps)
{
	int i;

	for (i = 0; i < MAX_WPS_VENDOR_EXTENSIONS; i++)
		wpabuf_free(wps->dev.vendor_ext[i]);
	//wps_device_data_free(&wps->dev);
	wps->network_key = NULL;
	atbm_kfree(wps);
}

atbm_void wps_success_cb(atbm_void *ctx, const atbm_uint8 *mac_addr,
				const atbm_uint8 *uuid_e, const atbm_uint8 * dev_pw,
				atbm_size_t dev_pw_len)
{
	struct hostapd_data *hapd = ctx;
	wifi_printk(WIFI_ALWAYS, "wps_success_cb %p\n", hapd->wps);
	if(hapd->wps){
		hapd->wps->wpa_success_deauth = 1;
	}
}

/**
 * hostapd_init_wps - init wps AP structure
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at program initialization to enable wps AP.
 */
 int hostapd_init_wps(struct atbmwifi_vif *priv)
{
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct hostapd_data *hapd = (struct hostapd_data *)priv->appdata;
	struct wps_context *wps = 0;
	struct wps_registrar_config cfg;
	atbm_uint8 os_version[4];


	wifi_printk(WIFI_DBG_ERROR, "hostapd_init_wps\n");
	
	hapd->priv->pbc = 0;
	hapd->priv->pin = 0;
	hapd->wps_tx_hdr = ATBM_NULL;

	wps = (struct wps_context *)atbm_kzalloc(sizeof(*wps), GFP_KERNEL);
	if (wps == NULL)
	{
		wpa_printf(MSG_ERROR, "hostapd_init_wps: invalid wps\n");
		return -1;
	}

	wps->rf_band_cb = hostapd_wps_rf_band_cb;
	wps->cb_ctx = hapd;
	wps->wps_state = WPS_STATE_CONFIGURED;
	wps->ap_setup_locked = 0;
	if (uuid_str2bin("12345678-9abc-def0-1234-56789abcdef0", wps->uuid)) {
		wpa_printf(MSG_ERROR, "hostapd_init_wps: invalid UUID\n");	
		goto fail;
	}
	//wpa_hexdump(MSG_DEBUG, "WPS: Use configured UUID",
	//		    wps->uuid, UUID_LEN);
	wps->ssid_len = hapd->priv->config.ssid_len;
	atbm_memcpy(wps->ssid, hapd->priv->config.ssid, wps->ssid_len);
	wpa_printf(MSG_DEBUG, "wps: ssid %s", wps->ssid);
	wps->ap = 1;
	
#if CONFIG_P2P
	if(hapd->priv->p2p_ap){
		atbm_p2p_dev_info_set(hapd->priv, wps);
	}else
#endif
	{
		atbm_memcpy(wps->dev.mac_addr, hapd->own_addr, ATBM_ETH_ALEN);
		wps->dev.device_name = "iot_AP";
		wps->dev.manufacturer = "altobeam";
		wps->dev.model_name = "IOT";
		wps->dev.model_number = "123";
		wps->dev.serial_number = "12345";
	}

	wps->config_methods = wps_config_methods_str2bin("virtual_display virtual_push_button keypad");

	if (wps_dev_type_str2bin("6-0050F204-1", wps->dev.pri_dev_type))
	{
		wpa_printf(MSG_ERROR, "hostapd_init_wps: invalid device_type\n");	
		goto fail;
	}
	if (atbmwifi_hexstr2bin(os_version, "01020300", 4)) {
		wpa_printf(MSG_ERROR, "hostapd_init_wps: invalid os_version\n");
		goto fail;
	}
	wps->dev.os_version = ATBM_WPA_GET_BE32(os_version);

	wps->dev.rf_bands = WPS_RF_24GHZ; /* FIX: dualband AP */

	if (config->wpa & ATBM_WPA_PROTO_RSN) {
		if (config->key_mgmt & ATBM_WPA_KEY_MGMT_PSK)
			wps->auth_types |= WPS_AUTH_WPA2PSK;

		if (config->pairwise_cipher & (ATBM_WPA_CIPHER_CCMP | ATBM_WPA_CIPHER_GCMP))
			wps->encr_types |= WPS_ENCR_AES;
		if (config->pairwise_cipher & ATBM_WPA_CIPHER_TKIP)
			wps->encr_types |= WPS_ENCR_TKIP;
	}

	if (config->wpa & ATBM_WPA_PROTO_WPA) {
		if (config->key_mgmt & ATBM_WPA_KEY_MGMT_PSK)
			wps->auth_types |= WPS_AUTH_WPAPSK;

		if (config->pairwise_cipher & ATBM_WPA_CIPHER_CCMP)
			wps->encr_types |= WPS_ENCR_AES;
		if (config->pairwise_cipher & ATBM_WPA_CIPHER_TKIP)
			wps->encr_types |= WPS_ENCR_TKIP;
	}
	wpa_printf(MSG_DEBUG, "wps: auth_types 0x%x,encr_types 0x%x", wps->auth_types,wps->encr_types);

	if (config->password_len) {
		wps->network_key = config->password;
		wps->network_key_len = config->password_len;
		
		wpa_printf(MSG_DEBUG, "wps: network_key %s", wps->network_key);
	} 
	wps->ap_auth_type = wps->auth_types;
	wps->ap_encr_type = wps->encr_types;
	
	atbm_memset(&cfg, 0, sizeof(cfg));
	cfg.cb_ctx = hapd;
	cfg.skip_cred_build = 0;
	cfg.disable_auto_conf = 0;
	cfg.reg_success_cb = wps_success_cb;
	//cfg.dualband = 0;
	//cfg.force_per_enrollee_psk = 0;

	wps->registrar = wps_registrar_init(wps, &cfg);
	if (wps->registrar == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize WPS Registrar");
		goto fail;
	}
	wifi_printk(WIFI_DBG_ERROR, "wps init end\n");

	hapd->wps = wps;

	return 0;

fail:
	hostapd_free_wps(wps);
	return -1;
}


 int hostapd_init_wps_complete(struct hostapd_data *hapd)
{
	struct wps_context *wps = hapd->wps;

	if (wps == NULL)
		return 0;

#if CONFIG_WPS_UPNP
	if (hostapd_wps_upnp_init(hapd, wps) < 0) {
		wpa_printf(MSG_ERROR, "Failed to initialize WPS UPnP");
		wps_registrar_deinit(wps->registrar);
		hostapd_free_wps(wps);
		hapd->wps = NULL;
		return -1;
	}
#endif /* CONFIG_WPS_UPNP */

	return 0;
}

/**
 * hostapd_deinit_wps - destrory wps AP structure
 * Returns: NULL
 *
 * This function is called at program over run to destroy wps AP.
 */
 atbm_void hostapd_deinit_wps(struct hostapd_data *hapd)
{
	//atbmwifi_eloop_cancel_timeout(hostapd_wps_reenable_ap_pin, hapd, NULL);
	atbmwifi_eloop_cancel_timeout(hostapd_wps_ap_pin_timeout, hapd, NULL);
	//atbmwifi_eloop_cancel_timeout(wps_reload_config, hapd->iface, NULL);
	if (hapd->wps == NULL) {
		hostapd_wps_clear_ies(hapd, 1);
		return;
	}
#if CONFIG_WPS_UPNP
	hostapd_wps_upnp_deinit(hapd);
#endif /* CONFIG_WPS_UPNP */
	wps_registrar_deinit(hapd->wps->registrar);
	//wps_free_pending_msgs(hapd->wps->upnp_msgs);
	hostapd_free_wps(hapd->wps);
	if(hapd->wps_tx_hdr){
		atbm_kfree(hapd->wps_tx_hdr);
		hapd->wps_tx_hdr = ATBM_NULL;
	}
	hapd->wps = NULL;
	hostapd_wps_clear_ies(hapd, 1);
}

 int wps_add_pin(struct hostapd_data *hapd, atbm_void *ctx)
{
	int ret = 0;

	if (hapd->wps == NULL)
		return 0;
	ret = wps_registrar_add_pin(hapd->wps->registrar, 0);
	return ret;
}

 int hostapd_wps_button_pushed(struct hostapd_data *hapd, atbm_void *ctx)
{
	const atbm_uint8 *p2p_dev_addr = ctx;
	if (hapd->wps == NULL)
		return -1;
	return wps_registrar_button_pushed(hapd->wps->registrar, p2p_dev_addr);
}

 int hostapd_cancel_wps(struct hostapd_data *hapd)
{
	if (hapd->wps == NULL)
		return -1;

	wps_registrar_wps_cancel(hapd->wps->registrar);
	//ap_for_each_sta(hapd, ap_sta_wps_cancel, NULL);

	return 0;
}

 atbm_void hostapd_wps_ap_pin_disable(struct hostapd_data *hapd)
{
	wpa_printf(MSG_DEBUG, "WPS: Disabling AP PIN");
}

 static atbm_void hostapd_wps_ap_pin_timeout(void *eloop_data, atbm_void *user_ctx)
{
	struct hostapd_data *hapd = eloop_data;
	wpa_printf(MSG_DEBUG, "WPS: AP PIN timed out");
	hostapd_wps_ap_pin_disable(hapd);
	//wpa_msg(hapd->msg_ctx, MSG_INFO, WPS_EVENT_AP_PIN_DISABLED);
}


/* static atbm_void hostapd_wps_ap_pin_enable(struct hostapd_data *hapd, int timeout)
{
}*/


/* static int wps_ap_pin_disable(struct hostapd_data *hapd, atbm_void *ctx)
{
	return 0;
}*/




 int hostapd_wps_config_ap(struct hostapd_data *hapd, char *ssid,
			  const char *auth, const char *encr, char *key)
{
	struct wps_credential cred;
	atbm_size_t len;

	atbm_memset(&cred, 0, sizeof(cred));

	len = strlen(ssid);
	if ((len & 1) || len > 2 * sizeof(cred.ssid) ||
	    atbmwifi_hexstr2bin((atbm_uint8 *)ssid, (const char*)cred.ssid, len / 2))
		return -1;
	cred.ssid_len = len / 2;

	if (strncmp(auth, "OPEN", 4) == 0)
		cred.auth_type = WPS_AUTH_OPEN;
	else if (strncmp(auth, "WPAPSK", 6) == 0)
		cred.auth_type = WPS_AUTH_WPAPSK;
	else if (strncmp(auth, "WPA2PSK", 7) == 0)
		cred.auth_type = WPS_AUTH_WPA2PSK;
	else
		return -1;

	if (encr) {
		if (strncmp(encr, "NONE", 4) == 0)
			cred.encr_type = WPS_ENCR_NONE;
		else if (strncmp(encr, "TKIP", 4) == 0)
			cred.encr_type = WPS_ENCR_TKIP;
		else if (strncmp(encr, "CCMP", 4) == 0)
			cred.encr_type = WPS_ENCR_AES;
		else
			return -1;
	} else
		cred.encr_type = WPS_ENCR_NONE;

	if (key) {
		len = strlen(key);
		if ((len & 1) || len > 2 * sizeof(cred.key) ||
		    atbmwifi_hexstr2bin((atbm_uint8 *)key, (const char*)cred.key, len / 2))
			return -1;
		cred.key_len = len / 2;
	}

	return wps_registrar_config_ap(hapd->wps->registrar, &cred);
}


