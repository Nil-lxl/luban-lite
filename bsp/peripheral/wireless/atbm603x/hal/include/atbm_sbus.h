/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef ATBM_SBUS_H
#define ATBM_SBUS_H
#include "atbm_hal.h"
#include "atbm_os_atomic.h"
#include "atbm_os_spinlock.h"
#include "atbm_os_mutex.h"
/*
 * sbus priv forward definition.
 * Implemented and instantiated in particular modules.
 */
typedef atbm_void (*sbus_irq_handler)(atbm_void *priv);
typedef atbm_void (*sbus_callback_handler)(atbm_void *priv,atbm_void * arg);
struct sbus_priv {
	struct dvobj_priv *drvobj;
	struct atbmwifi_common	*core;
	struct atbm_platform_data *pdata;
	//struct atbm_buff *rx_skb;
	//struct atbm_buff *tx_skb;
	atbm_void 			*tx_data;
	int   			tx_vif_selected;
	unsigned int   	tx_seqnum;
	unsigned int   	rx_seqnum;
	atbm_atomic_t 			rx_lock;
	atbm_atomic_t 			tx_lock;
	//sbus_callback_handler	tx_callback_handler;
	//sbus_callback_handler	rx_callback_handler;
	atbm_spinlock_t		lock;
	//atbm_mutex * 	sbus_mutex;
	atbm_mutex      sbus_mutex;
	int 			auto_suspend;
	int 			suspend;
	atbm_uint8 *usb_data;
	atbm_uint8 *usb_req_data;
	//struct sbus_wtd         * wtd;
	////////////////////////////////////
	//sdio
	struct atbm_sdio_func	*func;
	sbus_irq_handler	irq_handler;
	void			*irq_priv;
	unsigned int   	rx_queue_qnum;
};

struct sbus_ops {
	int (*sbus_memcpy_fromio)(struct sbus_priv *self, unsigned int addr,atbm_void *dst, int count);/*rx queue mode*/
	int (*sbus_memcpy_toio)(struct sbus_priv *self, unsigned int addr,const atbm_void *src, int count);/*tx queue mode*/
	int (*sbus_read_async)(struct sbus_priv *self, unsigned int addr,atbm_void *dst, int count);/*rx queue mode*/
	int (*sbus_write_async)(struct sbus_priv *self, unsigned int addr,const atbm_void *src, int count);/*tx queue mode*/
	int (*sbus_read_sync)(struct sbus_priv *self, unsigned int addr, atbm_void *dst, int len);/*read register,download firmware,len <=256*/
	int (*sbus_write_sync)(struct sbus_priv *self, unsigned int addr, const atbm_void *src, int len);/*write register,download firmware,len <=256*/
	atbm_void (*lock)(struct sbus_priv *self);
	atbm_void (*unlock)(struct sbus_priv *self);	
	int (*irq_subscribe)(struct sbus_priv *self, sbus_irq_handler handler,
				atbm_void *priv);
	int (*irq_unsubscribe)(struct sbus_priv *self);
	int (*reset)(struct sbus_priv *self);
	atbm_uint32 (*align_size)(struct sbus_priv *self, atbm_uint32 size);
	int (*power_mgmt)(struct sbus_priv *self, ATBM_BOOL suspend);
	int (*set_block_size)(struct sbus_priv *self, atbm_uint32 size);
	atbm_void (*wtd_wakeup)( struct sbus_priv *self);
	#ifdef ATBM_USB_RESET
	int (*usb_reset)(struct sbus_priv *self);
	#endif
	int (*bootloader_debug_config)(struct sbus_priv *self,atbm_uint16 enable);	
	int (*lmac_start)(struct sbus_priv *self);
	int (*ep0_cmd)(struct sbus_priv *self);
	int (*abort)(struct sbus_priv *self);
	atbm_void (*sdio_irq_en)(struct sbus_priv *self,atbm_uint8 en);

};

#endif /* ATBM_SBUS_H */

