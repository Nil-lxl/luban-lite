/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBM_USB_BH_H
#define ATBM_USB_BH_H
#include "atbm_usb.h"
#include "atbm_sbus.h"
#define USB_BLOCK_SIZE (512)
#define RX_BUFFER_SIZE (16*1024) //must biger than amsdu
#define RX_LONG_BUFFER_SIZE 4000

/* Suspend state privates */
enum atbm_bh_pm_state {
	 ATBMWIFI__BH_RESUMED = 0,
	 ATBMWIFI__BH_SUSPEND,
	 ATBMWIFI__BH_SUSPENDED,
	 ATBMWIFI__BH_RESUME,
};
int atbm_usb_pm(struct sbus_priv *self, ATBM_BOOL  suspend);
#endif
