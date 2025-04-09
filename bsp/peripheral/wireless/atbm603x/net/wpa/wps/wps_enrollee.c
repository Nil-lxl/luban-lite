/**************************************************************************************************************
* altobeam IOT Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "wpa_debug.h"

#define NO_WORKAROUND_M8  //altobeam@20181019, pre-handle M8 msg.

//extern atbm_uint32 hal_get_prio_thread(hal_thread_t *th);
//extern int hal_set_prio_thread(hal_thread_t *th, atbm_uint32 val);
//extern atbm_uint32 atbm_GetOsTime(void);
extern int wps_build_device_attrs(struct wps_device_data *dev, struct wpabuf *msg);
extern int wps_build_rf_bands(struct wps_device_data *dev, struct wpabuf *msg);
extern int wps_build_os_version(struct wps_device_data *dev, struct wpabuf *msg);
extern int wps_build_vendor_ext_m1(struct wps_device_data *dev, struct wpabuf *msg);
extern int wps_process_device_attrs(struct wps_device_data *dev,
			     struct wps_parse_attr *attr);
 static int wps_build_mac_addr(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * MAC Address");
	wpabuf_put_be16(msg, ATTR_MAC_ADDR);
	wpabuf_put_be16(msg, ATBM_ETH_ALEN);
	wpabuf_put_data(msg, wps->mac_addr_e, ATBM_ETH_ALEN);
	return 0;
}


 static int wps_build_wps_state(struct wps_data *wps, struct wpabuf *msg)
{
	atbm_uint8 state;
	if (wps->wps->ap)
		state = wps->wps->wps_state;
	else
		state = WPS_STATE_NOT_CONFIGURED;
	wpa_printf(MSG_DEBUG, "WPS:  * Wi-Fi Protected Setup State (%d)",
		   state);
	wpabuf_put_be16(msg, ATTR_WPS_STATE);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, state);
	return 0;
}


 static int wps_build_e_hash(struct wps_data *wps, struct wpabuf *msg)
{
	atbm_uint8 *hash;
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];

	if (random_get_bytes(wps->snonce, 2 * WPS_SECRET_NONCE_LEN) < 0)
		return -1;
	wpa_hexdump(MSG_DEBUG, "WPS: E-S1", wps->snonce, WPS_SECRET_NONCE_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: E-S2",
		    wps->snonce + WPS_SECRET_NONCE_LEN, WPS_SECRET_NONCE_LEN);

	if (wps->dh_pubkey_e == NULL || wps->dh_pubkey_r == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: DH public keys not available for "
			   "E-Hash derivation");
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPS:  * E-Hash1");
	wpabuf_put_be16(msg, ATTR_E_HASH1);
	wpabuf_put_be16(msg, SHA256_MAC_LEN);
	hash = wpabuf_put(msg, SHA256_MAC_LEN);
	/* E-Hash1 = HMAC_AuthKey(E-S1 || PSK1 || PK_E || PK_R) */
	addr[0] = wps->snonce;
	len[0] = WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk1;
	len[1] = WPS_PSK_LEN;
	addr[2] = wpabuf_head(wps->dh_pubkey_e);
	len[2] = wpabuf_len(wps->dh_pubkey_e);
	addr[3] = wpabuf_head(wps->dh_pubkey_r);
	len[3] = wpabuf_len(wps->dh_pubkey_r);
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);
	wpa_hexdump(MSG_DEBUG, "WPS: E-Hash1", hash, SHA256_MAC_LEN);

	wpa_printf(MSG_DEBUG, "WPS:  * E-Hash2");
	wpabuf_put_be16(msg, ATTR_E_HASH2);
	wpabuf_put_be16(msg, SHA256_MAC_LEN);
	hash = wpabuf_put(msg, SHA256_MAC_LEN);
	/* E-Hash2 = HMAC_AuthKey(E-S2 || PSK2 || PK_E || PK_R) */
	addr[0] = wps->snonce + WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk2;
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);
	wpa_hexdump(MSG_DEBUG, "WPS: E-Hash2", hash, SHA256_MAC_LEN);

	return 0;
}


 static int wps_build_e_snonce1(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * E-SNonce1");
	wpabuf_put_be16(msg, ATTR_E_SNONCE1);
	wpabuf_put_be16(msg, WPS_SECRET_NONCE_LEN);
	wpabuf_put_data(msg, wps->snonce, WPS_SECRET_NONCE_LEN);
	return 0;
}


 static int wps_build_e_snonce2(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * E-SNonce2");
	wpabuf_put_be16(msg, ATTR_E_SNONCE2);
	wpabuf_put_be16(msg, WPS_SECRET_NONCE_LEN);
	wpabuf_put_data(msg, wps->snonce + WPS_SECRET_NONCE_LEN,
			WPS_SECRET_NONCE_LEN);
	return 0;
}

 static struct wpabuf * wps_build_m1(struct wps_data *wps)
{
	struct wpabuf *msg;
	atbm_uint16 config_methods;
	
	
	//d_prio = hal_get_prio_thread(NULL);
	//l_set_prio_thread(NULL, 20);
	if (random_get_bytes(wps->nonce_e, WPS_NONCE_LEN) < 0)
		return NULL;
	wpa_hexdump(MSG_DEBUG, "WPS: Enrollee Nonce",
		    wps->nonce_e, WPS_NONCE_LEN);

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M1---%dms", atbm_GetOsTime());


	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	config_methods = wps->wps->config_methods;
	if (wps->wps->ap /*&& !wps->pbc_in_m1*/ &&
	    (wps->dev_password_len != 0 ||
	     (config_methods & WPS_CONFIG_DISPLAY))) {
		/*
		 * These are the methods that the AP supports as an Enrollee
		 * for adding external Registrars, so remove PushButton.
		 *
		 * As a workaround for Windows 7 mechanism for probing WPS
		 * capabilities from M1, leave PushButton option if no PIN
		 * method is available or if WPS configuration enables PBC
		 * workaround.
		 */
		config_methods &= ~WPS_CONFIG_PUSHBUTTON;
#if CONFIG_WPS2
		config_methods &= ~(WPS_CONFIG_VIRT_PUSHBUTTON |
				    WPS_CONFIG_PHY_PUSHBUTTON);
#endif /* CONFIG_WPS2 */
	}

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M1) ||
	    wps_build_uuid_e(msg, wps->uuid_e) ||
	    wps_build_mac_addr(wps, msg) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_public_key(wps, msg) ||
	    wps_build_auth_type_flags(wps, msg) ||
	    wps_build_encr_type_flags(wps, msg) ||
	    wps_build_conn_type_flags(wps, msg) ||
	    wps_build_config_methods(msg, config_methods) ||
	    wps_build_wps_state(wps, msg) ||
	    wps_build_device_attrs(&wps->wps->dev, msg) ||
	    wps_build_rf_bands(&wps->wps->dev, msg) ||
	    wps_build_assoc_state(wps, msg) ||
	    wps_build_dev_password_id(msg, wps->dev_pw_id) ||
	    wps_build_config_error(msg, WPS_CFG_NO_ERROR) ||
	    wps_build_os_version(&wps->wps->dev, msg) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_vendor_ext_m1(&wps->wps->dev, msg)) {
		wpabuf_free(msg);
		return NULL;
	}

	wps->state = RECV_M2;
	//l_set_prio_thread(NULL, old_prio);

	return msg;
}


 static struct wpabuf * wps_build_m3(struct wps_data *wps)
{
	struct wpabuf *msg;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M3---%dms", atbm_GetOsTime());

