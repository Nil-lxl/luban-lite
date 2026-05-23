/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-26        the first version
 */

#pragma once

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

typedef struct touch_point {
    rt_uint8_t id;
    rt_uint8_t status;
    int16_t touch_x;
    int16_t touch_y;
}touch_point_t;

#define ILI21XX_MAX_TOUCH           5

#define ILI21XX_POINT_LEN           5


#define ILI21XX_SALVE_ADDR          0x41

/* ILI21XX regs */
#define ILI21XX_TOUCH_INFO       0x10
#define ILI21XX_TOUCH0_X_HIGH       0x11
#define ILI21XX_TOUCH0_X_LOW        0x12
#define ILI21XX_TOUCH0_Y_HIGH       0x13
#define ILI21XX_TOUCH0_Y_LOW        0x14
#define ILI21XX_TOUCH0_PRESS        0x15

#define ILI21XX_MAX_X_H_COORDINATE  0x20
#define ILI21XX_MAX_X_L_COORDINATE  0x21
#define ILI21XX_MAX_Y_H_COORDINATE  0x22
#define ILI21XX_MAX_Y_L_COORDINATE  0x23
#define ILI21XX_MAX_POINT           0x26
