/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#include "atbm_hal.h"
#if ATBM_SUPPORT_BRIDGE
#include "atbm_bridge.h"
#endif

atbm_void atbmwifi_mcast_timeout(atbm_void *data1,atbm_void *data2);
extern atbm_void atbmwifi_queued_timeout(atbm_void *data1,atbm_void *data2);

int _atbmwifi_unmap_link(struct atbmwifi_vif *priv, int link_id)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_map_link maplink;
	maplink.link_id = link_id;
	maplink.unmap = ATBM_TRUE;
	if (link_id)
		atbm_memcpy(&maplink.mac_addr[0],
			priv->link_id_db[link_id - 1].mac, ATBM_ETH_ALEN);
	return wsm_map_link(hw_priv, &maplink, priv->if_id);
}
atbm_void atbmwifi_link_id_lmac(struct atbmwifi_vif *priv,int link_id )
{
	ATBM_BOOL need_reset;
	atbm_uint32 mask;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_map_link map_link;
	map_link.link_id = 0;
	map_link.unmap = 0;
	//int i=0;
	if (priv->join_status != ATBMWIFI__JOIN_STATUS_AP)
		return;

	//wsm_lock_tx(hw_priv);
	//for (i = 0; i < ATBMWIFI__MAX_STA_IN_AP_MODE; ++i) {
		need_reset = ATBM_FALSE;
		mask = BIT(link_id);
		if(priv->link_id_db[link_id-1].status == ATBMWIFI__LINK_HARD) {
			atbm_spin_lock_bh(&priv->ps_state_lock);
			if (priv->link_id_map & mask) {
				priv->sta_asleep_mask &= ~mask;
				priv->pspoll_mask &= ~mask;
				need_reset = ATBM_TRUE;
			}
			priv->link_id_map |= mask;
			
			atbm_memcpy(map_link.mac_addr, priv->link_id_db[link_id-1].mac,ATBM_ETH_ALEN);
			atbm_spin_unlock_bh(&priv->ps_state_lock);
			if (need_reset) {
				_atbmwifi_unmap_link(priv, link_id);
			}
			map_link.link_id = link_id;
			wsm_map_link(hw_priv, &map_link, priv->if_id);
		} 	
}

int atbmwifi_alloc_link_id(struct atbmwifi_vif *priv, const atbm_uint8 *mac)
{
	int i, ret = 0;
	for (i = 0; i < ATBMWIFI__MAX_STA_IN_AP_MODE; ++i) {
		if (!priv->link_id_db[i].status) {
			ret = i + 1;
			break;
		}
	}
	ATBM_WARN_ON_FUNC(ret>ATBMWIFI__MAX_STA_IN_AP_MODE);
	ATBM_WARN_ON_FUNC(ret==0);
	
	if (ret) {
		struct atbmwifi_link_entry *entry = &priv->link_id_db[ret - 1];
		wifi_printk(WIFI_ALWAYS,"[AP] STA added, link_id: %d\n",ret);
		entry->status = ATBMWIFI__LINK_RESERVE;
		atbm_memcpy(&entry->mac, mac, ATBM_ETH_ALEN);
		atbm_memset(&entry->buffered, 0, ATBMWIFI__MAX_TID);
		atbm_memcpy(entry->sta_priv.mac, mac, ATBM_ETH_ALEN);
		entry->sta_priv.priv =priv ;
		entry->sta_priv.link_id =ret ;
		entry->sta_priv.flags =0 ;
		entry->sta_priv.driver_buffered_tids =0 ;
	} 

	return ret;
}

/*find hard connected link id ,get sta mac address, and copy together*/
int atbmwifi_get_hard_linked_macs(struct atbmwifi_vif *priv,  atbm_uint8 *mac, atbm_uint32 maccnt)
{
	int i;
	atbm_uint8 *tmp = mac;
	int mac_copyed_len = 0;
	atbm_spin_lock_bh(&priv->ps_state_lock);

	for (i = 0; i <  ATBMWIFI__MAX_STA_IN_AP_MODE; ++i) {
		if ((priv->link_id_db[i].status==ATBMWIFI__LINK_HARD)) {

			atbm_memcpy(tmp, priv->link_id_db[i].mac,  ATBM_ETH_ALEN);
			mac_copyed_len++;
			tmp += ATBM_ETH_ALEN;	
			if(mac_copyed_len >= maccnt)
			{
				break;
			}
		}
	}
	atbm_spin_unlock_bh(&priv->ps_state_lock);
	return mac_copyed_len;
}
/*find link id ,have alloc link id*/
int atbmwifi_find_link_id(struct atbmwifi_vif *priv, const atbm_uint8 *mac)
{
	int i, ret = 0;
	atbm_spin_lock_bh(&priv->ps_state_lock);

	for (i = 0; i < ATBMWIFI__MAX_STA_IN_AP_MODE; ++i) {
		if (!atbm_memcmp(mac, priv->link_id_db[i].mac, ATBM_ETH_ALEN) &&
				priv->link_id_db[i].status) {
			ret = i + 1;
			break;
		}
	}
	atbm_spin_unlock_bh(&priv->ps_state_lock);
	return ret;
}
/*find link id ,have connect link id*/
int atbmwifi_find_hard_link_id(struct atbmwifi_vif *priv, const atbm_uint8 *mac)
{
	int i, ret = 0;
	atbm_spin_lock_bh(&priv->ps_state_lock);

	for (i = 0; i < ATBMWIFI__MAX_STA_IN_AP_MODE; ++i) {
		if (!atbm_memcmp(mac, priv->link_id_db[i].mac, ATBM_ETH_ALEN) &&
				(priv->link_id_db[i].status==ATBMWIFI__LINK_HARD)) {
			//priv->link_id_db[i].timestamp = atbm_GetOsTimeMs;
			ret = i + 1;
			break;
		}
	}
	atbm_spin_unlock_bh(&priv->ps_state_lock);
	return ret;
}


