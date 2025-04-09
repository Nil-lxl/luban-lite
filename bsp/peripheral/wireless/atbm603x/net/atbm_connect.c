/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
extern atbm_void sta_scan_start_timer_func(atbm_void *data1,atbm_void *data2);
extern struct atbmwifi_common g_hw_prv;
#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64
#define MAX_INTF_ID 2
#define ATBM_CHECK_AGRV_AVAILABLE(PARAM_1,PARAM_2) do{ \
		atbm_uint8 ret;\
		if(PARAM_1>MAX_SSID_LEN){ \
			ret=-1; \
		} \
		if(PARAM_2>MAX_PASSWORD_LEN){\
			ret= -2; \
		} \
	}while(0)
	
#define ATBM_CHECK_AGRV_AVAILABLE_POINTER(PARAM_1,PARAM_2) do{ \
				atbm_uint8 ret;\
				if(PARAM_1==ATBM_NULL){ \
					ret=-1; \
					goto EXIT;\
				} \
				if(PARAM_2==ATBM_NULL){\
					ret= -2; \
					goto EXIT;\
				} \
			}while(0)
 atbm_int32 wifi_ConnectAP_vif(atbm_uint8 if_id,atbm_uint8 * ssid,int ssidlen,atbm_uint8 * password,int passwdlen,ATBM_SECURITY_TYPE key_mgmt)
{
	atbm_int32 ret = 0;
	struct atbmwifi_vif *priv;
	ATBM_CHECK_AGRV_AVAILABLE(ssidlen,passwdlen);

	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);
	
	if(priv == ATBM_NULL)
		return -1;
	
	atbm_memset(priv->config.ssid, 0, sizeof(priv->config.ssid));
	atbm_memset(priv->config.password, 0, sizeof(priv->config.password));

	atbm_memcpy(priv->config.ssid,ssid,ssidlen);
	priv->config.ssid_len = priv->ssid_length= ssidlen;
	atbm_memcpy(priv->ssid,priv->config.ssid,ssidlen);
	if(passwdlen){
		atbm_memcpy(priv->config.password,password,passwdlen);
	}
	if(ATBM_KEY_WEP_SHARE != key_mgmt){
		priv->config.auth_alg = ATBM_WLAN_AUTH_OPEN;
	}
	else {
		priv->config.auth_alg = ATBM_WLAN_AUTH_SHARED_KEY;
	}
	priv->config.password_len = passwdlen;
	priv->config.privacy = passwdlen?1:0;
	priv->config.key_mgmt = key_mgmt;
	priv->config.key_id = 0;
	priv->scan_expire = INITIAL_SCAN_EXPIRE;
	priv->connect_expire=0;
	//atbmwifi_autoconnect(priv);
#if CONFIG_IEEE80211W
	if(priv->config.key_mgmt == ATBM_KEY_SAE)
		priv->config.ieee80211w = 2;
	else
		priv->config.ieee80211w = 1;
#endif
	wpa_connect_ap(priv);
	return ret;
}

int wifi_SetAPConfig(struct atbmwifi_vif *priv,atbm_uint8 * ssid,int ssidlen,atbm_uint8 * password,int passwdlen,int channel,ATBM_SECURITY_TYPE key_mgmt,ATBM_BOOL ssidBcst){
	if(ssidlen == 0 || ssidlen >32){
		wifi_printk(WIFI_DBG_ERROR,"wifi_StartAP_vif ssid len is zero err\n");
		return -1;
	}

#if (CONFIG_5G_SUPPORT == 0)
	if((channel > 14)){
		wifi_printk(WIFI_DBG_ERROR,"wifi_StartAP_vif channel is zero err\n");
		return -1;
	}
#endif
	if(passwdlen>64){
		return -1;
	}

	atbm_memcpy(priv->config.ssid,ssid,ssidlen);
	priv->config.ssid_len = priv->ssid_length= ssidlen;
	atbm_memcpy(priv->ssid,priv->config.ssid,ssidlen);
	if(passwdlen){
		atbm_memcpy(priv->config.password,password,passwdlen);
		priv->config.password_len = passwdlen;
		priv->config.privacy = 1;
	}else{
		priv->config.privacy = 0;
	}
	priv->config.key_mgmt = key_mgmt;
	priv->config.hide_ssid = ssidBcst;
	/*Other Ap initial*/
	priv->config.beaconInterval = TEST_BEACON_INTV; 
	priv->config.DTIMPeriod = TEST_DTIM_INTV;
	priv->config.preambleType = TEST_SHORT_PREAMBLE;
	priv->config.basicRateSet=TEST_BASIC_RATE;
	///////////////////////////////////////////////////
	/*If use the ap auto channel function,user need set channle 0*/
	//channel=0;
	channel = channel ? channel : atbmwifi_iee80211_peerif_channel(priv);
	if(channel==0){
		atbm_autoChann_Select(priv,&priv->config.channel_index);
	}else{
		priv->config.channel_index = channel;
#if BW_40M_SUPPORT
		priv->bss.channel_type = atbmwifi_iee80211_peerif_channel_type(priv);
#else
		priv->bss.channel_type = g_hw_prv.channel_type;
#endif
	}
	return 0;
}

