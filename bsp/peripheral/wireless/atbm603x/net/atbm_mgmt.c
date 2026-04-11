
#include "atbm_hal.h"

#if CONFIG_WPS
extern int wps_parse_msg_attr(const atbm_uint8 *msg, atbm_uint8 length, struct wps_parse_attr *attr);
#endif
extern atbm_void atbmwifi_ap_join_timeout(atbm_void *data1,atbm_void *data2);

int atbmwifi_rx_authen(struct atbmwifi_vif *priv,struct atbm_buff *skb);

#if CONFIG_IEEE80211W
static void sme_stop_sa_query(struct wpa_supplicant *wpa_s);

static int sme_check_sa_query_timeout(struct wpa_supplicant *wpa_s)
{
	atbm_uint32 now, passed;

	now = atbm_GetOsTime();
	passed = now - wpa_s->sa_query_start;

	if (1000 < passed) {
		wpa_printf(MSG_WARNING, "SME: SA Query timed out");
		sme_stop_sa_query(wpa_s);
		sta_deauth(wpa_s->priv);
		return 1;
	}

	return 0;
}

static void sme_sa_query_timer(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	unsigned int timeout, sec, usec;
	atbm_uint8 *trans_id, *nbuf;
	struct atbm_buff *skb;
	struct atbmwifi_vif *priv = wpa_s->priv;

	if (wpa_s->sa_query_count > 0 &&
	    sme_check_sa_query_timeout(wpa_s))
		return;

	nbuf = atbm_realloc_array(wpa_s->sa_query_trans_id,
				wpa_s->sa_query_count + 1,
				WLAN_SA_QUERY_TR_ID_LEN);
	if (nbuf == NULL) {
		sme_stop_sa_query(wpa_s);
		return;
	}

	if (wpa_s->sa_query_count == 0) {
		/* Starting a new SA Query procedure */
		wpa_s->sa_query_start = atbm_GetOsTime();
	}
	trans_id = nbuf + wpa_s->sa_query_count * WLAN_SA_QUERY_TR_ID_LEN;
	wpa_s->sa_query_trans_id = nbuf;
	wpa_s->sa_query_count++;

	if (atbmwifi_os_get_random(trans_id, WLAN_SA_QUERY_TR_ID_LEN) < 0) {
		wpa_printf(MSG_DEBUG, "Could not generate SA Query ID");
		sme_stop_sa_query(wpa_s);
		return;
	}

	atbmwifi_eloop_register_timeout(0, 250, sme_sa_query_timer, wpa_s, NULL);

	wpa_printf(MSG_DEBUG, "SME: Association SA Query attempt %d",
		wpa_s->sa_query_count);

	skb = atbmwifi_ieee80211_send_saquery(wpa_s->priv, priv->bssid, priv->bssid, ATBM_WLAN_ACTION_SA_QUERY_REQUEST, trans_id);

	atbmwifi_tx(priv->hw_priv, skb, priv);
}


static void sme_stop_sa_query(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->sa_query_trans_id)
		wpa_printf(MSG_DEBUG, "SME: Stop SA Query");
	atbmwifi_eloop_cancel_timeout(sme_sa_query_timer, wpa_s, NULL);
	atbm_kfree(wpa_s->sa_query_trans_id);
	wpa_s->sa_query_trans_id = NULL;
	wpa_s->sa_query_count = 0;
}

static void sme_start_sa_query(struct wpa_supplicant *wpa_s)
{
	sme_sa_query_timer(wpa_s, NULL);
}


void atbmwifi_handle_unprot_disassoc(struct atbmwifi_vif *priv,struct atbm_buff *skb){
	struct atbmwifi_ieee80211_mgmt *mgmt = ATBM_OS_SKB_DATA(skb);
	struct wpa_supplicant *wpa_s;
	atbm_uint16 reason_code = -1;
	atbm_uint32 now;

	if(priv->iftype == ATBM_NL80211_IFTYPE_STATION){
		wpa_s = (struct wpa_supplicant *)priv->appdata;
		if(wpa_s->wpa_state != ATBM_WPA_COMPLETED){
			return;
		}
		if (atbm_memcmp(mgmt->sa, priv->bssid, ATBM_ETH_ALEN) != 0){
			return;
		}
		reason_code = mgmt->u.deauth.reason_code;
		if(reason_code != ATBM_WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA &&
			reason_code != ATBM_WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA){
			return;
		}
		now = atbm_GetOsTime();
		if(wpa_s->last_unprot_disconnect &&
			(now - wpa_s->last_unprot_disconnect) < 10 * 1000){
			return;
		}
		wpa_s->last_unprot_disconnect = now;
		sme_start_sa_query(wpa_s);
	}
}

int
atbmwifi_ieee80211_drop_unencrypted_mgmt(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_hdr *hdr = ATBM_OS_SKB_DATA(skb);
	struct atbmwifi_ieee80211_rx_status *status = ATBM_IEEE80211_SKB_RXCB(skb);
	atbm_uint16 fc = hdr->frame_control;

	/*
	 * Pass through unencrypted frames if the hardware has
	 * decrypted them already.
	 */
	if (status->flag & ATBM_RX_FLAG_DECRYPTED)
		return 0;

	if (priv->iftype == ATBM_NL80211_IFTYPE_STATION && priv->connect.crypto_igtkgroup) {

		if (atbm_unlikely(!atbmwifi_ieee80211_has_protected(fc) &&
			!atbm_is_multicast_ether_addr(hdr->addr1) &&
			atbmwifi_ieee80211_is_robust_mgmt_frame(hdr))) {
			if(atbmwifi_ieee80211_is_deauth(fc) ||
				atbmwifi_ieee80211_is_disassoc(fc)){
				atbmwifi_handle_unprot_disassoc(priv, skb);
			}

			return -ATBM_EACCES;
		}
		/* BIP does not use Protected field, so need to check MMIE */
		if(atbm_unlikely(atbm_is_multicast_ether_addr(hdr->addr1) &&
			atbmwifi_ieee80211_is_robust_mgmt_frame(hdr)
			)){
			if(atbmwifi_ieee80211_is_deauth(fc) ||
				atbmwifi_ieee80211_is_disassoc(fc)){
				atbmwifi_handle_unprot_disassoc(priv, skb);
			}
			return -ATBM_EACCES;
		}
		
		/*
		 * When using MFP, Action frames are not allowed prior to
		 * having configured keys.
		 */
		if (atbm_unlikely(atbmwifi_ieee80211_is_action(fc) && !priv->connect.crypto_igtkgroup &&
				atbmwifi_ieee80211_is_robust_mgmt_frame(hdr)
			)){
			return -ATBM_EACCES;
		}
	}

	return 0;
}

static void atbmwifi_ieee80211_process_sa_query_req(struct atbmwifi_vif *priv,
					   struct atbmwifi_ieee80211_mgmt *mgmt,
					   atbm_size_t len)
{
	struct sk_buff *skb;

	if (atbm_compare_ether_addr(mgmt->da, priv->mac_addr) != 0) {
		/* Not to own unicast address */
		return;
	}

	if (atbm_compare_ether_addr(mgmt->sa, priv->bssid) != 0 ||
	    atbm_compare_ether_addr(mgmt->bssid, priv->bssid) != 0) {
		/* Not from the current AP or not associated yet. */
		return;
	}

	if (len < 24 + 1 + sizeof(mgmt->u.action.u.sa_query)) {
		/* Too short SA Query request frame */
		return;
	}

	skb = atbmwifi_ieee80211_send_saquery(priv, priv->bssid, priv->bssid, ATBM_WLAN_ACTION_SA_QUERY_RESPONSE, mgmt->u.action.u.sa_query.trans_id);

	atbmwifi_tx(priv->hw_priv, skb, priv);
}
#endif //CONFIG_IEEE80211W

atbm_void atbmwifi_rx_actionFrame(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{	
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
	atbm_uint8 *tmp_ie;
	struct atbmwifi_ieee80211_channel_sw_packed_ie sw_packed_ie;
	sw_packed_ie.chan_sw_ie = ATBM_NULL;
	sw_packed_ie.ex_chan_sw_ie = ATBM_NULL;
	sw_packed_ie.sec_chan_offs_ie = ATBM_NULL;
	switch (mgmt->u.action.category) {
#if ATBM_PKG_REORDER
	case ATBM_WLAN_CATEGORY_BACK:
		{
#if ATBM_DRIVER_PROCESS_BA
			if(priv->hw_priv->driver_setup_ba){
				struct atbmwifi_ieee80211_rx_status *hdr = ATBM_IEEE80211_SKB_RXCB(skb);
				struct atbmwifi_sta_priv *sta_priv;
				sta_priv = atbmwifi_sta_find(priv,mgmt->sa);

				wifi_printk(WIFI_BASETUP, "RX BA action %d\n", mgmt->u.action.u.addba_req.action_code);
				if(!sta_priv || !sta_priv->ampdu_mlme.init){
					wifi_printk(WIFI_DBG_ERROR, "Rx unexpected BA setup Actions\n");
					return;
				}
				switch (mgmt->u.action.u.addba_req.action_code) {
				case ATBM_WLAN_ACTION_ADDBA_REQ:
					ieee80211_process_addba_request(priv->hw_priv, 
								sta_priv, mgmt, ATBM_OS_SKB_LEN(skb));
					break;
				case ATBM_WLAN_ACTION_DELBA:
					ieee80211_process_delba(priv->hw_priv, sta_priv, mgmt, ATBM_OS_SKB_LEN(skb));
					break;
				case ATBM_WLAN_ACTION_ADDBA_RESP:
					ieee80211_process_addba_resp(priv->hw_priv, sta_priv, mgmt, ATBM_OS_SKB_LEN(skb));
					break;
				default:
					break;
				}
			}else
#endif
			{
				struct atbmwifi_ieee80211_rx_status *hdr = ATBM_IEEE80211_SKB_RXCB(skb);
				struct atbm_ba_params ba_params;
				ba_params.link_id = hdr->link_id;

				switch (mgmt->u.action.u.addba_req.action_code) {
				case ATBM_WLAN_ACTION_ADDBA_REQ:
				{
					atbm_uint16 capab;
					ba_params.action = ATBM_BA__ACTION_RX_ADDBR;
					ba_params.timeout = atbm_le16_to_cpu(mgmt->u.action.u.addba_req.timeout);
					capab = atbm_le16_to_cpu(mgmt->u.action.u.addba_req.capab);
					ba_params.tid =  (capab & ATBM_IEEE80211_ADDBA_PARAM_TID_MASK) >> 2;
	//					ba_params.win_size =  (capab & IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK) >> 6;
					ba_params.win_size = BUFF_STORED_LEN;
					ba_params.ssn = atbm_le16_to_cpu(mgmt->u.action.u.addba_req.start_seq_num) >> 4;
					wifi_printk(WIFI_ALWAYS,"rx ADDBA_REQ:ssn(%x),tid(%d),link_id(%d),win_size(%d)\n",ba_params.ssn,ba_params.tid,ba_params.link_id,ba_params.win_size);
					atbm_updata_ba_tid_params(priv,&ba_params);
					break;
				}
				case ATBM_WLAN_ACTION_DELBA:
				{
					atbm_uint16 params;
					params = atbm_le16_to_cpu(mgmt->u.action.u.delba.params);
					ba_params.tid = (params & ATBM_IEEE80211_DELBA_PARAM_TID_MASK) >> 12;
					ba_params.action = ATBM_BA__ACTION_RX_DELBA;
					atbm_updata_ba_tid_params(priv,&ba_params);
					break;
				}
				default:
					break;
				}
			}
		}
		break;
#endif
	case ATBM_WLAN_CATEGORY_SPECTRUM_MGMT:
		// add 8.5.2.6 Channel Switch Announcement frame format
		if(ATBM_WLAN_ACTION_SPCT_CHL_SWITCH == mgmt->u.action.u.chan_switch.action_code)
		{
			sw_packed_ie.chan_sw_ie = &mgmt->u.action.u.chan_switch.sw_elem;
			tmp_ie = ((atbm_uint8 *)sw_packed_ie.chan_sw_ie) + 3;

			if((tmp_ie[0] == ATBM_WLAN_EID_SECONDARY_CH_OFFSET)&&(tmp_ie[1]==1))
			{
				sw_packed_ie.sec_chan_offs_ie = (struct atbmwifi_ieee80211_sec_chan_offs_ie *)(tmp_ie+2);
			}
			atbm_ieee80211_sta_process_chanswitch(priv,&priv->bss,&sw_packed_ie,0);
		}
		break;
	case ATBM_WLAN_CATEGORY_PUBLIC:
		//add 8.5.8.7 Extended Channel Switch Announcement frame format
		if(ATBM_WLAN_PUB_ACTION_EX_CHL_SW_ANNOUNCE == mgmt->u.action.u.ext_chan_switch.action_code)
		{
			sw_packed_ie.ex_chan_sw_ie = &mgmt->u.action.u.ext_chan_switch.ext_sw_elem;
	
			atbm_ieee80211_sta_process_chanswitch(priv,&priv->bss,&sw_packed_ie,0);
			break;
		}
#if CONFIG_P2P
	case ATBM_WLAN_CATEGORY_VENDOR_SPECIFIC:
		{
			struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *)ATBM_OS_SKB_DATA(skb);
			struct atbmwifi_ieee80211_rx_status *hw_hdr = ATBM_IEEE80211_SKB_RXCB(skb);
			atbm_uint8 *data = (atbm_uint8 *)ATBM_OS_SKB_DATA(skb) + offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_req.variable) + 1;
			int len = ATBM_OS_SKB_LEN(skb) - offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_req.variable) - 1;
			atbm_p2p_rx_action(priv, mgmt->da, mgmt->sa, mgmt->bssid,
				mgmt->u.action.category, data, len, hw_hdr->freq);
		}
#endif
		break;
	case ATBM_WLAN_CATEGORY_HT:
		{
			//add 8.5.12.2 Notify Channel Width frame format
			if(ATBM_WLAN_HT_ACTION_NOTIFY_CHANWIDTH == mgmt->u.action.u.notify_chan_width.action_code)
			{
				atbm_uint8 prev_lock = priv->bss.ht_40M;
				atbm_uint8 new_lock  = 0;
				if(mgmt->u.action.u.notify_chan_width.chan_width)
				{
					/*
					*if not surport ht40, we should not receive this frame.
					*/
					if(priv->bss.channel_type<=ATBM_NL80211_CHAN_HT20){
						wifi_printk(WIFI_DBG_ERROR, "%s:in 20M mode,we can not recive this frame,what happend",__func__); 
						priv->bss.ht_40M=0;
					}
				}
				else
				{
					if(priv->bss.channel_type>=ATBM_NL80211_CHAN_HT40MINUS){
						priv->bss.ht_40M=1;
					}
				}
				new_lock =priv->bss.ht_40M;
				
				if(prev_lock^new_lock)
				{					
					atbm_queue_work(priv->hw_priv,priv->chantype_switch_work);
				}	
			}
		}
		break;
#if CONFIG_IEEE80211W
	case ATBM_WLAN_CATEGORY_SA_QUERY:
		{
			switch (mgmt->u.action.u.sa_query.action){
			case ATBM_WLAN_ACTION_SA_QUERY_REQUEST:
				if (priv->iftype != ATBM_NL80211_IFTYPE_STATION)
					break;
				atbmwifi_ieee80211_process_sa_query_req(priv, mgmt, ATBM_OS_SKB_LEN(skb));
			}
		}
		break;
#endif
	default:
		break;
	}
}

/*
is bssid in the scan result(hw_priv->scan_ret.info),
if in return  0;
not return  1;
*/
ATBM_BOOL atbmwifi_rx_scan_is_valid(struct atbmwifi_vif *priv,
						struct atbmwifi_ieee80211_mgmt *mgmt/*atbm_uint8 * ie*/)
{
	int i;