int atbmwifi_sta_alloc(struct atbmwifi_vif *priv,
		  atbm_uint8 *sta_mac)
{
	int link_id = 0;
	if (!atbmwifi_is_ap_mode(priv->iftype))
		return 0;
	link_id = atbmwifi_find_link_id(priv, sta_mac);	
	if(link_id==0) {
		link_id = atbmwifi_alloc_link_id(priv, sta_mac);
		if(link_id ==0){
			wifi_printk(WIFI_CONNECT,"%s %d Err1\n",__FUNCTION__, __LINE__);
			return -1;
		}
	}
	else {
		//if(priv->link_id_db[link_id-1].status != ATBMWIFI__LINK_RESERVE)
#if CONFIG_SAE
		struct hostapd_data *hapd	 = (struct hostapd_data *)priv->appdata;
		struct hostapd_sta_info *sta = ap_get_sta(hapd, sta_mac);
		if(sta && ((sta->aid != link_id) || (sta->sae.state != SAE_COMMITTED)))
#endif
		{
			atbmwifi_sta_del(priv,sta_mac);
		}
		priv->link_id_db[link_id-1].status = ATBMWIFI__LINK_RESERVE;
		atbm_memset(priv->link_id_db[link_id-1].buffered, 0, ATBMWIFI__MAX_TID);
	}

	return link_id;
}
int atbmwifi_sta_add(struct atbmwifi_vif *priv,
		  atbm_uint8 *sta_mac)
{
	int link_id = 0;
	struct atbmwifi_sta_priv *sta_priv = ATBM_NULL;
	//struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	
	if (!atbmwifi_is_ap_mode(priv->iftype))
		return 0;
	link_id = atbmwifi_find_hard_link_id(priv, sta_mac); 
	if(link_id !=0){
		wifi_printk(WIFI_CONNECT,"sta_add again just drop \n");
		return 0;
	}

	link_id = atbmwifi_find_link_id(priv, sta_mac);	
	if(link_id ==0){
		wifi_printk(WIFI_CONNECT,"sta_add Error \n");
		return -1;
	}
	
	priv->link_id_db[link_id-1].status = ATBMWIFI__LINK_HARD;
	sta_priv = &priv->link_id_db[link_id-1].sta_priv;
	sta_priv->link_id = link_id;
	atbm_memset(&priv->link_id_db[link_id-1].sta_retry,0xff,sizeof(struct atbmwifi_filter_retry));
#if RATE_CONTROL_MODE
	priv->link_id_db[link_id-1].sta_priv.sta_rc_priv = mac80211_ratectrl->alloc_sta();
	mac80211_ratectrl->sta_rate_init(&sta_priv->rate,sta_priv->sta_rc_priv);
#endif
	priv->sta_asleep_mask &= ~BIT(link_id);
	priv->buffered_set_mask &= ~BIT(link_id);
	atbmwifi_link_id_lmac(priv,link_id);
	wifi_printk(WIFI_CONNECT,"[ap]:assoc OK %d\n",link_id);
#if ATBM_DRIVER_PROCESS_BA
	if(priv->hw_priv->driver_setup_ba){
		int i;

		atbm_spin_lock_init(&sta_priv->lock);
		if(!sta_priv->ampdu_mlme.init){
			atbm_os_mutexLockInit(&sta_priv->ampdu_mlme.mtx);
			sta_priv->ampdu_mlme.work = atbm_init_work(priv->hw_priv, 
							ieee80211_ba_session_work, sta_priv);
			sta_priv->ampdu_mlme.init = 1;
		}
		sta_priv->priv = priv;
		atbm_memcpy(sta_priv->mac, sta_mac, ATBM_ETH_ALEN);
		sta_priv->link_id = link_id;
		for (i = 0; i < STA_TID_NUM; i++) {
			/*
			 * timer_to_tid must be initialized with identity mapping
			 * to enable session_timer's data differentiation. See
			 * sta_rx_agg_session_timer_expired for usage.
			 */
			sta_priv->timer_to_tid[i] = i;
			sta_priv->rx_token[i] = -1;
			sta_priv->ampdu_mlme.trylimit[i] = 0;
			sta_priv->ampdu_mlme.timestamp[i] = atbm_GetOsTimeMs();
		}

	}
#endif
	return 0;
}
int atbmwifi_sta_del(struct atbmwifi_vif *priv,
		 atbm_uint8 * staMacAddr)
{
	int link_id =0;

	wifi_printk(WIFI_CONNECT,"[ap]:atbmwifi_sta_del \n");

	if (!atbmwifi_is_ap_mode(priv->iftype))
		return 0;


	link_id = atbmwifi_find_link_id(priv, staMacAddr);
	if((link_id > ATBMWIFI__MAX_STA_IN_AP_MODE) || ( link_id<=0)){
		wifi_printk(WIFI_DBG_MSG,"[ap]:sta_del link_id 0 drop\n");
		return -1;
	}
	//del hostapd sta priv
	priv->link_id_db[link_id-1].sta_priv.reserved = ATBM_NULL;
	atbmwifi_event_uplayer(priv,ATBM_WIFI_DEAUTH_EVENT,staMacAddr);

	if((link_id <= ATBMWIFI__MAX_STA_IN_AP_MODE) && link_id>0){
#if ATBM_SUPPORT_BRIDGE
		remove_item_from_brpool(link_id);
#endif
#if ATBM_DRIVER_PROCESS_BA
		if(priv->hw_priv->driver_setup_ba){
			if(priv->link_id_db[link_id-1].sta_priv.ampdu_mlme.init){
				ieee80211_sta_tear_down_BA_sessions(&priv->link_id_db[link_id-1].sta_priv, ATBM_TRUE);
				atbm_os_DeleteMutex(&priv->link_id_db[link_id-1].sta_priv.ampdu_mlme.mtx);
			}
		}
#endif
		priv->link_id_db[link_id-1].status = ATBMWIFI__LINK_OFF;
		__atbm_flush(priv->hw_priv, ATBM_FALSE, priv->if_id, BIT(link_id));
#if RATE_CONTROL_MODE
		if(priv->link_id_db[link_id-1].sta_priv.sta_rc_priv){
			mac80211_ratectrl->free_sta(priv->link_id_db[link_id-1].sta_priv.sta_rc_priv);
			priv->link_id_db[link_id-1].sta_priv.sta_rc_priv = ATBM_NULL;
		}
#endif
		atbmwifi_del_key(priv,0, link_id);
#if ATBM_PKG_REORDER
		atbm_reorder_func_reset(priv,link_id - 1);
#endif	//ATBM_PKG_REORDER		
		_atbmwifi_unmap_link(priv, link_id);
		priv->pspoll_mask &= ~BIT(link_id);
		priv->sta_asleep_mask &= ~BIT(link_id);
		priv->buffered_set_mask &= ~BIT(link_id);
		atbm_memset(&priv->link_id_db[link_id-1].sta_retry,0xff,sizeof(struct atbmwifi_filter_retry));
		wifi_printk(WIFI_DBG_ANY,"[ap]:sta_del link_id %d\n",link_id);
		atbm_memset(&priv->link_id_db[link_id-1].sta_priv, 0, sizeof(struct atbmwifi_sta_priv));
	}

	return 0;
}

