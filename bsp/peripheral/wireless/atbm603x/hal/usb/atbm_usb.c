/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "../../include/svn_version.h"
#define DPLL_CLOCK 40

//extern atbm_int32 atbm_usb_submit_urb(atbm_urb_s *purb, int param);
extern atbm_void atbm_usb_kill_urb(atbm_urb_s *purb);
extern atbm_void atbm_usb_free_urb(atbm_urb_s *purb);
//extern atbm_urb_s *atbm_usb_alloc_urb(atbm_int32 iso_packets, atbm_int32 mem_flags);
extern atbm_void atbm_core_release(struct atbmwifi_common *hw_priv);
extern int atbm_usb_register_init(atbm_void);
extern int atbm_usb_register_deinit(atbm_void);
static int atbm_usb_memcpy_fromio_async(struct sbus_priv *self,
				     unsigned int addr,
				     atbm_void *dst, int count,sbus_callback_handler func);

extern void frame_hexdump(char *prefix, atbm_uint8 *data, atbm_uint8 len);

#if CONFIG_USB_AGGR_URB_TX
void atbm_usb_free_txDMABuf_all(struct sbus_priv *self,atbm_uint8 * buffer,int cnt);
#endif

struct build_info{
	int ver;
	int dpll;
	char driver_info[64];
};
const char DRIVER_INFO[]={"[===USB-ATHENAB=="};
static int driver_build_info(atbm_void)
{
	struct build_info build;
	build.ver=SVN_VERSION;
	build.dpll=DPLL_CLOCK;
	atbm_memcpy(build.driver_info,(atbm_void*)DRIVER_INFO,sizeof(DRIVER_INFO));
	wifi_printk(WIFI_DBG_ANY,"SVN_VER=%d,DPLL_CLOCK=%d,BUILD_TIME=%s\n",build.ver,build.dpll,build.driver_info);
	return 0;
}
/* sbus_ops implemetation */
int atbm_usbctrl_vendorreq_sync(struct sbus_priv *self, atbm_uint8 request,atbm_uint8 b_write,
					atbm_uint16 value, atbm_uint16 index, atbm_void *pdata,atbm_uint16 len)
{
	unsigned int pipe;
	int status;
	atbm_uint8 reqtype;
	int vendorreq_times = 0;
	struct atbm_usb_device *udev = self->drvobj->pusbdev;
	
	static int count;
	atbm_uint8 * reqdata=self->usb_req_data;
	if (!reqdata){
		wifi_printk(WIFI_DBG_ERROR,"regdata is Null\n");
	}
	if(len > ATBM_USB_EP0_MAX_SIZE){
		wifi_printk(WIFI_DBG_ERROR,"usbctrl_vendorreq request 0x%x, b_write %d! len:%d >%d too long \n",
		       request, b_write, len,ATBM_USB_EP0_MAX_SIZE);
		return -1;
	}
	if(b_write){
		pipe = atbm_usb_sndctrlpipe(udev, /*0*/atbm_usb_sndctrlpipe(udev, 0)); /* write_out */
		reqtype =  ATBM_USB_VENQT_WRITE;//host to device
		// reqdata must used dma data
		atbm_memcpy(reqdata,pdata,len);
	}
	else {
		pipe = atbm_usb_rcvctrlpipe(udev, /*0*/atbm_usb_rcvctrlpipe(udev, 0)); /* read_in */
		reqtype =  ATBM_USB_VENQT_READ;//device to host
	}
	do {
		status = atbm_usb_control_msg(udev, pipe, request, reqtype, value,
						 index, reqdata, len, 500); /*500 ms. timeout*/
		if (status < 0) {
			wifi_printk(WIFI_DBG_ERROR, "%s:err(%d)addr[%x] len[%d],b_write %d request %d\n",__FUNCTION__,status,value|(index<<16),len,b_write, request);
		} else if(status != len) {
			wifi_printk(WIFI_DBG_ERROR,"%s:len err(%d)\n",__FUNCTION__,status);
		}
		else{
			break;
		}
	} while (++vendorreq_times < 3);

	if((b_write==0) && (status>0)){
		atbm_memcpy(pdata,reqdata,len);
	}
	if (status < 0 && count++ < 4)
		wifi_printk(WIFI_DBG_ERROR,"reg 0x%x, usbctrl_vendorreq TimeOut! status:0x%x value=0x%x\n",
		       value, status, *(atbm_uint32 *)pdata);
	return status;
}
#define HW_DOWN_FW

#ifdef HW_DOWN_FW
static int atbm_usb_hw_read_port(struct sbus_priv *self, atbm_uint32 addr, atbm_void *pdata,int len)
{
	int ret = 0;
	atbm_uint8 request = VENDOR_HW_READ; //HW
	atbm_uint16 wvalue = (atbm_uint16)(addr & 0x0000ffff);
	atbm_uint16 index = addr >> 16;

	wifi_printk(WIFI_ALWAYS,"atbm_usb_hw_read_port addr %x,len %x\n",addr,len);

	//hardware just support len=4
	ATBM_WARN_ON_FUNC((len != 4) && (request== VENDOR_HW_READ));
	ret = atbm_usbctrl_vendorreq_sync(self,request,0,wvalue, index, pdata,len);
	if (ret < 0)
	{
		wifi_printk(WIFI_DBG_ERROR, "ERR read addr %x,len %x\n",addr,len);
	}
	return ret;
}

static int atbm_usb_hw_write_port(struct sbus_priv *self, atbm_uint32 addr, const atbm_void *pdata,int len)
{

	atbm_uint8 request = VENDOR_HW_WRITE; //HW
	atbm_uint16 wvalue = (atbm_uint16)(addr & 0x0000ffff);
	atbm_uint16 index = addr >> 16;
	int ret =0;

	atbm_usb_pm(self,0);

	//printk(KERN_ERR "%s:addr(%x)\n",__func__,addr);
	//hardware just support len=4
	//ATBM_WARN_ON((len != 4) && (request== VENDOR_HW_WRITE));
	ret =  atbm_usbctrl_vendorreq_sync(self,request,1,wvalue, index, (atbm_void *)pdata,len);
	if (ret < 0)
	{
		wifi_printk(WIFI_DBG_ERROR, "ERR write addr %x,len %x\n",addr,len);
	}
	atbm_usb_pm(self,1);
	return ret;
}


#else
static int atbm_usb_sw_read_port(struct sbus_priv *self, atbm_uint32 addr, atbm_void *pdata,int len)
{
	atbm_uint8 request = VENDOR_SW_READ; //SW
	atbm_uint16 wvalue = (atbm_uint16)(addr & 0x0000ffff);
	atbm_uint16 index = addr >> 16;

	ATBM_WARN_ON_FUNC(len > ATBM_USB_EP0_MAX_SIZE);
	return atbm_usbctrl_vendorreq_sync(self,request,0,wvalue, index, pdata,len);
}
//#ifndef HW_DOWN_FW
static int atbm_usb_sw_write_port(struct sbus_priv *self, atbm_uint32 addr,const atbm_void *pdata,int len)
{
	atbm_uint8 request = VENDOR_SW_WRITE; //SW
	atbm_uint16 wvalue = (atbm_uint16)(addr & 0x0000ffff);
	atbm_uint16 index = addr >> 16;

	ATBM_WARN_ON_FUNC(len > ATBM_USB_EP0_MAX_SIZE);
	return atbm_usbctrl_vendorreq_sync(self,request,1,wvalue, index, (atbm_void *)pdata,len);
}
#endif

int atbm_lmac_start(struct sbus_priv *self)
{
	//pTargetUsb->loadFirmwareSuccess;
	//atbm_uint8 request = VENDOR_SW_CPU_JUMP;
	//static int tmpdata =0;
	//return atbm_usbctrl_vendorreq_sync(self,request,1,0, 0, &tmpdata,0);
	//TargetUsb_lmac_start();
	return 1;
}

int atbm_usb_ep0_cmd(struct sbus_priv *self)
{
	atbm_uint8 request = VENDOR_EP0_CMD; //SW
	
	static int tmpdata =0;
	return atbm_usbctrl_vendorreq_sync(self,request,1,0, 0, &tmpdata,0);
}
/*
wvalue=1 : open uart debug;
 wvalue=0 : close uart debug;
 */
