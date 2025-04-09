/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifdef ATBM_DHCP
#include <udp.h>
#include <ip.h>
#endif

#include "atbm_hal.h"
#include "atbm_proto.h"
extern struct atbmwifi_common g_hw_prv;
extern 	atbm_int32 globle_rate ;
/* TX policy cache implementation					*/
#define IS_BOOTP_PORT(src_port,des_port) ((((src_port) == 67)&&((des_port) == 68)) || \
										   (((src_port) == 68)&&((des_port) == 67)))
#define ATBM_APOLLO_INVALID_RATE_ID (0xFF)
#define RATE_INDEX_N_6_5M 14
#define RX_SKB__TX_STATUS	(1)
#define RX_SKB__RX_PACKAGE  (2)
#define __hweight8(w)		\
      (	(!!((w) & (1ULL << 0))) +	\
	(!!((w) & (1ULL << 1))) +	\
	(!!((w) & (1ULL << 2))) +	\
	(!!((w) & (1ULL << 3))) +	\
	(!!((w) & (1ULL << 4))) +	\
	(!!((w) & (1ULL << 5))) +	\
	(!!((w) & (1ULL << 6))) +	\
	(!!((w) & (1ULL << 7)))	)

#ifndef LINUX_OS
#define hweight16(w) ((__hweight8(w))+((__hweight8(w))>>8))
#endif

/* This is a version of the rx handler that can be called from hard irq
 * context. Post the skb on the queue and schedule the tasklet */

 int atbmwifi_rx_filter_retry(struct atbmwifi_vif *priv,atbm_uint8 link_id,struct atbm_buff *skb);
 int atbmwifi_ieee80211_rx(struct atbmwifi_vif *priv,struct atbm_buff *skb);
 int __atbmwifi_ieee80211_rx(struct atbmwifi_vif *priv,struct atbm_buff *skb);
extern atbm_uint8 atbm_wmm_status_get(atbm_void);
extern atbm_void hostapd_eap_wsc_init(struct atbmwifi_vif *priv,atbm_uint8 *da);
extern atbm_void atbmwifi_set_rssi(atbm_int8 rssi);
atbm_void atbm_netrx_task(struct atbmwifi_vif *priv)
{
	struct atbm_buff *skb = ATBM_NULL;
	int needfree = 1;
	do{
		skb = atbm_skb_dequeue(&priv->rx_task_skb_list);
		if(skb == ATBM_NULL){
			break;
		}
		#ifdef ATBM_RX_STATUS_USE_QUEUE
		if(skb->Type == RX_SKB__TX_STATUS){
			atbmwifi_ieee80211_tx_status(priv,skb);
			needfree = 1;
		}
		else if(skb->Type == RX_SKB__RX_PACKAGE)
		#endif
		needfree = __atbmwifi_ieee80211_rx(priv,skb);
		if(needfree){
			atbm_dev_kfree_skb(skb);
		}
	}while(1);
}

 int atbm_rx_task_work(atbm_void *work)
{
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)work;
	atbm_netrx_task(priv);
	return 0;
}
/*return 0 ,need  free in this function(atbmwifi_ieee80211_rx_irqsafe), 
else not free before return*/
 int atbmwifi_ieee80211_rx_irqsafe(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_rx_status *hdr = (struct atbmwifi_ieee80211_rx_status *)ATBM_IEEE80211_SKB_RXCB(skb);
	
	if(atbmwifi_rx_filter_retry(priv,hdr->link_id,skb) <0){		
		wifi_printk(WIFI_RX, "[rx_retry] filter\n");
		return -1;
	}

	
	#ifdef ATBM_RX_STATUS_USE_QUEUE
	skb->Type = RX_SKB__RX_PACKAGE;
	#endif
#if (ATBM_RX_TASK_QUEUE==1)
	atbm_skb_queue_tail(&priv->rx_task_skb_list,skb);
	/*Do rx task schedule,change task excute text*/
	
	///TODO atbm_net_RxTask
	atbm_queue_work(priv->hw_priv, priv->rx_task_work);

#else //ATBM_RX_TASK_QUEUE
	if(__atbmwifi_ieee80211_rx(priv,skb)){
		atbm_dev_kfree_skb(skb);
	}
#endif //ATBM_RX_TASK_QUEUE
	return 0;
} 

#ifdef ATBM_RX_STATUS_USE_QUEUE
 int atbmwifi_ieee80211_tx_status_irqsafe(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	skb->Type = RX_SKB__TX_STATUS;
	atbm_skb_queue_tail(&priv->rx_task_skb_list,skb);
	/*Do rx task schedule,change task excute text*/
	
	///TODO atbm_net_RxTask
	//atbm_netrx_task();
	atbm_queue_work(priv->hw_priv, priv->rx_task_work);
}
#endif

#if ATBM_PKG_REORDER

#define TID_IS_SAFE(tid_index,action)	if((tid_index)>ATBM_RX_DATA_QUEUES)	action
#define SEQ_IS_SAFE(seq_index,action) if((seq_index)>=BUFF_STORED_LEN)	action
#define reorder_debug(debug_en,...)  if(debug_en)	iot_printf(ATBM_KERN_DEBUG __VA_ARGS__)
#define REORDER_DEBUG		(0)
#define REORDER_ERROR		(1)
#define THE_RETRY_PKG_INEDX	(0x800)
#define BUFF_INDEX_IS_SAFE(index)	((index)&(BUFF_STORED_LEN-1))
#define DEUG_SPINLOCK		(0)
#if DEUG_SPINLOCK
#define spinlock_debug(type)		wifi_printk(WIFI_ALWAYS,"%s:tid_params_%s\n",__func__,#type)
#else
#define spinlock_debug(type)
#endif
#define tid_params_spin_lock(lock,type) \
	do									\
	{									\
		spinlock_debug(type);			\
		type(lock,0);						\
	}while(0)
#define tid_params_spin_unlock(lock,type)		\
	do										\
	{										\
		spinlock_debug(type);				\
		type(lock);							\
	}while(0)


static struct atbm_ba_tid_params *atbm_get_tid_params(struct atbm_reorder_queue_comm * atbm_reorder,atbm_uint8 tid)
{
	struct atbm_ba_tid_params *tid_params = ATBM_NULL;
	
	if(tid>=ATBM_RX_DATA_QUEUES)
	{
		return ATBM_NULL;
	}

//	atbm_os_mutexLock(&atbm_reorder->reorder_mutex,0);
	tid_params = &atbm_reorder->atbm_rx_tid[tid];
//	atbm_os_mutexUnLock(&atbm_reorder->reorder_mutex);

	return tid_params;
}
static atbm_void atbm_skb_buff_queue(struct atbm_ba_tid_params *tid_params,atbm_uint8 index,struct atbm_buff *skb)
{
	atbm_uint8 start_index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	tid_params->skb_reorder_buff[index] = skb;

	//set time to the oldest time
	while(tid_params->frame_rx_time[index]==0){
		tid_params->frame_rx_time[index] = atbm_GetOsTimeMs();
		//reorder_debug(REORDER_DEBUG,"enqueue index %x\n",index);
		if(start_index == index)
			break;
		index =BUFF_INDEX_IS_SAFE(index-1);
	}

	tid_params->skb_buffed++; 
	ATBM_WARN_ON_FUNC(tid_params->wind_size<tid_params->skb_buffed);
 	//__atbm_skb_queue_tail(&tid_params->header, skb);
}

static atbm_void atbm_skb_buff_dequeue(struct atbmwifi_vif *priv,struct atbm_ba_tid_params *tid_params,atbm_uint8 index,int uplayer)
{
	struct atbm_buff *skb;
	 
	skb = tid_params->skb_reorder_buff[index];
	//reorder_debug(REORDER_DEBUG,"dequeue index %x\n",index);
	if(skb == ATBM_NULL){
		tid_params->frame_rx_time[index] = 0;
	}
	else {
		ATBM_WARN_ON_FUNC(tid_params->skb_buffed ==0);
		ATBM_WARN_ON_FUNC(tid_params->wind_size<tid_params->skb_buffed);
		tid_params->skb_buffed--;
		tid_params->frame_rx_time[index] = 0;
		tid_params->skb_reorder_buff[index] = ATBM_NULL; 
		
		if(uplayer==1){
			if(atbmwifi_ieee80211_rx_irqsafe(priv, skb) != 0)
				atbm_dev_kfree_skb(skb);
		}
		else {			
			atbm_dev_kfree_skb(skb);
		}
	}
	//__atbm_skb_dequeue(&tid_params->header)
}

int atbm_reorder_skb_forcedrop(struct atbmwifi_vif *priv,struct atbm_ba_tid_params *tid_params,int need_free)
{
	atbm_uint8 i = 0;
	atbm_uint8 seq = 0;	

	//reorder_debug(REORDER_DEBUG,"%s:has_buffed(%d)\n",__func__,tid_params->skb_buffed);
	seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	for(i=0;i<need_free;i++)
	{
		atbm_skb_buff_dequeue(priv,tid_params,seq,0);
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	}		

	return 0;
}
int atbm_reorder_skb_forcefree(struct atbmwifi_vif *priv,struct atbm_ba_tid_params *tid_params,int need_free)
{
	atbm_uint8 i = 0;
	atbm_uint8 seq = 0;	

	//reorder_debug(REORDER_DEBUG,"%s:has_buffed(%d)\n",__func__,tid_params->skb_buffed);
	seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	for(i=0;i<need_free;i++)
	{
		atbm_skb_buff_dequeue(priv,tid_params,seq,1);
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	}		

	return 0;
}

int atbm_reorder_skb_uplayer(struct atbmwifi_vif *priv,struct atbm_ba_tid_params *tid_params)
{
	atbm_uint16 seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);

	while(tid_params->skb_buffed)
	{
		if(tid_params->skb_reorder_buff[seq] == ATBM_NULL)
		{
			//reorder_debug(REORDER_DEBUG,"%s,index(%d)\n",__func__,seq);
			break;
		}
		atbm_skb_buff_dequeue(priv,tid_params,seq,1);
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		seq = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	}

	return 0;
}

static atbm_void atbm_reorder_pkg_timeout(atbm_void *data1,atbm_void *data2)
{
	struct atbm_ba_tid_params *tid_params= (struct atbm_ba_tid_params *)data1;
	struct atbmwifi_vif *priv = (struct atbmwifi_vif *)tid_params->reorder_priv;
	//struct atbm_buff_head frames;
	atbm_uint8 i = 0;
	atbm_uint16 index = 0;
	if(!atbm_test_bit(BAR_TID_EN,&tid_params->tid_en))
	{
		return;
	}
	tid_params_spin_lock(&tid_params->skb_reorder_spinlock,atbm_os_mutexLock);
	if(!tid_params->skb_buffed)
		goto exit;
	index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
	reorder_debug(REORDER_DEBUG,"atbm_reorder_pkg_timeout end_time(%d),end_tick(%d),start_index(%d),start_seq(%x)\n",atbm_GetOsTimeMs(),atbm_GetOsTime(),index,tid_params->start_seq);
	while(i<tid_params->wind_size) 
	{
		if(tid_params->frame_rx_time[index] ==0){
			reorder_debug(REORDER_DEBUG,"atbm_reorder_pkg_timeout frame_rx_time(%d),start_index(%d)\n",tid_params->frame_rx_time[index],index);
			break;
		}
		if(!atbm_TimeAfter(tid_params->frame_rx_time[index], (tid_params->timeout/2))){
			reorder_debug(REORDER_DEBUG,"ReOrder:timeout %x>%x sn %x\n",tid_params->frame_rx_time[index],atbm_GetOsTimeMs(),tid_params->start_seq);
			atbm_skb_buff_dequeue(priv,tid_params,index,1);
		}
		else {
			break;
		}
		tid_params->start_seq = (tid_params->start_seq+1)&SEQ_NUM_MASKER;
		index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
		i++;			
	}
	atbm_reorder_skb_uplayer(priv,tid_params);
	index = BUFF_INDEX_IS_SAFE(tid_params->start_seq);
 	if(tid_params->skb_buffed){		
		wifi_printk(WIFI_RX,"atbm: atbm_reorder_pkg_timeout(), start_time(%d),start_index(%d),start_seq(%x)\n",atbm_GetOsTimeMs(),index,tid_params->start_seq);		
		wifi_printk(WIFI_RX,"atbm: atbm_reorder_pkg_timeout(), warning-->timeout(%d)\n",tid_params->timeout);
		atbmwifi_eloop_register_timeout(0,tid_params->timeout,atbm_reorder_pkg_timeout,(atbm_void *)tid_params,ATBM_NULL);
 	}
	else {
	 	atbm_clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
	}
exit:
	tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,atbm_os_mutexUnLock);
}




