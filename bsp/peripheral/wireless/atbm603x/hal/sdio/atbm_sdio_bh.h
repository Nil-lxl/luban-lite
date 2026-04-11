/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef ATBM_SDIO_BH_H
#define ATBM_SDIO_BH_H
#define ATBM_SDIO_BLOCK_SIZE 256
#include "atbm_hal.h"
#if (ATBM_PLATFORM==AK_RTOS_300) || (ATBM_PLATFORM==AK_RTOS_37D)
#include "interrupt.h"
#endif
void atbm_irq_handler(struct atbmwifi_common *hw_priv);
int atbm_powerave_sdio_sync(struct atbmwifi_common *hw_priv);
int atbm_device_wakeup(struct atbmwifi_common *hw_priv);
ATBM_BOOL atbm_sdio_wait_enough_space(struct atbmwifi_common	*hw_priv, atbm_uint32 n_needs);
#endif
