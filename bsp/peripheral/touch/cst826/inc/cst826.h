/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-26        the first version
 */

#ifndef __CST826_H__
#define __CST826_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

#define CST826_MAX_TOUCH        2
#define CST826_POINT_LEN        6
#define CST826_SALVE_ADDR       0x15
/* cst826 reg */
#define CST826_WORK_MODE        0x00
#define CST826_PROX_STATE       0x01
#define CST826_TOUCH_NUM        0x02
#define CST826_TOUCH1_XH        0x03
#define CST826_TOUCH1_XL        0x04
#define CST826_TOUCH1_YH        0x05
#define CST826_TOUCH1_YL        0x06
#define CST826_TOUCH1_PRES      0x07
#define CST826_TOUCH1_AREA      0x08
#define CST826_SLEEP_MODE       0xA5
#define CST826_FW_VERSION1      0xA6
#define CST826_FW_VERSION2      0xA7
#define CST826_MODULE_ID        0xA8
#define CST826_PROJECT_NAME     0xA9
#define CST826_CHIP_TYPE1       0xAA
#define CST826_CHIP_TYPE2       0xAB
#define CST826_CHECKSUM1        0xAC
#define CST826_CHECKSUM2        0xAD
#define CST826_PROX_MODE        0xB0
#define CST826_GES_MODE         0xD0
#define CST826_GESTURE_ID       0xD3

#define AIC_TOUCH_PANEL_CST826_X_RANGE  480
#define AIC_TOUCH_PANEL_CST826_Y_RANGE  272

#endif