int atbm_reorder_skb_queue(struct atbmwifi_vif *priv,struct atbm_buff *skb,atbm_uint8 link_id)
{
	atbm_uint16 index = 0;
	int res = 0;
	struct atbm_reorder_queue_comm * atbm_reorder = ATBM_NULL;
	struct atbm_ba_tid_params *tid_params;
	struct atbmwifi_ieee80211_hdr *frame = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	atbm_uint8 frag =  frame->seq_ctrl & ATBM_IEEE80211_SCTL_FRAG;
	atbm_uint8 more = atbmwifi_ieee80211_has_morefrags(frame->frame_control);
	atbm_uint8 *qc = atbmwifi_ieee80211_get_qos_ctl(frame);
	int tid = 0;
	atbm_uint16 frame_seq = 0;
	atbm_uint16 start_seq = 0;
	atbm_uint16 now_frame_id = 0;
	
	if(link_id>ATBMWIFI__MAX_STA_IN_AP_MODE)
	{
		ATBM_WARN_ON_FUNC(1);
		return 0;
	}
	/*
	*fragment frame cant not been buffedn in 
	*ampdu queue;
	*/
	if(frag|more)
	{
		reorder_debug(REORDER_DEBUG,"is a fregment frames\n");
		return 0;
	}
	tid = *qc & /*IEEE80211_QOS_CTL_TID_MASK(*/7;
	/*
	*if it is a fragment frame,we add it to the buff.
	*/
	atbm_reorder = &priv->atbm_reorder_link_id[link_id];
	tid_params =atbm_get_tid_params(atbm_reorder,tid);
	if(tid_params == ATBM_NULL)
	{
		return 0;
	}
	if(!atbm_test_bit(BAR_TID_EN,&tid_params->tid_en))
	{
		return 0;
	}
	if(atbmwifi_ieee80211_is_qos_nullfunc(frame->frame_control))
	{
		return 0;
	}
#if ATBM_DRIVER_PROCESS_BA
	/* reset session timer */
	if (tid_params->session_timeout){
		atbmwifi_eloop_cancel_timeout(sta_rx_agg_session_timer_expired, &tid_params->sta_priv->timer_to_tid[tid], ATBM_NULL);
		atbmwifi_eloop_register_timeout(0, tid_params->session_timeout, sta_rx_agg_session_timer_expired,
			  &tid_params->sta_priv->timer_to_tid[tid], ATBM_NULL);
	}
#endif

	start_seq = tid_params->start_seq;
	frame_seq = (frame->seq_ctrl>>4)&SEQ_NUM_MASKER;
	now_frame_id = BUFF_INDEX_IS_SAFE(frame_seq);
	
	tid_params_spin_lock(&tid_params->skb_reorder_spinlock,atbm_os_mutexLock);
	index = ((frame_seq)-(tid_params->start_seq))&SEQ_NUM_MASKER;
	if (
		//only hope that frame come,so directly queue
		((index == 0)&&(tid_params->skb_buffed == 0))	
	    )
	{
		tid_params->start_seq =(frame_seq+1)&SEQ_NUM_MASKER;
		res = 0;
		goto exit_reorder;
	}
	/*
	*the frame maybe has been lost,so
	*send to the mac80211.
	*/
	if(index>=THE_RETRY_PKG_INEDX)
	{
		res = -1;
		atbm_dev_kfree_skb(skb);
		reorder_debug(REORDER_DEBUG,"<WARNNING>rx a prev frame,seq(%x),start(%x)\n",(frame->seq_ctrl&(ATBM_IEEE80211_SCTL_SEQ))>>4,start_seq);
		goto exit_reorder;
	}
	/*
	*skb buff has been overflowed,maybe same frames has bee buffed a long time,
	*so send some prev frames;
	*/
	if(index >= tid_params->wind_size)
	{
		int need_free = index-tid_params->wind_size+1;
		reorder_debug(REORDER_ERROR,"overflow:need_free(%d),sn(%x),ssn(%x)\n",need_free,frame_seq,tid_params->start_seq);
		if(need_free>=tid_params->wind_size)
		{
			reorder_debug(REORDER_ERROR,"<WARNNING> seq unexception,just to uplayer %d\n",need_free);
			res = 0;
			goto exit_reorder;;
		}
		atbm_reorder_skb_forcefree(priv,tid_params,need_free);	
	}
	else {
		/*
		*retry frames or fragment frames 
		*/
		if(tid_params->skb_reorder_buff[now_frame_id] != ATBM_NULL)
		{
			/*
			*if it is a retry frame,we discare it;
			*/
			res = -1;
			atbm_dev_kfree_skb(skb);
			reorder_debug(REORDER_DEBUG,"%s:ieee80211_has_retry\n",__func__);
			goto exit_reorder;
		}
		else
		{
			//reorder_debug(REORDER_DEBUG,"seq(%x),start(%x),index(%d)\n",frame_seq,start_seq,index);
		}
	}
	atbm_skb_buff_queue(tid_params,now_frame_id,skb);
	
//maybe_send_order:
	/*
	*dequeue the sequential frames after the the "index" frame;
	*/
	atbm_reorder_skb_uplayer(priv,tid_params);
	
//		reorder_debug(REORDER_DEBUG,"header(%d),ampdu reordered,buffed(%d)---2\n",tid_params->index_hread,tid_params->skb_buffed);
	if((!atbm_test_bit(REORDER_TIMER_RUNING,&tid_params->timer_running)||(start_seq!= tid_params->start_seq))&&
		(tid_params->skb_buffed)&&
		(tid_params->timeout))
	{
		unsigned int time;
		index= BUFF_INDEX_IS_SAFE(tid_params->start_seq);

		atbm_set_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);	
		reorder_debug(REORDER_DEBUG,"mod_timer,skb_buffed(%d),start_seq(%x),frame_seq(%x),timeout(%d),start_time(%d),start_tick(%d)\n",tid_params->skb_buffed,tid_params->start_seq,frame_seq,tid_params->timeout,atbm_GetOsTimeMs(),atbm_GetOsTime());

		if(tid_params->frame_rx_time[index] == 0){
			int i =0;
			
			wifi_printk(WIFI_ALWAYS,"now_frame_id =%d=%d %d\n ",index,now_frame_id,start_seq );
			while(i < 64){
				wifi_printk(WIFI_ALWAYS,"tid_params->frame_rx_time[%d]=%d\n ",index,tid_params->frame_rx_time[index] );
				index++;
				index = BUFF_INDEX_IS_SAFE(index);
				i++;
			}
		}
		ATBM_WARN_ON_FUNC(tid_params->frame_rx_time[index] == 0);
		wifi_printk(WIFI_RX,"atbm_reorder(), warning-->timeout(%d)\n",tid_params->timeout);

		time = atbm_GetOsTimeMs() - tid_params->frame_rx_time[index];
		atbmwifi_eloop_cancel_timeout(atbm_reorder_pkg_timeout, (atbm_void *)tid_params, ATBM_NULL);
		atbmwifi_eloop_register_timeout(0,(tid_params->timeout > time) ? (tid_params->timeout - time) : 0,atbm_reorder_pkg_timeout,(atbm_void *)tid_params,ATBM_NULL);
	}
	else if(tid_params->skb_buffed==0)
	{
		atbm_clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
		atbmwifi_eloop_cancel_timeout(atbm_reorder_pkg_timeout, (atbm_void *)tid_params, ATBM_NULL);
	}
	res = -1;
exit_reorder:
	tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,atbm_os_mutexUnLock);
	return res;
}
atbm_void atbm_reorder_func_init(struct atbmwifi_vif *priv)
{
	
	atbm_uint8 i, k;
	struct atbm_reorder_queue_comm * atbm_reorder;
	struct atbm_ba_tid_params *tid_params;
	reorder_debug(REORDER_DEBUG,"%s\n",__func__);
	for(k=0;k<ATBMWIFI__MAX_STA_IN_AP_MODE;k++)
	{
		atbm_reorder = &priv->atbm_reorder_link_id[k];
		for(i=0;i<ATBM_RX_DATA_QUEUES;i++)
		{
			tid_params = &atbm_reorder->atbm_rx_tid[i];
			atbm_clear_bit(BAR_TID_EN,&tid_params->tid_en);
			atbm_os_mutexLockInit(&tid_params->skb_reorder_spinlock);
			//tid_params->overtime_timer.data = (unsigned long)tid_params;
			//tid_params->overtime_timer.function = atbm_reorder_pkg_timeout;
			//init_timer(&tid_params->overtime_timer);
			//atbm_InitTimer(&tid_params->overtime_timer,atbm_reorder_pkg_timeout,(atbm_void*)tid_params);
			//atbm_reorder->atbm_rx_tid[i].skb_reorder_buff = NULL;
			//atbm_reorder->atbm_rx_tid[i].frame_rx_time = NULL;
		}
		//atbm_os_mutexLockInit(&atbm_reorder->reorder_mutex);
		atbm_reorder->link_id=k;
	}
}

atbm_void atbm_reorder_func_deinit(struct atbmwifi_vif *priv)
{
	atbm_uint8 i, k;
	struct atbm_reorder_queue_comm * atbm_reorder;
	struct atbm_ba_tid_params *tid_params;
	reorder_debug(REORDER_DEBUG,"%s\n",__func__);
	for(k=0;k<ATBMWIFI__MAX_STA_IN_AP_MODE;k++)
	{
		atbm_reorder = &priv->atbm_reorder_link_id[k];
		for(i=0;i<ATBM_RX_DATA_QUEUES;i++)
		{
			tid_params = &atbm_reorder->atbm_rx_tid[i];
			atbm_os_DeleteMutex(&tid_params->skb_reorder_spinlock);
		}
	}
}

atbm_void atbm_reorder_tid_buffed_clear(struct atbmwifi_vif *priv,struct atbm_ba_tid_params *tid_params)
{
	//atbm_uint8 header= 0;
	if((priv == ATBM_NULL)||(tid_params==ATBM_NULL))
	{
		return;
	}
	if(!tid_params->tid_en)
	{
		reorder_debug(REORDER_DEBUG,"!tid_params->tid_en\n");
		return;
	}
	
	if(!tid_params->skb_buffed)
		return;

	atbm_reorder_skb_forcefree(priv,tid_params,tid_params->wind_size);
	atbmwifi_eloop_cancel_timeout(atbm_reorder_pkg_timeout, (atbm_void *)tid_params, ATBM_NULL);
	atbm_clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
}

