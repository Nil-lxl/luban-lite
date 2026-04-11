/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
/* TODO: Verify these numbers with WSM specification. */
#include "atbm_hal.h"
#include "atbm_sdio.h"
#include "atbm_sdio_bh.h"
#include "atbm_sdio_hwio.h"

#define DOWNLOAD_BLOCK_SIZE_WR	(0x2000 - 4)
/* an SPI message cannot be bigger than (2"12-1)*2 bytes
 * "*2" to cvt to bytes */
#define MAX_SZ_RD_WR_BUFFERS	(DOWNLOAD_BLOCK_SIZE_WR*2)
#define PIGGYBACK_CTRL_REG	(2)
#define EFFECTIVE_BUF_SIZE	(MAX_SZ_RD_WR_BUFFERS - PIGGYBACK_CTRL_REG)
#define SDIO_TX_MAXLEN 4096
#if ATBM_MUTIL_PACKET
static atbm_uint8 *  AggrBuffer=ATBM_NULL;
#endif
#ifdef ATBM_SDIO_READ_ENHANCE

enum atbm_bh_read_action {
	ATBM_BH_READ_ACT__NO_DATA,
	ATBM_BH_READ_ACT__HAS_DATA,
	ATBM_BH_READ_ACT__READ_ERR,
	ATBM_BH_READ_ACT__NONE,
};
typedef int (*reed_ctrl_handler_t)(struct atbmwifi_common *hw_priv,atbm_uint16 *ctrl_reg);
typedef int (*reed_data_handler_t)(struct atbmwifi_common *hw_priv, void *buf, atbm_uint32 buf_len);
#endif

extern int atbm_reg_read_16(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint16 *val);
extern int atbm_reg_read_32(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint32 *val);
extern int atbm_reg_write_32(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint32 val);

static struct atbm_buff *atbm_get_skb(struct atbmwifi_common *hw_priv, atbm_uint32 len)
{
	struct atbm_buff *skb;
	
	atbm_uint32 alloc_len = (len > ATBM_SDIO_BLOCK_SIZE) ? len : ATBM_SDIO_BLOCK_SIZE;
	skb = atbm_dev_alloc_skb(alloc_len
				+ WSM_TX_EXTRA_HEADROOM
				+ 8  /* TKIP IV */
				+ 12 /* TKIP ICV + MIC */
				- 2  /* Piggyback */);
		/* In AP mode RXed SKB can be looped back as a broadcast.
		 * Here we reserve enough space for headers. */
		//atbm_skb_reserve(skb, WSM_TX_EXTRA_HEADROOM
				//+ 8 /* TKIP IV */
				//- WSM_RX_EXTRA_HEADROOM);
	ATBM_BUG_ON(skb->ref != 1);
	return skb;
}


void atbm_put_skb(struct atbmwifi_common *hw_priv, struct atbm_buff *skb)
{
	ATBM_BUG_ON(skb->ref != 1);
	#if 0
	if (hw_priv->skb_cache){
		atbm_dev_kfree_skb(skb);
	}
	else
		hw_priv->skb_cache = skb;
	#else
	atbm_dev_kfree_skb(skb);
	#endif
}
static int atbm_bh_read_ctrl_reg(struct atbmwifi_common *hw_priv,
					  atbm_uint16 *ctrl_reg)
{
	int ret=0,retry=0;
	while (retry <= MAX_RETRY) {
		ret = atbm_reg_read_16(hw_priv,
				ATBM_HIFREG_CONTROL_REG_ID, ctrl_reg);
		if(!ret){
				break;
		}else{
			/*reset sdio internel reg by send cmd52 to abort*/
			ATBM_WARN_ON(hw_priv->sbus_ops->abort(hw_priv->sbus_priv));
			retry++;
			atbm_mdelay(retry);
			wifi_printk(WIFI_BH,"[BH] Failed to read control register.ret=%x\n",ret);
		}
	}
	return ret;
}
static void atbm_hw_buff_reset(struct atbmwifi_common *hw_priv)
{
	int i;
	hw_priv->wsm_tx_seq = 0;
	hw_priv->buf_id_tx = 0;
	hw_priv->wsm_rx_seq = 0;
	hw_priv->hw_bufs_used = 0;
	hw_priv->buf_id_rx = 0;
	for (i = 0; i < ATBM_WIFI_MAX_VIFS; i++)
		hw_priv->hw_bufs_used_vif[i] = 0;
}
int atbm_powerave_sdio_sync(struct atbmwifi_common *hw_priv)
{
	int ret=0;
	atbm_uint32 config_reg;
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &config_reg);
	if (ret < 0) {
		wifi_printk(WIFI_DBG_ERROR, "%s: enable_irq: can't read config register.\n", __func__);
	}

	if(config_reg & ATBM_HIFREG_PS_SYNC_SDIO_FLAG)
	{
		atbm_hw_buff_reset(hw_priv);
		config_reg |= ATBM_HIFREG_PS_SYNC_SDIO_CLEAN;
		atbm_reg_write_32(hw_priv,ATBM_HIFREG_CONFIG_REG_ID,config_reg);
	}
	return ret;
}

int atbm_device_wakeup(struct atbmwifi_common *hw_priv)
{
	atbm_uint16 ctrl_reg;
	int ret=0;
	int loop = 1;

	/* To force the device to be always-on, the host sets WLAN_UP to 1 */
	ret = atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID,
			ATBM_HIFREG_CONT_WUP_BIT);
	if (ATBM_WARN_ON(ret))
		return ret;

	while(1){
		ret = atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
		if (ATBM_WARN_ON(ret)){
			wifi_printk(WIFI_ALWAYS,"[BH] wake up err\n");
			goto __loop_continue;
		}
		/* If the device returns WLAN_RDY as 1, the device is active and will
		 * remain active. */
		if (ctrl_reg & ATBM_HIFREG_CONT_RDY_BIT) {
			wifi_printk(WIFI_BH,"[BH] Device awake.<%d>\n",loop);
			ret= 1;
			goto __wup_exit;
		}
__loop_continue:
		//wait 1S,else fail
		if(loop++ > 1000){
			ret = atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			wifi_printk(WIFI_ALWAYS,"[BH] Device wakeup Fail<%d> %x\n",loop,ctrl_reg);
			ret= -1;
			goto __wup_exit;
		}
		atbm_mdelay(2);
	}
	wifi_printk(WIFI_BH,"[BH]  wakeup err\n");
__wup_exit:

	if((ctrl_reg & (ATBM_HIFREG_CONT_RDY_BIT|ATBM_HIFREG_CONT_WUP_BIT))
		!= (ATBM_HIFREG_CONT_RDY_BIT|ATBM_HIFREG_CONT_WUP_BIT)){
		wifi_printk(WIFI_BH,"[BH]----->	wakeup ctrl_reg %04x\n",ctrl_reg);
	}

	return ret;
}