int atbm_usb_debug_config(struct sbus_priv *self,atbm_uint16 wvalue)
{
	atbm_uint8 request = 0x6;
	atbm_uint16 index = 0;

	wifi_printk(WIFI_DBG_ERROR, "atbm_usb_debug_config\n");

	return atbm_usbctrl_vendorreq_sync(self,request,1,wvalue, index, &wvalue,0);
}
#if (USE_MAIL_BOX==0)
int tx_cmp_cnt=0;
int tx_cnt=0;
atbm_void atbm_urb_coml(struct atbmwifi_common *hw_priv){
	unsigned long flags=0;
    int ret = 0;
	if(atbm_bh_is_term(hw_priv))
	{
		wifi_printk(WIFI_ALWAYS,"atbm_urb_coml Error\n");
		return;
	}
	atbm_spin_lock_irqsave(&hw_priv->tx_com_lock,&flags);
	while (!atbm_list_empty(&hw_priv->tx_urb_cmp)) {
	    struct sbus_urb *urb_cmp = atbm_list_first_entry(&hw_priv->tx_urb_cmp, struct sbus_urb, list);
		atbm_list_del(&urb_cmp->list);
		atbm_spin_unlock_irqrestore(&hw_priv->tx_com_lock,flags);
		struct sbus_priv *self = urb_cmp->obj;
		struct atbmwifi_common	*hw_priv	= self->core;
		if(urb_cmp->data!=ATBM_NULL){
			struct wsm_tx *tx = (struct wsm_tx *)urb_cmp->data;
			atbm_uint8 queue_id;
			struct atbmwifi_queue *queue=ATBM_NULL; 
			if(!(tx->htTxParameters & atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM))){
				queue_id = atbmwifi_queue_get_queue_id(tx->packetID);
				queue = &hw_priv->tx_queue[queue_id];
				if(queue_id>=4){
					frame_hexdump("Error",(atbm_uint8*)tx,sizeof(*tx));
				}
				ATBM_BUG_ON(queue_id>=4);
				ret=atbmwifi_queue_remove(queue,tx->packetID);
					if(ret){
					wifi_printk(WIFI_DBG_ERROR,">>>>RET=%x\n",ret);
				}
				wsm_release_tx_buffer(hw_priv, 1);
			}
		}
		atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_cmp->urb_id);
		urb_cmp->link =0;
		atbm_spin_lock_irqsave(&hw_priv->tx_com_lock,&flags);
	}
	atbm_spin_unlock_irqrestore(&hw_priv->tx_com_lock,flags);
	atbm_bh_schedule_tx(hw_priv);
}
#else
atbm_void atbm_release_tx_buffer(atbm_void *data)
{
	struct atbmwifi_common	*hw_priv	= (struct atbmwifi_common*)data;
    int ret = 0;
	unsigned long flags;
    atbm_urb_s *atbm_urb = ATBM_NULL;
	while(1){
		atbm_urb=atbm_mailBox_recv(OS_WAIT_FOREVER);
		//dump_mem(atbm_urb->context,64);
		if(atbm_urb!=ATBM_NULL){
			if(atbm_urb->status!=0){
				wifi_printk(WIFI_ALWAYS,"TX URB ERR\n");
			}
			struct sbus_urb *tx_urb=(struct sbus_urb*)(atbm_urb->context);
			struct sbus_priv *self = tx_urb->obj;
			if(tx_urb->data!=ATBM_NULL){
				struct wsm_tx *tx = (struct wsm_tx *)tx_urb->data;
				atbm_uint8 queue_id;
				struct atbmwifi_queue *queue=ATBM_NULL; 
				if(!(tx->htTxParameters & atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM))){
					queue_id = atbmwifi_queue_get_queue_id(tx->packetID);
					queue = &hw_priv->tx_queue[queue_id];
					ATBM_BUG_ON(queue_id>=4);
					ret=atbmwifi_queue_remove(queue,tx->packetID);
					if(ret){
						wifi_printk(WIFI_DBG_ERROR,">>>>RET=%x\n",ret);
					}
					wsm_release_tx_buffer(hw_priv, 1);
				}
			}
			tx_urb->link =0;
			atbm_usb_urb_put(self,self->drvobj->tx_urb_map,tx_urb->urb_id);
			atbm_bh_schedule_tx(hw_priv);
			//wifi_printk(WIFI_ALWAYS,"atbm_release_tx_buffer >>end\n");
		}
	}
}
#endif
static atbm_void atbm_usb_xmit_data_complete(atbm_urb_s *atbm_urb)
{
	struct sbus_urb *urb_cmp=(struct sbus_urb*)(atbm_urb->context);
	struct sbus_priv *self = urb_cmp->obj;
	struct atbmwifi_common	*hw_priv	= self->core;
	unsigned long flags;
	int ret = 0;

	switch(atbm_urb->status){
		case 0:
			break;
		default:
			wifi_printk(WIFI_ALWAYS,"WARNING> status %d\n",atbm_urb->status);
			break;
	}

#if CONFIG_USB_AGGR_URB_TX
	atbm_usb_free_txDMABuf_all(self,urb_cmp->pallocated_buf,urb_cmp->dma_buff_alloced);
#endif

#if ATBM_TX_SKB_NO_TXCONFIRM
#if USE_MAIL_BOX
	/*Rtt-thread  need use mailbox to release txbuffer*/
	if(atbm_mailBox_send(atbm_urb)!=0){
		wifi_printk(WIFI_ALWAYS,"Sent mailbox full\n");
	}
#else
#if ATBM_DIRECT_TX
	{
		//atbm_spin_unlock_irqrestore(&hw_priv->tx_com_lock,flags);
		if(urb_cmp->data!=ATBM_NULL){
			struct wsm_tx *tx = (struct wsm_tx *)urb_cmp->data;
			atbm_uint8 queue_id;
			struct atbmwifi_queue *queue=ATBM_NULL;
			if(!(tx->htTxParameters & atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM))){
				queue_id = atbmwifi_queue_get_queue_id(tx->packetID);
				queue = &hw_priv->tx_queue[queue_id];
				ATBM_BUG_ON(queue_id>=4);
				if(atbmwifi_queue_remove(queue,tx->packetID)){
					wifi_printk(WIFI_DBG_ERROR,">>>>error\n");
				}
				wsm_release_tx_buffer(hw_priv, 1);
			}
		}
		//atbm_spin_lock_irqsave(&hw_priv->tx_com_lock,flags);
		atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_cmp->urb_id);
		urb_cmp->link =0;
		atbm_bh_schedule_tx(hw_priv);
	}
#else
	{
		/*Add tx urb to list tail*/
		atbm_spin_lock_irqsave(&hw_priv->tx_com_lock,&flags);
		atbm_list_add_tail(&urb_cmp->list,&hw_priv->tx_urb_cmp);
		atbm_spin_unlock_irqrestore(&hw_priv->tx_com_lock,flags);
		if (atbm_atomic_add_return(1, &hw_priv->urb_comp) == 1){
			atbm_os_wakeup_event(&hw_priv->bh_wq);
		}
	}
#endif
#endif
#endif
	return ;
}

void atbm_usb_free_err_cmd(struct sbus_priv *self)
{
	struct atbmwifi_common	*hw_priv = self->core;

	atbm_spin_lock_bh(&hw_priv->wsm_cmd.lock);
	hw_priv->wsm_cmd.ret = -1;
	hw_priv->wsm_cmd.done = 1;
	hw_priv->wsm_cmd.cmd = 0xFFFF;
	atbm_spin_unlock_bh(&hw_priv->wsm_cmd.lock);
	wifi_printk(WIFI_DBG_ERROR, "%s:release wsm_cmd.lock\n",__func__);
	atbm_os_wakeup_event(&hw_priv->wsm_cmd_wq);		
}


void atbm_usb_free_err_data(struct sbus_priv *self,struct sbus_urb *tx_urb)
{
	struct atbmwifi_common	*hw_priv = self->core;
	struct wsm_tx *wsm = (struct wsm_tx *)tx_urb->data;	
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

	if(!ATBM_WARN_ON(atbmwifi_queue_get_skb(queue, wsm->packetID, &skb, &txpriv))) {

		struct atbmwifi_ieee80211_tx_info *tx = ATBM_IEEE80211_SKB_TXCB(skb);
		//int tx_count = 0;
		int i;
//		wsm_release_vif_tx_buffer_Nolock(hw_priv,txpriv->if_id,1);
		tx->flags |= ATBM_IEEE80211_TX_STAT_ACK;
		tx->control.rates[0].count = 1;
		for (i = 1; i < ATBM_IEEE80211_TX_MAX_RATES; ++i) {
			tx->control.rates[i].count = 0;
			tx->control.rates[i].idx = -1;
		}
#ifdef CONFIG_ATBM_APOLLO_TESTMODE
		atbmwifi_queue_remove(hw_priv, queue, wsm->packetID);
#else
		atbmwifi_queue_remove(queue, wsm->packetID);
#endif
	}else {
//		wsm_release_vif_tx_buffer_Nolock(hw_priv,atbm_queue_get_if_id(wsm->packetID),1);
	}
}


#if CONFIG_USB_AGGR_URB_TX
char * atbm_usb_pick_txDMABuf(struct sbus_priv *self)
{
	unsigned long flags=0;
	char * buf;
	atbm_spin_lock_irqsave(&self->lock, &flags);
	if(self->drvobj->NextAllocPost == self->drvobj->NextFreePost){
		if(self->drvobj->tx_dma_addr_buffer_full){
			wifi_printk(WIFI_ALWAYS, "atbm_usb_pick_txDMABuf:tx_dma_addr_buffer_full %d \n",self->drvobj->tx_dma_addr_buffer_full);
			atbm_spin_unlock_irqrestore(&self->lock, flags);
			return NULL;
		}
	}
	buf = self->drvobj->NextAllocPost;
	atbm_spin_unlock_irqrestore(&self->lock, flags);
	return buf;
}

char * atbm_usb_get_txDMABuf(struct sbus_priv *self)
{
	char * buf;
	unsigned long flags=0;
	atbm_spin_lock_irqsave(&self->lock, &flags);
	if(self->drvobj->NextAllocPost == self->drvobj->NextFreePost){
		if(self->drvobj->tx_dma_addr_buffer_full){
			atbm_spin_unlock_irqrestore(&self->lock, flags);
			return NULL;
		}
	}
	self->drvobj->free_dma_buffer_cnt--;
	if(self->drvobj->free_dma_buffer_cnt <0){
		self->drvobj->free_dma_buffer_cnt=0;
		wifi_printk(WIFI_ALWAYS, "free_dma_buffer_cnt ERR,NextAllocPost %p drvobj->NextFreePost %p\n",self->drvobj->NextAllocPost, self->drvobj->NextFreePost);
		
		ATBM_BUG_ON(1);
	}
	
	buf = self->drvobj->NextAllocPost;
	self->drvobj->NextAllocPost += BUFF_ALLOC_LEN;
	if(self->drvobj->NextAllocPost >= self->drvobj->tx_dma_addr_buffer_end){
		self->drvobj->NextAllocPost = self->drvobj->tx_dma_addr_buffer;
	}
	if(self->drvobj->NextAllocPost == self->drvobj->NextFreePost){
		self->drvobj->tx_dma_addr_buffer_full =1;
	}
	atbm_spin_unlock_irqrestore(&self->lock, flags);
	return buf;
}