	if (wps->dev_password == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No Device Password available");
		return NULL;
	}
	wps_derive_psk(wps, wps->dev_password, wps->dev_password_len);

	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M3) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_e_hash(wps, msg) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
		wpabuf_free(msg);
		return NULL;
	}

	wps->state = RECV_M4;
	return msg;
}


 static struct wpabuf * wps_build_m5(struct wps_data *wps)
{
	struct wpabuf *msg, *plain;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M5---%dms", atbm_GetOsTime());

	plain = wpabuf_alloc(200);
	if (plain == NULL)
		return NULL;

	msg = wpabuf_alloc(1000);
	if (msg == NULL) {
		wpabuf_free(plain);
		return NULL;
	}

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M5) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_e_snonce1(wps, plain) ||
	    wps_build_key_wrap_auth(wps, plain) ||
	    wps_build_encr_settings(wps, msg, plain) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
		wpabuf_free(plain);
		wpabuf_free(msg);
		return NULL;
	}
	wpabuf_free(plain);

	wps->state = RECV_M6;
	return msg;
}


 static int wps_build_cred_ssid(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * SSID");
	wpabuf_put_be16(msg, ATTR_SSID);
	wpabuf_put_be16(msg, wps->wps->ssid_len);
	wpabuf_put_data(msg, wps->wps->ssid, wps->wps->ssid_len);
	return 0;
}


 static int wps_build_cred_auth_type(struct wps_data *wps, struct wpabuf *msg)
{
	atbm_uint16 auth_type = wps->wps->auth_types;

	/* Select the best authentication type */
	if (auth_type & WPS_AUTH_WPA2PSK)
		auth_type = WPS_AUTH_WPA2PSK;
	else if (auth_type & WPS_AUTH_WPAPSK)
		auth_type = WPS_AUTH_WPAPSK;
	else if (auth_type & WPS_AUTH_OPEN)
		auth_type = WPS_AUTH_OPEN;
	else if (auth_type & WPS_AUTH_SHARED)
		auth_type = WPS_AUTH_SHARED;

	wpa_printf(MSG_DEBUG, "WPS:  * Authentication Type (0x%x)", auth_type);
	wpabuf_put_be16(msg, ATTR_AUTH_TYPE);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, auth_type);
	return 0;
}


 static int wps_build_cred_encr_type(struct wps_data *wps, struct wpabuf *msg)
{
	atbm_uint16 encr_type = wps->wps->encr_types;

	/* Select the best encryption type */
	if (wps->wps->auth_types & (WPS_AUTH_WPA2PSK | WPS_AUTH_WPAPSK)) {
		if (encr_type & WPS_ENCR_AES)
			encr_type = WPS_ENCR_AES;
		else if (encr_type & WPS_ENCR_TKIP)
			encr_type = WPS_ENCR_TKIP;
	} else {
		if (encr_type & WPS_ENCR_WEP)
			encr_type = WPS_ENCR_WEP;
		else if (encr_type & WPS_ENCR_NONE)
			encr_type = WPS_ENCR_NONE;
	}

	wpa_printf(MSG_DEBUG, "WPS:  * Encryption Type (0x%x)", encr_type);
	wpabuf_put_be16(msg, ATTR_ENCR_TYPE);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, encr_type);
	return 0;
}


 static int wps_build_cred_network_key(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * Network Key");
	wpabuf_put_be16(msg, ATTR_NETWORK_KEY);
	wpabuf_put_be16(msg, wps->wps->network_key_len);
	wpabuf_put_data(msg, wps->wps->network_key, wps->wps->network_key_len);
	return 0;
}


 static int wps_build_cred_mac_addr(struct wps_data *wps, struct wpabuf *msg)
{
	wpa_printf(MSG_DEBUG, "WPS:  * MAC Address (AP BSSID)");
	wpabuf_put_be16(msg, ATTR_MAC_ADDR);
	wpabuf_put_be16(msg, ATBM_ETH_ALEN);
	wpabuf_put_data(msg, wps->wps->dev.mac_addr, ATBM_ETH_ALEN);
	return 0;
}


 static int wps_build_ap_settings(struct wps_data *wps, struct wpabuf *plain)
{
	if (wps->wps->ap_settings) {
		wpa_printf(MSG_DEBUG, "WPS:  * AP Settings (pre-configured)");
		wpabuf_put_data(plain, wps->wps->ap_settings,
				wps->wps->ap_settings_len);
		return 0;
	}

	return wps_build_cred_ssid(wps, plain) ||
		wps_build_cred_mac_addr(wps, plain) ||
		wps_build_cred_auth_type(wps, plain) ||
		wps_build_cred_encr_type(wps, plain) ||
		wps_build_cred_network_key(wps, plain);
}


 static struct wpabuf * wps_build_m7(struct wps_data *wps)
{
	struct wpabuf *msg, *plain;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message M7---%dms", atbm_GetOsTime());

	plain = wpabuf_alloc(500 + wps->wps->ap_settings_len);
	if (plain == NULL)
		return NULL;

	msg = wpabuf_alloc(1000 + wps->wps->ap_settings_len);
	if (msg == NULL) {
		wpabuf_free(plain);
		return NULL;
	}

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_M7) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_e_snonce2(wps, plain) ||
	    (wps->wps->ap && wps_build_ap_settings(wps, plain)) ||
	    wps_build_key_wrap_auth(wps, plain) ||
	    wps_build_encr_settings(wps, msg, plain) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0) ||
	    wps_build_authenticator(wps, msg)) {
		wpabuf_free(plain);
		wpabuf_free(msg);
		return NULL;
	}
	wpabuf_free(plain);

	if (wps->wps->ap && wps->wps->registrar) {
		/*
		 * If the Registrar is only learning our current configuration,
		 * it may not continue protocol run to successful completion.
		 * Store information here to make sure it remains available.
		 */
		//wps_device_store(wps->wps->registrar, &wps->peer_dev,
		//		 wps->uuid_r);
		wpa_printf(MSG_ERROR, "WPS: Building Message M7--error handling...\n");
	}

	wps->state = RECV_M8;
	return msg;
}


 static struct wpabuf * wps_build_wsc_done(struct wps_data *wps)
{
	struct wpabuf *msg;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message WSC_Done---%dms", atbm_GetOsTime());

	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_WSC_DONE) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0)) {
		wpabuf_free(msg);
		return NULL;
	}

	if (wps->wps->ap)
		wps->state = RECV_ACK;
	else {
		wps_success_event(wps->wps);
		wps->state = WPS_FINISHED;
	}
	return msg;
}


 struct wpabuf * wps_enrollee_get_msg(struct wps_data *wps,
				     enum wsc_op_code *op_code)
{
	struct wpabuf *msg;

