/*
 * EAP common peer/server definitions
 * Copyright (c) 2004-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
#include "atbm_hal.h"
#include "eap_common.h"

extern atbm_void hostapd_eap_wsc_process(struct hostapd_data *hapd,struct hostapd_sta_info *sta,struct wpabuf *respData);
 static inline int os_snprintf_error(atbm_size_t size, int res)
{
	return res < 0 || (atbm_uint32) res >= size;
}


 static inline int _wpa_snprintf_hex(char *buf, atbm_size_t buf_size, const atbm_uint8 *data,
				    atbm_size_t len, int uppercase)
{
	atbm_size_t i;
	char *pos = buf, *end = buf + buf_size;
	int ret;
	if (buf_size == 0)
		return 0;
	for (i = 0; i < len; i++) {
		ret = sprintf(pos, uppercase ? "%02X" : "%02x",
				  data[i]);
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return pos - buf;
		}
		pos += ret;
	}
	end[-1] = '\0';
	return pos - buf;
}


/**
 * atbmwifi_wpa_snprintf_hex - Print data as a hex string into a buffer
 * @buf: Memory area to use as the output buffer
 * @buf_size: Maximum buffer size in bytes (should be at least 2 * len + 1)
 * @data: Data to be printed
 * @len: Length of data in bytes
 * Returns: Number of bytes written
 */
 int atbmwifi_wpa_snprintf_hex(char *buf, atbm_size_t buf_size, const atbm_uint8 *data, atbm_size_t len)
{
	return _wpa_snprintf_hex(buf, buf_size, data, len, 0);
}

/**
 * eap_hdr_len_valid - Validate EAP header length field
 * @msg: EAP frame (starting with EAP header)
 * @min_payload: Minimum payload length needed
 * Returns: 1 for valid header, 0 for invalid
 *
 * This is a helper function that does minimal validation of EAP messages. The
 * length field is verified to be large enough to include the header and not
 * too large to go beyond the end of the buffer.
 */
 atbm_int32 eap_hdr_len_valid(const struct wpabuf *msg, atbm_size_t min_payload)
{
	const struct atbm_eap_hdr *hdr;
	atbm_size_t len;

	if (msg == NULL)
		return 0;

	hdr = wpabuf_head(msg);

	if (wpabuf_len(msg) < sizeof(*hdr)) {
		wpa_printf(MSG_INFO, "EAP: Too short EAP frame");
		return 0;
	}

	len = atbm_be_to_host16(hdr->length);
	if (len < sizeof(*hdr) + min_payload || len > wpabuf_len(msg)) {
		wpa_printf(MSG_INFO, "EAP: Invalid EAP length");
		return 0;
	}

	return 1;
}


/**
 * eap_hdr_validate - Validate EAP header
 * @vendor: Expected EAP Vendor-Id (0 = IETF)
 * @eap_type: Expected EAP type number
 * @msg: EAP frame (starting with EAP header)
 * @plen: Pointer to variable to contain the returned payload length
 * Returns: Pointer to EAP payload (after type field), or %NULL on failure
 *
 * This is a helper function for EAP method implementations. This is usually
 * called in the beginning of struct eap_method::process() function to verify
 * that the received EAP request packet has a valid header. This function is
 * able to process both legacy and expanded EAP headers and in most cases, the
 * caller can just use the returned payload pointer (into *plen) for processing
 * the payload regardless of whether the packet used the expanded EAP header or
 * not.
 */
 const atbm_uint8 * eap_hdr_validate(EapVenType vendor, EapType eap_type,
			    const struct wpabuf *msg, atbm_size_t *plen)
{
	const struct atbm_eap_hdr *hdr;
	const atbm_uint8 *pos;
	atbm_size_t len;

	if (!eap_hdr_len_valid(msg, 1))
		return NULL;

	hdr = wpabuf_head(msg);
	len = atbm_be_to_host16(hdr->length);
	pos = (const atbm_uint8 *) (hdr + 1);

	if (*pos == ATBM_EAP_TYPE_EXPANDED) {
		int exp_vendor;
		atbm_uint32 exp_type;
		if (len < sizeof(*hdr) + 8) {
			wpa_printf(MSG_INFO, "EAP: Invalid expanded EAP "
				   "length");
			return NULL;
		}
		pos++;
		exp_vendor = ATBM_WPA_GET_BE24(pos);
		pos += 3;
		exp_type = ATBM_WPA_GET_BE32(pos);
		pos += 4;
		if (exp_vendor != vendor || exp_type != (atbm_uint32) eap_type) {
			wpa_printf(MSG_INFO, "EAP: Invalid expanded frame "
				   "type");
			return NULL;
		}

		*plen = len - sizeof(*hdr) - 8;
		return pos;
	} else {
		if (vendor != ATBM_EAP_VENDOR_IETF || *pos != eap_type) {
			wpa_printf(MSG_INFO, "EAP: Invalid frame type");
			return NULL;
		}
		*plen = len - sizeof(*hdr) - 1;
		return pos + 1;
	}
}