void atbm_usb_free_txDMABuf(struct sbus_priv *self)
{
	if(self->drvobj->NextAllocPost == self->drvobj->NextFreePost){
		if(self->drvobj->tx_dma_addr_buffer_full==0){
			wifi_printk(WIFI_ALWAYS, "self->drvobj->free_dma_buffer_cnt %d\n",self->drvobj->free_dma_buffer_cnt);
			ATBM_BUG_ON(self->drvobj->tx_dma_addr_buffer_full==0);
			return;
		}
		self->drvobj->tx_dma_addr_buffer_full = 0;
	}
	else {
		
	}

	self->drvobj->free_dma_buffer_cnt++;
	if(self->drvobj->total_dma_buffer_cnt < self->drvobj->free_dma_buffer_cnt){
		wifi_printk(WIFI_ALWAYS, "<WARNING> atbm_usb_free_txDMABuf (buffer(%p)  NextFreePost(%p))free_dma_buffer_cnt %d \n",
			self->drvobj->NextAllocPost, 
			self->drvobj->NextFreePost,
			self->drvobj->free_dma_buffer_cnt);
		
		ATBM_BUG_ON(1);
		
	}
	self->drvobj->NextFreePost += BUFF_ALLOC_LEN;
	if(self->drvobj->NextFreePost >= self->drvobj->tx_dma_addr_buffer_end){
		self->drvobj->NextFreePost = self->drvobj->tx_dma_addr_buffer;
	}
}

void atbm_usb_init_txDMABuf(struct sbus_priv *self,struct sbus_urb * pUrb,int max_num, int len)
{
	int i=0;

	self->drvobj->tx_dma_addr_buffer_len=len*max_num*URB_AGGR_NUM;
	self->drvobj->tx_dma_addr_buffer = atbm_kmalloc(self->drvobj->tx_dma_addr_buffer_len, GFP_KERNEL);
	self->drvobj->NextAllocPost = self->drvobj->tx_dma_addr_buffer ;
	self->drvobj->NextFreePost = self->drvobj->tx_dma_addr_buffer;
	self->drvobj->tx_dma_addr_buffer_end = self->drvobj->tx_dma_addr_buffer+self->drvobj->tx_dma_addr_buffer_len;
	self->drvobj->tx_dma_addr_buffer_full = 0;
	self->drvobj->free_dma_buffer_cnt= self->drvobj->tx_dma_addr_buffer_len/BUFF_ALLOC_LEN;
	self->drvobj->total_dma_buffer_cnt= self->drvobj->tx_dma_addr_buffer_len/BUFF_ALLOC_LEN;
	wifi_printk(WIFI_ALWAYS, "CONFIG_USB_AGGR_URB_TX enable cnt tx_dma_addr_buffer_end(%p)tx_dma_addr_buffer(%p),%d\n",
		self->drvobj->tx_dma_addr_buffer,self->drvobj->tx_dma_addr_buffer_end,(int)self->drvobj->tx_dma_addr_buffer_len/BUFF_ALLOC_LEN);
	for(i=0;i<max_num;i++){
#if 0
		pUrb[i].pallocated_buf = ATBM_NULL;
		pUrb[i].pallocated_buf_len = 0;
#endif
		pUrb[i].pallocated_buf = self->drvobj->tx_dma_addr_buffer + BUFF_ALLOC_LEN * i;
		pUrb[i].pallocated_buf_len = BUFF_ALLOC_LEN;
	}
}


void atbm_usb_free_txDMABuf_all(struct sbus_priv *self,atbm_uint8 * buffer,int cnt)
{
	unsigned long flags=0;
	atbm_spin_lock_irqsave(&self->lock, &flags);
	ATBM_WARN_ON(cnt==0);
	if((char *)buffer != self->drvobj->NextFreePost){			
		wifi_printk(WIFI_ALWAYS, "<WARNING> atbm_usb_free_txDMABuf_all (buffer(%p) != NextFreePost(%p))free_dma_buffer_cnt %d cnt %d\n",buffer, self->drvobj->NextFreePost,self->drvobj->free_dma_buffer_cnt,cnt);
		ATBM_BUG_ON(1);
		self->drvobj->NextFreePost = buffer;
	}
	while(cnt--){
		atbm_usb_free_txDMABuf(self);
	}
	atbm_spin_unlock_irqrestore(&self->lock, flags);
}

int atbm_usb_free_tx_wsm(struct sbus_priv *self,struct sbus_urb *tx_urb)
{
	struct wsm_tx *wsm = NULL;

	wsm = tx_urb->data; 
	if((wsm) && (!(wsm->htTxParameters&atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM)))){
		
		struct atbmwifi_queue *queue;
		atbm_uint8 queue_id;
		struct atbmwifi_common	*hw_priv = self->core;
		struct atbm_buff *skb;
		const struct atbmwifi_txpriv *txpriv;

		queue_id = atbmwifi_queue_get_queue_id(wsm->packetID);

		ATBM_BUG_ON(queue_id >= 4);

		queue = &hw_priv->tx_queue[queue_id];
		ATBM_BUG_ON(queue == NULL);

		if(!ATBM_WARN_ON(atbmwifi_queue_get_skb(queue, wsm->packetID, &skb, &txpriv))) {

			struct atbmwifi_ieee80211_tx_info *tx = ATBM_IEEE80211_SKB_TXCB(skb);
			//int tx_count = 0;
			int i;

//			wsm_release_vif_tx_buffer_Nolock(hw_priv,txpriv->if_id,1);
			wsm_release_tx_buffer_nolock(hw_priv, 1);
			
			tx->flags |= ATBM_IEEE80211_TX_STAT_ACK;
			tx->control.rates[0].count = 1;
			for (i = 1; i < ATBM_IEEE80211_TX_MAX_RATES; ++i) {
				tx->control.rates[i].count = 0;
				tx->control.rates[i].idx = -1;
			}

#ifdef CONFIG_ATBM_APOLLO_TESTMODE
			atbmwifi_queue_remove(hw_priv, queue, wsm->packetID);
#else
			atbmwifi_queue_remove(queue, wsm->packetID);
#endif
		}else {
//			wsm_release_vif_tx_buffer_Nolock(hw_priv,atbm_queue_get_if_id(wsm->packetID),1);
			wsm_release_tx_buffer_nolock(hw_priv, 1);
		}
		tx_urb->data = NULL;
		return 1;
	}
	return 0;
}

int atbm_usb_xmit_data(struct sbus_priv *self)
{
	unsigned int pipe;
	int status=0;
	int tx_burst=0;
	int vif_selected;
	struct wsm_hdr_tx *wsm;
	struct atbmwifi_common *hw_priv=self->core;
	atbm_void *txdata =ATBM_NULL;
	atbm_uint8 *data =ATBM_NULL;
	int tx_len=0;
	int ret = 0;
	int urb_id =-1;	
	struct sbus_urb *tx_urb = ATBM_NULL;
	unsigned long flags;
	atbm_uint8 *txdmabuff = ATBM_NULL;
	atbm_uint8 *usb_aggr_buff = ATBM_NULL;
	//If Usb disconnecting, stop tx usb packet to host
	if(self->suspend){
		return -2;
	}

	wifi_printk(WIFI_IF, "atbm_usb_xmit_data++\n");
	atbm_spin_lock_irqsave(&hw_priv->tx_com_lock,&flags);
	urb_id = atbm_usb_urb_get(self,self->drvobj->tx_urb_map,TX_URB_NUM);
	if(urb_id<0){
		wifi_printk(WIFI_DBG_ERROR, "atbm_usb_xmit_data:urb_id<0\n");
		status=-4;
		goto error;
	}
	tx_urb = &self->drvobj->tx_urb[urb_id];
	tx_urb->pallocated_buf_len = 0;
	tx_urb->dma_buff_alloced = 0;
	tx_urb->frame_cnt=0;
	tx_urb->data=NULL;
	tx_urb->pallocated_buf = atbm_usb_pick_txDMABuf(self);

	if(tx_urb->pallocated_buf==NULL)
	{
		atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_id);
		status=-7;
		goto error;
	}

	do{
		wsm_alloc_tx_buffer_nolock(hw_priv);
		ret = wsm_get_tx(hw_priv, &data, &tx_len, &tx_burst,
					&vif_selected);
		if (ret <= 0) {
			  wsm_release_tx_buffer_nolock(hw_priv, 1);
//			  atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_id);
			  wifi_printk(WIFI_IF,"tx:atbm_usb_urb_put ATBM_NULL %d\n",urb_id);
			  status=-3;
			  break;
		} 

		if(usb_aggr_buff == NULL){
			usb_aggr_buff= atbm_usb_get_txDMABuf(self);
			ATBM_BUG_ON(usb_aggr_buff == NULL);
			tx_urb->dma_buff_alloced ++;
		}

		txdmabuff = usb_aggr_buff;
		tx_urb = &self->drvobj->tx_urb[urb_id];
		wsm = (struct wsm_hdr_tx *)data;
		tx_urb->data = data;
#if PROJ_TYPE<ARES_A
/*		wsm->usb_len=ATBM_ALIGN(tx_len,4);
		if((wsm->usb_len  % 512)==0)
			wsm->usb_len += 4;
		tx_len= wsm->usb_len;
		wsm->usb_id=0; */
		wsm->usb_len = PER_PACKET_LEN;
#else
		wsm->usb_len = atbm_cpu_to_le16(tx_len < 1538 ? 1538 : ATBM_ALIGN(tx_len,4));
#endif

		tx_urb->frame_cnt++; 
		   
#if (ATBM_USB_BUS==0)
		atbm_atomic_add(1, &hw_priv->bh_tx);
#endif  //ATBM_IMMD_TX
		self->tx_vif_selected =vif_selected;

		wsm->flag = (__atbm_cpu_to_le16(0xe569)<<16) | BIT(6)| (self->tx_seqnum & 0x1f);
		//wsm->id &= atbm_cpu_to_le32(~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
		wsm->id |= atbm_cpu_to_le32(WSM_TX_SEQ(hw_priv->wsm_tx_seq));
		txdata =(atbm_void*)wsm;
		self->tx_seqnum++;

//		self->tx_hwChanId++;
		hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;

		if (vif_selected != -1) {
			hw_priv->hw_bufs_used_vif[vif_selected]++;
		}

		atbm_memcpy(txdmabuff,txdata, tx_len);
//		atbm_xmit_linearize(hw_priv,(struct wsm_tx *)wsm,txdmabuff,actual_len);
		tx_urb->pallocated_buf_len += PER_PACKET_LEN;
		/*
		*if the data is cmd ,keep it last in the aggr buff
		*/

		wifi_printk(WIFI_IF, "wsm->len:%d:wsm->id %d seq %d urb_id %d\n",wsm->len, wsm->id,hw_priv->wsm_tx_seq,urb_id);
		if(wsm_txed(hw_priv, data)==0){
			hw_priv->wsm_txframe_num++;
		}
		else {
			tx_urb->data = 0;
		}
#if ATBM_TX_SKB_NO_TXCONFIRM
		//cmd or need confirm frame not agg
		if(atbm_usb_free_tx_wsm(self,tx_urb)==0)
			break;
#endif  //CONFIG_TX_NO_CONFIRMCONFIG_TX_NO_CONFIRM
		tx_urb->data = 0;

		//the last dma buffer
		usb_aggr_buff += PER_PACKET_LEN;
		if(tx_urb->frame_cnt>=(PER_BUFF_AGGR_NUM*tx_urb->dma_buff_alloced)){
			if(usb_aggr_buff >= self->drvobj->tx_dma_addr_buffer_end)
				break;
			usb_aggr_buff = atbm_usb_pick_txDMABuf(self);
			if(usb_aggr_buff == NULL)
				break;
			usb_aggr_buff = NULL;
		}
	}while(1);

	if(tx_urb->pallocated_buf_len == 0){
		atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_id);
		status= -6;
		goto error;
	}

	if(tx_urb->frame_cnt ==0){
		ATBM_WARN_ON(1);
	}