struct atbmwifi_sta_priv *atbmwifi_sta_find(struct atbmwifi_vif *priv,const atbm_uint8 *mac)
{	
	int i =0;	
	
	if (atbmwifi_is_sta_mode(priv->iftype))
		return &priv->bss.sta_priv;
	
	for (i = 0; i < ATBMWIFI__MAX_STA_IN_AP_MODE; ++i) {
		if ((priv->link_id_db[i].status == ATBMWIFI__LINK_HARD) &&
			!atbm_memcmp(mac, priv->link_id_db[i].mac, ATBM_ETH_ALEN)){
				return &priv->link_id_db[i].sta_priv;
		}
	}

	return ATBM_NULL;
}

struct atbmwifi_sta_priv *atbmwifi_sta_find_form_hard_linkid(struct atbmwifi_vif *priv,const atbm_uint8 linkid)
{	

	if (!atbmwifi_is_ap_mode(priv->iftype))
		return ATBM_NULL;
	
	if(linkid > ATBMWIFI__MAX_STA_IN_AP_MODE)
		return ATBM_NULL;
	if(linkid == 0)
		return ATBM_NULL;
	
	if(priv->link_id_db[linkid-1].status == ATBMWIFI__LINK_HARD)
		return &priv->link_id_db[linkid-1].sta_priv;
	else 
		return ATBM_NULL;
}

struct atbmwifi_sta_priv *atbmwifi_sta_find_form_linkid(struct atbmwifi_vif *priv,const atbm_uint8 linkid)
{	

	if (!atbmwifi_is_ap_mode(priv->iftype))
		return ATBM_NULL;
	
	if(linkid > ATBMWIFI__MAX_STA_IN_AP_MODE)
		return ATBM_NULL;
	if(linkid == 0)
		return ATBM_NULL;
	
	if(priv->link_id_db[linkid-1].status != ATBMWIFI__LINK_OFF)
		return &priv->link_id_db[linkid-1].sta_priv;
	else 
		return ATBM_NULL;
}

static atbm_void __atbm_sta_notify(struct atbmwifi_vif *priv,
				enum sta_notify_cmd notify_cmd,
				int link_id)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	atbm_uint32 bit, prev;

	/* Zero link id means "for all link IDs" */
	if (link_id){
		bit = BIT(link_id);
	}
	else if (ATBM_WARN_ON(notify_cmd != STA_NOTIFY_AWAKE)){
		bit = 0;
	}
	else{
		bit = priv->link_id_map;
	}
	prev = priv->sta_asleep_mask & bit;
	switch (notify_cmd) {
	case STA_NOTIFY_SLEEP:
		if (!prev) {
			if (priv->buffered_multicasts &&
					!priv->sta_asleep_mask)
				atbm_queue_work(priv->hw_priv, priv->set_tim_work);
			priv->sta_asleep_mask |= bit;
			wifi_printk(WIFI_PS,"STA_NOTIFY_SLEEP--->sta_asleep_mask %x\n",priv->sta_asleep_mask);
		}
		break;
	case STA_NOTIFY_AWAKE:
		if (prev) {
			priv->sta_asleep_mask &= ~bit;
			priv->pspoll_mask &= ~bit;
			priv->link_id_uapsd_mask &= ~bit;
			if (priv->tx_multicast && link_id &&
					!priv->sta_asleep_mask)
				atbm_queue_work(priv->hw_priv, priv->set_tim_work);
			atbm_bh_wakeup(hw_priv);
			wifi_printk(WIFI_PS,"STA_NOTIFY_AWAKE--->sta_asleep_mask %x\n",priv->sta_asleep_mask);
		}
		break;
	}
}