int rxMutiCnt[17]={0};
int rxCnt=0;
int rxMutiCnt_Num;
int atbm_rx_tasklet(struct atbmwifi_common *hw_priv, int id,
		  struct wsm_hdr *wsm, struct atbm_buff **skb_p)
{
	struct atbm_buff *skb = *skb_p;
	struct atbm_buff *skb_copy;
	//struct wsm_hdr *wsm;
	atbm_uint32 wsm_len;
	int wsm_id;
	int data_len;
#define RX_ALLOC_BUFF_OFFLOAD (  (40+16)/*RX_DESC_OVERHEAD*/ -16 /*WSM_HI_RX_IND*/)
	wsm_len = __atbm_le32_to_cpu(wsm->len);
	wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
	if(wsm_id == WSM_MULTI_RECEIVE_INDICATION_ID ||
		wsm_id == WSM_SINGLE_CHANNEL_MULTI_RECEIVE_INDICATION_ID){
		struct wsm_multi_rx *  multi_rx = (struct wsm_multi_rx *)ATBM_OS_SKB_DATA(skb);			
		int RxFrameNum = multi_rx->RxFrameNum;
		
		data_len = wsm_len ;
		data_len -= sizeof(struct wsm_multi_rx);
		
		rxMutiCnt[ATBM_ALIGN(wsm_len,1024)/1024]++;
		rxMutiCnt_Num+=RxFrameNum;
		
		wsm = (struct wsm_hdr *)(multi_rx+1);
		wsm_len = __atbm_le32_to_cpu(wsm->len);
		wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
		
		//frame_hexdump("dump sdio wsm rx ->",wsm,32);
		do {

			if(data_len < wsm_len){
				wifi_printk(WIFI_BH,"skb->len %x,wsm_len %x data_len %x\n",ATBM_OS_SKB_LEN(skb),wsm_len,data_len);
				//frame_hexdump("dump sdio wsm rx ->",skb->data,64);
				break;
			}
			ATBM_BUG_ON((wsm_id  & (~WSM_TX_LINK_ID(WSM_TX_LINK_ID_MAX))) !=  WSM_RECEIVE_INDICATION_ID);
			skb_copy = atbm_dev_alloc_skb(wsm_len + 16);
			ATBM_BUG_ON(skb_copy == NULL);
			atbm_skb_reserve(skb_copy,(8 - (((unsigned long)ATBM_OS_SKB_DATA(skb_copy))&7))/*ALIGN 8*/);
			
			atbm_memmove(ATBM_OS_SKB_DATA(skb_copy), wsm, wsm_len);
			atbm_skb_put(skb_copy,wsm_len);
			wsm_handle_rx(hw_priv,wsm_id,wsm,&skb_copy);
			data_len -= ATBM_ALIGN(wsm_len + RX_ALLOC_BUFF_OFFLOAD,4);
			RxFrameNum--;

			wsm = (struct wsm_hdr *)((atbm_uint8 *)wsm +ATBM_ALIGN(( wsm_len + RX_ALLOC_BUFF_OFFLOAD),4));
			wsm_len = __atbm_le32_to_cpu(wsm->len);
			wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
			
			if(skb_copy != ATBM_NULL){
				atbm_dev_kfree_skb(skb_copy);
			}
		}while((RxFrameNum>0) && (data_len > 32));
		ATBM_BUG_ON(RxFrameNum != 0);
	}
	else {
		//rxMutiCnt[ALIGN(wsm_len,1024)/1024]++;
		rxCnt++;
		return wsm_handle_rx(hw_priv,id,wsm,skb_p);
	}
	return 0;
}
/*
static void atbm_oob_intr_set(struct atbmwifi_common *hw_priv,atbm_uint8 enable)
{
	if(hw_priv->sbus_ops->sdio_irq_en)
		hw_priv->sbus_ops->sdio_irq_en(hw_priv->sbus_priv,enable);
}
*/
#if ATBM_SDIO_READ_ENHANCE
int atbm_sdio_process_read_data(struct atbmwifi_common *hw_priv,reed_ctrl_handler_t read_ctrl_func,
	reed_data_handler_t read_data_func)
{
	#define LMAC_MAX_RX_BUFF	(24)
	atbm_uint32 read_len_lsb = 0;
	atbm_uint32 read_len_msb = 0;
	atbm_uint32 read_len;
	struct atbm_buff *skb_rx = NULL;
	atbm_uint16 ctrl_reg = 0;
	atbm_uint32 alloc_len;
	atbm_uint8 *data;
	atbm_uint8 rx_continue_cnt = 0;

rx_continue:
	if (ATBM_WARN_ON(read_ctrl_func(
			hw_priv, &ctrl_reg))){
			wifi_printk(WIFI_ALWAYS,"%s:read_ctrl_func err\n",__func__);
			goto err;
	}
	read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
	read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
	read_len=((read_len_msb>>2)+read_len_lsb);
	if (!read_len) {
		return 0;
	}

	if (ATBM_WARN_ON((read_len < sizeof(struct wsm_hdr)) ||
			(read_len > EFFECTIVE_BUF_SIZE))) {
			wifi_printk(WIFI_ALWAYS, "Invalid read len: %d,read_cnt(%d)\n",
				read_len,rx_continue_cnt);
		goto err;
	}
	/* Add SIZE of PIGGYBACK reg (CONTROL Reg)
	 * to the NEXT Message length + 2 Bytes for SKB */
	read_len = read_len + 2;
	alloc_len = read_len;
	if (alloc_len % ATBM_SDIO_BLOCK_SIZE ) {
		alloc_len -= (alloc_len % ATBM_SDIO_BLOCK_SIZE );
		alloc_len += ATBM_SDIO_BLOCK_SIZE;
	}
	/* Check if not exceeding CW1200 capabilities */
	if (ATBM_WARN_ON(alloc_len > EFFECTIVE_BUF_SIZE)) {
		wifi_printk(WIFI_ALWAYS,"Read aligned len: %d\n",
			alloc_len);
	}
	skb_rx = atbm_get_skb(hw_priv, alloc_len);
	if (ATBM_WARN_ON(!skb_rx)){
		wifi_printk(WIFI_ALWAYS, "%s:can not get skb,rx_continue_cnt(%d)\n",__func__,rx_continue_cnt);
		goto err;
	}

	atbm_skb_trim(skb_rx, 0);
	atbm_skb_put(skb_rx, read_len);
	data = ATBM_OS_SKB_DATA(skb_rx);
	if (ATBM_WARN_ON(!data)){
		goto err;
	}
	if (ATBM_WARN_ON(read_data_func(hw_priv, data, alloc_len))){
		wifi_printk(WIFI_ALWAYS, "%s:read_data_func err,rx_continue_cnt(%d)\n",__func__,rx_continue_cnt);
		goto err;
	}
	/* Piggyback */
	#if 0
	ctrl_reg = __atbm_le16_to_cpu(
		((__le16 *)data)[alloc_len / 2 - 1]);
	#endif
	if(atbm_bh_is_term(hw_priv) || atbm_atomic_read(&hw_priv->bh_term)){
		goto err;
	}
	else {
		atbm_skb_queue_tail(&hw_priv->rx_frame_queue, skb_rx);
	}
	read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
	read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
	read_len=((read_len_msb>>2)+read_len_lsb);
	rx_continue_cnt++;
	if (atbm_atomic_add_return(1, &hw_priv->bh_rx) == 1){
		atbm_os_wakeup_event(&hw_priv->bh_wq);
	}
	goto rx_continue;
err:
	if(skb_rx)
		atbm_dev_kfree_skb(skb_rx);
	return -1;
}

enum atbm_bh_read_action atbm_bh_read_miss_data(struct atbmwifi_common *hw_priv)
{
	enum atbm_bh_read_action action = ATBM_BH_READ_ACT__NO_DATA;
	if(!atbm_os_mutextryLock(&hw_priv->sdio_rx_process_lock)){
		int ret = 0;
		ret = atbm_sdio_process_read_data(hw_priv,atbm_bh_read_ctrl_reg,atbm_data_read);
		if(ret == 0)
			action = atbm_atomic_xchg(&hw_priv->bh_rx, 0) ? ATBM_BH_READ_ACT__HAS_DATA : ATBM_BH_READ_ACT__NO_DATA;
		else
			action = ATBM_BH_READ_ACT__READ_ERR;
		atbm_os_mutexUnLock(&hw_priv->sdio_rx_process_lock);
	}else {
		action = ATBM_BH_READ_ACT__HAS_DATA;
	}
	return action;
}
int atbm_rx_bh_flush(struct atbmwifi_common *hw_priv)
{
	struct atbm_buff *skb ;

	while ((skb = atbm_skb_dequeue(&hw_priv->rx_frame_queue)) != NULL) {
		//printk("test=====>kfree skb %p \n",skb);
		atbm_dev_kfree_skb(skb);
	}
	return 0;
}
#define ATBM_BH_CHECK_MISS_DATA(_hw_priv,_rx,rx_lable,err_lable)	\
	do													\
	{													\
		switch(atbm_bh_read_miss_data(_hw_priv))		\
		{												\
			case ATBM_BH_READ_ACT__NO_DATA:				\
				break;									\
			case ATBM_BH_READ_ACT__HAS_DATA:			\
				_rx = 1;								\
				goto rx_lable;							\
			case ATBM_BH_READ_ACT__READ_ERR:			\
				_hw_priv->bh_error = 1;					\
				goto err_lable;							\
			default:									\
				break;									\
		}												\
	}while(0)											
#endif


#if (ATBM_PLATFORM==AK_RTOS_37D)
extern int g_get_sdio_which_mci;
#endif