//	hw_priv->hw_bufs_used_vif[vif_selected]++;	
	pipe = atbm_usb_sndbulkpipe(self->drvobj->pusbdev, self->drvobj->ep_out);
//	self->drvobj->tx_urb->transfer_flags |= URB_ZERO_PACKET;
	atbm_usb_fill_bulk_urb(tx_urb->test_urb,
		self->drvobj->pusbdev, pipe,tx_urb->pallocated_buf,tx_urb->pallocated_buf_len,
		atbm_usb_xmit_data_complete,tx_urb);
	tx_urb->link =1;
	status = atbm_usb_submit_urb(tx_urb->test_urb, GFP_ATOMIC);

	if (status) {
		atbm_uint8 i = 0;
		int wsm_id;		
		struct wsm_tx *wsm_txd = NULL;		
		status = 1;

		tx_urb->test_urb->status = 0;
		for(i = 0;i<tx_urb->frame_cnt;i++){			
			wsm_txd = (struct wsm_tx *)(tx_urb->pallocated_buf+i*PER_PACKET_LEN);			
			wsm_id = atbm_le16_to_cpu(wsm_txd->hdr.id) & 0x3F;			
			wifi_printk(WIFI_DBG_ERROR, "%s:wsm_id(%x)\n",__func__,wsm_id);			
//			self->tx_hwChanId--;			
			hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq - 1) & WSM_TX_SEQ_MAX;			
//			if(wsm_id == WSM_FIRMWARE_CHECK_ID){
//				continue;			
//			}			
			if(wsm_id == WSM_TRANSMIT_REQ_MSG_ID){
				#if ATBM_TX_SKB_NO_TXCONFIRM	
				if(!(wsm_txd->htTxParameters&atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM))){
					continue;
				}				
				#endif
				tx_urb->data = wsm_txd;				
				atbm_usb_free_err_data(self,tx_urb);			
			}else {				
				atbm_usb_free_err_cmd(self);			
			}						
			wsm_release_tx_buffer_nolock(hw_priv, 1);	
		}		
		atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_id);		
		tx_urb->data = NULL;		
		status = 1;		
		wifi_printk(WIFI_ALWAYS, "release all data finished\n");	
		atbm_usb_free_txDMABuf_all(self,tx_urb->pallocated_buf,tx_urb->dma_buff_alloced);
		goto error;
	}

	if(status==0){
		status = tx_burst;
	}
error:
	atbm_spin_unlock_irqrestore(&hw_priv->tx_com_lock,flags);
	//atbm_atomic_set(&self->tx_lock, 0);
	return status;
}
#else

int atbm_usb_xmit_data(struct sbus_priv *self)
{
	unsigned int pipe;
	int status=0;
	int tx_burst=0;
	int vif_selected;
	struct wsm_hdr_tx *wsm;
	struct atbmwifi_common *hw_priv=self->core;
	atbm_void *txdata =ATBM_NULL;
	atbm_uint8 *data =ATBM_NULL;
	int tx_len=0;
	int ret = 0;
	int urb_id =-1;	
	struct sbus_urb *tx_urb = ATBM_NULL;
	unsigned long flags;
	//If Usb disconnecting, stop tx usb packet to host
	if(self->suspend){
		return -2;
	}
	wifi_printk(WIFI_IF, "atbm_usb_xmit_data++\n");
	atbm_spin_lock_irqsave(&hw_priv->tx_com_lock,&flags);
	urb_id = atbm_usb_urb_get(self,self->drvobj->tx_urb_map,TX_URB_NUM);
	if(urb_id<0){
		wifi_printk(WIFI_DBG_MSG, "atbm_usb_xmit_data:urb_id<0\n");
		status=-4;
		goto error;
	}
	/*if (atbm_atomic_read(&self->tx_lock)==0)*/{

		wsm_alloc_tx_buffer_nolock(hw_priv);
		ret = wsm_get_tx(hw_priv, &data, &tx_len, &tx_burst,
					&vif_selected);
		if (ret <= 0) {
			  wsm_release_tx_buffer_nolock(hw_priv, 1);
			  atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_id);
			  wifi_printk(WIFI_IF,"tx:atbm_usb_urb_put ATBM_NULL %d\n",urb_id);
			  status=-3;
			  goto error;
		} else {
			tx_urb = &self->drvobj->tx_urb[urb_id];
			wsm = (struct wsm_hdr_tx *)data;
			tx_urb->data = data;
			#if 0
			wsm->usb_len=ATBM_ALIGN(tx_len,4);
			if((wsm->usb_len  % 512)==0)
				wsm->usb_len += 4;
			tx_len= wsm->usb_len;
			wsm->usb_id=0;
			#else
			wsm->usb_len=tx_len;
			if(wsm->usb_len<1538){
				wsm->usb_len =1538;
			}
			tx_len= wsm->usb_len;
			wsm->usb_id=0;


			
		   #endif
		   
		   
#if (ATBM_USB_BUS==0)
			atbm_atomic_add(1, &hw_priv->bh_tx);
#endif  //ATBM_IMMD_TX
			self->tx_vif_selected =vif_selected;

			wsm->flag = (__atbm_cpu_to_le16(0xe569)<<16) | BIT(6)| (self->tx_seqnum & 0x1f);
			//wsm->id &= atbm_cpu_to_le32(~WSM_TX_SEQ(WSM_TX_SEQ_MAX));
			wsm->id |= atbm_cpu_to_le32(WSM_TX_SEQ(hw_priv->wsm_tx_seq));
			txdata =(atbm_void*)wsm;
	
			//wifi_printk(WIFI_ALWAYS, "wsm->id:wsm->id %d seq %d urb_id %d\n",wsm->id,hw_priv->wsm_tx_seq,urb_id);
			if(wsm_txed(hw_priv, data)==0){
                struct wsm_tx *tx = (struct wsm_tx *)tx_urb->data;
				hw_priv->wsm_txframe_num++;
				//add by wp, 
				//this is to fix bug , because in sometime atbm_urb_coml is call later than txconfirm
				//when need txconfirm frame , txconfirm will free skb, but atbm_urb_coml need skb
                if(tx->htTxParameters & atbm_cpu_to_le32(WSM_HT_TX_NEED_CONFIRM)){
                    tx_urb->data = ATBM_NULL;
                }
			}
			else {
				tx_urb->data = ATBM_NULL;
			}		
			hw_priv->hw_bufs_used_vif[vif_selected]++;	
			pipe = atbm_usb_sndbulkpipe(self->drvobj->pusbdev, self->drvobj->ep_out);
			///self->drvobj->tx_urb->transfer_flags |= URB_ZERO_PACKET;
			atbm_usb_fill_bulk_urb(tx_urb->test_urb,
				self->drvobj->pusbdev, pipe,txdata,tx_len,
				atbm_usb_xmit_data_complete,tx_urb);
			tx_urb->link =1;
			status = atbm_usb_submit_urb(tx_urb->test_urb, GFP_ATOMIC);
			if (status) {
				status = -5;
				hw_priv->hw_bufs_used_vif[vif_selected]--;
				if (vif_selected != -1) {
					atbm_usb_free_err_data(self,tx_urb);
				}else {
					atbm_usb_free_err_cmd(self);
				}
				wsm_release_tx_buffer_nolock(hw_priv, 1);
				atbm_usb_urb_put(self,self->drvobj->tx_urb_map,urb_id);
				wifi_printk(WIFI_DBG_ANY," <ERROR>tx:atbm_usb_urb_put %d\n",urb_id);
				//msleep(1000);
				goto error;
			}
			self->tx_seqnum++;
			wifi_printk(WIFI_IF, "tx_seq %d\n",self->tx_seqnum);
			hw_priv->wsm_tx_seq = (hw_priv->wsm_tx_seq + 1) & WSM_TX_SEQ_MAX;
		}
	}
	if(status==0){
		status = tx_burst;
	}
error:
	atbm_spin_unlock_irqrestore(&hw_priv->tx_com_lock,flags);
	//atbm_atomic_set(&self->tx_lock, 0);
	return status;
}
#endif

