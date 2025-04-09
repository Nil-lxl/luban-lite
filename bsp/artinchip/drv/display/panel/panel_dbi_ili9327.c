/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

#define RESET_PIN "PD.23"
static struct gpio_desc reset;

static const u8 ili9327_commands[] = {
    0xE9,   1,  0x20,
    0xEA,   2,  0x00,   0xC0,
    0xE6,   3,  0x0B,   0x00,   0x00,
    0xD1,   3,  0x00,   0x65,   0x1A,
    0xD0,   3,  0x07,   0x01,   0x8B,
    0x36,   1,  0x48,
    0x3A,   1,  0x55,
    0xC1,   4,  0x10,   0x10,   0X02,   0x02,
    0xC0,   6,  0x00,   0x35,   0x00,   0x00,   0x01,   0x02,
    0xC5,   1,  0x02,
    0x21,   0,
    0xD2,   2,  0x01,   0x22,
    0xC8,   15, 0x00,   0x77,   0x77,   0x03,   0x07,   0x08,
                0x00,   0x00,   0x77,   0x30,   0x00,   0x07,
                0x08,   0x80,   0x00,
    0x2A,   4,  0x00,   0x00,   0x00,   0xF0,
    0x2B,   4,  0x00,   0x00,   0x01,   0x90,
    0x35,   1,  0x00,
    0x11,   0,
    0x00,   1,  200,
    0x29,   0,
    0x00,   1,  20,
};

static int panel_prepare(void)
{
    panel_get_gpio(&reset, RESET_PIN);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(1);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(50);

    return 0;
}

static struct aic_panel_funcs ili9327_funcs = {
    .prepare = panel_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing ili9327_timing = {
    .pixelclock   = 2900000,

    .hactive      = 240,
    .hback_porch  = 2,
    .hfront_porch = 3,
    .hsync_len    = 1,

    .vactive      = 400,
    .vback_porch  = 3,
    .vfront_porch = 2,
    .vsync_len    = 1,
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
    .commands = {
        .buf = ili9327_commands,
        .len = ARRAY_SIZE(ili9327_commands),
    }
};

struct aic_panel dbi_ili9327 = {
    .name = "panel-ili9327",
    .timings = &ili9327_timing,
    .funcs = &ili9327_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
