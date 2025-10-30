/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef ATBM_USB_H
#define ATBM_USB_H
#include "atbm_sbus.h"
#include "atbm_os_usb.h"
#include "atbm_os_atomic.h"
#include "atbm_os_spinlock.h"

#ifdef CONFIG_USB_AGGR_URB_TX
#define PER_PACKET_LEN 2048 //must pow 512
#define URB_AGGR_NUM 8
#define PER_BUFF_AGGR_NUM 8
#define TX_URB_NUM 4
#define RX_URB_NUM 4
#define BUFF_ALLOC_LEN (PER_BUFF_AGGR_NUM*PER_PACKET_LEN)
#define TX_BUFFER_SIZE 2048
#else //CONFIG_USB_AGGR_URB_TX
#define TX_URB_NUM 4 //must be 4.can't change
#define RX_URB_NUM 4 //must <= 4
#define TX_BUFFER_SIZE 1680
#endif //CONFIG_USB_AGGR_URB_TX
#define ATBM_USB_EP0_MAX_SIZE 64
#define ATBM_USB_EP1_MAX_RX_SIZE 512
#define ATBM_USB_EP2_MAX_TX_SIZE 512
#define ATBM_USB_VENQT_WRITE  0x40
#define ATBM_USB_VENQT_READ 0xc0


#define RX_SKB__RX_QUEUE_PACKAGE 3

/*usb vendor define type, EP0, bRequest*/
enum {
	VENDOR_HW_READ=0,
	VENDOR_HW_WRITE=1,
	VENDOR_HW_RESVER=2,
	VENDOR_SW_CPU_JUMP=3,/*cpu jump to real lmac code,after fw download*/
	VENDOR_SW_READ=4,
	VENDOR_SW_WRITE=5,	
#if (PROJ_TYPE<ARES_B)
	VENDOR_DBG_SWITCH=6,
#else
	VENDOR_HW_RESET =6,
#endif
	VENDOR_EP0_CMD=7,
};
struct sbus_urb {
	struct sbus_priv* obj;
	atbm_urb_s *test_urb;
	struct atbm_buff *test_skb;
	struct atbm_list_head list;
	void * data;
	int urb_id;
	int test_seq;
	int test_hwseq;
	int link;
#if CONFIG_USB_AGGR_URB_TX
	int dma_buff_alloced;
	int frame_cnt;
	int pallocated_buf_len;
	atbm_uint8 *pallocated_buf;
#endif //CONFIG_USB_AGGR_URB_TX
};
struct dvobj_priv{
	struct atbm_usb_device *pusbdev;
	struct atbm_usb_interface *pusbintf;
#ifdef CONFIG_USB_AGGR_URB_TX
	char * tx_dma_addr_buffer;
	atbm_uint32 tx_dma_addr_buffer_len;
	struct sbus_urb *tx_save_urb;
	int tx_save_urb_data_len;
	char * NextAllocPost;
	char * NextFreePost;
	char * tx_dma_addr_buffer_end;
	ATBM_BOOL tx_dma_addr_buffer_full;
	int  free_dma_buffer_cnt;
	int  total_dma_buffer_cnt;
#endif  //CONFIG_USB_AGGR_URB_TX
	//struct atbm_buff *rx_skb;
	struct sbus_urb rx_urb[RX_URB_NUM];
	struct sbus_urb tx_urb[TX_URB_NUM];
	//unsigned long	 rx_urb_map[BITS_TO_LONGS(RX_URB_NUM)];
	//unsigned long	 tx_urb_map[BITS_TO_LONGS(TX_URB_NUM)];
	atbm_uint32 	rx_urb_map[1];  //current used 8 URBs, 1 byte
	atbm_uint32	 	tx_urb_map[1];
	int tx_test_seq_need; //just fot test
	int tx_test_hwseq_need;//just fot test
	atbm_urb_s *cmd_urb;
	//struct usb_anchor submitted;
	struct sbus_priv *self;
	struct atbm_net_device *netdev;
	atbm_uint8	usb_speed; // 1.1, 2.0 or 3.0
	atbm_uint8	nr_endpoint;
	int ep_in;
	int ep_in_size;
	int ep_out;
	int ep_out_size;
	int	ep_num[6]; //endpoint number
	struct atbm_buff *suspend_skb;
	unsigned long suspend_skb_len;
};

struct sbus_wtd {
	int 	wtd_init;
	struct task_struct		*wtd_thread;
//	atbm_os_wait_queue_head_t		wtd_evt_wq;
	atbm_atomic_t				wtd_term;
	atbm_atomic_t				wtd_run;
	atbm_atomic_t				wtd_probe;
};
extern struct sbus_ops atbm_usb_sbus_ops ;
int atbm_usb_pm(struct sbus_priv *self, ATBM_BOOL  auto_suspend);
int atbm_usb_pm_async(struct sbus_priv *self, ATBM_BOOL  auto_suspend);
atbm_void atbm_usb_urb_put(struct sbus_priv *self,atbm_uint32 *bitmap,int id);
int atbm_usb_urb_get(struct sbus_priv *self,atbm_uint32 *bitmap,int max_urb);
typedef int (*sbus_complete_handler)(atbm_urb_s *atbm_urb);
int atbm_usb_xmit_data(struct sbus_priv *self);
atbm_void atbm_release_tx_buffer(atbm_void *data);
atbm_void atbm_usb_module_init(atbm_void);
int atbm_usbctrl_vendorreq_sync(struct sbus_priv *self, atbm_uint8 request,atbm_uint8 b_write,
					atbm_uint16 value, atbm_uint16 index, atbm_void *pdata,atbm_uint16 len);
int __atbm_usb_suspend(struct sbus_priv *self);
atbm_void atbm_usb_kill_all_rxurb(struct sbus_priv *self);
int __atbm_usb_resume(struct sbus_priv *self);
atbm_void atbm_urb_coml(struct atbmwifi_common *hw_priv);
atbm_void atbm_usb_module_exit(atbm_void);

#endif//ATBM_USB_H