atbm_void atbm_usb_receive_data_cancel(struct sbus_priv *self)
{
	wifi_printk(WIFI_IF,"&&&fuc=%s\n",__FUNCTION__);
	//usb_kill_urb(self->drvobj->rx_urb);
	atbm_usb_pm(self,1);
}

void atbm_usb_kill_all_txurb(struct sbus_priv *self)
{
	int i=0;
	for(i=0;i<TX_URB_NUM;i++){
		atbm_usb_kill_urb(self->drvobj->tx_urb[i].test_urb);
	}

}
void atbm_usb_kill_all_rxurb(struct sbus_priv *self)
{
	int i=0;
	for(i=0;i<RX_URB_NUM;i++){
		atbm_usb_kill_urb(self->drvobj->rx_urb[i].test_urb);
	}

}

atbm_void atbm_usb_urb_free(struct sbus_priv *self,struct sbus_urb * pUrb,int max_num)
{
	int i=0;
#if CONFIG_USB_AGGR_URB_TX
	if(self->drvobj->tx_dma_addr_buffer){
		atbm_kfree(self->drvobj->tx_dma_addr_buffer);
		self->drvobj->tx_dma_addr_buffer = ATBM_NULL;
	}
#endif //CONFIG_USB_AGGR_URB_TX
	for(i=0;i<max_num;i++){
		if(pUrb[i].link ==1){
			atbm_usb_kill_urb(pUrb[i].test_urb);
		}
		atbm_usb_free_urb(pUrb[i].test_urb);
		atbm_dev_kfree_skb(pUrb[i].test_skb);
		pUrb[i].test_skb =ATBM_NULL;
		pUrb[i].test_urb =ATBM_NULL;
	}
}
int atbm_usb_urb_malloc(struct sbus_priv *self,struct sbus_urb * pUrb,int max_num,int len)
{
	int i=0;
	for(i=0;i<max_num;i++){
		pUrb[i].test_urb=atbm_usb_alloc_urb(0,0);
		if (!pUrb[i].test_urb){
			wifi_printk(WIFI_DBG_ERROR, "Can't allocate test_urb.");
			goto __free_urb;
		}
		pUrb[i].test_skb =atbm_dev_alloc_skb(len);
		if (!pUrb[i].test_skb){
			wifi_printk(WIFI_DBG_ERROR, "Can't allocate test_skb.");
			goto __free_skb;
		}
		pUrb[i].urb_id = i;
		pUrb[i].obj =self;
		
		pUrb[i].link =0;
	}
	return 0;
__free_skb:
	for( ;i>=0;--i){
		atbm_dev_kfree_skb(pUrb[i].test_skb);
	}
	i = max_num;
__free_urb:
	for( ;i>=0;--i){
		atbm_usb_free_urb(pUrb[i].test_urb);
	}

	return -ATBM_ENOMEM;
}


//atbm_uint8 bitmap_zero[4] = {
//	0,1,0,255
//};

#ifndef LINUX_OS
atbm_uint8 bitmap_zero[16] = {
	0,1,0,2,0,1,0,3,
	0,1,0,2,0,1,0,255
};
#endif

int atbm_usb_urb_get(struct sbus_priv *self,atbm_uint32 *bitmap,int max_urb)
{
	int id = 0;
	unsigned long flags=0;
	
#if 0
	atbm_spin_lock_irqsave(&self->lock, &flags);
	if(*bitmap >= 0xf){
		atbm_spin_unlock_irqrestore(&self->lock, flags);
		return -1;
	}
	id= bitmap_zero[*bitmap];
	*bitmap |= BIT(id);
	atbm_spin_unlock_irqrestore(&self->lock, flags);
#else
	atbm_spin_lock_irqsave(&self->lock, &flags);
	id= atbm_find_first_zero_bit(bitmap,max_urb);
	if((id>=max_urb)||(id<0)){
		atbm_spin_unlock_irqrestore(&self->lock, flags);
		return -1;
	}
	atbm_set_bit(id,bitmap);   
	atbm_spin_unlock_irqrestore(&self->lock, flags);
#endif //0
	return id;
}
atbm_void  atbm_usb_urb_put(struct sbus_priv *self,atbm_uint32 *bitmap,int id)
{
	unsigned long flags=0;
	atbm_spin_lock_irqsave(&self->lock, &flags);
	*bitmap &= ~BIT(id);
	atbm_spin_unlock_irqrestore(&self->lock, flags);
}



atbm_void  atbm_usb_receive_data_complete( atbm_urb_s *atbm_urb)
{
	struct sbus_urb *rx_urb=(struct sbus_urb*)atbm_urb->context;
	struct sbus_priv *self = rx_urb->obj;
	struct atbm_buff *skb=rx_urb->test_skb;
	struct atbmwifi_common *hw_priv=self->core;
	int RecvLength=atbm_urb->actual_length;
	struct wsm_hdr *wsm;
	unsigned long flags;

	wifi_printk(WIFI_IF, "rxendd  Len %d urb_id %d\n",RecvLength,rx_urb->urb_id);

	if(!hw_priv)
		goto __free;

	rx_urb->link =0;

	switch(atbm_urb->status){
		case 0:
			break;
		default:
			wifi_printk(WIFI_DBG_ANY, "atbm_usb_rx_complete2 error status=%d len %d\n",atbm_urb->status,RecvLength);
			goto __free;
	}
#if USB_SUSPEND_SUPPORT
	if((self->drvobj->suspend_skb_len != 0)&&(self->drvobj->suspend_skb != ATBM_NULL))
	{
		if(atbm_skb_tailroom(self->drvobj->suspend_skb)<self->drvobj->suspend_skb_len+RecvLength)
		{
			struct atbm_buff * long_suspend_skb = ATBM_NULL;
			ATBM_BUG_ON(self->drvobj->suspend_skb_len+RecvLength>RX_BUFFER_SIZE);
			long_suspend_skb = atbm_dev_alloc_skb(RX_BUFFER_SIZE+64);
			ATBM_BUG_ON(!long_suspend_skb);
			atbm_skb_reserve(long_suspend_skb, 64);
			atbm_memcpy((atbm_uint8 *)ATBM_OS_SKB_DATA(long_suspend_skb),ATBM_OS_SKB_DATA(self->drvobj->suspend_skb),self->drvobj->suspend_skb_len);
			atbm_dev_kfree_skb(self->drvobj->suspend_skb);
			self->drvobj->suspend_skb = long_suspend_skb;
		}
		atbm_memcpy((atbm_uint8 *) ATBM_OS_SKB_DATA(self->drvobj->suspend_skb) + self->drvobj->suspend_skb_len,ATBM_OS_SKB_DATA(skb),RecvLength);
		RecvLength += self->drvobj->suspend_skb_len;
		atbm_memcpy(ATBM_OS_SKB_DATA(skb),(atbm_uint8 *)ATBM_OS_SKB_DATA(self->drvobj->suspend_skb),RecvLength);
		self->drvobj->suspend_skb_len = 0;					
	}
	
	wsm = (struct wsm_hdr *)ATBM_OS_SKB_DATA(skb);
	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_usb_receive_data_complete()--len=%d, id=%d\n",wsm->len, wsm->id);
	if (wsm->len != RecvLength){
		if(((wsm->len  % 512)==0) && ((wsm->len+1) == RecvLength)){
			//this correct , lmac output len = (wsm len +1 ) ,inorder to let hmac usb callback
		}
		else {
			wifi_printk(WIFI_IF,"rx rebulid usbsuspend  id %d wsm->len %d,RecvLength %d\n",wsm->id,wsm->len,RecvLength);

			if(self->drvobj->suspend_skb_len== 0){ 
				if(wsm->len > RX_BUFFER_SIZE){
					wifi_printk(WIFI_DBG_ERROR," %s %d id %d wsm->len %d,RecvLength %d\n",__FUNCTION__,__LINE__,wsm->id,wsm->len,RecvLength);
					goto resubmit;
				}
				wifi_printk(WIFI_DBG_ERROR, "rx rebulid usbsuspend0	\n");
				/*
				* alloc 4K buff for suspend_skb is save
				*/
				ATBM_BUG_ON(self->drvobj->suspend_skb == ATBM_NULL);
				atbm_memcpy((atbm_uint8 *)ATBM_OS_SKB_DATA(self->drvobj->suspend_skb),ATBM_OS_SKB_DATA(skb),RecvLength);
				self->drvobj->suspend_skb_len = RecvLength;
				goto resubmit;
			}
		}
	}
	
	ATBM_WARN_ON_FUNC(self->drvobj->suspend_skb_len != 0);
#endif  //USB_SUSPEND_SUPPORT

	if (ATBM_WARN_ON(4 > RecvLength)){
		wifi_printk(WIFI_DBG_ERROR,"%s %d id %d wsm->len %d,RecvLength %d\n",__FUNCTION__,__LINE__,wsm->id,wsm->len,RecvLength);
		//frame_hexdump("atbm_usb_receive_data_complete",(atbm_uint8 *)wsm,32);
		goto resubmit;
		//goto __free;
	}

	ATBM_BUG_ON(RecvLength > RX_BUFFER_SIZE);
	self->rx_seqnum++;

	//atbm_spin_lock_irqsave(&self->lock, &flags);
#if ATBM_IMMD_RX
	{
		rx_urb->test_skb = ATBM_NULL;
		atbm_usb_urb_put(self,self->drvobj->rx_urb_map,rx_urb->urb_id);
#if HI_RX_MUTIL_FRAME
#define RX_ALLOC_BUFF_OFFLOAD (  (36+16)/*RX_DESC_OVERHEAD*/+4/*FCS_LEN*/ -16 /*WSM_HI_RX_IND*/)
		struct wsm_hdr *wsm;
		atbm_uint32 wsm_len, wsm_id, data_len;
		struct atbm_buff *skb_copy;

		wsm = (struct wsm_hdr *)ATBM_OS_SKB_DATA(skb);
		wsm_len = __atbm_le32_to_cpu(wsm->len);
		wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
		//wifi_printk(WIFI_DBG_ERROR, "%s rxdata %x\n",__func__,wsm_id);

		if(wsm_id == WSM_MULTI_RECEIVE_INDICATION_ID){
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
				ATBM_BUG_ON((wsm_id  & ~WSM_TX_LINK_ID(WSM_TX_LINK_ID_MAX)) !=	WSM_RECEIVE_INDICATION_ID);
				skb_copy = atbm_dev_alloc_skb(wsm_len + 16);
				/* In AP mode RXed SKB can be looped back as a broadcast.
				 * Here we reserve enough space for headers. */
				atbm_skb_reserve(skb_copy,	(8 - (((unsigned long)ATBM_OS_SKB_DATA(skb_copy))&7))/*ATBM_ALIGN 8*/);
				
				atbm_memmove(ATBM_OS_SKB_DATA(skb_copy), wsm, wsm_len);
				atbm_skb_put(skb_copy,wsm_len);
				atbm_rx_bh_cb(hw_priv,skb_copy);
				data_len -= ATBM_ALIGN(wsm_len + RX_ALLOC_BUFF_OFFLOAD,4);
				RxFrameNum--;

				wsm = (struct wsm_hdr *)((atbm_uint8 *)wsm +ATBM_ALIGN(( wsm_len + RX_ALLOC_BUFF_OFFLOAD),4));
				wsm_len = __atbm_le32_to_cpu(wsm->len);
				wsm_id	= __atbm_le32_to_cpu(wsm->id) & 0xFFF;
				
			}while((RxFrameNum>0) && (data_len > 32));
			ATBM_BUG_ON(RxFrameNum != 0);
			/*atbm transmit packet to device*/
			hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
			hw_priv->sbus_ops->sbus_read_async(hw_priv->sbus_priv,0x2,skb,RX_BUFFER_SIZE);
			hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
		}
		else
#endif  //HI_RX_MUTIL_FRAME
		{
			atbm_rx_bh_cb(hw_priv,skb);
			/*atbm transmit packet to device*/			
			hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
			hw_priv->sbus_ops->sbus_read_async(hw_priv->sbus_priv,0x2,ATBM_NULL,RX_BUFFER_SIZE);
			hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
		}

		return;
	}
