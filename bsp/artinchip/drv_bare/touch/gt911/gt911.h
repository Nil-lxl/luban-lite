/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __GT911_H__
#define __GT911_H__

#include <aic_core.h>
#include "../touch.h"
#include "hal_i2c.h"

#define GT911_ADDR_LEN       2
#define GT911_REGITER_LEN    2
#define GT911_MAX_TOUCH      5
#define GT911_POINT_INFO_NUM 5

#define GT911_ADDRESS_HIGH 0x5D
#define GT911_ADDRESS_LOW  0x14

#define GT911_COMMAND_REG 0x8040
#define GT911_CONFIG_REG  0x8047
#define GT911_MOD_SWT_REG 0x804D

#define GT911_PRODUCT_ID  0x8140
#define GT911_VENDOR_ID   0x814A
#define GT911_READ_STATUS 0x814E

#define GT911_POINT1_REG 0x814F
#define GT911_POINT2_REG 0x8157
#define GT911_POINT3_REG 0x815F
#define GT911_POINT4_REG 0x8167
#define GT911_POINT5_REG 0x816F

#define GT911_CHECK_SUM 0x80FF

#define GT911_I2C_CHAN  AIC_TOUCH_GT911_I2C_CHA
#define GT911_RST_PIN   AIC_TOUCH_GT911_RST_PIN
#define GT911_INT_PIN   AIC_TOUCH_GT911_INT_PIN

int gt911_init(struct touch *touch);

#endif /* gt911.h */