	for(i=0;i<priv->scan_ret.len;i++ ){
		if(!atbm_memcmp(mgmt->bssid, priv->scan_ret.info[i].BSSID,6)){
			return ATBM_FALSE;
		}
	}
	return ATBM_TRUE;
}

 static ATBM_BOOL is_uapsd_supported(struct atbmwifi_ieee802_11_elems *elems)
 {
	 atbm_uint8 qos_info;
 
	 if (elems->wmm_info && elems->wmm_info_len == 7
		 && elems->wmm_info[5] == 1)
		 qos_info = elems->wmm_info[6];
	 else if (elems->wmm_param && elems->wmm_param_len == 24
		  && elems->wmm_param[5] == 1)
		 qos_info = elems->wmm_param[6];
	 else
		 /* no valid wmm information or parameter element found */
		 return ATBM_FALSE;
 
	 return qos_info & ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK;
 }

 atbm_void atbmwifi_ap_rx_probe_resp(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{

	atbm_uint8 * data = (atbm_uint8 *)ATBM_OS_SKB_DATA(skb)+offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
	int len = ATBM_OS_SKB_LEN(skb)-offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
	atbm_uint8 *ie ;
//	ATBM_BOOL privacy = 0;
	struct atbmwifi_ieee802_11_elems * pelems;
	struct atbmwifi_ieee80211_rx_status *hwhdr = ATBM_IEEE80211_SKB_RXCB(skb);
	int ret = -1;
	pelems = (struct atbmwifi_ieee802_11_elems *)atbm_kmalloc(sizeof(struct atbmwifi_ieee802_11_elems),GFP_KERNEL);

	if(pelems==ATBM_NULL){
		ret = -101;
		goto __error; 
	}
	ie = atbmwifi_find_ie(ATBM_WLAN_EID_SSID,data,len);	
	if(ie==ATBM_NULL){
		ret = -100;
		goto __error; 
	}
	if(priv->scan.ApScan_in_process){
		if((priv->scan_ret.info)
			&&(priv->scan_no_connect)
			&&(priv->scan_ret.len < MAX_SCAN_INFO_NUM)){
			if(atbmwifi_rx_scan_is_valid(priv,mgmt)){					
				struct atbmwifi_scan_result_info *info = priv->scan_ret.info + priv->scan_ret.len;
				atbm_ieee802_11_parse_elems(data,len,pelems);
				if(pelems->ssid_len){
					atbm_memcpy(info->ssid ,pelems->ssid,pelems->ssid_len);
				}
				atbm_memcpy(info->BSSID,mgmt->bssid,6);
				info->ssid[pelems->ssid_len] = 0;
				info->ssidlen = pelems->ssid_len;
				if(pelems->ds_params)
					info->channel = pelems->ds_params[0];
				if(pelems->tim)
					info->dtim_period = pelems->tim->dtim_period;
				info->beacon_interval = mgmt->u.probe_resp.beacon_int;
				info->capability  = mgmt->u.probe_resp.capab_info;
				info->ht = pelems->ht_cap_elem?1:(pelems->ht_info_elem?1:0);
				info->wpa= pelems->wpa ?1:0;
				info->rsn= pelems->rsn?1:0;
				info->encrypt = (mgmt->u.probe_resp.capab_info& ATBM_WLAN_CAPABILITY_PRIVACY)?1:0;
				info->rssi = hwhdr->signal;
				//info->b40M= ?1:0;
				priv->scan_ret.len++;
				if(priv->scan_ret.len==1){
					wifi_printk(WIFI_ALWAYS,"scan_no_connect: %d\n",priv->scan_no_connect);
				}
				wifi_printk(WIFI_ALWAYS,"SSID: %s\n",info->ssid);
				wifi_printk(WIFI_ALWAYS,"	channel %d\n",info->channel);
				wifi_printk(WIFI_ALWAYS,"	ht[%d] wpa[%d] rsn[%d] enc[%d]\n",info->ht,info->wpa,info->rsn,info->encrypt);
			}
		}
	}
	atbm_kfree(pelems);
	return;
__error:
	wifi_printk(WIFI_DBG_MSG,"%s++ ret %d\n",__FUNCTION__,ret);
	atbm_kfree(pelems);
	return;
}

 atbm_int8 atbmwifi_get_security_mode(atbm_int32 protect, struct atbmwifi_ieee802_11_elems *elems){
	//struct atbmwifi_wpa_ie_data ie;

	if(!protect){
		return ATBM_KEY_NONE;
	}
	if(elems->wpa){
		if(elems->rsn){
			return ATBM_KEY_MIX;
		}
		return ATBM_KEY_WPA;
	}
	if(elems->rsn){
		return ATBM_KEY_WPA2;
	}
	return ATBM_KEY_WEP;
}

 atbm_void atbmwifi_rx_probe_resp(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	//struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	atbm_uint8 * data = (atbm_uint8 *)ATBM_OS_SKB_DATA(skb)+offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
	int len = ATBM_OS_SKB_LEN(skb)-offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
#if CONFIG_WPS
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	struct wps_parse_attr *attr = NULL;
#endif
	atbm_uint8 *ie ;
	ATBM_BOOL privacy = 0;
	struct atbmwifi_ieee802_11_elems * pelems;
	struct atbmwifi_ieee80211_rx_status *hwhdr = ATBM_IEEE80211_SKB_RXCB(skb);
	int ret = -1;

	
	pelems = (struct atbmwifi_ieee802_11_elems *)atbm_kmalloc(sizeof(struct atbmwifi_ieee802_11_elems),GFP_KERNEL);

	if(pelems==ATBM_NULL){
		ret = -101;
		goto __error; 
	}
	ie = atbmwifi_find_ie(ATBM_WLAN_EID_SSID,data,len);	
	if(ie==ATBM_NULL){
		ret = -100;
		goto __error; 
	}
	 //in scan process we get scan info
	 //in no scan process we check info
	 if(priv->scan.in_progress||priv->scan.ApScan_in_process){
#if CONFIG_P2P
		if(priv->p2p_scan){
			atbm_p2p_prash_ie(priv, mgmt->bssid, hwhdr->freq, hwhdr->signal, data, len);
		}else
#endif
#if CONFIG_WPS
		if(wpa_s->wps_mode != WPS_MODE_UNKNOWN){
			attr = (struct wps_parse_attr *)atbm_kmalloc(sizeof(*attr),GFP_KERNEL);
			if(attr == ATBM_NULL){
				ret = -200;
				goto __error;
			}
			atbm_ieee802_11_parse_elems(data, len, pelems);
			if(wps_parse_msg_attr(pelems->wps_ie, pelems->wps_ie_len, attr)){
				wifi_printk(WIFI_DBG_ERROR, "wps parse failed.\n");
			}

#if CONFIG_P2P
			if(priv->p2p_join){
				if(priv->ssid_length != pelems->ssid_len ||
					atbm_memcmp(priv->ssid, pelems->ssid, priv->ssid_length)){
					ret = -200;
					goto __error;
				}
			}
#endif
			if(check_valid_wps_ap(priv, attr, wpa_s->own_addr)){
//			if((attr->selected_registrar) &&
//				(*(attr->selected_registrar) == 1) &&
//				(attr->primary_dev_type != ATBM_NULL)){

				priv->bss.sta_priv.capability = mgmt->u.probe_resp.capab_info;
				priv->bss.sta_priv.beacon_interval = mgmt->u.probe_resp.beacon_int;
				if(pelems->tim){
					priv->bss.dtim_period = pelems->tim->dtim_period;
				}else{
					priv->bss.dtim_period = 0;
				}
				if(pelems->erp_info){
					priv->bss.sta_priv.short_preamble = (pelems->erp_info[0] & ATBM_WLAN_ERP_BARKER_PREAMBLE) == 0;
				}
				priv->bss.sta_priv.wpa = (pelems->rsn_len || pelems->wpa_len) ? 1 : 0;
				priv->bss.sta_priv.ht = (pelems->ht_info_elem || pelems->ht_cap_elem) ? 1 : 0;
				priv->bss.sta_priv.rate.ht = priv->bss.sta_priv.ht;
				if(pelems->ht_cap_elem){
					atbmwifi_ieee80211_ht_cap_ie_to_sta_ht_cap(&atbmwifi_band_2ghz, pelems->ht_cap_elem, &priv->bss.sta_priv.rate.ht_cap);
				}
				priv->bss.sta_priv.rate.basic_rates = 0;
				priv->bss.sta_priv.rate.support_rates = 0;
				atbmwifi_ieee80211_get_sta_rateinfo(&priv->bss.sta_priv.rate, &atbmwifi_band_2ghz,
								pelems->supp_rates, pelems->supp_rates_len);
				atbmwifi_ieee80211_get_sta_rateinfo(&priv->bss.sta_priv.rate, &atbmwifi_band_2ghz,
								pelems->ext_supp_rates, pelems->ext_supp_rates_len);
				wifi_printk(WIFI_WPS, "priv->bss.sta_priv.rate support_rates(%x) %x \n", priv->bss.sta_priv.rate.support_rates, priv->bss.sta_priv.rate.basic_rates);
				wifi_printk(WIFI_WPS, "probe_resp wpa_len(%d), wpa(%d) \n", pelems->wpa_len, priv->bss.sta_priv.wpa);
				//free the element ie
				if(priv->bss.information_elements && 
					(priv->bss.len_information_elements < len)){
					atbm_kfree(priv->bss.information_elements);
					priv->bss.information_elements = ATBM_NULL;
				}
				if(priv->bss.information_elements == ATBM_NULL){
					priv->bss.information_elements = (atbm_uint8 *)atbm_kmalloc(len, GFP_KERNEL);
					if(priv->bss.information_elements == ATBM_NULL){
						ret = -205;
						goto __error;
					}
				}
				atbm_memcpy(priv->bss.information_elements, data, len);
				priv->bss.len_information_elements = len;
				if((atbm_memcmp(priv->bssid, mgmt->bssid, ATBM_ETH_ALEN) != 0)
					|| (wpa_s->wps_ap_cnt == 0)){
					wpa_s->wps_ap_cnt++;
				}
				atbm_memcpy(priv->daddr, mgmt->bssid, 6);
				atbm_memcpy(priv->bssid, mgmt->bssid, 6);
				atbm_memcpy(priv->ssid, pelems->ssid, pelems->ssid_len);
				priv->ssid[pelems->ssid_len] = '\0';
				priv->ssid_length = pelems->ssid_len;
				atbm_memcpy(priv->config.ssid, pelems->ssid, pelems->ssid_len);
				priv->config.ssid_len = pelems->ssid_len;

				priv->scan.status = 1;
				wifi_printk(WIFI_WPS, "Found wps AP ssid=%s "MACSTR"<===\n", (char*)priv->ssid, MAC2STR(priv->bssid));
			}
			if(attr != ATBM_NULL){
				atbm_kfree(attr);
			}
			//goto out;
		}
#endif

		if((priv->scan_ret.info)
			&&(priv->scan_no_connect)
			&&(priv->scan_ret.len < MAX_SCAN_INFO_NUM)){
			if(atbmwifi_rx_scan_is_valid(priv,mgmt)){					
				struct atbmwifi_scan_result_info *info = priv->scan_ret.info + priv->scan_ret.len;
				atbm_ieee802_11_parse_elems(data,len,pelems);
				atbm_memcpy(info->ssid ,pelems->ssid,pelems->ssid_len);
				atbm_memcpy(info->BSSID,mgmt->bssid,6);
				info->ssid[pelems->ssid_len] = 0;
				info->ssidlen = pelems->ssid_len;
				if( pelems->ds_params!=ATBM_NULL){
					priv->bss.channel_num =  pelems->ds_params[0];
				}else{
					priv->bss.channel_num = atbmwifi_ieee80211_frequency_to_channel(hwhdr->freq);
				}
#if CONFIG_5G_SUPPORT
				if(priv->scan.cur_scan_req && priv->scan.cur_scan_req->req_type == ATBM_SCAN_5G_PASSAVE_DFS){
					atbm_set_bit(info->channel, priv->scan.dfs_map);
				}
#endif
				if(pelems->tim)
					info->dtim_period = pelems->tim->dtim_period;
				info->beacon_interval = mgmt->u.probe_resp.beacon_int;
				info->capability  = mgmt->u.probe_resp.capab_info;
				info->ht = pelems->ht_cap_elem?1:(pelems->ht_info_elem?1:0);
				info->wpa= pelems->wpa ?1:0;
				info->rsn= pelems->rsn?1:0;
				info->encrypt = (mgmt->u.probe_resp.capab_info& ATBM_WLAN_CAPABILITY_PRIVACY)?1:0;
				info->rssi = hwhdr->signal;
				info->channel = priv->bss.channel_num;
				info->security = atbmwifi_get_security_mode(info->encrypt, pelems);
				//info->b40M= ?1:0;
				priv->scan_ret.len++;
				if(priv->scan_ret.len==1){
					wifi_printk(WIFI_ALWAYS,"scan_no_connect: %d\n",priv->scan_no_connect);
				}
				//wifi_printk(WIFI_ALWAYS,"SSID: %s\n",info->ssid);
				//wifi_printk(WIFI_ALWAYS,"	channel %d\n",info->channel);
				//wifi_printk(WIFI_ALWAYS,"	ht[%d] wpa[%d] rsn[%d] enc[%d] security[%d]\n",info->ht,info->wpa,info->rsn,info->encrypt,info->security);
			}
			goto out;
		}

		if(!priv->scan_no_connect
			&&(priv->config.ssid_len != 0)
			&&(ie[1] == priv->config.ssid_len)
			&&(atbm_memcmp(&ie[2],priv->config.ssid,priv->config.ssid_len)==0)){	
			/*||
			(!priv->connect_ok
			&&(priv->config.ssid_len)
			&&(ie[1] == priv->config.ssid_len)
			&&(atbm_memcmp(&ie[2],priv->config.ssid,priv->config.ssid_len)==0))){*/	
			if(atbm_is_valid_ether_addr(priv->config.bssid)
				&& atbm_memcmp(mgmt->bssid,priv->config.bssid,6)){
					goto out;
			}
			ret = 0;
			
			atbm_ieee802_11_parse_elems(data,len,pelems);
			atbm_memcpy(priv->daddr,mgmt->bssid,6);
		    atbm_memcpy(priv->bssid,mgmt->bssid,6);
			priv->bss.sta_priv.capability =  mgmt->u.probe_resp.capab_info;
			privacy = !!(priv->bss.sta_priv.capability & ATBM_WLAN_CAPABILITY_PRIVACY);
			#ifdef STA_KEY_TYPE_SET
			if(privacy != config->privacy){
				ret = -2;
				wifi_printk(WIFI_ALWAYS,"privacy miss:\n");
				goto __error;
			}
			if(((config->wpa == ATBM_WPA_PROTO_RSN) ||(config->wpa == ATBM_WPA_PROTO_WPA))
				&&(pelems->rsn_len + pelems->wpa_len ==0)){
				ret = -3;
				wifi_printk(WIFI_DBG_ERROR,"%s key_mgmt %d,rsn_len %d,wpa_len %d\n",__FUNCTION__,config->wpa,pelems->rsn_len
					,pelems->wpa_len);
				goto __error;
			}
			#endif
			priv->bss.sta_priv.beacon_interval =  mgmt->u.probe_resp.beacon_int;
			//ie = atbmwifi_find_ie(ATBM_WLAN_EID_DS_PARAMS,data,len);
				
			if( pelems->ds_params!=ATBM_NULL){
				priv->bss.channel_num =  pelems->ds_params[0];
			}else{
				priv->bss.channel_num = atbmwifi_ieee80211_frequency_to_channel(hwhdr->freq);
			}

#if CONFIG_5G_SUPPORT
			if(priv->bss.channel_num >= 36)
				priv->bss.sta_priv.band = ATBM_IEEE80211_BAND_5GHZ;
			else
#endif
				priv->bss.sta_priv.band = ATBM_IEEE80211_BAND_2GHZ;

			atbm_memcpy(priv->bss.bssid, mgmt->bssid,6);
			if(pelems->tim)
				priv->bss.dtim_period = pelems->tim->dtim_period;
			else
				priv->bss.dtim_period = 0;
			if(pelems->erp_info)
				priv->bss.sta_priv.short_preamble = (pelems->erp_info[0]& ATBM_WLAN_ERP_BARKER_PREAMBLE) == 0;
			
			priv->bss.rssi = hwhdr->signal;
			/*		
			if (erp_valid) {
				use_protection = (erp & WLAN_ERP_USE_PROTECTION) != 0;
				use_short_preamble = (erp & ATBM_WLAN_ERP_BARKER_PREAMBLE) == 0;
			} else {
				use_protection = ATBM_FALSE;
				use_short_preamble = !!(capab & ATBM_WLAN_CAPABILITY_SHORT_PREAMBLE);
			}
			*/
			priv->bss.sta_priv.wpa = !!(pelems->rsn_len || pelems->wpa_len);
#if CONFIG_WPS
			priv->bss.sta_priv.wps = pelems->wps_ie_len != 0;
#endif
			priv->bss.privacy = privacy;
			//priv->bss.sta_priv.wps = pelems->wps_ie_len ? 1:0;
			//priv->bss.sta_priv.p2p = pelems->p2p_ie_len ? 1:0;
			//priv->bss.sta_priv.bcm_ap = elems->bcm_ie_len ? 1:0;	
			priv->bss.sta_priv.ht =  (pelems->ht_info_elem || pelems->ht_cap_elem)? 1:0;
			priv->bss.sta_priv.rate.ht = priv->bss.sta_priv.ht;
			priv->bss.sta_priv.wmm_used = (pelems->wmm_param || pelems->wmm_info) ? 1 : 0;
			priv->bss.sta_priv.uapsd_supported = is_uapsd_supported(pelems);
			if(pelems->ht_cap_elem){
				atbmwifi_ieee80211_ht_cap_ie_to_sta_ht_cap(priv->hw_priv->bands[priv->bss.sta_priv.band],
							 	 pelems->ht_cap_elem,
							  	&priv->bss.sta_priv.rate.ht_cap);
			}
			
			atbmwifi_ieee80211_get_sta_rateinfo(&priv->bss.sta_priv.rate,priv->hw_priv->bands[priv->bss.sta_priv.band],pelems->supp_rates, pelems->supp_rates_len);
			atbmwifi_ieee80211_get_sta_rateinfo(&priv->bss.sta_priv.rate,priv->hw_priv->bands[priv->bss.sta_priv.band],pelems->ext_supp_rates, pelems->ext_supp_rates_len);

			//wifi_printk(WIFI_SCAN,"probe_resp wpa_len(%d),wpa(%d) \n",pelems->wpa_len,priv->bss.sta_priv.wpa);
			//free the element ie
			if(priv->bss.information_elements &&
				(priv->bss.len_information_elements < len)){
				atbm_kfree(priv->bss.information_elements);
				priv->bss.information_elements = ATBM_NULL;
			}
			
			if(priv->bss.information_elements == ATBM_NULL){
				priv->bss.information_elements = (atbm_uint8 *)atbm_kmalloc(len,GFP_KERNEL);
				if(priv->bss.information_elements==ATBM_NULL)
				{
					ret = -5;

					wifi_printk(WIFI_ALWAYS,"information_elements miss:\n");
					goto __error;
				}
			}
			atbm_memcpy(priv->bss.information_elements,data,len);
			priv->bss.len_information_elements =  len;
			
			
			priv->scan.status = ATBMWIFI_SCAN_CONNECT_AP_SUCCESS;	
			
			wifi_printk(WIFI_ALWAYS,"[atbm_wifi] connect AP have been Scanned\n");
			//wifi_printk(WIFI_RATE,"support_rates %x,basic_rates %x\n",priv->bss.support_rates,priv->bss.basic_rates);		
		
		}
	
	}
	 else {
	 	//TODO add to check beacon info code,if ap beacon info change we need disconnect it
	 }

#if CONFIG_5G_SUPPORT
	 if(priv->scan.cur_scan_req && priv->scan.cur_scan_req->req_type == ATBM_SCAN_5G_PASSAVE_DFS){
		 ie = atbmwifi_find_ie(ATBM_WLAN_EID_DS_PARAMS,data,len);
		 if(ie && ie[2])
		 	atbm_set_bit(ie[2], priv->scan.dfs_map);
		 else
		 	atbm_set_bit(atbmwifi_ieee80211_frequency_to_channel(hwhdr->freq), priv->scan.dfs_map);
	 }
#endif

	 atbm_kfree(pelems);
	return;
__error:
#if CONFIG_WPS
	if(attr != ATBM_NULL)
		atbm_kfree(attr);
#endif
	wifi_printk(WIFI_DBG_MSG,"%s++ ret %d\n",__FUNCTION__,ret);
out:	
	atbm_kfree(pelems);
	return;
}
 atbm_void atbmwifi_switch_channletpye(struct atbmwifi_vif *priv,struct atbmwifi_ieee802_11_elems *elems)
{
	struct atbmwifi_ieee80211_channel_sw_packed_ie sw_packed_ie;
	if(elems->ch_switch_elem)
	{
		sw_packed_ie.chan_sw_ie = (struct atbmwifi_ieee80211_channel_sw_ie *)elems->ch_switch_elem;
		atbm_ieee80211_sta_process_chanswitch(priv,
								&priv->bss,
								&sw_packed_ie,
								0);
	}
	if(elems->secondary_ch_elem){
		sw_packed_ie.sec_chan_offs_ie= (struct atbmwifi_ieee80211_sec_chan_offs_ie *)elems->secondary_ch_elem;
		atbm_ieee80211_sta_process_chanswitch(priv,
								&priv->bss,
								&sw_packed_ie,
								0);
	}
	if(elems->extended_ch_switch_elem)
	{
		sw_packed_ie.ex_chan_sw_ie= (struct atbmwifi_ieee80211_ext_chansw_ie *)elems->extended_ch_switch_elem;
		atbm_ieee80211_sta_process_chanswitch(priv,
								&priv->bss,
								&sw_packed_ie,
								0);
	
	}

}

#if SUPPORT_LIGHT_SLEEP
 //Probe response not indicate DTIM period, so it should be updated after connected..
  atbm_void atbmwifi_rx_beacon(struct atbmwifi_vif *priv,struct atbm_buff *skb)
 {
	 int  baselen, len;
	 struct atbmwifi_ieee80211_mgmt *mgmt;
	 struct atbmwifi_ieee802_11_elems elems;
 
	 if(priv->assoc_ok ==0)
		 return;
 
	 elems.tim = ATBM_NULL;
	 if(!priv->bss.dtim_period){
		 len = ATBM_OS_SKB_LEN(skb)-offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
		 mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
		 baselen = (atbm_uint8 *) mgmt->u.beacon.variable - (atbm_uint8 *) mgmt;
		 atbm_ieee802_11_parse_elems(mgmt->u.beacon.variable, len - baselen, &elems);
		 if(elems.tim){
			 wifi_printk(WIFI_DBG_ERROR, "beacon dtim_period:%d beacon_interval:%d\n", priv->bss.dtim_period, priv->bss.sta_priv.beacon_interval);
			 wsm_set_beacon_wakeup_period(_atbmwifi_vifpriv_to_hwpriv(priv),
						  priv->bss.sta_priv.beacon_interval * priv->bss.dtim_period >
						 MAX_BEACON_SKIP_TIME_MS ? 1 :
						 priv->bss.dtim_period, priv->bss.sta_priv.beacon_interval, priv->if_id);
		 }
	 }
 }
#else
 atbm_void atbmwifi_rx_beacon(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
//	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct atbmwifi_ieee802_11_elems elems;
	int erp_valid =ATBM_FALSE;
	atbm_uint8 erp_value = 0;
	int  baselen;
	int changed = 0;
	int len = ATBM_OS_SKB_LEN(skb)-offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
	/* Process beacon from the current BSS */
	baselen = (atbm_uint8 *) mgmt->u.beacon.variable - (atbm_uint8 *) mgmt;
	if(priv->assoc_ok == 0){
		return ;
	}else{
		atbm_ieee802_11_parse_elems(mgmt->u.beacon.variable, len - baselen, &elems);
		/*parse 40M ies*/
		atbmwifi_switch_channletpye(priv,&elems);
		/*parse wmm param*/
		if (elems.wmm_param){
			atbmwifi_ieee80211_sta_wmm_params(priv,elems.wmm_param,elems.wmm_param_len);
		}
		/*parse ps mode */
		//
		if (elems.erp_info && elems.erp_info_len >= 1) {
			erp_valid = ATBM_TRUE;
			erp_value = elems.erp_info[0];
		} else {
			erp_valid = ATBM_FALSE;
		}
		changed |= atbm_ieee80211_handle_bss_capability(priv,atbm_le16_to_cpu(mgmt->u.beacon.capab_info),erp_valid, erp_value);
		if (elems.ht_cap_elem && elems.ht_info_elem && elems.wmm_param &&
		    !(config->flags & ATBM_IEEE80211_STA_DISABLE_11N)) {
			struct atbmwifi_ieee80211_supported_band *sband;
			atbm_uint16 ap_ht_cap_flags;
			sband = priv->hw_priv->bands[priv->bss.sta_priv.band];

			atbmwifi_ieee80211_ht_cap_ie_to_sta_ht_cap(sband,
					elems.ht_cap_elem, &priv->bss.sta_priv.rate.ht_cap);

			ap_ht_cap_flags = priv->bss.sta_priv.rate.ht_cap.cap;

			changed |= atbmwifi_ieee80211_enable_ht(elems.ht_info_elem,
						       priv, ap_ht_cap_flags, ATBM_TRUE);
		}
		/* Note: country IE parsing is done for us by cfg80211 */
		if (elems.country_elem) {
			///TODO;;
		}
		atbmwifi_ieee80211_bss_info_change_notify(priv, changed);
	}
	return;
}
#endif

 atbm_void atbmwifi_rx_sta_mgmtframe(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{	
	struct atbmwifi_ieee80211_hdr * hdr = (struct atbmwifi_ieee80211_hdr *) ATBM_OS_SKB_DATA(skb);
	atbm_uint16 stype = hdr->frame_control & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_STYPE);	
	int ret= 0;
	wifi_printk(WIFI_DBG_MSG,"atbmwifi_rx_sta_mgmtframe++ stype %x\n",stype);
     switch(stype){
	 	case ATBM_IEEE80211_STYPE_BEACON:
#if SUPPORT_LIGHT_SLEEP
			atbmwifi_rx_beacon(priv,skb);
#else
			//atbmwifi_rx_beacon(priv,skb);
#endif
		case ATBM_IEEE80211_STYPE_PROBE_RESP:
			atbmwifi_rx_probe_resp(priv,skb);
			break;
		case ATBM_IEEE80211_STYPE_AUTH:
			{
				int res;
				res = atbmwifi_rx_authen(priv,skb);
				if(res == ATBM_WLAN_STATUS_SUCCESS_NEXTSETP){
					//send assoc req
					atbmwifi_event_uplayer(priv,ATBM_WIFI_AUTH_EVENT,(atbm_uint8*)0);
				}
				else if(res>0)
				{
					wifi_printk(WIFI_CONNECT,"[sta]:rx_sta_mgm AUTH fail just DEAUTH\n");
					atbmwifi_event_uplayer(priv,ATBM_WIFI_DEAUTH_EVENT,(atbm_uint8*)&res);
				}
				else {
					//drop
				}
			}
			break;
		case ATBM_IEEE80211_STYPE_ASSOC_RESP:
		case ATBM_IEEE80211_STYPE_REASSOC_RESP:
			ret= atbmwifi_rx_assoc_rsp(priv,skb);
			if(!ret)
			{
				atbmwifi_assoc_success(priv,skb);
			}
			else
			{
				priv->assoc_ok = 0;
				priv->connect_ok = 0;

				atbmwifi_event_uplayer(priv,ATBM_WIFI_DEASSOC_EVENT,0);
				if(priv->auto_connect_when_lost){
					atbmwifi_autoconnect(priv, 0);
				}
			}			
			break;
		case ATBM_IEEE80211_STYPE_DEAUTH:
		case ATBM_IEEE80211_STYPE_DISASSOC:
			if(priv->assoc_ok || priv->join_status == ATBMWIFI__JOIN_STATUS_STA){
				wifi_printk(WIFI_CONNECT,"[sta]:rx_sta_mgm DEAUTH\n");
				wifi_printk(WIFI_ALWAYS,"atbmwifi_rx_sta_mgmtframe() ---deauth\n");
#if FAST_CONNECT_MODE
				if(priv->fast_channel){
					priv->fast_connect = 1;
				}
#endif
#if FAST_CONNECT_NO_SCAN
				if(priv->auth_retry && !priv->assoc_ok){
					priv->auth_retry = 0;
					atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,ATBM_NULL,WPA_EVENT__SUPPLICANT_AUTHEN,ATBM_WPA_EVENT_NOACK);
					break;
				}
#endif
				sta_deauth(priv);
			}
			//atbmwifi_event_uplayer(priv,ATBM_WIFI_DEAUTH_EVENT,0);
			break;
#if CONFIG_P2P
		case ATBM_IEEE80211_STYPE_PROBE_REQ:
			{
				struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *)ATBM_OS_SKB_DATA(skb);
				struct atbmwifi_ieee80211_rx_status *hw_hdr = ATBM_IEEE80211_SKB_RXCB(skb);
				atbm_uint8 *data = (atbm_uint8 *)ATBM_OS_SKB_DATA(skb) + offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_req.variable);
				int len = ATBM_OS_SKB_LEN(skb) - offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_req.variable);
				atbm_p2p_probe_req_rx(priv, mgmt->sa, mgmt->da, mgmt->bssid,
					data, len, hw_hdr->freq, (int)hw_hdr->signal);
			}
			break;
