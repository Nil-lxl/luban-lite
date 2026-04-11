/*
 * Wi-Fi Protected Setup - common functionality
 * Copyright (c) 2008-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"
#include "wpa_debug.h"

 atbm_void wps_kdf(const atbm_uint8 *key, const atbm_uint8 *label_prefix, atbm_size_t label_prefix_len,
	     const char *label, atbm_uint8 *res, atbm_size_t res_len)
{
	atbm_uint8 i_buf[4], key_bits[4];
	const atbm_uint8 *addr[4];
	atbm_size_t len[4];
	atbm_int32 i, iter;
	atbm_uint8 hash[SHA256_MAC_LEN], *opos;
	atbm_size_t left;

	ATBM_WPA_PUT_BE32(key_bits, res_len * 8);

	addr[0] = i_buf;
	len[0] = sizeof(i_buf);
	addr[1] = label_prefix;
	len[1] = label_prefix_len;
	addr[2] = (const atbm_uint8 *) label;
	len[2] = strlen(label);
	addr[3] = key_bits;
	len[3] = sizeof(key_bits);

	iter = (res_len + SHA256_MAC_LEN - 1) / SHA256_MAC_LEN;
	opos = res;
	left = res_len;

	for (i = 1; i <= iter; i++) {
		ATBM_WPA_PUT_BE32(i_buf, i);
		atbmwifi_hmac_sha256_vector(key, SHA256_MAC_LEN, 4, addr, len, hash);
		if (i < iter) {
			atbm_memcpy(opos, hash, SHA256_MAC_LEN);
			opos += SHA256_MAC_LEN;
			left -= SHA256_MAC_LEN;
		} else
			atbm_memcpy(opos, hash, left);
	}
}


 atbm_int32 wps_derive_keys(struct wps_data *wps)
{
	struct wpabuf *pubkey, *dh_shared;
	atbm_uint8 *dhkey, *kdk;
	const atbm_uint8 *addr[3];
	atbm_size_t len[3];
	atbm_uint8 *keys;

	if (wps->dh_privkey == NULL) {
		wpa_printf(MSG_DEBUG, "WPS: Own DH private key not available");
		return -1;
	}

	pubkey = wps->registrar ? wps->dh_pubkey_e : wps->dh_pubkey_r;
	if (pubkey == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Peer DH public key not available");
		return -1;
	}

	wpa_hexdump_buf_key(MSG_DEBUG, "WPS: DH Private Key", wps->dh_privkey);
	wpa_hexdump_buf(MSG_DEBUG, "WPS: DH peer Public Key", pubkey);
	dh_shared = dh5_derive_shared(wps->dh_ctx, pubkey, wps->dh_privkey);
	dh5_free(wps->dh_ctx);
	wps->dh_ctx = NULL;
	dh_shared = wpabuf_zeropad(dh_shared, 192);
	if (dh_shared == NULL) {
		wpa_printf(MSG_ERROR, "WPS: Failed to derive DH shared key");
		return -1;
	}

	/* Own DH private key is not needed anymore */
	wpabuf_free(wps->dh_privkey);
	wps->dh_privkey = NULL;

	wpa_printf(MSG_ERROR, "WPS: DH shared key");
	dhkey = (atbm_uint8 *)atbm_kzalloc(SHA256_MAC_LEN, GFP_KERNEL);
	if (dhkey == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: DH shared key alloc dhkey failed");
		wpabuf_free(dh_shared);
		return -1;
	}
	kdk = (atbm_uint8 *)atbm_kzalloc(SHA256_MAC_LEN, GFP_KERNEL);
	if (kdk == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: DH shared key alloc kdk failed");	
		wpabuf_free(dh_shared);
		atbm_kfree(dhkey);
		return -1;
	}

	/* DHKey = SHA-256(g^AB mod p) */
	addr[0] = wpabuf_head(dh_shared);
	len[0] = wpabuf_len(dh_shared);
	atbmwifi_sha256_vector(1, addr, len, dhkey);
	//wpa_hexdump_key(MSG_DEBUG, "WPS: DHKey", dhkey, SHA256_MAC_LEN);
	wpabuf_free(dh_shared);

	/* KDK = HMAC-SHA-256_DHKey(N1 || EnrolleeMAC || N2) */
	addr[0] = wps->nonce_e;
	len[0] = WPS_NONCE_LEN;
	addr[1] = wps->mac_addr_e;
	len[1] = ATBM_ETH_ALEN;
	addr[2] = wps->nonce_r;
	len[2] = WPS_NONCE_LEN;
	atbmwifi_hmac_sha256_vector(dhkey, SHA256_MAC_LEN, 3, addr, len, kdk);
	//wpa_hexdump_key(MSG_DEBUG, "WPS: KDK", kdk, SHA256_MAC_LEN);

	keys = (atbm_uint8 *)atbm_kzalloc(WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN + WPS_EMSK_LEN, GFP_KERNEL);
	if (keys == NULL)
	{
		wpa_printf(MSG_ERROR, "WPS: DH shared key alloc mem failed");
		atbm_kfree(dhkey);
		atbm_kfree(kdk);
		return -1;
	}

	wps_kdf(kdk, NULL, 0, "Wi-Fi Easy and Secure Key Derivation",
		keys, WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN + WPS_EMSK_LEN);
	atbm_memcpy(wps->authkey, keys, WPS_AUTHKEY_LEN);
	atbm_memcpy(wps->keywrapkey, keys + WPS_AUTHKEY_LEN, WPS_KEYWRAPKEY_LEN);
	atbm_memcpy(wps->emsk, keys + WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN,
		  WPS_EMSK_LEN);

	//wpa_hexdump_key(MSG_DEBUG, "WPS: AuthKey",
	//		wps->authkey, WPS_AUTHKEY_LEN);
	//wpa_hexdump_key(MSG_DEBUG, "WPS: KeyWrapKey",
	//		wps->keywrapkey, WPS_KEYWRAPKEY_LEN);
	//wpa_hexdump_key(MSG_DEBUG, "WPS: EMSK", wps->emsk, WPS_EMSK_LEN);
	atbm_kfree(dhkey);
	atbm_kfree(keys);
	atbm_kfree(kdk);

	return 0;
}


 atbm_void wps_derive_psk(struct wps_data *wps, const atbm_uint8 *dev_passwd,
		    atbm_size_t dev_passwd_len)
{
	atbm_uint8 hash[SHA256_MAC_LEN];

	atbmwifi_hmac_sha256(wps->authkey, WPS_AUTHKEY_LEN, dev_passwd,
		    (dev_passwd_len + 1) / 2, hash);
	atbm_memcpy(wps->psk1, hash, WPS_PSK_LEN);
	atbmwifi_hmac_sha256(wps->authkey, WPS_AUTHKEY_LEN,
		    dev_passwd + (dev_passwd_len + 1) / 2,
		    dev_passwd_len / 2, hash);
	atbm_memcpy(wps->psk2, hash, WPS_PSK_LEN);

	wpa_hexdump_ascii_key(MSG_DEBUG, "WPS: Device Password",
			      dev_passwd, dev_passwd_len);
	wpa_hexdump_key(MSG_DEBUG, "WPS: PSK1", wps->psk1, WPS_PSK_LEN);
	wpa_hexdump_key(MSG_DEBUG, "WPS: PSK2", wps->psk2, WPS_PSK_LEN);
}


 struct wpabuf * wps_decrypt_encr_settings(struct wps_data *wps, const atbm_uint8 *encr,
					  atbm_size_t encr_len)
{
	struct wpabuf *decrypted;
	const atbm_size_t block_size = 16;
	atbm_size_t i;
	atbm_uint8 pad;
	const atbm_uint8 *pos;

	/* AES-128-CBC */
	if (encr == NULL || encr_len < 2 * block_size || encr_len % block_size)
	{
		wpa_printf(MSG_ERROR, "WPS: No Encrypted Settings received");
		return NULL;
	}

	decrypted = wpabuf_alloc(encr_len - block_size);
	if (decrypted == NULL){
		wpa_printf(MSG_ERROR, "WPS: wpabuf_alloc err");
		return NULL;
	}

	wpa_hexdump(MSG_MSGDUMP, "WPS: Encrypted Settings", encr, encr_len);
	wpabuf_put_data(decrypted, encr + block_size, encr_len - block_size);
	if (atbmwifi_aes_128_cbc_decrypt(wps->keywrapkey, encr, wpabuf_mhead(decrypted),
				wpabuf_len(decrypted))) {
		wpa_printf(MSG_ERROR, "WPS: atbmwifi_aes_128_cbc_decrypt err");
		wpabuf_free(decrypted);
		return NULL;
	}

	wpa_hexdump_buf_key(MSG_MSGDUMP, "WPS: Decrypted Encrypted Settings",
			    decrypted);

	pos = wpabuf_head_u8(decrypted) + wpabuf_len(decrypted) - 1;
	pad = *pos;
	if (pad > wpabuf_len(decrypted)) {
		wpa_printf(MSG_ERROR, "WPS: Invalid PKCS#5 v2.0 pad value");
		wpabuf_free(decrypted);
		return NULL;
	}
	for (i = 0; i < pad; i++) {
		if (*pos-- != pad) {
			wpa_printf(MSG_ERROR, "WPS: Invalid PKCS#5 v2.0 pad "
				   "string");
			wpabuf_free(decrypted);
			return NULL;
		}
	}
	decrypted->used -= pad;

	return decrypted;
}