atbm_void wifi_StartAP_vif(atbm_uint8 if_id,atbm_uint8 * ssid,int ssidlen,atbm_uint8 * password,int passwdlen,int channel,ATBM_SECURITY_TYPE key_mgmt,ATBM_BOOL ssidBcst)
{
	struct atbmwifi_vif *priv;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,if_id);

	wifi_printk(WIFI_ALWAYS,"%s @@@@ if_id=%d, scan.if_id=%d\n", __func__, priv->if_id, priv->scan.if_id);

	if(wifi_SetAPConfig(priv, ssid, ssidlen, password, passwdlen, channel, key_mgmt, ssidBcst)){
		return;
	}
	atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,ATBM_NULL,WPA_EVENT__HOSTAPD_START,ATBM_WPA_EVENT_ACK);
}

 atbm_void atbmwifi_wpa_event_start_ap(struct atbmwifi_vif *priv)
{
	struct atbmwifi_cfg *config = NULL;

	if(priv == ATBM_NULL){		
		wifi_printk(WIFI_ALWAYS, "%s priv is null\n", __func__);
		return;
	}

	config = atbmwifi_get_config(priv);
	if(atbmwifi_iee80211_check_combination(priv,config->channel_index) == ATBM_FALSE){
        wifi_printk(WIFI_ALWAYS, "%s AP(ch:%d) and STA is not work at same channel\n", __func__, config->channel_index);
		return;
	}
	hostapd_start(priv);
}
/*
change wifi powersave mode
*/
 atbm_void wifi_ChangePsMode(struct atbmwifi_vif *priv,atbm_uint8 enable,atbm_uint8 ds_timeout)
{
	atbmwifi_set_pm(priv,enable,ds_timeout);
}

 atbm_void AT_WDisConnect_vif(struct atbmwifi_vif *priv,char *pLine)
{		
	atbmwifi_ieee80211_send_deauth_disassoc(priv, priv->daddr,priv->bssid,
				       ATBM_IEEE80211_STYPE_DEAUTH,
				       ATBM_WLAN_REASON_DEAUTH_LEAVING,
				       ATBM_NULL, ATBM_TRUE);
	if(!priv->enabled){
		wifi_printk(WIFI_ALWAYS,"not support not enabled!\n");
		return;
	}
	if(!atbmwifi_is_sta_mode(priv->iftype)) {
		return;
	}
	sta_deauth(priv);
	atbm_mdelay(100);
	wpa_disconnect(priv);
	atbm_mdelay(200);
}

 atbm_void AT_WDisConnect(char *pLine)
{		
	struct atbmwifi_common *hw_priv;
	struct atbmwifi_vif *priv;
	hw_priv=&g_hw_prv;
	priv= _atbmwifi_hwpriv_to_vifpriv(&g_hw_prv,0);
	if(priv!=ATBM_NULL){
		AT_WDisConnect_vif(priv,pLine);
	}
}
/*
**************************************************************
*@breif		连接到指定的ap
*@params	netdev:网卡设备
*			essid:网络名称
*			key_mgmt:秘钥加密方式
*@retval		none
**************************************************************
*/
extern int atbmwifi_sta_scan(struct atbmwifi_vif *priv);
int wpa_connect_ap(struct atbmwifi_vif *priv)
{
	atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,
		ATBM_NULL,WPA_EVENT__SUPPLICANT_START_CONNECT,ATBM_WPA_EVENT_NOACK);
	return 0;
}

int atbmwifi_wpa_event_associte_ap(struct atbmwifi_vif *priv)
{	
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)priv->appdata;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);

	ATBM_CHECK_AGRV_AVAILABLE_POINTER(wpa_s,config);
	priv->scan_no_connect_back = priv->scan_no_connect = 0;

	if(priv->join_status == ATBMWIFI__JOIN_STATUS_STA){
		wifi_printk(WIFI_ALWAYS,"atbmwifi_wpa_event_associte_ap() ---deauth\n");
		sta_deauth(priv);
	}