atbm_void atbm_ps_notify(struct atbmwifi_vif *priv,
		      int link_id, ATBM_BOOL ps)
{
	if (link_id > ATBMWIFI__MAX_STA_IN_AP_MODE)
		return;

	wifi_printk(WIFI_PS,"%s for LinkId: %d. STAs asleep: %.8X\n",
			ps ? "Stop" : "Start",
			link_id, priv->sta_asleep_mask);

	/* TODO:COMBO: __atbm_sta_notify changed. */
	__atbm_sta_notify(priv,
		ps ? STA_NOTIFY_SLEEP : STA_NOTIFY_AWAKE, link_id);
}

int atbm_set_tim_impl(struct atbmwifi_vif *priv)
{
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	
	struct atbmwifi_cfg *config=atbmwifi_get_config(priv);
	atbm_uint8 * tim_ie =ATBM_NULL;
	atbm_uint8 * tim_ie_end=ATBM_NULL;
	atbm_uint8 aid0_bit_set;
	struct wsm_update_ie update_ie={0};
	update_ie.what = WSM_UPDATE_IE_BEACON;
	update_ie.count = 1;
	update_ie.length = 0;
	tim_ie = (atbm_uint8 *)atbm_kmalloc(sizeof(struct atbmwifi_ieee80211_tim_ie)+ ATBMWIFI__MAX_STA_IN_AP_MODE/8 + 4,GFP_KERNEL);
	
	atbm_spin_lock_bh(&priv->ps_state_lock);
	aid0_bit_set = priv->buffered_multicasts;
	tim_ie_end = atbmwifi_add_tim(tim_ie,priv,priv->buffered_multicasts);
	atbm_spin_unlock_bh(&priv->ps_state_lock);
	if(aid0_bit_set != priv->aid0_bit_set){
		long tmo=0;
		tmo=config->DTIMPeriod*(config->beaconInterval + 20);
		atbmwifi_eloop_register_timeout(0,tmo,atbmwifi_mcast_timeout,(atbm_void *)priv,ATBM_NULL);
	}

	update_ie.ies = tim_ie;
	update_ie.length = (atbm_uint32)tim_ie_end-(atbm_uint32)tim_ie;
	ATBM_WARN_ON_FUNC(wsm_update_ie(hw_priv, &update_ie, priv->if_id));
	priv->aid0_bit_set = aid0_bit_set;

	atbm_kfree(tim_ie);

	return 0;
}

atbm_void atbm_ap_set_tim_work(struct atbm_work_struct *work)
{
	struct atbmwifi_vif *priv=(struct atbmwifi_vif *)work;
	if(atbm_bh_is_term(priv->hw_priv)){
		return;
	}
	atbm_set_tim_impl(priv);
}

int atbm_set_tim(struct atbmwifi_vif *priv, struct atbmwifi_sta_priv  *sta_priv,ATBM_BOOL set)
{
	if(atbm_bh_is_term(priv->hw_priv)){
		return 0;
	}
#ifdef P2P_MULTIVIF
	ATBM_WARN_ON_FUNC(priv->if_id == ATBM_WIFI_GENERIC_IF_ID);
#endif
	ATBM_WARN_ON_FUNC(!atbmwifi_is_ap_mode(priv->iftype));
	if(atbmwifi_set_tim(priv,sta_priv->link_id,set)){
		if(sta_priv->priv->sta_asleep_mask & BIT(sta_priv->link_id)){
			atbm_queue_work(priv->hw_priv, priv->set_tim_work);
		}
	}
	return 0;
}
/*if timeout ,must send mulcast frame*/
atbm_void atbmwifi_mcast_timeout(atbm_void *data1,atbm_void *data2)
{
	struct atbmwifi_vif *priv =	(struct atbmwifi_vif *)data1;	
	atbm_spin_lock_bh(&priv->ps_state_lock);
	priv->tx_multicast = priv->aid0_bit_set && priv->buffered_multicasts;
	if (priv->tx_multicast){
//		atbm_os_wakeup_event(&_atbmwifi_vifpriv_to_hwpriv(priv)->tx_bh_wq);
		atbm_bh_schedule_tx(_atbmwifi_vifpriv_to_hwpriv(priv));
	}
	atbm_spin_unlock_bh(&priv->ps_state_lock);
}

/* ******************************************************************** */
/* WSM callback		 LMACtoUMAC_SuspendResumeTxInd when tx DTIM	*/
atbm_void atbm_suspend_resume(struct atbmwifi_vif *priv,
			   struct wsm_suspend_resume *arg)
{
	struct atbmwifi_common *hw_priv =
		_atbmwifi_vifpriv_to_hwpriv(priv);

	wifi_printk(WIFI_PS, "[AP] %s: %s\n",
			arg->stop ? "stop" : "start",
			arg->multicast ? "broadcast" : "unicast");

