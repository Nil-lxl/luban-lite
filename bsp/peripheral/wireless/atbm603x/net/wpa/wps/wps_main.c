/**************************************************************************************************************
* altobeam  Wi-Fi
*
* Copyright (c) 2018, altobeam.inc   All rights reserved.
*
* The source code contains proprietary information of AltoBeam, and shall not be distributed, 
* copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "uuid.h"
#include "wps.h"
#include "wps_i.h"
#include "wps_defs.h"
#include "wps_dev_attr.h"
#include "wps_attr_parse.h"
#include "wps_hostapd.h"
#include "wpabuf.h"
#include "wps_attr_parse.h"
#include "wps_sta.h"
#include "wpa_debug.h"

extern struct wpabuf * eap_msg_alloc(EapVenType vendor, EapType type, atbm_uint32 payload_len,
			      EapCodeType code, atbm_uint8 identifier);
extern const atbm_uint8 * eap_hdr_validate(EapVenType vendor, EapType eap_type,
			    const struct wpabuf *msg, atbm_size_t *plen);
extern atbm_int32 wps_set_attr(struct wps_parse_attr *attr, atbm_uint16 type,
			const atbm_uint8 *pos, atbm_uint16 len);
extern atbm_uint8 eap_get_id(const struct wpabuf *msg);
extern int wps_add_pin(struct hostapd_data *hapd, atbm_void *ctx);

/**
 * wps_build_probe_req_ie - Build WPS IE for Probe Request
 * @pw_id: Password ID (DEV_PW_PUSHBUTTON for active PBC and DEV_PW_DEFAULT for
 * most other use cases)
 * @dev: Device attributes
 * @uuid: Own UUID
 * @req_type: Value for Request Type attribute
 * @num_req_dev_types: Number of requested device types
 * @req_dev_types: Requested device types (8 * num_req_dev_types octets) or
 *	%NULL if none
 * Returns: WPS IE or %NULL on failure
 *
 * The caller is responsible for freeing the buffer.
 */
 struct wpabuf * wps_build_probe_req_ie(atbm_uint16 pw_id, struct wps_device_data *dev,
				       const atbm_uint8 *uuid,
				       enum wps_request_type req_type,
				       atbm_uint32 num_req_dev_types,
				       const atbm_uint8 *req_dev_types)
{
	struct wpabuf *ie;

	wpa_printf(MSG_DEBUG, "WPS: Building WPS IE for Probe Request");

	ie = wpabuf_alloc(500);
	if (ie == NULL)
		return NULL;

	if (wps_build_version(ie) ||
	    wps_build_req_type(ie, req_type) ||
	    wps_build_config_methods(ie, dev->config_methods) ||
	    wps_build_uuid_e(ie, uuid) ||
	    wps_build_primary_dev_type(dev, ie) ||
	    wps_build_rf_bands(dev, ie) ||
	    wps_build_assoc_state(NULL, ie) ||
	    wps_build_config_error(ie, WPS_CFG_NO_ERROR) ||
	    wps_build_dev_password_id(ie, pw_id) ||
#if CONFIG_WPS2
	    wps_build_manufacturer(dev, ie) ||
	    wps_build_model_name(dev, ie) ||
	    wps_build_model_number(dev, ie) ||
	    wps_build_dev_name(dev, ie) ||
	    wps_build_wfa_ext(ie, req_type == WPS_REQ_ENROLLEE, NULL, 0) ||
#endif /* CONFIG_WPS2 */
	    wps_build_req_dev_type(dev, ie, num_req_dev_types, req_dev_types)
	    ||
	    wps_build_secondary_dev_type(dev, ie)
		) {
		wpabuf_free(ie);
		return NULL;
	}

#if (CONFIG_WPS2 == 0)
	if (dev->p2p && wps_build_dev_name(dev, ie)) {
		wpabuf_free(ie);
		return NULL;
	}
#endif /* CONFIG_WPS2 */

	return wps_ie_encapsulate(ie);
}
extern  atbm_void sys_dump_mem(const atbm_void *pMem, int size);
 atbm_uint8 * atbmwifi_ieee80211_add_preq_wps_ie(struct atbmwifi_vif *priv,atbm_uint8 * pos)
{
	enum wps_request_type req_type = WPS_REQ_ENROLLEE_INFO;
	struct wpa_supplicant *wpa_s;
	struct wps_context *wps_ctx;
	//char WIFI_OUI[3] = {0x00, 0x50, 0xF2};
	//char WIFI_WPS_OUI_TYPE = 0x04;
	
	wpa_s = (struct wpa_supplicant *)priv->appdata;
	if( wpa_s->wsc_data == NULL)
		return pos;
	wps_ctx = wpa_s->wsc_data->wps_ctx;
	wpa_printf(MSG_DEBUG,"WPS: atbmwifi_ieee80211_add_preq_wps_ie, wps_mode=%d", wpa_s->wps_mode);	
	if (wpa_s->wps_mode != WPS_MODE_UNKNOWN) {
		struct wpabuf *wps_ie;
		wps_ie = wps_build_probe_req_ie(wpa_s->wps_mode == WPS_MODE_PBC ? DEV_PW_PUSHBUTTON :
						DEV_PW_DEFAULT,
						&wps_ctx->dev,
						wps_ctx->uuid, req_type,
						0, NULL);
		if (wps_ie) {
			//*pos++ = WLAN_EID_VENDOR_SPECIFIC;
			//*pos++ = 4/*OUI + OUI Type*/+wps_ie->used;
			//atbm_memcpy(pos, WIFI_OUI, 3);
			//pos += 3;
			//*pos++ = WIFI_WPS_OUI_TYPE;
			wpa_printf(MSG_DEBUG,"WPS: ie length = %d", wps_ie->used);
			//sys_dump_mem(wps_ie->buf, wps_ie->used);
			
			atbm_memcpy(pos, wps_ie->buf, wps_ie->used);
			pos += wps_ie->used;
			
			wpabuf_free(wps_ie);
		}else{
			wpa_printf(MSG_DEBUG, "WPS: Building WPS IE for Probe Request failed.");	
		}
	}

	return pos;
}
 atbm_uint8 * atbmwifi_ieee80211_add_assocreq_wps_ie(struct atbmwifi_vif *priv,atbm_uint8 * pos)
{
	struct wpa_supplicant *wpa_s;
	struct wpabuf *wps_ie;
	atbm_uint8 wps_oui[] = {0x00, 0x50, 0xF2, 0x04};

	wpa_s = (struct wpa_supplicant *)priv->appdata;

	if (wpa_s->wps_mode == WPS_MODE_UNKNOWN) {
		return pos;// not to do anything
	}

	wps_ie = wpabuf_alloc(100);
	if(wps_ie == NULL){
		wpa_printf(MSG_ERROR, "assoc req add wps ie failed.");
		return pos;
	}

	if (wps_build_version(wps_ie) ||
	    wps_build_req_type(wps_ie, WPS_REQ_ENROLLEE) ||
	    wps_build_wfa_ext(wps_ie, 0, NULL, 0)){
		wpabuf_free(wps_ie);
		return pos;
	}

	//wps_ie_encapsulate(wps_ie);

	//wps ie id
	*pos++ = ATBM_WLAN_EID_VENDOR_SPECIFIC;

	//wps ie length
	*pos++ = wps_ie->used + 4;

	//wps oui
	atbm_memcpy(pos, wps_oui, 4);
	pos += 4;

	atbm_memcpy(pos, wps_ie->buf, wps_ie->used);
	pos += wps_ie->used;

	wpabuf_free(wps_ie);
	
	return pos;
}

 static int wps_set_vendor_ext_wfa_subelem(struct wps_parse_attr *attr,
					  atbm_uint8 id, atbm_uint8 len, const atbm_uint8 *pos)
{
	wpa_printf(MSG_DEBUG, "WPS: WFA subelement id=%u len=%u", id, len);
	switch (id) {
	case WFA_ELEM_VERSION2:
		if (len != 1) {
			wpa_printf(MSG_ERROR, "WPS: Invalid Version2 length %u", len);
			return -1;
		}
		attr->version2 = pos;
		break;
	case WFA_ELEM_AUTHORIZEDMACS:
		attr->authorized_macs = pos;
		attr->authorized_macs_len = len;
		break;
	case WFA_ELEM_NETWORK_KEY_SHAREABLE:
		if (len != 1) {
			wpa_printf(MSG_ERROR, "WPS: Invalid Network Key "
				   "Shareable length %u", len);
			return -1;
		}
		attr->network_key_shareable = pos;
		break;
	case WFA_ELEM_REQUEST_TO_ENROLL:
		if (len != 1) {
			wpa_printf(MSG_ERROR, "WPS: Invalid Request to Enroll "
				   "length %u", len);
			return -1;
		}
		attr->request_to_enroll = pos;
		break;
	case WFA_ELEM_SETTINGS_DELAY_TIME:
		if (len != 1) {
			wpa_printf(MSG_ERROR, "WPS: Invalid Settings Delay "
				   "Time length %u", len);
			return -1;
		}
		attr->settings_delay_time = pos;
		break;
	default:
		wpa_printf(MSG_ERROR, "WPS: Skipped unknown WFA Vendor "
			   "Extension subelement %u", id);
		break;
	}

	return 0;
}


 static int wps_parse_vendor_ext_wfa(struct wps_parse_attr *attr, const atbm_uint8 *pos,
					atbm_uint16 len)
{
	const atbm_uint8 *end = pos + len;
	atbm_uint8 id, elen;

	while (pos + 2 <= end) {
		id = *pos++;
		elen = *pos++;
		if (pos + elen > end)
			break;
		if (wps_set_vendor_ext_wfa_subelem(attr, id, elen, pos) < 0)
			return -1;
		pos += elen;
	}

	return 0;
}


  int wps_parse_vendor_ext(struct wps_parse_attr *attr, const atbm_uint8 *pos,
				atbm_uint16 len)
{
	atbm_uint32 vendor_id;

	if (len < 3) {
		wpa_printf(MSG_ERROR, "WPS: Skip invalid Vendor Extension");
		return 0;
	}

	vendor_id = ATBM_WPA_GET_BE24(pos);
	switch (vendor_id) {
	case WPS_VENDOR_ID_WFA:
		return wps_parse_vendor_ext_wfa(attr, pos + 3, len - 3);
	}

	/* Handle unknown vendor extensions */

	wpa_printf(MSG_DEBUG, "WPS: Unknown Vendor Extension (Vendor ID %u)",
		   vendor_id);

	if (len > WPS_MAX_VENDOR_EXT_LEN) {
		wpa_printf(MSG_ERROR, "WPS: Too long Vendor Extension (%u)",
			   len);
		return -1;
	}

	if (attr->num_vendor_ext >= MAX_WPS_PARSE_VENDOR_EXT) {
		wpa_printf(MSG_ERROR, "WPS: Skipped Vendor Extension "
			   "attribute (max %d vendor extensions)",
			   MAX_WPS_PARSE_VENDOR_EXT);
		return -1;
	}
	attr->vendor_ext[attr->num_vendor_ext] = pos;
	attr->vendor_ext_len[attr->num_vendor_ext] = len;
	attr->num_vendor_ext++;

	return 0;
}

 int wps_parse_msg_attr(const atbm_uint8 *msg, atbm_uint8 length, struct wps_parse_attr *attr)
{
	const atbm_uint8 *pos, *end;
	atbm_uint16 type, len;
#ifdef WPS_WORKAROUNDS
	atbm_uint16 prev_type = 0;
#endif /* WPS_WORKAROUNDS */

	atbm_memset(attr, 0, sizeof(*attr));
	//wpa_printf(MSG_DEBUG,"WPS: parse msg attr.");

	//sys_dump_mem(msg, length);

	end = msg + length;
	pos = msg+4;/* OUI: 3bytes OUI type:1byte*/

	while (pos < end) {
		if (end - pos < 4) {
			wpa_printf(MSG_ERROR, "WPS: Invalid message - "
				   "%lu bytes remaining",
				   (unsigned long) (end - pos));
			return -1;
		}

		type = ATBM_WPA_GET_BE16(pos);
		pos += 2;
		len = ATBM_WPA_GET_BE16(pos);
		pos += 2;
		//wpa_printf(MSG_DEBUG, "WPS: attr type=0x%x len=%u", type, len);
		if (len > end - pos) {
			wpa_printf(MSG_WARNING, "WPS: Attribute overflow");
			//wpa_hexdump_buf(MSG_MSGDUMP, "WPS: Message data", msg);
#ifdef WPS_WORKAROUNDS
			/*
			 * Some deployed APs seem to have a bug in encoding of
			 * Network Key attribute in the Credential attribute
			 * where they add an extra octet after the Network Key
			 * attribute at least when open network is being
			 * provisioned.
			 */
			if ((type & 0xff00) != 0x1000 &&
				prev_type == ATTR_NETWORK_KEY) {
				wpa_printf(MSG_WARNING, "WPS: Workaround - try "
					   "to skip unexpected octet after "
					   "Network Key");
				pos -= 3;
				continue;
			}
#endif /* WPS_WORKAROUNDS */
			return -1;
		}

#ifdef WPS_WORKAROUNDS
		if (type == 0 && len == 0) {
			/*
			 * Mac OS X 10.6 seems to be adding 0x00 padding to the
			 * end of M1. Skip those to avoid interop issues.
			 */
			int i;
			for (i = 0; i < end - pos; i++) {
				if (pos[i])
					break;
			}
			if (i == end - pos) {
				wpa_printf(MSG_WARNING, "WPS: Workaround - skip "
					   "unexpected message padding");
				break;
			}
		}
#endif /* WPS_WORKAROUNDS */

		if (wps_set_attr(attr, type, pos, len) < 0)
			return -1;

#ifdef WPS_WORKAROUNDS
		prev_type = type;
#endif /* WPS_WORKAROUNDS */
		pos += len;
	}

	return 0;
}

 int wps_is_addr_authorized(struct wps_parse_attr *attr, const atbm_uint8 *addr)
 {
	 unsigned int i;
	 const atbm_uint8 *pos;
	 const atbm_uint8 bcast[ATBM_ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
 
	 if (!attr->authorized_macs)
		 return 0;
 
	 pos = attr->authorized_macs;
	 for (i = 0; i < attr->authorized_macs_len / ATBM_ETH_ALEN; i++) {
		 if (atbm_memcmp(pos, addr, ATBM_ETH_ALEN) == 0)
			 return 2;
		 if (atbm_memcmp(pos, bcast, ATBM_ETH_ALEN) == 0)
			 return 1;
		 pos += ATBM_ETH_ALEN;
	 }
 
	 return 0;
 }

#define WPS_PIN_SCAN_IGNORE_SEL_REG 3
#define WPS_PIN_TIME_IGNORE_SEL_REG 10
int check_valid_wps_ap(struct atbmwifi_vif *priv, struct wps_parse_attr *attr, atbm_uint8 *addr){
	//struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;

	if(attr->selected_registrar){
		if(*(attr->selected_registrar) == 0)
			return 0;
		else if(attr->primary_dev_type != ATBM_NULL)
			return 1;
	}
	if(priv->pin){
		if(wps_is_addr_authorized(attr, addr)){
			return 1;
		}
		/*for wps Interoperability Test(5.4.5) wsc 1.0 ap with external registar*/ 
#if 0
		if(wpa_s->scan_runs >= WPS_PIN_SCAN_IGNORE_SEL_REG ||
			((atbm_GetOsTime() - wpa_s->wps_pin_start_time >= WPS_PIN_TIME_IGNORE_SEL_REG * HZ)){
			return 1;
		}
#endif
	}
	return 0;
}

 static const char * eap_wsc_state_txt(int state)
{
	switch (state) {
	case WAIT_START:
		return "WAIT_START";
	case MESG:
		return "MESG";
	case FRAG_ACK:
		return "FRAG_ACK";
	case WAIT_FRAG_ACK:
		return "WAIT_FRAG_ACK";
	case ATBM_WPS_DONE:
		return "ATBM_WPS_DONE";
	case ATBM_WPS_FAIL:
		return "ATBM_WPS_FAIL";
	default:
		return "?";
	}
}

 atbm_void eap_wsc_state(struct eap_wsc_data *data, int state)
{
	wpa_printf(WIFI_WPA, "EAP-WSC State: %s -> %s",
		   eap_wsc_state_txt(data->state),
		   eap_wsc_state_txt(state));
	data->state = state;
	return;
}

 static int wpa_supplicant_ap_wps_pin(struct hostapd_data *hapd, const atbm_uint8 *bssid,
			      const char *pin, char *buf, atbm_size_t buflen)
{
	return 0;
}


/**
 * wps_get_msg - Build a WPS message
 * @wps: WPS Registration protocol data from wps_init()
 * @op_code: Buffer for returning message OP Code
 * Returns: The generated WPS message or %NULL on failure
 *
 * This function is used to build a response to a message processed by calling
 * wps_process_msg(). The caller is responsible for freeing the buffer.
 */
 struct wpabuf * wps_get_msg(struct wps_data *wps, enum wsc_op_code *op_code)
{
	if (wps->registrar)
		return wps_registrar_get_msg(wps, op_code);
	else
		return wps_enrollee_get_msg(wps, op_code);
}

/**
 * wps_process_msg - Process a WPS message
 * @wps: WPS Registration protocol data from wps_init()
 * @op_code: Message OP Code
 * @msg: Message data
 * Returns: Processing result
 *
 * This function is used to process WPS messages with OP Codes WSC_ACK,
 * WSC_NACK, WSC_MSG, and WSC_Done. The caller (e.g., EAP server/peer) is
 * responsible for reassembling the messages before calling this function.
 * Response to this message is built by calling wps_get_msg().
 */
 enum wps_process_res wps_process_msg(struct wps_data *wps,
				     enum wsc_op_code op_code,
				     const struct wpabuf *msg)
{
	if (wps->registrar)
		return wps_registrar_process_msg(wps, op_code, msg);
	else
		return wps_enrollee_process_msg(wps, op_code, msg);
}

 struct wpabuf * eap_wsc_build_frag_ack(atbm_uint8 id, atbm_uint8 code)
{
	struct wpabuf *msg;

	msg = eap_msg_alloc(ATBM_EAP_VENDOR_WFA, EAP_VENDOR_TYPE_WSC, 2, code, id);
	if (msg == NULL) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Failed to allocate memory for FRAG_ACK");
		return NULL;
	}

	wpa_printf(MSG_DEBUG, "EAP-WSC: Send WSC/FRAG_ACK");
	wpabuf_put_u8(msg, WSC_FRAG_ACK); /* Op-Code */
	wpabuf_put_u8(msg, 0); /* Flags */

	return msg;
}

 static struct wpabuf * eap_wsc_process_fragment(struct eap_wsc_data *data,
						atbm_uint8 id, atbm_uint8 flags, atbm_uint8 op_code,
						atbm_uint16 message_length,
						const atbm_uint8 *buf, atbm_size_t len)
{
	/* Process a fragment that is not the last one of the message */
	if (data->in_buf == NULL && !(flags & WSC_FLAGS_LF)) {
		wpa_printf(MSG_ERROR,"EAP-WSC: No Message Length field in a fragmented packet");
		return NULL;
	}

	if (data->in_buf == NULL) {
		/* First fragment of the message */
		data->in_buf = wpabuf_alloc(message_length);
		if (data->in_buf == NULL) {
			wpa_printf(MSG_ERROR,"EAP-WSC: No memory for message");
			return NULL;
		}
		data->in_op_code = op_code;
		wpabuf_put_data(data->in_buf, buf, len);
		wpa_printf(MSG_ERROR,"EAP-WSC: Received %lu bytes in first "
			   "fragment, waiting for %lu bytes more",
			   (unsigned long) len,
			   (unsigned long) wpabuf_tailroom(data->in_buf));
	}

	return eap_wsc_build_frag_ack(id, EAP_CODE_RESPONSE);
}

 static int eap_wsc_process_cont(struct eap_wsc_data *data,
				const atbm_uint8 *buf, atbm_size_t len, atbm_uint8 op_code)
{
	/* Process continuation of a pending message */
	if (op_code != data->in_op_code) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Unexpected Op-Code %d in "
			   "fragment (expected %d)",
			   op_code, data->in_op_code);
		return -1;
	}

	if (len > wpabuf_tailroom(data->in_buf)) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Fragment overflow");
		eap_wsc_state(data, ATBM_WPS_FAIL);
		return -1;
	}

	wpabuf_put_data(data->in_buf, buf, len);
	wpa_printf(MSG_DEBUG, "EAP-WSC: Received %lu bytes, waiting "
		   "for %lu bytes more", (unsigned long) len,
		   (unsigned long) wpabuf_tailroom(data->in_buf));

	return 0;
}
 static struct wpabuf * eap_wsc_build_msg(struct eap_wsc_data *data,atbm_uint8 id)
{
	struct wpabuf *resp;
	atbm_uint8 flags;
	atbm_size_t send_len, plen;

