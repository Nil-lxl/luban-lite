/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 h040a12_commands[] = {
    0xE0, 15, 0x00, 0x07, 0x0F, 0x04, 0x11, 0x06, 0x39, 0x67, 0x4E, 0x02, 0x0A, 0x09, 0x2D, 0x33, 0x0F,
    0xE1, 15, 0x00, 0x0F, 0x14, 0x03, 0x10, 0x06, 0x33, 0x34, 0x45, 0x06, 0x0E, 0x0C, 0x2A, 0x30, 0x0F,
    0xC0, 2,  0x14, 0x14,
    0xC1, 1,  0x45,
    0xC5, 3,  0x00, 0x55, 0x80,
    0x36, 1,  0x4E,
    0x3A, 1,  0x55,
    0xB1, 2,  0xB0, 0x11,
    0xB4, 1,  0x02,
    0xB6, 2,  0x02, 0x02,
    0xE9, 1,  0x00,
    0xF7, 4,  0xA9, 0x51, 0x2C, 0x82,
    0x2a, 4,  0x00, 0x00, 0x01, 0x3f,
    0x2b, 4,  0x00, 0x00, 0x01, 0xdf,
    0x21, 0,
    0x11, 0,
    0x00, 1,  120,
    0x29, 0,
};

static struct aic_panel_funcs h040a12_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing h040a12_timing = {
    .pixelclock = 20 * 1000 * 1000,

    .hactive = 320,
    .hback_porch = 2,
    .hfront_porch = 3,
    .hsync_len = 1,

    .vactive = 320,
    .vback_porch = 3,
    .vfront_porch = 2,
    .vsync_len = 1,
};

static struct panel_dbi dbi = {
    .type = I8080,
    .format = I8080_RGB565_16BIT,
    .commands = {
        .buf = h040a12_commands,
        .len = ARRAY_SIZE(h040a12_commands),
    }
};

struct aic_panel dbi_h040a12 = {
    .name = "panel-h040a12",
    .timings = &h040a12_timing,
    .funcs = &h040a12_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
