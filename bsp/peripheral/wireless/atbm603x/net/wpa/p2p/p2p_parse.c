/*
 * P2P - IE parser
 * Copyright (c) 2009-2010, Atheros Communications
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "p2p_common.h"
#include "p2p_defs.h"
#include "p2p_list.h"

#include "wps_i.h"
#include "p2p_i.h"
#include "atbm_hal.h"

#if CONFIG_P2P

void p2p_copy_filter_devname(char *dst, atbm_size_t dst_len,
			     const void *src, atbm_size_t src_len)
{
	atbm_size_t i;

	if (src_len >= dst_len)
		src_len = dst_len - 1;
	atbm_memcpy(dst, src, src_len);
	dst[src_len] = '\0';
	for (i = 0; i < src_len; i++) {
		if (dst[i] == '\0')
			break;
		if (is_ctrl_char(dst[i]))
			dst[i] = '_';
	}
}


static int p2p_parse_attribute(atbm_uint8 id, const atbm_uint8 *data, atbm_uint16 len,
			       struct p2p_message *msg)
{
	const atbm_uint8 *pos;
	atbm_uint16 nlen;
	char devtype[WPS_DEV_TYPE_BUFSIZE];

	switch (id) {
	case P2P_ATTR_CAPABILITY:
		if (len < 2) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Capability "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->capability = data;
		p2p_printf(MSG_DEBUG, "P2P: * Device Capability %02x "
			   "Group Capability %02x",
			   data[0], data[1]);
		break;
	case P2P_ATTR_DEVICE_ID:
		if (len < ATBM_ETH_ALEN) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Device ID "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->device_id = data;
		p2p_printf(MSG_DEBUG, "P2P: * Device ID " MACSTR,
			   MAC2STR(msg->device_id));
		break;
	case P2P_ATTR_GROUP_OWNER_INTENT:
		if (len < 1) {
			p2p_printf(MSG_DEBUG, "P2P: Too short GO Intent "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->go_intent = data;
		p2p_printf(MSG_DEBUG, "P2P: * GO Intent: Intent %u "
			   "Tie breaker %u", data[0] >> 1, data[0] & 0x01);
		break;
	case P2P_ATTR_STATUS:
		if (len < 1) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Status "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->status = data;
		p2p_printf(MSG_DEBUG, "P2P: * Status: %d", data[0]);
		break;
	case P2P_ATTR_LISTEN_CHANNEL:
		if (len == 0) {
			p2p_printf(MSG_DEBUG, "P2P: * Listen Channel: Ignore "
				   "null channel");
			break;
		}
		if (len < 5) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Listen Channel "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->listen_channel = data;
		p2p_printf(MSG_DEBUG, "P2P: * Listen Channel: "
			   "Country %c%c(0x%02x) Regulatory "
			   "Class %d Channel Number %d", data[0], data[1],
			   data[2], data[3], data[4]);
		break;
	case P2P_ATTR_OPERATING_CHANNEL:
		if (len == 0) {
			p2p_printf(MSG_DEBUG, "P2P: * Operating Channel: "
				   "Ignore null channel");
			break;
		}
		if (len < 5) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Operating "
				   "Channel attribute (length %d)", len);
			return -1;
		}
		msg->operating_channel = data;
		p2p_printf(MSG_DEBUG, "P2P: * Operating Channel: "
			   "Country %c%c(0x%02x) Regulatory "
			   "Class %d Channel Number %d", data[0], data[1],
			   data[2], data[3], data[4]);
		break;
	case P2P_ATTR_CHANNEL_LIST:
		if (len < 3) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Channel List "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->channel_list = data;
		msg->channel_list_len = len;
		p2p_printf(MSG_DEBUG, "P2P: * Channel List: Country String "
			   "'%c%c(0x%02x)'", data[0], data[1], data[2]);
		p2p_hexdump(MSG_MSGDUMP, "P2P: Channel List",
			    msg->channel_list, msg->channel_list_len);
		break;
	case P2P_ATTR_GROUP_INFO:
		msg->group_info = data;
		msg->group_info_len = len;
		p2p_printf(MSG_DEBUG, "P2P: * Group Info");
		break;
	case P2P_ATTR_DEVICE_INFO:
		if (len < ATBM_ETH_ALEN + 2 + 8 + 1) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Device Info "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->p2p_device_info = data;
		msg->p2p_device_info_len = len;
		pos = data;
		msg->p2p_device_addr = pos;
		pos += ATBM_ETH_ALEN;
		msg->config_methods = ATBM_WPA_GET_BE16(pos);
		pos += 2;
		msg->pri_dev_type = pos;
		pos += 8;
		msg->num_sec_dev_types = *pos++;
		if (msg->num_sec_dev_types * 8 > data + len - pos) {
			p2p_printf(MSG_DEBUG, "P2P: Device Info underflow");
			return -1;
		}
		pos += msg->num_sec_dev_types * 8;
		if (data + len - pos < 4) {
			p2p_printf(MSG_DEBUG, "P2P: Invalid Device Name "
				   "length %d", (int) (data + len - pos));
			return -1;
		}
		if (ATBM_WPA_GET_BE16(pos) != ATTR_DEV_NAME) {
			p2p_hexdump(MSG_DEBUG, "P2P: Unexpected Device Name "
				    "header", pos, 4);
			return -1;
		}
		pos += 2;
		nlen = ATBM_WPA_GET_BE16(pos);
		pos += 2;
		if (nlen > data + len - pos || nlen > WPS_DEV_NAME_MAX_LEN) {
			p2p_printf(MSG_DEBUG, "P2P: Invalid Device Name "
				   "length %u (buf len %d)", nlen,
				   (int) (data + len - pos));
			return -1;
		}
		p2p_copy_filter_devname(msg->device_name,
					sizeof(msg->device_name), pos, nlen);
		p2p_printf(MSG_DEBUG, "P2P: * Device Info: addr " MACSTR
			   " primary device type %s device name '%s' "
			   "config methods 0x%x",
			   MAC2STR(msg->p2p_device_addr),
			   wps_dev_type_bin2str(msg->pri_dev_type, devtype,
						sizeof(devtype)),
			   msg->device_name, msg->config_methods);
		break;
	case P2P_ATTR_CONFIGURATION_TIMEOUT:
		if (len < 2) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Configuration "
				   "Timeout attribute (length %d)", len);
			return -1;
		}
		msg->config_timeout = data;
		p2p_printf(MSG_DEBUG, "P2P: * Configuration Timeout");
		break;
	case P2P_ATTR_INTENDED_INTERFACE_ADDR:
		if (len < ATBM_ETH_ALEN) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Intended P2P "
				   "Interface Address attribute (length %d)",
				   len);
			return -1;
		}
		msg->intended_addr = data;
		p2p_printf(MSG_DEBUG, "P2P: * Intended P2P Interface Address: "
			   MACSTR, MAC2STR(msg->intended_addr));
		break;
	case P2P_ATTR_GROUP_BSSID:
		if (len < ATBM_ETH_ALEN) {
			p2p_printf(MSG_DEBUG, "P2P: Too short P2P Group BSSID "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->group_bssid = data;
		p2p_printf(MSG_DEBUG, "P2P: * P2P Group BSSID: " MACSTR,
			   MAC2STR(msg->group_bssid));
		break;
	case P2P_ATTR_GROUP_ID:
		if (len < ATBM_ETH_ALEN || len > ATBM_ETH_ALEN + SSID_MAX_LEN) {
			p2p_printf(MSG_DEBUG, "P2P: Invalid P2P Group ID "
				   "attribute length %d", len);
			return -1;
		}
		msg->group_id = data;
		msg->group_id_len = len;
		p2p_printf(MSG_DEBUG, "P2P: * P2P Group ID: Device Address "
			   MACSTR, MAC2STR(msg->group_id));
		p2p_hexdump_ascii(MSG_DEBUG, "P2P: * P2P Group ID: SSID",
				  msg->group_id + ATBM_ETH_ALEN,
				  msg->group_id_len - ATBM_ETH_ALEN);
		break;
	case P2P_ATTR_INVITATION_FLAGS:
		if (len < 1) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Invitation "
				   "Flag attribute (length %d)", len);
			return -1;
		}
		msg->invitation_flags = data;
		p2p_printf(MSG_DEBUG, "P2P: * Invitation Flags: bitmap 0x%x",
			   data[0]);
		break;
	case P2P_ATTR_MANAGEABILITY:
		if (len < 1) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Manageability "
				   "attribute (length %d)", len);
			return -1;
		}
		msg->manageability = data;
		p2p_printf(MSG_DEBUG, "P2P: * Manageability: bitmap 0x%x",
			   data[0]);
		break;
	case P2P_ATTR_NOTICE_OF_ABSENCE:
		if (len < 2) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Notice of "
				   "Absence attribute (length %d)", len);
			return -1;
		}
		msg->noa = data;
		msg->noa_len = len;
		p2p_printf(MSG_DEBUG, "P2P: * Notice of Absence");
		break;
	case P2P_ATTR_EXT_LISTEN_TIMING:
		if (len < 4) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Extended Listen "
				   "Timing attribute (length %d)", len);
			return -1;
		}
		msg->ext_listen_timing = data;
		p2p_printf(MSG_DEBUG, "P2P: * Extended Listen Timing "
			   "(period %u msec  interval %u msec)",
			   ATBM_WPA_GET_LE16(msg->ext_listen_timing),
			   ATBM_WPA_GET_LE16(msg->ext_listen_timing + 2));
		break;
	case P2P_ATTR_MINOR_REASON_CODE:
		if (len < 1) {
			p2p_printf(MSG_DEBUG, "P2P: Too short Minor Reason "
				   "Code attribute (length %d)", len);
			return -1;
		}
		msg->minor_reason_code = data;
		p2p_printf(MSG_DEBUG, "P2P: * Minor Reason Code: %u",
			   *msg->minor_reason_code);
		break;
	case P2P_ATTR_OOB_GO_NEG_CHANNEL:
		if (len < 6) {
			p2p_printf(MSG_DEBUG, "P2P: Too short OOB GO Neg "
				   "Channel attribute (length %d)", len);
			return -1;
		}
		msg->oob_go_neg_channel = data;
		p2p_printf(MSG_DEBUG, "P2P: * OOB GO Neg Channel: "
			   "Country %c%c(0x%02x) Operating Class %d "
			   "Channel Number %d Role %d",
			   data[0], data[1], data[2], data[3], data[4],
			   data[5]);
		break;
	case P2P_ATTR_SERVICE_HASH:
		if (len < P2PS_HASH_LEN) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Too short Service Hash (length %u)",
				   len);
			return -1;
		}
		msg->service_hash_count = len / P2PS_HASH_LEN;
		msg->service_hash = data;
		p2p_hexdump(MSG_DEBUG, "P2P: * Service Hash(s)", data, len);
		break;
	case P2P_ATTR_SESSION_INFORMATION_DATA:
		msg->session_info = data;
		msg->session_info_len = len;
		p2p_printf(MSG_DEBUG, "P2P: * Service Instance: %u bytes - %p",
			   len, data);
		break;
	case P2P_ATTR_CONNECTION_CAPABILITY:
		if (len < 1) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Too short Connection Capability (length %u)",
				   len);
			return -1;
		}
		msg->conn_cap = data;
		p2p_printf(MSG_DEBUG, "P2P: * Connection Capability: 0x%x",
			   *msg->conn_cap);
		break;
	case P2P_ATTR_ADVERTISEMENT_ID:
		if (len < 10) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Too short Advertisement ID (length %u)",
				   len);
			return -1;
		}
		msg->adv_id = data;
		msg->adv_mac = &data[sizeof(atbm_uint32)];
		p2p_printf(MSG_DEBUG, "P2P: * Advertisement ID %x",
			   ATBM_WPA_GET_LE32(data));
		break;
	case P2P_ATTR_ADVERTISED_SERVICE:
		if (len < 8) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Too short Service Instance (length %u)",
				   len);
			return -1;
		}
		msg->adv_service_instance = data;
		msg->adv_service_instance_len = len;
		if (len <= 255 + 8) {
			char str[256];
			atbm_uint8 namelen;

			namelen = data[6];
			if (namelen > len - 7)
				break;
			atbm_memcpy(str, &data[7], namelen);
			str[namelen] = '\0';
			p2p_printf(MSG_DEBUG, "P2P: * Service Instance: %x-%s",
				   ATBM_WPA_GET_LE32(data), str);
		} else {
			p2p_printf(MSG_DEBUG, "P2P: * Service Instance: %p",
				   data);
		}
		break;
	case P2P_ATTR_SESSION_ID:
		if (len < sizeof(atbm_uint32) + ATBM_ETH_ALEN) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Too short Session ID Info (length %u)",
				   len);
			return -1;
		}
		msg->session_id = data;
		msg->session_mac = &data[sizeof(atbm_uint32)];
		p2p_printf(MSG_DEBUG, "P2P: * Session ID: %x " MACSTR,
			   ATBM_WPA_GET_LE32(data), MAC2STR(msg->session_mac));
		break;
	case P2P_ATTR_FEATURE_CAPABILITY:
		if (!len) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Too short Feature Capability (length %u)",
				   len);
			return -1;
		}
		msg->feature_cap = data;
		msg->feature_cap_len = len;
		p2p_printf(MSG_DEBUG, "P2P: * Feature Cap (length=%u)", len);
		break;
	case P2P_ATTR_PERSISTENT_GROUP:
	{
		if (len < ATBM_ETH_ALEN || len > ATBM_ETH_ALEN + SSID_MAX_LEN) {
			p2p_printf(MSG_DEBUG,
				   "P2P: Invalid Persistent Group Info (length %u)",
				   len);
			return -1;
		}

		msg->persistent_dev = data;
		msg->persistent_ssid_len = len - ATBM_ETH_ALEN;
		msg->persistent_ssid = &data[ATBM_ETH_ALEN];
		p2p_printf(MSG_DEBUG, "P2P: * Persistent Group: " MACSTR " %s",
			   MAC2STR(msg->persistent_dev),
			   wpa_ssid_txt(msg->persistent_ssid,
					msg->persistent_ssid_len));
		break;
	}
	default:
		p2p_printf(MSG_DEBUG, "P2P: Skipped unknown attribute %d "
			   "(length %d)", id, len);
		break;
	}

	return 0;
}


/**
 * p2p_parse_p2p_ie - Parse P2P IE
 * @buf: Concatenated P2P IE(s) payload
 * @msg: Buffer for returning parsed attributes
 * Returns: 0 on success, -1 on failure
 *
 * Note: Caller is responsible for clearing the msg data structure before
 * calling this function.
 */
