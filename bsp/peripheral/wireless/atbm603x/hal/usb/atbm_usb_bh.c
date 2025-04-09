/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
extern int wsm_recovery(struct atbmwifi_common *hw_priv);
int atbm_rx_bh_cb(struct atbmwifi_common *hw_priv,struct atbm_buff *skb, ATBM_BOOL checkseq)
{
	struct wsm_hdr *wsm;
	atbm_uint32 wsm_len;
	int wsm_id;
	atbm_uint8 wsm_seq;
	//wifi_printk(WIFI_DBG_ERROR, "%s ++\n",__func__);

	wsm = (struct wsm_hdr *)ATBM_OS_SKB_DATA(skb);

	wsm_len = __atbm_le32_to_cpu(wsm->len);

	wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;

	ATBM_BUG_ON(wsm_len > 4096);
	atbm_skb_trim(skb,0);
	atbm_skb_put(skb,wsm_len);

	if (atbm_unlikely(wsm_id == 0x0800)) {
		wsm_handle_exception(hw_priv,
			 &ATBM_OS_SKB_DATA(skb)[sizeof(*wsm)],
			wsm_len - sizeof(*wsm));
		if(wsm_recovery(hw_priv)== RECOVERY_ERR){
			ATBM_BUG_ON(1);
		}
		goto __free;
	}
	if(checkseq){
		wsm_seq = (__atbm_le32_to_cpu(wsm->id) >> 13) & 7;
		if (ATBM_WARN_ON(wsm_seq != hw_priv->wsm_rx_seq)) {
			wifi_printk(WIFI_DBG_ERROR,"rx wsm_seq error %d %d \n",wsm_seq,hw_priv->wsm_rx_seq);
			if(wsm_recovery(hw_priv)== RECOVERY_ERR){
				ATBM_BUG_ON(1);
			}
			goto __free;
		}
		hw_priv->wsm_rx_seq = (wsm_seq + 1) & 7;
	}

	if (wsm_id & 0x0400) {
		int rc = wsm_release_tx_buffer(hw_priv, 1);
		if (ATBM_WARN_ON(rc < 0)){
			wifi_printk(WIFI_ALWAYS,"%s %d \n",__FUNCTION__,__LINE__);
			ATBM_BUG_ON(1);
			goto __free;
		}
	}

	/* atbm_wsm_rx takes care on SKB livetime */
	if (ATBM_WARN_ON(wsm_handle_rx(hw_priv, wsm_id, wsm,&skb))){
		wifi_printk(WIFI_ALWAYS,"%s %d \n",__FUNCTION__,__LINE__);
		if(wsm_recovery(hw_priv)== RECOVERY_ERR){
			ATBM_BUG_ON(1);
		}
		goto __free;
	}

__free:
	//wifi_printk(WIFI_DBG_ERROR, "%s %d\n",__func__,__LINE__);
	if(skb != ATBM_NULL){
		atbm_dev_kfree_skb(skb);
	}

	return 0;
}
/*Tx Task Func*/
atbm_void atbm_tx_task(struct atbmwifi_common *hw_priv)
{
	int status =0;
	do {
		//if(hw_priv->bh_term)|| (hw_priv->bh_error))
		//{
		//	wifi_printk(WIFI_IF,"atbm_tx_task term(%d),err(%d)\n",atbm_atomic_read(&hw_priv->bh_term),hw_priv->bh_error);
		//	return;
		//}
		/*atbm transmit packet to device*/
		hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
		status = hw_priv->sbus_ops->sbus_memcpy_toio(hw_priv->sbus_priv,0x1,ATBM_NULL,TX_BUFFER_SIZE);
		hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	}while(status > 0);
}
/*Rx Task Func*/
atbm_void atbm_rx_task(struct atbmwifi_common *hw_priv)
{
	struct atbm_buff *skb;
#if HI_RX_MUTIL_FRAME
	struct wsm_hdr *wsm;
	atbm_uint32 wsm_len, wsm_id, data_len;
	struct atbm_buff *skb_copy;
	ATBM_BOOL checkseq;
	ATBM_BOOL multirx;
#endif

#define RX_ALLOC_BUFF_OFFLOAD (  (36+16)/*RX_DESC_OVERHEAD*/+4/*FCS_LEN*/ -16 /*WSM_HI_RX_IND*/)
	while ((skb = atbm_skb_dequeue(&hw_priv->rx_frame_queue)) != ATBM_NULL) {
		if(hw_priv->bh_term|| hw_priv->bh_error)
			break;
#if HI_RX_MUTIL_FRAME
		multirx = ATBM_FALSE;
		checkseq = ATBM_TRUE;
		wsm = (struct wsm_hdr *)ATBM_OS_SKB_DATA(skb);
		wsm_len = __atbm_le32_to_cpu(wsm->len);
		wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
		//wifi_printk(WIFI_DBG_ERROR, "%s rxdata %x\n",__func__,wsm_id);

		if(wsm_id == WSM_MULTI_RECEIVE_INDICATION_ID){
			multirx = ATBM_TRUE; 
		}else if(wsm_id == WSM_SINGLE_CHANNEL_MULTI_RECEIVE_INDICATION_ID){
			atbm_uint8 wsm_seq;

			multirx = ATBM_TRUE;
			wsm_seq = (__atbm_le32_to_cpu(wsm->id) >> 13) & 7;
			if (ATBM_WARN_ON(wsm_seq != hw_priv->wsm_rx_seq)) {
				wifi_printk(WIFI_DBG_ERROR,"rx wsm_seq error %d %d \n",wsm_seq,hw_priv->wsm_rx_seq);
				if(wsm_recovery(hw_priv)== RECOVERY_ERR){
					ATBM_BUG_ON(1);
				}
				goto multirx_end;
			}
			hw_priv->wsm_rx_seq = (wsm_seq + 1) & 7;
			checkseq = ATBM_FALSE;
		}
		if(multirx){
			struct wsm_multi_rx *  multi_rx = (struct wsm_multi_rx *)ATBM_OS_SKB_DATA(skb);			
			int RxFrameNum = multi_rx->RxFrameNum;
			data_len = wsm_len ;
			data_len -= sizeof(struct wsm_multi_rx);
			wsm = (struct wsm_hdr *)(multi_rx+1);
			wsm_len = __atbm_le32_to_cpu(wsm->len);
			wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
			do {
						
				if(data_len < wsm_len){
					wifi_printk(WIFI_DBG_ERROR,"skb->len %x,wsm_len %x\n",ATBM_OS_SKB_LEN(skb),wsm_len);
					break;
				}
				ATBM_BUG_ON((wsm_id  & ~WSM_TX_LINK_ID(WSM_TX_LINK_ID_MAX)) !=  WSM_RECEIVE_INDICATION_ID);
				skb_copy = atbm_dev_alloc_skb(wsm_len + 16);
				/* In AP mode RXed SKB can be looped back as a broadcast.
				 * Here we reserve enough space for headers. */
				atbm_skb_reserve(skb_copy,  (8 - (((unsigned long)ATBM_OS_SKB_DATA(skb_copy))&7))/*ATBM_ALIGN 8*/);
				
				atbm_memmove(ATBM_OS_SKB_DATA(skb_copy), wsm, wsm_len);
				atbm_skb_put(skb_copy,wsm_len);
				atbm_rx_bh_cb(hw_priv,skb_copy,checkseq);
				data_len -= ATBM_ALIGN(wsm_len + RX_ALLOC_BUFF_OFFLOAD,4);
				RxFrameNum--;

				wsm = (struct wsm_hdr *)((atbm_uint8 *)wsm +ATBM_ALIGN(( wsm_len + RX_ALLOC_BUFF_OFFLOAD),4));
				wsm_len = __atbm_le32_to_cpu(wsm->len);
				wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
				
			}while((RxFrameNum>0) && (data_len > 32));
			ATBM_BUG_ON(RxFrameNum != 0);
multirx_end:
#if RX_QUEUE_IMMD
			if(skb->Type == RX_SKB__RX_QUEUE_PACKAGE){
				/*atbm transmit packet to device*/
				hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
				hw_priv->sbus_priv->rx_queue_qnum--;
				hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);	
				atbm_dev_kfree_skb(skb);
			}else
#endif
			{
				//wifi_printk(WIFI_ALWAYS, "1 zzzz\n");
				/*atbm transmit packet to device*/
				hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
				hw_priv->sbus_ops->sbus_read_async(hw_priv->sbus_priv,0x2,skb,RX_BUFFER_SIZE);
				hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
			}
		}
		else
#endif
		{
			atbm_rx_bh_cb(hw_priv,skb,checkseq);
			/*atbm transmit packet to device*/
			hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
#if	RX_QUEUE_IMMD
			if(skb->Type == RX_SKB__RX_QUEUE_PACKAGE){
				hw_priv->sbus_priv->rx_queue_qnum--;
			}else
#endif
			{
				//wifi_printk(WIFI_ALWAYS, "2 zzzz\n");
				hw_priv->sbus_ops->sbus_read_async(hw_priv->sbus_priv,0x2,ATBM_NULL,RX_BUFFER_SIZE);
			}
			hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
		}
	}
}
atbm_void atbm_usb_bh(atbm_void *arg)
{
	struct atbmwifi_common *hw_priv = (struct atbmwifi_common *)arg;
	int rx=0, tx=0,urb_compl=0;
	//int pending_tx = 0;
	//ATBM_BOOL powersave_enabled;
	//int loop=0;
	//int prink_test =0;
	#define __ALL_HW_BUFS_USED (hw_priv->hw_bufs_used)
	while (1){
		do {	 
			
			rx = atbm_atomic_xchg(&hw_priv->bh_rx, 0);
			tx = atbm_atomic_xchg(&hw_priv->bh_tx, 0);
			urb_compl = atbm_atomic_xchg(&hw_priv->urb_comp, 0);
			//suspend = pending_tx ?0 : atbm_atomic_read(&hw_priv->bh_suspend);
			if(tx || rx|| urb_compl){
				//atbm_os_try_to_wait_event_timeout(&hw_priv->bh_wq,1);
				break;		
			}
			atbm_os_wait_event_timeout(&hw_priv->bh_wq,100*HZ);	
			
			//wifi_printk(WIFI_ALWAYS,"atbm_usb_bh waitevent ---atbm_usb_bh\n");
			rx = atbm_atomic_xchg(&hw_priv->bh_rx, 0);
			tx = atbm_atomic_xchg(&hw_priv->bh_tx, 0);
			urb_compl = atbm_atomic_xchg(&hw_priv->urb_comp, 0);
		
		}while(0);
		
		if ( hw_priv->bh_term || hw_priv->bh_error){
			wifi_printk(WIFI_DBG_ERROR,"%s BH thread break %d %d\n",__FUNCTION__,hw_priv->bh_term,hw_priv->bh_error);
			break;
		}
		if(urb_compl){
			//wifi_printk(WIFI_ALWAYS," urb_compl ----->.\n");
			atbm_urb_coml(hw_priv);
		}
        
		if (rx){
			atbm_rx_task(hw_priv);	
		}
		if (tx){
			atbm_tx_task(hw_priv);
		}

		wifi_printk(WIFI_BH,"atbm_usb_bh while--\n");
	}
	wifi_printk(WIFI_DBG_ERROR, "%s --\n",__func__);
	
	atbm_ThreadStopEvent(hw_priv->bh_thread);
}