#endif
		default:
			break;
	}
}

#if CONFIG_SAE
#define WPA_EVENT_SAE_UNKNOWN_PASSWORD_IDENTIFIER \
	 "CTRL-EVENT-SAE-UNKNOWN-PASSWORD-IDENTIFIER "
 
static int sme_sae_auth(struct wpa_supplicant *wpa_s, atbm_uint16 auth_transaction,
			atbm_uint16 status_code, const atbm_uint8 *data, atbm_size_t len,
			const atbm_uint8 *sa)
{
	int *groups;
 
	wpa_printf(MSG_DEBUG, "SME: SAE authentication transaction %u "
		"status code %u", auth_transaction, status_code);
 
	if (auth_transaction == 1 &&
			status_code == ATBM_WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ &&
			wpa_s->sae.state == SAE_COMMITTED){
		int default_groups[] = { 19, 20, 21, 0 };
		atbm_uint16 group;
 
		groups = wpa_s->sae_groups;
		if (!groups || groups[0] <= 0)
			groups = default_groups;
 
		if (len < sizeof(atbm_uint16)) {
			wpa_printf(MSG_DEBUG,
				"SME: Too short SAE anti-clogging token request");
			return -1;
		}
		group = ATBM_WPA_GET_LE16(data);
		wpa_printf(MSG_DEBUG,
			"SME: SAE anti-clogging token requested (group %u)",
			group);
		if (sae_group_allowed(&wpa_s->sae, groups, group) !=
			ATBM_WLAN_STATUS_SUCCESS) {
			wpa_printf(MSG_ERROR,
				"SME: SAE group %u of anti-clogging request is invalid",
				group);
			return -1;
		}
		wpabuf_free(wpa_s->sae_token);
		wpa_s->sae_token = wpabuf_alloc_copy(data + sizeof(atbm_uint16),
							 len - sizeof(atbm_uint16));
 // 	 sme_send_authentication(wpa_s, wpa_s->current_bss,
 // 				 wpa_s->current_ssid, 2);
		wpa_s->sae_start = 2;
		wpa_prepare_auth(wpa_s->priv);
	}
 
	if (auth_transaction == 1 &&
			status_code == ATBM_WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED &&
			wpa_s->sae.state == SAE_COMMITTED) {
		wpa_printf(MSG_DEBUG, "SME: SAE group not supported");
		wpa_s->sae_group_index++;
		if (sme_set_sae_group(wpa_s) < 0)
			return -1; /* no other groups enabled */
		wpa_printf(MSG_DEBUG, "SME: Try next enabled SAE group");
 // 	 sme_send_authentication(wpa_s, wpa_s->current_bss,
 // 				 wpa_s->current_ssid, 1);
		wpa_s->sae_start = 1;
		wpa_prepare_auth(wpa_s->priv);
	}
 
	if (auth_transaction == 1 &&
		status_code == ATBM_WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER) {
		const atbm_uint8 *bssid = sa ? sa : wpa_s->priv->bss.bssid;
 
		wpa_printf(MSG_INFO,
			WPA_EVENT_SAE_UNKNOWN_PASSWORD_IDENTIFIER MACSTR,
			MAC2STR(bssid));
		return -1;
	}
 
	if (status_code != ATBM_WLAN_STATUS_SUCCESS)
		return -1;
 
	if (auth_transaction == 1) {
		atbm_uint16 res;
		int default_groups[] = { 19, 20, 21, 0 };

		groups = wpa_s->sae_groups;
		if (!groups || groups[0] <= 0)
			groups = default_groups;
 
		wpa_printf(MSG_DEBUG, "SME SAE commit");
 // 	 if (wpa_s->current_bss == NULL ||
 // 		 wpa_s->current_ssid == NULL)
 // 		 return -1;
		if (wpa_s->sae.state != SAE_COMMITTED)
			return -1;
		if (groups && groups[0] <= 0)
			groups = NULL;
		res = sae_parse_commit(&wpa_s->sae, data, len, NULL, NULL,
						groups);
		if (res == SAE_SILENTLY_DISCARD) {
			wpa_printf(MSG_DEBUG,
					"SAE: Drop commit message due to reflection attack");
			return 0;
		}
		if (res != ATBM_WLAN_STATUS_SUCCESS)
			return -1;
 
		if (sae_process_commit(&wpa_s->sae) < 0) {
			wpa_printf(MSG_DEBUG, "SAE: Failed to process peer "
					"commit");
			return -1;
		}
 
		wpabuf_free(wpa_s->sae_token);
		wpa_s->sae_token = NULL;
		wpa_s->sae_start = 0;
 // 	 sme_send_authentication(wpa_s, wpa_s->current_bss,
 // 				 wpa_s->current_ssid, 0);
		wpa_prepare_auth(wpa_s->priv);
		return 0;
	 } else if (auth_transaction == 2) {
		wpa_printf(MSG_DEBUG, "SME SAE confirm");
		if (wpa_s->sae.state != SAE_CONFIRMED)
			return -1;
		if (sae_check_confirm(&wpa_s->sae, data, len) < 0)
			return -1;
		wpa_s->sae.state = SAE_ACCEPTED;
		sae_clear_temp_data(&wpa_s->sae);
 
		return 1;
	}
 
	return -1;
}
#endif
 int atbmwifi_rx_authen(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	atbm_uint16 auth_alg, auth_transaction, status_code=-1;
	atbm_uint16 fc; 
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
//	atbm_uint8 * data = (atbm_uint8 *)OS_SKB_DATA(skb)+offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_resp.variable);
	int len = ATBM_OS_SKB_LEN(skb)-offsetof(struct atbmwifi_ieee80211_mgmt, u.auth.variable);
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
//	atbm_uint16 resp = ATBM_WLAN_STATUS_SUCCESS;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	atbm_uint8 *resp_ies = ATBM_NULL;
	atbm_size_t resp_ies_len = 0;
	struct atbm_buff *skb_tx = ATBM_NULL;
#if CONFIG_SAE
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
#endif

	auth_alg = mgmt->u.auth.auth_alg;
	auth_transaction = mgmt->u.auth.auth_transaction;
	status_code = mgmt->u.auth.status_code;
	fc = mgmt->frame_control;

	if(status_code != ATBM_WLAN_STATUS_SUCCESS)
	{
		return 	status_code;
	}
	if(auth_alg != config->auth_alg)
	{
		return 	ATBM_WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
	}

	if(auth_alg == ATBM_WLAN_AUTH_OPEN)
	{
		return ATBM_WLAN_STATUS_SUCCESS_NEXTSETP;
	}
	else if((auth_alg == ATBM_WLAN_AUTH_SHARED_KEY))
	{
		switch(auth_transaction)
		{
			case 1:
				return -2;
			case 2:
			//if(priv->connect_state != WAIT_AUTH_4)
			{
				wifi_printk(WIFI_DBG_MSG,"atbmwifi_rx_sta_mgmtframe++:authen len(%d) \n",len);
				if ((len >= 2 + ATBM_WLAN_AUTH_CHALLENGE_LEN) &&
	    		(mgmt->u.auth.variable[0] == ATBM_WLAN_EID_CHALLENGE) &&
	    		(mgmt->u.auth.variable[1] == ATBM_WLAN_AUTH_CHALLENGE_LEN))
				{
					resp_ies = (atbm_uint8 *)atbm_kmalloc(2 + ATBM_WLAN_AUTH_CHALLENGE_LEN,GFP_KERNEL);

					if(resp_ies == ATBM_NULL)
					{
						status_code = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
						
						return status_code;
					}
					resp_ies[0] = ATBM_WLAN_EID_CHALLENGE;
					resp_ies[1] = ATBM_WLAN_AUTH_CHALLENGE_LEN;
					atbm_memcpy(resp_ies + 2, &mgmt->u.auth.variable[2],
						  ATBM_WLAN_AUTH_CHALLENGE_LEN);
					resp_ies_len = 2 + ATBM_WLAN_AUTH_CHALLENGE_LEN;
					wifi_printk(WIFI_DBG_MSG,"atbmwifi_rx_sta_mgmtframe++:challenge \n");
					priv->connect_state = WAIT_AUTH_4;
				}
				
				break;
			}
			case 4:
				if(priv->connect_state == WAIT_AUTH_4)
					return ATBM_WLAN_STATUS_SUCCESS_NEXTSETP;
				else 
					return ATBM_WLAN_STATUS_JUST_DROP;
					
		}
	}
#if CONFIG_SAE
	else if((auth_alg == ATBM_WLAN_AUTH_SAE)){
		int res;
		res = sme_sae_auth(wpa_s, mgmt->u.auth.auth_transaction,
				   mgmt->u.auth.status_code, mgmt->u.auth.variable,
				   len,  ATBM_NULL);
		if (res < 0) {
			wpa_deauthen(priv);
			//fixme:reconnect here
			if(priv->auto_connect_when_lost){
				atbmwifi_autoconnect(priv, priv->scan_expire);
			}
		}
		if (res != 1)
			return ATBM_WLAN_STATUS_JUST_DROP;

		wpa_printf(MSG_DEBUG, "SME: SAE completed - setting PMK for "
			   "4-way handshake");

		wpa_sm_set_pmk(wpa_s->wpa, wpa_s->sae.pmk, ATBM_PMK_LEN,
				   wpa_s->sae.pmkid, wpa_s->priv->bss.bssid);

		wpa_s->sae_pmk_set = 1;
		return ATBM_WLAN_STATUS_SUCCESS_NEXTSETP;
	}
#endif
	else {
		return ATBM_WLAN_STATUS_JUST_DROP;
	}

	if(priv->extra_ie){
		atbm_kfree(priv->extra_ie );
	}
	priv->extra_ie = resp_ies;
	priv->extra_ie_len = resp_ies_len;
	skb_tx = atbmwifi_ieee80211_send_auth(priv,auth_transaction+1,auth_alg,mgmt->sa,priv->bssid,0);
	atbmwifi_tx(hw_priv,skb_tx,priv);
	priv->extra_ie = ATBM_NULL;
	priv->extra_ie_len = 0;

	atbm_kfree(resp_ies);
	return status_code;
	
}

 int atbmwifi_ieee80211_build_preq_ies(struct atbmwifi_vif *priv,atbm_uint8 *buffer,
			     const atbm_uint8 *ie, atbm_size_t ie_len,atbm_uint8 channel)
{
	atbm_uint8 *pos;
	atbm_uint8 band;

	pos = buffer;

#if CONFIG_5G_SUPPORT
	if(priv->scan.cur_scan_req && priv->scan.cur_scan_req->scan.band == WSM_PHY_BAND_5G)
		band = WSM_PHY_BAND_5G;
	else
#endif
		band = WSM_PHY_BAND_2_4G;

	/* SSID */
	*pos++ = ATBM_WLAN_EID_SSID;
	*pos++ = 0;
	//*pos++ = priv->ssid_length;
	//atbm_memcpy(pos,priv->ssid, priv->ssid_length);
	//pos += priv->ssid_length;
	
	/* Supported rates */
	/* Extended supported rates */	
	pos = atbmwifi_ieee80211_add_rate_ie(pos ,(band==WSM_PHY_BAND_5G),~0);
	
	
	if (channel ) {
		*pos++ = ATBM_WLAN_EID_DS_PARAMS;
		*pos++ = 1;
		*pos++ = channel;
	}

	pos = atbmwifi_ieee80211_add_ht_ie(priv,priv->hw_priv->bands[band],pos);

#if CONFIG_WPS
	pos = atbmwifi_ieee80211_add_preq_wps_ie(priv, pos);
#endif

#if CONFIG_P2P
	if(priv->p2p_scan){
		pos = atbm_p2p_add_scan_ie(priv, pos);
	}else if(priv->p2p_join){
		if(priv->p2p_assoc_req_ie && priv->p2p_assoc_req_ie_len){
			atbm_memcpy(pos, priv->p2p_assoc_req_ie, priv->p2p_assoc_req_ie_len);
			pos += priv->p2p_assoc_req_ie_len;
		}
	}
#endif

	/* add any remaining custom IEs */
	if (ie && ie_len) {
		atbm_memcpy(pos, ie ,ie_len);
		pos += ie_len;
	}

	return pos - buffer;
}



 struct atbm_buff *atbmwifi_ieee80211_build_probe_req(struct atbmwifi_vif *priv, 
					  atbm_uint8 *dst, const atbm_uint8 *ie, atbm_size_t ie_len)
{
	struct atbm_buff *skb=ATBM_NULL;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	atbm_size_t buf_len;
	atbm_uint8 *buf;
	struct atbmwifi_ieee80211_hdr_3addr *hdr;
	 
	skb = atbm_dev_alloc_skb(1024);
	if (!skb)
		return ATBM_NULL;
		
	hdr = (struct atbmwifi_ieee80211_hdr_3addr *) atbm_skb_put(skb, sizeof(*hdr));
	atbm_memset(hdr, 0, sizeof(*hdr));
	hdr->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
					 ATBM_IEEE80211_STYPE_PROBE_REQ);
	atbm_memcpy(hdr->addr2, priv->mac_addr, ATBM_ETH_ALEN);
	hdr->seq_ctrl  = 0;
	hdr->duration_id = 0;
	if (dst) {
		mgmt = (struct atbmwifi_ieee80211_mgmt *) hdr;
		atbm_memcpy(mgmt->da, dst, ATBM_ETH_ALEN);
		atbm_memcpy(mgmt->bssid, dst, ATBM_ETH_ALEN);
	}
	else {
		atbm_memset(hdr->addr1, 0xff, ATBM_ETH_ALEN);
		atbm_memset(hdr->addr3, 0xff, ATBM_ETH_ALEN);
	}
	buf = (atbm_uint8 *)(hdr + 1);

	buf_len = atbmwifi_ieee80211_build_preq_ies(priv, buf, ie, ie_len,
					  priv->bss.channel_num);

	atbm_skb_put(skb, buf_len);
	
	ATBM_IEEE80211_SKB_TXCB(skb)->flags = ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;

	return skb;
}


 struct atbm_buff * atbmwifi_ieee80211_send_probe_req(struct atbmwifi_vif *priv, atbm_uint8 *dst,
			      const atbm_uint8 *ie, atbm_size_t ie_len, ATBM_BOOL no_cck)
{
	struct atbm_buff *skb;

	skb = atbmwifi_ieee80211_build_probe_req(priv, dst,ie, ie_len);
	if (skb) {
		if (no_cck)
			ATBM_IEEE80211_SKB_TXCB(skb)->flags |=
				ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;
	}
#if CONFIG_P2P
	if(priv->p2pdata){
		ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;
	}
#endif
	return skb;
}

 struct atbm_buff * atbmwifi_ieee80211_send_assoc_req(struct atbmwifi_vif *priv)
{
	//struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	atbm_uint8 *pos;
	//atbm_size_t offset = 0, noffset;
	atbm_uint16 capab;
	atbm_uint32 rates = 0,rates_len;
#ifdef CONFIG_WPS
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
#endif