	wpa_printf(MSG_DEBUG, "EAP-WSC: Generating Response");

	flags = 0;
	send_len = wpabuf_len(data->out_buf) - data->out_used;
	if (2 + send_len > data->fragment_size) {
		send_len = data->fragment_size - 2;
		flags |= WSC_FLAGS_MF;
		if (data->out_used == 0) {
			flags |= WSC_FLAGS_LF;
			send_len -= 2;
		}
	}
	plen = 2 + send_len;
	if (flags & WSC_FLAGS_LF)
		plen += 2;
	resp = eap_msg_alloc(ATBM_EAP_VENDOR_WFA, EAP_VENDOR_TYPE_WSC, plen,
			     EAP_CODE_RESPONSE, id);
	if (resp == NULL)
		return NULL;

	wpabuf_put_u8(resp, data->out_op_code); /* Op-Code */
	wpabuf_put_u8(resp, flags); /* Flags */
	if (flags & WSC_FLAGS_LF)
		wpabuf_put_be16(resp, wpabuf_len(data->out_buf));

	wpabuf_put_data(resp, wpabuf_head_u8(data->out_buf) + data->out_used,
			send_len);
	data->out_used += send_len;

	if (data->out_used == wpabuf_len(data->out_buf)) {
		wpa_printf(MSG_DEBUG, "EAP-WSC: Sending out %lu bytes "
			   "(message sent completely)",
			   (unsigned long) send_len);
		wpabuf_free(data->out_buf);
		data->out_buf = NULL;
		data->out_used = 0;
		if ((data->state == ATBM_WPS_FAIL && data->out_op_code == WSC_ACK) ||
		    data->out_op_code == WSC_NACK ||
		    data->out_op_code == WSC_Done) {
		    wpa_printf(MSG_ERROR, "EAP-WSC: building msg failed.");
			eap_wsc_state(data, ATBM_WPS_FAIL);
		} else
			eap_wsc_state(data, MESG);
	} else {
		wpa_printf(MSG_DEBUG, "EAP-WSC: Sending out %lu bytes "
			   "(%lu more to send)", (unsigned long) send_len,
			   (unsigned long) wpabuf_len(data->out_buf) -
			   data->out_used);
		eap_wsc_state(data, WAIT_FRAG_ACK);
	}