	switch (wps->state) {
	case SEND_M1:
		msg = wps_build_m1(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M3:
		msg = wps_build_m3(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M5:
		msg = wps_build_m5(wps);
		*op_code = WSC_MSG;
		break;
	case SEND_M7:
		msg = wps_build_m7(wps);
		*op_code = WSC_MSG;
		break;
	case RECEIVED_M2D:
		if (wps->wps->ap) {
			msg = wps_build_wsc_nack(wps);
			*op_code = WSC_NACK;
			break;
		}
		msg = wps_build_wsc_ack(wps);
		*op_code = WSC_ACK;
		if (msg) {
			/* Another M2/M2D may be received */
			wps->state = RECV_M2;
		}
		break;
	case SEND_WSC_NACK:
		msg = wps_build_wsc_nack(wps);
		*op_code = WSC_NACK;
		break;
	case WPS_MSG_DONE:
		msg = wps_build_wsc_done(wps);
		*op_code = WSC_Done;
		break;
	default:
		wpa_printf(MSG_DEBUG, "WPS: Unsupported state %d for building "
			   "a message", wps->state);
		msg = NULL;
		break;
	}

	if (*op_code == WSC_MSG && msg) {
		/* Save a copy of the last message for Authenticator derivation
		 */
		wpabuf_free(wps->last_msg);
		wps->last_msg = wpabuf_dup(msg);
	}

	return msg;
}


 static int wps_process_registrar_nonce(struct wps_data *wps, const atbm_uint8 *r_nonce)
{
	if (r_nonce == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No Registrar Nonce received");
		return -1;
	}

	atbm_memcpy(wps->nonce_r, r_nonce, WPS_NONCE_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: Registrar Nonce",
		    wps->nonce_r, WPS_NONCE_LEN);

	return 0;
}


 static int wps_process_enrollee_nonce(struct wps_data *wps, const atbm_uint8 *e_nonce)
{
	if (e_nonce == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No Enrollee Nonce received");
		return -1;
	}

	if (atbm_memcmp(wps->nonce_e, e_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "WPS: Invalid Enrollee Nonce received");
		return -1;
	}

	return 0;
}


 static int wps_process_uuid_r(struct wps_data *wps, const atbm_uint8 *uuid_r)
{
	if (uuid_r == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No UUID-R received");
		return -1;
	}

	atbm_memcpy(wps->uuid_r, uuid_r, WPS_UUID_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: UUID-R", wps->uuid_r, WPS_UUID_LEN);

	return 0;
}


 static int wps_process_pubkey(struct wps_data *wps, const atbm_uint8 *pk,
			      atbm_size_t pk_len)
{
	if (pk == NULL || pk_len == 0) {
		wpa_printf(MSG_DEBUG, "WPS: No Public Key received");
		return -1;
	}

#ifdef CONFIG_WPS_OOB
	if (wps->dev_pw_id != DEV_PW_DEFAULT &&
	    wps->wps->oob_conf.pubkey_hash) {
		const atbm_uint8 *addr[1];
		atbm_uint8 hash[WPS_HASH_LEN];

		addr[0] = pk;
		atbmwifi_sha256_vector(1, addr, &pk_len, hash);
		if (atbm_memcmp(hash,
			      wpabuf_head(wps->wps->oob_conf.pubkey_hash),
			      WPS_OOB_PUBKEY_HASH_LEN) != 0) {
			wpa_printf(MSG_ERROR, "WPS: Public Key hash error");
			return -1;
		}
	}
#endif /* CONFIG_WPS_OOB */

	wpabuf_free(wps->dh_pubkey_r);
	wps->dh_pubkey_r = wpabuf_alloc_copy(pk, pk_len);
	if (wps->dh_pubkey_r == NULL)
		return -1;

	if (wps_derive_keys(wps) < 0)
		return -1;

	return 0;
}


 static int wps_process_r_hash1(struct wps_data *wps, const atbm_uint8 *r_hash1)
{
	if (r_hash1 == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No R-Hash1 received");
		return -1;
	}

	atbm_memcpy(wps->peer_hash1, r_hash1, WPS_HASH_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: R-Hash1", wps->peer_hash1, WPS_HASH_LEN);

	return 0;
}


 static int wps_process_r_hash2(struct wps_data *wps, const atbm_uint8 *r_hash2)
{
	if (r_hash2 == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No R-Hash2 received");
		return -1;
	}

	atbm_memcpy(wps->peer_hash2, r_hash2, WPS_HASH_LEN);
	wpa_hexdump(MSG_DEBUG, "WPS: R-Hash2", wps->peer_hash2, WPS_HASH_LEN);

	return 0;
}


 static int wps_process_r_snonce1(struct wps_data *wps, const atbm_uint8 *r_snonce1)
{
	atbm_uint8 hash[SHA256_MAC_LEN];
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];

	if (r_snonce1 == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No R-SNonce1 received");
		return -1;
	}

	//wpa_hexdump_key(MSG_DEBUG, "WPS: R-SNonce1", r_snonce1,
	//		WPS_SECRET_NONCE_LEN);

	/* R-Hash1 = HMAC_AuthKey(R-S1 || PSK1 || PK_E || PK_R) */
	addr[0] = r_snonce1;
	len[0] = WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk1;
	len[1] = WPS_PSK_LEN;
	addr[2] = wpabuf_head(wps->dh_pubkey_e);
	len[2] = wpabuf_len(wps->dh_pubkey_e);
	addr[3] = wpabuf_head(wps->dh_pubkey_r);
	len[3] = wpabuf_len(wps->dh_pubkey_r);
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);

	if (atbm_memcmp(wps->peer_hash1, hash, WPS_HASH_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "WPS: R-Hash1 derived from R-S1 does "
			   "not match with the pre-committed value");
		wps->config_error = WPS_CFG_DEV_PASSWORD_AUTH_FAILURE;
		wps_pwd_auth_fail_event(wps->wps, 1, 1);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPS: Registrar proved knowledge of the first "
		   "half of the device password");

	return 0;
}


 static int wps_process_r_snonce2(struct wps_data *wps, const atbm_uint8 *r_snonce2)
{
	atbm_uint8 hash[SHA256_MAC_LEN];
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];

	if (r_snonce2 == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: No R-SNonce2 received");
		return -1;
	}

	//wpa_hexdump_key(MSG_DEBUG, "WPS: R-SNonce2", r_snonce2,
	//		WPS_SECRET_NONCE_LEN);

	/* R-Hash2 = HMAC_AuthKey(R-S2 || PSK2 || PK_E || PK_R) */
	addr[0] = r_snonce2;
	len[0] = WPS_SECRET_NONCE_LEN;
	addr[1] = wps->psk2;
	len[1] = WPS_PSK_LEN;
	addr[2] = wpabuf_head(wps->dh_pubkey_e);
	len[2] = wpabuf_len(wps->dh_pubkey_e);
	addr[3] = wpabuf_head(wps->dh_pubkey_r);
	len[3] = wpabuf_len(wps->dh_pubkey_r);
	atbmwifi_hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 4, addr, len, hash);

