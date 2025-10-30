/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-06-11        the first version
 */

#ifndef __CST3240_H__
#define __CST3240_H__

#include "drivers/touch.h"
#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>

#define CST3240_SLAVE_ADDR              0x5A
#define CST3240_MAX_TOUCH_NUM           5
#define CST3240_INFO_LEN                5

#define CST3240_POINT1_REG              0xD000
#define CST3240_FIXED_REG               0xD006
#define CST3240_POINT2_REG              0xD007
#define CST3240_DEBUG_MODE              0xD101
#define CST3240_NORMAL_MODE             0xD109
#define CST3240_X_Y_RESOLUTION          0xD1F8
#define CST3240_CHIP_TYPE_ID            0xD204

int rt_hw_cst3240_init(const char *name, struct rt_touch_config *cfg);

#endif /* _CST3240_H_ */