	/*
	* In case AP not provide any supported rates information
	* before association, we send information element(s) with
	* all rates that we support.
	*/
	rates = priv->bss.sta_priv.rate.support_rates;
	if(rates < 0xf)
		rates = 0xffffffff;
	rates_len = atbmwifi_g_rates_size;

	skb = atbm_dev_alloc_skb(
			sizeof(*mgmt) + /* bit too much but doesn't matter */
			2 + priv->ssid_length+ /* SSID */
			4 + rates_len + /* (extended) rates */
			4 + /* power capability */
			2 + /* supported channels */
			2 + sizeof(struct atbmwifi_ieee80211_ht_cap) + /* HT */
			500 + /* wps IE*/
			priv->extra_ie_len+ /* extra IEs */
			9);/* WMM */
	if (!skb)
		return ATBM_NULL;


	capab = ATBM_WLAN_CAPABILITY_ESS;


	capab |= ATBM_WLAN_CAPABILITY_SHORT_SLOT_TIME;
	capab |= ATBM_WLAN_CAPABILITY_SHORT_PREAMBLE;

	if (config->privacy)
	capab |= ATBM_WLAN_CAPABILITY_PRIVACY;

	mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
	atbm_memset(mgmt, 0, 24);
	
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->da, priv->daddr, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->bssid, priv->bssid, ATBM_ETH_ALEN);


	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
						  ATBM_IEEE80211_STYPE_ASSOC_REQ);
	mgmt->u.assoc_req.capab_info = atbm_cpu_to_le16(capab);
	mgmt->u.assoc_req.listen_interval =
				atbm_cpu_to_le16(priv->bss.sta_priv.beacon_interval);
	pos= mgmt->u.assoc_req.variable;
	
	/* SSID */
	*pos++ = ATBM_WLAN_EID_SSID;
	*pos++ = priv->ssid_length;
	atbm_memcpy(pos,priv->ssid, priv->ssid_length);
	pos += priv->ssid_length;
	
	/* add all rates which were marked to be used above */
	pos = atbmwifi_ieee80211_add_rate_ie_from_ap(pos,(priv->bss.sta_priv.band==ATBM_IEEE80211_BAND_5GHZ),rates,priv->bss.sta_priv.rate.basic_rates);
	/*
	if (priv->bss.channel_num) {
		*pos++ = ATBM_WLAN_EID_DS_PARAMS;
		*pos++ = 1;
		*pos++ = priv->bss.channel_num;
	}*/

	pos = atbmwifi_ieee80211_add_ht_ie(priv,priv->hw_priv->bands[priv->bss.sta_priv.band], pos);

#if CONFIG_P2P
	if(priv->p2p_join){
		if(priv->p2p_assoc_req_ie && priv->p2p_assoc_req_ie_len){
			atbm_memcpy(pos, priv->p2p_assoc_req_ie, priv->p2p_assoc_req_ie_len);
			pos += priv->p2p_assoc_req_ie_len;
		}
	}
#endif

	if (priv->bss.sta_priv.wmm_used){
		pos = (atbm_uint8*)atbmwifi_ieee80211_add_wme(priv,pos);
	}

#if CONFIG_WPS
	if(wpa_s->wps_mode != WPS_MODE_UNKNOWN)
		pos = atbmwifi_ieee80211_add_assocreq_wps_ie(priv, pos);
	else
#endif
	/* if present, add any custom IEs that go before HT */
	if (priv->extra_ie && priv->extra_ie_len) {
		atbm_memcpy(pos, priv->extra_ie, priv->extra_ie_len);
		pos += priv->extra_ie_len;
		wifi_printk(WIFI_DBG_MSG,"assciating:ie(%d)\n", priv->extra_ie_len);
	}
	
	atbm_skb_put(skb, pos-(atbm_uint8 *)mgmt);

#if CONFIG_P2P
	if(priv->p2pdata){
		ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;
	}else
#endif
	{
		ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	}
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;
	return skb;
}

 atbm_void atbmwifi_tx_sta_mgmtframe(struct atbmwifi_vif *priv,atbm_uint16 stype,atbm_uint16 transaction )
{
	
	//struct atbmwifi_ieee80211_hdr *hdr;
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb = ATBM_NULL;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	wifi_printk(WIFI_TX,"atbmwifi_tx_sta_mgmtframe  bssid=%x %x:%x\n",(atbm_uint32)priv->bssid[0],(atbm_uint32)priv->bssid[4],(atbm_uint32)priv->bssid[5]);
	//
	///build mgmt frame 
	//
	switch(stype){
		case ATBM_IEEE80211_STYPE_PROBE_REQ:
			//atbmwifi_ieee80211_send_probe_req(priv,NULL,priv->ssid,priv->ssid_length,priv->extra_ie,priv->extra_ie_len,-1,0);
			break;
		case ATBM_IEEE80211_STYPE_AUTH:
			wifi_printk((WIFI_TX|WIFI_CONNECT),"[sta]:send STYPE_AUTH \n");
			skb = atbmwifi_ieee80211_send_auth(priv,transaction,config->auth_alg,priv->daddr,priv->bssid,0);
			break;
		case ATBM_IEEE80211_STYPE_ASSOC_REQ:
		case ATBM_IEEE80211_STYPE_REASSOC_REQ:
			wifi_printk((WIFI_TX|WIFI_CONNECT),"[sta]:send ASSOC_REQ \n");
			skb = atbmwifi_ieee80211_send_assoc_req(priv);
			break;
		default:
			break;
	}
	if(skb == ATBM_NULL){
		wifi_printk((WIFI_DBG_ERROR|WIFI_TX),"atbmwifi_tx_sta_mgmtframe %x error!! \n",stype);
		return;
	}
	//
	//send mgmt frame
	//
	atbmwifi_tx(hw_priv,skb,priv);
}

#if CONFIG_SAE
static int atbmwifi_send_auth_reply(struct atbmwifi_vif *priv,
			atbm_uint16 transaction, atbm_uint16 auth_alg,  const atbm_uint8 *da,const atbm_uint8 *bssid,atbm_uint16 resp)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct hostapd_sta_info *sta = ap_get_sta((struct hostapd_data *)priv->appdata, da);
	//struct atbmwifi_ieee80211_tx_info * tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	struct atbm_buff *skb_tx = atbmwifi_ieee80211_send_auth(priv,transaction,auth_alg,da,priv->bssid,resp);
	atbmwifi_tx(hw_priv,skb_tx,priv);
	if(resp == ATBM_WLAN_STATUS_SUCCESS) {
		if(sta)
			priv->connect_timer_linkid = sta->aid;
		wifi_printk(WIFI_WPA,"atbm: atbmwifi_send_auth_reply(), ms=%lu\n", (long unsigned int)atbm_GetOsTimeMs());
		atbmwifi_eloop_register_timeout(0,ATBM_WIFI_AUTH_TIMEOUT,atbmwifi_ap_join_timeout,(atbm_void *)priv,ATBM_NULL);
	}
	return resp;
}

static void sae_set_state(struct hostapd_sta_info *sta, enum sae_state state,
			  const char *reason)
{
	wpa_printf(MSG_DEBUG, "SAE: State %s -> %s for peer " MACSTR " (%s)",
			sae_state_txt(sta->sae.state), sae_state_txt(state),
			MAC2STR(sta->addr), reason);
	sta->sae.state = state;
}

static struct wpabuf * auth_build_sae_commit(struct hostapd_data *hapd,
						 struct hostapd_sta_info *sta, int update)
{
	struct wpabuf *buf;
	const char *password = NULL;
	struct sae_password_entry *pw;
	const char *rx_id = NULL;
 
	if (sta->sae.tmp)
		rx_id = sta->sae.tmp->pw_id;
 
	for (pw = hapd->sae_passwords; pw; pw = pw->next) {
		if (!atbm_is_broadcast_ether_addr(pw->peer_addr) &&
			atbm_memcmp(pw->peer_addr, sta->addr, ATBM_ETH_ALEN) != 0)
			continue;
		if ((rx_id && !pw->identifier) || (!rx_id && pw->identifier))
			continue;
		if (rx_id && pw->identifier &&
			strcmp(rx_id, pw->identifier) != 0)
			continue;
		password = pw->password;
		break;
	}
	if (!password)
		password = hapd->wpa_passphrase;
	if (!password) {
		wpa_printf(MSG_DEBUG, "SAE: No password available");
		return NULL;
	}

	if (update &&
		sae_prepare_commit(hapd->own_addr, sta->addr,
					(atbm_uint8 *) password, strlen(password), rx_id,
					&sta->sae) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not pick PWE");
		return NULL;
	}
 
	if (pw && pw->vlan_id) {
		if (!sta->sae.tmp) {
			wpa_printf(MSG_INFO,
					"SAE: No temporary data allocated - cannot store VLAN ID");
			return NULL;
		}
		sta->sae.tmp->vlan_id = pw->vlan_id;
	}
 
	buf = wpabuf_alloc(SAE_COMMIT_MAX_LEN +
				(rx_id ? 3 + strlen(rx_id) : 0));
	if (buf == NULL)
		return NULL;
	sae_write_commit(&sta->sae, buf, sta->sae.tmp ?
			 sta->sae.tmp->anti_clogging_token : ATBM_NULL, rx_id);
 
	return buf;
}

static struct wpabuf * auth_build_sae_confirm(struct hostapd_data *hapd,
						  struct hostapd_sta_info *sta)
{
	struct wpabuf *buf;

	buf = wpabuf_alloc(SAE_CONFIRM_MAX_LEN);
	if (buf == NULL)
		return NULL;

	sae_write_confirm(&sta->sae, buf);

	return buf;
}