	if (arg->multicast) {
		ATBM_BOOL cancel_tmo = ATBM_FALSE;
		atbm_spin_lock_bh(&priv->ps_state_lock);
		if (arg->stop) {
			priv->tx_multicast = ATBM_FALSE;
		} else {
#if NEW_SUPPORT_PS
			//atbm_uint32 ac,n_frames;
			/* Firmware sends this indication every DTIM if there
			 * is a STA in powersave connected. There is no reason
			 * to suspend, following wakeup will consume much more
			 * power than it could be saved. */	
			 
			 priv->tx_multicast = (priv->aid0_bit_set &&priv->buffered_multicasts);
			 if(priv->tx_multicast)
			 {
			 	cancel_tmo= ATBM_TRUE;
//			 	wifi_printk(WIFI_PS,"--->muticast num n_frames =%d\n",n_frames); 
				atbm_bh_schedule_tx(priv->hw_priv);
			 }

			 
#endif
		}
		atbm_spin_unlock_bh(&priv->ps_state_lock);
		if (cancel_tmo)
		atbmwifi_eloop_cancel_timeout(atbmwifi_mcast_timeout, (atbm_void *)priv, ATBM_NULL);
	} else {
		/*lmac call here when p2p ps mode*/
		atbm_spin_lock_bh(&priv->ps_state_lock);
		atbm_ps_notify(priv, arg->link_id, arg->stop);
		atbm_spin_unlock_bh(&priv->ps_state_lock);
		if (!arg->stop)
			atbm_bh_wakeup(hw_priv);
	}
	return;
}
int atbmwifi_ap_deauth(struct atbmwifi_vif *priv,atbm_uint8 *staMacAddr)
{
	int link_id = atbmwifi_find_link_id(priv, staMacAddr);
	//wifi_printk(WIFI_ALWAYS,"%s\n",__FUNCTION__);
	if((link_id > ATBMWIFI__MAX_STA_IN_AP_MODE) || ( link_id<0)){
		return -1;
	}
	atbmwifi_sta_del(priv,staMacAddr);
	return 1;
}

int atbmwifi_ap_start_proberesp(struct atbmwifi_vif *priv)
{	
	int ret;
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_template_frame frame={0};
	frame.frame_type = WSM_FRAME_TYPE_PROBE_RESPONSE;
	frame.disable =0; 
	//frame.rate = test_config_txrx.Rate;
#if CONFIG_P2P
	if(priv->p2p_ap)
		frame.rate = RATE_INDEX_A_6M;
	else
#endif
#if CONFIG_5G_SUPPORT
	if(priv->bss.sta_priv.band == ATBM_IEEE80211_BAND_5GHZ)
		frame.rate = RATE_INDEX_A_6M;
	else
#endif
	frame.rate = RATE_INDEX_B_1M;
	frame.skb = atbmwifi_ieee80211_send_proberesp(priv,priv->extra_ie,priv->extra_ie_len);
	if (ATBM_WARN_ON(!frame.skb))
		return 0;
	ret = wsm_set_template_frame(hw_priv, &frame, priv->if_id);
	atbm_dev_kfree_skb(frame.skb);

	return ret;
}
int  atbmwifi_ap_start_beacon(struct atbmwifi_vif *priv)
{
	int ret = 0;
	struct atbmwifi_common *hw_priv =_atbmwifi_vifpriv_to_hwpriv(priv);	
	struct wsm_template_frame frame={0};
	frame.frame_type = WSM_FRAME_TYPE_BEACON;	
	frame.disable =0; 

	//frame.rate = test_config_txrx.Rate;
#if CONFIG_P2P
	if(priv->p2p_ap)
		frame.rate = RATE_INDEX_A_6M;
	else
#endif
#if CONFIG_5G_SUPPORT
	if(priv->bss.sta_priv.band == ATBM_IEEE80211_BAND_5GHZ)
		frame.rate = RATE_INDEX_A_6M;
	else
#endif
	frame.rate = RATE_INDEX_B_1M;
	frame.skb = atbmwifi_ieee80211_send_beacon(priv,priv->extra_ie,priv->extra_ie_len);
	if (ATBM_WARN_ON(!frame.skb))
		return 0;
	
	ret = wsm_set_template_frame(hw_priv, &frame, priv->if_id);
	if (!ret)
	{
		atbmwifi_ap_start_proberesp(priv);
	}
	
	atbm_dev_kfree_skb(frame.skb);

	return ret;	
}
int atbm_start_ap(struct atbmwifi_vif *priv)
{
	int ret;
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct wsm_start start={0};
	struct wsm_inactivity inactivity={0} ;
	struct wsm_operational_mode mode={0};
	start.mode =  priv->config.mode;
	start.band =  priv->config.band;
	start.channelNumber =  config->channel_index;
	start.beaconInterval = config->beaconInterval;
	start.DTIMPeriod 	= config->DTIMPeriod;
	start.preambleType 	= config->preambleType;
	start.probeDelay 	= 100;
	start.basicRateSet 	= config->basicRateSet;
//	start.channel_type = hw_priv->channel_type;
	start.channel_type = priv->bss.channel_type;

	inactivity.min_inactivity = 39;
	inactivity.max_inactivity = 1;

	mode.power_mode = wsm_power_mode_active;
	mode.disableMoreFlagUsage = ATBM_TRUE;
	
	if (priv->if_id)
		start.mode |= WSM_FLAG_MAC_INSTANCE_1;
	else
		start.mode &= ~WSM_FLAG_MAC_INSTANCE_1;

	if(priv->hw_priv->channel_type	== ATBM_NL80211_CHAN_HT40PLUS){
		config->secondary_channel=1;//above bandwidth
	}else{
		config->secondary_channel=-1;//below bandwidth
	}
	atbmwifi_ap_start_beacon(priv);

	priv->tx_multicast = ATBM_FALSE;
	priv->aid0_bit_set = ATBM_FALSE;
	priv->buffered_multicasts = ATBM_FALSE;
	priv->pspoll_mask = 0;
	priv->link_id_uapsd_mask=0;
	//atbm_InitTimer(&priv->mcast_timeout,atbmwifi_mcast_timeout,(atbm_void*)priv);

	start.ssidLength = priv->config.ssid_len;
	atbm_memcpy(&start.ssid[0], priv->config.ssid, start.ssidLength);
	atbm_memset(&priv->link_id_db[0], 0, sizeof(priv->link_id_db));

	wifi_printk(WIFI_CONNECT, "[AP] ch: %d(%d), bcn: %d(%d), "
		"brt: 0x%x, ssid: %s %d\n",
		start.channelNumber, start.band,
		start.beaconInterval, start.DTIMPeriod,
		start.basicRateSet,
		start.ssid,priv->if_id);
	
	ret = wsm_start(hw_priv, &start, priv->if_id);
	wsm_set_inactivity(hw_priv, &inactivity, priv->if_id);
	if (!ret) {		
		priv->join_status = ATBMWIFI__JOIN_STATUS_AP;
		atbmwifi_event_uplayer(priv,ATBM_WIFI_ENABLE_NET_EVENT,ATBM_NULL);
	}

	wsm_set_block_ack_policy(hw_priv,
		hw_priv->ba_tid_tx_mask,
		hw_priv->ba_tid_rx_mask,
		priv->if_id);
	wsm_set_operational_mode(hw_priv, &mode, priv->if_id);
	return ret;
}