	if (atbm_memcmp(wps->peer_hash2, hash, WPS_HASH_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "WPS: R-Hash2 derived from R-S2 does "
			   "not match with the pre-committed value");
		wps->config_error = WPS_CFG_DEV_PASSWORD_AUTH_FAILURE;
		wps_pwd_auth_fail_event(wps->wps, 1, 2);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "WPS: Registrar proved knowledge of the second "
		   "half of the device password");

	return 0;
}


 static int wps_process_cred_e(struct wps_data *wps, const atbm_uint8 *cred,
			      atbm_size_t cred_len, int wps2)
{
	int ret = 0;
	struct wps_parse_attr *attr;
	struct wpabuf msg;

	wpa_printf(MSG_ALWAYS, "WPS: Received Credential");

	attr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received WSC_MSG, malloc failed");
		ret = -1;
		goto end_process;
	}

	atbm_memset(&wps->cred, 0, sizeof(wps->cred));
	wpabuf_set(&msg, cred, cred_len);
	if (wps_parse_msg(&msg, attr) < 0 ||
	    wps_process_cred(attr, &wps->cred)){
		ret = -1;
		goto end_process;
	}

	if (atbm_memcmp(wps->cred.mac_addr, wps->wps->dev.mac_addr, ATBM_ETH_ALEN) !=
	    0) {
		wpa_printf(MSG_DEBUG, "WPS: MAC Address in the Credential ("
			   MACSTR ") does not match with own address (" MACSTR
			   ")", MAC2STR(wps->cred.mac_addr),
			   MAC2STR(wps->wps->dev.mac_addr));
		/*
		 * In theory, this could be consider fatal error, but there are
		 * number of deployed implementations using other address here
		 * due to unclarity in the specification. For interoperability
		 * reasons, allow this to be processed since we do not really
		 * use the MAC Address information for anything.
		 */
#ifdef CONFIG_WPS_STRICT
		if (wps2) {
			wpa_printf(MSG_DEBUG, "WPS: Do not accept incorrect "
				   "MAC Address in AP Settings");
			ret = -1;
			goto end_process;
		}
#endif /* CONFIG_WPS_STRICT */
	}

#if CONFIG_WPS2
	if (!(wps->cred.encr_type &
	      (WPS_ENCR_NONE | WPS_ENCR_TKIP | WPS_ENCR_AES))) {
		if (wps->cred.encr_type & WPS_ENCR_WEP) {
			wpa_printf(MSG_ERROR, "WPS: Reject Credential "
				   "due to WEP configuration");
			wps->error_indication = WPS_EI_SECURITY_WEP_PROHIBITED;
			ret = -2;
			goto end_process;
		}

		wpa_printf(MSG_ERROR, "WPS: Reject Credential due to "
			   "invalid encr_type 0x%x", wps->cred.encr_type);
		ret = -1;
		goto end_process;
	}
#endif /* CONFIG_WPS2 */

	if (wps->wps->cred_cb) {
		wps->cred.cred_attr = cred - 4;
		wps->cred.cred_attr_len = cred_len + 4;
		wps->wps->cred_cb(wps->wps->cb_ctx, &wps->cred);
		wps->cred.cred_attr = NULL;
		wps->cred.cred_attr_len = 0;
	}
	
end_process:
	if(attr != NULL)
		atbm_kfree(attr);
	
	return ret;
}


 static int wps_process_creds(struct wps_data *wps, const atbm_uint8 *cred[],
			     atbm_size_t cred_len[], atbm_size_t num_cred, int wps2)
{
	atbm_size_t i;
	int ok = 0;

	if (wps->wps->ap)
		return 0;

	if (num_cred == 0) {
		wpa_printf(MSG_ERROR, "WPS: No Credential attributes "
			   "received");
		return -1;
	}

	for (i = 0; i < num_cred; i++) {
		int res;
		res = wps_process_cred_e(wps, cred[i], cred_len[i], wps2);
		if (res == 0)
			ok++;
		else if (res == -2)
			wpa_printf(MSG_ERROR, "WPS: WEP credential skipped");
		else{
			iot_printf("<WPSs>%s %d\n",__func__,__LINE__);
			return -1;
		}
	}

	if (ok == 0) {
		wpa_printf(MSG_ERROR, "WPS: No valid Credential attribute "
			   "received");
		return -1;
	}

	return 0;
}

#if 0
 static int wps_process_ap_settings_e(struct wps_data *wps,
				     struct wps_parse_attr *attr,
				     struct wpabuf *attrs, int wps2)
{
	struct wps_credential cred;

	if (!wps->wps->ap)
		return 0;

	if (wps_process_ap_settings(attr, &cred) < 0)
		return -1;

	wpa_printf(MSG_ALWAYS, "WPS: Received new AP configuration from "
		   "Registrar");

