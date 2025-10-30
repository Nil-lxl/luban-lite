/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"

#ifndef ST7789_MATCH_ID
#define ST7789_MATCH_ID 0
#endif

/* Init sequence, each line consists of command, count of data, data... */
static const u8 v3_commands[] = {
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

#define RESET_PIN ST7789_PANEL_RESET
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
static struct display_timing v3_timing = {
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

static struct panel_dbi v3_dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
    .commands = {
        .buf = v3_commands,
        .len = ARRAY_SIZE(v3_commands),
    }
};

/* Init sequence, each line consists of command, count of data, data... */
static const u8 t3_commands[] = {
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

static struct display_timing t3_timing = {
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

static struct panel_dbi t3_dbi = {
    .type = SPI,
    .format = SPI_4LINE_RGB565,
    .commands = {
        .buf = t3_commands,
        .len = ARRAY_SIZE(t3_commands),
    }
};

/* Init sequence, each line consists of command, count of data, data... */
static const u8 p3_commands[] = {
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

static struct display_timing p3_timing = {
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

static struct panel_dbi p3_dbi = {
    .type = I8080,
    .format = I8080_RGB565_8BIT,
    .commands = {
        .buf = p3_commands,
        .len = ARRAY_SIZE(p3_commands),
    }
};

static struct aic_panel_funcs st7789_funcs = {
    .prepare = panel_prepare,
    .enable = panel_dbi_default_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct panel_desc st7789_desc[] = {
    [0] = {
        .name = "p3",
        .dbi = &p3_dbi,
        .timings = &p3_timing,
        .funcs = &st7789_funcs,
    },
    [1] = {
        .name = "t3",
        .dbi = &t3_dbi,
        .timings = &t3_timing,
        .funcs = &st7789_funcs,
    },
    [2] = {
        .name = "v3",
        .dbi = &v3_dbi,
        .timings = &v3_timing,
        .funcs = &st7789_funcs,
    },
};

struct aic_panel dbi_st7789 = {
    .name = "panel-st7789",
    .desc = st7789_desc,
    .match_num = ARRAY_SIZE(st7789_desc),
    .match_id = ST7789_MATCH_ID,
    .connector_type = AIC_DBI_COM,
};