atbm_void atbmwifi_ap_deauth_sta(struct atbmwifi_vif *priv,atbm_uint8 link_id,int reason_code)
{

	struct atbmwifi_sta_priv * sta_priv = atbmwifi_sta_find_form_linkid(priv,priv->connect_timer_linkid);
	if(sta_priv){		
		wifi_printk(WIFI_ALWAYS,"atbmwifi_ap_deauth_sta\n");
		atbmwifi_ieee80211_tx_mgmt_deauth(priv,sta_priv->mac,priv->bssid,reason_code);
		if(atbmwifi_ap_deauth(priv,sta_priv->mac) < 0){	
			atbmwifi_event_uplayer(priv,ATBM_WIFI_DEAUTH_EVENT,sta_priv->mac);
		}
	}
}
atbm_void atbmwifi_ap_join_timeout(atbm_void *data1,atbm_void *data2)
{	
	//atbmwifi_autoconnect(priv);
	struct atbmwifi_vif *priv=(struct atbmwifi_vif *)data1;
	wifi_printk(WIFI_WPA,"atbm: atbmwifi_ap_join_timeout(), ms=%d\n", atbm_GetOsTimeMs());
	atbmwifi_ap_deauth_sta(priv,priv->connect_timer_linkid,ATBM_WLAN_REASON_DISASSOC_STA_HAS_LEFT);
}
atbm_void atbmwifi_ap_deauth_all(struct atbmwifi_vif *priv)
{
	//del group key
	atbmwifi_del_key(priv,1,1);
	if(priv->bss.information_elements){		
		atbm_kfree(priv->bss.information_elements);
		priv->bss.information_elements = ATBM_NULL;
		priv->bss.len_information_elements = 0;
	}
#if ATBM_PKG_REORDER
	atbm_reorder_func_reset(priv,0xff);
#endif

}

atbm_void atbmwifi_stop_ap(struct atbmwifi_vif *priv)
{
	int i;
	atbm_uint8 cnt = 0;
	atbm_uint32 link_id_map = 0;
	
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);
	struct wsm_reset reset={0};
	struct wsm_operational_mode mode={0};
	{
		reset.reset_statistics = ATBM_TRUE;
	};

	if(!priv->enabled){
		wifi_printk(WIFI_ALWAYS,"atbmwifi_stop_ap drop\n");
		goto ap_off;
	}
	wifi_printk(WIFI_ALWAYS,"%s\n",__FUNCTION__);

	if(!atbmwifi_is_ap_mode(priv->iftype)){
		goto ap_off;
	}

	atbm_cancel_work(hw_priv, priv->scan.ap_scan_work);	


	link_id_map = priv->link_id_map;
	wifi_printk(WIFI_ALWAYS,"%s link_id_map=0x%x\n",__FUNCTION__, link_id_map);
	for (i = 0; link_id_map; ++i) {
		if (link_id_map & BIT(i)) {
			if(i > 0){				
				while(cnt++ < 3){
					atbmwifi_ieee80211_tx_mgmt_deauth(priv, priv->link_id_db[i-1].mac, priv->bssid, ATBM_WLAN_REASON_DISASSOC_AP_BUSY);
					atbmwifi_ieee80211_send_deauth_disassoc(priv, priv->link_id_db[i-1].mac,priv->bssid,
							   ATBM_IEEE80211_STYPE_DISASSOC,
							   ATBM_WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY,
							   ATBM_NULL, ATBM_TRUE);

				}
				
				cnt = 0;
			}
			link_id_map &= ~BIT(i);
		}
	}
	
	__atbm_flush(hw_priv, ATBM_FALSE, priv->if_id, BIT(0));
	mode.power_mode = wsm_power_mode_quiescent;
	mode.disableMoreFlagUsage = ATBM_TRUE;
	
	for (i = 0; priv->link_id_map; ++i) {
		if (priv->link_id_map & BIT(i)) {
			if(i > 0){
				atbmwifi_ap_deauth(priv, priv->link_id_db[i-1].mac);
			}
			priv->link_id_map &= ~BIT(i);
		}
	}
	atbmwifi_ap_deauth_all(priv);
	priv->enabled = 0;
	atbm_memset(priv->link_id_db, 0,sizeof(priv->link_id_db));
	priv->sta_asleep_mask = 0;
	priv->buffered_set_mask = 0;
	priv->enable_beacon = ATBM_FALSE;
	priv->tx_multicast = ATBM_FALSE;
	priv->aid0_bit_set = ATBM_FALSE;
	priv->buffered_multicasts = ATBM_FALSE;
	priv->pspoll_mask = 0;
	reset.link_id = 0;
	wsm_reset(priv->hw_priv, &reset, priv->if_id);
	ATBM_WARN_ON_FUNC(wsm_set_operational_mode(priv->hw_priv, &mode, priv->if_id));
	priv->connect.crypto_pairwise=0;
	priv->connect.crypto_group=0;
	priv->connect.encrype = 0;
	priv->assoc_ok = 0;
	priv->connect_ok = 0;
	atbmwifi_eloop_cancel_timeout(atbmwifi_mcast_timeout, (atbm_void *)priv, ATBM_NULL);
	atbmwifi_eloop_cancel_timeout(atbmwifi_ap_join_timeout, (atbm_void *)priv, ATBM_NULL);
	atbmwifi_eloop_cancel_timeout(atbmwifi_queued_timeout,(atbm_void *)hw_priv->tx_queue,ATBM_NULL);
	
	atbmwifi_event_uplayer(priv,ATBM_WIFI_DEAUTH_EVENT,atbm_broadcast_ether_addr);
	atbmwifi_event_uplayer(priv,ATBM_WIFI_DISENABLE_NET_EVENT,ATBM_NULL);
	priv->join_status = ATBMWIFI__JOIN_STATUS_PASSIVE;
	atbm_memset(config,0,sizeof(struct atbmwifi_cfg));
	atbm_clear_scan_req(priv);

	if(priv->extra_ie){
		atbm_kfree(priv->extra_ie);
		priv->extra_ie = ATBM_NULL;
	}
	free_hostapd(priv);
	priv->appdata = ATBM_NULL;
	atbm_mdelay(100);