int p2p_parse_p2p_ie(const struct wpabuf *buf, struct p2p_message *msg)
{
	const atbm_uint8 *pos = wpabuf_head_u8(buf);
	const atbm_uint8 *end = pos + wpabuf_len(buf);

	p2p_printf(MSG_DEBUG, "P2P: Parsing P2P IE");

	while (pos < end) {
		atbm_uint16 attr_len;
		atbm_uint8 id;

		if (end - pos < 3) {
			p2p_printf(MSG_DEBUG, "P2P: Invalid P2P attribute");
			return -1;
		}
		id = *pos++;
		attr_len = ATBM_WPA_GET_LE16(pos);
		pos += 2;
		p2p_printf(MSG_DEBUG, "P2P: Attribute %d length %u",
			   id, attr_len);
		if (attr_len > end - pos) {
			p2p_printf(MSG_DEBUG, "P2P: Attribute underflow "
				   "(len=%u left=%d)",
				   attr_len, (int) (end - pos));
			p2p_hexdump(MSG_MSGDUMP, "P2P: Data", pos, end - pos);
			return -1;
		}
		if (p2p_parse_attribute(id, pos, attr_len, msg))
			return -1;
		pos += attr_len;
	}

	return 0;
}


static int p2p_parse_wps_ie(const struct wpabuf *buf, struct p2p_message *msg)
{
	struct wps_parse_attr attr;
	int i;

	p2p_printf(MSG_DEBUG, "P2P: Parsing WPS IE");
	if (wps_parse_msg(buf, &attr))
		return -1;
	if (attr.dev_name && attr.dev_name_len < sizeof(msg->device_name) &&
	    !msg->device_name[0])
		atbm_memcpy(msg->device_name, attr.dev_name, attr.dev_name_len);
	if (attr.config_methods) {
		msg->wps_config_methods =
			ATBM_WPA_GET_BE16(attr.config_methods);
		p2p_printf(MSG_DEBUG, "P2P: Config Methods (WPS): 0x%x",
			   msg->wps_config_methods);
	}
	if (attr.dev_password_id) {
		msg->dev_password_id = ATBM_WPA_GET_BE16(attr.dev_password_id);
		p2p_printf(MSG_DEBUG, "P2P: Device Password ID: %d",
			   msg->dev_password_id);
		msg->dev_password_id_present = 1;
	}
	if (attr.primary_dev_type) {
		char devtype[WPS_DEV_TYPE_BUFSIZE];
		msg->wps_pri_dev_type = attr.primary_dev_type;
		p2p_printf(MSG_DEBUG, "P2P: Primary Device Type (WPS): %s",
			   wps_dev_type_bin2str(msg->wps_pri_dev_type, devtype,
						sizeof(devtype)));
	}
	if (attr.sec_dev_type_list) {
		msg->wps_sec_dev_type_list = attr.sec_dev_type_list;
		msg->wps_sec_dev_type_list_len = attr.sec_dev_type_list_len;
	}

	for (i = 0; i < P2P_MAX_WPS_VENDOR_EXT; i++) {
		msg->wps_vendor_ext[i] = attr.vendor_ext[i];
		msg->wps_vendor_ext_len[i] = attr.vendor_ext_len[i];
	}

	msg->manufacturer = attr.manufacturer;
	msg->manufacturer_len = attr.manufacturer_len;
	msg->model_name = attr.model_name;
	msg->model_name_len = attr.model_name_len;
	msg->model_number = attr.model_number;
	msg->model_number_len = attr.model_number_len;
	msg->serial_number = attr.serial_number;
	msg->serial_number_len = attr.serial_number_len;

	msg->oob_dev_password = attr.oob_dev_password;
	msg->oob_dev_password_len = attr.oob_dev_password_len;

	return 0;
}


