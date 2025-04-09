/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"

#define ST7789_RESET    "PC.7"
static struct gpio_desc reset;

/* Init sequence, each line consists of command, count of data, data... */
static const u8 st7789_commands[] = {
    0x11,   0,
    0x00,   1,  120,
    0x36,   1,  0x00,
    0x3a,   1,  0x55,
    0xB2,   5,  0x0C, 0x0C, 0x00, 0x33, 0x33,
    0xB7,   1,  0x75,
    0xBB,   1,  0x12,
    0xC0,   1,  0x2C,
    0xC2,   1,  0x01,
    0xC3,   1,  0x0F,
    0xC4,   1,  0x20,
    0xC6,   1,  0x0F,
    0xD0,   2,  0xA7, 0xA1,
    0xD0,   2,  0xA4, 0xA1,
    0xD6,   1,  0xA1,
    0xE0,   14, 0xF0, 0x06, 0x0D, 0x08, 0x08, 0x05, 0x35, 0x44, 0x4A, 0x38,
                0x13, 0x11, 0x2E, 0x33,
    0xE1,   14, 0xF0, 0x0D, 0x12, 0x0D, 0x0C, 0x27, 0x34, 0x43, 0x4A, 0x36,
                0x10, 0x12, 0x2C, 0x34,
    0x00,   1, 120,
    0x21,   0,
    0x00,   1, 120,
    0x29,   0,
    0x2C,   0,
};

static int panel_enable(struct aic_panel *panel)
{
    panel_get_gpio(&reset, ST7789_RESET);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);

    return panel_dbi_default_enable(panel);
}

static int panel_disable(struct aic_panel *panel)
{
    panel_default_disable(panel);

    panel_gpio_set_value(&reset, 0);
    return 0;
}

static struct aic_panel_funcs st7789_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .disable = panel_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing st7789_timing = {
    .pixelclock   = 4000000,

    .hactive      = 240,
    .hback_porch  = 2,
    .hfront_porch = 3,
    .hsync_len    = 1,

    .vactive      = 320,
    .vback_porch  = 3,
    .vfront_porch = 2,
    .vsync_len    = 1,
};

static struct panel_dbi dbi = {
    .type = I8080,
    .format = I8080_RGB565_8BIT,
    .commands = {
        .buf = st7789_commands,
        .len = ARRAY_SIZE(st7789_commands),
    }
};

struct aic_panel dbi_st7789 = {
    .name = "panel-st7789-240",
    .timings = &st7789_timing,
    .funcs = &st7789_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
