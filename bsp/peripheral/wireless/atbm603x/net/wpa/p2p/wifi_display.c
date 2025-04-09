/*
 * wpa_supplicant - Wi-Fi Display
 * Copyright (c) 2011, Atheros Communications, Inc.
 * Copyright (c) 2011-2012, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "p2p.h"
#include "wpa_supplicant_i.h"
#include "wifi_display.h"


#define WIFI_DISPLAY_SUBELEM_HEADER_LEN 3

#ifdef CONFIG_WIFI_DISPLAY

int wifi_display_init(struct atbmwifi_vif *priv)
{
	priv->wifi_display = 1;
	return 0;
}


void wifi_display_deinit(struct atbmwifi_vif *priv)
{
	int i;
	for (i = 0; i < MAX_WFD_SUBELEMS; i++) {
		wpabuf_free(priv->wfd_subelem[i]);
		priv->wfd_subelem[i] = NULL;
	}
}


struct wpabuf * wifi_display_get_wfd_ie(struct atbmwifi_vif *priv)
{
	struct wpabuf *ie;
	atbm_size_t len;
	int i;

	if (priv->p2pdata == NULL)
		return NULL;

	len = 0;
	for (i = 0; i < MAX_WFD_SUBELEMS; i++) {
		if (priv->wfd_subelem[i])
			len += wpabuf_len(priv->wfd_subelem[i]);
	}

	ie = wpabuf_alloc(len);
	if (ie == NULL)
		return NULL;

	for (i = 0; i < MAX_WFD_SUBELEMS; i++) {
		if (priv->wfd_subelem[i])
			wpabuf_put_buf(ie, priv->wfd_subelem[i]);
	}

	return ie;
}


static int wifi_display_update_wfd_ie(struct atbmwifi_vif *priv)
{
	struct wpabuf *ie, *buf;
	atbm_size_t len, plen;

	if (priv->p2pdata == NULL)
		return 0;

	wpa_printf(MSG_DEBUG, "WFD: Update WFD IE");

	if (!priv->wifi_display) {
		wpa_printf(MSG_DEBUG, "WFD: Wi-Fi Display disabled - do not "
			   "include WFD IE");
		p2p_set_wfd_ie_beacon(priv->p2pdata, NULL);
		p2p_set_wfd_ie_probe_req(priv->p2pdata, NULL);
		p2p_set_wfd_ie_probe_resp(priv->p2pdata, NULL);
		p2p_set_wfd_ie_assoc_req(priv->p2pdata, NULL);
		p2p_set_wfd_ie_invitation(priv->p2pdata, NULL);
		p2p_set_wfd_ie_prov_disc_req(priv->p2pdata, NULL);
		p2p_set_wfd_ie_prov_disc_resp(priv->p2pdata, NULL);
		p2p_set_wfd_ie_go_neg(priv->p2pdata, NULL);
		p2p_set_wfd_dev_info(priv->p2pdata, NULL);
		p2p_set_wfd_r2_dev_info(priv->p2pdata, NULL);
		p2p_set_wfd_assoc_bssid(priv->p2pdata, NULL);
		p2p_set_wfd_coupled_sink_info(priv->p2pdata, NULL);
		return 0;
	}

	p2p_set_wfd_dev_info(priv->p2pdata,
			     priv->wfd_subelem[WFD_SUBELEM_DEVICE_INFO]);
	p2p_set_wfd_r2_dev_info(
		priv->p2pdata, priv->wfd_subelem[WFD_SUBELEM_R2_DEVICE_INFO]);
	p2p_set_wfd_assoc_bssid(
		priv->p2pdata,
		priv->wfd_subelem[WFD_SUBELEM_ASSOCIATED_BSSID]);
	p2p_set_wfd_coupled_sink_info(
		priv->p2pdata, priv->wfd_subelem[WFD_SUBELEM_COUPLED_SINK]);

	/*
	 * WFD IE is included in number of management frames. Two different
	 * sets of subelements are included depending on the frame:
	 *
	 * Beacon, (Re)Association Request, GO Negotiation Req/Resp/Conf,
	 * Provision Discovery Req:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 *
	 * Probe Request:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 * [WFD Extended Capability]
	 *
	 * Probe Response:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 * [WFD Extended Capability]
	 * [WFD Session Info]
	 *
	 * (Re)Association Response, P2P Invitation Req/Resp,
	 * Provision Discovery Resp:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 * [WFD Session Info]
	 */
	len = 0;
	if (priv->wfd_subelem[WFD_SUBELEM_DEVICE_INFO])
		len += wpabuf_len(priv->wfd_subelem[
					  WFD_SUBELEM_DEVICE_INFO]);

	if (priv->wfd_subelem[WFD_SUBELEM_R2_DEVICE_INFO])
		len += wpabuf_len(priv->wfd_subelem[
					  WFD_SUBELEM_R2_DEVICE_INFO]);

	if (priv->wfd_subelem[WFD_SUBELEM_ASSOCIATED_BSSID])
		len += wpabuf_len(priv->wfd_subelem[
					  WFD_SUBELEM_ASSOCIATED_BSSID]);
	if (priv->wfd_subelem[WFD_SUBELEM_COUPLED_SINK])
		len += wpabuf_len(priv->wfd_subelem[
					  WFD_SUBELEM_COUPLED_SINK]);
	if (priv->wfd_subelem[WFD_SUBELEM_SESSION_INFO])
		len += wpabuf_len(priv->wfd_subelem[
					  WFD_SUBELEM_SESSION_INFO]);
	if (priv->wfd_subelem[WFD_SUBELEM_EXT_CAPAB])
		len += wpabuf_len(priv->wfd_subelem[WFD_SUBELEM_EXT_CAPAB]);
	buf = wpabuf_alloc(len);
	if (buf == NULL)
		return -1;

	if (priv->wfd_subelem[WFD_SUBELEM_DEVICE_INFO])
		wpabuf_put_buf(buf,
			       priv->wfd_subelem[WFD_SUBELEM_DEVICE_INFO]);

	if (priv->wfd_subelem[WFD_SUBELEM_R2_DEVICE_INFO])
		wpabuf_put_buf(buf,
			       priv->wfd_subelem[WFD_SUBELEM_R2_DEVICE_INFO]);

	if (priv->wfd_subelem[WFD_SUBELEM_ASSOCIATED_BSSID])
		wpabuf_put_buf(buf, priv->wfd_subelem[
				       WFD_SUBELEM_ASSOCIATED_BSSID]);
	if (priv->wfd_subelem[WFD_SUBELEM_COUPLED_SINK])
		wpabuf_put_buf(buf,
			       priv->wfd_subelem[WFD_SUBELEM_COUPLED_SINK]);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Beacon", ie);
	p2p_set_wfd_ie_beacon(priv->p2pdata, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for (Re)Association Request",
			ie);
	p2p_set_wfd_ie_assoc_req(priv->p2pdata, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for GO Negotiation", ie);
	p2p_set_wfd_ie_go_neg(priv->p2pdata, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Provision Discovery "
			"Request", ie);
	p2p_set_wfd_ie_prov_disc_req(priv->p2pdata, ie);

	plen = buf->used;
	if (priv->wfd_subelem[WFD_SUBELEM_EXT_CAPAB])
		wpabuf_put_buf(buf,
			       priv->wfd_subelem[WFD_SUBELEM_EXT_CAPAB]);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Probe Request", ie);
	p2p_set_wfd_ie_probe_req(priv->p2pdata, ie);

	if (priv->wfd_subelem[WFD_SUBELEM_SESSION_INFO])
		wpabuf_put_buf(buf,
			       priv->wfd_subelem[WFD_SUBELEM_SESSION_INFO]);
	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Probe Response", ie);
	p2p_set_wfd_ie_probe_resp(priv->p2pdata, ie);

	/* Remove WFD Extended Capability from buffer */
	buf->used = plen;
	if (priv->wfd_subelem[WFD_SUBELEM_SESSION_INFO])
		wpabuf_put_buf(buf,
			       priv->wfd_subelem[WFD_SUBELEM_SESSION_INFO]);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for P2P Invitation", ie);
	p2p_set_wfd_ie_invitation(priv->p2pdata, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Provision Discovery "
			"Response", ie);
	p2p_set_wfd_ie_prov_disc_resp(priv->p2pdata, ie);

	wpabuf_free(buf);

	return 0;
}


