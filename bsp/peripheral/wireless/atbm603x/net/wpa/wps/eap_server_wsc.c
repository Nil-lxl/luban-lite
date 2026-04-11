/*
 * EAP-WSC server for Wi-Fi Protected Setup
 * Copyright (c) 2007-2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
#include "atbm_hal.h"
#include "wpa_debug.h"
extern struct wpabuf * eap_msg_alloc(EapVenType vendor, EapType type, atbm_uint32 payload_len,
			      EapCodeType code, atbm_uint8 identifier);
extern const atbm_uint8 * eap_hdr_validate(EapVenType vendor, EapType eap_type,
			    const struct wpabuf *msg, atbm_size_t *plen);

extern struct wpabuf * wps_get_msg(struct wps_data *wps, enum wsc_op_code *op_code);
extern atbm_void wps_deinit(struct wps_data *data);
extern enum wps_process_res wps_process_msg(struct wps_data *wps,
				     enum wsc_op_code op_code, const struct wpabuf *msg);
 atbm_void hostapd_eap_wsc_init(struct atbmwifi_vif *priv,atbm_uint8 *da)
{
	struct hostapd_data *hapd;
	int link_id;
	struct hostapd_sta_info *sta;
	int registrar = 1;
	struct wps_data *data;
#if CONFIG_P2P
	struct wpabuf *privkey = (struct wpabuf *)priv->p2p_wps_privkey;
	struct wpabuf *pubkey = (struct wpabuf *)priv->p2p_wps_pubkey;
#endif
	struct atbmwifi_cfg *config=NULL;
	
	wpa_printf(MSG_DEBUG, "EAP-WSC: hostapd_eap_wsc_init start");


	//atbm_memset(&data, 0, sizeof(data));
	link_id = atbmwifi_find_link_id(priv, da);
	if ((link_id == 0))
	{
		wifi_printk(WIFI_DBG_ERROR, "hostapd_eap_wsc_init,link_id=%d fail\n",link_id);
		return;
	}
	sta = (struct hostapd_sta_info *)priv->link_id_db[link_id-1].sta_priv.reserved;
	if ((sta == NULL))
	{
		wifi_printk(WIFI_DBG_ERROR, "hostapd_eap_wsc_init,link_id=%d\n",link_id);
		return;
	}
	data = (struct wps_data *)atbm_kzalloc(sizeof(struct wps_data), GFP_KERNEL);
	if (data == NULL)
		return;
	hapd = (struct hostapd_data *)priv->appdata;
	if(hapd->wpsdata){
		wps_deinit(hapd->wpsdata);
		hapd->wpsdata = NULL;
	}
	//wpa_printf(MSG_DEBUG, "EAP-WSC: eap_wsc_init 111\n");
	data->wps = hapd->wps;
	data->registrar = registrar;
	if (registrar) {
		atbm_memcpy(data->uuid_r, hapd->wps->uuid, WPS_UUID_LEN);
	} else {
		atbm_memcpy(data->mac_addr_e, hapd->wps->dev.mac_addr, ATBM_ETH_ALEN);
		atbm_memcpy(data->uuid_e, hapd->wps->uuid, WPS_UUID_LEN);
	}

	if (config != NULL){
		if (config->dev_password_len > 0) {
			data->dev_pw_id = DEV_PW_DEFAULT;
			data->dev_password = (atbm_uint8 *)strdup((char *)config->dev_password);

			data->dev_password_len = config->dev_password_len;
			wpa_printf(MSG_DEBUG, "WPS: AP PIN dev_password %s %d",
					data->dev_password, data->dev_password_len);
		}
	}

	data->pbc = priv->pbc;
	if (priv->pbc) {
		/* Use special PIN '00000000' for PBC */
		data->dev_pw_id = DEV_PW_PUSHBUTTON;		
		data->dev_password = (atbm_uint8 *)strdup("00000000");
		data->dev_password_len = 8;
	}

#if CONFIG_P2P
	if(priv->p2p_ap){
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
		}
	}
	
	data->state = data->registrar ? RECV_M1 : SEND_M1;

	atbm_memcpy(data->peer_dev.mac_addr, sta->addr, ATBM_ETH_ALEN);
	data->last_msg = 0 ;

	//data->pbc_in_m1 = 1;
	wpa_printf(MSG_DEBUG, "EAP-WSC: hostapd_eap_wsc_init\n");

	hapd->wpsdata = data;
	hapd->wps_last_rx_data = NULL;
	return ;
}


 atbm_void eap_wsc_dinit(struct atbmwifi_vif *priv)
{
	struct hostapd_data *hapd;
	hapd = (struct hostapd_data *)priv->appdata;

	wps_deinit(hapd->wpsdata);
	hapd->wpsdata = NULL;
	if (hapd->wps_last_rx_data)
		wpabuf_free(hapd->wps_last_rx_data);
	hapd->wps_last_rx_data = NULL;
}

 struct wpabuf * eap_wsc_build_start(atbm_uint8 id)
{
	struct wpabuf *req;