/**
 * wps_pin_checksum - Compute PIN checksum
 * @pin: Seven digit PIN (i.e., eight digit PIN without the checksum digit)
 * Returns: Checksum digit
 */
 atbm_uint32 wps_pin_checksum(atbm_uint32 pin)
{
	atbm_uint32 accum = 0;
	while (pin) {
		accum += 3 * (pin % 10);
		pin /= 10;
		accum += pin % 10;
		pin /= 10;
	}

	return (10 - accum % 10) % 10;
}


/**
 * wps_pin_valid - Check whether a PIN has a valid checksum
 * @pin: Eight digit PIN (i.e., including the checksum digit)
 * Returns: 1 if checksum digit is valid, or 0 if not
 */
 atbm_uint32 wps_pin_valid(atbm_uint32 pin)
{
	return wps_pin_checksum(pin / 10) == (pin % 10);
}


/**
 * wps_generate_pin - Generate a random PIN
 * Returns: Eight digit PIN (i.e., including the checksum digit)
 */
 atbm_uint32 wps_generate_pin(atbm_void)
{
	atbm_uint32 val;
	atbm_uint32 sec, msec;
	/* Generate seven random digits for the PIN */
	if (random_get_bytes((atbm_uint8 *) &val, sizeof(val)) < 0) {
		sec = atbm_GetOsTime() / 1000;
		msec = atbm_GetOsTime();
		val = atbmwifi_os_random() ^ sec ^ msec;
	}
	val %= 10000000;

	/* Append checksum digit */
	return val * 10 + wps_pin_checksum(val);
}


 atbm_int32 wps_pin_str_valid(const char *pin)
{
	const char *p;
	atbm_size_t len;

	p = pin;
	while (*p >= '0' && *p <= '9')
		p++;
	if (*p != '\0')
		return 0;

	len = p - pin;
	return len == 4 || len == 8;
}


 atbm_void wps_fail_event(struct wps_context *wps, enum wps_msg_type msg,
		    atbm_uint16 config_error, atbm_uint16 error_indication)
{
	union wps_event_data data;

	if (wps->event_cb == NULL)
		return;

	atbm_memset(&data, 0, sizeof(data));
	data.fail.msg = msg;
	data.fail.config_error = config_error;
	data.fail.error_indication = error_indication;
	wps->event_cb(wps->cb_ctx, WPS_EV_FAIL, &data);
}


 atbm_void wps_success_event(struct wps_context *wps)
{
	//union wps_event_data data;

	if (wps->event_cb == NULL)
		return;

	//atbm_memset(&data, 0, sizeof(data));
	//atbm_memcpy(data.success.peer_macaddr, mac_addr, ATBM_ETH_ALEN);
	wps->event_cb(wps->cb_ctx, WPS_EV_SUCCESS, /*&data*/NULL);
}


 atbm_void wps_pwd_auth_fail_event(struct wps_context *wps, atbm_int32 enrollee, atbm_int32 part)
{
	union wps_event_data data;

	if (wps->event_cb == NULL)
		return;

	atbm_memset(&data, 0, sizeof(data));
	data.pwd_auth_fail.enrollee = enrollee;
	data.pwd_auth_fail.part = part;
	//atbm_memcpy(data.pwd_auth_fail.peer_macaddr, mac_addr, ATBM_ETH_ALEN);
	wps->event_cb(wps->cb_ctx, WPS_EV_PWD_AUTH_FAIL, &data);
}


 atbm_void wps_pbc_overlap_event(struct wps_context *wps)
{
	if (wps->event_cb == NULL)
		return;

	wps->event_cb(wps->cb_ctx, WPS_EV_PBC_OVERLAP, NULL);
}


 atbm_void wps_pbc_timeout_event(struct wps_context *wps)
{
	if (wps->event_cb == NULL)
		return;

	wps->event_cb(wps->cb_ctx, WPS_EV_PBC_TIMEOUT, NULL);
}


 atbm_void wps_pbc_active_event(struct wps_context *wps)
{
	if (wps->event_cb == NULL)
		return;

	wps->event_cb(wps->cb_ctx, WPS_EV_PBC_ACTIVE, NULL);
}


 atbm_void wps_pbc_disable_event(struct wps_context *wps)
{
	if (wps->event_cb == NULL)
		return;

	wps->event_cb(wps->cb_ctx, WPS_EV_PBC_DISABLE, NULL);
}




 atbm_int32 wps_dev_type_str2bin(const char *str, atbm_uint8 dev_type[WPS_DEV_TYPE_LEN])
{
	char *pos;

	/* <categ>-<OUI>-<subcateg> */
	ATBM_WPA_PUT_BE16(dev_type, atoi(str));
	pos = strchr(str, '-');
	if (pos == NULL)
		return -1;
	pos++;
	if (atbmwifi_hexstr2bin(&dev_type[2], pos, 4))
		return -1;
	pos = strchr(pos, '-');
	if (pos == NULL)
		return -1;
	pos++;
	ATBM_WPA_PUT_BE16(&dev_type[6], atoi(pos));


	return 0;
}


 char * wps_dev_type_bin2str(const atbm_uint8 dev_type[WPS_DEV_TYPE_LEN], char *buf,
			    atbm_size_t buf_len)
{
	atbm_int32 ret;

	ret = sprintf(buf,  "%u-%08X-%u",
			  ATBM_WPA_GET_BE16(dev_type), ATBM_WPA_GET_BE32(&dev_type[2]),
			  ATBM_WPA_GET_BE16(&dev_type[6]));
	//if (os_snprintf_error(buf_len, ret))
	//	return NULL;

	return buf;
}


 atbm_void uuid_gen_mac_addr(const atbm_uint8 *mac_addr, atbm_uint8 *uuid)
{
	const atbm_uint8 *addr[2];
	atbm_size_t len[2];
	atbm_uint8 hash[SHA1_MAC_LEN];
	atbm_uint8 nsid[16] = {
		0x52, 0x64, 0x80, 0xf8,
		0xc9, 0x9b,
		0x4b, 0xe5,
		0xa6, 0x55,
		0x58, 0xed, 0x5f, 0x5d, 0x60, 0x84
	};

	addr[0] = nsid;
	len[0] = sizeof(nsid);
	addr[1] = mac_addr;
	len[1] = 6;
	atbmwifi_sha1_vector(2, addr, len, hash);
	atbm_memcpy(uuid, hash, 16);

	/* Version: 5 = named-based version using SHA-1 */
	uuid[6] = (5 << 4) | (uuid[6] & 0x0f);

	/* Variant specified in RFC 4122 */
	uuid[8] = 0x80 | (uuid[8] & 0x3f);
}


 atbm_uint16 wps_config_methods_str2bin(const char *str)
{
	atbm_uint16 methods = 0;

	if (str == NULL) {
		/* Default to enabling methods based on build configuration */
		methods |= WPS_CONFIG_DISPLAY | WPS_CONFIG_KEYPAD;
		methods |= WPS_CONFIG_VIRT_DISPLAY;
#ifdef CONFIG_WPS_NFC
		methods |= WPS_CONFIG_NFC_INTERFACE;
#endif /* CONFIG_WPS_NFC */
#if CONFIG_P2P
		methods |= WPS_CONFIG_P2PS;
#endif /* CONFIG_P2P */
	} else {
		if (strstr(str, "ethernet"))
			methods |= WPS_CONFIG_ETHERNET;
		if (strstr(str, "label"))
			methods |= WPS_CONFIG_LABEL;
		if (strstr(str, "display"))
			methods |= WPS_CONFIG_DISPLAY;
		if (strstr(str, "ext_nfc_token"))
			methods |= WPS_CONFIG_EXT_NFC_TOKEN;
		if (strstr(str, "int_nfc_token"))
			methods |= WPS_CONFIG_INT_NFC_TOKEN;
		if (strstr(str, "nfc_interface"))
			methods |= WPS_CONFIG_NFC_INTERFACE;
		if (strstr(str, "push_button"))
			methods |= WPS_CONFIG_PUSHBUTTON;
		if (strstr(str, "keypad"))
			methods |= WPS_CONFIG_KEYPAD;
		if (strstr(str, "virtual_display"))
			methods |= WPS_CONFIG_VIRT_DISPLAY;
		if (strstr(str, "physical_display"))
			methods |= WPS_CONFIG_PHY_DISPLAY;
		if (strstr(str, "virtual_push_button"))
			methods |= WPS_CONFIG_VIRT_PUSHBUTTON;
		if (strstr(str, "physical_push_button"))
			methods |= WPS_CONFIG_PHY_PUSHBUTTON;
		if (strstr(str, "p2ps"))
			methods |= WPS_CONFIG_P2PS;
	}

	return methods;
}

