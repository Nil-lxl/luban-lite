/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-09-03        the first version
 */

#ifndef __C145HAX01_H__
#define __C145HAX01_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

#define C145HAX01_SALVE_ADDR        0x38
#define C145HAX01_REGITER_LEN       1
#define C145HAX01_MAX_TOUCH         5
#define C145HAX01_POINT_INFO_LEN    6

#define C145HAX01_DEVICE_MODE       0x0
#define C145HAX01_GEST_ID           0x1
#define C145HAX01_TD_STATUS         0x2
#define C145HAX01_POINT1_XH         0x3
#define C145HAX01_POINT1_XL         0x4
#define C145HAX01_POINT1_YH         0x5
#define C145HAX01_POINT1_YL         0x6
#define C145HAX01_POINT1_W          0x7
#define C145HAX01_POINT1_MISC       0x8

#endif