	return resp;
}

 struct wpabuf * wpas_eap_wsc_process(struct wpa_supplicant *wpa_s, const struct wpabuf *reqData)
{
	struct eap_wsc_data *data;
	const atbm_uint8 *start, *pos, *end;
	atbm_size_t len;
	atbm_uint8 op_code, flags, id;
	atbm_uint16 message_length = 0;
	enum wps_process_res res;
	struct wpabuf tmpbuf;
	struct wpabuf *r;
	

	data = wpa_s->wsc_data;

	pos = eap_hdr_validate(ATBM_EAP_VENDOR_WFA, EAP_VENDOR_TYPE_WSC, reqData,&len);
	if (pos == NULL || len < 2) {
		return NULL;
	}

	id = eap_get_id(reqData);

	start = pos;
	end = start + len;

	op_code = *pos++;
	flags = *pos++;
	if (flags & WSC_FLAGS_LF) {
		if (end - pos < 2) {
			wpa_printf(MSG_ERROR, "EAP-WSC: Message underflow");
			return NULL;
		}
		message_length = ATBM_WPA_GET_BE16(pos);
		pos += 2;

		if (message_length < end - pos) {
			wpa_printf(MSG_ERROR, "EAP-WSC: Invalid Message Length");
			return NULL;
		}
	}