/**
 * wps_build_assoc_resp_ie - Build WPS IE for (Re)Association Response
 * Returns: WPS IE or %NULL on failure
 *
 * The caller is responsible for freeing the buffer.
 */
 struct wpabuf * wps_build_assoc_resp_ie(atbm_void)
{
	struct wpabuf *ie;
	atbm_uint8 *len;

	wpa_printf(MSG_DEBUG, "WPS: Building WPS IE for (Re)Association "
		   "Response");
	ie = wpabuf_alloc(100);
	if (ie == NULL)
		return NULL;

	wpabuf_put_u8(ie, ATBM_WLAN_EID_VENDOR_SPECIFIC);
	len = wpabuf_put(ie, 1);
	wpabuf_put_be32(ie, WPS_DEV_OUI_WFA);

	if (wps_build_version(ie) ||
	    wps_build_resp_type(ie, WPS_RESP_AP) ||
	    wps_build_wfa_ext(ie, 0, NULL, 0)) {
		wpabuf_free(ie);
		return NULL;
	}

	*len = wpabuf_len(ie) - 2;

	return ie;
}

 struct wpabuf * wps_build_wsc_ack(struct wps_data *wps)
{
	struct wpabuf *msg;

	wpa_printf(MSG_DEBUG, "WPS: Building Message WSC_ACK");

	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_WSC_ACK) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0)) {
		wpabuf_free(msg);
		return NULL;
	}

	return msg;
}


 struct wpabuf * wps_build_wsc_nack(struct wps_data *wps)
{
	struct wpabuf *msg;

	wpa_printf(MSG_ALWAYS, "WPS: Building Message WSC_NACK");

	msg = wpabuf_alloc(1000);
	if (msg == NULL)
		return NULL;

	if (wps_build_version(msg) ||
	    wps_build_msg_type(msg, WPS_WSC_NACK) ||
	    wps_build_enrollee_nonce(wps, msg) ||
	    wps_build_registrar_nonce(wps, msg) ||
	    wps_build_config_error(msg, wps->config_error) ||
	    wps_build_wfa_ext(msg, 0, NULL, 0)) {
		wpabuf_free(msg);
		return NULL;
	}

	return msg;
}


