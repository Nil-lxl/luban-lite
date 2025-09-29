/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 h040a27_commands[] = {
    0x36, 1,  0x4E,
    0x3A, 1,  0x55,
    
    0xF0, 1,  0xC3,
    0xF0, 1,  0x96,
    0xB4, 1,  0x01,
    0xB7, 1,  0xC6,
    0xB9, 2,  0x02, 0xE0,
    0xC0, 2,  0x80, 0x07,
    0xC1, 1,  0x15,
    0xC2, 1,  0xA7,
    0xC5, 1,  0x07,
    0xE8, 8,  0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33,
    0xE0, 14, 0xF0, 0x04, 0x0E, 0x03, 0x02, 0x13, 0x34, 0x44, 0x4A, 0x3A, 0x15, 0x15, 0x2F, 0x34,
    0xE1, 14, 0xF0, 0x0F, 0x16, 0x0C, 0x09, 0x05, 0x34, 0x43, 0x4A, 0x35, 0x11, 0x11, 0x2C, 0x32,
    0xF0, 1,  0x3C,
    0xF0, 1,  0x69,
    0x00, 1,  120,

    0x21, 0,  //h040a27 enable
    0x11, 0,
    0x00, 1,  120,
    0x29, 0,
    // 0x00, 1,  200,
};

static struct aic_panel_funcs h040a27_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing h040a27_timing = {
    .pixelclock = 20 * 1000 * 1000,

    .hactive = 320,
    .hback_porch = 2,
    .hfront_porch = 3,
    .hsync_len = 1,

    .vactive = 480,
    .vback_porch = 3,
    .vfront_porch = 2,
    .vsync_len = 1,
};

static struct panel_dbi dbi = {
    .type = I8080,
    .format = I8080_RGB565_16BIT,
    .commands = {
        .buf = h040a27_commands,
        .len = ARRAY_SIZE(h040a27_commands),
    }
};

struct aic_panel dbi_h040a27 = {
    .name = "panel-h040a27",
    .timings = &h040a27_timing,
    .funcs = &h040a27_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