	req = eap_msg_alloc(ATBM_EAP_VENDOR_WFA,EAP_VENDOR_TYPE_WSC, 2,EAP_CODE_REQUEST,id);
	if (req == NULL) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Failed to allocate memory for "
			   "request");
		return NULL;
	}

	//wpa_printf(MSG_DEBUG, "EAP-WSC: Send WSC/Start");
	wpabuf_put_u8(req, WSC_Start); /* Op-Code */
	wpabuf_put_u8(req, 0); /* Flags */

	return req;
}


 static struct wpabuf * eap_wsc_build_msg(struct eap_wsc_data *data, atbm_uint8 id)
{
	struct wpabuf *req;
	atbm_uint8 flags;
	atbm_uint32 send_len, plen;

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
	req = eap_msg_alloc(ATBM_EAP_VENDOR_WFA, EAP_VENDOR_TYPE_WSC, plen,
			    EAP_CODE_REQUEST, id);
	if (req == NULL) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Failed to allocate memory for "
			   "request");
		return NULL;
	}

	wpabuf_put_u8(req, data->out_op_code); /* Op-Code */
	wpabuf_put_u8(req, flags); /* Flags */
	if (flags & WSC_FLAGS_LF)
		wpabuf_put_be16(req, wpabuf_len(data->out_buf));

	wpabuf_put_data(req, wpabuf_head_u8(data->out_buf) + data->out_used,
			send_len);
	data->out_used += send_len;

	if (data->out_used == wpabuf_len(data->out_buf)) {
		wpa_printf(MSG_DEBUG, "EAP-WSC: Sending out %lu bytes "
			   "(message sent completely)",
			   (unsigned long) send_len);
		wpabuf_free(data->out_buf);
		data->out_buf = NULL;
		data->out_used = 0;
		eap_wsc_state(data, MESG);
	} else {
		wpa_printf(MSG_DEBUG, "EAP-WSC: Sending out %lu bytes "
			   "(%lu more to send)", (unsigned long) send_len,
			   (unsigned long) wpabuf_len(data->out_buf) -
			   data->out_used);
		eap_wsc_state(data, WAIT_FRAG_ACK);
	}

	return req;
}

 static struct wpabuf * eap_wsc_build_msg_fail(atbm_uint8 id)
{
	struct wpabuf *req;
	struct atbm_eap_hdr *hdr;

	req = wpabuf_alloc(sizeof(struct atbm_eap_hdr));
	if (req == NULL) {
		wpa_printf(MSG_ERROR, "EAP-WSC: Failed to allocate memory for "
			   "request");
		return NULL;
	}
	hdr = wpabuf_put(req, sizeof(*hdr));
	hdr->code = EAP_CODE_FAILURE;
	hdr->identifier = id;
	hdr->length = atbm_host_to_be16(sizeof(struct atbm_eap_hdr));
	return req;
}