/**
 * eap_msg_alloc - Allocate a buffer for an EAP message
 * @vendor: Vendor-Id (0 = IETF)
 * @type: EAP type
 * @payload_len: Payload length in bytes (data after Type)
 * @code: Message Code (EAP_CODE_*)
 * @identifier: Identifier
 * Returns: Pointer to the allocated message buffer or %NULL on error
 *
 * This function can be used to allocate a buffer for an EAP message and fill
 * in the EAP header. This function is automatically using expanded EAP header
 * if the selected Vendor-Id is not IETF. In other words, most EAP methods do
 * not need to separately select which header type to use when using this
 * function to allocate the message buffers. The returned buffer has room for
 * payload_len bytes and has the EAP header and Type field already filled in.
 */
struct wpabuf * eap_msg_alloc(EapVenType vendor, EapType type, atbm_uint32 payload_len,
			      EapCodeType code, atbm_uint8 identifier)
{
	struct wpabuf *buf;
	struct atbm_eap_hdr *hdr;
	atbm_size_t len;

	len = sizeof(struct atbm_eap_hdr) + (vendor == ATBM_EAP_VENDOR_IETF ? 1 : 8) +
		payload_len;
	buf = wpabuf_alloc(len);
	if (buf == NULL)
		return NULL;

	hdr = wpabuf_put(buf, sizeof(*hdr));
	hdr->code = code;
	hdr->identifier = identifier;
	hdr->length = atbm_host_to_be16(len);

	if (vendor == ATBM_EAP_VENDOR_IETF) {
		wpabuf_put_u8(buf, type);
	} else {
		wpabuf_put_u8(buf, ATBM_EAP_TYPE_EXPANDED);
		wpabuf_put_be24(buf, vendor);
		wpabuf_put_be32(buf, type);
	}

	return buf;
}


/**
 * eap_update_len - Update EAP header length
 * @msg: EAP message from eap_msg_alloc
 *
 * This function updates the length field in the EAP header to match with the
 * current length for the buffer. This allows eap_msg_alloc() to be used to
 * allocate a larger buffer than the exact message length (e.g., if exact
 * message length is not yet known).
 */
 atbm_void eap_update_len(struct wpabuf *msg)
{
	struct atbm_eap_hdr *hdr;
	hdr = wpabuf_mhead(msg);
	if (wpabuf_len(msg) < sizeof(*hdr))
		return;
	hdr->length = atbm_host_to_be16(wpabuf_len(msg));
}


/**
 * eap_get_id - Get EAP Identifier from wpabuf
 * @msg: Buffer starting with an EAP header
 * Returns: The Identifier field from the EAP header
 */
 atbm_uint8 eap_get_id(const struct wpabuf *msg)
{
	const struct atbm_eap_hdr *eap;

	if (wpabuf_len(msg) < sizeof(*eap))
		return 0;

	eap = wpabuf_head(msg);
	return eap->identifier;
}


