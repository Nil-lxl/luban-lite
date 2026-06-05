/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui Mai <huahui.mai@artinchip.com>
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <drivers/i2c.h>

#define I2C2_NAME "i2c2"
#define I2C3_NAME "i2c3"

#define D01_I2C_ADDR  0x4A
#define LCOS_I2C_ADDR 0x49

#define D01_RESET   "PE.15"
#define LCOS_RESET  "PE.14"

static struct rt_i2c_bus_device *i2c2_bus = RT_NULL;
static struct rt_i2c_bus_device *i2c3_bus = RT_NULL;

static struct gpio_desc d01_reset_gpio;
static struct gpio_desc lcos_reset_gpio;

static const u8 svc_init_table[][2] = {
    {0x00, 0x10},
    {0x01, 0x05},
    {0x02, 0x02},
    {0x03, 0x05},
    {0x04, 0x02},
    {0x05, 0x05},
    {0x06, 0x02},
    {0x0f, 0x11},
    {0x10, 0x00},
    {0x11, 0x01},
    {0x12, 0x03},
    {0x13, 0x07},
    {0x14, 0x0c},
    {0x15, 0x14},
    {0x16, 0x1e},
    {0x17, 0x2a},
    {0x18, 0x38},
    {0x19, 0x49},
    {0x1a, 0x5b},
    {0x1b, 0x71},
    {0x1c, 0x89},
    {0x1d, 0xa3},
    {0x1e, 0xc0},
    {0x1f, 0xdf},
    {0x41, 0x01},
    {0x57, 0x03},
    {0x61, 0x18},
    {0x8a, 0xff},
};

static void i2c_reg_write(struct rt_i2c_bus_device *bus, u8 addr, u8 reg, u8 val)
{
    u8 buf[2];
    struct rt_i2c_msg msg;

    buf[0] = reg;
    buf[1] = val;

    msg.addr  = addr;
    msg.flags = RT_I2C_WR;
    msg.buf   = buf;
    msg.len   = 2;

    if (rt_i2c_transfer(bus, &msg, 1) != 1)
        pr_err("i2c write reg 0x%x fail\n", reg);
}

static inline void d01_i2c_write_byte(u8 reg, u8 val)
{
    i2c_reg_write(i2c3_bus, D01_I2C_ADDR, reg, val);
}

static inline void lcos_i2c_write_byte(u8 reg, u8 val)
{
    i2c_reg_write(i2c2_bus, LCOS_I2C_ADDR, reg, val);
}

static int i2c_setup(void)
{
    i2c2_bus = rt_i2c_bus_device_find(I2C2_NAME);
    if (i2c2_bus == RT_NULL) {
        pr_err("can't find %s device\n", I2C2_NAME);
        return -1;
    }

    i2c3_bus = rt_i2c_bus_device_find(I2C3_NAME);
    if (i2c3_bus == RT_NULL) {
        pr_err("can't find %s device\n", I2C3_NAME);
        return -1;
    }

    return 0;
}

static void lcos_init(void)
{
    // lcos_i2c_write_byte(0x15, 0x80);  // bist
    lcos_i2c_write_byte(0x16, 0x00);
    lcos_i2c_write_byte(0x40, 0xb1);
    lcos_i2c_write_byte(0x50, 0x00);
    lcos_i2c_write_byte(0x51, 0x1e);
    lcos_i2c_write_byte(0x52, 0x3f);
    lcos_i2c_write_byte(0x53, 0x00);
    lcos_i2c_write_byte(0x54, 0x21);
    lcos_i2c_write_byte(0x55, 0x3f);
    lcos_i2c_write_byte(0x08, 0x88);
    lcos_i2c_write_byte(0x4d, 0x01);
}

static void d01_init(void)
{
    int i;

    for (i = 0; i < sizeof(svc_init_table) / sizeof(svc_init_table[0]); i++) {
        d01_i2c_write_byte(svc_init_table[i][0], svc_init_table[i][1]);
        aicos_mdelay(1);
    }

    for (i = 5; i > 0; i--) {
        pr_info("wait %ds for D01 device initialization to stabilize...\n", i);
        aicos_mdelay(1000);
    }
}

static void panel_gpio_init(void)
{
    panel_get_gpio(&d01_reset_gpio, D01_RESET);
    panel_get_gpio(&lcos_reset_gpio, LCOS_RESET);

    panel_gpio_set_value(&d01_reset_gpio, 0);
    panel_gpio_set_value(&lcos_reset_gpio, 0);

    aicos_mdelay(120);

    panel_gpio_set_value(&d01_reset_gpio, 1);
    panel_gpio_set_value(&lcos_reset_gpio, 1);

    aicos_mdelay(120);
}

static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init();

    panel_di_enable(panel, 0);
    panel_dsi_setup_realmode(panel);
    panel_de_timing_enable(panel, 0);

    if (i2c_setup())
        return -1;

    lcos_init();
    d01_init();

    d01_i2c_write_byte(0x8d, 0x90);
    d01_i2c_write_byte(0x58, 0x01);

    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing svc1k26_timing = {
    .pixelclock = 74250000,
    .hactive = 1280,
    .hfront_porch = 74,
    .hback_porch = 216,
    .hsync_len = 80,
    .vactive = 720,
    .vfront_porch = 3,
    .vback_porch = 22,
    .vsync_len = 5,
};

static struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST | DSI_CLOCK_NON_CONTINUOUS | DSI_MOD_VID_HBP,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_svc1k26 = {
    .name = "panel-svc1k26",
    .timings = &svc1k26_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
