/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"
#include "disp_gpio.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 st77916_commands[] = {
    0x11, 0,
    0x00, 1,  120,
    0x36, 1,  0x18,
    0x3A, 1,  0x55,
    0xF0, 1,  0xC3,
    0xF0, 1,  0x96,
    0xB4, 1,  0x01,
    0xB0, 1,  0x80,
    0xB6, 3,  0xA0, 0x02, 0x3b,
    0xC1, 1,  0x1D,
    0xC2, 1,  0xA7,
    0xC5, 1,  0x23,
    0xE8, 8,  0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33,
    0xE0, 14, 0xF0, 0x03, 0x0A, 0x12, 0x15, 0x1D, 0x42, 0x44, 0x50, 0x28, 0x16, 0x15, 0x20, 0x21,
    0xE1, 14, 0xF0, 0x03, 0x0A, 0x12, 0x15, 0x1C, 0x42, 0x44, 0x52, 0x28, 0x16, 0x15, 0x20, 0x23,
    0xF0, 1,  0x3C,
    0xF0, 1,  0x69,
    // 0x21, 0,
    // 0x00, 1,  120,
    0x29, 0,
    0x00, 1,  120
};
static struct gpio_desc reset;

static int panel_prepare(void) {
    panel_get_gpio(&reset, RESET_PIN);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(1);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);
    return 0;
}

static struct aic_panel_funcs st77916_funcs = {
    .prepare = panel_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing st77916_timing = {
    .pixelclock = 8 * 1000 * 1000,

    .hactive = 320,
    .hback_porch = 3,
    .hfront_porch = 3,
    .hsync_len = 3,

    .vactive = 480,
    .vback_porch = 3,
    .vfront_porch = 3,
    .vsync_len = 3,
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB666,
    .commands = {
        .buf = st77916_commands,
        .len = ARRAY_SIZE(st77916_commands),
    }
};

struct aic_panel dbi_st77916 = {
    .name = "panel-st77916",
    .timings = &st77916_timing,
    .funcs = &st77916_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