ATBM_BOOL atbm_sdio_wait_enough_space(struct atbmwifi_common	*hw_priv, atbm_uint32 n_needs)
{
#define MAX_LOOP_POLL_CNT  (2*3000)
	atbm_uint32 hw_xmited = 0;
	ATBM_BOOL enough = ATBM_FALSE;
	int ret = 0;
	int loop = 0;
	atbm_uint32 print = 0;

	atbm_spin_lock_bh(&hw_priv->tx_com_lock);
	enough = hw_priv->hw_bufs_free >= n_needs ? ATBM_TRUE : ATBM_FALSE;
	atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
	
	while(enough == ATBM_FALSE){
		if(atbm_bh_is_term(hw_priv)){
			wifi_printk(WIFI_DBG_ERROR, "%s:bh term\n",__func__);
			return ATBM_FALSE;
		}
#if ATBM_TX_SKB_NO_TXCONFIRM	
		hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
		ret = atbm_direct_read_unlock(hw_priv,hw_priv->wsm_caps.NumOfHwXmitedAddr,&hw_xmited);
		hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);

		if(ret){
			enough = ATBM_FALSE;
			break;
		}
		
		atbm_spin_lock_bh(&hw_priv->tx_com_lock);
		ATBM_BUG_ON((int)(hw_priv->n_xmits) < (int)hw_xmited);
		ATBM_BUG_ON((int)(hw_priv->n_xmits - hw_xmited) > hw_priv->wsm_caps.numInpChBufs);
		ATBM_BUG_ON((int)(hw_priv->n_xmits - hw_xmited)<0);
		hw_priv->hw_xmits = hw_xmited;
		hw_priv->hw_bufs_free =  (hw_priv->wsm_caps.numInpChBufs) - 
								 (hw_priv->n_xmits-hw_xmited);
		enough = hw_priv->hw_bufs_free >= n_needs ? ATBM_TRUE : ATBM_FALSE;
		hw_priv->hw_bufs_free_init = hw_priv->hw_bufs_free;
		atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
		if(enough == ATBM_FALSE){
			loop ++;
			if(loop>=MAX_LOOP_POLL_CNT)
				break;
			if((loop >= 3)&&(print == 0)){			
				wifi_printk(WIFI_DBG_ERROR, "%s:n_xmits(%d),hw_xmited(%d),need(%d)\n",__func__,
					hw_priv->n_xmits,hw_xmited,n_needs);
				print = 1;
			}
			atbm_SleepMs(2);
		}
#else
		atbm_spin_lock_bh(&hw_priv->tx_com_lock);
		hw_priv->hw_bufs_free = hw_priv->wsm_caps.numInpChBufs - hw_priv->hw_bufs_used;
		hw_priv->hw_bufs_free_init = hw_priv->hw_bufs_free;
		enough = hw_priv->hw_bufs_free >= n_needs ? ATBM_TRUE : ATBM_FALSE;
		atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
		if(enough == ATBM_FALSE){
			return ATBM_FALSE;
		}
#endif
	}
	return enough;
}

ATBM_BOOL atbm_sdio_have_enough_space(struct atbmwifi_common	*hw_priv, atbm_uint32 n_needs)
{
	atbm_uint32 hw_xmited = 0;
	ATBM_BOOL enough = ATBM_FALSE;
	int ret = 0;

	atbm_spin_lock_bh(&hw_priv->tx_com_lock);
	enough = hw_priv->hw_bufs_free >= n_needs ? ATBM_TRUE : ATBM_FALSE;
	atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
	if(enough == ATBM_FALSE){
#if ATBM_TX_SKB_NO_TXCONFIRM
		if(atbm_bh_is_term(hw_priv)){
			//wifi_printk(WIFI_DBG_ERROR, "%s:bh term\n",__func__);
			atbm_spin_lock_bh(&hw_priv->tx_com_lock);
			hw_priv->hw_bufs_free = n_needs;
			atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
			return ATBM_TRUE;
		}
		hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
		ret = atbm_direct_read_unlock(hw_priv,hw_priv->wsm_caps.NumOfHwXmitedAddr,&hw_xmited);
		hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
		//wifi_printk(WIFI_ALWAYS, "bbbhw_xmited:%d\n", hw_xmited);

		if(ret){
			enough = ATBM_FALSE;
		}else {
			atbm_spin_lock_bh(&hw_priv->tx_com_lock);
			ATBM_BUG_ON((int)hw_priv->n_xmits < (int)hw_xmited);
			ATBM_BUG_ON((int)(hw_priv->n_xmits - hw_xmited) > hw_priv->wsm_caps.numInpChBufs);
			ATBM_BUG_ON((int)(hw_priv->n_xmits - hw_xmited)<0);
			hw_priv->hw_xmits = hw_xmited;
			hw_priv->hw_bufs_free =  (hw_priv->wsm_caps.numInpChBufs) - 
									 (hw_priv->n_xmits-hw_xmited);
			hw_priv->hw_bufs_free_init = hw_priv->hw_bufs_free;
			enough = hw_priv->hw_bufs_free >= n_needs ? ATBM_TRUE : ATBM_FALSE;
			atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
			//wifi_printk(WIFI_ALWAYS, "hw_bufs_free:%d hw_bufs_free_init:%d hw_xmits:%d\n", hw_priv->hw_bufs_free, hw_priv->hw_bufs_free_init, hw_priv->hw_xmits);
		}
#else
		atbm_spin_lock_bh(&hw_priv->tx_com_lock);
		hw_priv->hw_bufs_free = hw_priv->wsm_caps.numInpChBufs - hw_priv->hw_bufs_used;
		enough = hw_priv->hw_bufs_free >= n_needs ? ATBM_TRUE : ATBM_FALSE;
		hw_priv->hw_bufs_free_init = hw_priv->hw_bufs_free;
		atbm_spin_unlock_bh(&hw_priv->tx_com_lock);
#endif
	}
	return enough;
}

#if (ATBM_TXRX_IN_ONE_THREAD && ATBM_TX_SKB_NO_TXCONFIRM)
atbm_void atbm_sdio_wait_timer_func(atbm_void *data1,atbm_void *data2){
	struct atbmwifi_common	*hw_priv = data1;
	if(atbm_sdio_have_enough_space(hw_priv, 1)){
		atbm_os_wakeup_event(&hw_priv->bh_wq);
		atbm_atomic_set(&hw_priv->wait_timer_start, 0);
	}else{
		atbm_atomic_set(&hw_priv->wait_timer_start, 1);
		atbmwifi_eloop_register_timeout(0, 5, atbm_sdio_wait_timer_func, hw_priv, ATBM_NULL);
	}
}
#endif

static void atbm_sdio_release_err_data(struct atbmwifi_common	*hw_priv,struct wsm_tx *wsm)
{
	struct atbmwifi_queue *queue;
	atbm_uint8 queue_id;
	struct atbm_buff *skb;
	const struct atbmwifi_txpriv *txpriv;
	
	wifi_printk(WIFI_DBG_ERROR, "%s:release tx pakage\n",__func__);
	ATBM_BUG_ON(wsm == NULL);
	queue_id = atbmwifi_queue_get_queue_id(wsm->packetID);

	ATBM_BUG_ON(queue_id >= 4);
	queue = &hw_priv->tx_queue[queue_id];
	ATBM_BUG_ON(queue == NULL);
	
	wsm_release_tx_buffer(hw_priv, 1);
	if(!ATBM_WARN_ON(atbmwifi_queue_get_skb(queue, wsm->packetID, &skb, &txpriv))) {
		//int tx_count = 0;
		wsm_release_vif_tx_buffer(hw_priv,txpriv->if_id,1);
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
		atbmwifi_queue_remove(hw_priv, queue, wsm->packetID);
#else
		atbmwifi_queue_remove(queue, wsm->packetID);
#endif
	}else {
		wsm_release_vif_tx_buffer(hw_priv,atbmwifi_queue_get_if_id(wsm->packetID),1);
	}
}


static int atbm_sdio_free_tx_wsm(struct atbmwifi_common	*hw_priv,struct wsm_tx *wsm)
{
	if((wsm) && (!(wsm->htTxParameters&atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM)))){
		struct atbmwifi_queue *queue;
		atbm_uint8 queue_id;
		struct atbm_buff *skb;
		const struct atbmwifi_txpriv *txpriv;

		queue_id = atbmwifi_queue_get_queue_id(wsm->packetID);

		ATBM_BUG_ON(queue_id >= 4);

		queue = &hw_priv->tx_queue[queue_id];
		ATBM_BUG_ON(queue == NULL);

		if(!ATBM_WARN_ON(atbmwifi_queue_get_skb(queue, wsm->packetID, &skb, &txpriv))) {
			wsm_release_vif_tx_buffer(hw_priv,txpriv->if_id,1);
			wsm_release_tx_buffer(hw_priv, 1);
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
			atbmwifi_queue_remove(hw_priv, queue, wsm->packetID);
#else
			atbmwifi_queue_remove(queue, wsm->packetID);
#endif
		}else {
			wsm_release_vif_tx_buffer(hw_priv,atbmwifi_queue_get_if_id(wsm->packetID),1);
			wsm_release_tx_buffer(hw_priv, 1);
		}
		return 1;
	}
	return 0;
}