	if (atbm_memcmp(cred.mac_addr, wps->wps->dev.mac_addr, ATBM_ETH_ALEN) !=
	    0) {
		wpa_printf(MSG_ERROR, "WPS: MAC Address in the AP Settings ("
			   MACSTR ") does not match with own address (" MACSTR
			   ")", MAC2STR(cred.mac_addr),
			   MAC2STR(wps->wps->dev.mac_addr));
		/*
		 * In theory, this could be consider fatal error, but there are
		 * number of deployed implementations using other address here
		 * due to unclarity in the specification. For interoperability
		 * reasons, allow this to be processed since we do not really
		 * use the MAC Address information for anything.
		 */
#ifdef CONFIG_WPS_STRICT
		if (wps2) {
			wpa_printf(MSG_DEBUG, "WPS: Do not accept incorrect "
				   "MAC Address in AP Settings");
			return -1;
		}
#endif /* CONFIG_WPS_STRICT */
	}

#if CONFIG_WPS2
	if (!(cred.encr_type & (WPS_ENCR_NONE | WPS_ENCR_TKIP | WPS_ENCR_AES)))
	{
		if (cred.encr_type & WPS_ENCR_WEP) {
			wpa_printf(MSG_ERROR, "WPS: Reject new AP settings "
				   "due to WEP configuration");
			wps->error_indication = WPS_EI_SECURITY_WEP_PROHIBITED;
			return -1;
		}

		wpa_printf(MSG_ERROR, "WPS: Reject new AP settings due to "
			   "invalid encr_type 0x%x", cred.encr_type);
		return -1;
	}
#endif /* CONFIG_WPS2 */

#ifdef CONFIG_WPS_STRICT
	if (wps2) {
		if ((cred.encr_type & (WPS_ENCR_TKIP | WPS_ENCR_AES)) ==
		    WPS_ENCR_TKIP ||
		    (cred.auth_type & (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) ==
		    WPS_AUTH_WPAPSK) {
			wpa_printf(MSG_DEBUG, "WPS-STRICT: Invalid WSC 2.0 "
				   "AP Settings: WPA-Personal/TKIP only");
			wps->error_indication =
				WPS_EI_SECURITY_TKIP_ONLY_PROHIBITED;
			return -1;
		}
	}
#endif /* CONFIG_WPS_STRICT */

#if CONFIG_WPS2
	if ((cred.encr_type & (WPS_ENCR_TKIP | WPS_ENCR_AES)) == WPS_ENCR_TKIP)
	{
		wpa_printf(MSG_ERROR, "WPS: Upgrade encr_type TKIP -> "
			   "TKIP+AES");
		cred.encr_type |= WPS_ENCR_AES;
	}

	if ((cred.auth_type & (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) ==
	    WPS_AUTH_WPAPSK) {
		wpa_printf(MSG_ERROR, "WPS: Upgrade auth_type WPAPSK -> "
			   "WPAPSK+WPA2PSK");
		cred.auth_type |= WPS_AUTH_WPA2PSK;
	}
#endif /* CONFIG_WPS2 */

	if (wps->wps->cred_cb) {
		cred.cred_attr = wpabuf_head(attrs);
		cred.cred_attr_len = wpabuf_len(attrs);
		wps->wps->cred_cb(wps->wps->cb_ctx, &cred);
	}

	return 0;
}
#endif

 static enum wps_process_res wps_process_m2(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	wpa_printf(MSG_ALWAYS, "WPS: Received M2---%dms", atbm_GetOsTime());

	if (wps->state != RECV_M2) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M2", wps->state);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps_process_registrar_nonce(wps, attr->registrar_nonce) ||
	    wps_process_enrollee_nonce(wps, attr->enrollee_nonce) ||
	    wps_process_uuid_r(wps, attr->uuid_r)) {
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	/*
	 * Stop here on an AP as an Enrollee if AP Setup is locked unless the
	 * special locked mode is used to allow protocol run up to M7 in order
	 * to support external Registrars that only learn the current AP
	 * configuration without changing it.
	 */
	if (wps->wps->ap &&
	    ((wps->wps->ap_setup_locked && wps->wps->ap_setup_locked != 2) ||
	     wps->dev_password == NULL)) {
		wpa_printf(MSG_ERROR, "WPS: AP Setup is locked - refuse "
			   "registration of a new Registrar");
		wps->config_error = WPS_CFG_SETUP_LOCKED;
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	if (wps_process_pubkey(wps, attr->public_key, attr->public_key_len) ||
	    wps_process_authenticator(wps, attr->authenticator, msg) ||
	    wps_process_device_attrs(&wps->peer_dev, attr)) {
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	wps->state = SEND_M3;
	return WPS_CONTINUE;
}


 static enum wps_process_res wps_process_m2d(struct wps_data *wps,
					    struct wps_parse_attr *attr)
{
	wpa_printf(MSG_ALWAYS, "WPS: Received M2D---%dms", atbm_GetOsTime());

	if (wps->state != RECV_M2) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M2D", wps->state);
		wps->state = SEND_WSC_NACK;
		return WPS_CONTINUE;
	}

	wpa_printf(MSG_DEBUG, "WPS: Manufacturer %s %d\n",
			  attr->manufacturer, attr->manufacturer_len);
	wpa_printf(MSG_DEBUG, "WPS: Model Name %s %d\n",
			  attr->model_name, attr->model_name_len);
	wpa_printf(MSG_DEBUG, "WPS: Model Number %s %d\n",
			  attr->model_number, attr->model_number_len);
	wpa_printf(MSG_DEBUG, "WPS: Serial Number %s %d\n",
			  attr->serial_number, attr->serial_number_len);
	wpa_printf(MSG_DEBUG, "WPS: Device Name %s %d\n",
			  attr->dev_name, attr->dev_name_len);

	if (wps->wps->event_cb) {
		union wps_event_data data;
		struct wps_event_m2d *m2d = &data.m2d;
		atbm_memset(&data, 0, sizeof(data));
		if (attr->config_methods)
			m2d->config_methods =
				ATBM_WPA_GET_BE16(attr->config_methods);
		m2d->manufacturer = attr->manufacturer;
		m2d->manufacturer_len = attr->manufacturer_len;
		m2d->model_name = attr->model_name;
		m2d->model_name_len = attr->model_name_len;
		m2d->model_number = attr->model_number;
		m2d->model_number_len = attr->model_number_len;
		m2d->serial_number = attr->serial_number;
		m2d->serial_number_len = attr->serial_number_len;
		m2d->dev_name = attr->dev_name;
		m2d->dev_name_len = attr->dev_name_len;
		m2d->primary_dev_type = attr->primary_dev_type;
		if (attr->config_error)
			m2d->config_error =
				ATBM_WPA_GET_BE16(attr->config_error);
		if (attr->dev_password_id)
			m2d->dev_password_id =
				ATBM_WPA_GET_BE16(attr->dev_password_id);
		wps->wps->event_cb(wps->wps->cb_ctx, WPS_EV_M2D, &data);
	}

	wps->state = RECEIVED_M2D;
	return WPS_CONTINUE;
}


 static enum wps_process_res wps_process_m4(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	enum wps_process_res ret = WPS_CONTINUE;
	struct wpabuf *decrypted;
	struct wps_parse_attr *eattr;

	wpa_printf(MSG_ALWAYS, "WPS: Received M4---%dms", atbm_GetOsTime());

	eattr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*eattr), GFP_KERNEL);
	if(eattr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received M4, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps->state != RECV_M4) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M4", wps->state);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps_process_enrollee_nonce(wps, attr->enrollee_nonce) ||
	    wps_process_authenticator(wps, attr->authenticator, msg) ||
	    wps_process_r_hash1(wps, attr->r_hash1) ||
	    wps_process_r_hash2(wps, attr->r_hash2)) {
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	decrypted = wps_decrypt_encr_settings(wps, attr->encr_settings,
					      attr->encr_settings_len);
	if (decrypted == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to decrypted Encrypted "
			   "Settings attribute");
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps_validate_m4_encr(decrypted, attr->version2 != NULL) < 0) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	wpa_printf(MSG_DEBUG, "WPS: Processing decrypted Encrypted Settings "
		   "attribute");
	if (wps_parse_msg(decrypted, eattr) < 0 ||
	    wps_process_key_wrap_auth(wps, decrypted, eattr->key_wrap_auth) ||
	    wps_process_r_snonce1(wps, eattr->r_snonce1)) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}
	wpabuf_free(decrypted);

	wps->state = SEND_M5;
