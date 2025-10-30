/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"

extern int atbmwifi_event_OsCallback(atbm_void *prv,int eventid,atbm_void *param);
extern atbm_void hostap_sta_del(struct atbmwifi_vif *priv,atbm_uint8 * staMacAddr);

/*
event up to wpa_supplicant
event up to hostapd
event up to os

param: must copy to malloc buffer, will free 
if this is used to interface to wpa_supplicant ,may need used workqueu to call event,
because some  event callfunction  must notwait
*/
int atbmwifi_event_uplayer(struct atbmwifi_vif *priv,int eventid,atbm_uint8 *param)
{
	if((priv->iftype == ATBM_NL80211_IFTYPE_AP)||
		(priv->iftype == ATBM_NL80211_IFTYPE_P2P_GO)){
		wifi_printk(WIFI_DBG_MSG,"atbm: atbmwifi_event_uplayer(), event id=%d\n", eventid);
		switch(eventid){
			case ATBM_WIFI_AUTH_EVENT:
				break;
			case ATBM_WIFI_JOIN_EVENT:
				break;
			case ATBM_WIFI_ASSOC_EVENT:
				if((priv != ATBM_NULL) && (param!=ATBM_NULL))
				 	return hostapd_rx_assoc_req_event(priv,(struct atbm_buff*)param);
				break;
			case ATBM_WIFI_ASSOCRSP_TXOK_EVENT:
				if((priv != ATBM_NULL) && (param!=ATBM_NULL))
					hostapd_setup_4_way_handshake(priv,param);
				break;
			case ATBM_WIFI_DEAUTH_EVENT:
				if((priv != ATBM_NULL) && (param!=ATBM_NULL))
					hostap_sta_del(priv,(atbm_uint8 *) param);
				break;
			case ATBM_WIFI_DEASSOC_EVENT:
				break;
#if 1//!ATBM_SUPPORT_BRIDGE
			case ATBM_WIFI_ENABLE_NET_EVENT:
				if(priv != ATBM_NULL)
					tcp_opt->net_enable(priv->ndev);
				break;			
			case ATBM_WIFI_DISENABLE_NET_EVENT:
				if(priv != ATBM_NULL) 
					tcp_opt->net_disable(priv->ndev);
				break;
#endif
			case ATBM_WIFI_WPS_SUCCESS:
				break;
			default:
				break;
		}
	}
	else {	
		switch(eventid){
			case ATBM_WIFI_SCANSTART_EVENT:
				break;
			case ATBM_WIFI_SCANDONE_EVENT:
				break;
			case ATBM_WIFI_JOIN_EVENT:
				break;
			case ATBM_WIFI_AUTH_EVENT:
				if(priv != ATBM_NULL) 
					wpa_prepare_assciating(priv);
				break;
			case ATBM_WIFI_ASSOC_EVENT:
				if(priv != ATBM_NULL) 
					wpa_supplicant_event_assoc(priv,0);
				break;
			case ATBM_WIFI_DEAUTH_EVENT:
				if((priv != ATBM_NULL) && (param!=ATBM_NULL))
					wpa_supplicant_event_disauthen(priv,*(atbm_uint16 *)param);
				break;
			case ATBM_WIFI_DEASSOC_EVENT:
				if(priv != ATBM_NULL) 
					wpa_supplicant_event_disassoc(priv);
				break;
			case ATBM_WIFI_ENABLE_NET_EVENT:
				if(priv != ATBM_NULL) 
					tcp_opt->net_enable(priv->ndev);
				break;
			case ATBM_WIFI_DISENABLE_NET_EVENT:
				if(priv != ATBM_NULL) 
					tcp_opt->net_disable(priv->ndev);
				break;
			case ATBM_WIFI_WPS_SUCCESS:
                break;
			default:
				break;
		}
	}
	atbmwifi_event_OsCallback(priv, eventid,param);
	return 0;
}