static void atbm_sdio_force_free_wsm(struct atbmwifi_common	*hw_priv,struct wsm_tx *wsm)
{
	int wsm_id;

	if(wsm == NULL)
		return;

	wsm_id = __atbm_le16_to_cpu(wsm->hdr.id) & 0x3F;

	switch(wsm_id){
	case WSM_TRANSMIT_REQ_MSG_ID:
		atbm_sdio_release_err_data(hw_priv,wsm);
		break;
	default:
		wsm_release_tx_buffer(hw_priv, 1);
		atbm_spin_lock_bh(&hw_priv->wsm_cmd.lock);
		if(hw_priv->wsm_cmd.cmd != 0XFFFF){
			hw_priv->wsm_cmd.ret = -1;
			hw_priv->wsm_cmd.done = 1;
			hw_priv->wsm_cmd.cmd = 0xFFFF;
			hw_priv->wsm_cmd.ptr = NULL;
			hw_priv->wsm_cmd.arg = NULL;
			wifi_printk(WIFI_DBG_ERROR, "%s:release wsm_cmd.lock\n",__func__);		
			atbm_os_wakeup_event(&hw_priv->wsm_cmd_wq);
		}	
		atbm_spin_unlock_bh(&hw_priv->wsm_cmd.lock);
		break;
	}
}

#if ATBM_TXRX_IN_ONE_THREAD
static void atbm_sdio_bh(void *arg)
{
	static atbm_uint16 tx_cnt = 0;
	struct atbmwifi_common *hw_priv = arg;
	//struct atbmwifi_vif *priv = ATBM_NULL;
	struct atbm_buff *skb_rx = ATBM_NULL;
	atbm_uint32 read_len;
	atbm_uint32 read_len_lsb = 0;
	atbm_uint32 read_len_msb = 0;
	int dorx, dotx=0;
	struct wsm_hdr_tx *wsm_tx;
	struct wsm_hdr *wsm;
	atbm_uint32 wsm_len;
	int wsm_id;
	atbm_uint8 wsm_seq;
	int rx_resync = 1;
	atbm_uint16 ctrl_reg = 0;
	int tx_allowed;
	int tx_burst;
	int vif_selected;
	int status = 0;
	int ret_flush;				
#if ATBM_MUTIL_PACKET
	AggrBuffer= (atbm_uint8 *)atbm_kmalloc(SDIO_TX_MAXLEN,GFP_KERNEL);
	if (!AggrBuffer) {
		wifi_printk(WIFI_BH,"can't allocate bootloader buffer.\n");
		return ;
	}
#endif //ATBM_MUTIL_PACKET	
#define __ALL_HW_BUFS_USED (hw_priv->hw_bufs_used)
	while (1) {
		do {
			dorx = atbm_atomic_xchg(&hw_priv->bh_rx, 0);
			dotx = atbm_atomic_xchg(&hw_priv->bh_tx, 0);
			if(dorx || dotx){
				//wifi_printk(WIFI_ALWAYS, "dorx:%d dotx:%d dorx1:%d\n", dorx, dotx, atbm_atomic_read(&hw_priv->bh_rx));
				break;
			}
#if (ATBM_TX_SKB_NO_TXCONFIRM == 0)
			if (__ALL_HW_BUFS_USED)
			{
				atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
				if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)
				{
					dorx = 1;
					goto RxAction;
				}
			}
#endif

#if SUPPORT_LIGHT_SLEEP
			if(!hw_priv->device_can_sleep)
			{
				wifi_printk(WIFI_DBG_MSG, "Enter light sleep!!\n");
				ATBM_WARN_ON(atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID, 0));
				hw_priv->device_can_sleep = ATBM_TRUE;
			}
#endif
			if (hw_priv->bh_term){
				wifi_printk(WIFI_ALWAYS, "exit %s()\n", __func__);
				goto exit_thread;
			}

			status = atbm_os_wait_event_timeout(&hw_priv->bh_wq,10*HZ);
			dorx = atbm_atomic_xchg(&hw_priv->bh_rx, 0);
			dotx = atbm_atomic_xchg(&hw_priv->bh_tx, 0);
		}while(0);

		if(status == 0){
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)
			{
				dorx = 1;
				goto RxAction;
			}
		}
		if (dorx){
			atbm_uint32 alloc_len;
			atbm_uint8 *data;
			if(!(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)){				
				if (ATBM_WARN_ON(atbm_bh_read_ctrl_reg(
						hw_priv, &ctrl_reg)))
						break;
			}
RxAction:
			
			read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
			read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
			read_len=((read_len_msb>>2)+read_len_lsb);
			if (!read_len) {
				ctrl_reg = 0;
				goto RxContinue;
			}
			if (ATBM_WARN_ON((read_len < sizeof(struct wsm_hdr)) ||
					(read_len > EFFECTIVE_BUF_SIZE))) {
					wifi_printk(WIFI_BH,"Invalid read len: %d",
						read_len);
				break;
			}
			/* Add SIZE of PIGGYBACK reg (CONTROL Reg)
			 * to the NEXT Message length + 2 Bytes for SKB */
			read_len = read_len + 2;
			alloc_len = read_len;
			if (alloc_len % ATBM_SDIO_BLOCK_SIZE ) {
				alloc_len -= (alloc_len % ATBM_SDIO_BLOCK_SIZE );
				alloc_len += ATBM_SDIO_BLOCK_SIZE;
			}
			/* Check if not exceeding CW1200 capabilities */
			if (ATBM_WARN_ON(alloc_len > EFFECTIVE_BUF_SIZE)) {
				wifi_printk(WIFI_BH,"Read aligned len: %d\n",
					alloc_len);
			}
			skb_rx = atbm_get_skb(hw_priv, alloc_len);
			if (ATBM_WARN_ON(!skb_rx))
				break;

			atbm_skb_trim(skb_rx, 0);
			atbm_skb_put(skb_rx, read_len);
			data = ATBM_OS_SKB_DATA(skb_rx);
			if (ATBM_WARN_ON(!data))
				break;
			
			if (ATBM_WARN_ON(atbm_data_read(hw_priv, data, alloc_len)))
				break;			
			/* Piggyback */
		#if 1
			ctrl_reg = __atbm_le16_to_cpu(
				((atbm_uint16*)data)[alloc_len / 2 - 1]);
		#else
			ctrl_reg = 0;
		#endif
			wsm = (struct wsm_hdr *)data;
			//frame_hexdump("dump sdio wsm rx ->",wsm,32);
			wsm_len = __atbm_le16_to_cpu(wsm->len);
			if (ATBM_WARN_ON(wsm_len > read_len)){
				ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
				if(ret_flush==RECOVERY_ERR){
					wifi_printk(WIFI_ALWAYS,"RESET CHANN ERR %d\n",__LINE__);
					break;
				}else{
					ctrl_reg=0;
					//free skb
					atbm_dev_kfree_skb(skb_rx);
					skb_rx=NULL;
					continue;
				}
			}
			//Uart_PrintfBuf("dump_mem vsm",wsm,read_len);
			wsm_id	= __atbm_le16_to_cpu(wsm->id) & 0xFFF;
			wsm_seq = (__atbm_le32_to_cpu(wsm->id) >> 13) & 7;
			atbm_skb_trim(skb_rx, wsm_len);
			if (atbm_unlikely(wsm_id == 0x0800)) {
				wsm_handle_exception(hw_priv,
					 &data[sizeof(*wsm)],
					wsm_len - sizeof(*wsm));
				ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
				if(ret_flush==RECOVERY_ERR){
					wifi_printk(WIFI_ALWAYS,"RESET CHANN ERR %d\n",__LINE__);
					hw_priv->bh_error = 1;
					break;
				}else{
					ctrl_reg=0;
					//free skb
					atbm_dev_kfree_skb(skb_rx);
					skb_rx=NULL;
					//goto txrx;
 					continue;
				}
			} else if (atbm_unlikely(!rx_resync)) {
				if (ATBM_WARN_ON(wsm_seq != hw_priv->wsm_rx_seq)) {
					ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
					if(ret_flush==RECOVERY_ERR){
						wifi_printk(WIFI_ALWAYS,"RESET CHANN ERR %d\n",__LINE__);
						hw_priv->bh_error = 1;
						break;
					}else{
						ctrl_reg=0;
						//free skb
						atbm_dev_kfree_skb(skb_rx);
						skb_rx=NULL;
						continue;
					}
					break;
				}
			}
			hw_priv->wsm_rx_seq = (wsm_seq + 1) & 7;
			rx_resync = 0;

			if (wsm_id & 0x0400) {
				int rc = wsm_release_tx_buffer(hw_priv, 1);
				if (ATBM_WARN_ON(rc < 0))
					break;
				else if (rc > 0)
					dotx = 1;
			}
			/* atbm_wsm_rx takes care on SKB livetime */
			if (ATBM_WARN_ON(atbm_rx_tasklet(hw_priv, wsm_id, wsm,
						  &skb_rx))){
				wifi_printk(WIFI_DBG_ERROR,"atbm_rx_tasklet err\n");
				ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
                 if(ret_flush==RECOVERY_ERR){
                    wifi_printk(WIFI_DBG_ERROR,"RESET CHANN ERR %d\n",__LINE__);
                    hw_priv->bh_error = 1;
                    break;
                  }else{
                    ctrl_reg=0;
                    //free skb
                    atbm_dev_kfree_skb(skb_rx);
                    skb_rx=NULL;
                    continue;
                  }
				break;
			}
			if (skb_rx) {
				atbm_put_skb(hw_priv, skb_rx);
				skb_rx = ATBM_NULL;
			}
			read_len = 0;
			atbm_atomic_set(&hw_priv->bh_rx, 1);
			goto RxAction;
RxContinue:
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK){
				goto RxAction;
			}
			atbm_sdio_host_enable_irq(1); //Enable irq at first rxframe?
		}
		if(dotx)