/**
 * p2p_parse_ies - Parse P2P message IEs (both WPS and P2P IE)
 * @data: IEs from the message
 * @len: Length of data buffer in octets
 * @msg: Buffer for returning parsed attributes
 * Returns: 0 on success, -1 on failure
 *
 * Note: Caller is responsible for clearing the msg data structure before
 * calling this function.
 *
 * Note: Caller must free temporary memory allocations by calling
 * p2p_parse_free() when the parsed data is not needed anymore.
 */
int p2p_parse_ies(const atbm_uint8 *data, atbm_size_t len, struct p2p_message *msg)
{
	struct atbmwifi_ieee802_11_elems elems;

	atbm_ieee802_11_parse_elems((atbm_uint8 *)data, len, &elems);
	if (elems.ds_params)
		msg->ds_params = elems.ds_params;
	if (elems.ssid)
		msg->ssid = elems.ssid - 2;

	msg->wps_attributes = ieee802_11_vendor_ie_concat(data, len,
							  WPS_DEV_OUI_WFA);
	if (msg->wps_attributes &&
	    p2p_parse_wps_ie(msg->wps_attributes, msg)) {
		p2p_parse_free(msg);
		return -1;
	}

	msg->p2p_attributes = ieee802_11_vendor_ie_concat(data, len,
							  P2P_IE_VENDOR_TYPE);
	if (msg->p2p_attributes &&
	    p2p_parse_p2p_ie(msg->p2p_attributes, msg)) {
		p2p_printf(MSG_DEBUG, "P2P: Failed to parse P2P IE data");
		if (msg->p2p_attributes)
			p2p_hexdump_buf(MSG_MSGDUMP, "P2P: P2P IE data",
					msg->p2p_attributes);
		p2p_parse_free(msg);
		return -1;
	}

#ifdef CONFIG_WIFI_DISPLAY
	if (elems.wfd) {
		msg->wfd_subelems = ieee802_11_vendor_ie_concat(
			data, len, WFD_IE_VENDOR_TYPE);
	}
#endif /* CONFIG_WIFI_DISPLAY */

	msg->pref_freq_list = elems.pref_freq_list;
	msg->pref_freq_list_len = elems.pref_freq_list_len;

	return 0;
}