int atbm_register_bh(struct atbmwifi_common *hw_priv)
{
	atbm_thread_internal_t *bh_thread;
	int status =0;
	atbm_atomic_set(&hw_priv->bh_rx, 0);
	atbm_atomic_set(&hw_priv->bh_tx, 0);
	atbm_os_init_waitevent(&hw_priv->bh_wq);
	atbm_os_init_waitevent(&hw_priv->wsm_cmd_wq);
	hw_priv->bh_term=0;
	hw_priv->bh_error=0;
	wifi_printk(WIFI_DBG_ERROR,"atbm_register_bh\n");

	/*Create RxData Task*/
	bh_thread=atbm_createThreadInternal("atbm_bh",atbm_usb_bh,(atbm_void*)hw_priv,BH_TASK_PRIO);
	if (!bh_thread){
		wifi_printk(WIFI_DBG_ERROR,"bh_thread Failed\n");
		return -1;
	}
	hw_priv->bh_thread = bh_thread;
#if USE_MAIL_BOX
	/*create the usb tx complete mailbox*/
	atbm_createMailBox((atbm_void*)hw_priv);
#endif
	return status;
}
atbm_void atbm_unregister_bh(struct atbmwifi_common *hw_priv)
{
	//kill createThread
	hw_priv->bh_term=1;
	atbm_os_wakeup_event(&hw_priv->bh_wq);
	atbm_stopThread(hw_priv->bh_thread);

	atbm_os_delete_waitevent(&hw_priv->bh_wq);
	atbm_os_delete_waitevent(&hw_priv->wsm_cmd_wq);
	atbm_os_DeleteMutex(&hw_priv->wsm_cmd_mux);
}

