
/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#define ATBMWIFI_MAC80211_RC_DEFAULT "minstrel_ht"
struct atbm_net_device_ops wifi_net_ops;
extern struct atbmwifi_common g_hw_prv;
extern atbm_void * netdev_drv_priv(struct atbm_net_device *ndev);
struct tcpip_opt * tcp_opt = ATBM_NULL;
struct atbm_net_device *atbm_netintf_init(atbm_void)
{
	struct atbm_net_device *ndev = ATBM_NULL;
	ndev = (struct atbm_net_device *)atbm_alloc_netdev(sizeof(struct atbmwifi_vif));
	if (!ndev){
		return ATBM_NULL;
	}
	ndev->netdev_ops = &wifi_net_ops;	
	return ndev;
}
 static int atbmwifi_ieee80211_open(struct atbmwifi_vif *priv)
{
	return 0;
}

 static int atbmwifi_ieee80211_stop(struct atbmwifi_vif *priv)
{
	return 0;
}

 int atbmwifi_ieee80211_subif_start_xmit(struct atbmwifi_vif *priv,struct atbm_buff *skb )
{
	skb->priority = atbm_cfg80211_classify8021d(skb);
#if ATBM_SUPPORT_BRIDGE
	if(atbmwifi_br_tx_skb(priv, skb))
#endif
	{
		atbmwifi_tx_start(skb, priv);
	}
	return 0;
}

 static int atbmwifi_ieee80211_change_mac(struct atbmwifi_vif *priv, atbm_uint8 *addr)
{
	int ret = 0;

	atbm_memcpy(priv->mac_addr,addr, ATBM_ETH_ALEN);

	return ret;
}
atbm_void net_device_ops_init(atbm_void)
{
	wifi_net_ops.ndo_open = atbmwifi_ieee80211_open;
	wifi_net_ops.ndo_stop = atbmwifi_ieee80211_stop;
	wifi_net_ops.ndo_start_xmit = atbmwifi_ieee80211_subif_start_xmit;
	wifi_net_ops.ndo_set_mac_address =atbmwifi_ieee80211_change_mac;
}
 atbm_void Iwip_Init(void)
{
	extern struct tcpip_opt lwip_tcp_opt;
	tcp_opt =  &lwip_tcp_opt;
}
 atbm_void  atbmwifi_netstack_init(struct atbmwifi_common *hw_priv)
{	
	atbm_skbbuffer_init();	
	/*Initial iwip net/stack*/
	Iwip_Init();
	wifi_printk(WIFI_ALWAYS,"atbmwifi_netstack_init\n");
	hmac_rc_init(hw_priv,ATBMWIFI_MAC80211_RC_DEFAULT);   //FIXME add minstel 
	/*other inital*/
	wpa_event_init();
	///TODO;
	wifi_printk(WIFI_ALWAYS,"atbmwifi_netstack_init   END\n");
}
 atbm_void  atbmwifi_netstack_deinit(atbm_void)
{
	
}
int atbm_inital_common(struct atbmwifi_vif *priv)
{
	struct config_edca_params *wmm_param;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	priv->rx_task_work		= atbm_init_work(hw_priv, atbm_rx_task_work,priv);	
	priv->set_tim_work 		= atbm_init_work(hw_priv, atbm_ap_set_tim_work,priv);
	priv->chantype_switch_work = atbm_init_work(hw_priv, atbmwifi_set_channel_work,priv);
	priv->event_work = atbm_init_work(hw_priv, atbm_wsm_event_work,priv);

	priv->enabled = 0;
	priv->sta_asleep_mask = 0;
	priv->buffered_set_mask = 0;
	priv->link_id_map = 0;
	priv->extra_ie= ATBM_NULL;
	priv->extra_ie_len= 0;
	priv->bss.sta_priv.wmm_used = 1;
	priv->bss.sta_priv.uapsd_supported = 1;
	priv->bss.sta_priv.ht = 1;
	priv->bss.parameter_set_count=15;
	priv->bss.channel_type = ATBM_NL80211_CHAN_HT20;
	
	priv->config.n_pairwise_cipher  =4;
	priv->uapsd_queues=ATBM_IEEE80211_DEFAULT_UAPSD_QUEUES;
	priv->uapsd_max_sp_len=ATBM_IEEE80211_DEFAULT_MAX_SP_LEN;
	
	priv->scan_ret.info = ATBM_NULL;
	priv->scan_ret.len = 0;
	priv->scan.scan_smartconfig = 0;
	ATBM_INIT_LIST_HEAD(&priv->scan.scan_req_list);

	priv->disable_beacon_filter = 1;
	priv->rx_filter.bssid = 1;	 
	/*if host send the probe Responde set 1,else lmac send set 0*/
	priv->rx_filter.probeResponder = 0;
	priv->rx_filter.keepalive = 1;

	atbm_spin_lock_init(&priv->ps_state_lock);
	//////////
	//initial WMM paramter

	//BE
	wmm_param=  &priv->wmm_params[ATBM_D11_ACI_AC_BE];
	wmm_param->wmep_acm = 0;
	wmm_param->aifns = 7;
	wmm_param->cwMin= 4;
	wmm_param->cwMax= 10;
	wmm_param->txOpLimit = 0;
	wmm_param->wmep_noackPolicy= 0;
	wmm_param->uapsdEnable = 1;

	//BK
	wmm_param=  &priv->wmm_params[ATBM_D11_ACI_AC_BK];
	wmm_param->wmep_acm = 0;
	wmm_param->aifns = 3;
	wmm_param->cwMin= 4;
	wmm_param->cwMax= 10;
	wmm_param->txOpLimit = 0;
	wmm_param->wmep_noackPolicy= 0;
	wmm_param->uapsdEnable = 1;

	//VI
	wmm_param=  &priv->wmm_params[ATBM_D11_ACI_AC_VI];
	wmm_param->wmep_acm = 0;
	wmm_param->aifns = 2;
	wmm_param->cwMin= 3;
	wmm_param->cwMax= 4;
	wmm_param->txOpLimit = 94;
	wmm_param->wmep_noackPolicy= 0;
	wmm_param->uapsdEnable = 1;
	//VO
	wmm_param=  &priv->wmm_params[ATBM_D11_ACI_AC_VO];
	wmm_param->wmep_acm = 0;
	wmm_param->aifns = 2;
	wmm_param->cwMin= 2;
	wmm_param->cwMax= 3;
	wmm_param->txOpLimit = 47;
	wmm_param->wmep_noackPolicy= 0;
	wmm_param->uapsdEnable = 1;
	return 0;
}
int atbm_free_common(struct atbmwifi_vif *priv)
{	
	struct atbmwifi_common *hw_priv;
	hw_priv=priv->hw_priv;
	
	atbm_os_DeleteMutex(&hw_priv->wsm_cmd_mux);	
	return 0;
}