	wpa_printf(MSG_DEBUG, "EAP-WSC: Received packet: Op-Code %d "
		   "Flags 0x%x Message Length %d",
		   op_code, flags, message_length);

	if (data->state == WAIT_FRAG_ACK) {
		if (op_code != WSC_FRAG_ACK) {
			wpa_printf(MSG_ERROR, "EAP-WSC: Unexpected Op-Code %d "
				   "in WAIT_FRAG_ACK state", op_code);
			return NULL;
		}
		wpa_printf(MSG_DEBUG, "EAP-WSC: Fragment acknowledged");
		eap_wsc_state(data, MESG);
		return eap_wsc_build_msg(data, id);
	}

	if (op_code != WSC_ACK && op_code != WSC_NACK && op_code != WSC_MSG &&
		op_code != WSC_Done && op_code != WSC_Start) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Unexpected Op-Code %d",
			   op_code);
		return NULL;
	}

	if (data->state == WAIT_START) {
		if (op_code != WSC_Start) {
			wpa_printf(MSG_ERROR, "EAP-WSC: Unexpected Op-Code %d "
				   "in WAIT_START state", op_code);
			return NULL;
		}
		wpa_printf(MSG_DEBUG, "EAP-WSC: Received start");
		eap_wsc_state(data, MESG);
		/* Start message has empty payload, skip processing */
		goto send_msg;
	} else if (op_code == WSC_Start) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Unexpected Op-Code %d",
			   op_code);
		return NULL;
	}

	if (data->in_buf &&
		eap_wsc_process_cont(data, pos, end - pos, op_code) < 0) {
		return NULL;
	}

	if (flags & WSC_FLAGS_MF) {
		return eap_wsc_process_fragment(data, id, flags, op_code,
						message_length, pos,
						end - pos);
	}

	if (data->in_buf == NULL) {
		/* Wrap unfragmented messages as wpabuf without extra copy */
		wpabuf_set(&tmpbuf, pos, end - pos);
		data->in_buf = &tmpbuf;
	}

	res = wps_process_msg(data->wps, op_code, data->in_buf);
	switch (res) {
	case WPS_DONE:
		wpa_printf(MSG_ALWAYS, "EAP-WSC: WPS processing completed "
			   "successfully - wait for EAP failure");
		eap_wsc_state(data, ATBM_WPS_FAIL);
		break;
	case WPS_CONTINUE:
		eap_wsc_state(data, MESG);
		break;
	case WPS_FAILURE:
	case WPS_PENDING:
		wpa_printf(MSG_ALWAYS, "EAP-WSC: WPS processing failed");
		eap_wsc_state(data, ATBM_WPS_FAIL);
		break;
	}

	if (data->in_buf != &tmpbuf)
		wpabuf_free(data->in_buf);
	data->in_buf = NULL;