atbm_void atbm_reorder_tid_reset(struct atbmwifi_vif *priv,struct atbm_ba_tid_params *tid_params)
{
	//struct atbm_buff *clear_skb;
	if((priv == ATBM_NULL)||(tid_params==ATBM_NULL))
	{
		return;
	}
	if(!atbm_test_bit(BAR_TID_EN,&tid_params->tid_en))
	{
		reorder_debug(REORDER_DEBUG,"!tid_params->tid_en\n");
		return;
	}
	
	tid_params_spin_lock(&tid_params->skb_reorder_spinlock,atbm_os_mutexLock);
	if(!tid_params->skb_buffed)
		goto exit_tid_reset;
	atbm_reorder_skb_forcedrop(priv,tid_params,tid_params->wind_size);

	tid_params->start_seq = 0;
	atbmwifi_eloop_cancel_timeout(atbm_reorder_pkg_timeout, (atbm_void *)tid_params, ATBM_NULL);
	atbm_clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
exit_tid_reset:
	tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,atbm_os_mutexUnLock);

}
atbm_void atbm_reorder_func_reset(struct atbmwifi_vif *priv,atbm_uint8 link_id)
{
	atbm_uint8 link_start, link_end,tid;
	struct atbm_reorder_queue_comm * atbm_reorder;
	if(link_id == 0xff)
	{
		link_start = 0;
		link_end = ATBMWIFI__MAX_STA_IN_AP_MODE;
	}
	else
	{
		if(link_id>=ATBMWIFI__MAX_STA_IN_AP_MODE)
		{
			return;
		}
		link_start = link_id;
		link_end = link_start+1;
	}
	for(;link_start<link_end;link_start++)
	{	
		atbm_reorder =  &priv->atbm_reorder_link_id[link_start];
//		atbm_os_mutexLock(&atbm_reorder->reorder_mutex);
		for(tid=0;tid<ATBM_RX_DATA_QUEUES;tid++)
		{
			if(!atbm_test_bit(BAR_TID_EN,&atbm_reorder->atbm_rx_tid[tid].tid_en))
			{
				continue;
			}
			atbm_reorder_tid_reset(priv,&atbm_reorder->atbm_rx_tid[tid]);
			tid_params_spin_lock(&atbm_reorder->atbm_rx_tid[tid].skb_reorder_spinlock,atbm_os_mutexLock);
			atbm_clear_bit(BAR_TID_EN,&atbm_reorder->atbm_rx_tid[tid].tid_en);
			atbm_reorder->atbm_rx_tid[tid].ssn = 0;
			atbm_reorder->atbm_rx_tid[tid].wind_size = 0;
			atbm_reorder->atbm_rx_tid[tid].start_seq = 0;
			tid_params_spin_unlock(&atbm_reorder->atbm_rx_tid[tid].skb_reorder_spinlock,atbm_os_mutexUnLock);
		}
//		atbm_os_mutexUnLock(&atbm_reorder->reorder_mutex);
	}
}
atbm_void atbm_updata_ba_tid_params(struct atbmwifi_vif *priv,struct atbm_ba_params *ba_params)
{
	atbm_uint8 tid;
	atbm_uint8 link_id;
	struct atbm_reorder_queue_comm * atbm_reorder =ATBM_NULL;
	struct atbm_ba_tid_params *tid_params = ATBM_NULL;
	atbm_uint8 action = ba_params->action;
	int i=0;

	if(priv == ATBM_NULL)
	{
		reorder_debug(REORDER_DEBUG,"err:%s:priv == ATBM_NULL\n",__func__);
		return;
	}
	if(priv->join_status < ATBMWIFI__JOIN_STATUS_MONITOR)
	{
		reorder_debug(REORDER_DEBUG,"interface type(%d) err\n",priv->join_status);
		return;
	}
	if((priv->join_status == ATBMWIFI__JOIN_STATUS_AP)&&
		(ba_params->link_id >= ATBMWIFI__MAX_STA_IN_AP_MODE))
	{
		reorder_debug(REORDER_DEBUG,"%s link_id(%d) err\n",__FUNCTION__,ba_params->link_id );
		return;
	}
	if (priv->join_status == ATBMWIFI__JOIN_STATUS_STA)
	{
		ba_params->link_id = 1;
	}
	tid = ba_params->tid;
	link_id = ba_params->link_id;
	atbm_reorder = &priv->atbm_reorder_link_id[link_id-1];
	tid_params = &atbm_reorder->atbm_rx_tid[tid];
	switch(action)
	{
		case ATBM_BA__ACTION_RX_ADDBR:
		{
			reorder_debug(REORDER_DEBUG,"WSM_BA_FLAGS__RX_ADDBA_RE\n");
//			atbm_os_mutexLock(&atbm_reorder->reorder_mutex);
			if(atbm_test_bit(BAR_TID_EN,&tid_params->tid_en))
			{
				wifi_printk(WIFI_ALWAYS,"tid_params->tid_en\n");	
				goto exit_action_rx_addba;
			}
			tid_params_spin_lock(&tid_params->skb_reorder_spinlock,atbm_os_mutexLock);
			tid_params->ssn = ba_params->ssn;
			tid_params->wind_size = ba_params->win_size;
			tid_params->timeout = ba_params->timeout;
			if(tid_params->timeout == 0)
				tid_params->timeout = AMPDU_REORDER_TIME_INTERVAL;
			else if(tid_params->timeout >= AMPDU_REORDER_TIME_INTERVAL*2)
				tid_params->timeout = AMPDU_REORDER_TIME_INTERVAL*2;
			else if(tid_params->timeout < AMPDU_REORDER_TIME_INTERVAL/2)
				tid_params->timeout = AMPDU_REORDER_TIME_INTERVAL/2;
			//tid_params->timeout = 100;
			tid_params->start_seq = (ba_params->ssn)&SEQ_NUM_MASKER;
			tid_params->index_tail = tid_params->wind_size-1;
			for(i=0;i<BUFF_STORED_LEN;i++){
				tid_params->skb_reorder_buff[i]=ATBM_NULL;
				tid_params->frame_rx_time[i]=0;
			}
			tid_params->session_timeout = ba_params->timeout;
			//__skb_queue_head_init(&tid_params->header);
			tid_params->reorder_priv = priv;
			tid_params->skb_buffed = 0;
			tid_params->sta_priv = ba_params->sta_priv;
			atbm_set_bit(BAR_TID_EN,&tid_params->tid_en);
			tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,atbm_os_mutexUnLock);
exit_action_rx_addba:
//			atbm_os_mutexUnLock(&atbm_reorder->reorder_mutex);
			break;
		}
		case ATBM_BA__ACTION_RX_DELBA:
		{
			
			if(!atbm_test_bit(BAR_TID_EN,&tid_params->tid_en))
				goto exit_action_rx_delba;
			reorder_debug(REORDER_DEBUG,"WSM_BA_FLAGS__RX_DELBA_RE,link_id(%d)\n",link_id);
			atbm_reorder_tid_reset(priv,tid_params);
//			atbm_os_mutexLock(&atbm_reorder->reorder_mutex);
			tid_params_spin_lock(&tid_params->skb_reorder_spinlock,atbm_os_mutexLock);
			atbm_clear_bit(BAR_TID_EN,&atbm_reorder->atbm_rx_tid[tid].tid_en);
			tid_params->ssn = 0;
			tid_params->wind_size = 0;
			tid_params->start_seq = 0;
			tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,atbm_os_mutexUnLock);
exit_action_rx_delba:
//			atbm_os_mutexUnLock(&atbm_reorder->reorder_mutex);
			break;			
		}
		case ATBM_BA__ACTION_RX_BAR:
		{
			atbm_uint16 pre_start_seq = 0;
			 
//			atbm_os_mutexLock(&atbm_reorder->reorder_mutex);
			if(!atbm_test_bit(BAR_TID_EN,&tid_params->tid_en))
				goto exit_actin_bar;
			tid_params_spin_lock(&tid_params->skb_reorder_spinlock,atbm_os_mutexLock);
			pre_start_seq = tid_params->start_seq;
			//reorder_debug(REORDER_DEBUG,"RX_BAR,BARsn(%x),Ssn(%x)\n",ba_params->ssn,pre_start_seq);
			if(((ba_params->ssn-pre_start_seq)&SEQ_NUM_MASKER)>THE_RETRY_PKG_INEDX)
				goto exit_actin_bar_spin_unlock;
			if((ba_params->ssn-pre_start_seq)==0){
				goto exit_actin_bar_spin_unlock;
			}
			reorder_debug(REORDER_DEBUG,"RX_BAR,BARsn(%x),Ssn(%x)\n",ba_params->ssn,pre_start_seq);
			/*
			*if pre_start_seq+1 <ba_params->ssn maybe we have missed
			*same package so clear the buff queue
			*/
			if((((ba_params->ssn-pre_start_seq)&SEQ_NUM_MASKER) <= tid_params->wind_size)
				&&tid_params->skb_buffed)
			{
				atbm_uint8 need_free = ((ba_params->ssn - pre_start_seq)&SEQ_NUM_MASKER);
				
				reorder_debug(REORDER_DEBUG,"RX_BAR,BARsn(%x),Ssn(%x)\n",ba_params->ssn,pre_start_seq);
				reorder_debug(REORDER_DEBUG,"_RX_BAR case 3,ssn(%x),buffed(%d)need_free %d\n",tid_params->start_seq,tid_params->skb_buffed,need_free);
				
				/*
				*1. a)if need_free >tid_params->wind_size, we dequeue the skb_queue ,then tid_params->skb_buffed
				*	  must be equal to zero;
				*    b)if need_free <tid_params->wind_size,we dequeue the skb_queue,then pre_start_seq must be 
				*	 equal to ba_params->ssn;
				*/
				atbm_reorder_skb_forcefree(priv,tid_params,need_free);
				/*
				*2. if need_free<tid_params->wind_size,then pre_start_seq must equal to ba_params->ssn
				*/
				if(tid_params->skb_buffed&&(tid_params->start_seq != ba_params->ssn))
				{
					reorder_debug(REORDER_DEBUG,"WSM_BA_FLAGS__RX_BAR err\n");
				}
				/*
				*3.from the index_head ,the skb_buff has some buff that is not equal to NULL,we should dequeue it.
				*but must MAKE SURE THAT :pre_start_seq == ba_params->ssn
				*/				
				atbm_reorder_skb_uplayer(priv,tid_params);
				
			}
			else {
				if(tid_params->skb_buffed){
					reorder_debug(REORDER_DEBUG,"_RX_BAR free ALL case 4,(%x)=ssn(%x)-start_seq(%x),buffed(%d)\n",(ba_params->ssn-pre_start_seq),
					ba_params->ssn,tid_params->start_seq,tid_params->skb_buffed);
					atbm_reorder_skb_forcefree(priv,tid_params,tid_params->wind_size);
				}
				tid_params->start_seq = ba_params->ssn;
			}

			if((!atbm_test_bit(REORDER_TIMER_RUNING,&tid_params->timer_running)||(pre_start_seq!= tid_params->start_seq))&&
				(tid_params->skb_buffed)&&
				(tid_params->timeout))
			{		
				atbm_uint16 index= BUFF_INDEX_IS_SAFE(tid_params->start_seq);
				atbm_set_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
				ATBM_WARN_ON_FUNC(tid_params->frame_rx_time[index] == 0);
				wifi_printk(WIFI_RX,"atbm: atbm_updata_ba_tid_params(), warning-->timeout(%d)\n",tid_params->timeout);
				atbmwifi_eloop_register_timeout(0,tid_params->timeout,atbm_reorder_pkg_timeout,(atbm_void *)tid_params,ATBM_NULL);
			}
			else if(tid_params->skb_buffed==0)
			{
				atbm_clear_bit(REORDER_TIMER_RUNING,&tid_params->timer_running);
				atbmwifi_eloop_cancel_timeout(atbm_reorder_pkg_timeout, (atbm_void *)tid_params, ATBM_NULL);
			}
			tid_params->ssn = ba_params->ssn;
exit_actin_bar_spin_unlock:
			tid_params_spin_unlock(&tid_params->skb_reorder_spinlock,atbm_os_mutexUnLock);
exit_actin_bar:
			break;
		}
		default:
		{
			wifi_printk(WIFI_ALWAYS,"%s:action err\n",__func__);
			break;
		}
	}
}
#endif
#if NEW_SUPPORT_PS
static atbm_void atbmwifi_ieee80211_rx_h_sta_process(struct atbmwifi_vif *priv,struct atbmwifi_ieee80211_hdr *hdr) 
{
	int tid;
	struct atbmwifi_sta_priv * sta_priv = atbmwifi_sta_find(priv,hdr->addr2);
	
	if(sta_priv) {		
		/*
		 * Change STA power saving mode only at the end of a frame
		 * exchange sequence.
		 */
		if(BIT(sta_priv->link_id) & priv->sta_asleep_mask) {
			/*
			 * Ignore doze->wake transitions that are
			 * indicated by non-data frames, the standard
			 * is unclear here, but for example going to
			 * PS mode and then scanning would cause a
			 * doze->wake transition for the probe request,
			 * and that is clearly undesirable.
			 */
			if (atbmwifi_ieee80211_is_data(hdr->frame_control) &&
			           !atbmwifi_ieee80211_has_pm(hdr->frame_control)){
				atbm_ps_notify(priv,sta_priv->link_id,0);
			}
		} else {
			if (atbmwifi_ieee80211_has_pm(hdr->frame_control)){
				atbm_ps_notify(priv,sta_priv->link_id,1);
				if(sta_priv->link_id > 0 && sta_priv->link_id <= ATBMWIFI__MAX_STA_IN_AP_MODE){
					atbm_spin_lock_bh(&priv->ps_state_lock);
					for(tid = 0; tid < ATBMWIFI__MAX_TID; tid++){
						if(priv->link_id_db[sta_priv->link_id - 1].buffered[tid] > 0){
							atbmwifi_sta_set_buffered(sta_priv, tid, ATBM_TRUE);
							priv->buffered_set_mask |= BIT(sta_priv->link_id);
							break;
						}
					}
					atbm_spin_unlock_bh(&priv->ps_state_lock);
				}
			}
		}
	}

}
static int atbmwifi_rx_uapsd_and_pspoll(struct atbmwifi_vif *priv,struct atbmwifi_ieee80211_hdr * hdr)
{
	int ret;
	atbm_uint16 fc;
	int link_id;
	int tid;
	//struct atbmwifi_ieee80211_hdr *hdr=ATBM_NULL;
	//hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	fc = hdr->frame_control;
	link_id = atbmwifi_find_hard_link_id(priv,&hdr->addr2[0]);
	tid = *atbmwifi_ieee80211_get_qos_ctl(hdr) & ATBM_IEEE80211_QOS_CTL_TID_MASK;
	wifi_printk(WIFI_PS,"atbmwifi_rx_uapsd_and_pspoll-->link_id=%d fc %x tid =%d\n",link_id,fc,tid);
	/*If no station is sleep,todo continue*/
	if(((priv->sta_asleep_mask &BIT(link_id))==0)){
		return RX_CONTINUE;
	}else{
		wifi_printk(WIFI_PS,"atbmwifi_rx_uapsd_and_pspoll-->link_id=%d,sta_asleep_mask=%x\n",link_id,priv->sta_asleep_mask);
		/* The device handles station powersave, so don't do anything about
		 * uAPSD and PS-Poll frames */	
		if (atbmwifi_ieee80211_is_pspoll(fc)){
			wifi_printk(WIFI_PS,"do ps poll prcoess\n");
			/* if we are in ps , do nothing */
			if(priv->link_id_db[link_id-1].sta_priv.flags & WLAN_STA_PS/*atbm_test_bit(WLAN_STA_PS,&priv->link_id_db[link_id-1].sta_priv.flags)*/){
				return RX_CONTINUE;
			}
			atbmwifi_deliver_poll_response(priv,hdr,link_id);
			/*It's tiger Frame,FrameCtrl|=PM bit 0|qosData|NullData*/
		}else if((atbmwifi_ieee80211_is_qos_nullfunc(fc)||
						atbmwifi_ieee80211_is_data_qos(fc))/*||
						atbmwifi_ieee80211_is_nullfunc(fc)*/){ /*do u-apsd process*/
			/* if we are in a service period, do nothing */
			if(priv->link_id_db[link_id-1].sta_priv.flags & WLAN_STA_SP/*atbm_test_bit(WLAN_STA_SP,&priv->link_id_db[link_id-1].sta_priv.flags)*/){
				return RX_CONTINUE;
			}
			wifi_printk(WIFI_PS,"do uapsd prcoess\n");
			ret=atbmwifi_deliver_uapsd_response(priv,hdr,link_id,tid);
			if(ret<0){
				wifi_printk(WIFI_PS,"Error!!!No queue support uapsd\n");
			}
		}
	}	
	return RX_CONTINUE;
}
#endif

 int __atbmwifi_ieee80211_rx(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_hdr *hdr=ATBM_NULL;
	atbm_uint16 fc;
	atbm_uint8 ret=0;
	atbm_uint16 needfree = 0;
	struct atbmwifi_ieee80211_rx_status *hwhdr = (struct atbmwifi_ieee80211_rx_status *)ATBM_IEEE80211_SKB_RXCB(skb);
	hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	fc = hdr->frame_control;
#if NEW_SUPPORT_PS
	 atbmwifi_ieee80211_rx_h_sta_process(priv,hdr); 
	 atbmwifi_rx_uapsd_and_pspoll(priv,hdr);
#endif
	if(atbmwifi_ieee80211_is_ctl(fc) && !atbmwifi_ieee80211_is_back_req(fc)){
		needfree = 1;
	}
	else if(atbmwifi_ieee80211_is_nullfunc(fc)){
		needfree = 1;
	}else if(atbmwifi_ieee80211_is_qos_nullfunc(fc)){
		needfree = 1;
	}
	else if (atbmwifi_ieee80211_is_data_present(fc)) {
		needfree = 1;
		//amsdu
		atbmwifi_ieee80211_parse_qos(priv,skb);
		ret = atbmwifi_ieee80211_rx_h_amsdu(priv,skb);
		if(ret){
			needfree = 0;
			goto __free_return;
		}
		ret = atbmwifi_ieee80211_data_to_8023(skb, priv->mac_addr,priv->iftype);
		if(ret){
			//needfree = 1;
			wifi_printk(WIFI_RX,"atbmwifi_ieee80211_data_to_8023 err\n");
			goto __free_return;
		}
		//update rssi
		priv->bss.rssi = hwhdr->signal;
		atbmwifi_ieee80211_deliver_skb(priv,skb,&needfree);
	}
	else {
		ATBM_BUG_ON(skb->ref != 1);
		atbmwifi_wpa_event_queue((atbm_void*)priv,(atbm_void*)skb,ATBM_NULL,WPA_EVENT__RX_PKG,ATBM_WPA_EVENT_NOACK);
		needfree = 0;
	}
	
__free_return:
	return needfree;

}

 int atbmwifi_ieee80211_rx(struct atbmwifi_vif *priv,struct atbm_buff *skb)
{
	struct atbmwifi_ieee80211_hdr *hdr = ATBM_NULL;
	atbm_uint16 fc;

	int needfree = 1;

	hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	fc = hdr->frame_control;
	
	ATBM_BUG_ON(skb->ref != 1);

#if ATBM_PKG_REORDER
	if(atbmwifi_ieee80211_is_back_req(hdr->frame_control))
	{
		struct atbm_ba_params ba_params;
		struct atbmwifi_ieee80211_bar * bar_data = (struct atbmwifi_ieee80211_bar *)ATBM_OS_SKB_DATA(skb);
		struct atbmwifi_ieee80211_rx_status *hdr = ATBM_IEEE80211_SKB_RXCB(skb);

		ba_params.tid = atbm_le16_to_cpu(bar_data->control) >> 12;
		ba_params.ssn =  atbm_le16_to_cpu(bar_data->start_seq_num) >> 4;
		ba_params.action= ATBM_BA__ACTION_RX_BAR;
		ba_params.link_id = hdr->link_id;
		wifi_printk(WIFI_ALWAYS,"rx BAR:ssn(%x),tid(%d),link_id(%d)\n",ba_params.ssn,ba_params.tid,ba_params.link_id);
		atbm_updata_ba_tid_params(priv,&ba_params);
		goto exit;
	}
#endif //ATBM_PKG_REORDER

	if(atbmwifi_ieee80211_is_mgmt(fc)) {		
#if CONFIG_IEEE80211W
		if(atbmwifi_ieee80211_drop_unencrypted_mgmt(priv,skb)){
			return needfree;
		}
#endif
		if (atbmwifi_ieee80211_is_action(fc)){
			atbmwifi_rx_actionFrame(priv,skb);
		}else {
			if(atbmwifi_is_ap_mode(priv->iftype)){
				atbmwifi_rx_ap_mgmtframe(priv,skb);
			}
			else{ 
				atbmwifi_rx_sta_mgmtframe(priv,skb);
			}
		}
    }
exit:
	return needfree;
}
 int atbmwifi_ieee80211_tx_status(struct atbmwifi_vif *priv,struct atbm_buff *skb, struct wsm_tx_confirm *arg)
{
	struct atbmwifi_ieee80211_tx_info *tx = (struct atbmwifi_ieee80211_tx_info*)ATBM_IEEE80211_SKB_TXCB(skb);
	struct atbmwifi_cfg *config = atbmwifi_get_config(priv);

	struct atbmwifi_ieee80211_hdr *hdr;
	atbm_uint16 fc;
	atbm_uint16 stype;

	hdr=(struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	fc = hdr->frame_control; 
	if(atbmwifi_ieee80211_is_mgmt(fc)){
		stype = fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_STYPE);
		if(((stype==ATBM_IEEE80211_STYPE_ASSOC_RESP)||
		  	(stype==ATBM_IEEE80211_STYPE_REASSOC_RESP)) &&
		  	atbmwifi_is_ap_mode(priv->iftype)){
			struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *) ATBM_OS_SKB_DATA(skb);
				//TO DO 4way-handshake
#if CONFIG_WPS
			if(!mgmt->u.assoc_resp.status_code && ((priv->pbc) || (priv->pin)))
			{
				hostapd_eap_wsc_init(priv, mgmt->da);
			}else
#endif
			if(!mgmt->u.assoc_resp.status_code && config->wpa)
			{
				atbmwifi_event_uplayer(priv,ATBM_WIFI_ASSOCRSP_TXOK_EVENT,mgmt->da);
			}
		}
#if CONFIG_P2P
		else if(stype == ATBM_IEEE80211_STYPE_ACTION && ((priv->iftype == ATBM_NL80211_IFTYPE_P2P_GO) || (priv->iftype == ATBM_NL80211_IFTYPE_P2P_CLIENT))){
			struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *)ATBM_OS_SKB_DATA(skb);
			atbm_p2p_tx_action_ack(priv, mgmt, arg->status);
		}
