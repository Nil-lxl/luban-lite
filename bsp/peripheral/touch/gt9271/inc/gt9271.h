/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 */

#pragma once

#include "drivers/touch.h"
#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>

#define GT9271_ADDR_LEN         2
#define GT9271_REGITER_LEN      2
#define GT9271_MAX_TOUCH        10
#define GT9271_POINT_INFO_NUM   8

#define GT9271_ADDRESS_HIGH     0x5D
#define GT9271_ADDRESS_LOW      0x14

#define GT9271_COMMAND_REG      0x8040
#define GT9271_CONFIG_REG       0x8047
#define GT9271_MOD_SWT_REG      0x804D

#define GT9271_PRODUCT_ID       0x8140
#define GT9271_VENDOR_ID        0x814A
#define GT9271_READ_STATUS      0x814E

#define GT9271_POINT1_REG       0x814F
#define GT9271_POINT2_REG       0x8157
#define GT9271_POINT3_REG       0x815F
#define GT9271_POINT4_REG       0x8167
#define GT9271_POINT5_REG       0x816F

#define GT9271_CHECK_SUM        0x80FF

