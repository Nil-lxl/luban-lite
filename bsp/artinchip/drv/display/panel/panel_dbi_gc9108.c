/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

static const u8 gc9108_commands[] = {
    0xfe, 0,
    0xfe, 0,
    0xef, 0,

    0xb3, 1, 0x03,
    0xb6, 1, 0x11,
    0xa3, 1, 0x11,
    0x36, 1, 0xd8,
    0x3a, 1, 0x05,
    0xb4, 1, 0x21,
    0xac, 1, 0x08,
    0xb1, 1, 0xc0,
    0xe6, 2, 0x50, 0x43,
    0xe7, 2, 0x56, 0x43,
    /* gamma */
    0xF0, 14, 0x1A, 0x60, 0x33, 0x6F, 0xfD, 0x3D, 0x30, 0x0, 0x0, 0x1f, 0x1f, 0x18, 0x1a, 0xF,
    0xF1, 14, 0x02, 0x27, 0x1B, 0x37, 0x59, 0x03, 0x30, 0x0, 0x0, 0x1f, 0xf, 0x17, 0x1a, 0xF,
    /* gamma end */

    0x35, 1, 0x00,
    0x44, 1, 0x00,
    0x11, 0,
    0xfe, 0,
    0xff, 0,

    0x11, 0,
    0x00, 1, 120,
    0x29, 0,

    0x2a, 4, 0x00, 0x00, 0x00, 0x7F,
    0x2b, 4, 0x00, 0x00, 0x00, 0x7F,
    0x2c, 0,
    0x00, 1, 120,
};

static struct aic_panel_funcs gc9108_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing gc9108_timing = {
    .pixelclock   = 300000,

    .hactive      = 128,
    .hback_porch  = 2,
    .hfront_porch = 3,
    .hsync_len    = 1,

    .vactive      = 128,
    .vback_porch  = 3,
    .vfront_porch = 2,
    .vsync_len    = 1,
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
    .flags = DBI_REFRESH_LINE | DBI_COMMAND_INSERT_NONE,
    .commands = {
        .buf = gc9108_commands,
        .len = ARRAY_SIZE(gc9108_commands),
    }
};

struct aic_panel dbi_gc9108 = {
    .name = "panel-gc9108",
    .timings = &gc9108_timing,
    .funcs = &gc9108_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
