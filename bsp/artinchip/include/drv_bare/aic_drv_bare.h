/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Weihui.Xu <weihui.xu@artinchip.com>
 */

#ifndef _ARTINCHIP_AIC_DRV_BARE_H_
#define _ARTINCHIP_AIC_DRV_BARE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_hal.h>
#include "driver.h"
#include "aic_tlsf.h"
#include "tlsf.h"
#include "heap.h"
#include "aic_stdio.h"
#include "console.h"
#include "drv_rtc.h"
#include "uart.h"
#include "mmc.h"
#include "mtd.h"
#ifdef AIC_SPINAND_DRV
#include "spinand_port.h"
#endif
#include "wdt.h"
#include "gpai.h"

#ifdef __cplusplus
}
#endif

#endif /* _ARTINCHIP_AIC_DRV_BARE_H_ */