int atbm_wifi_add_interfaces(struct atbmwifi_common *hw_priv,enum atbm_nl80211_iftype iftype, char *if_name)
{
	struct atbm_net_device *ndev = ATBM_NULL;
	struct atbmwifi_vif *priv = ATBM_NULL;

	ndev = atbm_netintf_init();
	if(!ndev)
		return -ATBM_ENOMEM;
	priv = (struct atbmwifi_vif *)netdev_drv_priv(ndev);
	priv->ndev = ndev;
	priv->hw_priv=hw_priv;
	ATBM_ASSERT(hw_priv->vif_current < 2);
	priv->if_id = hw_priv->vif_current++;
	hw_priv->vif_list[priv->if_id] = priv;
	atbm_memcpy(priv->if_name,if_name,ATBM_IFNAMSIZ);
	atbm_memcpy(priv->mac_addr,hw_priv->addresses[priv->if_id].addr,6);
#ifdef LINUX_OS
	atbm_memcpy(ndev->nif->perm_addr, hw_priv->addresses[priv->if_id].addr,6);
	atbm_memcpy(ndev->nif->dev_addr, ndev->nif->perm_addr,6);
#endif
	priv->iftype = ATBM_NUM_NL80211_IFTYPES;
	atbm_inital_common(priv);
	atbm_skb_queue_head_init(&priv->rx_task_skb_list);
#if ATBM_PKG_REORDER
	atbm_reorder_func_init(priv);
#endif
	atbmwifi_vif_setup(priv);
	atbmwifi_setup_mac_pvif(priv);
	atbmwifi_update_filtering(priv);
	return 0;
}

int atbm_wifi_remove_interfaces(struct atbmwifi_vif *priv){
#if ATBM_PKG_REORDER
	atbm_reorder_func_deinit(priv);
#endif
	atbm_free_netdev(priv->ndev);
}

int atbm_wifi_initialed(atbm_uint8 if_id)
{
	struct atbmwifi_vif *priv;
	priv=g_hw_prv.vif_list[if_id];
	return (priv!=ATBM_NULL) ? 1:0;
}
