/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 st7789v_commands[] = {
    0x11,   0,
    0x36,   1,  0x00,
    0x3a,   1,  0x05,
    0xB2,   5,  0x0C, 0x0C, 0x00, 0x33, 0x33,
    0xB7,   1,  0x75,
    0xBB,   1,  0x21,
    0xC0,   1,  0x2C,
    0xC2,   1,  0x01,
    0xC3,   1,  0x13,
    0xC4,   1,  0x20,
    0xC6,   1,  0x0F,
    0xD0,   2,  0xA4, 0xA1,
    0xD6,   1,  0xA1,
    0xE0,   14, 0x70, 0x04, 0x0A, 0x08, 0x07, 0x05, 0x32, 0x32, 0x48, 0x38,
                0x15, 0x15, 0x2A, 0x2E,
    0xE1,   14, 0x70, 0x07, 0x0D, 0x09, 0x09, 0x16, 0x30, 0x44, 0x49, 0x39,
                0x16, 0x16, 0x2B, 0x2F,
    0x21,   0,
    0x29,   0,
};

#define RESET_PIN "PD.23"

static struct gpio_desc reset;

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

static struct aic_panel_funcs st7789v_funcs = {
    .prepare = panel_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing st7789v_timing = {
    .pixelclock   = 3600000,

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
    .type = SPI,
    .format = SPI_4LINE_RGB565,
    .commands = {
        .buf = st7789v_commands,
        .len = ARRAY_SIZE(st7789v_commands),
    }
};

struct aic_panel dbi_st7789v = {
    .name = "panel-st7789v",
    .timings = &st7789v_timing,
    .funcs = &st7789v_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};