static int auth_sae_send_commit(struct hostapd_data *hapd,
				struct hostapd_sta_info *sta,
				const atbm_uint8 *bssid, int update)
{
	//struct wpabuf *data;
	int reply_res;

	wpabuf_free(hapd->sae_data);
	hapd->sae_data = auth_build_sae_commit(hapd, sta, update);
	if (!hapd->sae_data && sta->sae.tmp && sta->sae.tmp->pw_id)
		return ATBM_WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER;
	if (hapd->sae_data == NULL)
		return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;

	/*
	reply_res = send_auth_reply(hapd, sta->addr, bssid, ATBM_WLAN_AUTH_SAE, 1,
					ATBM_WLAN_STATUS_SUCCESS, wpabuf_head(data),
					wpabuf_len(data), "sae-send-commit");
	*/
	reply_res = atbmwifi_send_auth_reply(hapd->priv,1,ATBM_WLAN_AUTH_SAE,sta->addr,bssid,ATBM_WLAN_STATUS_SUCCESS);

	wpabuf_free(hapd->sae_data);

	hapd->sae_data = ATBM_NULL;

	return reply_res;
}

static int auth_sae_send_confirm(struct hostapd_data *hapd,
				struct hostapd_sta_info *sta,
				const atbm_uint8 *bssid)
{
	//struct wpabuf *data;
	int reply_res;

	wpabuf_free(hapd->sae_data);
	hapd->sae_data = auth_build_sae_confirm(hapd, sta);
	if (hapd->sae_data == NULL)
		return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;

/*
	reply_res = send_auth_reply(hapd, sta->addr, bssid, ATBM_WLAN_AUTH_SAE, 2,
					ATBM_WLAN_STATUS_SUCCESS, wpabuf_head(data),
					wpabuf_len(data), "sae-send-confirm");
*/
	reply_res = atbmwifi_send_auth_reply(hapd->priv,2,ATBM_WLAN_AUTH_SAE,sta->addr,bssid,ATBM_WLAN_STATUS_SUCCESS);

	wpabuf_free(hapd->sae_data);

	hapd->sae_data = ATBM_NULL;
 
	return reply_res;
}
 
static int use_sae_anti_clogging(struct hostapd_data *hapd)
{
	struct hostapd_sta_info *sta;
	unsigned int open = 0;
	int i;

	if (hapd->sae_anti_clogging_threshold == 0)
		return 1;

	for(i=0;i<ATBMWIFI__MAX_STA_IN_AP_MODE;i++){
		sta = hapd->sta_list[i];
		if(sta==ATBM_NULL) 	 {
			continue;
		}
		if (sta->sae.state != SAE_COMMITTED &&
				sta->sae.state != SAE_CONFIRMED)
			continue;
		open++;
		if (open >= hapd->sae_anti_clogging_threshold)
			return 1;
	}

	/* In addition to already existing open SAE sessions, check whether
	* there are enough pending commit messages in the processing queue to
	* potentially result in too many open sessions. */
	if (open >= //open + dl_list_len(&hapd->sae_commit_queue) >=
		hapd->sae_anti_clogging_threshold)
		return 1;

	return 0;
}

static atbm_uint8 sae_token_hash(struct hostapd_data *hapd, const atbm_uint8 *addr)
{
	atbm_uint8 hash[SHA256_MAC_LEN];

	atbmwifi_hmac_sha256(hapd->sae_token_key, sizeof(hapd->sae_token_key),
			addr, ATBM_ETH_ALEN, hash);
	return hash[0];
}

static int check_sae_token(struct hostapd_data *hapd, const atbm_uint8 *addr,
				const atbm_uint8 *token, atbm_size_t token_len)
{
	atbm_uint8 mac[SHA256_MAC_LEN];
	const atbm_uint8 *addrs[2];
	atbm_size_t len[2];
	atbm_uint16 token_idx;
	atbm_uint8 idx;

	if (token_len != SHA256_MAC_LEN)
		return -1;
	idx = sae_token_hash(hapd, addr);
	token_idx = hapd->sae_pending_token_idx[idx];
	if (token_idx == 0 || token_idx != ATBM_WPA_GET_BE16(token)) {
		wpa_printf(MSG_DEBUG, "SAE: Invalid anti-clogging token from "
				MACSTR " - token_idx 0x%04x, expected 0x%04x",
				MAC2STR(addr), ATBM_WPA_GET_BE16(token), token_idx);
		return -1;
	}
 
	addrs[0] = addr;
	len[0] = ATBM_ETH_ALEN;
	addrs[1] = token;
	len[1] = 2;
	if (atbmwifi_hmac_sha256_vector(hapd->sae_token_key, sizeof(hapd->sae_token_key),
			2, addrs, len, mac) < 0 ||
			atbm_memcmp(token + 2, &mac[2], SHA256_MAC_LEN - 2) != 0)
		return -1;
 
	hapd->sae_pending_token_idx[idx] = 0; /* invalidate used token */
 
	return 0;
}
 
static struct wpabuf * auth_build_token_req(struct hostapd_data *hapd,
						int group, const atbm_uint8 *addr)
{
	struct wpabuf *buf;
	atbm_uint8 *token;
	atbm_uint32 now;
	atbm_uint8 idx[2];
	const atbm_uint8 *addrs[2];
	atbm_size_t len[2];
	atbm_uint8 p_idx;
	atbm_uint16 token_idx;
 
	now = atbm_GetOsTime();
	if (!(hapd->last_sae_token_key_update) ||
		((now - hapd->last_sae_token_key_update) > 60 * 1000) ||
		hapd->sae_token_idx == 0xffff) {
		if (random_get_bytes(hapd->sae_token_key,
				sizeof(hapd->sae_token_key)) < 0)
			return NULL;
		wpa_hexdump(MSG_DEBUG, "SAE: Updated token key",
				hapd->sae_token_key, sizeof(hapd->sae_token_key));
		hapd->last_sae_token_key_update = now;
		hapd->sae_token_idx = 0;
		atbm_memset(hapd->sae_pending_token_idx, 0,
				sizeof(hapd->sae_pending_token_idx));
	}

	buf = wpabuf_alloc(sizeof(atbm_uint16) + SHA256_MAC_LEN);
	if (buf == NULL)
		return NULL;
 
	wpabuf_put_le16(buf, group); /* Finite Cyclic Group */

	p_idx = sae_token_hash(hapd, addr);
	token_idx = hapd->sae_pending_token_idx[p_idx];
	if (!token_idx) {
		hapd->sae_token_idx++;
		token_idx = hapd->sae_token_idx;
		hapd->sae_pending_token_idx[p_idx] = token_idx;
	}
	ATBM_WPA_PUT_BE16(idx, token_idx);
	token = wpabuf_put(buf, SHA256_MAC_LEN);
	addrs[0] = addr;
	len[0] = ATBM_ETH_ALEN;
	addrs[1] = idx;
	len[1] = sizeof(idx);
	if (atbmwifi_hmac_sha256_vector(hapd->sae_token_key, sizeof(hapd->sae_token_key),
			2, addrs, len, token) < 0) {
		wpabuf_free(buf);
		return NULL;
	}
	ATBM_WPA_PUT_BE16(token, token_idx);

	return buf;
 }
 
 
static int sae_check_big_sync(struct hostapd_data *hapd, struct hostapd_sta_info *sta)
{
	if (sta->sae.sync > hapd->sae_sync) {
		sae_set_state(sta, SAE_NOTHING, "Sync > dot11RSNASAESync");
		sta->sae.sync = 0;
		return -1;
	}
	return 0;
}

void sae_accept_sta(struct hostapd_data *hapd, struct hostapd_sta_info *sta)
{
#define CONFIG_NO_VLAN
#ifndef CONFIG_NO_VLAN
	 struct vlan_description vlan_desc;
 
	 if (sta->sae->tmp && sta->sae->tmp->vlan_id > 0) {
		 wpa_printf(MSG_DEBUG, "SAE: Assign STA " MACSTR
				" to VLAN ID %d",
				MAC2STR(sta->addr), sta->sae->tmp->vlan_id);
 
		 os_memset(&vlan_desc, 0, sizeof(vlan_desc));
		 vlan_desc.notempty = 1;
		 vlan_desc.untagged = sta->sae->tmp->vlan_id;
		 if (!hostapd_vlan_valid(hapd->conf->vlan, &vlan_desc)) {
			 wpa_printf(MSG_INFO,
					"Invalid VLAN ID %d in sae_password",
					sta->sae->tmp->vlan_id);
			 return;
		 }
 
		 if (ap_sta_set_vlan(hapd, sta, &vlan_desc) < 0 ||
			 ap_sta_bind_vlan(hapd, sta) < 0) {
			 wpa_printf(MSG_INFO,
					"Failed to assign VLAN ID %d from sae_password to "
					MACSTR, sta->sae->tmp->vlan_id,
					MAC2STR(sta->addr));
			 return;
		 }
	 }
#endif /* CONFIG_NO_VLAN */
 
	 sta->flags |= WLAN_STA_AUTH;
	 sta->auth_alg = ATBM_WLAN_AUTH_SAE;
	 //mlme_authenticate_indication(hapd, sta);
	 //wpa_auth_sm_event(sta->wpa_sm, WPA_AUTH);
	 sae_set_state(sta, SAE_ACCEPTED, "Accept Confirm");
extern int wpa_auth_pmksa_add_sae(struct hostapd_data *hapd, const atbm_uint8 *addr,			   const atbm_uint8 *pmk, const atbm_uint8 *pmkid);
	 wpa_auth_pmksa_add_sae(hapd, sta->addr,
					sta->sae.pmk, sta->sae.pmkid);
	 //sae_sme_send_external_auth_status(hapd, sta, ATBM_WLAN_STATUS_SUCCESS);
}

