/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: hjh <jiehua.huang@artinchip.com>
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <stdint.h>
#include <stdbool.h>
#include <drivers/i2c-bit-ops.h>
#include "aic_hal_clk.h"
#include "aic_common.h"
#include "aic_log.h"

#define AIC_SOFT_I2C_TIMEOUT            1000

struct aic_soft_i2c_pin {
    u32 sda_pin;
    u32 scl_pin;
};

struct aic_soft_i2c_bus {
    struct rt_i2c_bus_device bus;
    struct aic_soft_i2c_pin aic_i2c_pin;
};

static struct aic_soft_i2c_bus g_aic_soft_i2c;

static void aic_soft_i2c_set_sda(void *data, rt_int32_t state)
{
    rt_pin_mode(g_aic_soft_i2c.aic_i2c_pin.sda_pin, PIN_MODE_OUTPUT);
    rt_pin_write(g_aic_soft_i2c.aic_i2c_pin.sda_pin, state);
}

static void aic_soft_i2c_set_scl(void *data, rt_int32_t state)
{
    rt_pin_mode(g_aic_soft_i2c.aic_i2c_pin.scl_pin, PIN_MODE_OUTPUT);
    rt_pin_write(g_aic_soft_i2c.aic_i2c_pin.scl_pin, state);
}

static rt_int32_t aic_soft_i2c_get_sda(void *data)
{
    rt_int32_t ret;

    rt_pin_mode(g_aic_soft_i2c.aic_i2c_pin.sda_pin, PIN_MODE_INPUT_PULLUP);
    ret = rt_pin_read(g_aic_soft_i2c.aic_i2c_pin.sda_pin);

    return ret;
}

static rt_int32_t aic_soft_i2c_get_scl(void *data)
{
    rt_int32_t ret;

    rt_pin_mode(g_aic_soft_i2c.aic_i2c_pin.scl_pin, PIN_MODE_INPUT_PULLUP);
    ret = rt_pin_read(g_aic_soft_i2c.aic_i2c_pin.scl_pin);

    return ret;
}

static void aic_soft_i2c_udelay(rt_uint32_t us)
{
    aic_udelay(us);
}

struct rt_i2c_bit_ops aic_soft_i2c_ops = {
    .data = NULL,
    .set_sda = aic_soft_i2c_set_sda,
    .set_scl = aic_soft_i2c_set_scl,
    .get_sda = aic_soft_i2c_get_sda,
    .get_scl = aic_soft_i2c_get_scl,
    .udelay = aic_soft_i2c_udelay,
#ifdef AIC_USING_SOFT_I2C
    .delay_us = AIC_DEV_SOFT_I2C_DELAY_TIME,
#endif
    .timeout = AIC_SOFT_I2C_TIMEOUT,
};

static int aic_soft_i2c_register()
{
    int ret = RT_EOK;

#ifdef AIC_USING_SOFT_I2C
    g_aic_soft_i2c.aic_i2c_pin.sda_pin = rt_pin_get(AIC_SOFT_I2C_SDA_PIN);
    g_aic_soft_i2c.aic_i2c_pin.scl_pin = rt_pin_get(AIC_SOFT_I2C_SCL_PIN);

    g_aic_soft_i2c.bus.priv = &aic_soft_i2c_ops;

    ret = rt_i2c_bit_add_bus(&g_aic_soft_i2c.bus, "soft_i2c");
    if (ret) {
        hal_log_err("register soft i2c fail\r\n");
        return ret;
    }
#endif

    return ret;
}

INIT_DEVICE_EXPORT(aic_soft_i2c_register);