/**
 * p2p_parse - Parse a P2P Action frame contents
 * @data: Action frame payload after Category and Code fields
 * @len: Length of data buffer in octets
 * @msg: Buffer for returning parsed attributes
 * Returns: 0 on success, -1 on failure
 *
 * Note: Caller must free temporary memory allocations by calling
 * p2p_parse_free() when the parsed data is not needed anymore.
 */
int p2p_parse(const atbm_uint8 *data, atbm_size_t len, struct p2p_message *msg)
{
	//printf("data:%p, msg->dialog_token:%p\n", data, &msg->dialog_token);
	atbm_memset(msg, 0, sizeof(*msg));
	p2p_printf(MSG_DEBUG, "P2P: Parsing the received message");
	if (len < 1) {
		p2p_printf(MSG_DEBUG, "P2P: No Dialog Token in the message");
		return -1;
	}
	
	msg->dialog_token = data[0];
	p2p_printf(MSG_DEBUG, "P2P: * Dialog Token: %d", msg->dialog_token);

	return p2p_parse_ies(data + 1, len - 1, msg);
}


int p2p_parse_ies_separate(const atbm_uint8 *wsc, atbm_size_t wsc_len, const atbm_uint8 *p2p,
			   atbm_size_t p2p_len, struct p2p_message *msg)
{
	atbm_memset(msg, 0, sizeof(*msg));

	msg->wps_attributes = wpabuf_alloc_copy(wsc, wsc_len);
	if (msg->wps_attributes &&
	    p2p_parse_wps_ie(msg->wps_attributes, msg)) {
		p2p_parse_free(msg);
		return -1;
	}

	msg->p2p_attributes = wpabuf_alloc_copy(p2p, p2p_len);
	if (msg->p2p_attributes &&
	    p2p_parse_p2p_ie(msg->p2p_attributes, msg)) {
		p2p_printf(MSG_DEBUG, "P2P: Failed to parse P2P IE data");
		if (msg->p2p_attributes)
			p2p_hexdump_buf(MSG_MSGDUMP, "P2P: P2P IE data",
					msg->p2p_attributes);
		p2p_parse_free(msg);
		return -1;
	}

	return 0;
}