#endif
	}else if(tx->b_eapol){
		if(atbmwifi_is_sta_mode(priv->iftype)){
			wpa_supplicant_eapol_notice_ack(priv);
		}
#if CONFIG_WPS
		else{
			struct hostapd_data *hostapd = (struct hostapd_data *)(priv->appdata);
			if(hostapd && hostapd->wpsdata && hostapd->wpsdata->state == RECV_DONE){
				wpa_supplicant_eapol_notice_ack(priv);
			}
		}
#endif
	}
	return 0;
}


extern atbm_void sta_tx_test_end(int status);
 atbm_void atbmwifi_tx_confirm_cb(struct atbmwifi_common *hw_priv,
			  struct wsm_tx_confirm *arg,int if_id,	int link_id)
{
	atbm_uint8 queue_id = atbmwifi_queue_get_queue_id(arg->packetID);
	struct atbmwifi_queue *queue = &hw_priv->tx_queue[queue_id];
	struct atbm_buff *skb;
	const struct atbmwifi_txpriv *txpriv;
	struct atbmwifi_vif *priv;
	/*tx count is the times of number the frame was transmited*/
	priv = _atbmwifi_hwpriv_to_vifpriv(hw_priv, if_id);
	if (atbm_unlikely(!priv))
		return;

	if (arg->status)
		wifi_printk(WIFI_TX, "TX failed: %d.\n",
				arg->status);