ap_off:
	priv->iftype = ATBM_NUM_NL80211_IFTYPES;
}
atbm_void atbmwifi_start_hostapd(struct atbmwifi_vif *priv)
{
	priv->appdata = init_hostapd(priv);
}
atbm_void atbmwifi_start_ap(struct atbmwifi_vif *priv)
{
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	wifi_printk(WIFI_ALWAYS,"atbmwifi_start_ap++\n");
	priv->config.mode = (priv->iftype == ATBM_NL80211_IFTYPE_P2P_GO) ? WSM_START_MODE_P2P_GO : WSM_START_MODE_AP;

#if CONFIG_P2P
	priv->bss.sta_priv.wmm_used = 1;
	priv->bss.sta_priv.uapsd_supported = 1;
	priv->bss.sta_priv.ht = 1;
	priv->bss.parameter_set_count=1;
	priv->bss.channel_type = ATBM_NL80211_CHAN_HT20;
#endif

	atbm_memcpy(priv->bssid ,priv->mac_addr,ATBM_ETH_ALEN);
	atbm_memcpy(priv->config.bssid,priv->bssid,6);
	priv->enabled = 1;
	atbmwifi_start_hostapd(priv);
	//atbm_InitTimer(&priv->connect_expire_timer,atbmwifi_ap_join_timeout,(atbm_void*)priv);
	priv->scan.ap_scan_work = atbm_init_work(hw_priv, atbmwifi_ap_scan_start,priv);
}

int atbmwifi_ap_scan_start(struct atbm_work_struct *work)
{
	struct atbmwifi_vif *priv=(struct atbmwifi_vif *)work;

	if(priv->scan.ApScan_in_process){
		return;
	}

	priv->scan.ApScan_in_process = 1;

	atbmwifi_init_scan_req(priv, ATBM_SCAN_24G_ACTIVE);
#if CONFIG_5G_SUPPORT
	atbmwifi_init_scan_req(priv, ATBM_SCAN_5G_PASSAVE);
#endif
	atbm_req_scan_start(priv);

	return 0;
}

int ap_scan(struct atbmwifi_vif *priv)
{	
	struct atbmwifi_common *hw_priv = _atbmwifi_vifpriv_to_hwpriv(priv);
	
	atbm_queue_work(hw_priv,priv->scan.ap_scan_work);
	return 0;
}

int atbmwifi_ap_auto_process(struct atbmwifi_vif *priv)
{
	wifi_printk(WIFI_ALWAYS,"%s ApScan_in_process=%d\n",__FUNCTION__,priv->scan.ApScan_in_process);
	if(!priv->scan.ApScan_in_process){
		if(priv->scan_ret.info==ATBM_NULL){
			priv->scan_ret.info = (struct atbmwifi_scan_result_info *)atbm_kmalloc(sizeof(struct atbmwifi_scan_result_info) * MAX_SCAN_INFO_NUM,GFP_KERNEL);
			if(priv->scan_ret.info ==ATBM_NULL){
				wifi_printk(WIFI_ALWAYS,"scan malloc fail!");
				return -1;
			}
		}
		priv->scan_ret.len = 0;
		priv->scan.if_id = priv->if_id;
		priv->scan_no_connect = 1;
		return ap_scan(priv);
	}
	else {
		wifi_printk(WIFI_ALWAYS,"scan busy!please try later!");
		return -2;
	}
}
static int WEIGHT(int x){
	if((x)<=-75){ 
		return 0; 
	}else if((x)<=-65){ 
		return 1; 
	}else if((x)<=-50){
		return 2;
	}else if((x)>-50){
		return 3;
	}
	return 3;
}

#define NUM_AP_CHNN 13
#define MIN(x,y) ((x)<(y)?(x):(y))

#define WAIT_CMP(x) \
	while (x) { \
		atbm_mdelay(3000); \
	} \