TxAction:
		{
#define WSM_SDIO_TX_MULT_BLOCK_SIZE	(ATBM_SDIO_BLOCK_SIZE)
#define ATBM_SDIO_FREE_BUFF_ERR(condition,free,prev_free,xmiteds,hw_xmiteds)	\
		do{ 																		\
			if(condition)	{																\
				wifi_printk(WIFI_DBG_ERROR, "%s[%d]:free(%x),prev_free(%x),xmiteds(%x),hw_xmiteds(%x)\n",__func__,__LINE__,free,prev_free,xmiteds,hw_xmiteds);	\
				ATBM_BUG_ON(1); 		\
			}\
		}while(0)

#if ATBM_TX_SKB_NO_TXCONFIRM
			static atbm_uint8 loop = 1;
#else
			static atbm_uint8 loop = 0;
#endif
			int tx_burst;
			struct wsm_hdr_tx *wsm_tx;
			int vif_selected;
			atbm_uint32 tx_len=0;
			atbm_uint32 putLen=0;
			atbm_uint8 *data;
			atbm_uint8 *need_confirm = NULL;
			int ret=0;
			int txMutiFrameCount=0;
#if (PROJ_TYPE>=ARES_A)
			atbm_uint32 wsm_flag_u32 = 0;
			atbm_uint16 wsm_len_u16[2];
			atbm_uint16 wsm_len_sum;
#endif	//	(PROJ_TYPE==ARES_A)	
			ATBM_BOOL enough = ATBM_FALSE;

#if SUPPORT_LIGHT_SLEEP
			if (hw_priv->device_can_sleep) {
				wifi_printk(WIFI_DBG_MSG, "Exit light sleep!!!\n");
				ret = atbm_device_wakeup(hw_priv);
				if (ATBM_WARN_ON(ret < 0))
					return;
				else if (ret)
					hw_priv->device_can_sleep = ATBM_FALSE;
			}
#endif

xmit_continue:
			txMutiFrameCount = 0;
			putLen = 0;
			enough = ATBM_FALSE;
			need_confirm = ATBM_NULL;
			do {
				enough = atbm_sdio_have_enough_space(hw_priv,1);
				if(enough == ATBM_FALSE){
					if(txMutiFrameCount > 0)
						break;
					else
						goto xmit_wait;
				}
				ret = wsm_get_tx(hw_priv, &data, &tx_len, &tx_burst,&vif_selected);
				
				if (ret <= 0) {
					if(txMutiFrameCount > 0)
						break;
					else
						goto xmit_finished;
				}
				txMutiFrameCount++;
				wsm_tx = (struct wsm_hdr_tx *)data;
				ATBM_BUG_ON(tx_len < sizeof(*wsm_tx));
				ATBM_BUG_ON(__atbm_le32_to_cpu(wsm_tx->len) != tx_len);
				
#if (PROJ_TYPE>=ARES_A)
				wsm_flag_u32 = (tx_len) & 0xffff;
				wsm_len_u16[0] = wsm_flag_u32 & 0xff;
				wsm_len_u16[1] = (wsm_flag_u32 >> 8)& 0xff;
				wsm_len_sum = wsm_len_u16[0] + wsm_len_u16[1];
				if (wsm_len_sum & BIT(8)){
					wsm_flag_u32 |= ((wsm_len_sum + 1) & 0xff) << 24;
				}else {
					wsm_flag_u32 |= (wsm_len_sum & 0xff) << 24;
				}
				wsm_tx->flag=__atbm_le32_to_cpu(wsm_flag_u32);
#endif //(PROJ_TYPE==ARES_A)
				if (tx_len <= 8)
					tx_len = 16;
		
				if (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) ) {
					tx_len -= (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) );
					tx_len += WSM_SDIO_TX_MULT_BLOCK_SIZE;
				}
		
				/* Check if not exceeding atbm
				capabilities */
				if (ATBM_WARN_ON(tx_len > EFFECTIVE_BUF_SIZE)) {
					wifi_printk(WIFI_DBG_ERROR, "Write aligned len:"
					" %d\n", tx_len);
				}
#if (PROJ_TYPE<ARES_A)					
				wsm_tx->flag=(((tx_len/ATBM_SDIO_BLOCK_SIZE)&0xff)-1);
#endif //(PROJ_TYPE==ARES_A)
				wsm_tx->id &= __atbm_le32_to_cpu(~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
				wsm_tx->id |= atbm_cpu_to_le32(WSM_TX_SEQ(hw_priv->wsm_tx_seq));
#if ATBM_WSM_SDIO_TX_MULT
				wsm_tx->tx_len = tx_len;
				wsm_tx->tx_id = wsm_tx->id;
#endif
				wsm_alloc_tx_buffer(hw_priv);
				atbm_spin_lock_bh(&hw_priv->tx_com_lock);
				ATBM_SDIO_FREE_BUFF_ERR(hw_priv->hw_bufs_free <= 0,hw_priv->hw_bufs_free,hw_priv->hw_bufs_free_init,hw_priv->n_xmits,hw_priv->hw_xmits);
				hw_priv->n_xmits ++;
				hw_priv->hw_bufs_free --;		
				ATBM_SDIO_FREE_BUFF_ERR(hw_priv->hw_bufs_free < 0,hw_priv->hw_bufs_free,hw_priv->hw_bufs_free_init,hw_priv->n_xmits,hw_priv->hw_xmits);
				atbm_spin_unlock_bh(&hw_priv->tx_com_lock);

				atbm_memcpy(&hw_priv->xmit_buff[putLen], data, wsm_tx->len);
				putLen += tx_len;
				hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;
		
				if (vif_selected != -1) {
					hw_priv->hw_bufs_used_vif[vif_selected]++;
				}

				if(wsm_txed(hw_priv, data)){
					need_confirm = data;
					wifi_printk(WIFI_DBG_MSG, "%s:cmd free(%d),used(%d)\n",__func__,hw_priv->hw_bufs_free,hw_priv->hw_bufs_used);
					break;
				}else {
					hw_priv->wsm_txframe_num++;
#if ATBM_TX_SKB_NO_TXCONFIRM
					if(atbm_sdio_free_tx_wsm(hw_priv,(struct wsm_tx *)data) == 0){
						need_confirm = data;
						wifi_printk(WIFI_DBG_MSG, "%s:confirm free(%d),used(%d)\n",__func__,hw_priv->hw_bufs_free,hw_priv->hw_bufs_used);
						break;
					}
#else
					need_confirm = data;
#endif
				}
				
				if (putLen+hw_priv->wsm_caps.sizeInpChBuf>SDIO_TX_MAXLEN){
					break;
				}
			}while(loop);
			ATBM_BUG_ON(putLen == 0);
			hw_priv->buf_id_offset = txMutiFrameCount;
			//atbm_atomic_add(1, &hw_priv->bh_tx);
		
			if(atbm_bh_is_term(hw_priv)){
				wifi_printk(WIFI_DBG_ERROR, "%s:bh term\n",__func__);
				atbm_sdio_force_free_wsm(hw_priv,(struct wsm_tx *)need_confirm);
				goto xmit_continue;
			}

			//dump_mem(hw_priv->xmit_buff, putLen);
			if (ATBM_WARN_ON(atbm_data_write(hw_priv,hw_priv->xmit_buff, putLen))) {	
				wifi_printk(WIFI_DBG_ERROR, "%s: xmit data err\n",__func__);
				goto xmit_err;
			}

			if(!atbm_sdio_have_enough_space(hw_priv,1)){
				goto xmit_wait;
			}
			goto xmit_continue;