static int sae_sm_step(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
				const atbm_uint8 *bssid, atbm_uint8 auth_transaction, int allow_reuse,
				int *sta_removed)
{
	int ret;

	*sta_removed = 0;

	if (auth_transaction != 1 && auth_transaction != 2)
		return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
 
	wpa_printf(MSG_DEBUG, "SAE: Peer " MACSTR " state=%s auth_trans=%u",
			MAC2STR(sta->addr), sae_state_txt(sta->sae.state),
			auth_transaction);
	switch (sta->sae.state) {
	case SAE_NOTHING:
		if (auth_transaction == 1) {
			ret = auth_sae_send_commit(hapd, sta, bssid,
							!allow_reuse);
			if (ret)
				return ret;
			sae_set_state(sta, SAE_COMMITTED, "Sent Commit");
 
			if (sae_process_commit(&sta->sae) < 0)
				return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
			sta->sae.sync = 0;
			//sae_set_retransmit_timer(hapd, sta);
		} else {
			/*
			hostapd_logger(hapd, sta->addr,
						HOSTAPD_MODULE_IEEE80211,
						HOSTAPD_LEVEL_DEBUG,
						"SAE confirm before commit");
			*/
		}
		break;
	case SAE_COMMITTED:
		//sae_clear_retransmit_timer(hapd, sta);
		if (auth_transaction == 1) {
			if (sae_process_commit(&sta->sae) < 0)
				return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
 
			ret = auth_sae_send_confirm(hapd, sta, bssid);
			if (ret)
				return ret;
			sae_set_state(sta, SAE_CONFIRMED, "Sent Confirm");
			sta->sae.sync = 0;
			//sae_set_retransmit_timer(hapd, sta);
		} else {
			 /*
			  * For instructure BSS, send the postponed Confirm from
			  * Nothing -> Confirmed transition that was reduced to
			  * Nothing -> Committed above.
			  */
			ret = auth_sae_send_confirm(hapd, sta, bssid);
			if (ret)
				return ret;
 
			sae_set_state(sta, SAE_CONFIRMED, "Sent Confirm");
 
			 /*
			  * Since this was triggered on Confirm RX, run another
			  * step to get to Accepted without waiting for
			  * additional events.
			  */
			return sae_sm_step(hapd, sta, bssid, auth_transaction,
						0, sta_removed);
		}
		break;
	case SAE_CONFIRMED:
		//sae_clear_retransmit_timer(hapd, sta);
		if (auth_transaction == 1) {
			if (sae_check_big_sync(hapd, sta))
				return ATBM_WLAN_STATUS_SUCCESS;
			sta->sae.sync++;
 
			ret = auth_sae_send_commit(hapd, sta, bssid, 1);
			if (ret)
				return ret;
 
			if (sae_process_commit(&sta->sae) < 0)
				return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
 
			ret = auth_sae_send_confirm(hapd, sta, bssid);
			if (ret)
				return ret;
 
			//sae_set_retransmit_timer(hapd, sta);
		} else {
			sta->sae.send_confirm = 0xffff;
			sae_accept_sta(hapd, sta);
		}
		break;
	case SAE_ACCEPTED:
		if (auth_transaction == 1) {
			wpa_printf(MSG_DEBUG, "SAE: Start reauthentication");
			ret = auth_sae_send_commit(hapd, sta, bssid, 1);
			if (ret)
				return ret;
			sae_set_state(sta, SAE_COMMITTED, "Sent Commit");
 
			if (sae_process_commit(&sta->sae) < 0)
				return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
			sta->sae.sync = 0;
			//sae_set_retransmit_timer(hapd, sta);
		} else {
			if (sae_check_big_sync(hapd, sta))
				return ATBM_WLAN_STATUS_SUCCESS;
			sta->sae.sync++;
 
			ret = auth_sae_send_confirm(hapd, sta, bssid);
			sae_clear_temp_data(&sta->sae);
			if (ret)
				return ret;
		}
		break;
	default:
		wpa_printf(MSG_ERROR, "SAE: invalid state %d",
				sta->sae.state);
		return ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	return ATBM_WLAN_STATUS_SUCCESS;
}

static void sae_pick_next_group(struct hostapd_data *hapd, struct hostapd_sta_info *sta)
{
	struct sae_data *sae = &sta->sae;
	int i, *groups = hapd->sae_groups;
	int default_groups[] = { 19, 0 };

	if (sae->state != SAE_COMMITTED)
		return;
 
	wpa_printf(MSG_DEBUG, "SAE: Previously selected group: %d", sae->group);

	if (!groups)
		groups = default_groups;
	for (i = 0; groups[i] > 0; i++) {
		if (sae->group == groups[i])
			break;
	}

	if (groups[i] <= 0) {
		wpa_printf(MSG_DEBUG,
				"SAE: Previously selected group not found from the current configuration");
		return;
	}
 
	for (;;) {
		i++;
		if (groups[i] <= 0) {
			wpa_printf(MSG_DEBUG,
					"SAE: No alternative group enabled");
			return;
		}
 
		if (sae_set_group(sae, groups[i]) < 0)
			continue;
 
		break;
	}
	wpa_printf(MSG_DEBUG, "SAE: Selected new group: %d", groups[i]);
}

static void handle_auth_sae(struct hostapd_data *hapd, struct hostapd_sta_info *sta,
				const struct atbmwifi_ieee80211_mgmt *mgmt, atbm_size_t len,
				atbm_uint16 auth_transaction, atbm_uint16 status_code)
{
	int resp = ATBM_WLAN_STATUS_SUCCESS;
	//struct wpabuf *data = NULL;
	int *groups = hapd->sae_groups;
	int default_groups[] = { 19, 0 };
	const atbm_uint8 *pos, *end;
	int sta_removed = 0;

	if (!groups)
		groups = default_groups;
 
	if (status_code != ATBM_WLAN_STATUS_SUCCESS) {
		resp = -1;
		goto remove_sta;
	}

	if (auth_transaction == 1) {
		const atbm_uint8 *token = NULL;
		atbm_size_t token_len = 0;
		int allow_reuse = 0;

		sae_set_state(sta, SAE_NOTHING, "Init");
		sta->sae.sync = 0;
		/*
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
					HOSTAPD_LEVEL_DEBUG,
					"start SAE authentication (RX commit, status=%u (%s))",
					status_code, status2str(status_code));
 		*/
		if (sta->sae.state == SAE_COMMITTED) {
			 /* This is needed in the infrastructure BSS case to
			  * address a sequence where a STA entry may remain in
			  * hostapd across two attempts to do SAE authentication
			  * by the same STA. The second attempt may end up trying
			  * to use a different group and that would not be
			  * allowed if we remain in Committed state with the
			  * previously set parameters. */
			pos = mgmt->u.auth.variable;
			end = ((const atbm_uint8 *) mgmt) + len;
			if (end - pos >= (int) sizeof(atbm_uint16) &&
				sae_group_allowed(&sta->sae, groups,
						ATBM_WPA_GET_LE16(pos)) ==
				ATBM_WLAN_STATUS_SUCCESS) {
				/* Do not waste resources deriving the same PWE
				* again since the same group is reused. */
				sae_set_state(sta, SAE_NOTHING,
						   "Allow previous PWE to be reused");
				allow_reuse = 1;
			} else {
				sae_set_state(sta, SAE_NOTHING,
						  "Clear existing state to allow restart");
				sae_clear_data(&sta->sae);
			}
		}
		resp = sae_parse_commit(&sta->sae, mgmt->u.auth.variable,
					((const atbm_uint8 *) mgmt) + len -
					mgmt->u.auth.variable, &token,
					&token_len, groups);
		if (resp == SAE_SILENTLY_DISCARD) {
			wpa_printf(MSG_DEBUG,
					"SAE: Drop commit message from " MACSTR " due to reflection attack",
					MAC2STR(sta->addr));
			goto remove_sta;
		}

		if (resp == ATBM_WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER) {
			/*
			wpa_msg(hapd->msg_ctx, MSG_INFO,
				WPA_EVENT_SAE_UNKNOWN_PASSWORD_IDENTIFIER
				MACSTR, MAC2STR(sta->addr));
			*/
			 //sae_clear_retransmit_timer(hapd, sta);
			sae_set_state(sta, SAE_NOTHING,
					  "Unknown Password Identifier");
			goto remove_sta;
		}

		if (token && check_sae_token(hapd, sta->addr, token, token_len)
			< 0) {
			wpa_printf(MSG_DEBUG, "SAE: Drop commit message with "
					"incorrect token from " MACSTR,
					MAC2STR(sta->addr));
			resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
			goto remove_sta;
		}

		if (resp != ATBM_WLAN_STATUS_SUCCESS)
			goto reply;
 
		if (!token && use_sae_anti_clogging(hapd) && !allow_reuse) {
			wpa_printf(MSG_INFO,
					"SAE: Request anti-clogging token from "
					MACSTR, MAC2STR(sta->addr));
			wpabuf_free(hapd->sae_data);
			hapd->sae_data = auth_build_token_req(hapd, sta->sae.group,
							sta->addr);
			resp = ATBM_WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ;
			goto reply;
		}
		resp = sae_sm_step(hapd, sta, mgmt->bssid, auth_transaction,
					allow_reuse, &sta_removed);
	} else if (auth_transaction == 2) {
	 	/*
		 hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
					HOSTAPD_LEVEL_DEBUG,
					"SAE authentication (RX confirm, status=%u (%s))",
					status_code, status2str(status_code));
		*/
		if (status_code != ATBM_WLAN_STATUS_SUCCESS)
			goto remove_sta;
		if (sta->sae.state >= SAE_CONFIRMED) {
			const atbm_uint8 *var;
			atbm_size_t var_len;
			atbm_uint16 peer_send_confirm;
 
			var = mgmt->u.auth.variable;
			var_len = ((atbm_uint8 *) mgmt) + len - mgmt->u.auth.variable;
			if (var_len < 2) {
				resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto reply;
			}
 
			peer_send_confirm = ATBM_WPA_GET_LE16(var);
 
			if (sta->sae.state == SAE_ACCEPTED &&
					(peer_send_confirm <= sta->sae.rc ||
					peer_send_confirm == 0xffff)) {
				wpa_printf(MSG_DEBUG,
						"SAE: Silently ignore unexpected Confirm from peer "
						MACSTR
						" (peer-send-confirm=%u Rc=%u)",
						MAC2STR(sta->addr),
						peer_send_confirm, sta->sae.rc);
				return;
			}
 
			if (sae_check_confirm(&sta->sae, var, var_len) < 0) {
				resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto reply;
			}
			sta->sae.rc = peer_send_confirm;
		}
		resp = sae_sm_step(hapd, sta, mgmt->bssid, auth_transaction, 0,
			&sta_removed);
	} else {
	 	/*
		 hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
					HOSTAPD_LEVEL_DEBUG,
					"unexpected SAE authentication transaction %u (status=%u (%s))",
					auth_transaction, status_code,
					status2str(status_code));
		*/
		if (status_code != ATBM_WLAN_STATUS_SUCCESS)
			goto remove_sta;
		resp = ATBM_WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION;
	}
 
 reply:
	 if (!sta_removed && resp != ATBM_WLAN_STATUS_SUCCESS) {
		pos = mgmt->u.auth.variable;
		end = ((const atbm_uint8 *) mgmt) + len;
 
		/* Copy the Finite Cyclic Group field from the request if we
		 * rejected it as unsupported group. */
		if (resp == ATBM_WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED &&
			!hapd->sae_data && end - pos >= 2)
			hapd->sae_data = wpabuf_alloc_copy(pos, 2);
 
		 //sae_sme_send_external_auth_status(hapd, sta, resp);
		atbmwifi_send_auth_reply(hapd->priv,auth_transaction,ATBM_WLAN_AUTH_SAE,mgmt->sa,mgmt->bssid,resp);
		/*
		 send_auth_reply(hapd, mgmt->sa, mgmt->bssid, ATBM_WLAN_AUTH_SAE,
				 auth_transaction, resp,
				 data ? wpabuf_head(data) : (atbm_uint8 *) "",
				 data ? wpabuf_len(data) : 0, "auth-sae");
		*/
	}
remove_sta:
	if (!sta_removed &&
		(resp != ATBM_WLAN_STATUS_SUCCESS ||
		status_code != ATBM_WLAN_STATUS_SUCCESS)) {
		atbmwifi_sta_del(hapd->priv, sta->addr);
	}

	wpabuf_free(hapd->sae_data);
	hapd->sae_data = ATBM_NULL;
}
#endif

static atbm_uint8 sta_chllenge[ATBM_WLAN_AUTH_CHALLENGE_LEN];

atbm_void atbmwifi_rx_ap_auth(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	atbm_uint16 auth_alg, auth_transaction, status_code;
	atbm_uint16 fc;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	int len = ATBM_OS_SKB_LEN(skb);
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
	atbm_uint16 resp = ATBM_WLAN_STATUS_SUCCESS;
	struct atbmwifi_ieee80211_tx_info * tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	atbm_uint8 *resp_ies = ATBM_NULL;
	atbm_size_t resp_ies_len = 0;
	struct atbm_buff *skb_tx = ATBM_NULL;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
#if CONFIG_SAE
	struct hostapd_sta_info *sta;
	struct hostapd_data *hapd	 = (struct hostapd_data *)priv->appdata;
#endif /* CONFIG_SAE */

	auth_alg = mgmt->u.auth.auth_alg;
	auth_transaction = mgmt->u.auth.auth_transaction;
	status_code = mgmt->u.auth.status_code;
	fc = mgmt->frame_control;

	wifi_printk(WIFI_WPA,"ap:rx_auth:alg(%d),tran(%d),status(%d),fc(%x)\n",
	auth_alg,auth_transaction,status_code,fc);


	if (!(((config->auth_alg & ATBM_WPA_AUTH_ALG_OPEN) &&
			auth_alg == ATBM_WLAN_AUTH_OPEN) ||
#if CONFIG_SAE
			(config->auth_alg & ATBM_WPA_AUTH_ALG_SAE &&
			auth_alg == ATBM_WLAN_AUTH_SAE) ||
#endif
			((config->auth_alg & ATBM_WPA_AUTH_ALG_SHARED) &&
			auth_alg == ATBM_WLAN_AUTH_SHARED_KEY)	       )) {
		wifi_printk(WIFI_WPA,"ap:rx_auth:Unsupport rxalg(%d!=%d)\n",auth_alg,config->auth_alg);
 		dump_mem(mgmt,32);
		resp = ATBM_WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
		goto fail;
	}

	if (!(auth_transaction == 1 ||
#if CONFIG_SAE
			auth_alg == ATBM_WLAN_AUTH_SAE ||
#endif
			(auth_alg == ATBM_WLAN_AUTH_SHARED_KEY && auth_transaction == 3))) {
#ifdef WPA_HOST_DEBUG
			wifi_printk(WIFI_DBG_INIT,"ap:Unknown auth transnum (%d)\n",
			auth_transaction);
#endif
		resp = ATBM_WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION;
		goto fail;
	}

	if (atbm_memcmp(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN) == 0)
	{
		wifi_printk(WIFI_DBG_INIT,"Sta " MACSTR " not allow to auth\n",
		       MAC2STR(mgmt->sa));
		resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
		goto fail;
	}

	if(auth_alg == ATBM_WLAN_AUTH_SHARED_KEY)
	{
		
		if(auth_transaction == 1)
		{
			atbm_uint8 key[8]= {0,1,2,3,4,5,6,7};
			resp_ies = (atbm_uint8 *)atbm_kmalloc(2 + ATBM_WLAN_AUTH_CHALLENGE_LEN,GFP_KERNEL);

			if(resp_ies == ATBM_NULL)
			{
				resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto fail;
			}
			atbmwifi_rc4_skip(key, sizeof(key), 0,
				 sta_chllenge, ATBM_WLAN_AUTH_CHALLENGE_LEN);

			resp_ies[0] = ATBM_WLAN_EID_CHALLENGE;
			resp_ies[1] = ATBM_WLAN_AUTH_CHALLENGE_LEN;
			atbm_memcpy(resp_ies + 2, sta_chllenge,
				  ATBM_WLAN_AUTH_CHALLENGE_LEN);
			resp_ies_len = 2 + ATBM_WLAN_AUTH_CHALLENGE_LEN;
			
		}
		else if(auth_transaction == 3)
		{
			const atbm_uint8 *challenge = ATBM_NULL;
			if ((len >=2 + ATBM_WLAN_AUTH_CHALLENGE_LEN )&&
	    		(mgmt->u.auth.variable[0] == ATBM_WLAN_EID_CHALLENGE) &&
	    		(mgmt->u.auth.variable[1] == ATBM_WLAN_AUTH_CHALLENGE_LEN))
				{
					challenge = &mgmt->u.auth.variable[2];
				}
			if ( !challenge ||
	   			atbm_memcmp(sta_chllenge, challenge, ATBM_WLAN_AUTH_CHALLENGE_LEN))
				{
				    wifi_printk(WIFI_WPA,"ap_rx_auth challenge ATBM_FALSE\n\r");
					resp = ATBM_WLAN_STATUS_CHALLENGE_FAIL;
				}
		}
	}

#if CONFIG_SAE
	if(auth_alg == ATBM_WLAN_AUTH_SAE){
		sta = ap_get_sta(hapd, mgmt->sa);
		if(sta == ATBM_NULL)
		{
			sta = ap_sta_add(hapd, mgmt->sa,tx_info->link_id);
			if(sta == ATBM_NULL)
			{	 
				wifi_printk(WIFI_DBG_ERROR,"Sta " MACSTR " add FAIL.\n",
						MAC2STR(mgmt->sa));
				resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto fail;
			}
			sta->aid = tx_info->link_id;
		}
		handle_auth_sae(hapd, sta, mgmt, len, auth_transaction,
				status_code);
		return;
	}
#endif
	
	if(priv->extra_ie){
		atbm_kfree(priv->extra_ie);
	}
	priv->extra_ie = resp_ies;
	priv->extra_ie_len = resp_ies_len;
	
	skb_tx = atbmwifi_ieee80211_send_auth(priv,auth_transaction+1,auth_alg,mgmt->sa,priv->bssid,resp);
	atbmwifi_tx(hw_priv,skb_tx,priv);
	if(resp  == ATBM_WLAN_STATUS_SUCCESS) {
		priv->connect_timer_linkid = tx_info->link_id;
		wifi_printk(WIFI_WPA,"atbm: atbmwifi_rx_ap_auth(), ms=%lu\n", (long unsigned int)atbm_GetOsTimeMs());
		atbmwifi_eloop_register_timeout(0,ATBM_WIFI_AUTH_TIMEOUT,atbmwifi_ap_join_timeout,(atbm_void *)priv,ATBM_NULL);
	}
fail:
	if(resp_ies)
		atbm_kfree(resp_ies);
	priv->extra_ie_len = 0;
	priv->extra_ie = ATBM_NULL;
	
}

 int atbmwifi_rx_assoc_req(struct atbmwifi_vif *priv, struct atbm_buff *skb, struct atbmwifi_ieee80211_tx_info * tx_info)
{
	struct atbmwifi_ieee80211_mgmt *mgmt = ATBM_OS_SKB_DATA(skb);
	int len = ATBM_OS_SKB_LEN(skb);
	atbm_uint16 type = mgmt->frame_control & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_STYPE);
	atbm_uint16 resp = ATBM_WLAN_STATUS_SUCCESS;
	struct atbmwifi_ieee802_11_elems elems;
	const atbm_uint8 *wpa_ie=ATBM_NULL;
	atbm_size_t wpa_ie_len=0;
	atbm_uint8 * data=ATBM_NULL;
	int ret =0;
	atbm_uint16 ap_ht_cap_flags;
	atbm_uint8 atbm_is_40m = 0;
	atbm_uint8 sta_supp_40m = 0;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct atbmwifi_sta_priv *sta_priv = atbmwifi_sta_find_form_linkid(priv,tx_info->link_id);
#if CONFIG_WPS
	const atbm_uint8 *wps_ie = ATBM_NULL;
	atbm_size_t wps_ie_len = 0;
	struct wpabuf *wpaBuff = ATBM_NULL;
#endif

	if(sta_priv == ATBM_NULL){
		wifi_printk(WIFI_CONNECT, "<ERROR>atbmwifi_rx_assoc_req sta_priv ATBM_NULL just drop \n");
		resp = ATBM_WLAN_STATUS_AUTH_TIMEOUT;
		goto fail;
	}
	wifi_printk(WIFI_WPA,"ATBM_IEEE80211_STYPE_ASSOC_REQ 1\n");

	if(type == ATBM_IEEE80211_STYPE_ASSOC_REQ)
	{
		wifi_printk(WIFI_WPA,"ATBM_IEEE80211_STYPE_ASSOC_REQ  2\n");
		data = mgmt->u.assoc_req.variable;
		len  = len-offsetof(struct atbmwifi_ieee80211_mgmt, u.assoc_req.variable);
		
	}
	else if(type == ATBM_IEEE80211_STYPE_REASSOC_REQ)
	{
		data = mgmt->u.reassoc_req.variable;
		len  = len-offsetof(struct atbmwifi_ieee80211_mgmt, u.reassoc_req.variable);
		
	}
	else {
		return -1;
	}
	atbm_ieee802_11_parse_elems(data,len,&elems);

	if ((config->wpa & ATBM_WPA_PROTO_RSN) && elems.rsn) {
		wpa_ie = elems.rsn;
		wpa_ie_len = elems.rsn_len;
	} else if ((config->wpa & ATBM_WPA_PROTO_WPA) && elems.wpa) {
		wpa_ie = elems.wpa;
		wpa_ie_len = elems.wpa_len;
	} else{
		wpa_ie = ATBM_NULL;
		wpa_ie_len = 0;
	}
	resp = check_ssid(config,  elems.ssid, elems.ssid_len);
	if (resp != ATBM_WLAN_STATUS_SUCCESS)
		goto fail;
	
	if (!elems.supp_rates) {		
		resp = ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE;
		goto fail;
	}
#if CONFIG_WPS
	if((priv->pbc || priv->pin) && elems.wps_ie)
	{
		wpaBuff = wps_build_assoc_resp_ie();
		if(!wpaBuff){
			resp = ATBM_WLAN_REASON_UNSPECIFIED;
			goto fail;
		}
		wps_ie = wpaBuff->buf;
		wps_ie_len = wpaBuff->used;
	}else
#endif
	if (config->wpa && wpa_ie == ATBM_NULL) {
		resp = ATBM_WLAN_STATUS_INVALID_IE;
		goto fail;
	}
	else if ((config->wpa==0) && wpa_ie ) {
		resp = ATBM_WLAN_STATUS_INVALID_IE;
		goto fail;
	}

	if (elems.wmm_param || elems.wmm_info){
		sta_priv->uapsd_support_queues=is_uapsd_supported(&elems);
		if(elems.wmm_info){
			sta_priv->max_sp=(*(elems.wmm_info+8)>>5&0x3);
		}else if(elems.wmm_param){
			sta_priv->max_sp=(*(elems.wmm_param+8)>>5&0x3);
		}
		sta_priv->wmm_used = 1;	
		wifi_printk(WIFI_CONNECT,"uapsd_support_queues=%x,max_sp=%d\n",sta_priv->uapsd_support_queues,sta_priv->max_sp);
	}else{
		sta_priv->wmm_used = 0;
	}
	sta_priv->beacon_interval = mgmt->u.assoc_req.listen_interval;
	if(elems.erp_info)
		sta_priv->short_preamble = (elems.erp_info[0]& ATBM_WLAN_ERP_BARKER_PREAMBLE) == 0;
	sta_priv->wpa = (elems.rsn_len || elems.wpa_len) ? 1:0;
	sta_priv->ht =	elems.ht_cap_elem? 1:0;
	sta_priv->rate.ht = sta_priv->ht;
	sta_priv->band = priv->bss.sta_priv.band;
	if(elems.ht_cap_elem){
		atbmwifi_ieee80211_ht_cap_ie_to_sta_ht_cap(priv->hw_priv->bands[priv->bss.sta_priv.band],
						 elems.ht_cap_elem,
						&sta_priv->rate.ht_cap);
	}
	ap_ht_cap_flags = sta_priv->rate.ht_cap.cap;
	
	atbm_is_40m = !!atbmwifi_chtype_is_40M(priv->hw_priv->channel_type);
	sta_supp_40m = !!(sta_priv->rate.ht_cap.cap&ATBM_IEEE80211_HT_CAP_SUP_WIDTH_20_40);

	if(atbm_is_40m^sta_supp_40m){
		sta_priv->rate.channel_type = sta_priv->ht ? ATBM_NL80211_CHAN_HT20 : ATBM_NL80211_CHAN_NO_HT;
	}else {
		sta_priv->rate.channel_type = sta_priv->ht ? priv->hw_priv->channel_type : ATBM_NL80211_CHAN_NO_HT;
	}

	if(sta_priv->ht&&(ap_ht_cap_flags&(ATBM_IEEE80211_HT_CAP_SGI_20|ATBM_IEEE80211_HT_CAP_SGI_40))){
		sta_priv->sgi = 1;
	}else {
		sta_priv->sgi = 0;
	}
	
	wifi_printk(WIFI_WPA,"atbm_is_40m [%x],sta_supp_40m [%x],sta_chtype[%x]\n",atbm_is_40m,sta_supp_40m,sta_priv->rate.channel_type);
	atbmwifi_ieee80211_get_sta_rateinfo(&sta_priv->rate,priv->hw_priv->bands[priv->bss.sta_priv.band],elems.supp_rates, elems.supp_rates_len);
	atbmwifi_ieee80211_get_sta_rateinfo(&sta_priv->rate,priv->hw_priv->bands[priv->bss.sta_priv.band],elems.ext_supp_rates, elems.ext_supp_rates_len);
	
	wifi_printk(WIFI_RATE,"support_rates %x,basic_rates %x\n",sta_priv->rate.support_rates,sta_priv->rate.basic_rates);

	ret = atbmwifi_sta_add(priv,mgmt->sa);
	if(ret){	
		resp = ATBM_WLAN_STATUS_AUTH_TIMEOUT;
		goto fail;
	}
	priv->assoc_ok = 1;
	atbmwifi_eloop_cancel_timeout(atbmwifi_ap_join_timeout, (atbm_void *)priv, ATBM_NULL);
	resp = atbmwifi_event_uplayer(priv,ATBM_WIFI_ASSOC_EVENT,(atbm_uint8*)skb);
fail:
	{
		struct atbm_buff *skb = ATBM_NULL;	
		atbm_memcpy(priv->daddr,mgmt->sa,6);
#if CONFIG_WPS
		skb = atbmwifi_ieee80211_send_assoc_resp(priv,resp,type == ATBM_IEEE80211_STYPE_REASSOC_REQ ? 1:0,wps_ie,wps_ie_len,tx_info->link_id);
		wpabuf_free(wpaBuff);
#else
		skb = atbmwifi_ieee80211_send_assoc_resp(priv,resp,type == ATBM_IEEE80211_STYPE_REASSOC_REQ ? 1:0,wpa_ie,wpa_ie_len,tx_info->link_id);
#endif
		atbmwifi_tx(priv->hw_priv,skb,priv);
		//atbm_spin_unlock(mgm_tx);
	}
	return resp;
}

 atbm_void atbmwifi_rx_ap_mgmtframe(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_hdr * hdr = (struct atbmwifi_ieee80211_hdr *) ATBM_OS_SKB_DATA(skb);
	atbm_uint16 stype = hdr->frame_control & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_STYPE);
	struct atbmwifi_ieee80211_tx_info * tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	
	
     switch(stype){
#if CONFIG_P2P
		 case ATBM_IEEE80211_STYPE_PROBE_REQ:
			 {
				 struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *)ATBM_OS_SKB_DATA(skb);
				 struct atbmwifi_ieee80211_rx_status *hw_hdr = ATBM_IEEE80211_SKB_RXCB(skb);
				 atbm_uint8 *data = (atbm_uint8 *)ATBM_OS_SKB_DATA(skb) + offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_req.variable);
				 int len = ATBM_OS_SKB_LEN(skb) - offsetof(struct atbmwifi_ieee80211_mgmt, u.probe_req.variable);

				 atbm_p2p_probe_req_rx(priv, mgmt->sa, mgmt->da, mgmt->bssid,
					 data, len, hw_hdr->freq, (int)hw_hdr->signal);
			 }
			 break;
