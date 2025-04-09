/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

/* Init sequence, each line consists of command, count of data, data... */
static const u8 st7789t3_commands[] = {
    0x11, 0,
    0x00, 1,  200,
    0x36, 1,  0x00,
    0x3a, 1,  0x55,
    0xb2, 5,  0x0c, 0x0c, 0x00, 0x33, 0x33,
    0xb7, 1,  0x56,
    0xbb, 1,  0x20,
    0xc0, 1,  0x2c,
    0xc2, 1,  0x01,
    0xc3, 1,  0x0f,
    0xc4, 1,  0x20,
    0xc6, 1,  0x0f,
    0xd0, 2,  0xa4, 0xa1,
    0xd0, 1,  0xa1,
    0xe0, 14, 0xf0, 0x00, 0x06, 0x06, 0x07, 0x05, 0x30, 0x44, 0x48, 0x38, 0x11,
              0x10, 0x2e, 0x34,
    0xe1, 14, 0xf0, 0x0a, 0x0e, 0x0d, 0x0b, 0x27, 0x2f, 0x44, 0x47, 0x35, 0x12,
              0x12, 0x2c, 0x32,
    0x35, 1,  0x00,
    0x21, 0,
    0x29, 0,
};

static struct aic_panel_funcs st7789t3_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing st7789t3_timing = {
    .pixelclock   = 5600000,

    .hactive      = 240,
    .hback_porch  = 10,
    .hfront_porch = 10,
    .hsync_len    = 4,

    .vactive      = 320,
    .vback_porch  = 10,
    .vfront_porch = 10,
    .vsync_len    = 8,
};


static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
    .commands = {
        .buf = st7789t3_commands,
        .len = ARRAY_SIZE(st7789t3_commands),
    }
};

struct aic_panel dbi_st7789t3 = {
    .name = "panel-st7789t3",
    .timings = &st7789t3_timing,
    .funcs = &st7789t3_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};