send_msg:
	if (data->out_buf == NULL) {
		data->out_buf = wps_get_msg(data->wps, &data->out_op_code);
		if (data->out_buf == NULL) {
			wpa_printf(MSG_ERROR, "EAP-WSC: Failed to receive "
				   "message from WPS");
			return NULL;
		}
		data->out_used = 0;
	}

	eap_wsc_state(data, MESG);
	r = eap_wsc_build_msg(data, id);
	if (data->state == ATBM_WPS_FAIL) {
		/* Use reduced client timeout for WPS to avoid long wait */
	}
	return r;
}

/**
 * atbmwps_start_pbc - start wps via pbc
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at user start wps via pbc.
 */
 int atbmwps_start_pbc(struct atbmwifi_vif *priv, atbm_uint8 *p2p_info)
{
	int ret = 0;
	int p2p_group=0;

	if(priv == NULL){
		wpa_printf(MSG_ERROR, "wps: pbc mode, priv is not finish init.");
		return -1;
	}

	switch(priv->iftype){

		case ATBM_NL80211_IFTYPE_AP:
			priv->config.dev_password_len = 0; 
			priv->pbc = 1;
			priv->pin = 0;
			ret = hostapd_wps_button_pushed(priv->appdata, NULL); 

			break;
		case ATBM_NL80211_IFTYPE_STATION:
			if(p2p_info){
				atbm_memcpy(&p2p_group, p2p_info, sizeof(int));
			}
			ret = wpas_wps_start_pbc(priv->appdata, NULL, p2p_group);

			break;
		default:
			wpa_printf(MSG_ERROR, "wps: pbc mode, iftype is not support.");
			ret = -1;
			break;
	}

	return ret;
}