void wifi_display_enable(struct atbmwifi_vif *priv, int enabled)
{
	wpa_printf(MSG_DEBUG, "WFD: Wi-Fi Display %s",
		   enabled ? "enabled" : "disabled");
	priv->wifi_display = enabled;
	wifi_display_update_wfd_ie(priv);
}


int wifi_display_subelem_set(struct atbmwifi_vif *priv, int subelem, char *val)
{
	atbm_size_t len;
	struct wpabuf *e;

	if (subelem < 0 || subelem >= MAX_WFD_SUBELEMS)
		return -1;

	len = val ? strlen(val) : 0;

	if(len & 1)
		return -1;

	len >>= 1;

	if (len == 0) {
		/* Clear subelement */
		e = NULL;
		wpa_printf(MSG_DEBUG, "WFD: Clear subelement %d", subelem);
	} else {
		e = wpabuf_alloc(1 + len);
		if (e == NULL)
			return -1;
		wpabuf_put_u8(e, subelem);
		if (atbmwifi_hexstr2bin(wpabuf_put(e, len), val, len) < 0) {
			wpabuf_free(e);
			return -1;
		}
		wpa_printf(MSG_DEBUG, "WFD: Set subelement %d", subelem);
	}

	wpabuf_free(priv->wfd_subelem[subelem]);
	priv->wfd_subelem[subelem] = e;
	wifi_display_update_wfd_ie(priv);

	return 0;
}