	if ((arg->status == WSM_REQUEUE) &&
	    (arg->flags & WSM_TX_STATUS_REQUEUE)) {
		wifi_printk(WIFI_TX, "Requeue for link_id %d (try %d)."
			" STAs asleep: 0x%x\n",
			link_id,
			atbmwifi_queue_get_generation(arg->packetID) + 1,
			priv->sta_asleep_mask);

		atbmwifi_queue_requeue(queue,arg->packetID);
	} 
	else if (!atbmwifi_queue_get_skb(queue, arg->packetID, &skb, &txpriv))
	{
		atbm_skb_pull(skb, txpriv->offset);
#ifndef ATBM_RX_STATUS_USE_QUEUE
		atbmwifi_ieee80211_tx_status(priv, skb, arg);
#endif
		atbmwifi_queue_remove(queue, arg->packetID);
	}
	else {
		ATBM_WARN_ON_FUNC(1);
	}
}

 static int
atbmwifi_tx_h_calc_link_ids(struct atbmwifi_vif *priv,
			  struct atbmwifi_txinfo *t)
{
	if (t->sta_priv && t->sta_priv->link_id){
		t->txpriv.raw_link_id =
				t->txpriv.link_id =
				t->sta_priv->link_id;
	}
	else if (atbmwifi_is_sta_mode(priv->iftype)){
		t->txpriv.raw_link_id =
				t->txpriv.link_id = 0;
	}
	else if (atbm_is_multicast_ether_addr(t->da)) {
		t->txpriv.raw_link_id = 0;
		t->txpriv.link_id = priv->link_id_after_dtim;
	} else {
		if(atbmwifi_is_ap_mode(priv->iftype)&&
		(atbmwifi_ieee80211_is_auth(t->hdr->frame_control)|| 
		atbmwifi_ieee80211_is_action(t->hdr->frame_control)|| 
		atbmwifi_ieee80211_is_probe_resp(t->hdr->frame_control)||
		atbmwifi_ieee80211_is_assoc_resp(t->hdr->frame_control)||
		atbmwifi_ieee80211_is_deauth(t->hdr->frame_control) ||
		atbmwifi_ieee80211_is_reassoc_resp(t->hdr->frame_control))){ 
			t->txpriv.link_id = ATBMWIFI__LINK_ID_AUTH;  
			t->txpriv.raw_link_id = t->txpriv.link_id;
		} 
		else {  
			return -1;
		}
	}
	return 0;
}
/* Default mapping in classifier to work with default
 * queue setup.
 */
const int ieee802_1d_to_ac[8] = {
	ATBM_IEEE80211_AC_BE,
	ATBM_IEEE80211_AC_BK,
	ATBM_IEEE80211_AC_BK,
	ATBM_IEEE80211_AC_BE,
	ATBM_IEEE80211_AC_VI,
	ATBM_IEEE80211_AC_VI,
	ATBM_IEEE80211_AC_VO,
	ATBM_IEEE80211_AC_VO
};

static atbm_void  atbmwifi_notify_buffered_tx(struct atbmwifi_vif *priv, struct atbm_buff *skb,int link_id, atbm_uint32 tid)
{
	struct atbmwifi_sta_priv *sta;
	atbm_uint8 *buffered;
	atbm_uint8 still_buffered = 0;

	if (link_id && (tid < ATBMWIFI__MAX_TID)) {
		atbm_spin_lock_bh(&priv->ps_state_lock);
		buffered = &priv->link_id_db[link_id - 1].buffered[tid];
		sta = &priv->link_id_db[link_id - 1].sta_priv;
		if (!ATBM_WARN_ON(!buffered[tid]))
			still_buffered = --buffered[tid];
		if (!still_buffered && (tid < ATBMWIFI__MAX_TID)) {
			if (sta && (priv->buffered_set_mask & BIT(sta->link_id))){
				atbmwifi_sta_set_buffered(sta, tid, ATBM_FALSE);
				priv->buffered_set_mask &= ~BIT(sta->link_id);
			}
		}
		atbm_spin_unlock_bh(&priv->ps_state_lock);
	}
}

static int __INLINE atbm_get_frame_type(struct atbmwifi_vif *priv, struct atbmwifi_txinfo *t){
	if(!atbm_is_multicast_ether_addr(t->hdr->addr1) &&
		(atbmwifi_ieee80211_is_data_present(t->hdr->frame_control)
#if CONFIG_IEEE80211W
			|| (atbmwifi_ieee80211_is_mgmt(t->hdr->frame_control)		
			&& atbmwifi_ieee80211_is_robust_mgmt_frame(t->hdr)
			&& atbm_get_crypto(priv,2))
#endif
		)){
		return 0;
	}else{
		if(atbmwifi_ieee80211_is_data_present(t->hdr->frame_control))
			return 1;
#if CONFIG_IEEE80211W
		else if(atbmwifi_ieee80211_is_mgmt(t->hdr->frame_control) &&
			atbmwifi_ieee80211_is_robust_mgmt_frame(t->hdr) &&
			atbm_get_crypto(priv,2))
			return 2;
#endif
	}
	return -1;
}

/* IV/ICV injection. */
/* TODO: Quite unoptimal. It's better co modify mac80211
 * to reserve space for IV */
 static int
atbmwifi_tx_h_crypt(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t)
{
	atbm_size_t iv_len=0;
	atbm_size_t icv_len=0;
	atbm_uint8 *icv;
	atbm_uint8 *newhdr;
	//int encrype = 0;
	int entryIndex;

	int frame_type = atbm_get_frame_type(priv, t);
	entryIndex = atbm_get_key(priv,frame_type,t->txpriv.link_id);
	//wifi_printk(WIFI_ALWAYS, "entryIndex:%x frame_type:%d crypto:%x",
	//	entryIndex, frame_type, atbm_get_crypto(priv,!atbm_is_multicast_ether_addr(t->hdr->addr1)));
	if(entryIndex == ATBM_INVALID_KEY){
			return 0;
	}

	switch(atbm_get_crypto(priv, frame_type)){
	case ATBM_WLAN_CIPHER_SUITE_WEP40:
	case ATBM_WLAN_CIPHER_SUITE_WEP104:
		iv_len = WEP_IV_LEN;
		icv_len = WEP_ICV_LEN;
		break;
	case ATBM_WLAN_CIPHER_SUITE_TKIP:
		wifi_printk(WIFI_DBG_MSG, "tkip crypt\n");
		iv_len = TKIP_IV_LEN;
		icv_len = TKIP_ICV_LEN + 8;
		break;
	case ATBM_WLAN_CIPHER_SUITE_CCMP:
		iv_len = CCMP_HDR_LEN;
		icv_len = CCMP_MIC_LEN;
		break;
	case ATBM_WLAN_CIPHER_SUITE_SMS4:
		iv_len = WAPI_IV_LEN;
		icv_len = WAPI_ICV_LEN;
		break;
#if CONFIG_IEEE80211W
	case ATBM_WLAN_CIPHER_SUITE_AES_CMAC:
		if(atbmwifi_is_sta_mode(priv->iftype) || t->sta_priv->ieee_80211w)
		{
			struct atbmwifi_ieee80211_mmie *mmie = (struct atbmwifi_ieee80211_mmie *) atbm_skb_put(t->skb, sizeof(struct atbmwifi_ieee80211_mmie));
			atbm_memset(mmie,0,sizeof(struct atbmwifi_ieee80211_mmie));
			mmie->element_id = ATBM_WLAN_EID_MMIE;
			mmie->length = sizeof(*mmie) - 2;
			mmie->key_id = atbm_cpu_to_le16(priv->connect.key_idx);
			t->hdr->duration_id = 0x00;
		}
		return 0;
#endif //#if CONFIG_IEEE80211W
	default:
		ATBM_WARN_ON_FUNC(1);
		break;
	}	
	t->hdr->frame_control |= atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_PROTECTED);
	if ((atbm_skb_headroom(t->skb) + atbm_skb_tailroom(t->skb) <
			 iv_len + icv_len ) ||
			(atbm_skb_headroom(t->skb) <
			 iv_len )) {
		wifi_printk(WIFI_DBG_ERROR, "<ERROR>ATBM_ENOMEM\n");
		return -ATBM_ENOMEM;
	} 
	/*
	else if (atbm_skb_tailroom(t->skb) < icv_len) {
		atbm_size_t offset = icv_len - atbm_skb_tailroom(t->skb);
		atbm_uint8 *p;
		p = atbm_skb_push(t->skb, offset);
		atbm_memmove(p, &p[offset], OS_SKB_LEN(t->skb) - offset);
		atbm_skb_trim(t->skb, OS_SKB_LEN(t->skb) - offset);
		wifi_printk(WIFI_DBG_ERROR, "<ERROR>ENOMEM2\n");
	}*/

	newhdr = atbm_skb_push(t->skb, iv_len);
	atbm_memmove(newhdr, newhdr + iv_len, t->hdrlen);
	t->hdr = (struct atbmwifi_ieee80211_hdr *) newhdr;
	t->hdrlen += iv_len;
	icv = atbm_skb_put(t->skb, icv_len);
	return 0;
}
 static int
atbmwifi_tx_h_align(struct atbmwifi_vif *priv,struct atbmwifi_txinfo *t,atbm_uint8 *flags)
{
	atbm_size_t offset = (atbm_size_t)ATBM_OS_SKB_DATA(t->skb) & 3;

	if (!offset)
		return 0;

	if (offset & 1) {
		wifi_printk(WIFI_DBG_ERROR,
			"Bug: tx wrong alig: %d\n",offset);
		return -ATBM_EINVAL;
	}

	atbm_skb_push(t->skb, offset);
	t->hdrlen += offset;
	t->txpriv.offset += offset;
	*flags |= WSM_TX_2BYTES_SHIFT;
	t->tx_info->offset=1;
	return 0;
}
/* Add WSM header */
 static struct wsm_tx *
atbmwifi_tx_h_wsm(struct atbmwifi_vif *priv,
		struct atbmwifi_txinfo *t)
{
	struct wsm_tx *wsm;


	wsm = (struct wsm_tx *)atbm_skb_push(t->skb, sizeof(struct wsm_tx));
	t->txpriv.offset += sizeof(struct wsm_tx);
	atbm_memset(wsm, 0, sizeof(*wsm));
	wsm->hdr.len = __atbm_cpu_to_le16(ATBM_OS_SKB_LEN(t->skb));
	wsm->hdr.id = __atbm_cpu_to_le16(WSM_TRANSMIT_REQ_MSG_ID);
	wsm->queueId =
		(t->txpriv.raw_link_id << 2) | wsm_queue_id_to_wsm(t->queue);
	return wsm;
}
 static int
atbmwifi_tx_h_rate_build(struct atbmwifi_vif *priv,
			struct atbmwifi_txinfo *t,
			struct wsm_tx *wsm)
{

	atbm_int8 tx_rate = RATE_INDEX_B_1M;
	struct atbmwifi_sta_priv *sta_priv = t->sta_priv;

	switch(priv->iftype){
		case ATBM_NL80211_IFTYPE_STATION:
		case ATBM_NL80211_IFTYPE_AP:
			if(sta_priv == NULL){
				t->tx_info->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
			}
#if CONFIG_5G_SUPPORT
			if(priv->bss.sta_priv.band == ATBM_IEEE80211_BAND_5GHZ){
				t->tx_info->flags |= ATBM_IEEE80211_TX_CTL_NO_CCK_RATE;
			}
#endif
			if (!(t->tx_info->flags & ATBM_IEEE80211_TX_CTL_USE_MINRATE)){
				if(sta_priv && (sta_priv->rate.ht_cap.ht_supported == ATBM_TRUE)){
					tx_rate = RATE_INDEX_N_65M;
				}else{
					tx_rate = RATE_INDEX_A_54M;
				}
			}
			break;
		case ATBM_NL80211_IFTYPE_P2P_CLIENT:
		case ATBM_NL80211_IFTYPE_P2P_GO:
			tx_rate = RATE_INDEX_A_54M;
			break;
		default:
			tx_rate = ATBM_APOLLO_INVALID_RATE_ID;
			break;

	}
	if(globle_rate)
		tx_rate = globle_rate;		