static atbm_uint8 ReturnMin_InSort_1(atbm_uint8 * aCHList,atbm_uint8 *ChanArray,atbm_uint8 index,atbm_uint8 offset){
	atbm_uint8 i,j;
	atbm_uint8 minIndex;
		minIndex=i=0;
		for(j=i+1;j<index;++j){
			if(aCHList[ChanArray[j]-offset]<aCHList[ChanArray[minIndex]-offset])
				minIndex=j;
		}
	//	if(minIndex!=i)	
		//	AtbmSwap(&aCHList[ChanArray[i]-offset], &aCHList[ChanArray[minIndex]-offset]);

		//wifi_printk(WIFI_ALWAYS,"Array[ChanArray[%d] %d\n",i,aCHList[ChanArray[i]-offset]);
	//}
	//wifi_printk(WIFI_ALWAYS,"ReturnMin_InSort_1 minIndex=%d: %d\n",minIndex,ChanArray[minIndex]);
	return ChanArray[minIndex];
}	
int	atbm_autoChann_Select(struct atbmwifi_vif *priv,atbm_uint8 *SetChan)
{
	atbm_uint8 chanNum,index=0,ssidNum=0;
	atbm_uint8 aCHList[NUM_AP_CHNN]={0};
	atbm_uint8 MinChanNum[14]={1,2,3,4,5,6,7,8,9,10,11,12,13,14};

	WLAN_SCAN_RESULT *AutoScanBuf=ATBM_NULL;
	WLAN_BSS_INFO *bss_info;
	struct atbmwifi_scan_result_info *info;
	struct atbmwifi_common *hw_priv =priv->hw_priv;
	/*For AP ScanEnd Flag */
	hw_priv->ApScan_process_flag=1;
	/*Do ap auto scan process*/
	WAIT_CMP(!priv->enabled);
	if(atbmwifi_ap_auto_process(priv)){
		return -1;
	}
	/*Wait for scan cmp*/
	WAIT_CMP(priv->scan.ApScan_in_process);
	/*It means No ap in all channl,default channel 11*/
	if(priv->scan_ret.len==0){
		*SetChan=2;
	}else{
		AutoScanBuf=(WLAN_SCAN_RESULT*)atbm_kmalloc(priv->scan_ret.len*sizeof(WLAN_SCAN_RESULT),GFP_KERNEL);
		bss_info =  (WLAN_BSS_INFO *)(&AutoScanBuf->bss_info[0]);
		for(ssidNum=0;ssidNum<priv->scan_ret.len;ssidNum++){
			info = priv->scan_ret.info + ssidNum;
			bss_info->chanspec      = info->channel;
			bss_info->RSSI = info->rssi;
			/*Auto chann select process*/
			switch(bss_info->chanspec){
				case 1:
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec+3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+3]+=WEIGHT(bss_info->RSSI-8);
					break;
				case 2:
					if(bss_info->chanspec-1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec+3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+3]+=WEIGHT(bss_info->RSSI-8);
					break;
				case 3:
					if(bss_info->chanspec-2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec-1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec+3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+3]+=WEIGHT(bss_info->RSSI-8);
					break;
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if(bss_info->chanspec-3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-3]+=WEIGHT(bss_info->RSSI-8);
					if(bss_info->chanspec-2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec-1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec+3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+3]+=WEIGHT(bss_info->RSSI-8);
					break;
				case 11:
					if(bss_info->chanspec-3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-3]+=WEIGHT(bss_info->RSSI-8);
					if(bss_info->chanspec-2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec-1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+2]+=WEIGHT(bss_info->RSSI-4);
					break;
				case 12:
					if(bss_info->chanspec-3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-3]+=WEIGHT(bss_info->RSSI-8);
					if(bss_info->chanspec-2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec-1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-1]+=WEIGHT(bss_info->RSSI-2);
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec+1]+=WEIGHT(bss_info->RSSI-2);
					break;
				case 13:
					if(bss_info->chanspec-3 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-3]+=WEIGHT(bss_info->RSSI-8);
					if(bss_info->chanspec-2 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-2]+=WEIGHT(bss_info->RSSI-4);
					if(bss_info->chanspec+1 >= NUM_AP_CHNN) break;
					aCHList[bss_info->chanspec-1]+=WEIGHT(bss_info->RSSI-2);						
					break;
				default:
					wifi_printk(WIFI_ALWAYS,"Channel Num > 13\n");
					break;
				}
			aCHList[bss_info->chanspec]+=WEIGHT(bss_info->RSSI);
			
			bss_info++;
		}	
		
		for(chanNum=1;chanNum<NUM_AP_CHNN+1;chanNum++){
			if(hw_priv->busy_ratio[chanNum]<110){
				MinChanNum[index]=chanNum;
				index++;
			}
		}
		/*There is no busy_ratio<60 channl*/
		if(index==0){
			/*Do hw_priv->busy_ratio[chanNum] sort*/
			//*SetChan=ReturnMin_InSort(&hw_priv->busy_ratio[0],0,chanNum);			
			*SetChan=ReturnMin_InSort_1(&hw_priv->busy_ratio[1],&MinChanNum[0],NUM_AP_CHNN,1);
		}else{
			/*Do MinChanNum[index] sort*/
			//*SetChan=ReturnMin_InSort(&aCHList[MinChanNum[0]],MinChanNum[0],MinChanNum[index]);			
			*SetChan=ReturnMin_InSort_1(&aCHList[MinChanNum[0]],&MinChanNum[0],index,MinChanNum[0]);
		}
	}
	atbm_kfree(AutoScanBuf);
	wifi_printk(WIFI_ALWAYS,"SetChan =%d\n",*SetChan);
	return 0;
}