/**
 * eap_get_id - Get EAP Type from wpabuf
 * @msg: Buffer starting with an EAP header
 * Returns: The EAP Type after the EAP header
 */
 EapType eap_get_type(const struct wpabuf *msg)
{
	if (wpabuf_len(msg) < sizeof(struct atbm_eap_hdr) + 1)
		return ATBM_EAP_TYPE_NONE;

	return ((const atbm_uint8 *) wpabuf_head(msg))[sizeof(struct atbm_eap_hdr)];
}




 static atbm_void eap_identity_process(struct hostapd_data *hapd,struct hostapd_sta_info *sta, struct wpabuf *respData)
{
	struct atbmwifi_vif *priv = hapd->priv;
	const atbm_uint8 *pos;
	atbm_size_t len;
	struct wpabuf * req = 0;
	struct atbmwifi_ieee802_1x_hdr *hdr = 0;
	
	pos = eap_hdr_validate(ATBM_EAP_VENDOR_IETF, ATBM_EAP_TYPE_IDENTITY,
			       respData, &len);
	if (pos == NULL)
		return; /* Should not happen - frame already validated */

	wpa_printf(MSG_INFO, "EAP-Identity: Peer identity\n");

	//wpa_printf(MSG_INFO, "EAP-Response/Identity '%s'\n", pos);

	if (len == WSC_ID_ENROLLEE_LEN &&
	      atbm_memcmp(pos, WSC_ID_ENROLLEE,WSC_ID_ENROLLEE_LEN) == 0)
	{
		
	}
	
	req = eap_wsc_build_start(1);	
	if (req == NULL) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Failed to allocate memory for "
			   "request");
		return;
	}
	wpa_printf(MSG_DEBUG, "EAP-WSC: Send WSC/Start");
	
	len = sizeof(struct atbmwifi_ieee802_1x_hdr) + req->used;
	if(hapd->wps_tx_hdr){
		atbm_kfree(hapd->wps_tx_hdr);
		hapd->wps_tx_hdr = ATBM_NULL;
	}
	hdr = (struct atbmwifi_ieee802_1x_hdr *)atbm_kzalloc(len, GFP_KERNEL);
	if (hdr == NULL)
	{
		wpabuf_free(req);
		return;
	}
	// TO DO GET VERSION
	hdr->version = EAPOL_VERSION;
	hdr->type = ATBM_IEEE802_1X_TYPE_EAP_PACKET;
	hdr->length = atbm_host_to_be16(req->used);
	atbm_memcpy((atbm_uint8 *)(hdr + 1), req->buf, req->used);
	
	hostapd_send_eapol(priv, sta->addr,ATBM_ETH_P_EAPOL,(atbm_uint8 *)hdr,len);
	wpabuf_free(req);
	atbm_kfree(hdr);
	hdr = 0;
}

 static atbm_void handle_eap_response(struct hostapd_data *hapd,
				struct hostapd_sta_info *sta, struct atbm_eap_hdr *eap,
				atbm_size_t len)
{
	atbm_uint8 type, *data;
	struct wpabuf *buf;

	data = (atbm_uint8 *) (eap + 1);

	if (len < sizeof(*eap) + 1) {
		wpa_printf(MSG_INFO, "handle_eap_response: too short response data");
		return;
	}
	type = data[0];

	wpa_printf(MSG_DEBUG,  "received EAP packet (code=%d "
		       "id=%d len=%d) from STA: EAP Response (%d)\n",
		       eap->code, eap->identifier, atbm_be_to_host16(eap->length),type);
	buf = wpabuf_alloc_copy(eap, len);
	switch(type){
		case ATBM_EAP_TYPE_IDENTITY:
			eap_identity_process(hapd,sta, buf);
			break;
		case ATBM_EAP_TYPE_EXPANDED:
			hostapd_eap_wsc_process(hapd, sta, buf);
			break;
		default:
			break;
	}
	wpabuf_free(buf);
}



/* Process incoming EAP packet from Supplicant */
 atbm_void handle_eap(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
		       atbm_uint8 *buf, atbm_size_t len)
{
	struct atbm_eap_hdr *eap;
	atbm_uint16 eap_len;

	if (len < sizeof(*eap)) {
		wpa_printf(MSG_INFO, "   too short EAP packet");
		return;
	}

	eap = (struct atbm_eap_hdr *) buf;

	eap_len = atbm_be_to_host16(eap->length);
	wpa_printf(MSG_DEBUG, "EAP: code=%d identifier=%d length=%d\n",
		   eap->code, eap->identifier, eap_len);
	if (eap_len < sizeof(*eap)) {
		wpa_printf(MSG_DEBUG, "   Invalid EAP length");
		return;
	} else if (eap_len > len) {
		wpa_printf(MSG_DEBUG, "   Too short frame to contain this EAP "
			   "packet");
		return;
	} else if (eap_len < len) {
		wpa_printf(MSG_DEBUG, "   Ignoring %lu extra bytes after EAP "
			   "packet", (unsigned long) len - eap_len);
	}

	switch (eap->code) {
	case EAP_CODE_REQUEST:
		wpa_printf(MSG_DEBUG, " (request)");
		return;
	case EAP_CODE_RESPONSE:
		wpa_printf(MSG_DEBUG, " (response)\n");
		handle_eap_response(hapd, sta, eap, eap_len);
		break;
	case EAP_CODE_SUCCESS:
		wpa_printf(MSG_DEBUG, " (success)");
		return;
	case EAP_CODE_FAILURE:
		wpa_printf(MSG_DEBUG, " (failure)");
		return;
	case EAP_CODE_INITIATE:
		wpa_printf(MSG_DEBUG, " (initiate)");
		//handle_eap_initiate(hapd, sta, eap, eap_len);
		break;
	case EAP_CODE_FINISH:
		wpa_printf(MSG_DEBUG, " (finish)");
		break;
	default:
		wpa_printf(MSG_DEBUG, " (unknown code)");
		return;
	}
	
}

