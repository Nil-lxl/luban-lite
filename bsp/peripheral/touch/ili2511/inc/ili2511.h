/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-26        the first version
 */

#ifndef __ILI2511_H__
#define __ILI2511_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

#define ILI2511_MAX_TOUCH           5
#define ILI2511_POINT_LEN           5
#define ILI2511_SALVE_ADDR          0x41
/* ILI2511 reg */
#define ILI2511_PACKET_NUMBER       0x10
#define ILI2511_TOCUH0_X_HIGH       0x11
#define ILI2511_TOCUH0_X_LOW        0x12
#define ILI2511_TOCUH0_Y_HIGH       0x13
#define ILI2511_TOCUH0_Y_LOW        0x14
#define ILI2511_TOCUH0_PRESS        0x15
#define ILI2511_REPORT_ID_0_TO_5    0x1
#define ILI2511_REPORT_ID_6_TO_9    0x2
#define ILI2511_TOUCH_DOWN          0x1
#define ILI2511_TOUCH_OFF           0x0
#define ILI2511_MAX_X_H_COORDINATE  0x20
#define ILI2511_MAX_X_L_COORDINATE  0x21
#define ILI2511_MAX_Y_H_COORDINATE  0x22
#define ILI2511_MAX_Y_L_COORDINATE  0x23
#define ILI2511_MAX_POINT           0x26

#endif