extern struct hostapd_data *g_hostapd;

 atbm_void wps_send_eapol_timeout(void *data1, void *data2)
{
	struct hostapd_sta_info *sta = data2;
	if(g_hostapd && sta){
		wifi_printk(WIFI_ALWAYS, "wps_send_eapol_timeout tx retry!!\n");
		hostapd_send_eapol(wpa_get_driver_priv(g_hostapd->priv), sta->addr, ATBM_ETH_P_EAPOL, (atbm_uint8*)g_hostapd->wps_tx_hdr, g_hostapd->wps_tx_hdr_len);
	}
}

 atbm_void hostapd_eap_wsc_process(struct hostapd_data *hapd,struct hostapd_sta_info *sta,struct wpabuf *respData)
{
	struct atbmwifi_vif *priv = wpa_get_driver_priv(hapd->priv);
	const atbm_uint8 *start, *pos, *end;
	atbm_size_t len;
	atbm_uint8 op_code, flags;
	//atbm_uint16 message_length = 0;
	enum wps_process_res res;
	struct wpabuf tmpbuf;
	struct eap_wsc_data data;
	struct wpabuf *Sndbuf;
	struct atbmwifi_ieee802_1x_hdr *hdr;

	atbm_memset(&data, 0, sizeof(data));

	pos = eap_hdr_validate(ATBM_EAP_VENDOR_WFA, EAP_VENDOR_TYPE_WSC,
			       respData, &len);
	if (pos == NULL || len < 2)
		return; /* Should not happen; message already verified */

	start = pos;
	end = start + len;

	op_code = *pos++;
	flags = *pos++;
	/*if (flags & WSC_FLAGS_LF) {
		if (end - pos < 2) {
			wpa_printf(MSG_DEBUG, "EAP-WSC: Message underflow");
			return;
		}
		message_length = ATBM_WPA_GET_BE16(pos);
		pos += 2;

		if (message_length < end - pos || message_length > 50000) {
			wpa_printf(MSG_DEBUG, "EAP-WSC: Invalid Message "
				   "Length");
			return;
		}
	}*/

	wpa_printf(MSG_DEBUG, "EAP-WSC: Received packet: Op-Code %d "
		   "Flags 0x%x",
		   op_code, flags);

	if (op_code != WSC_ACK && op_code != WSC_NACK && op_code != WSC_MSG &&
	    op_code != WSC_Done) {
		wpa_printf(MSG_DEBUG, "EAP-WSC: Unexpected Op-Code %d",
			   op_code);
		return;
	}

	////if (flags & WSC_FLAGS_MF) {
	//	return;
	//}

	/* Wrap unfragmented messages as wpabuf without extra copy */
	wpabuf_set(&tmpbuf, pos, end - pos);

	res = wps_process_msg(hapd->wpsdata, op_code,  &tmpbuf);
	switch (res) {
	case WPS_DONE:
		wpa_printf(MSG_DEBUG, "EAP-WSC: WPS processing completed "
			   "successfully - report EAP failure");
		//eap_wsc_state(data, ATBM_WPS_FAIL);

		atbmwifi_eloop_cancel_timeout(wps_send_eapol_timeout, (atbm_void *)hapd, (atbm_void *)sta);
		hapd->priv->pbc = 0;
		hapd->priv->pin = 0;
		hapd->wpsdata->id++;
//		hapd->wpsdata->state = RECV_DONE;
		Sndbuf = eap_wsc_build_msg_fail(hapd->wpsdata->id);
		goto send;
	case WPS_CONTINUE:
		//eap_wsc_state(data, MESG);
		atbmwifi_eloop_cancel_timeout(wps_send_eapol_timeout, (atbm_void *)hapd, (atbm_void *)sta);
		break;
	case WPS_FAILURE:
		wpa_printf(MSG_DEBUG, "EAP-WSC: WPS processing failed");
		//eap_wsc_state(data, ATBM_WPS_FAIL);
		break;
	case WPS_PENDING:
		break;
	}
	data.fragment_size = 1400;

	data.out_buf = wps_get_msg(hapd->wpsdata,
						    &data.out_op_code);
	if (data.out_buf == NULL) {
		wpa_printf(MSG_DEBUG, "EAP-WSC: Failed to "
			   "receive message from WPS");
		return ;
	}
	data.out_used = 0;
	hapd->wpsdata->id++;
	Sndbuf = eap_wsc_build_msg(&data, hapd->wpsdata->id);
send:		
	len = sizeof(struct atbmwifi_ieee802_1x_hdr) + Sndbuf->used;

	if(hapd->wps_tx_hdr){
		atbm_kfree(hapd->wps_tx_hdr);
		hapd->wps_tx_hdr = ATBM_NULL;
	}
	
	hdr = (struct atbmwifi_ieee802_1x_hdr *)atbm_kzalloc(len, GFP_KERNEL);
	if (hdr == NULL)
	{
		wpabuf_free(Sndbuf);
		return;
	}
	// TO DO GET VERSION
	hdr->version = EAPOL_VERSION;
	hdr->type = ATBM_IEEE802_1X_TYPE_EAP_PACKET;
	hdr->length = atbm_host_to_be16(Sndbuf->used);
	atbm_memcpy((atbm_uint8 *)(hdr + 1), Sndbuf->buf, Sndbuf->used);
	
	hostapd_send_eapol(priv, sta->addr,ATBM_ETH_P_EAPOL,(atbm_uint8 *)hdr,len);
	wpabuf_free(Sndbuf);
//	atbm_kfree(hdr);
//	hdr = 0;
	
	if ((hapd->priv->pbc == 0) && (hapd->priv->pin == 0)){
#if 0
		eap_wsc_dinit(hapd->priv);
#if CONFIG_P2P
		if(hapd->priv->p2p_ap){
			atbm_p2p_wps_sucess(hapd->priv, 1);
		}
#endif
#endif
	}else{
		hapd->wps_tx_hdr = hdr;
		hapd->wps_tx_hdr_len = len;
		atbmwifi_eloop_register_timeout(WPA_SEND_EAPOL_TIMEOUT, 0,
			        wps_send_eapol_timeout, (atbm_void *)hapd, (atbm_void *)sta);
	}
}