#endif //ATBM_IMMD_RX

	rx_urb->test_skb = ATBM_NULL;	
	ATBM_OS_SKB_LEN(skb) = RecvLength;

	atbm_usb_urb_put(self,self->drvobj->rx_urb_map,rx_urb->urb_id);

#if RX_QUEUE_IMMD
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	if(hw_priv->sbus_priv->rx_queue_qnum <= USB_MAX_RX_QUEUE_NUM){
		hw_priv->sbus_priv->rx_queue_qnum++;
		atbm_usb_memcpy_fromio_async(hw_priv->sbus_priv,0x2,ATBM_NULL,RX_BUFFER_SIZE,ATBM_NULL);
		skb->Type = RX_SKB__RX_QUEUE_PACKAGE;
	}else{
		//wifi_printk(WIFI_ALWAYS, "rx queue full!!\n");
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
#endif

	atbm_skb_queue_tail(&hw_priv->rx_frame_queue, skb);

	atbm_atomic_set(&hw_priv->bh_rx, 1);
	wifi_printk(WIFI_BH,"atbm_bh_wakeup rxend\n");
	atbm_os_wakeup_event(&hw_priv->bh_wq);

	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_usb_receive_data_complete() 5.\n");

	if(!hw_priv->init_done){
		wifi_printk(WIFI_DBG_ERROR, "[BH] irq. init_done =0 drop\n");
		goto __free;
	}
	if (/* ATBM_WARN_ON */(hw_priv->bh_error))
		goto __free;

	wifi_printk(WIFI_DBG_MSG,"atbm: atbm_usb_receive_data_complete() 6.\n");
	return;
	
__free:
	if(self->drvobj->suspend_skb_len != 0){
		wifi_printk(WIFI_DBG_ERROR,"rx rebulid usbsuspend3	rx drop\n");
	}
	//atbm_usb_urb_put(self,self->drvobj->rx_urb_map,rx_urb->urb_id);
	wifi_printk(WIFI_DBG_ERROR, "[WARNING] atbm_usb_receive_data drop\n");

	return;

resubmit:
	if(!hw_priv->init_done){
		wifi_printk(WIFI_DBG_ERROR, "[BH] irq. init_done =0 drop\n");
		goto __free;
	}
	if (/* ATBM_WARN_ON */(hw_priv->bh_error))
		goto __free;

	atbm_usb_urb_put(self,self->drvobj->rx_urb_map,rx_urb->urb_id);
	atbm_usb_memcpy_fromio_async(hw_priv->sbus_priv,0x2,ATBM_NULL,RX_BUFFER_SIZE,ATBM_NULL);
	wifi_printk(WIFI_DBG_ANY, "atbm_usb_receive_data resubmit\n");
	return;

}

static int atbm_usb_receive_data(struct sbus_priv *self,unsigned int addr,atbm_void  *dst, int count)
{
	unsigned int pipe;
	int status=0;
	struct atbm_buff *skb;
	struct sbus_urb *rx_urb;
	int urb_id;
	struct atbmwifi_common *hw_priv=self->core;
	unsigned long flags;
		//If Usb disconnecting, stop rx usb packet from host
	if(self->suspend){
		return -2;
	}
	urb_id = atbm_usb_urb_get(self,self->drvobj->rx_urb_map,RX_URB_NUM);
	if(urb_id<0){
		status=-4;
		goto __err_rx;
	}
	rx_urb = &self->drvobj->rx_urb[urb_id];
	//if not rxdata complete
	//initial new rxdata
	if(rx_urb->test_skb == ATBM_NULL){
		if(dst ){
			rx_urb->test_skb=dst;
			atbm_skb_trim(rx_urb->test_skb,0);
		}
		else {
			skb=atbm_dev_alloc_skb(count);
			if (!skb){
				status=-1;
				wifi_printk(WIFI_DBG_ERROR,"atbm_usb_receive_data++ atbm_dev_alloc_skb %p ERROR\n",skb);
				atbm_atomic_set(&self->rx_lock, 0);

				goto __err_skb;
			}
			rx_urb->test_skb=skb;
		}
	}
	else {
		if(dst ){
			atbm_dev_kfree_skb(dst);
		}
	}
	//atbm_spin_lock_irqsave(&hw_priv->rx_com_lock,&flags);
	skb = rx_urb->test_skb;
	pipe = atbm_usb_rcvbulkpipe(self->drvobj->pusbdev, self->drvobj->ep_in);
	atbm_usb_fill_bulk_urb(rx_urb->test_urb, self->drvobj->pusbdev, pipe,ATBM_OS_SKB_DATA(skb),count,atbm_usb_receive_data_complete,rx_urb);
	rx_urb->link =1;
	//atbm_spin_unlock_irqrestore(&hw_priv->rx_com_lock,flags);
	status = atbm_usb_submit_urb(rx_urb->test_urb, 0);
	//atbm_spin_unlock_irqrestore(&hw_priv->rx_com_lock,flags);
	wifi_printk(WIFI_IF, "atbm_usb_submit_urb rx urb_id %d\n",urb_id);

	if (status) {
		status = -2;
		wifi_printk(WIFI_DBG_ERROR,"receive_data atbm_usb_submit_urb ++ ERR %d\n",status);
		goto __err_skb;
	}
__err_skb:
	if(status < 0){
		atbm_usb_urb_put(self,self->drvobj->rx_urb_map,urb_id);
	}
__err_rx:
	return status;
}


static int atbm_usb_memcpy_fromio(struct sbus_priv *self,
				     unsigned int addr,
				     atbm_void *dst, int count)
{
	int i=0;
	if(atbm_atomic_add_return(0, &self->rx_lock)){
		return -1;
	}
	for(i=0;i<RX_URB_NUM;i++){
	 	atbm_usb_receive_data(self,addr,dst,count);
	}
	return 0;
}

static int atbm_usb_memcpy_toio(struct sbus_priv *self,
				   unsigned int addr,
				   const atbm_void  *src, int count)
{
	return atbm_usb_xmit_data(self);
}

static int atbm_usb_memcpy_fromio_async(struct sbus_priv *self,
				     unsigned int addr,
				     atbm_void  *dst, int count,sbus_callback_handler func)
{

	 return atbm_usb_receive_data(self,addr,dst,count);
}

static int atbm_usb_memcpy_toio_async(struct sbus_priv *self,
				   unsigned int addr,
				   const atbm_void *src, int count)
{
	 int ret =  0;
	 atbm_usb_xmit_data(self);

	 return ret;
}

static atbm_void atbm_usb_lock(struct sbus_priv *self)
{
	//atbm_os_mutexLock(&self->sbus_mutex,0);

}

static atbm_void atbm_usb_unlock(struct sbus_priv *self)
{
	//atbm_os_mutexUnLock(&self->sbus_mutex);
}


static int atbm_usb_reset(struct sbus_priv *self)
{
	atbm_uint32 regdata = 1;
	wifi_printk(WIFI_IF," %s\n",__FUNCTION__);
	atbm_usb_hw_write_port(self,0x16100074,&regdata,4);
	return 0;
}

static atbm_uint32 atbm_usb_align_size(struct sbus_priv *self, atbm_uint32 size)
{
	atbm_size_t aligned = size;
	return aligned;
}

int atbm_usb_set_block_size(struct sbus_priv *self, atbm_uint32 size)
{
	return 0;
}