end_process:
	if(eattr != NULL)
		atbm_kfree(eattr);
	
	return ret;
}


 static enum wps_process_res wps_process_m6(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	enum wps_process_res ret = WPS_CONTINUE;
	struct wpabuf *decrypted;
	struct wps_parse_attr *eattr;

	wpa_printf(MSG_ALWAYS, "WPS: Received M6---%dms", atbm_GetOsTime());
	
	eattr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*eattr), GFP_KERNEL);
	if(eattr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received M6, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps->state != RECV_M6) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M6", wps->state);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps_process_enrollee_nonce(wps, attr->enrollee_nonce) ||
	    wps_process_authenticator(wps, attr->authenticator, msg)) {
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	decrypted = wps_decrypt_encr_settings(wps, attr->encr_settings,
					      attr->encr_settings_len);
	if (decrypted == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to decrypted Encrypted "
			   "Settings attribute");
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps_validate_m6_encr(decrypted, attr->version2 != NULL) < 0) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	wpa_printf(MSG_DEBUG, "WPS: Processing decrypted Encrypted Settings "
		   "attribute");
	if (wps_parse_msg(decrypted, eattr) < 0 ||
	    wps_process_key_wrap_auth(wps, decrypted, eattr->key_wrap_auth) ||
	    wps_process_r_snonce2(wps, eattr->r_snonce2)) {
		wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}
	wpabuf_free(decrypted);

	if (wps->wps->ap)
		wps->wps->event_cb(wps->wps->cb_ctx, WPS_EV_AP_PIN_SUCCESS,
				   NULL);

	wps->state = SEND_M7;
end_process:
	if(eattr != NULL)
		atbm_kfree(eattr);
	
	return ret;
}

#ifndef NO_WORKAROUND_M8
//altobeam@20181019,  
//Because M8 decrypt ap settings cost time > 3secs, it will caused AP side timeout.
//so, add M8 pre-handle that response WSC DONE message immediately.
//We create task to process M8 message.

struct proc_msg_m8{
	struct wps_data *wps;
	struct wpabuf *msg;
	struct wps_parse_attr *attr;
};

static hal_thread_t process_m8_task;

 static enum wps_process_res wps_process_m8(struct wps_data *wps,const struct wpabuf *msg,struct wps_parse_attr *attr)
{
	enum wps_process_res ret = WPS_CONTINUE;
	struct wpabuf *decrypted;
	struct wps_parse_attr *eattr;
	atbm_uint32 start_T, end_T;
	start_T = atbm_GetOsTime();
	wpa_printf(MSG_ALWAYS, "WPS: Received M8---%dms", start_T);

	eattr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*eattr), GFP_KERNEL);
	if(eattr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received M8, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps->state != WPS_FINISHED) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M8", wps->state);
		ret = WPS_FAILURE;
		goto end_process;
	}

	decrypted = wps_decrypt_encr_settings(wps, attr->encr_settings,
						  attr->encr_settings_len);
	if (decrypted == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to decrypted Encrypted "
			   "Settings attribute");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps_validate_m8_encr(decrypted, wps->wps->ap,
				 attr->version2 != NULL) < 0) {
		wpa_printf(MSG_ALWAYS, "WPS:%s %d ERR",__func__,__LINE__);
		wpabuf_free(decrypted);
		ret = WPS_FAILURE;
		goto end_process;
	}

	wpa_printf(MSG_DEBUG, "WPS: Processing decrypted Encrypted Settings "
		   "attribute");
	if (wps_parse_msg(decrypted, eattr) < 0 ||
		wps_process_key_wrap_auth(wps, decrypted, eattr->key_wrap_auth) ||
		wps_process_creds(wps, eattr->cred, eattr->cred_len,
				  eattr->num_cred, attr->version2 != NULL)) {
		wpa_printf(MSG_ALWAYS, "WPS:%s %d ERR",__func__,__LINE__);
		wpabuf_free(decrypted);
		ret = WPS_FAILURE;
		goto end_process;
	}

	wpabuf_free(decrypted);

	end_T = atbm_GetOsTime();
	wpa_printf(MSG_ALWAYS, "WPS: M8 cost---%dms", end_T-start_T);

end_process:
	if(eattr != NULL)
		atbm_kfree(eattr);

	return ret;
}

 atbm_void wps_process_m8_task(void *data)
{
	enum wps_process_res ret = WPS_CONTINUE;
	
	struct proc_msg_m8 *proc_m8 = (struct proc_msg_m8 *)data;

	//wpa_printf(MSG_ALWAYS, "WPS: proc m8 task stack1 %d", atbm_getThreadStackFreesize());

	ret = wps_process_m8(proc_m8->wps, proc_m8->msg, proc_m8->attr);
	if(ret == WPS_FAILURE)
		wpa_printf(MSG_ERROR, "EAP-WSC: M8 task failed");
	else
		wpa_printf(MSG_ERROR, "EAP-WSC: M8 task success");

	//wpa_printf(MSG_ALWAYS, "WPS: proc m8 task stack2 %d", atbm_getThreadStackFreesize());

	wpabuf_free(proc_m8->msg);
	atbm_kfree(proc_m8->attr);
	atbm_kfree(proc_m8);
}
 static enum wps_process_res wps_process_m8_preHandle(struct wps_data *wps, const struct wpabuf *msg, struct wps_parse_attr *attr)
{
	enum wps_process_res ret = WPS_CONTINUE;

	atbm_uint32 start_T, end_T;
	struct proc_msg_m8 *proc_msg;

	start_T = atbm_GetOsTime();
	wpa_printf(MSG_ALWAYS, "WPS: Received pre M8---%dms", start_T);

	proc_msg = (struct proc_msg_m8 *)atbm_kzalloc(sizeof(*proc_msg), GFP_KERNEL);
	if(proc_msg == NULL){
		wpa_printf(MSG_ERROR, "WPS: M8 workaround process msg alloc failed");
		ret = WPS_FAILURE;
		goto __error__;
	}

	proc_msg->msg = wpabuf_dup(msg);
	if(proc_msg->msg == NULL){
		wpa_printf(MSG_ERROR, "WPS: M8 workaround msg alloc failed");
		ret = WPS_FAILURE;
		goto __error__;
	}
	
	proc_msg->attr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*proc_msg->attr), GFP_KERNEL);
	if(proc_msg->attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: M8 workaround attr alloc failed");
		ret = WPS_FAILURE;
		goto __error__;
	}
	
	atbm_memcpy(proc_msg->attr, attr, sizeof(*attr));	
	proc_msg->wps = wps;

	//create m8 process task
	process_m8_task.fn				= wps_process_m8_task;
	process_m8_task.name			= "proc_m8_t";
	process_m8_task.ptos			= NULL;
	process_m8_task.stack_size		= 1024*2;
	process_m8_task.arg				= (void *)proc_msg;
	process_m8_task.prio			= CONFIG_HAL_THREAD_PRIORITY;
	process_m8_task.task			= NULL;

	if(HAL_FAILURE == hal_create_thread(&process_m8_task)){
		wpa_printf(MSG_ERROR,"WPS: proc m8 task failed\n");
		ret = WPS_FAILURE;
		goto __error__;
	}


	if (wps->state != RECV_M8) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving pre M8", wps->state);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto __error__;
	}

	if(wps_process_enrollee_nonce(wps, attr->enrollee_nonce)){
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto __error__;
	}
	
	if (wps_process_authenticator(wps, attr->authenticator, msg)) {
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto __error__;
	}

	if (wps->wps->ap && wps->wps->ap_setup_locked) {
		/*
		 * Stop here if special ap_setup_locked == 2 mode allowed the
		 * protocol to continue beyond M2. This allows ER to learn the
		 * current AP settings without changing them.
		 */
		wpa_printf(MSG_ERROR, "WPS: AP Setup is locked - refuse "
			   "registration of a new Registrar");
		wps->config_error = WPS_CFG_SETUP_LOCKED;
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto __error__;
	}	
	wps->state = WPS_MSG_DONE;

	wps->wsc_data->out_buf = wps_enrollee_get_msg(wps, &wps->wsc_data->out_op_code);
 
	if (wps->wsc_data->out_buf == NULL) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Failed to get msg in pre M8");
		ret = WPS_FAILURE;
		goto __error__;
	}
	
	wps->wsc_data->out_used = 0;

	
	end_T = atbm_GetOsTime();
	wpa_printf(MSG_ALWAYS, "WPS: M8 pre cost---%dms", end_T-start_T);

	return ret;