#if CONFIG_P2P
	if(!priv->p2pdata)
#endif
		priv->auto_connect_when_lost = 1;
#if (FAST_CONNECT_MODE == 0) && (FAST_CONNECT_NO_SCAN == 0)
	config->psk_set=0;
#endif
	wpa_s->wpa_state = ATBM_WPA_SCANNING;
	wpa_comm_init_extra_ie(priv);
#if FAST_CONNECT_NO_SCAN
	if(!priv->fast_conn_noscan){
		if(!priv->scan.in_progress)
			atbmwifi_sta_scan(priv);
	}else{
		atbm_queue_work(_atbmwifi_vifpriv_to_hwpriv(priv),priv->join_work);
	}
#else
	if(!priv->scan.in_progress){		
		atbmwifi_eloop_register_timeout(0,priv->scan_expire*1000,sta_scan_start_timer_func,(atbm_void *)priv,ATBM_NULL);
	}
	atbmwifi_sta_scan(priv);
#endif
	atbm_kfree(priv->extra_ie);
	priv->extra_ie = ATBM_NULL;
	priv->extra_ie_len = 0;
EXIT:
	return 0;
}

/****************************************************
Function Name: atbmwifi_scan_process
Return: scan status,0 success,other fail
******************************************************/
 int atbmwifi_scan_process(struct atbmwifi_vif *priv)
{
	if(!atbmwifi_is_sta_mode(priv->iftype)) {	
		wifi_printk(WIFI_ALWAYS,"not support scan in AP mode!\n");
		return -1;		
	}
	if(!priv->enabled){
		wifi_printk(WIFI_ALWAYS,"not support not enabled!\n");
		return -2;
	}

	if(priv->scan.scan_smartconfig){
		wifi_printk(WIFI_ALWAYS,"scan_smartconfig now!please try later!\n");
		return -3;
   }
	
	if((priv->assoc_ok==0) && ( priv->join_status == ATBMWIFI__JOIN_STATUS_STA)){
		 wifi_printk(WIFI_ALWAYS,"join now!please try later!\n");
		 return -6;
	}

	if(!priv->scan.in_progress){
		if(priv->scan_ret.info==ATBM_NULL){
			priv->scan_ret.info = (struct atbmwifi_scan_result_info *)atbm_kmalloc(sizeof(struct atbmwifi_scan_result_info) * MAX_SCAN_INFO_NUM,GFP_KERNEL);
			if(priv->scan_ret.info ==ATBM_NULL){
				wifi_printk(WIFI_ALWAYS,"scan malloc fail!");
				return -4;
			}
		}
		priv->scan_ret.len = 0;
		priv->scan.if_id = priv->if_id;
		priv->scan_expire = INITIAL_SCAN_EXPIRE;
		priv->scan_no_connect_back = priv->scan_no_connect;
		priv->scan_no_connect = 1;
		return atbmwifi_sta_scan(priv);
	}
	else {
		wifi_printk(WIFI_ALWAYS,"scan busy!please try later!");
		return -5;
	}
}
/*********************station interface**********************/
/****************************************************
Function Name: atbmwifi_scan
Return: NULL
******************************************************/

 int atbmwifi_scan(struct atbmwifi_vif *priv)
{
	 atbmwifi_wpa_event_queue((atbm_void*)priv,ATBM_NULL,
			 ATBM_NULL,WPA_EVENT__SUPPLICANT_SCAN,ATBM_WPA_EVENT_ACK);

	return 0;
}
 atbm_void atbmwifi_event_handler(struct atbmwifi_vif *priv,atbm_uint32 eventId,atbm_uint32 eventData)
{
	switch (eventId) {
		case WSM_EVENT_ERROR:
			/* I even don't know what is it about.. */
			//STUB();
			break;
		case WSM_EVENT_BSS_LOST:
		{
			wifi_printk(WIFI_DBG_ERROR,"[CQM] BSS lost.\n");
			
			atbmwifi_ieee80211_connection_loss(priv);
			if(atbmwifi_is_sta_mode(priv->iftype))
			{
				wifi_printk(WIFI_ALWAYS,"atbmwifi_event_handler() ---deauth\n");
				sta_deauth(priv);
			}
			else
			{
				//atbmwifi_ap_deauth(priv,StaMac);
			}
			break;
		}
		case WSM_EVENT_BSS_REGAINED:
		{
			//sta_printk(KERN_DEBUG "[CQM] BSS regained.\n");
			//priv->delayed_link_loss = 0;
			//atbm_spin_lock(&priv->bss_loss_lock);
			//priv->bss_loss_status = ATBMWIFI__BSS_LOSS_NONE;		
			//atbm_spin_unlock(&priv->bss_loss_lock);
			//cancel_delayed_work_sync(&priv->bss_loss_work);
			//cancel_delayed_work_sync(&priv->connection_loss_work);
			break;
		}
		case WSM_EVENT_RADAR_DETECTED:
			//STUB();
			break;
		case WSM_EVENT_RCPI_RSSI:
		{
			break;
		}
		case WSM_EVENT_BT_INACTIVE:
			//STUB();
			break;
		case WSM_EVENT_BT_ACTIVE:
			//STUB();
			break;
		case WSM_EVENT_INACTIVITY://WSM_EVENT_IND_INACTIVITY
		{
			int link_id = atbm_ffs((atbm_uint32)eventData) - 1;
			struct atbm_buff *skb;
	        struct atbmwifi_ieee80211_mgmt *deauth;
	        struct atbmwifi_link_entry *entry = ATBM_NULL;

			wifi_printk(WIFI_DBG_ERROR, "Inactivity Event Rx "
					"link_id %d\n", link_id);
			_atbmwifi_unmap_link(priv, link_id);

			skb = atbm_dev_alloc_skb(sizeof(struct atbmwifi_ieee80211_mgmt));
			//atbm_skb_reserve(skb, 64);
			deauth = (struct atbmwifi_ieee80211_mgmt *)atbm_skb_put(skb, sizeof(struct atbmwifi_ieee80211_mgmt));
            ATBM_WARN_ON_FUNC(!deauth);
            entry = &priv->link_id_db[link_id - 1];
            deauth->duration = 0;	
			atbm_memcpy(deauth->da, priv->mac_addr, ATBM_ETH_ALEN);
            atbm_memcpy(deauth->sa, entry->mac/*priv->link_id_db[i].mac*/, ATBM_ETH_ALEN);
            atbm_memcpy(deauth->bssid,priv->mac_addr, ATBM_ETH_ALEN);
			deauth->frame_control = atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT |
	                                            ATBM_IEEE80211_STYPE_DEAUTH |
	                                            ATBM_IEEE80211_FCTL_TODS);
            deauth->u.deauth.reason_code = ATBM_WLAN_REASON_DEAUTH_LEAVING;
            deauth->seq_ctrl = 0;
            if(atbmwifi_ieee80211_rx_irqsafe(priv, skb) != 0){
				atbm_dev_kfree_skb(skb);
			}
			atbm_set_tim_impl(priv);
			break;
		}
		case WSM_EVENT_PS_MODE_ERROR:
		{
		}
	}
}
int atbmwifi_disable_listening(struct atbmwifi_vif *priv)
{
	int ret;
	struct wsm_reset reset={0};	
	reset.reset_statistics = ATBM_TRUE;
	
#ifdef P2P_MULTIVIF
	if(priv->if_id != 2) {
		ATBM_WARN_ON_FUNC(priv->join_status > ATBMWIFI__JOIN_STATUS_MONITOR);
		return 0;
	}
#endif //change by wp
	priv->join_status = ATBMWIFI__JOIN_STATUS_PASSIVE;

	ATBM_WARN_ON_FUNC(priv->join_status > ATBMWIFI__JOIN_STATUS_MONITOR);

	ret = wsm_reset(priv->hw_priv, &reset, ATBM_WIFI_GENERIC_IF_ID);
	return ret;
}
atbm_void atbmwifi_stop(void)
{
		struct atbmwifi_common * hw_priv;
		int i = 0;	
		struct atbmwifi_vif *priv;	
		hw_priv = &g_hw_prv;
		atbm_for_each_vif(hw_priv,priv,i){
			switch (priv->join_status) {
				case ATBMWIFI__JOIN_STATUS_STA:
					atbmwifi_stop_sta(priv);
					break;
				case ATBMWIFI__JOIN_STATUS_AP:				
					atbmwifi_stop_ap(priv);				
					break;
				case ATBMWIFI__JOIN_STATUS_MONITOR:
					atbmwifi_disable_listening(priv);
					break;
				default:
					break;
				}	
			/* TODO:COMBO: May be reset of these variables "delayed_link_loss and
			 * join_status to default can be removed as dev_priv will be freed by
			 * mac80211 */
			//priv->delayed_link_loss = 0;
			priv->join_status = ATBMWIFI__JOIN_STATUS_PASSIVE;
			wsm_unlock_tx(hw_priv);
			priv->listening = ATBM_FALSE;
		}
		for (i = 0; i < 4; i++){
			atbmwifi_queue_clear(&hw_priv->tx_queue[i], -1);  //clear all queue
		}	
}