/**
 * p2p_parse_free - Free temporary data from P2P parsing
 * @msg: Parsed attributes
 */
void p2p_parse_free(struct p2p_message *msg)
{
	wpabuf_free(msg->p2p_attributes);
	msg->p2p_attributes = NULL;
	wpabuf_free(msg->wps_attributes);
	msg->wps_attributes = NULL;
#ifdef CONFIG_WIFI_DISPLAY
	wpabuf_free(msg->wfd_subelems);
	msg->wfd_subelems = NULL;
#endif /* CONFIG_WIFI_DISPLAY */
}


int p2p_group_info_parse(const atbm_uint8 *gi, atbm_size_t gi_len,
			 struct p2p_group_info *info)
{
	const atbm_uint8 *g, *gend;

	atbm_memset(info, 0, sizeof(*info));
	if (gi == NULL)
		return 0;

	g = gi;
	gend = gi + gi_len;
	while (g < gend) {
		struct p2p_client_info *cli;
		const atbm_uint8 *cend;
		atbm_uint16 count;
		atbm_uint8 len;

		cli = &info->client[info->num_clients];
		len = *g++;
		if (len > gend - g || len < 2 * ATBM_ETH_ALEN + 1 + 2 + 8 + 1)
			return -1; /* invalid data */
		cend = g + len;
		/* g at start of P2P Client Info Descriptor */
		cli->p2p_device_addr = g;
		g += ATBM_ETH_ALEN;
		cli->p2p_interface_addr = g;
		g += ATBM_ETH_ALEN;
		cli->dev_capab = *g++;

		cli->config_methods = ATBM_WPA_GET_BE16(g);
		g += 2;
		cli->pri_dev_type = g;
		g += 8;

		/* g at Number of Secondary Device Types */
		len = *g++;
		if (8 * len > cend - g)
			return -1; /* invalid data */
		cli->num_sec_dev_types = len;
		cli->sec_dev_types = g;
		g += 8 * len;

		/* g at Device Name in WPS TLV format */
		if (cend - g < 2 + 2)
			return -1; /* invalid data */
		if (ATBM_WPA_GET_BE16(g) != ATTR_DEV_NAME)
			return -1; /* invalid Device Name TLV */
		g += 2;
		count = ATBM_WPA_GET_BE16(g);
		g += 2;
		if (count > cend - g)
			return -1; /* invalid Device Name TLV */
		if (count >= WPS_DEV_NAME_MAX_LEN)
			count = WPS_DEV_NAME_MAX_LEN;
		cli->dev_name = (const char *) g;
		cli->dev_name_len = count;

		g = cend;

		info->num_clients++;
		if (info->num_clients == P2P_MAX_GROUP_ENTRIES)
			return -1;
	}