/**
 * atbmwps_start_pin - start wps via pin
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at user start wps via pin.
 */
 int atbmwps_start_pin(struct atbmwifi_vif *priv, const char *pin, atbm_uint8* buf, atbm_uint32 info)
{
	int ret = 0;
	int p2p_group=0;
	atbm_uint16 dev_pw_id;

	if(priv == NULL){
		wpa_printf(MSG_ERROR, "wps: pin mode, priv is not finish init.");
		return -1;
	}

	if(pin == NULL){
		wpa_printf(MSG_ERROR, "wps: pin mode, pin code is NULL.");
		return -1;		
	}
	
	switch(priv->iftype){
		
		case ATBM_NL80211_IFTYPE_AP:
			ret = wpa_supplicant_ap_wps_pin(priv->appdata, priv->bssid, pin, (char *)buf, (atbm_size_t)info/*buf length*/);
			if(ret != 0)
				break;		

			priv->config.dev_password_len = 8;//strlen(pin);	
			atbm_memcpy(priv->config.dev_password, pin, 8);	
			priv->pin = 1;
			priv->pbc = 0;
			ret = wps_add_pin((struct hostapd_data *)priv->appdata, NULL);

			break;
		case ATBM_NL80211_IFTYPE_STATION:
			if(buf){
				atbm_memcpy(&p2p_group, buf, sizeof(int));
			}
			dev_pw_id = info;
			ret = wpas_wps_start_pin(priv->appdata, priv->bssid, pin, p2p_group, dev_pw_id);

			break;
		default:
			wpa_printf(MSG_ERROR, "wps: pin mode, iftype is not support.");
			ret = -1;
			break;
	}

	return ret;
}