xmit_wait:
#if ATBM_TX_SKB_NO_TXCONFIRM
			if(!atbm_atomic_read(&hw_priv->wait_timer_start)){
				atbm_atomic_set(&hw_priv->wait_timer_start, 1);
				atbmwifi_eloop_register_timeout(0, 5, atbm_sdio_wait_timer_func, hw_priv, ATBM_NULL);
			}
#endif
			continue;
xmit_finished:
			if (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)
				goto RxAction;
			continue;
xmit_err:
			atbm_sdio_force_free_wsm(hw_priv,(struct wsm_tx *)need_confirm);
			//atbm_bh_halt(hw_priv);
			goto xmit_continue;
		}
	}
exit_thread:
	if (skb_rx) {
		atbm_put_skb(hw_priv, skb_rx);
		skb_rx = ATBM_NULL;
	}
#if ATBM_MUTIL_PACKET
	atbm_kfree(AggrBuffer);
#endif //#if ATBM_MUTIL_PACKET
	wifi_printk(WIFI_ALWAYS,"sdio bh exit\n");
	//atbm_monitor_pc(hw_priv);
	return ;
}

#else

static int atbm_sdio_rx_bh(void *arg){
	struct atbmwifi_common *hw_priv = (struct atbmwifi_common *)arg;
	int status = 0;
	int dorx=0;
	atbm_uint32 read_len;
	atbm_uint32 read_len_lsb = 0;
	atbm_uint32 read_len_msb = 0;
	atbm_uint16 ctrl_reg = 0;
	struct atbm_buff *skb_rx = ATBM_NULL;
	struct wsm_hdr *wsm;
	atbm_uint32 wsm_len;
	int wsm_id;
	atbm_uint8 wsm_seq;
	int ret_flush;
	int rx_resync = 1;

	while(!hw_priv->bh_term){
		status = atbm_os_wait_event_timeout(&hw_priv->rx_bh_wq, 5*HZ);
		dorx = atbm_atomic_xchg(&hw_priv->bh_rx, 0);
		if(!dorx && status == 0){
			atbm_bh_read_ctrl_reg(hw_priv, &ctrl_reg);
			if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK){
				wifi_printk(WIFI_DBG_ERROR, "bh miss [%x]!!\n", ctrl_reg);
				hw_priv->rx_inprogress = 1;
				goto RxAction;
			}
		}

		if (dorx){
			atbm_uint32 alloc_len;
			atbm_uint8 *data;
			if(!(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK)){				
				if (ATBM_WARN_ON(atbm_bh_read_ctrl_reg(
						hw_priv, &ctrl_reg)))
						break;
			}
			hw_priv->rx_inprogress = 1;
RxAction:
			read_len_lsb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK)*2;
			read_len_msb = (ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK)*2;
			read_len=((read_len_msb>>2)+read_len_lsb);
			if (!read_len) {
				ctrl_reg = 0;
				goto RxContinue;
			}
			if (ATBM_WARN_ON((read_len < sizeof(struct wsm_hdr)) ||
					(read_len > EFFECTIVE_BUF_SIZE))) {
					wifi_printk(WIFI_BH,"Invalid read len: %d",
						read_len);
				break;
			}
			/* Add SIZE of PIGGYBACK reg (CONTROL Reg)
			 * to the NEXT Message length + 2 Bytes for SKB */
			read_len = read_len + 2;
			alloc_len = read_len;
			if (alloc_len % ATBM_SDIO_BLOCK_SIZE ) {
				alloc_len -= (alloc_len % ATBM_SDIO_BLOCK_SIZE );
				alloc_len += ATBM_SDIO_BLOCK_SIZE;
			}
			/* Check if not exceeding CW1200 capabilities */
			if (ATBM_WARN_ON(alloc_len > EFFECTIVE_BUF_SIZE)) {
				wifi_printk(WIFI_BH,"Read aligned len: %d\n",
					alloc_len);
			}
			skb_rx = atbm_get_skb(hw_priv, alloc_len);
			if (ATBM_WARN_ON(!skb_rx))
				break;

			atbm_skb_trim(skb_rx, 0);
			atbm_skb_put(skb_rx, read_len);
			data = ATBM_OS_SKB_DATA(skb_rx);
			if (ATBM_WARN_ON(!data))
				break;
			if (ATBM_WARN_ON(atbm_data_read(hw_priv, data, alloc_len)))
				break;
			/* Piggyback */
#if 1
			ctrl_reg = __atbm_le16_to_cpu(
				((atbm_uint16*)data)[alloc_len / 2 - 1]);
#else
			ctrl_reg = 0;
#endif
			wsm = (struct wsm_hdr *)data;
			//frame_hexdump("dump sdio wsm rx ->",wsm,32);
			wsm_len = __atbm_le16_to_cpu(wsm->len);
			if (ATBM_WARN_ON(wsm_len > read_len)){
				wifi_printk(WIFI_DBG_ERROR, "rx len err %x %x\n", wsm_len, read_len);
				ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
				if(ret_flush==RECOVERY_ERR){
					wifi_printk(WIFI_ALWAYS,"RESET CHANN ERR %d\n",__LINE__);
					break;
				}else{
					ctrl_reg=0;
					//free skb
					atbm_dev_kfree_skb(skb_rx);
					skb_rx=NULL;
					goto RxContinue;
				}
			}
			//Uart_PrintfBuf("dump_mem vsm",wsm,read_len);
			wsm_id	= __atbm_le16_to_cpu(wsm->id) & 0xFFF;
			wsm_seq = (__atbm_le32_to_cpu(wsm->id) >> 13) & 7;
			atbm_skb_trim(skb_rx, wsm_len);
			if (atbm_unlikely(wsm_id == 0x0800)) {
				wsm_handle_exception(hw_priv,
					 &data[sizeof(*wsm)],
					wsm_len - sizeof(*wsm));
				ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
				if(ret_flush==RECOVERY_ERR){
					wifi_printk(WIFI_ALWAYS,"RESET CHANN ERR %d\n",__LINE__);
					hw_priv->bh_error = 1;
					break;
				}else{
					ctrl_reg=0;
					//free skb
					atbm_dev_kfree_skb(skb_rx);
					skb_rx=NULL;
					//goto txrx;
 					goto RxContinue;
				}
			} else if (atbm_unlikely(!rx_resync)) {
				if (ATBM_WARN_ON(wsm_seq != hw_priv->wsm_rx_seq)) {
					wifi_printk(WIFI_DBG_ERROR, "rx seq err %x %x\n", wsm_seq, hw_priv->wsm_rx_seq);
					ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
					if(ret_flush==RECOVERY_ERR){
						wifi_printk(WIFI_ALWAYS,"RESET CHANN ERR %d\n",__LINE__);
						hw_priv->bh_error = 1;
						break;
					}else{
						ctrl_reg=0;
						//free skb
						atbm_dev_kfree_skb(skb_rx);
						skb_rx=NULL;
						goto RxContinue;
					}
					break;
				}
			}
			hw_priv->wsm_rx_seq = (wsm_seq + 1) & 7;
			rx_resync = 0;

			if (wsm_id & 0x0400) {
				int rc = wsm_release_tx_buffer(hw_priv, 1);
				if (ATBM_WARN_ON(rc < 0))
					break;
			}
			/* atbm_wsm_rx takes care on SKB livetime */
			if (ATBM_WARN_ON(atbm_rx_tasklet(hw_priv, wsm_id, wsm,
						  &skb_rx))){
				wifi_printk(WIFI_DBG_ERROR,"atbm_rx_tasklet err\n");
				ret_flush=wsm_sync_channle_process(hw_priv,OUT_BH);
                 if(ret_flush==RECOVERY_ERR){
                    wifi_printk(WIFI_DBG_ERROR,"RESET CHANN ERR %d\n",__LINE__);
                    hw_priv->bh_error = 1;
                    break;
                  }else{
                    ctrl_reg=0;
                    //free skb
                    atbm_dev_kfree_skb(skb_rx);
                    skb_rx=NULL;
                    goto RxContinue;
                  }
				break;
			}
			if (skb_rx) {
				atbm_put_skb(hw_priv, skb_rx);
				skb_rx = ATBM_NULL;
			}
			read_len = 0;
			goto RxAction;
RxContinue:
			if(ctrl_reg & ATBM_HIFREG_CONT_NEXT_LEN_MASK){
				goto RxAction;
			}
#if SUPPORT_LIGHT_SLEEP
			atbm_os_mutexLock(&hw_priv->sleep_mutex, 0);
			hw_priv->rx_inprogress = 0;
			if(!(atbm_atomic_read(&hw_priv->bh_tx) || hw_priv->tx_inprogress)){
				if(!hw_priv->device_can_sleep)
				{
					wifi_printk(WIFI_DBG_MSG, "rx Enter light sleep!!\n");
					ATBM_WARN_ON(atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID, 0));
					hw_priv->device_can_sleep = ATBM_TRUE;
				}
			}
			atbm_os_mutexUnLock(&hw_priv->sleep_mutex);