int atbm_usb_pm(struct sbus_priv *self, ATBM_BOOL  auto_suspend)
{
	int ret = 0;
	self->auto_suspend  = auto_suspend;

	if(auto_suspend){
	///TODO suspend
	}
	return ret;
}
int atbm_usb_pm_async(struct sbus_priv *self, ATBM_BOOL  auto_suspend)
{
	int ret = 0;

	self->auto_suspend  = auto_suspend;

	if(auto_suspend){
		///TODO suspend
	}
	else {
		///TODO suspend
	}
	return ret;
}
static int atbm_usb_irq_subscribe(struct sbus_priv *self, sbus_irq_handler handler,atbm_void *priv)
{
	int ret = 0;
	return ret;
}
static int atbm_usb_irq_unsubscribe(struct sbus_priv *self)
{
	int ret = 0;
	return ret;
}
static struct dvobj_priv *usb_dvobj_init(struct atbm_usb_interface *usb_intf)
{
	//int	i;
	struct dvobj_priv *pdvobjpriv=ATBM_NULL;
	//struct atbm_usb_device_descriptor 	*pdev_desc;
	//struct atbm_usb_host_config			*phost_conf;
	//struct atbm_usb_config_descriptor		*pconf_desc;
	//struct atbm_usb_host_interface		*phost_iface;
	//struct atbm_usb_interface_descriptor	*piface_desc;
	//struct atbm_usb_host_endpoint		*phost_endp;
	//struct atbm_usb_endpoint_descriptor	*pendp_desc;
//	struct atbm_usb_device				*pusbd;
	
	pdvobjpriv = atbm_kzalloc(sizeof(*pdvobjpriv),GFP_KERNEL);
	if (!pdvobjpriv){
		wifi_printk(WIFI_IF, "Can't allocate USB dvobj.");
		goto exit;
	}
	pdvobjpriv->pusbintf = usb_intf ;
	pdvobjpriv->pusbdev = atbm_interface_to_usbdev(usb_intf);
	atbm_usb_set_intfdata(usb_intf, pdvobjpriv);
	
#if 0

	//pdvobjpriv->RtNumInPipes = 0;
	//pdvobjpriv->RtNumOutPipes = 0;
	phost_iface = &usb_intf->altsetting[0];
	piface_desc = &phost_iface->desc;
	pdev_desc = &pusbd->descriptor;
	wifi_printk(WIFI_IF,"\natbm_usb_device_descriptor:\n");
	wifi_printk(WIFI_IF,"bLength=%x\n", pdev_desc->bLength);
	wifi_printk(WIFI_IF,"bDescriptorType=%x\n", pdev_desc->bDescriptorType);
	wifi_printk(WIFI_IF,"bcdUSB=%x\n", pdev_desc->bcdUSB);
	wifi_printk(WIFI_IF,"bDeviceClass=%x\n", pdev_desc->bDeviceClass);
	wifi_printk(WIFI_IF,"bDeviceSubClass=%x\n", pdev_desc->bDeviceSubClass);
	wifi_printk(WIFI_IF,"bDeviceProtocol=%x\n", pdev_desc->bDeviceProtocol);
	wifi_printk(WIFI_IF,"bMaxPacketSize0=%x\n", pdev_desc->bMaxPacketSize0);
	wifi_printk(WIFI_IF,"idVendor=%x\n", pdev_desc->idVendor);
	wifi_printk(WIFI_IF,"idProduct=%x\n", pdev_desc->idProduct);
	wifi_printk(WIFI_IF,"bcdDevice=%x\n", pdev_desc->bcdDevice);
	wifi_printk(WIFI_IF,"iManufacturer=%x\n", pdev_desc->iManufacturer);
	wifi_printk(WIFI_IF,"iProduct=%x\n", pdev_desc->iProduct);
	wifi_printk(WIFI_IF,"iSerialNumber=%x\n", pdev_desc->iSerialNumber);
	wifi_printk(WIFI_IF,"bNumConfigurations=%x\n", pdev_desc->bNumConfigurations);

	phost_conf = pusbd->actconfig;
	pconf_desc = &phost_conf->desc;
	wifi_printk(WIFI_IF,"\natbm_usb_configuration_descriptor:\n");
	wifi_printk(WIFI_IF,"bLength=%x\n", pconf_desc->bLength);
	wifi_printk(WIFI_IF,"bDescriptorType=%x\n", pconf_desc->bDescriptorType);
	wifi_printk(WIFI_IF,"wTotalLength=%x\n", pconf_desc->wTotalLength);
	wifi_printk(WIFI_IF,"bNumInterfaces=%x\n", pconf_desc->bNumInterfaces);
	wifi_printk(WIFI_IF,"bConfigurationValue=%x\n", pconf_desc->bConfigurationValue);
	wifi_printk(WIFI_IF,"iConfiguration=%x\n", pconf_desc->iConfiguration);
	wifi_printk(WIFI_IF,"bmAttributes=%x\n", pconf_desc->bmAttributes);
	wifi_printk(WIFI_IF,"bMaxPower=%x\n", pconf_desc->bMaxPower);

	phost_iface = &usb_intf->altsetting[0];
	wifi_printk(WIFI_IF,"\natbm_usb_interface_descriptor:\n");
	wifi_printk(WIFI_IF,"bLength=%x\n", piface_desc->bLength);
	wifi_printk(WIFI_IF,"bDescriptorType=%x\n", piface_desc->bDescriptorType);
	wifi_printk(WIFI_IF,"bInterfaceNumber=%x\n", piface_desc->bInterfaceNumber);
	wifi_printk(WIFI_IF,"bAlternateSetting=%x\n", piface_desc->bAlternateSetting);
	wifi_printk(WIFI_IF,"bNumEndpoints=%x\n", piface_desc->bNumEndpoints);
	wifi_printk(WIFI_IF,"bInterfaceClass=%x\n", piface_desc->bInterfaceClass);
	wifi_printk(WIFI_IF,"bInterfaceSubClass=%x\n", piface_desc->bInterfaceSubClass);
	wifi_printk(WIFI_IF,"bInterfaceProtocol=%x\n", piface_desc->bInterfaceProtocol);
	wifi_printk(WIFI_IF,"iInterface=%x\n", piface_desc->iInterface);

	//pdvobjpriv->NumInterfaces = pconf_desc->bNumInterfaces;
	//pdvobjpriv->InterfaceNumber = piface_desc->bInterfaceNumber;
	pdvobjpriv->nr_endpoint = piface_desc->bNumEndpoints;


	for (i = 0; i < pdvobjpriv->nr_endpoint; i++)
	{
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp)
		{
			pendp_desc = &phost_endp->desc;

			wifi_printk(WIFI_IF,"\nusb_endpoint_descriptor(%d):\n", i);
			wifi_printk(WIFI_IF,"bLength=%x\n",pendp_desc->bLength);
			wifi_printk(WIFI_IF,"bDescriptorType=%x\n",pendp_desc->bDescriptorType);
			wifi_printk(WIFI_IF,"bEndpointAddress=%x\n",pendp_desc->bEndpointAddress);
			wifi_printk(WIFI_IF,"wMaxPacketSize=%d\n",atbm_le16_to_cpu(pendp_desc->wMaxPacketSize));
			wifi_printk(WIFI_IF,"bInterval=%x\n",pendp_desc->bInterval);

			if (atbm_usb_endpoint_is_bulk_in(pendp_desc))
			{
				wifi_printk(WIFI_IF,"usb_endpoint_is_bulk_in = %x\n",atbm_usb_endpoint_num(pendp_desc));
				pdvobjpriv->ep_in_size=atbm_le16_to_cpu(pendp_desc->wMaxPacketSize);
				pdvobjpriv->ep_in=atbm_usb_endpoint_num(pendp_desc);
			}
			else if (atbm_usb_endpoint_is_bulk_out(pendp_desc))
			{
				wifi_printk(WIFI_IF,"usb_endpoint_is_bulk_out = %x\n",atbm_usb_endpoint_num(pendp_desc));
				pdvobjpriv->ep_out_size=atbm_le16_to_cpu(pendp_desc->wMaxPacketSize);
				pdvobjpriv->ep_out=atbm_usb_endpoint_num(pendp_desc);
			}
			pdvobjpriv->ep_num[i] =atbm_usb_endpoint_num(pendp_desc);
		}
	}
	atbm_usb_get_dev(pusbd);
#endif
	pdvobjpriv->ep_in=1;
	pdvobjpriv->ep_out=2;
	wifi_printk(WIFI_IF,"nr_endpoint=%d, in_num=%d, out_num=%d\n\n", pdvobjpriv->nr_endpoint, pdvobjpriv->ep_in, pdvobjpriv->ep_out);
exit:
	return pdvobjpriv;
}