/**
 * atbmwps_cancel - Cancel the wps pbc/pin requests
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at the wps pbc/pin requests is cancelled.
 */
 int atbmwps_cancel(struct atbmwifi_vif *priv)
{
	int ret = 0;
	
	if(priv == NULL){
		wpa_printf(MSG_ERROR, "wps: cancel, priv is not finish init.");
		return -1;
	}
	
	if(atbmwifi_is_ap_mode(priv->iftype)){
		hostapd_cancel_wps(priv->appdata);
	}else if(atbmwifi_is_sta_mode(priv->iftype)){
		wpas_cancel_wps(priv->appdata);
	}else{
		wpa_printf(MSG_ERROR, "wps: cancel, iftype is not support.");
		ret = -1;
	}
	
	return ret;
}

#if CONFIG_P2P
/**
 * atbmwps_deinit - destrory wps structure
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at program over run to destroy wps.
 */
int atbmwps_deinit(struct atbmwifi_vif *priv)
{
	int ret = 0;

	if(priv == NULL){
		wpa_printf(MSG_ERROR, "wps: deinit, priv is not finish init.");
		return -1;
	}
	
	if(atbmwifi_is_ap_mode(priv->iftype)){
		hostapd_deinit_wps(priv->appdata);
	}else if(atbmwifi_is_sta_mode(priv->iftype)){
		wpas_deinit_wps(priv->appdata);
	}else{
		wpa_printf(MSG_ERROR, "wps: deinit, iftype is not support.");
		ret = -1;
	}

	return ret;
}

/**
 * atbmwps_init - init wps STA or AP
 * Returns: 0 on success, -1 on failure
 *
 * This function is called at program initialization to enable wps for STA/AP.
 */
int atbmwps_init(struct atbmwifi_vif *priv)
{
	int ret = 0;

	if(priv == NULL){
		wpa_printf(MSG_ERROR, "WPS: init, priv is not finish init.");
		return -1;
	}

	if(atbmwifi_is_ap_mode(priv->iftype)){
		ret = hostapd_init_wps(priv->appdata);
	}else if(atbmwifi_is_sta_mode(priv->iftype)){
		ret = wpas_init_wps(priv->appdata);
	}else{
		wpa_printf(MSG_ERROR, "WPS: init, iftype is not support.");
		ret = -1;
	}

	if(ret == 0)
		wpa_printf(MSG_DEBUG, "WPS: init OK.");
	else
		wpa_printf(MSG_DEBUG, "WPS: init failed.");

	return ret;
}
#endif