	if (!(t->tx_info->flags & ATBM_IEEE80211_TX_CTL_USE_MINRATE)){
		t->tx_info->hw_rate_id = tx_rate ;
		if(t->tx_info->hw_rate_id >=RATE_INDEX_N_6_5M){
			/*Ap Mode/P2P GO Mode*/
			if (t->sta_priv){
	  			if(t->sta_priv->rate.ht){
	  				t->tx_info->ht=1;
	  				if(t->sta_priv->rate.channel_type >= ATBM_NL80211_CHAN_HT40MINUS){
	  					t->tx_info->ht_40M=1;
	  				}else{
	  					t->tx_info->ht_40M=0;
	  				}
					if(t->sta_priv->sgi){
						t->tx_info->short_gi=1;
					}else{
						t->tx_info->short_gi=0;
					}
	  		 	}else{
	  				t->tx_info->ht=0;
	  			}
	  		}
		}
		/*If Rate is invalid*/
		if (t->tx_info->hw_rate_id ==ATBM_APOLLO_INVALID_RATE_ID)
		{
			wifi_printk(WIFI_CONNECT,"Drop here 1 !!! %d\n",__LINE__);
			return -1;
		}
		wsm->maxTxRate = t->tx_info->hw_rate_id;
	}else{
		/*The lowest RateId 0=>80211b 1M*/
		if(t->tx_info->flags & ATBM_IEEE80211_TX_CTL_NO_CCK_RATE){
			wsm->maxTxRate = RATE_INDEX_A_6M;
		}else{
			wsm->htTxParameters|=WSM_HT_TX_LONG_PREAMBLE;
			wsm->maxTxRate = RATE_INDEX_B_1M;
		}
	}
	if ( t->tx_info->ht){
		/*unlikely come here*/
		if (t->tx_info->greenfield){
			wsm->htTxParameters |=
				atbm_cpu_to_le32(WSM_HT_TX_GREENFIELD);
		}else{
			wsm->htTxParameters |=
				atbm_cpu_to_le32(WSM_HT_TX_MIXED);
		}
	}
	if (t->tx_info->ht_40M){
		wsm->htTxParameters &= ~WSM_HT_TX_WIDTH_40M;
		wsm->htTxParameters |= atbm_cpu_to_le32(WSM_HT_TX_WIDTH_40M);
	}
	if ( t->tx_info->short_gi) {
		wsm->htTxParameters |= atbm_cpu_to_le32(WSM_HT_TX_SGI);
	}
	/*
	*try to set WSM_HT_AGGR_EN
	*/
#if ATBM_DRIVER_PROCESS_BA
	wifi_printk(WIFI_DBG_MSG, "rate:%d sta_priv:%x qos:%d\n", wsm->maxTxRate, sta_priv, atbmwifi_ieee80211_is_data_qos(t->hdr->frame_control));
	if(priv->hw_priv->driver_setup_ba){
		if(wsm->maxTxRate >= 14){
			/*
			*ba has been installed,so we can try to send ampdu
			*/
			if(sta_priv && atbmwifi_ieee80211_is_data_qos(t->hdr->frame_control)){
				/*
				*try to setup ba
				*/
				if(t->tx_info->flags & ATBM_IEEE80211_TX_CTL_AMPDU){
					wsm->htTxParameters |= WSM_HT_AGGR_EN;
				}else if(ieee80211_start_tx_ba_session(sta_priv,t->txpriv.tid,0)){
					wifi_printk(WIFI_DBG_MSG, "start tx ba session fail[%pM][%d]\n",MAC2STR(sta_priv->mac),t->txpriv.tid);
				}
			}
		}
		if(ATBM_IEEE80211_TX_CTL_ASSIGN_SEQ & t->tx_info->flags){
			wsm->htTxParameters |= atbm_cpu_to_le32(WSM_HT_NEED_SEQ);
		}
	}
#endif

#if ATBM_TX_SKB_NO_TXCONFIRM
	if (t->tx_info->b_eapol || (t->tx_info->b_net == 0)) {		
		wsm->htTxParameters |= atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM);
	}
#endif //ATBM_TX_SKB_NO_TXCONFIRM
	return 0;
}
static ATBM_BOOL
atbmwifi_tx_h_pm_state(struct atbmwifi_vif *priv,
	struct atbmwifi_txinfo *t)
{
	int was_buffered = 1;

	if (t->txpriv.link_id == priv->link_id_after_dtim &&
			!priv->buffered_multicasts) {
		priv->buffered_multicasts = ATBM_TRUE;
		if (priv->sta_asleep_mask){
			atbm_queue_work(priv->hw_priv,priv->set_tim_work);
		}
	}
	if (t->txpriv.raw_link_id && (t->txpriv.tid < ATBMWIFI__MAX_TID)){
		was_buffered = priv->link_id_db[t->txpriv.raw_link_id - 1].buffered[t->txpriv.tid]++;
	}
	return !was_buffered;
}
atbm_void atbmwifi_sta_set_buffered(struct atbmwifi_sta_priv *sta_priv,atbm_uint32 tid,ATBM_BOOL buffered)
{
	if (!atbmwifi_is_ap_mode(sta_priv->priv->iftype))
		return;

	if (ATBM_WARN_ON(tid >= ATBMWIFI__MAX_TID))
		return;
	if((sta_priv->link_id >0) &&(sta_priv->link_id <= ATBMWIFI__MAX_STA_IN_AP_MODE)){
		atbm_set_tim(sta_priv->priv, sta_priv,buffered);
	}
}
#ifdef ATBM_DHCP
int atbmwifi_tx_dhcp_frame(struct atbmwifi_txinfo *t)
{	
	atbm_uint16 ethertype;
	atbm_uint8 *payload;
	payload=ATBM_OS_SKB_DATA(t->skb)+ t->hdrlen;
	ethertype = (payload[12] << 8) | payload[13];
	if (ethertype == atbm_ntohs(ATBM_ETH_P_IP))	{
		struct ip_hdr *iph; 
		struct udp_hdr *udph;
		
		iph = (struct ip_hdr *)(payload+14);
		udph = (struct udp_hdr *)((atbm_uint8*)iph+(iph->_v_hl)*4);
		if(IS_BOOTP_PORT(atbm_ntohs(udph->src),atbm_ntohs(udph->dest))){
			wifi_printk(WIFI_CONNECT,"0---1 dhcp tx \n");
			dump_mem((atbm_uint8*)udph,udph->len);
			return 1;			
		}
	}

	return 0;
}
int atbmwifi_rx_dhcp_frame(atbm_uint8 *payload)
{
	atbm_uint16 ethertype;
	ethertype = (payload[6] << 8) | payload[7];
	if (ethertype == htons(ATBM_ETH_P_IP))	{
		struct ip_hdr *iph; //= ip_hdr(skb);
		struct udp_hdr *udph;
		
		iph = (struct ip_hdr *)(payload+14);
		udph = (struct udp_hdr *)((atbm_uint8*)iph+(iph->_v_hl)*4);
		if(IS_BOOTP_PORT(ntohs(udph->src),ntohs(udph->dest))){
			wifi_printk(WIFI_CONNECT,"0---1 dhcp rx \n");
			dump_mem((atbm_uint8*)udph,udph->len);
			return 1;			
		}
	}

	return 0;
}
#endif
atbm_void atbmwifi_tx(struct atbmwifi_common *hw_priv, struct atbm_buff *skb,struct atbmwifi_vif *priv)
{
	struct wsm_tx *wsm;
	atbm_uint8 flags = 0;
	int ret;
	ATBM_BOOL tid_update = 0;
	struct atbmwifi_txinfo t;
	atbm_memset(&t,0,sizeof(struct atbmwifi_txinfo));
	t.skb = skb;
	t.queue = ieee802_1d_to_ac[skb->priority & ATBM_IEEE80211_QOS_CTL_TAG1D_MASK];
	t.tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	t.hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	t.txpriv.tid = skb->priority;
	t.txpriv.rate_id = ATBMWIFI__INVALID_RATE_ID;
	t.txpriv.offset = 0;

	if (!ATBM_OS_SKB_DATA(skb))
		ATBM_BUG_ON(1);
	
	if (!priv){
		ret = -1;
		goto drop; 
	}	
	if (priv->enabled == 0) {		
		ret = -2;
		goto drop;
	}

	while(priv->ndev->lwip_enable && !priv->ndev->lwip_queue_enable){
		atbm_os_wait_event_timeout(&priv->ndev->tx_enable, 50);
	}

	if((t.tx_info->b_net == 0) || (t.tx_info->b_multi == 1))
		t.tx_info->flags |= ATBM_IEEE80211_TX_CTL_USE_MINRATE;
	
	t.txpriv.if_id = priv->if_id;
	t.hdrlen = atbmwifi_ieee80211_hdrlen(t.hdr->frame_control);
	t.da = atbmwifi_ieee80211_get_DA(t.hdr);
	t.sta_priv =(struct atbmwifi_sta_priv * )atbmwifi_sta_find(priv,t.da);
#ifdef ATBM_DHCP
	atbmwifi_tx_dhcp_frame(&t);
#endif
	ret = atbmwifi_tx_h_calc_link_ids(priv, &t);
	if (ret){
		ret = -6;
		goto drop;
	}
	//wifi_printk(WIFI_ALWAYS, "[TX] TX %d bytes queue[%d] link_id[%d] FC=%x \n",
			// ATBM_OS_SKB_LEN(skb), t.queue, t.txpriv.link_id,t.hdr->frame_control);
	//dump_mem(t.hdr,64);
	//atbmwifi_tx_h_calc_tid(priv, &t);
	ret = atbmwifi_tx_h_crypt(priv, &t);
	if (ret){		
		ret = -7;
		wifi_printk(WIFI_DBG_MSG, "crypt err\n");
		goto drop;
	}
	ret = atbmwifi_tx_h_align(priv, &t, &flags);
	if (ret){		
		ret = -8;
		goto drop;
	}

	wsm = atbmwifi_tx_h_wsm(priv, &t);
	if (!wsm) {
		ret = -ATBM_ENOMEM;
		goto drop;
	}
	wsm->flags |= flags;
	ret = atbmwifi_tx_h_rate_build(priv, &t, wsm);
	if (ret){		
		ret = -11;
		goto drop;
	}

	ret = atbmwifi_queue_put(&hw_priv->tx_queue[t.queue],t.skb, &t.txpriv);	
	if(ret){
		ret = -13;
		goto drop;
	}

	atbm_spin_lock(&priv->ps_state_lock);
	tid_update = atbmwifi_tx_h_pm_state(priv, &t);

	/*If there are no stored packet,here no need updata tim elems*/
	if (tid_update && t.sta_priv && (priv->sta_asleep_mask & BIT(t.txpriv.link_id))){
		atbmwifi_sta_set_buffered(t.sta_priv,t.txpriv.tid, ATBM_TRUE);
		priv->buffered_set_mask |= BIT(t.txpriv.link_id);
	}
	atbm_spin_unlock(&priv->ps_state_lock);
	/*Do tx task schedule,change task excute text*/
	///TODO atbm_bh_schedule_tx or do wakeup Tx thread
	atbm_bh_schedule_tx(hw_priv);
	return;
drop:
	//if (atbm_atomic_add_return(1, &hw_priv->bh_tx) == 1){
	//	wifi_printk(WIFI_DBG_ERROR,"atbm_bh_wakeup tx2\n");
	//	atbm_os_wakeup_event(&hw_priv->bh_wq);
	//}
	//must set link_id = 0,inorder to not call atbmwifi_notify_buffered_tx in atbmwifi_skb_dtor
	t.skb = ATBM_NULL;
	t.tx_info = ATBM_NULL;
	t.hdr = ATBM_NULL;
	t.txpriv.raw_link_id = 0;
	t.txpriv.link_id = 0;
	wifi_printk((WIFI_TX|WIFI_WARN_CODE), "tx drop ret=%d \n",ret);
	//atbm_spin_lock(&priv->ps_state_lock);
	atbmwifi_skb_dtor(hw_priv, skb, &t.txpriv);
	//atbm_spin_unlock(&priv->ps_state_lock);
	return;
}

 atbm_void atbmwifi_skb_dtor(struct atbmwifi_common *hw_priv,
		     struct atbm_buff *skb,
		     const struct atbmwifi_txpriv *txpriv)
{
	struct atbmwifi_vif *priv =_atbmwifi_hwpriv_to_vifpriv(hw_priv, txpriv->if_id);
	if (priv&& atbmwifi_is_ap_mode(priv->iftype) && txpriv->raw_link_id) {
		atbmwifi_notify_buffered_tx(priv,skb,txpriv->raw_link_id,txpriv->tid);
	}
	#ifndef ATBM_RX_STATUS_USE_QUEUE
	atbm_dev_kfree_skb(skb);
	#else
	atbmwifi_ieee80211_tx_status_irqsafe(priv,skb);
	#endif
}
/* ******************************************************************** */
#if NEW_SUPPORT_PS
int atbmwifi_deliver_poll_response(struct atbmwifi_vif *priv,struct atbmwifi_ieee80211_hdr * hdr,int link_id)
{
	//atbm_uint32 drop = 1;
	int i;
	atbm_uint32 pspoll_mask = 0;
	struct atbm_buff *Newskb;
	struct atbmwifi_ieee80211_pspoll *pspoll=(struct atbmwifi_ieee80211_pspoll *) hdr;
	/*Deal with the legency powersave*/
	pspoll_mask = BIT(link_id);

	priv->pspoll_mask |= pspoll_mask;
	if (priv->join_status != ATBMWIFI__JOIN_STATUS_AP)
		goto DONE;
	/*PS-Poll period starts */
	//atbm_set_bit(WLAN_STA_PS,&priv->link_id_db[link_id-1].sta_priv.flags);
	priv->link_id_db[link_id-1].sta_priv.flags |= WLAN_STA_PS;
	/* Do not report pspols if data for given link id is
	 * queued already. */
	for (i = 0; i < ATBM_IEEE80211_NUM_ACS; ++i) {
		if (atbmwifi_queue_get_num_queued(priv,
				&priv->hw_priv->tx_queue[i],
				pspoll_mask)) {
			atbm_bh_schedule_tx(priv->hw_priv);
			break;
		}else{
			/*If there is no data queued,should send NullData*/
			Newskb = (struct atbm_buff *)atbmwifi_ieee80211_NullData(priv,pspoll->ta,pspoll->bssid);
			atbmwifi_tx(priv->hw_priv,Newskb,priv);
			
			break;
		}
	}
	/*PS-Poll period end */
	//atbm_clear_bit(WLAN_STA_PS,&priv->link_id_db[link_id-1].sta_priv.flags);
	priv->link_id_db[link_id-1].sta_priv.flags &= ~WLAN_STA_PS;
DONE:
	return 0;
}
int atbmwifi_deliver_uapsd_response(struct atbmwifi_vif *priv,struct atbmwifi_ieee80211_hdr * hdr,int link_id,int tid)
{
	int ac;	
	atbm_uint32 uapsd_mask=0;
	//atbm_uint8 delivery_enabled = priv->link_id_db[link_id-1].sta_priv.uapsd_support_queues;
	/*If this AC is not trigger-enabled do nothing,if receive a triger frame
	transmit all pending packet,if it need to transmit every tid.Modyfy--->*/
	if (priv->join_status != ATBMWIFI__JOIN_STATUS_AP)
		goto DONE;
	ac = ieee802_1d_to_ac[tid & 7];
	/*Deal with the uapsd powersave*/
	uapsd_mask |= BIT(link_id);

	priv->link_id_uapsd_mask |= uapsd_mask;	

	atbm_bh_schedule_tx(priv->hw_priv);
DONE:
	return 0;
}
#endif
 int atbmwifi_rx_filter_retry(struct atbmwifi_vif *priv,atbm_uint8 link_id,struct atbm_buff *skb)
{
	struct atbmwifi_link_entry *sta_link_id_db = ATBM_NULL;
	struct atbmwifi_ieee80211_hdr *hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	struct atbmwifi_ieee80211_rx_status *status = ATBM_IEEE80211_SKB_RXCB(skb);
	if(atbm_is_multicast_ether_addr(hdr->addr1)){
		return 0;
	}		
	if (!atbmwifi_ieee80211_is_data_present(hdr->frame_control)){
		return 0;
	}
	if(link_id == ATBMWIFI__LINK_ID_UNMAPPED)
		link_id = ATBMWIFI__MAX_STA_IN_AP_MODE+1;
	if(link_id>ATBMWIFI__MAX_STA_IN_AP_MODE+1){
		wifi_printk(WIFI_DBG_ERROR, "[rx_retry] link_id fail(%d)\n",link_id);
		return -1;
	}

	sta_link_id_db = &priv->link_id_db[link_id-1];
	if(sta_link_id_db->status == ATBMWIFI__LINK_OFF){
		wifi_printk(WIFI_DBG_ANY, "[rx_retry]lin_id(%d) status(%d)\n",link_id,sta_link_id_db->status);
		return 0;
	}
	atbmwifi_ieee80211_parse_qos(priv,skb);
	
	if (sta_link_id_db->sta_retry.last_rx_seq[status->seqno_idx] ==
		     hdr->seq_ctrl) {

		if( atbmwifi_ieee80211_has_retry(hdr->frame_control) ){
			wifi_printk(WIFI_DBG_MSG, "[rx_retry](%x)(%d)(%d(%d,(%x,%x)\n",hdr->frame_control,
										priv->if_id,link_id,status->seqno_idx,
										sta_link_id_db->sta_retry.last_rx_seq[status->seqno_idx],hdr->seq_ctrl);
			sta_link_id_db->sta_retry.num_duplicates++;
			return -1;
		}
		else {
			wifi_printk(WIFI_DBG_MSG, "[rx_retry1](%x)(%d)(%d(%d,(%x,%x) skb %x len %d %x\n",hdr->frame_control,
										priv->if_id,link_id,status->seqno_idx,
										sta_link_id_db->sta_retry.last_rx_seq[status->seqno_idx],hdr->seq_ctrl,(unsigned int)skb,skb->dlen,(unsigned int)ATBM_OS_SKB_DATA(skb));
		}
	}
	
	else{
		sta_link_id_db->sta_retry.last_rx_seq[status->seqno_idx] = hdr->seq_ctrl;
	}
	return 0;
}
static atbm_uint8 tmprxhdr_buffer[32];

