/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include "panel_com.h"

#define HX7033_I2C_NAME "i2c1"
#define HX7033_I2C_ADDR 0x90

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;

static void hx7033_i2c_write(struct rt_i2c_bus_device *i2c,
                            unsigned char addr, unsigned char val)
{
    unsigned char buf[2];
    struct rt_i2c_msg msgs;

    buf[0] = addr;
    buf[1] = val;

    msgs.addr  = HX7033_I2C_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = buf;
    msgs.len   = 2;

    if (rt_i2c_transfer(i2c, &msgs, 1) == 1)
    {
        return;
    }
    else
    {
        pr_err("i2c write addr %#x fail\n", addr);
    }

}

static void hx7033_video_on(void)
{
    i2c_bus = rt_i2c_bus_device_find(HX7033_I2C_NAME);
    if (i2c_bus == RT_NULL) {
        pr_err("can't find %s device\n", HX7033_I2C_NAME);
        return;
    }

    /* hx7033 config */
    hx7033_i2c_write(i2c_bus, 0x00, 0xD6); // LR=1,UD=1
    hx7033_i2c_write(i2c_bus, 0x03, 0x50); // TRIGMA disable
    hx7033_i2c_write(i2c_bus, 0x04, 0x84); // RGB666 enable, dual edge disable
    hx7033_i2c_write(i2c_bus, 0x05, 0xF5); // Suggest setting
    hx7033_i2c_write(i2c_bus, 0x06, 0x55); // Suggest setting
    hx7033_i2c_write(i2c_bus, 0x07, 0x55); // Suggest setting
    hx7033_i2c_write(i2c_bus, 0x08, 0x55); // Suggest setting
    hx7033_i2c_write(i2c_bus, 0x09, 0x55); // Suggest setting
    hx7033_i2c_write(i2c_bus, 0x0A, 0xFB); // COM Setting
    hx7033_i2c_write(i2c_bus, 0x0B, 0x80); // COM Setting
    hx7033_i2c_write(i2c_bus, 0x0C, 0x03); // RGMAP1 Setting
    hx7033_i2c_write(i2c_bus, 0x0D, 0x62); // RGMAP2 Setting
    hx7033_i2c_write(i2c_bus, 0x0E, 0x96); // RGMAP3 Setting
    hx7033_i2c_write(i2c_bus, 0x0F, 0xBB); // RGMAP4 Setting
    hx7033_i2c_write(i2c_bus, 0x10, 0XF2); // RGMAP5 Setting
    hx7033_i2c_write(i2c_bus, 0x11, 0XF6); // RGMAP6 Setting
    hx7033_i2c_write(i2c_bus, 0x12, 0x09); // RGMAN6 Setting
    hx7033_i2c_write(i2c_bus, 0x13, 0x0D); // RGMAN5 Setting
    hx7033_i2c_write(i2c_bus, 0x14, 0x44); // RGMAN4 Setting
    hx7033_i2c_write(i2c_bus, 0x15, 0x69); // RGMAN3 Setting
    hx7033_i2c_write(i2c_bus, 0x16, 0x9D); // RGMAN2 Setting
    hx7033_i2c_write(i2c_bus, 0x17, 0xFC); // RGMAN1 Setting
    hx7033_i2c_write(i2c_bus, 0x30, 0x05); // VRINGP Setting
    hx7033_i2c_write(i2c_bus, 0x31, 0xFA); // VRINGN Setting

    aic_delay_ms(5);

    /*******************SC control*********************/
    hx7033_i2c_write(i2c_bus, 0x01, 0x80); // SC enable
    aic_delay_ms(5);
    hx7033_i2c_write(i2c_bus, 0x01, 0x00); // SC disable

    aic_delay_ms(5);
}

static int panel_enable(struct aic_panel *panel)
{
    hx7033_video_on();

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs hx7033_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing hx7033_timing = {
    .pixelclock   = 14200000,
    .hactive      = 320,
    .hfront_porch = 38,
    .hback_porch  = 38,
    .hsync_len    = 4,
    .vactive      = 240,
    .vfront_porch = 30,
    .vback_porch  = 28,
    .vsync_len    = 2,

    .flags        = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW,
};

static struct panel_rgb rgb = {
    .mode        = PRGB,
    .format      = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order  = BGR,
    .data_mirror = 1,
};

struct aic_panel lcos_hx7033 = {
    .name           = "panel-lcos-hx7033",
    .timings        = &hx7033_timing,
    .funcs          = &hx7033_funcs,
    .rgb            = &rgb,
    .connector_type = AIC_RGB_COM,
};
