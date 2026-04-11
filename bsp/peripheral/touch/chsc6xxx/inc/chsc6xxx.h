/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-04-23        the first version
 */

#ifndef __CHSC6XXX_H__
#define __CHSC6XXX_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

#define CHSC6XXX_MAX_TOUCH          2
#define CHSC6XXX_POINT_LEN          6
#define CHSC6XXX_SALVE_ADDR         0x2e
/* CHSC6XXX REG */
#define CHSC6XXX_POINT_INFO         0x0000
#define CHSC6XXX_POINT_NUM          0x0002
#define CHSC6XXX_POINT1_X_H         0x0003
#define CHSC6XXX_POINT2_X_H         0x0009
#define CHSC6XXX_SLEEP_MODE         0x00a5
#define CHSC6XXX_GESTURE_MODE       0x00d0
#define CHSC5XXX_GESTURE_ID         0x00d3
#define CHSC6XXX_RELATED_INFO       0x42bd
#define CHSC6XXX_PID_CFG            0x9e00
#define CHSC6XXX_CHIP_TYPE          0x9e6b

#endif