int atbm_usb_probe(struct atbm_usb_interface *intf,
				   const struct atbm_usb_device_id *id)
{
	struct sbus_priv *self;
	struct dvobj_priv *dvobj;
    struct atbm_usb_device *udev = atbm_interface_to_usbdev(intf);
	int status;

	wifi_printk(WIFI_IF, "Probe called\n");

	self = atbm_kzalloc(sizeof(*self),GFP_KERNEL);
	if (!self) {
		wifi_printk(WIFI_DBG_ERROR, "Can't allocate USB sbus_priv.");
		return -ATBM_ENOMEM;
	}
	////TODO RTOS/////////////
	atbm_os_mutexLockInit(&self->sbus_mutex);
	atbm_spin_lock_init(&self->lock);
	/* 1--- Initialize dvobj_priv */
	dvobj = usb_dvobj_init(intf);
	if (!dvobj){
		wifi_printk(WIFI_DBG_ERROR, "Can't allocate USB dvobj.");
		return -ATBM_ENOMEM;
	}
    dvobj->pusbdev = atbm_usb_get_dev(udev);
	wifi_printk(WIFI_ALWAYS,"pusbdev =0x%x",dvobj->pusbdev);
    dvobj->pusbintf = atbm_usb_get_intf(intf);
	dvobj->self =self;
	self->drvobj=dvobj;
	/*2---alloc rx_urb*/
	dvobj->suspend_skb = atbm_dev_alloc_skb(RX_BUFFER_SIZE);
	ATBM_BUG_ON(dvobj->suspend_skb == ATBM_NULL);
	//atbm_skb_reserve(self->drvobj->suspend_skb, 64);
	dvobj->suspend_skb_len = 0;
	status = atbm_usb_urb_malloc(self,dvobj->rx_urb,RX_URB_NUM,RX_BUFFER_SIZE);
	if (status != 0){
		wifi_printk(WIFI_DBG_ERROR, "Can't allocate rx_urb.");
		return status;
	}
	atbm_memset(dvobj->rx_urb_map,0,sizeof(dvobj->rx_urb_map));
	/*3---alloc tx_urb*/
	status = atbm_usb_urb_malloc(self,dvobj->tx_urb,TX_URB_NUM,TX_BUFFER_SIZE);
	if (status){
		wifi_printk(WIFI_DBG_ERROR, "Can't allocate tx_urb.");
		return -ATBM_ENOMEM;
	}
#if CONFIG_USB_AGGR_URB_TX
	atbm_usb_init_txDMABuf(self,dvobj->tx_urb,TX_URB_NUM, TX_BUFFER_SIZE);
#endif
	atbm_memset(dvobj->tx_urb_map,0,sizeof(dvobj->tx_urb_map));
	/*5---alloc rx data buffer*/
	self->usb_data = atbm_kzalloc(ATBM_USB_EP0_MAX_SIZE+16,GFP_KERNEL);
	if (!self->usb_data)
		return -ATBM_ENOMEM;
	//self->rx_skb = NULL;
	self->usb_req_data = (atbm_uint8 *)ATBM_ALIGN((unsigned long)self->usb_data,4);

	self->tx_seqnum = 0;
	self->rx_seqnum = 0;
	self->drvobj->tx_test_seq_need =0;
	self->drvobj->tx_test_hwseq_need =0;
	//self->tx_callback_handler = NULL;
	//self->rx_callback_handler = NULL;
	atbm_atomic_xchg(&self->tx_lock, 0);
	atbm_atomic_xchg(&self->rx_lock, 0);

	//usb auto-suspend init
	self->suspend=0;	
	self->auto_suspend=0;	
/////////////////////////////////////////////////////////////////////////////How to do Rtos
	//self->drvobj->pusbdev->autosuspend_disabled = 0;//autosuspend disabled by the user
	//dvobj->pusbdev->do_remote_wakeup=1;
	//dvobj->pusbintf->needs_remote_wakeup = 1;
	////////USB Device suspend interface////////////
	//device_init_wakeup(&dvobj->pusbintf->dev,1);
	//pm_runtime_set_autosuspend_delay(&dvobj->pusbdev->dev,15000);
	//atbm_INIT_WORK(&dvobj->usbSuspendWork,putUsbSuspend);
	////////////////////////////////////////////////////
	//Entry atbmwifi common wifi
	status=Atbmwifi_halEntry(self);
	if (status!=0){
		wifi_printk(WIFI_DBG_ERROR,"<ERROR> %s %d\n",__FUNCTION__,__LINE__);
		atbm_usb_urb_free(self,self->drvobj->rx_urb,RX_URB_NUM);
		atbm_usb_urb_free(self,self->drvobj->tx_urb,TX_URB_NUM);
		if(self->drvobj->suspend_skb)
		{
			atbm_dev_kfree_skb(self->drvobj->suspend_skb);
			self->drvobj->suspend_skb_len = 0;
			self->drvobj->suspend_skb = ATBM_NULL;
		}
		if(dvobj->pusbdev)
			atbm_usb_put_dev(dvobj->pusbdev);
		atbm_usb_set_intfdata(intf, ATBM_NULL);
		atbm_kfree(dvobj);
		atbm_kfree(self);
	}
	return status;
}
extern struct atbmwifi_common g_hw_prv;
extern atbm_void atbm_unregister_wpa_event(atbm_void);
//extern atbm_void  atbmwifi_netstack_deinit(atbm_void);

//extern atbm_void atbm_unregister_eloop(atbm_void);

atbm_void atbm_usb_disconnect(struct atbm_usb_interface *intf)
{
	wifi_printk(WIFI_ALWAYS,"atbm_usb_disconnect \n");
	struct dvobj_priv *dvobj = atbm_usb_get_intfdata(intf);
	struct sbus_priv *self  = ATBM_NULL;
	struct atbm_usb_device *pdev=ATBM_NULL;
	if (dvobj) {
		self  = dvobj->self;
		pdev = dvobj->pusbdev;
		if (self->core) {
			atbm_core_release(self->core);
			self->core = ATBM_NULL;
		}
		self->suspend=1;
		atbm_usb_urb_free(self,self->drvobj->rx_urb,RX_URB_NUM);
		atbm_usb_urb_free(self,self->drvobj->tx_urb,TX_URB_NUM);
		if(self->drvobj->suspend_skb)
		{
			atbm_dev_kfree_skb(self->drvobj->suspend_skb);
			self->drvobj->suspend_skb = ATBM_NULL;
			self->drvobj->suspend_skb_len = 0;
		}
		atbm_os_DeleteMutex(&self->sbus_mutex);
		if(pdev)
		{   
		    ATBM_BUG_ON((pdev!=atbm_interface_to_usbdev(intf)));
			wifi_printk(WIFI_DBG_ERROR,"we have get dev,so put it in the end\n");
			atbm_usb_put_dev(pdev);
		}
		atbm_usb_set_intfdata(intf, ATBM_NULL);
		atbm_kfree(self->usb_data);
		atbm_kfree(self);
		atbm_kfree(dvobj);	
		wifi_printk(WIFI_IF,"atbm_usb_disconnect---->oK\n");
	}
}
	
int __atbm_usb_suspend(struct sbus_priv *self)
{
	wifi_printk(WIFI_ALWAYS,"***********func=%s,line=%d\n",__func__,__LINE__);
	self->suspend=1;
	return 0;
}

int __atbm_usb_resume(struct sbus_priv *self)
{
	wifi_printk(WIFI_ALWAYS, "===----===start resume state\n");
	self->suspend=0;
	atbm_usb_memcpy_fromio(self,0,NULL,RX_BUFFER_SIZE);
	return 0;
	
}

static int atbm_usb_suspend(struct atbm_usb_interface *intf)
{
	struct dvobj_priv *dvobj = atbm_usb_get_intfdata(intf);
	wifi_printk(WIFI_ALWAYS,"***********func=%s,line=%d\n",__func__,__LINE__);
	dvobj->self->suspend=1;
	//atbm_usb_suspend_start(dvobj->self);
	//msleep(20);
	//atbm_usb_urb_kill(dvobj->self,dvobj->rx_urb,RX_URB_NUM);
	return 0;

}

static int atbm_usb_resume(struct atbm_usb_interface *intf)
{
	struct dvobj_priv *dvobj = atbm_usb_get_intfdata(intf);
	wifi_printk(WIFI_ALWAYS,"===----===start resume state\n");
	dvobj->self->suspend=0;
	atbm_usb_memcpy_fromio(dvobj->self,0,NULL,RX_BUFFER_SIZE);
	return 0;
	
}
struct sbus_ops atbm_usb_sbus_ops;

static int  atbm_usb_init(atbm_void)
{
	{
	atbm_usb_sbus_ops.sbus_memcpy_fromio	= atbm_usb_memcpy_fromio;
	atbm_usb_sbus_ops.sbus_memcpy_toio	= atbm_usb_memcpy_toio;
	atbm_usb_sbus_ops.sbus_read_sync		= atbm_usb_hw_read_port;
	atbm_usb_sbus_ops.sbus_write_sync	= atbm_usb_hw_write_port;
	atbm_usb_sbus_ops.sbus_read_async	= atbm_usb_receive_data;
	atbm_usb_sbus_ops.sbus_write_async	= atbm_usb_memcpy_toio_async;
	atbm_usb_sbus_ops.lock				= atbm_usb_lock;
	atbm_usb_sbus_ops.unlock				= atbm_usb_unlock;
	atbm_usb_sbus_ops.reset				= atbm_usb_reset;
	atbm_usb_sbus_ops.align_size			= atbm_usb_align_size;
	atbm_usb_sbus_ops.power_mgmt			= atbm_usb_pm;
	atbm_usb_sbus_ops.set_block_size		= atbm_usb_set_block_size;
	//atbm_usb_sbus_ops.wtd_wakeup			= atbm_wtd_wakeup;
	#ifdef ATBM_USB_RESET
	atbm_usb_sbus_ops.usb_reset			= atbm_usb_reset;
	#else	
	//atbm_usb_sbus_ops.usb_reset			= NULL;
	#endif
	atbm_usb_sbus_ops.bootloader_debug_config = atbm_usb_debug_config;
	atbm_usb_sbus_ops.lmac_start			=atbm_lmac_start;
	atbm_usb_sbus_ops.irq_unsubscribe	= atbm_usb_irq_unsubscribe;
	atbm_usb_sbus_ops.irq_subscribe	= atbm_usb_irq_subscribe;
	};

	driver_build_info();
	wifi_printk(WIFI_IF, "atbm_usb_register\n");

	return atbm_usb_register_init();
}
static atbm_void  atbm_usb_exit(atbm_void)
{
	atbm_usb_register_deinit();
	wifi_printk(WIFI_IF,"atbm_usb_exit:usb_deregister\n");
}
atbm_void atbm_usb_module_init(atbm_void)
{
	wifi_printk(WIFI_IF, "atbm_usb_module_init\n");
	atbm_init_firmware();
	wifi_printk(WIFI_ALWAYS,"[Wifi] Enter %s \n", __func__);
	atbm_usb_init();
	return ;
}
atbm_void atbm_usb_module_exit(atbm_void)
{
	wifi_printk(WIFI_IF,"atbm_usb_module_exit\n");
	atbm_usb_exit();
	atbm_release_firmware();
	return ;
}