#endif
		case ATBM_IEEE80211_STYPE_BEACON:
		case ATBM_IEEE80211_STYPE_PROBE_RESP:
			atbmwifi_ap_rx_probe_resp(priv,skb);
			break;
		case ATBM_IEEE80211_STYPE_AUTH:	
		   {
				int linkid = atbmwifi_sta_alloc(priv,hdr->addr2);
				if(linkid < 0){
					break;
				}
				tx_info->link_id = linkid;
				atbmwifi_rx_ap_auth(priv,skb);			
				atbmwifi_event_uplayer(priv,ATBM_WIFI_AUTH_EVENT,(atbm_uint8*)skb);
		    }
			break;
		case ATBM_IEEE80211_STYPE_ASSOC_REQ:
		case ATBM_IEEE80211_STYPE_REASSOC_REQ:
			/*check linking station LinkId,if the link id is already alloced,do further,otherwise drop*/
			tx_info->link_id = atbmwifi_find_link_id(priv,hdr->addr2);
			if(tx_info->link_id == 0){
				//drop
				atbmwifi_ieee80211_tx_mgmt_deauth(priv,hdr->addr2,priv->bssid,ATBM_WLAN_REASON_PREV_AUTH_NOT_VALID);
				break;
			}
			if((stype == ATBM_IEEE80211_STYPE_ASSOC_REQ)
				&& (priv->link_id_db[tx_info->link_id-1].status != ATBMWIFI__LINK_RESERVE)){
				break;
			}
			//rx_info->link_id
			if(!atbmwifi_rx_assoc_req(priv, skb, tx_info) == ATBM_WLAN_STATUS_SUCCESS)
				atbmwifi_event_uplayer(priv,ATBM_WIFI_DEASSOC_EVENT,0);
			break;
		case ATBM_IEEE80211_STYPE_DEAUTH:
		case ATBM_IEEE80211_STYPE_DISASSOC:
			tx_info->link_id = atbmwifi_find_link_id(priv,hdr->addr2);
			wifi_printk(WIFI_WPA,"[ap]:rx DEAUTH \n");
			if((tx_info->link_id > ATBMWIFI__MAX_STA_IN_AP_MODE) || ( tx_info->link_id<=0)){
				break;
			}
			if(priv->link_id_db[tx_info->link_id-1].status != ATBMWIFI__LINK_SOFT) {
				priv->link_id_db[tx_info->link_id-1].status = ATBMWIFI__LINK_SOFT;
				atbmwifi_wpa_event_queue((atbm_void*)priv,(atbm_void*)&tx_info->link_id,ATBM_NULL,WPA_EVENT__SUPPLICANT_DEAUTHEN,ATBM_WPA_EVENT_NOACK);
				//atbmwifi_ap_deauth(priv,atbmwifi_ieee80211_get_SA(hdr));
			}
			break;
		default:
			break;
	}
}


 atbm_void atbmwifi_tx_ap_mgmtframe(struct atbmwifi_vif *priv,atbm_uint16 stype,atbm_uint16 transaction )
{
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb = ATBM_NULL;
	switch(stype){
		case ATBM_IEEE80211_STYPE_PROBE_RESP:
			/*probe resp is send by lmac*/
			skb = atbmwifi_ieee80211_send_proberesp(priv,priv->extra_ie,priv->extra_ie_len);
			break;
		case ATBM_IEEE80211_STYPE_AUTH:
			wifi_printk(WIFI_CONNECT, "[ap]:tx AUTH \n");
			skb = atbmwifi_ieee80211_send_auth(priv,transaction,ATBM_WLAN_AUTH_OPEN,priv->daddr,priv->bssid,0);
			break;
		case ATBM_IEEE80211_STYPE_ASSOC_RESP:
			wifi_printk(WIFI_CONNECT, "[ap]:tx ASSOC\n");
			skb = atbmwifi_ieee80211_send_assoc_resp(priv,0,0,ATBM_NULL,0,0);
			break;
		default:
			break;
	}
	if(skb == ATBM_NULL){
		wifi_printk(WIFI_DBG_ERROR, "atbmwifi_tx_ap_mgmtframe %x error!! \n",stype);
		return;
	}
	//
	//send mgmt frame
	//
	atbmwifi_tx(hw_priv,skb,priv);

}

 struct atbm_buff * atbmwifi_ieee80211_send_assoc_resp(struct atbmwifi_vif *priv,
			    atbm_uint16 status_code, int reassoc, const atbm_uint8 *ies,
			    atbm_size_t ies_len,atbm_uint16 aid)
{
	int send_len;
	struct atbm_buff *skb=ATBM_NULL;
	struct atbmwifi_ieee80211_mgmt *reply;
	atbm_uint8 *p = ATBM_NULL;
	atbm_uint16 capab=0;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);

	skb = atbm_dev_alloc_skb(
			sizeof(struct atbmwifi_ieee80211_mgmt) +1024+500);/* WMM */
	if (!skb){
		return ATBM_NULL;
	}

	p = ATBM_OS_SKB_DATA(skb);

	reply = (struct atbmwifi_ieee80211_mgmt *) p;
	atbm_memset(p, 0, sizeof(struct atbmwifi_ieee80211_mgmt));

	reply->frame_control =  atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
			      						ATBM_IEEE80211_STYPE_ASSOC_RESP);
	atbm_memcpy(reply->da, priv->daddr, ATBM_ETH_ALEN);
	atbm_memcpy(reply->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(reply->bssid, priv->bssid, ATBM_ETH_ALEN);
	send_len = ATBM_IEEE80211_HDRLEN;
	send_len += sizeof(reply->u.assoc_resp);
	
	if(config->privacy)	
		capab |= ATBM_WLAN_CAPABILITY_PRIVACY;
	
	capab |= ATBM_WLAN_CAPABILITY_SHORT_SLOT_TIME;
	capab |= ATBM_WLAN_CAPABILITY_ESS|ATBM_WLAN_CAPABILITY_SHORT_PREAMBLE;
	
	reply->u.assoc_resp.capab_info = capab;
	
	reply->u.assoc_resp.status_code = atbm_cpu_to_le16(status_code);
	reply->u.assoc_resp.aid = atbm_cpu_to_le16(aid/*aid=1*/   | BIT(14) | BIT(15));
	/* Supported rates */
	p = reply->u.assoc_resp.variable;	

	/* Supported rates */	
	/* Extended supported rates */	
	p = atbmwifi_ieee80211_add_rate_ie(p ,(config->band==WSM_PHY_BAND_5G),~0);

	p  = atbmwifi_ieee80211_add_wmm_param(priv,p);

#if CONFIG_IEEE80211N
	p = atbmwifi_ieee80211_add_ht_ie(priv,priv->hw_priv->bands[priv->bss.sta_priv.band],p);
	p = atbmwifi_ieee80211_add_ht_operation(priv,p);
#endif /* CONFIG_IEEE80211N */
	//p = atbmwifi_ieee80211_add_wpa_ie(priv,p);
#if CONFIG_WPS
	if(ies_len > 0){
		atbm_memcpy(p, ies, ies_len);
		p += ies_len;
	}
#endif

#if CONFIG_P2P
	if(priv->p2p_ap){
		p = atbm_p2p_add_ap_assoc_resp_ie(priv, p);
	}
#endif

	send_len += p - reply->u.assoc_resp.variable;
	atbm_skb_put(skb,send_len);
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	wifi_printk(WIFI_DBG_MSG,"atbm: atbmwifi_ieee80211_send_assoc_resp() <===\n");

	return skb;
	
}
 struct atbm_buff *atbmwifi_ieee80211_send_proberesp(struct atbmwifi_vif *priv,
				const atbm_uint8 *ies,
			    atbm_size_t ies_len)
{	
	int send_len;
	struct atbm_buff *skb=ATBM_NULL;
	struct atbmwifi_ieee80211_mgmt *reply;
	atbm_uint8 *p=ATBM_NULL;
	atbm_uint16 capab =0;	
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);

	skb = atbm_dev_alloc_skb(
			sizeof(struct atbmwifi_ieee80211_mgmt) +1024+500);/* WMM */
	if (!skb){
		wifi_printk(WIFI_DBG_ERROR,"alloc_skb ATBM_NULL\n");
		return ATBM_NULL;
	}

	p = ATBM_OS_SKB_DATA(skb);
	atbm_memset(p, 0, ATBM_OS_SKB_LEN(skb));
	reply = (struct atbmwifi_ieee80211_mgmt *) p;
	reply->frame_control =  atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
			      						ATBM_IEEE80211_STYPE_PROBE_RESP);
	
	atbm_memcpy(reply->da, priv->daddr, ATBM_ETH_ALEN);

	atbm_memcpy(reply->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(reply->bssid, priv->bssid, ATBM_ETH_ALEN);

	send_len = ATBM_IEEE80211_HDRLEN;
	send_len += sizeof(reply->u.probe_resp);


	
	if (config->privacy)	
		capab |= ATBM_WLAN_CAPABILITY_PRIVACY;
	capab |= ATBM_WLAN_CAPABILITY_SHORT_SLOT_TIME;
	capab |= ATBM_WLAN_CAPABILITY_ESS|ATBM_WLAN_CAPABILITY_SHORT_PREAMBLE;
	
	reply->u.probe_resp.beacon_int = config->beaconInterval;
	reply->u.probe_resp.capab_info = capab;

	p = reply->u.probe_resp.variable;

	/*If hide ssid,probeResponse need to add ssid in ies*/
	/* SSID */
	*p++ = ATBM_WLAN_EID_SSID;
	*p++ = priv->ssid_length;
	atbm_memcpy(p,priv->ssid, priv->ssid_length);
	p+= priv->ssid_length;
	/* Supported rates */	
	/* Extended supported rates */	
	p = atbmwifi_ieee80211_add_rate_ie(p ,(config->band==WSM_PHY_BAND_5G),~0);
	

	*p++ = ATBM_WLAN_EID_DS_PARAMS;
	*p++ = 1;
	*p++ = priv->config.channel_index;
	
	p  = atbmwifi_ieee80211_add_wmm_param(priv,p);

#if CONFIG_IEEE80211N
	p = atbmwifi_ieee80211_add_ht_ie(priv,priv->hw_priv->bands[priv->config.band],p);
	p = atbmwifi_ieee80211_add_ht_operation(priv,p);
#endif /* CONFIG_IEEE80211N */

	if (ies_len > 0) {
		atbm_memcpy(p,ies, ies_len);
		p += ies_len;
	}

#if CONFIG_WPS
	if(priv->wps_probe_resp_ie){
		atbm_memcpy(p, priv->wps_probe_resp_ie, priv->wps_probe_resp_ie_len);
		p += priv->wps_probe_resp_ie_len;
	}
#endif

#if CONFIG_P2P
	if(priv->p2p_ap){
		p = atbm_p2p_add_ap_pbresp_ie(priv, p);
	}
#endif

	send_len += p - reply->u.probe_resp.variable;

	atbm_skb_put(skb,send_len);

	return skb;

	
}
/*
 * Indicate whether there are frames queued for a station in power-save mode.
 */
int atbmwifi_set_tim(struct atbmwifi_vif  *priv, atbm_uint16 aid, ATBM_BOOL set)
{
    if ((aid == 0) || (aid > ATBMWIFI__MAX_STA_IN_AP_MODE))
            return 0;
    if (set != (atbm_test_bit(aid,(atbm_uint32 *)priv->tim_vbitmap) != 0)) {
        if (set) {
            atbm_set_bit(aid,(atbm_uint32 *)priv->tim_vbitmap);
			wifi_printk(WIFI_PS,"updata tim info %x\n",(unsigned int)priv->tim_vbitmap);
            priv->pspending_sta_num++;
        }
        else {
			wifi_printk(WIFI_PS,"clear tim info %x\n",(unsigned int)priv->tim_vbitmap);
            atbm_clear_bit(aid,(atbm_uint32 *)priv->tim_vbitmap);
            priv->pspending_sta_num--;
        }
       return 1;
    }
	return 0;
}

atbm_uint8 * atbmwifi_add_tim(atbm_uint8 *frm,
                        struct atbmwifi_vif  *priv,
                        atbm_uint8 mcast)
{
        struct atbmwifi_ieee80211_tim_ie *tim_ie = (struct atbmwifi_ieee80211_tim_ie *)frm;
        atbm_uint8 timoff;
        atbm_uint8 timlen=0;
        atbm_uint8 i;
        if (priv->pspending_sta_num != 0) {
                timoff = WIFI_TIMBITMAP_LEN;		/* impossibly large */
                for (i = 0; i < WIFI_TIMBITMAP_LEN; i++) {
                    if (priv->tim_vbitmap[i]) {
                            timoff = i & ATBM_TIM_BITCTL_UCAST_MASK;
                            break;
                    }
                }
                for (i = WIFI_TIMBITMAP_LEN-1; i >= timoff; i--) {
                    if (priv->tim_vbitmap[i])
                            break;
                }
                timlen = 1 + (i - timoff);
        }
        else {
			timoff = 0;
			timlen = 1;
        }
        /* update information element */
        tim_ie->tim_ie= ATBM_WLAN_EID_TIM;
        tim_ie->tim_len = 3 + timlen;
        tim_ie->dtim_count = 0;
        tim_ie->dtim_period= priv->config.DTIMPeriod;
        tim_ie->tim_bitmapctl = timoff;
        if (mcast && (tim_ie->dtim_count == 0))
                tim_ie->tim_bitmapctl |= ATBM_TIM_BITCTL_MCAST;
        else
                tim_ie->tim_bitmapctl &= ~ATBM_TIM_BITCTL_MCAST;
        atbm_memcpy(tim_ie->tim_vbitmap, priv->tim_vbitmap + timoff,
               timlen);

        return ((atbm_uint8 *)frm+tim_ie->tim_len+2);
}




 struct atbm_buff *atbmwifi_ieee80211_send_beacon(struct atbmwifi_vif  *priv,const atbm_uint8 *ies, atbm_size_t ies_len)
{
	int send_len;
	struct atbm_buff *skb=ATBM_NULL;	
	