__error__:
	wpabuf_free(proc_msg->msg);
	if(proc_msg->attr)
		atbm_kfree(proc_msg->attr);
	if(proc_msg)
		atbm_kfree(proc_msg);
	
	return ret;
}

#else
 static enum wps_process_res wps_process_m8(struct wps_data *wps,
					   const struct wpabuf *msg,
					   struct wps_parse_attr *attr)
{
	enum wps_process_res ret = WPS_CONTINUE;
	struct wpabuf *decrypted=NULL;
	struct wps_parse_attr *eattr;
	atbm_uint32 start_T, end_T;
	start_T = atbm_GetOsTime();
	wpa_printf(MSG_ALWAYS, "WPS: Received M8-%dms", start_T);

	eattr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*eattr), GFP_KERNEL);
	if(eattr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received M8, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps->state != RECV_M8) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected state (%d) for "
			   "receiving M8", wps->state);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if(wps_process_enrollee_nonce(wps, attr->enrollee_nonce)){
		wpa_printf(MSG_ERROR, "WPS: Unexpected enrollee_nonce");
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps_process_authenticator(wps, attr->authenticator, msg)) {
		wpa_printf(MSG_ERROR, "WPS: Unexpected authenticator");
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps->wps->ap && wps->wps->ap_setup_locked) {
		/*
		 * Stop here if special ap_setup_locked == 2 mode allowed the
		 * protocol to continue beyond M2. This allows ER to learn the
		 * current AP settings without changing them.
		 */
		wpa_printf(MSG_ERROR, "WPS: AP Setup is locked - refuse "
			   "registration of a new Registrar");
		wps->config_error = WPS_CFG_SETUP_LOCKED;
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	decrypted = wps_decrypt_encr_settings(wps, attr->encr_settings,
					      attr->encr_settings_len);
	if (decrypted == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to decrypted Encrypted "
			   "Settings attribute");
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	if (wps_validate_m8_encr(decrypted, wps->wps->ap,
				 attr->version2 != NULL) < 0) {
		wpa_printf(MSG_ERROR, "WPS: wps_validate_m8_encr");
		//wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	wpa_printf(MSG_DEBUG, "WPS: Processing decrypted Encrypted Settings "
		   "attribute");
	if (wps_parse_msg(decrypted, eattr) < 0 ||
	    wps_process_key_wrap_auth(wps, decrypted, eattr->key_wrap_auth) ||
	    wps_process_creds(wps, eattr->cred, eattr->cred_len,
			      eattr->num_cred, attr->version2 != NULL) /*||
	    wps_process_ap_settings_e(wps, eattr, decrypted,
				      attr->version2 != NULL)*/) {
		//wpabuf_free(decrypted);
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	wps->state = WPS_MSG_DONE;

	end_T = atbm_GetOsTime();
	wpa_printf(MSG_ALWAYS, "WPS: M8 cost---%dms", end_T-start_T);

end_process:
	if(eattr != NULL)
		atbm_kfree(eattr);
	wpabuf_free(decrypted);

	return ret;
}
#endif

 static enum wps_process_res wps_process_wsc_msg(struct wps_data *wps,
						const struct wpabuf *msg)
{
	struct wps_parse_attr *attr;
	enum wps_process_res ret = WPS_CONTINUE;

	wpa_printf(MSG_DEBUG, "WPS: Received WSC_MSG");
	attr = atbm_kmalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received WSC_MSG, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps_parse_msg(msg, attr) < 0){
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->enrollee_nonce == NULL ||
	    atbm_memcmp(wps->nonce_e, attr->enrollee_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: Mismatch in enrollee nonce");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->msg_type == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Message Type attribute");
		wps->state = SEND_WSC_NACK;
		ret = WPS_CONTINUE;
		goto end_process;
	}

	switch (*attr->msg_type) {
	case WPS_M2:
		if (wps_validate_m2(msg) < 0){
			ret = WPS_FAILURE;
			goto end_process;
		}
		ret = wps_process_m2(wps, msg, attr);
		break;
	case WPS_M2D:
		if (wps_validate_m2d(msg) < 0){
			ret = WPS_FAILURE;
			goto end_process;
		}
		ret = wps_process_m2d(wps, attr);
		break;
	case WPS_M4:
		if (wps_validate_m4(msg) < 0){
			ret = WPS_FAILURE;
			goto end_process;
		}
		ret = wps_process_m4(wps, msg, attr);
		if (ret == WPS_FAILURE || wps->state == SEND_WSC_NACK)
			wps_fail_event(wps->wps, WPS_M4, wps->config_error,
				       wps->error_indication);
		break;
	case WPS_M6:
		if (wps_validate_m6(msg) < 0){
			ret = WPS_FAILURE;
			goto end_process;
		}
		ret = wps_process_m6(wps, msg, attr);
		if (ret == WPS_FAILURE || wps->state == SEND_WSC_NACK)
			wps_fail_event(wps->wps, WPS_M6, wps->config_error,
				       wps->error_indication);
		break;
	case WPS_M8:
		if (wps_validate_m8(msg) < 0){
			ret = WPS_FAILURE;
			goto end_process;
		}
#ifndef NO_WORKAROUND_M8 //altobeam@20181019, pre-handle M8 msg.
		ret = wps_process_m8_preHandle(wps, msg, attr);
		if(ret == WPS_FAILURE){
			wps_fail_event(wps->wps, WPS_M8, wps->config_error,
				       wps->error_indication);
		}
#else
		ret = wps_process_m8(wps, msg, attr);
		if (ret == WPS_FAILURE || wps->state == SEND_WSC_NACK)
			wps_fail_event(wps->wps, WPS_M8, wps->config_error,
				       wps->error_indication);
#endif
		break;
	default:
		wpa_printf(MSG_ERROR, "WPS: Unsupported Message Type %d",
			   *attr->msg_type);
		ret = WPS_FAILURE;
		goto end_process;
	}

	/*
	 * Save a copy of the last message for Authenticator derivation if we
	 * are continuing. However, skip M2D since it is not authenticated and
	 * neither is the ACK/NACK response frame. This allows the possibly
	 * following M2 to be processed correctly by using the previously sent
	 * M1 in Authenticator derivation.
	 */
	if (ret == WPS_CONTINUE && *attr->msg_type != WPS_M2D) {
		/* Save a copy of the last message for Authenticator derivation
		 */
		wpabuf_free(wps->last_msg);
		wps->last_msg = wpabuf_dup(msg);
	}

end_process:
	if(attr != NULL)
		atbm_kfree(attr);

	return ret;
}


 static enum wps_process_res wps_process_wsc_ack(struct wps_data *wps,
						const struct wpabuf *msg)
{
	enum wps_process_res ret = WPS_CONTINUE;
	struct wps_parse_attr *attr;

	wpa_printf(MSG_ALWAYS, "WPS: Received WSC_ACK");

	attr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received WSC_ACK, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}
	
	if (wps_parse_msg(msg, attr) < 0){
		ret = WPS_FAILURE;
		goto end_process;
	}
	
	if (attr->msg_type == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Message Type attribute");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (*attr->msg_type != WPS_WSC_ACK) {
		wpa_printf(MSG_ERROR, "WPS: Invalid Message Type %d",
			   *attr->msg_type);
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->registrar_nonce == NULL ||
	    atbm_memcmp(wps->nonce_r, attr->registrar_nonce, WPS_NONCE_LEN) != 0)
	{
		wpa_printf(MSG_ERROR, "WPS: Mismatch in registrar nonce");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->enrollee_nonce == NULL ||
	    atbm_memcmp(wps->nonce_e, attr->enrollee_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: Mismatch in enrollee nonce");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps->state == RECV_ACK && wps->wps->ap) {
		wpa_printf(MSG_ERROR, "WPS: External Registrar registration "
			   "completed successfully");
		wps_success_event(wps->wps);
		wps->state = WPS_FINISHED;
		ret = WPS_DONE;
		goto end_process;
	}
	
end_process:
	if(attr)
		atbm_kfree(attr);
	
	return ret;
}


 static enum wps_process_res wps_process_wsc_nack(struct wps_data *wps,
						 const struct wpabuf *msg)
{
	enum wps_process_res ret = WPS_CONTINUE;
	struct wps_parse_attr *attr;
	atbm_uint16 config_error;

	wpa_printf(MSG_ALWAYS, "WPS: Received WSC_NACK");

	attr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*attr), GFP_KERNEL);
	if(attr == NULL){
		wpa_printf(MSG_ERROR, "WPS: Received WSC_NACK, malloc failed");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (wps_parse_msg(msg, attr) < 0){
		ret = WPS_FAILURE;
		goto end_process;
	}
	
	if (attr->msg_type == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Message Type attribute");
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (*attr->msg_type != WPS_WSC_NACK) {
		wpa_printf(MSG_ERROR, "WPS: Invalid Message Type %d",
			   *attr->msg_type);
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->registrar_nonce == NULL ||
	    atbm_memcmp(wps->nonce_r, attr->registrar_nonce, WPS_NONCE_LEN) != 0)
	{
		wpa_printf(MSG_ERROR, "WPS: Mismatch in registrar nonce");
		wpa_hexdump(MSG_ERROR, "WPS: Received Registrar Nonce",
			    attr->registrar_nonce, WPS_NONCE_LEN);
		wpa_hexdump(MSG_ERROR, "WPS: Expected Registrar Nonce",
			    wps->nonce_r, WPS_NONCE_LEN);
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->enrollee_nonce == NULL ||
	    atbm_memcmp(wps->nonce_e, attr->enrollee_nonce, WPS_NONCE_LEN) != 0) {
		wpa_printf(MSG_ERROR, "WPS: Mismatch in enrollee nonce");
		wpa_hexdump(MSG_ERROR, "WPS: Received Enrollee Nonce",
			    attr->enrollee_nonce, WPS_NONCE_LEN);
		wpa_hexdump(MSG_ERROR, "WPS: Expected Enrollee Nonce",
			    wps->nonce_e, WPS_NONCE_LEN);
		ret = WPS_FAILURE;
		goto end_process;
	}

	if (attr->config_error == NULL) {
		wpa_printf(MSG_ERROR, "WPS: No Configuration Error attribute "
			   "in WSC_NACK");
		ret = WPS_FAILURE;
		goto end_process;
	}

	config_error = ATBM_WPA_GET_BE16(attr->config_error);
	wpa_printf(MSG_DEBUG, "WPS: Registrar terminated negotiation with "
		   "Configuration Error %d", config_error);

	switch (wps->state) {
	case RECV_M4:
		wps_fail_event(wps->wps, WPS_M3, config_error,
			       wps->error_indication);
		break;
	case RECV_M6:
		wps_fail_event(wps->wps, WPS_M5, config_error,
			       wps->error_indication);
		break;
	case RECV_M8:
		wps_fail_event(wps->wps, WPS_M7, config_error,
			       wps->error_indication);
		break;
	default:
		break;
	}

	/* Followed by NACK if Enrollee is Supplicant or EAP-Failure if
	 * Enrollee is Authenticator */
	wps->state = SEND_WSC_NACK;

end_process:
	if(attr != NULL)
		atbm_kfree(attr);
	
	return ret;
}


 enum wps_process_res wps_enrollee_process_msg(struct wps_data *wps,
					      enum wsc_op_code op_code,
					      const struct wpabuf *msg)
{

	wpa_printf(MSG_DEBUG, "WPS: Processing received message (len=%lu "
		   "op_code=%d)",
		   (unsigned long) wpabuf_len(msg), op_code);

	if (op_code == WSC_UPnP) {
		/* Determine the OpCode based on message type attribute */
		struct wps_parse_attr *attr;
		attr = (struct wps_parse_attr *)atbm_kzalloc(sizeof(*attr), GFP_KERNEL);
		if(attr == NULL){
			wpa_printf(MSG_ERROR, "WPS: enro proc malloc failed");
			return WPS_FAILURE;			
		}
		if (wps_parse_msg(msg, attr) == 0 && attr->msg_type) {
			if (*attr->msg_type == WPS_WSC_ACK)
				op_code = WSC_ACK;
			else if (*attr->msg_type == WPS_WSC_NACK)
				op_code = WSC_NACK;
		}

		if(attr)
			atbm_kfree(attr);
	}

	switch (op_code) {
	case WSC_MSG:
	case WSC_UPnP:
		return wps_process_wsc_msg(wps, msg);
	case WSC_ACK:
		if (wps_validate_wsc_ack(msg) < 0)
			return WPS_FAILURE;
		return wps_process_wsc_ack(wps, msg);
	case WSC_NACK:
		if (wps_validate_wsc_nack(msg) < 0)
			return WPS_FAILURE;
		return wps_process_wsc_nack(wps, msg);
	default:
		wpa_printf(MSG_ERROR, "WPS: Unsupported op_code %d", op_code);
		return WPS_FAILURE;
	}
}