#endif
			atbm_sdio_host_enable_irq(1); //Enable irq at first rxframe?
		}
	}
}

static int atbm_sdio_tx_bh(void *arg){
#define WSM_SDIO_TX_MULT_BLOCK_SIZE	(ATBM_SDIO_BLOCK_SIZE)
#define ATBM_SDIO_FREE_BUFF_ERR(condition,free,prev_free,xmiteds,hw_xmiteds)	\
	do{ 																		\
		if(condition)	{																\
			wifi_printk(WIFI_DBG_ERROR, "%s[%d]:free(%x),prev_free(%x),xmiteds(%x),hw_xmiteds(%x)\n",__func__,__LINE__,free,prev_free,xmiteds,hw_xmiteds);	\
			ATBM_BUG_ON(1);			\
		}\
	}while(0)

	struct atbmwifi_common *hw_priv = (struct atbmwifi_common *)arg;
	int status = 0;
	int dotx=0;
	while(!hw_priv->bh_term){
		status = atbm_os_wait_event_timeout(&hw_priv->tx_bh_wq, 100*HZ);
		//wifi_printk(WIFI_ALWAYS, "wake up to tx!!!\n");
		dotx = atbm_atomic_xchg(&hw_priv->bh_tx, 0);
		//wifi_printk(WIFI_ALWAYS, "dotx:%d-%d\n", dotx, atbm_atomic_read(&hw_priv->bh_tx));
		if(dotx){
#ifdef ATBM_TX_SKB_NO_TXCONFIRM
			static atbm_uint8 loop = 1;
#else
			static atbm_uint8 loop = 0;
#endif
			int tx_burst;
			struct wsm_hdr_tx *wsm_tx;
			int vif_selected;
			atbm_uint32 tx_len=0;
			atbm_uint32 putLen=0;
			atbm_uint8 *data;
			atbm_uint8 *need_confirm = NULL;
			int ret=0;
			int txMutiFrameCount=0;
#if (PROJ_TYPE>=ARES_A)
			atbm_uint32 wsm_flag_u32 = 0;
			atbm_uint16 wsm_len_u16[2];
			atbm_uint16 wsm_len_sum;
#endif	//	(PROJ_TYPE==ARES_A)	
			ATBM_BOOL enough = ATBM_FALSE;

			hw_priv->tx_inprogress = 1;
#if SUPPORT_LIGHT_SLEEP
			atbm_os_mutexLock(&hw_priv->sleep_mutex, 0);
			if (hw_priv->device_can_sleep) {
				wifi_printk(WIFI_DBG_MSG, "Exit light sleep!!!\n");
				ret = atbm_device_wakeup(hw_priv);
				if (ATBM_WARN_ON(ret < 0)){
					atbm_os_mutexUnLock(&hw_priv->sleep_mutex);
					hw_priv->tx_inprogress = 0;
					continue;
				}
				else if (ret){
					hw_priv->device_can_sleep = ATBM_FALSE;
				}
			}
			atbm_os_mutexUnLock(&hw_priv->sleep_mutex);
#endif
xmit_continue:
			txMutiFrameCount = 0;
			putLen = 0;
			enough = ATBM_FALSE;
			do {
				enough = atbm_sdio_have_enough_space(hw_priv,1);
				if(enough == ATBM_FALSE){
					if(txMutiFrameCount > 0)
						break;
					else
						goto xmit_wait;
				}
				ret = wsm_get_tx(hw_priv, &data, &tx_len, &tx_burst,&vif_selected);
				
				if (ret <= 0) {
					if(txMutiFrameCount > 0)
						break;
					else
						goto xmit_finished;
				}
				need_confirm = ATBM_NULL;
				txMutiFrameCount++;
				wsm_tx = (struct wsm_hdr_tx *)data;
				ATBM_BUG_ON(tx_len < sizeof(*wsm_tx));
				ATBM_BUG_ON(__atbm_le32_to_cpu(wsm_tx->len) != tx_len);
				
#if (PROJ_TYPE>=ARES_A)
				wsm_flag_u32 = (tx_len) & 0xffff;
				wsm_len_u16[0] = wsm_flag_u32 & 0xff;
				wsm_len_u16[1] = (wsm_flag_u32 >> 8)& 0xff;
				wsm_len_sum = wsm_len_u16[0] + wsm_len_u16[1];
				if (wsm_len_sum & BIT(8)){
					wsm_flag_u32 |= ((wsm_len_sum + 1) & 0xff) << 24;
				}else {
					wsm_flag_u32 |= (wsm_len_sum & 0xff) << 24;
				}
				wsm_tx->flag=__atbm_le32_to_cpu(wsm_flag_u32);
#endif //(PROJ_TYPE==ARES_A)
				if (tx_len <= 8)
					tx_len = 16;
		
				if (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) ) {
					tx_len -= (tx_len % (WSM_SDIO_TX_MULT_BLOCK_SIZE) );
					tx_len += WSM_SDIO_TX_MULT_BLOCK_SIZE;
				}
		
				/* Check if not exceeding atbm
				capabilities */
				if (ATBM_WARN_ON(tx_len > EFFECTIVE_BUF_SIZE)) {
					wifi_printk(WIFI_DBG_ERROR, "Write aligned len:"
					" %d\n", tx_len);
				}
#if (PROJ_TYPE<ARES_A)					
				wsm_tx->flag=(((tx_len/ATBM_SDIO_BLOCK_SIZE)&0xff)-1);
#endif //(PROJ_TYPE==ARES_A)
				wsm_tx->id &= __atbm_le32_to_cpu(~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
				wsm_tx->id |= atbm_cpu_to_le32(WSM_TX_SEQ(hw_priv->wsm_tx_seq));
#if ATBM_WSM_SDIO_TX_MULT
				wsm_tx->tx_len = tx_len;
				wsm_tx->tx_id = wsm_tx->id;
#endif
				wsm_alloc_tx_buffer(hw_priv);
				atbm_spin_lock_bh(&hw_priv->tx_com_lock);
				ATBM_SDIO_FREE_BUFF_ERR(hw_priv->hw_bufs_free <= 0,hw_priv->hw_bufs_free,hw_priv->hw_bufs_free_init,hw_priv->n_xmits,hw_priv->hw_xmits);
				hw_priv->n_xmits ++;
				hw_priv->hw_bufs_free --;		
				ATBM_SDIO_FREE_BUFF_ERR(hw_priv->hw_bufs_free < 0,hw_priv->hw_bufs_free,hw_priv->hw_bufs_free_init,hw_priv->n_xmits,hw_priv->hw_xmits);
				atbm_spin_unlock_bh(&hw_priv->tx_com_lock);

				atbm_memcpy(&hw_priv->xmit_buff[putLen], data, wsm_tx->len);
				putLen += tx_len;
				hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;
		
				if (vif_selected != -1) {
					hw_priv->hw_bufs_used_vif[vif_selected]++;
				}

				if(wsm_txed(hw_priv, data)){
					need_confirm = data;
					wifi_printk(WIFI_DBG_MSG, "%s:cmd free(%d),used(%d)\n",__func__,hw_priv->hw_bufs_free,hw_priv->hw_bufs_used);
					break;
				}else {
					hw_priv->wsm_txframe_num++;
#if ATBM_TX_SKB_NO_TXCONFIRM
					if(atbm_sdio_free_tx_wsm(hw_priv,(struct wsm_tx *)data) == 0){
						need_confirm = data;
						wifi_printk(WIFI_DBG_MSG, "%s:confirm free(%d),used(%d)\n",__func__,hw_priv->hw_bufs_free,hw_priv->hw_bufs_used);
						break;
					}
#else
					need_confirm = data;
#endif
				}
				
				if (putLen+hw_priv->wsm_caps.sizeInpChBuf>SDIO_TX_MAXLEN){
					break;
				}
			}while(loop);
			ATBM_BUG_ON(putLen == 0);
			hw_priv->buf_id_offset = txMutiFrameCount;
			//atbm_atomic_add(1, &hw_priv->bh_tx);
		
			if(atbm_bh_is_term(hw_priv)){
				wifi_printk(WIFI_DBG_ERROR, "%s:bh term\n",__func__);
				atbm_sdio_force_free_wsm(hw_priv,(struct wsm_tx *)need_confirm);
				goto xmit_continue;
			}

			//dump_mem(hw_priv->xmit_buff, putLen);
			if (ATBM_WARN_ON(atbm_data_write(hw_priv,hw_priv->xmit_buff, putLen))) { 	
				wifi_printk(WIFI_DBG_ERROR, "%s: xmit data err\n",__func__);
				goto xmit_err;
			}
xmit_wait:	
			if((enough == ATBM_FALSE)&&(atbm_sdio_wait_enough_space(hw_priv,1) == ATBM_FALSE)){
				wifi_printk(WIFI_DBG_ERROR, "%s: wait space timeout\n",__func__);
				goto xmit_err;
			}
			goto xmit_continue;

xmit_finished:
#if SUPPORT_LIGHT_SLEEP
			atbm_os_mutexLock(&hw_priv->sleep_mutex, 0);
			hw_priv->tx_inprogress = 0;
			if(!(atbm_atomic_read(&hw_priv->bh_rx) || hw_priv->rx_inprogress || need_confirm)){
				if(!hw_priv->device_can_sleep)
				{
					wifi_printk(WIFI_DBG_MSG, "tx Enter light sleep!!\n");
					ATBM_WARN_ON(atbm_reg_write_16(hw_priv, 			ATBM_HIFREG_CONTROL_REG_ID, 0));
					hw_priv->device_can_sleep = ATBM_TRUE;
				}
			}
			atbm_os_mutexUnLock(&hw_priv->sleep_mutex);
#endif
			continue;
xmit_err:
			atbm_sdio_force_free_wsm(hw_priv,(struct wsm_tx *)need_confirm);
			//atbm_bh_halt(hw_priv);
			goto xmit_continue;
		}
	}
	return 0;
}
#endif