int wifi_display_subelem_set_from_ies(struct atbmwifi_vif *priv,
				      struct wpabuf *ie)
{
	int subelements[MAX_WFD_SUBELEMS] = {};
	const atbm_uint8 *pos, *end;
	unsigned int len, subelem;
	struct wpabuf *e;

	wpa_printf(MSG_DEBUG, "WFD IEs set: %p - %lu",
		   ie, ie ? (unsigned long) wpabuf_len(ie) : 0);

	if (ie == NULL || wpabuf_len(ie) < 6)
		return -1;

	pos = wpabuf_head(ie);
	end = pos + wpabuf_len(ie);

	while (end > pos) {
		if (pos + 3 > end)
			break;

		len = ATBM_WPA_GET_BE16(pos + 1) + 3;

		wpa_printf(MSG_DEBUG, "WFD Sub-Element ID %d - len %d",
			   *pos, len - 3);

		if (len > (unsigned int) (end - pos))
			break;

		subelem = *pos;
		if (subelem < MAX_WFD_SUBELEMS && subelements[subelem] == 0) {
			e = wpabuf_alloc_copy(pos, len);
			if (e == NULL)
				return -1;

			wpabuf_free(priv->wfd_subelem[subelem]);
			priv->wfd_subelem[subelem] = e;
			subelements[subelem] = 1;
		}

		pos += len;
	}

	for (subelem = 0; subelem < MAX_WFD_SUBELEMS; subelem++) {
		if (subelements[subelem] == 0) {
			wpabuf_free(priv->wfd_subelem[subelem]);
			priv->wfd_subelem[subelem] = NULL;
		}
	}

	return wifi_display_update_wfd_ie(priv);
}


int wifi_display_subelem_get(struct atbmwifi_vif *priv, char *cmd,
			     char *buf, atbm_size_t buflen)
{
	int subelem;

	if (strcmp(cmd, "all") == 0) {
		struct wpabuf *ie;
		int res;

		ie = wifi_display_get_wfd_ie(priv);
		if (ie == NULL)
			return 0;
		res = atbmwifi_wpa_snprintf_hex(buf, buflen, wpabuf_head(ie),
				       wpabuf_len(ie));
		wpabuf_free(ie);
		return res;
	}

	subelem = atoi(cmd);
	if (subelem < 0 || subelem >= MAX_WFD_SUBELEMS)
		return -1;

	if (priv->wfd_subelem[subelem] == NULL)
		return 0;

	return atbmwifi_wpa_snprintf_hex(buf, buflen,
				wpabuf_head_u8(priv->wfd_subelem[subelem]) +
				1,
				wpabuf_len(priv->wfd_subelem[subelem]) - 1);
}


char * wifi_display_subelem_hex(const struct wpabuf *wfd_subelems, atbm_uint8 id)
{
	char *subelem = NULL;
	const atbm_uint8 *buf;
	atbm_size_t buflen;
	atbm_size_t i = 0;
	atbm_uint16 elen;

	if (!wfd_subelems)
		return NULL;

	buf = wpabuf_head_u8(wfd_subelems);
	if (!buf)
		return NULL;

	buflen = wpabuf_len(wfd_subelems);

	while (i + WIFI_DISPLAY_SUBELEM_HEADER_LEN < buflen) {
		elen = ATBM_WPA_GET_BE16(buf + i + 1);
		if (i + WIFI_DISPLAY_SUBELEM_HEADER_LEN + elen > buflen)
			break; /* truncated subelement */

		if (buf[i] == id) {
			/*
			 * Limit explicitly to an arbitrary length to avoid
			 * unnecessarily large allocations. In practice, this
			 * is limited to maximum frame length anyway, so the
			 * maximum memory allocation here is not really that
			 * large. Anyway, the Wi-Fi Display subelements that
			 * are fetched with this function are even shorter.
			 */
			if (elen > 1000)
				break;
			subelem = atbm_kzalloc(2 * elen + 1, GFP_KERNEL);
			if (!subelem)
				return NULL;
			atbmwifi_wpa_snprintf_hex(subelem, 2 * elen + 1,
					 buf + i +
					 WIFI_DISPLAY_SUBELEM_HEADER_LEN,
					 elen);
			break;
		}

		i += elen + WIFI_DISPLAY_SUBELEM_HEADER_LEN;
	}

	return subelem;
}
#endif
