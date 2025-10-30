/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef __DRV_CAMERA_INNER_H__
#define __DRV_CAMERA_INNER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtdef.h"

#define REG8_ADDR_INVALID   0xFF
#define REG16_ADDR_INVALID   0xFFFF

#define PERCENT_TO_INT(min, max, p)     ((min) + ((max) - (min)) * (p) / 100)
#define PERCENT_IS_INVALID(p)           (p > 100)

struct rt_i2c_bus_device *camera_i2c_get(void);

u32 camera_xclk_rate_get(void);
void camera_xclk_enable(void);
void camera_xclk_disable(void);

u32 camera_rst_pin_get(void);
u32 camera_pwdn_pin_get(void);
void camera_pin_set_high(u32 pin);
void camera_pin_set_low(u32 pin);

struct reg8_info {
    u8 reg;
    u8 val;
};

struct reg16_info {
    u16 reg;
    u8  val;
};

#ifdef __cplusplus
}
#endif

#endif