#if CONFIG_WPA2_REINSTALL_CERTIFICATION
static void atbmwifi_ccmp_hdr2pn(atbm_uint8 *pn, atbm_uint8 *hdr)
{
	pn[0] = hdr[7];
	pn[1] = hdr[6];
	pn[2] = hdr[5];
	pn[3] = hdr[4];
	pn[4] = hdr[1];
	pn[5] = hdr[0];
}
#endif

int atbmwifi_ccmp_replaycnt(struct atbmwifi_vif *priv,struct atbmwifi_ieee80211_mgmt *mgmt)
{
#if CONFIG_WPA2_REINSTALL_CERTIFICATION
	 atbm_uint8 pn[CCMP_PN_LEN];
	 static const atbm_uint8 zero_pn[6] = {0};
	 int hdrlen = atbmwifi_ieee80211_hdrlen(mgmt->frame_control);
	 atbm_uint8 *qc = atbmwifi_ieee80211_get_qos_ctl((struct atbmwifi_ieee80211_hdr *)mgmt);
		 /* frame has qos control */
	 atbm_uint8  tid = *qc & ATBM_IEEE80211_QOS_CTL_TID_MASK;
	 atbm_uint8 *pOldPN;
	 atbm_uint8 *b_pn_init;
	 if(!atbmwifi_is_sta_mode(priv->iftype)){
		 return 0;	 
	 }


	 atbmwifi_ccmp_hdr2pn(pn, (atbm_uint8 *)mgmt + hdrlen);
	 
	 if(atbm_is_multicast_ether_addr(mgmt->da)==0){	 
	 	if(atbmwifi_ieee80211_is_data_qos(mgmt->frame_control)){			
			pOldPN = priv->connect.ptk_pn[tid];
			b_pn_init= &priv->connect.ptk_pn_init[tid] ;
	 	}
		else {
			pOldPN = priv->connect.ptk_noqos_pn;
			b_pn_init = &priv->connect.ptk_noqos_pn_init;

		}
	 }
	 else {
		pOldPN = priv->connect.gtk_pn;
		b_pn_init = &priv->connect.gtk_pn_init;
	 }

	 //if first PN just update
	 if(*b_pn_init == 0) {
	 	if(memcmp(pn, pOldPN, CCMP_PN_LEN) <= 0) {
	 	 	if(!((memcmp(pn, zero_pn, CCMP_PN_LEN)==0 )&&(memcmp(pOldPN, zero_pn, CCMP_PN_LEN)==0 ))){
			 	 wifi_printk(WIFI_DBG_ERROR, "[RX]ccmp_replaycnt drop %x:%x:%x:%x:%x:%x > %x:%x:%x:%x:%x:%x \n",pOldPN[0]
									 	,pOldPN[1]
									 	,pOldPN[2]
									 	,pOldPN[3]
									 	,pOldPN[4]
									 	,pOldPN[5]
									 	,pn[0]
									 	,pn[1]
									 	,pn[2]
									 	,pn[3]
									 	,pn[4]
									 	,pn[5]);
		 	 }
			 return RX_DROP_UNUSABLE;
		 }
		 else {
		 	int i=0;
			for(i=0;i<CCMP_PN_LEN;i++){
				if(pn[i]>pOldPN[i]){
					//PN      = xx,xx,xx,xx,0,0
					if(i>=CCMP_PN_LEN-2){
						break;
					}
					else {
						//pOldPN[i];
						//PN      = 1,0,0,0,0,0
						//pOldPN= 0,0xff,0xff,0xff,0,0
						if((pn[i]!=(pOldPN[i]+1))
							||(pOldPN[i+1]!=0xff)
							||(pn[i+1]!=0)){
							wifi_printk(WIFI_DBG_ERROR, "[RX]ccmp_replaycnt not updata\n");
						    wifi_printk(WIFI_DBG_ERROR, "[RX]ccmp_replaycnt drop %x:%x:%x:%x:%x:%x > %x:%x:%x:%x:%x:%x \n",pOldPN[0]
												 	,pOldPN[1]
												 	,pOldPN[2]
												 	,pOldPN[3]
												 	,pOldPN[4]
												 	,pOldPN[5]
												 	,pn[0]
												 	,pn[1]
												 	,pn[2]
												 	,pn[3]
												 	,pn[4]
												 	,pn[5]);
							return 0;
						}
					}
				}
			}
	 	}
	 }
	 else {
		*b_pn_init = 0;
	 }
	 atbm_memcpy(pOldPN,pn, CCMP_PN_LEN);
#endif //#ifndef CONFIG_WPA2_REINSTALL_CERTIFICATION
	 return 0;
}

static int atbmwifi_class3_err(struct atbmwifi_vif *priv, struct atbmwifi_ieee80211_hdr *frame, struct wsm_rx *arg, int link_id)
{
	struct atbmwifi_sta_priv *sta_priv;

	if(!atbmwifi_is_ap_mode(priv->iftype))
		return 0;

	sta_priv = atbmwifi_sta_find_form_hard_linkid(priv, (atbm_uint8) link_id);
	if(sta_priv == ATBM_NULL || atbm_memcmp(sta_priv->mac, frame->addr2, ATBM_ETH_ALEN)){
		/*
		*must be outside lock due to cfg80211
		*be that's not a problem
		*/
		atbmwifi_ieee80211_send_deauth_disassoc(priv, frame->addr2, priv->bssid, ATBM_IEEE80211_STYPE_DEAUTH,
			ATBM_WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY,
			ATBM_NULL, ATBM_TRUE);
		wifi_printk(WIFI_ALWAYS, "atbmwifi_class3_err\n");
		return 1;
	}
	return 0;
}