	return 0;
}


static int p2p_group_info_text(const atbm_uint8 *gi, atbm_size_t gi_len, char *buf,
			       char *end)
{
	char *pos = buf;
	int ret;
	struct p2p_group_info info;
	unsigned int i;

	if (p2p_group_info_parse(gi, gi_len, &info) < 0)
		return 0;

	for (i = 0; i < info.num_clients; i++) {
		struct p2p_client_info *cli;
		char name[WPS_DEV_NAME_MAX_LEN + 1];
		char devtype[WPS_DEV_TYPE_BUFSIZE];
		atbm_uint8 s;
		int count;

		cli = &info.client[i];
		ret = os_snprintf(pos, end - pos, "p2p_group_client: "
				  "dev=" MACSTR " iface=" MACSTR,
				  MAC2STR(cli->p2p_device_addr),
				  MAC2STR(cli->p2p_interface_addr));
		if (os_snprintf_error(end - pos, ret))
			return pos - buf;
		pos += ret;

		ret = os_snprintf(pos, end - pos,
				  " dev_capab=0x%x config_methods=0x%x "
				  "dev_type=%s",
				  cli->dev_capab, cli->config_methods,
				  wps_dev_type_bin2str(cli->pri_dev_type,
						       devtype,
						       sizeof(devtype)));
		if (os_snprintf_error(end - pos, ret))
			return pos - buf;
		pos += ret;

		for (s = 0; s < cli->num_sec_dev_types; s++) {
			ret = os_snprintf(pos, end - pos, " dev_type=%s",
					  wps_dev_type_bin2str(
						  &cli->sec_dev_types[s * 8],
						  devtype, sizeof(devtype)));
			if (os_snprintf_error(end - pos, ret))
				return pos - buf;
			pos += ret;
		}

		atbm_memcpy(name, cli->dev_name, cli->dev_name_len);
		name[cli->dev_name_len] = '\0';
		count = (int) cli->dev_name_len - 1;
		while (count >= 0) {
			if (is_ctrl_char(name[count]))
				name[count] = '_';
			count--;
		}

		ret = os_snprintf(pos, end - pos, " dev_name='%s'\n", name);
		if (os_snprintf_error(end - pos, ret))
			return pos - buf;
		pos += ret;
	}

	return pos - buf;
}


