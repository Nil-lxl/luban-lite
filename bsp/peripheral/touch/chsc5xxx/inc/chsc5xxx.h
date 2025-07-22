/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-03-10        the first version
 */

#ifndef __CHSC5XXX_H__
#define __CHSC5XXX_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

#define CHSC5XXX_MAX_TOUCH          5
#define CHSC5XXX_POINT_LEN          5
#define CHSC5XXX_SALVE_ADDR         0x2e
#define CHSC5XXX_NORNAL_TOUCH       0xff
#define CHSC5XXX_GESTURE_TOUCH      0xfe
/* CHSC5XXX REG */
#define CHSC5XXX_EVENT_TYPE         0x2000002c
#define CHSC5XXX_TOUCH_NUM          0x2000002d
#define CHSC5XXX_POINT1_X_L         0x2000002e
#define CHSC5XXX_POINT1_Y_L         0x2000002f
#define CHSC5XXX_POINT1_PRESS       0x20000030
#define CHSC5XXX_POINT1_XY_H        0x20000031
#define CHSC5XXX_POINT1_EVENT       0x20000032
#define CHSC5XXX_X_RESOLUTION       0x20000086
#define CHSC5XXX_Y_RESOLUTION       0x20000088

#endif