	struct atbmwifi_ieee80211_mgmt *beacon;
	atbm_uint8 *p=ATBM_NULL;
	atbm_uint16 capab =0;	
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);

	skb = atbm_dev_alloc_skb(
			sizeof(struct atbmwifi_ieee80211_mgmt) +1024+ies_len);/* WMM */
	if (!skb){
		wifi_printk(WIFI_DBG_ERROR,"alloc_skb ATBM_NULL \n");
		return ATBM_NULL;
	}

	p = ATBM_OS_SKB_DATA(skb);
	atbm_memset(p, 0, ATBM_OS_SKB_LEN(skb));
	beacon = (struct atbmwifi_ieee80211_mgmt *) p;
	beacon->frame_control =  atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
			      						ATBM_IEEE80211_STYPE_BEACON);
	//atbm_memcpy(beacon->da, priv->daddr, ATBM_ETH_ALEN);
	atbm_memset(beacon->da, 0xff, ATBM_ETH_ALEN);
	atbm_memcpy(beacon->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(beacon->bssid, priv->bssid, ATBM_ETH_ALEN);

	send_len = ATBM_IEEE80211_HDRLEN;
	send_len += sizeof(beacon->u.beacon);

	if (config->privacy)
		capab |= ATBM_WLAN_CAPABILITY_PRIVACY;
	capab |= ATBM_WLAN_CAPABILITY_SHORT_SLOT_TIME;
	capab |= ATBM_WLAN_CAPABILITY_ESS|ATBM_WLAN_CAPABILITY_SHORT_PREAMBLE;
	
	beacon->u.beacon.beacon_int = config->beaconInterval;
	beacon->u.beacon.capab_info = capab;

	p = beacon->u.beacon.variable;

	if(!config->hide_ssid/*hide_ssid*/){
		/* ssid*/
		*p++ = ATBM_WLAN_EID_SSID;
		*p++ = priv->ssid_length;
		atbm_memcpy(p,priv->ssid, priv->ssid_length);
		p += priv->ssid_length;
	}else{
		/*hide ssid need set ssidLen=0 or set ssidLen=1&&ssid=0*/
		*p++ = ATBM_WLAN_EID_SSID;
		*p++ = 0;
	}
	/* Supported rates */
	/* Extended supported rates */	
	p = atbmwifi_ieee80211_add_rate_ie(p ,(config->band==WSM_PHY_BAND_5G),~0);
	
	*p++ = ATBM_WLAN_EID_DS_PARAMS;
	*p++ = 1;
	*p++ = priv->config.channel_index;
	
	/* Supported ERP */
	*p++ = ATBM_WLAN_EID_ERP_INFO;
	*p++ = 1;
	*p++ =BIT(2);
	
	p  = atbmwifi_ieee80211_add_wmm_param(priv,p);
	p = atbmwifi_add_tim(p,priv,0);
	
#if CONFIG_IEEE80211N
	p = atbmwifi_ieee80211_add_ht_ie(priv,priv->hw_priv->bands[config->band], p);
	p = atbmwifi_ieee80211_add_ht_operation(priv,p);
#endif /* CONFIG_IEEE80211N */
	
	if (ies_len > 0) {
		atbm_memcpy(p,ies, ies_len);
		p += ies_len;
	}

#if CONFIG_WPS
	if(priv->wps_beacon_ie != ATBM_NULL){
		wifi_printk(WIFI_DBG_ERROR, "set wps_beacon_ie \n");
		atbm_memcpy(p, priv->wps_beacon_ie, priv->wps_beacon_ie_len);
		p += priv->wps_beacon_ie_len;
	}
#endif

#if CONFIG_P2P
	if(priv->p2p_ap){
		p = atbm_p2p_add_ap_beacon_ie(priv, p);
	}
#endif

	send_len += p - beacon->u.beacon.variable;

	atbm_skb_put(skb,send_len);
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	return skb;
}


 struct atbm_buff * atbmwifi_ieee80211_send_auth(struct atbmwifi_vif *priv,
			 atbm_uint16 transaction, atbm_uint16 auth_alg,  const atbm_uint8 *da,const atbm_uint8 *bssid,atbm_uint16 resp)
{
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	int err=1;
	int alloc_len = sizeof(*mgmt) + 6 + priv->extra_ie_len;
	int extra_sae_len = 0;
#if CONFIG_SAE
	struct wpa_supplicant *wpa_s = ATBM_NULL;
	struct hostapd_data *hapd = ATBM_NULL;

	if(atbmwifi_is_sta_mode(priv->iftype)){
		wpa_s = (struct wpa_supplicant *)priv->appdata;
		if(wpa_s && wpa_s->sae_data){
			extra_sae_len = wpabuf_len(wpa_s->sae_data) - 4;
		}
	}else{
		hapd = (struct hostapd_data *)priv->appdata;
		if(hapd && hapd->sae_data){
			extra_sae_len = wpabuf_len(hapd->sae_data);
		}
	}
#endif

	skb = atbm_dev_alloc_skb(alloc_len + extra_sae_len);
	if (!skb)
	{
		wifi_printk(WIFI_TX,"<ERROR> send_auth alloc skb \n");
		return ATBM_NULL;
	}
	wifi_printk(WIFI_WPA,"send_auth alg(%d),transaction(%d),ielen(%d)\n",auth_alg,transaction,priv->extra_ie_len);
	
	mgmt = (struct atbmwifi_ieee80211_mgmt *) atbm_skb_put(skb, 24 + 6);
	atbm_memset(mgmt, 0, 24 + 6);
	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT|
					  ATBM_IEEE80211_STYPE_AUTH);
	atbm_memcpy(mgmt->da, da, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->bssid, bssid, ATBM_ETH_ALEN);
	mgmt->u.auth.auth_alg = atbm_cpu_to_le16(auth_alg);
	mgmt->u.auth.auth_transaction = atbm_cpu_to_le16(transaction);
	mgmt->u.auth.status_code = atbm_cpu_to_le16(resp);

#if CONFIG_SAE
	if(extra_sae_len){
		if(atbmwifi_is_sta_mode(priv->iftype)){
			atbm_memcpy(atbm_skb_put(skb, extra_sae_len), wpabuf_head(wpa_s->sae_data) + 4, extra_sae_len);
		}else{
			atbm_memcpy(atbm_skb_put(skb, extra_sae_len), wpabuf_head(hapd->sae_data), extra_sae_len);
		}
	}
#endif

	if (priv->extra_ie && priv->extra_ie_len) {
		atbm_memcpy(atbm_skb_put(skb, priv->extra_ie_len), priv->extra_ie, priv->extra_ie_len);
	}
	
	if (auth_alg == ATBM_WLAN_AUTH_SHARED_KEY && transaction == 3) {
//		mgmt->frame_control |= atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_PROTECTED);
		ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;
		ATBM_WARN_ON_FUNC(err);
		return skb;
	}

#if CONFIG_P2P
	if(priv->p2pdata){
		ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;
	}else
#endif
	{
		ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	}

	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;

	return skb;
}

 struct atbm_buff * atbmwifi_ieee80211_send_deauth(struct atbmwifi_vif *priv,const atbm_uint8 *da,const atbm_uint8 *bssid,atbm_uint16 reason)
{
//	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
//	int err=1;

	skb = atbm_dev_alloc_skb(sizeof(*mgmt) + 2);
	if (!skb)
	{
		wifi_printk(WIFI_TX,"<ERROR> send_auth alloc skb \n");
		return ATBM_NULL;
	}
	wifi_printk(WIFI_TX,"send_deauth reason(%d)\n",reason);
	
	mgmt = (struct atbmwifi_ieee80211_mgmt *) atbm_skb_put(skb, 24 + 2);
	atbm_memset(mgmt, 0, 24 + 2);
	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT|
					  ATBM_IEEE80211_STYPE_DEAUTH);
	atbm_memcpy(mgmt->da, da, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->bssid, bssid, ATBM_ETH_ALEN);
	mgmt->u.deauth.reason_code = atbm_cpu_to_le16(reason);
	
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT;
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	return skb;
}
 int atbmwifi_ieee80211_tx_mgmt_deauth(struct atbmwifi_vif *priv,const atbm_uint8 *da,const atbm_uint8 *bssid,atbm_uint16 reason)
{
	struct atbm_buff *skb;

	skb = atbmwifi_ieee80211_send_deauth(priv,da,bssid,reason);

	if(skb == ATBM_NULL)
		return -1;

	atbmwifi_tx(priv->hw_priv,skb,priv);

	return 0;
}

#ifdef CONFIG_IEEE80211W
struct atbm_buff *atbmwifi_ieee80211_send_saquery(struct atbmwifi_vif *priv,const atbm_uint8 *da,const atbm_uint8 *bssid, int dir, const atbm_uint8 *trans_id)
{
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	struct wsm_protected_mgmt_policy mgmt_policy;

	skb = atbm_dev_alloc_skb(100);
	if (!skb)
	{
		wifi_printk(WIFI_TX,"<ERROR> send_auth alloc skb \n");
		return ATBM_NULL;
	}
	mgmt = (struct atbmwifi_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	atbm_memset(mgmt, 0, 24 + 6);
	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT|
					  ATBM_IEEE80211_STYPE_ACTION);
	atbm_memcpy(mgmt->da, da, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->bssid, bssid, ATBM_ETH_ALEN);
	atbm_skb_put(skb, 1 + sizeof(mgmt->u.action.u.sa_query));
	mgmt->u.action.category = ATBM_WLAN_CATEGORY_SA_QUERY;
	mgmt->u.action.u.sa_query.action = dir;
	atbm_memcpy(mgmt->u.action.u.sa_query.trans_id, trans_id, WLAN_SA_QUERY_TR_ID_LEN);

	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	return skb;
}
#endif

void atbmwifi_ieee80211_send_addba_request(struct atbmwifi_vif *priv,
			  const atbm_uint8 *da, atbm_uint16 tid,
			  atbm_uint8 dialog_token, atbm_uint16 start_seq_num,
			  atbm_uint16 agg_size, atbm_uint16 timeout)
{
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	atbm_uint16 capab;

	skb = atbm_dev_alloc_skb(sizeof(*mgmt));

	if (!skb)
		return;

	//skb_reserve(skb, local->hw.extra_tx_headroom);
	mgmt = (struct atbmwifi_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	atbm_memset(mgmt, 0, 24 + 10);
	atbm_memcpy(mgmt->da, da, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	if (priv->iftype == ATBM_NL80211_IFTYPE_AP)
		atbm_memcpy(mgmt->bssid, priv->mac_addr, ATBM_ETH_ALEN);
	else if (priv->iftype == ATBM_NL80211_IFTYPE_STATION)
		atbm_memcpy(mgmt->bssid, priv->bss.bssid, ATBM_ETH_ALEN);

	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
				   ATBM_IEEE80211_STYPE_ACTION);

	atbm_skb_put(skb, 1 + sizeof(mgmt->u.action.u.addba_req));

	mgmt->u.action.category = ATBM_WLAN_CATEGORY_BACK;
	mgmt->u.action.u.addba_req.action_code = ATBM_WLAN_ACTION_ADDBA_REQ;

	mgmt->u.action.u.addba_req.dialog_token = dialog_token;
	capab = (atbm_uint32)(1 << 0); 	 /* bit 0 A-MSDU support */
	capab |= (atbm_uint32)(1 << 1);	 /* bit 1 aggregation policy */
	capab |= (atbm_uint32)(tid << 2);	 /* bit 5:2 TID number */
	capab |= (atbm_uint32)(agg_size << 6);  /* bit 15:6 max size of aggergation */

	mgmt->u.action.u.addba_req.capab = atbm_cpu_to_le16(capab);

	mgmt->u.action.u.addba_req.timeout = atbm_cpu_to_le16(timeout);
	mgmt->u.action.u.addba_req.start_seq_num =
				 atbm_cpu_to_le16(start_seq_num << 4);

	atbmwifi_tx(priv->hw_priv,skb,priv);
}

void ieee80211_send_delba(struct atbmwifi_vif *priv,
	   const atbm_uint8 *da, atbm_uint16 tid,
	   atbm_uint16 initiator, atbm_uint16 reason_code)
{
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	atbm_uint16 params;

	skb = atbm_dev_alloc_skb(sizeof(*mgmt));
	if (!skb)
		return;

	mgmt = (struct atbmwifi_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	atbm_memset(mgmt, 0, 24 + 5);
	atbm_memcpy(mgmt->da, da, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	if (priv->iftype == ATBM_NL80211_IFTYPE_AP)
		atbm_memcpy(mgmt->bssid, priv->mac_addr, ATBM_ETH_ALEN);
	else if (priv->iftype == ATBM_NL80211_IFTYPE_STATION)
		atbm_memcpy(mgmt->bssid, priv->bss.bssid, ATBM_ETH_ALEN);

	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
				   ATBM_IEEE80211_STYPE_ACTION);

	atbm_skb_put(skb, 1 + sizeof(mgmt->u.action.u.delba));

	mgmt->u.action.category = ATBM_WLAN_CATEGORY_BACK;
	mgmt->u.action.u.delba.action_code = ATBM_WLAN_ACTION_DELBA;
	params = (atbm_uint16)(initiator << 11);	 /* bit 11 initiator */
	params |= (atbm_uint16)(tid << 12);		 /* bit 15:12 TID number */

	mgmt->u.action.u.delba.params = atbm_cpu_to_le16(params);
	mgmt->u.action.u.delba.reason_code = atbm_cpu_to_le16(reason_code);

	atbmwifi_tx(priv->hw_priv,skb,priv);
}

void ieee80211_send_addba_resp(struct atbmwifi_vif *priv, atbm_uint8 *da, atbm_uint16 tid,
					   atbm_uint8 dialog_token, atbm_uint16 status, atbm_uint16 policy,
					   atbm_uint16 buf_size, atbm_uint16 timeout)
{
	//struct ieee80211_local *local = sdata->local;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_mgmt *mgmt;
	atbm_uint16 capab;

	skb = atbm_dev_alloc_skb(sizeof(*mgmt));
	if (!skb)
		return;

	//atbm_skb_reserve(skb, local->hw.extra_tx_headroom);
	mgmt = (struct atbmwifi_ieee80211_mgmt *) atbm_skb_put(skb, 24);
	atbm_memset(mgmt, 0, 24 + 8);
	atbm_memcpy(mgmt->da, da, ATBM_ETH_ALEN);
	atbm_memcpy(mgmt->sa, priv->mac_addr, ATBM_ETH_ALEN);
	if (priv->iftype == ATBM_NL80211_IFTYPE_AP)
		atbm_memcpy(mgmt->bssid, priv->mac_addr, ATBM_ETH_ALEN);
	else if (priv->iftype == ATBM_NL80211_IFTYPE_STATION)
		atbm_memcpy(mgmt->bssid, priv->bss.bssid, ATBM_ETH_ALEN);

	mgmt->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
				   ATBM_IEEE80211_STYPE_ACTION);

	atbm_skb_put(skb, 1 + sizeof(mgmt->u.action.u.addba_resp));
	mgmt->u.action.category = ATBM_WLAN_CATEGORY_BACK;
	mgmt->u.action.u.addba_resp.action_code = ATBM_WLAN_ACTION_ADDBA_RESP;
	mgmt->u.action.u.addba_resp.dialog_token = dialog_token;

	capab = (atbm_uint16)(policy << 1); /* bit 1 aggregation policy */
	capab |= (atbm_uint16)(tid << 2);	 /* bit 5:2 TID number */
	capab |= (atbm_uint16)(buf_size << 6);  /* bit 15:6 max size of aggregation */

	mgmt->u.action.u.addba_resp.capab = atbm_cpu_to_le16(capab);
	mgmt->u.action.u.addba_resp.timeout = atbm_cpu_to_le16(timeout);
	mgmt->u.action.u.addba_resp.status = atbm_cpu_to_le16(status);

	atbmwifi_tx(priv->hw_priv,skb,priv);
}

#if NEW_SUPPORT_PS
 struct atbm_buff * atbmwifi_ieee80211_NullData(struct atbmwifi_vif *priv,const atbm_uint8 *da,const atbm_uint8 *bssid)
{
	//struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_hdr *nulldata;
	skb = atbm_dev_alloc_skb(sizeof(struct atbmwifi_ieee80211_hdr));
	if (!skb)
	{
		wifi_printk(WIFI_TX,"<ERROR> send_auth alloc skb \n");
		return ATBM_NULL;
	}	
	nulldata = (struct atbmwifi_ieee80211_hdr *) atbm_skb_put(skb, 24);
	atbm_memset(nulldata, 0, 24);
	nulldata->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA|
					  ATBM_IEEE80211_STYPE_NULLFUNC);
	atbm_memcpy(nulldata->addr1, da, ATBM_ETH_ALEN);
	atbm_memcpy(nulldata->addr2, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(nulldata->addr3, bssid, ATBM_ETH_ALEN);

	
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |=ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT| 
										ATBM_IEEE80211_TX_CTL_POLL_RESPONSE;
	return skb;
}
 struct atbm_buff * atbmwifi_ieee80211_QosNullData(struct atbmwifi_vif *priv,const atbm_uint8 *da,const atbm_uint8 *bssid,atbm_uint8 tid)
{
	//struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbm_buff *skb;
	struct atbmwifi_ieee80211_qos_hdr *QosNulldata;

	skb = atbm_dev_alloc_skb(sizeof(struct atbmwifi_ieee80211_qos_hdr));
	if (!skb)
	{
		wifi_printk(WIFI_TX,"<ERROR> send_auth alloc skb \n");
		return ATBM_NULL;
	}
	
	QosNulldata = (struct atbmwifi_ieee80211_qos_hdr *) atbm_skb_put(skb, 26);
	atbm_memset(QosNulldata, 0, 26);
	QosNulldata->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA|
					  ATBM_IEEE80211_STYPE_QOS_NULLFUNC);
	atbm_memcpy(QosNulldata->addr1, da, ATBM_ETH_ALEN);
	atbm_memcpy(QosNulldata->addr2, priv->mac_addr, ATBM_ETH_ALEN);
	atbm_memcpy(QosNulldata->addr3, bssid, ATBM_ETH_ALEN);
	QosNulldata->qos_ctrl = tid;
	QosNulldata->qos_ctrl |=
		atbm_cpu_to_le16(ATBM_IEEE80211_QOS_CTL_EOSP);
	ATBM_IEEE80211_SKB_TXCB(skb)->flags |= ATBM_IEEE80211_TX_INTFL_DONT_ENCRYPT|
										ATBM_IEEE80211_TX_STATUS_EOSP;
	return skb;
}
#endif