#define STR2MAC1		"%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR1(a)  a[0],a[1],a[2],a[3],a[4],a[5]
extern void ieee80211_special_filter_rx_package_handle(struct atbmwifi_vif *priv,struct atbm_buff *skb);
 atbm_void atbmwifi_rx_cb(struct atbmwifi_vif *priv,struct wsm_rx *arg,struct atbm_buff **skb_p,int link_id)
{
	struct atbm_buff *skb = *skb_p;
	struct atbmwifi_ieee80211_rx_status *hdr = ATBM_IEEE80211_SKB_RXCB(skb);
	struct atbmwifi_ieee80211_mgmt *mgmt = (struct atbmwifi_ieee80211_mgmt *)ATBM_OS_SKB_DATA(skb);
	struct atbmwifi_ieee80211_hdr *frame_hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
	char *da,*sa,*bssid;

	hdr->flag = 0;

	if (atbm_unlikely(arg->status)) {
#if CONFIG_IEEE80211W
		if(priv->iftype == ATBM_NL80211_IFTYPE_STATION && priv->connect.crypto_igtkgroup){
			if(atbmwifi_ieee80211_is_robust_mgmt_frame(skb)){
				wifi_printk(WIFI_DBG_ERROR, "[RX]:drop PMF\n");
				goto drop;
			}
		}
#endif
		if(priv->iftype == ATBM_NL80211_IFTYPE_AP || priv->iftype == ATBM_NL80211_IFTYPE_P2P_GO){
			bssid = frame_hdr->addr1;
			sa = frame_hdr->addr2;
			da = frame_hdr->addr3;
		}else{
			da = frame_hdr->addr1;
			bssid = frame_hdr->addr2;
			sa = frame_hdr->addr3;
			
		}

		if(memcmp(da,priv->mac_addr,6) != 0){
			goto drop;
		}
	

		atbmwifi_class3_err(priv, (struct atbmwifi_ieee80211_hdr *)mgmt, arg, link_id);
		if (arg->status == WSM_STATUS_MICFAILURE) {
				
           	 wifi_printk(WIFI_DBG_ERROR,"[RX] MIC failure. ENCRYPTION [%d],da:"STR2MAC1"],sa:["STR2MAC1"]bssid:["STR2MAC1"]\n",
                    WSM_RX_STATUS_ENCRYPTION(arg->flags),MAC2STR1(da),MAC2STR1(sa),MAC2STR1(bssid));
            hdr->flag |= ATBM_RX_FLAG_MMIC_ERROR;
        } else if (arg->status == WSM_STATUS_NO_KEY_FOUND) {
            	wifi_printk(WIFI_DBG_ERROR, "[RX] No key found.ENCRYPTION [%d],da["STR2MAC1"],sa["STR2MAC1"],bssid["STR2MAC1"]\n",
                            WSM_RX_STATUS_ENCRYPTION(arg->flags), MAC2STR1(da),MAC2STR1(sa),MAC2STR1(bssid));
                    goto drop;
                
        } else {
			
            wifi_printk(WIFI_DBG_ERROR,"[RX] Receive failure: %d.ENCRYPTION [%d],da["STR2MAC1"],sa["STR2MAC1"],bssid["STR2MAC1"],fc(%x)\n",
                    arg->status,WSM_RX_STATUS_ENCRYPTION(arg->flags),
                    MAC2STR1(da),MAC2STR1(sa),MAC2STR1(bssid),mgmt->frame_control);

			goto drop;
		}
	}else{
		if(link_id > ATBMWIFI__MAX_STA_IN_AP_MODE && atbmwifi_ieee80211_is_data(mgmt->frame_control)){
			if(atbmwifi_class3_err(priv, (struct atbmwifi_ieee80211_hdr *)mgmt, arg, link_id))
				goto drop;
		}
	}

	if ( ATBM_OS_SKB_LEN(skb) < sizeof(struct atbmwifi_ieee80211_pspoll)) {
		wifi_printk(WIFI_DBG_ERROR, "<ERROR> [RX] len %d\n",ATBM_OS_SKB_LEN(skb));
		goto drop;
	}
	hdr->link_id = link_id;
	if(atbmwifi_is_sta_mode(priv->iftype)){
		link_id = 1;	
	}

	hdr->band = (arg->channelNumber > 14) ?
			ATBM_NL80211_BAND_5GHZ : ATBM_IEEE80211_BAND_2GHZ;
	hdr->freq = atbmwifi_ieee80211_channel_to_frequency(
			arg->channelNumber,
			hdr->band);

	if (arg->rxedRate >= 14) {
		hdr->flag |= ATBM_RX_FLAG_HT;
		hdr->rate_idx = arg->rxedRate - 14;
	} else if (arg->rxedRate >= 4) {
			hdr->rate_idx = arg->rxedRate - 2;
	} else {
		hdr->rate_idx = arg->rxedRate;
	}

	hdr->signal = (atbm_int8)arg->rcpiRssi;


	//record rssi value
	atbmwifi_set_rssi(hdr->signal);

	hdr->antenna = 0;
    /*move the addtion header of the encrypt frame */
	if (WSM_RX_STATUS_ENCRYPTION(arg->flags)) {
		atbm_size_t iv_len = 0, icv_len = 0;
		atbm_size_t hdrlen = 0;
		hdrlen = atbmwifi_ieee80211_hdrlen(mgmt->frame_control);
		//wifi_printk(WIFI_DBG_MSG, "ENCRYPTION\n");
		hdr->flag |= ATBM_RX_FLAG_DECRYPTED;

		/* Oops... There is no fast way to ask mac80211 about
		 * IV/ICV lengths. Even defineas are not exposed.*/
		switch (WSM_RX_STATUS_ENCRYPTION(arg->flags)) {
		case WSM_RX_STATUS_WEP:
			iv_len = 4 /* WEP_IV_LEN */;
			icv_len = 4 /* WEP_ICV_LEN */;
			break;
		case WSM_RX_STATUS_TKIP:
			iv_len = 8 /* TKIP_IV_LEN */;
			icv_len = 4 /* TKIP_ICV_LEN */
				+ 8 /*MICHAEL_MIC_LEN*/;
			hdr->flag |= ATBM_RX_FLAG_MMIC_STRIPPED;
			break;
		case WSM_RX_STATUS_AES:
			iv_len = 8 /* CCMP_HDR_LEN */;
			icv_len = 8 /* CCMP_MIC_LEN */;
			if(atbmwifi_ccmp_replaycnt(priv,mgmt))
				goto drop;
			break;
		case WSM_RX_STATUS_WAPI:
			iv_len = 18 /* WAPI_HDR_LEN */;
			icv_len = 16 /* WAPI_MIC_LEN */;
			hdr->flag |= ATBM_RX_FLAG_IV_STRIPPED;
			break;
		default:
			ATBM_WARN_ON_FUNC("Unknown encryption type");
			goto drop;
		}

		/* Firmware strips ICV in case of MIC failure. */
		if (arg->status == WSM_STATUS_MICFAILURE) {
			icv_len = 0;
			hdr->flag |= ATBM_RX_FLAG_IV_STRIPPED;
		}

		if(ATBM_OS_SKB_LEN(skb) < hdrlen + iv_len + icv_len) {
			wifi_printk(WIFI_DBG_ERROR, "rxlen len lesser than crypto hdr.\n");
			goto drop;
		}

		/* Protocols not defined in mac80211 should be
		stripped/crypted in driver/firmware */
		atbm_skb_trim(skb,  ATBM_OS_SKB_LEN(skb) - icv_len);
		ATBM_BUG_ON(hdrlen > 32);
		atbm_memcpy(tmprxhdr_buffer, ATBM_OS_SKB_DATA(skb), hdrlen);
		atbm_memcpy(ATBM_OS_SKB_DATA(skb) + iv_len,tmprxhdr_buffer, hdrlen);
		atbm_skb_pull(skb, iv_len);
	}
#if ATBM_PKG_REORDER
	if(atbmwifi_ieee80211_is_data_qos(mgmt->frame_control))
	{
		if((link_id == 0)||(link_id-1>=WLAN_LINK_ID_MAX))
		{
			goto direct_queue;
		}
		if(atbm_reorder_skb_queue(priv,skb,link_id-1) == 0)
		{
			goto direct_queue;
		}
		else
		{
			*skb_p = ATBM_NULL;
			return;
		}
	}
direct_queue:
#endif //ATBM_PKG_REORDER
	if(atbmwifi_ieee80211_rx_irqsafe(priv,skb)==0){
		*skb_p = ATBM_NULL;
	}
	return;
drop:
	return;
}

 atbm_void atbmwifi_tx_start(struct atbm_buff *skb,struct atbmwifi_vif *priv)
{
	struct atbmwifi_common * hw_priv = priv->hw_priv;
	struct atbmwifi_ieee80211_tx_info * tx_info;
	struct atbmwifi_sta_priv *sta_priv = ATBM_NULL;
	int res;
	ATBM_BOOL qos = ATBM_FALSE;

	if(atbmwifi_is_sta_mode(priv->iftype)){
		if(!priv->assoc_ok){
			atbm_dev_kfree_skb(skb);			
			wifi_printk(WIFI_CONNECT,"atbmwifi_tx_start not connect_ok \n");
			return;
		}
	}

	tx_info = ATBM_IEEE80211_SKB_TXCB(skb);
	if(!atbm_is_multicast_ether_addr(ATBM_OS_SKB_DATA(skb))){
		sta_priv = atbmwifi_sta_find(priv,ATBM_OS_SKB_DATA(skb));
		if(!sta_priv){
			wifi_printk(WIFI_CONNECT,"atbmwifi_sta_find drop tx,%x\n",(unsigned int)skb);
			atbm_dev_kfree_skb(skb);	
			return;
		}
		qos = sta_priv->wmm_used;
	}else{
		tx_info->b_multi = 1;
	}
	tx_info->b_net = 1;

	res = atbmwifi_ieee80211_data_from_8023(skb,priv->mac_addr,
			    priv->iftype,priv->bssid, qos,priv->connect.encrype);
	if(res<0)
	{
		wifi_printk(WIFI_CONNECT,"8023=>80211 err(%d) skblen %d,%x\n",res,ATBM_OS_SKB_LEN(skb),(unsigned int)skb);
		atbm_dev_kfree_skb(skb);
		return;
	}


#if ATBM_DRIVER_PROCESS_BA
	if(hw_priv->driver_setup_ba){
		int tid;
		struct atbmwifi_ieee80211_hdr * hdr = (struct atbmwifi_ieee80211_hdr *)ATBM_OS_SKB_DATA(skb);
		if (sta_priv && qos){
			struct tid_ampdu_tx *tid_tx;
			atbm_uint8 *qc;
			
			qc = atbmwifi_ieee80211_get_qos_ctl(hdr);
			tid = *qc & ATBM_IEEE80211_QOS_CTL_TID_MASK;
		
			tid_tx = sta_priv->ampdu_mlme.tid_tx[tid];
			if (tid_tx) {
				ATBM_BOOL queued;
		
				queued = ieee80211_tx_prep_agg(sta_priv, skb, tx_info,
								   tid_tx, tid);
				//if(queued)
					//wifi_printk(WIFI_ALWAYS, "queued:%d\n", queued);
				if (atbm_unlikely(queued))
					return;
				if(!atbm_list_empty(&tid_tx->pending)){
					atbmwifi_tx_aggr_queued(priv, sta_priv, tid);
				}
			}
		}
		ieee80211_tx_h_sequence(sta_priv, skb, tid);
	}
#endif
	atbmwifi_tx(hw_priv,skb,priv);
	
	return;
}