/**
 * p2p_attr_text - Build text format description of P2P IE attributes
 * @data: P2P IE contents
 * @buf: Buffer for returning text
 * @end: Pointer to the end of the buf area
 * Returns: Number of octets written to the buffer or -1 on faikure
 *
 * This function can be used to parse P2P IE contents into text format
 * field=value lines.
 */
int p2p_attr_text(struct wpabuf *data, char *buf, char *end)
{
	struct p2p_message msg;
	char *pos = buf;
	int ret;

	atbm_memset(&msg, 0, sizeof(msg));
	if (p2p_parse_p2p_ie(data, &msg))
		return -1;

	if (msg.capability) {
		ret = os_snprintf(pos, end - pos,
				  "p2p_dev_capab=0x%x\n"
				  "p2p_group_capab=0x%x\n",
				  msg.capability[0], msg.capability[1]);
		if (os_snprintf_error(end - pos, ret))
			return pos - buf;
		pos += ret;
	}

	if (msg.pri_dev_type) {
		char devtype[WPS_DEV_TYPE_BUFSIZE];
		ret = os_snprintf(pos, end - pos,
				  "p2p_primary_device_type=%s\n",
				  wps_dev_type_bin2str(msg.pri_dev_type,
						       devtype,
						       sizeof(devtype)));
		if (os_snprintf_error(end - pos, ret))
			return pos - buf;
		pos += ret;
	}

	ret = os_snprintf(pos, end - pos, "p2p_device_name=%s\n",
			  msg.device_name);
	if (os_snprintf_error(end - pos, ret))
		return pos - buf;
	pos += ret;

	if (msg.p2p_device_addr) {
		ret = os_snprintf(pos, end - pos, "p2p_device_addr=" MACSTR
				  "\n",
				  MAC2STR(msg.p2p_device_addr));
		if (os_snprintf_error(end - pos, ret))
			return pos - buf;
		pos += ret;
	}

	ret = os_snprintf(pos, end - pos, "p2p_config_methods=0x%x\n",
			  msg.config_methods);
	if (os_snprintf_error(end - pos, ret))
		return pos - buf;
	pos += ret;

	ret = p2p_group_info_text(msg.group_info, msg.group_info_len,
				  pos, end);
	if (ret < 0)
		return pos - buf;
	pos += ret;

	return pos - buf;
}


int p2p_get_cross_connect_disallowed(const struct wpabuf *p2p_ie)
{
	struct p2p_message msg;

	atbm_memset(&msg, 0, sizeof(msg));
	if (p2p_parse_p2p_ie(p2p_ie, &msg))
		return 0;

	if (!msg.manageability)
		return 0;

	return !(msg.manageability[0] & P2P_MAN_CROSS_CONNECTION_PERMITTED);
}


atbm_uint8 p2p_get_group_capab(const struct wpabuf *p2p_ie)
{
	struct p2p_message msg;

	atbm_memset(&msg, 0, sizeof(msg));
	if (p2p_parse_p2p_ie(p2p_ie, &msg))
		return 0;

	if (!msg.capability)
		return 0;

	return msg.capability[1];
}


const atbm_uint8 * p2p_get_go_dev_addr(const struct wpabuf *p2p_ie)
{
	struct p2p_message msg;

	atbm_memset(&msg, 0, sizeof(msg));
	if (p2p_parse_p2p_ie(p2p_ie, &msg))
		return NULL;

	if (msg.p2p_device_addr)
		return msg.p2p_device_addr;
	if (msg.device_id)
		return msg.device_id;

	return NULL;
}
#endif