int atbm_register_bh(struct atbmwifi_common *hw_priv)
{
#if ATBM_TXRX_IN_ONE_THREAD
	atbm_thread_internal_t *bh_thread;
#else
	atbm_thread_internal_t *tx_bh_thread;
	atbm_thread_internal_t *rx_bh_thread;
#endif
	/*Create RxData Task*/
	wifi_printk(WIFI_ALWAYS,"atbm_register_bh++++\n");
	atbm_atomic_set(&hw_priv->bh_rx, 0);
	atbm_atomic_set(&hw_priv->bh_tx, 0);
	hw_priv->bh_term=0;
	hw_priv->bh_error=0;
	hw_priv->syncChanl_done=1;
#if ATBM_TXRX_IN_ONE_THREAD
	atbm_atomic_set(&hw_priv->wait_timer_start, 0);
	atbm_os_init_waitevent(&hw_priv->bh_wq);
#else
	atbm_os_init_waitevent(&hw_priv->tx_bh_wq);
	atbm_os_init_waitevent(&hw_priv->rx_bh_wq);
#endif
	atbm_os_init_waitevent(&hw_priv->wsm_cmd_wq);

	hw_priv->xmit_buff = atbm_kzalloc(SDIO_TX_MAXLEN, GFP_KERNEL);
	if(hw_priv->xmit_buff == NULL){
		return -1;
	}

	#if ATBM_SDIO_READ_ENHANCE
	atbm_os_mutexLockInit(&hw_priv->sdio_rx_process_lock);
	#endif
#if SUPPORT_LIGHT_SLEEP
	atbm_os_mutexLockInit(&hw_priv->sleep_mutex);
#endif

#if	ATBM_TXRX_IN_ONE_THREAD
	bh_thread=atbm_createThreadInternal("sdio_bh",atbm_sdio_bh,(atbm_void*)hw_priv,BH_TASK_PRIO);
	if (!bh_thread){
		wifi_printk(WIFI_ALWAYS,"bh_thread Failed\n");
		return -1;
	}
	hw_priv->bh_thread = bh_thread;
#else
	tx_bh_thread=atbm_createThreadInternal("atbm_tx",atbm_sdio_tx_bh,(atbm_void*)hw_priv,TX_BH_TASK_PRIO);
	if (!tx_bh_thread){
		wifi_printk(WIFI_ALWAYS,"bh_thread Failed\n");
		return -1;
	}
	hw_priv->tx_bh_thread = tx_bh_thread;

	rx_bh_thread=atbm_createThreadInternal("atbm_rx",atbm_sdio_rx_bh,(atbm_void*)hw_priv,RX_BH_TASK_PRIO);
	if (!rx_bh_thread){
		wifi_printk(WIFI_ALWAYS,"bh_thread Failed\n");
		return -1;
	}
	hw_priv->rx_bh_thread = rx_bh_thread;
#endif
	wifi_printk(WIFI_ALWAYS,"atbm_register_bh---\n");
	return 0;
}

atbm_void atbm_unregister_bh(struct atbmwifi_common *hw_priv)
{
	//kill createThread
	hw_priv->bh_term=1;
#if ATBM_TXRX_IN_ONE_THREAD
	atbm_os_wakeup_event(&hw_priv->bh_wq);
	atbm_stopThread(hw_priv->bh_thread);
	atbm_os_delete_waitevent(&hw_priv->bh_wq);
#else
	atbm_os_wakeup_event(&hw_priv->tx_bh_wq);
	atbm_os_wakeup_event(&hw_priv->rx_bh_wq);
	atbm_stopThread(hw_priv->tx_bh_thread);
	atbm_stopThread(hw_priv->rx_bh_thread);
	atbm_os_delete_waitevent(&hw_priv->tx_bh_wq);
	atbm_os_delete_waitevent(&hw_priv->rx_bh_wq);
#endif
	atbm_os_delete_waitevent(&hw_priv->wsm_cmd_wq);
	atbm_os_DeleteMutex(&hw_priv->wsm_cmd_mux);
#if ATBM_SDIO_READ_ENHANCE
	atbm_os_DeleteMutex(&hw_priv->sdio_rx_process_lock);
#endif
#if SUPPORT_LIGHT_SLEEP
	atbm_os_DeleteMutex(&hw_priv->sleep_mutex);
#endif

	if(hw_priv->xmit_buff){
		atbm_kfree(hw_priv->xmit_buff);
	}
}

#if ATBM_SDIO_READ_ENHANCE
void atbm_irq_handler(struct atbmwifi_common *hw_priv)
{
	reed_ctrl_handler_t read_ctrl_func;
	reed_data_handler_t read_data_func;
	
	read_ctrl_func = atbm_bh_read_ctrl_reg;
	read_data_func = atbm_data_read;
	
	wifi_printk(WIFI_BH,"[BH] irq.\n");
	/* To force the device to be always-on, the host sets WLAN_UP to 1 */
	if(!hw_priv->init_done){
		wifi_printk(WIFI_ALWAYS,"[BH] irq. init_done =0 drop\n");
		return;
	}
	if (/* ATBM_WARN_ON */(hw_priv->bh_error))
		return;
	
	if(!atbm_os_mutextryLock(&hw_priv->sdio_rx_process_lock)){
		if (atbm_sdio_process_read_data(hw_priv,read_ctrl_func,read_data_func) != 0){
			goto rx_err;
		}
	}
	atbm_os_mutexUnLock(&hw_priv->sdio_rx_process_lock);
	return;
rx_err:
	atbm_os_mutexUnLock(&hw_priv->sdio_rx_process_lock);
	if (atbm_bh_is_term(hw_priv) || atbm_atomic_read(&hw_priv->bh_term)){
		hw_priv->bh_error = 1;
		atbm_os_wakeup_event(&hw_priv->bh_wq);
	}
	return;
}
#else
void atbm_irq_handler(struct atbmwifi_common *hw_priv)
{

	/* To force the device to be always-on, the host sets WLAN_UP to 1 */
	if(!hw_priv->init_done){
		wifi_printk(WIFI_ALWAYS,"[BH] irq. init_done =0 drop\n");
		return;
	}
	if (/* ATBM_WARN_ON */(hw_priv->bh_error))
		return;
	if (atbm_atomic_add_return(1, &hw_priv->bh_rx) == 1){
#if ATBM_TXRX_IN_ONE_THREAD
		atbm_os_wakeup_event(&hw_priv->bh_wq);
#else
		atbm_os_wakeup_event(&hw_priv->rx_bh_wq);
#endif
	}
		
}
#endif
