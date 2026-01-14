/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dbi.h"
#include "disp_gpio.h"

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

static struct gpio_desc reset;
static int panel_prepare(void) {
    panel_get_gpio(&reset, RESET_PIN);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(1);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(100);

    return 0;
}
static struct display_timing v3_timing = {
    .pixelclock = 3600000,

    .hactive = 240,
    .hback_porch = 2,
    .hfront_porch = 3,
    .hsync_len = 1,

    .vactive = 320,
    .vback_porch = 3,
    .vfront_porch = 2,
    .vsync_len = 1,
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
    0x11,  0,
    0x00,  1,  120,
    0x36,  1,  0x00,
    0x3a,  1,  0x55,
    0xB2,  5,  0x0C, 0x0C, 0x00, 0x33, 0x33,
    0xB7,  1,  0x35,
    0xBB,  1,  0x2b,
    0xC0,  1,  0x2C,
    0xC2,  1,  0x01,
    0xC3,  1,  0x11,
    0xC4,  1,  0x20,
    0xC6,  1,  0x0F,
    0xD0,  2,  0xA4, 0xA1,
    0xD6,  1,  0xA1,
    0xE0,  14, 0xD0, 0x00, 0x05, 0x0e, 0x15, 0x0d, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19,
    0xE1,  14, 0xD0, 0x00, 0x05, 0x0d, 0x0c, 0x06, 0x2d, 0x44, 0x40, 0x0e, 0x1c, 0x18, 0x16, 0x19,
    0x00,  1,  120,
    0x00,  1,  120,
    0x29,  0,  120,
};

static struct display_timing t3_timing = {
    .pixelclock = 10 * 1000 * 1000,

    .hactive = 135,
    .hback_porch = 36,
    .hfront_porch = 64,
    .hsync_len = 8,

    .vactive = 240,
    .vback_porch = 18,
    .vfront_porch = 2,
    .vsync_len = 30,
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
    // 0x11,  0,
    // 0x00,  1,  120,
    // 0x36,  1,  0x00,
    // 0x3a,  1,  0x55,
    // 0xB2,  5,  0x0C, 0x0C, 0x00, 0x33, 0x33,
    // 0xB7,  1,  0x35,
    // 0xBB,  1,  0x2b,
    // 0xC0,  1,  0x2C,
    // 0xC2,  1,  0x01,
    // 0xC3,  1,  0x11,
    // 0xC4,  1,  0x20,
    // 0xC6,  1,  0x0F,
    // 0xD0,  2,  0xA4, 0xA1,
    // 0xD6,  1,  0xA1,
    // 0xE0,  14, 0xD0, 0x00, 0x05, 0x0e, 0x15, 0x0d, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19,
    // 0xE1,  14, 0xD0, 0x00, 0x05, 0x0d, 0x0c, 0x06, 0x2d, 0x44, 0x40, 0x0e, 0x1c, 0x18, 0x16, 0x19,
    // 0x00,  1, 120,
    // 0x00,  1, 120,
    // 0x29,  0, 120,

    0xE2, 3, 0x1d, 0x02, 0x54,

    0xE0, 1, 0x01,
    0x00, 1, 10,
    0xE0, 1, 0x03,
    0x00, 1, 10,
    0x01, 0,
    0x00, 1, 100,

    0xE6, 3, 0x00, 0xA4, 0x0F,

    0xB0, 7, 0x20, 0x00, 0x01, 0x3F, 0x00, 0xEF, 0x28,

    0xB4, 8, 0x01, 0xB6, 0x00, 0x08, 0x04, 0x00, 0x33, 0x00,

    0xB6, 7, 0x01, 0x21, 0x00, 0x04, 0x04, 0x00, 0x0f,

    0xBA, 1, 0x0F,
    0xB8, 2, 0x07, 0x01,
    0x36, 1, 0x00,
    0xF0, 1, 0x00,
    0x00, 1, 1,

    0xB8, 2, 0x0f, 0x01,
    0xBA, 1, 0x01,

    0x2A, 4, 0x00, 0x00, 0x00, 0xEF,

    0x2B, 4, 0x00, 0x00, 0x01, 0x3F,

    0x2C, 0,
    0x29, 0,

    // 0xBE, 6, 0x06, 0xf0, 0x01, 0xf0, 0x00, 0x00,
    // 0xd0, 1, 0x0d,

    // 0x2C, 0,
    
};

static struct display_timing p3_timing = {
    .pixelclock = 6 * 1000 * 1000,

    .hactive = 320,
    .hback_porch = 43,
    .hfront_porch = 8,
    .hsync_len = 4,

    .vactive = 240,
    .vback_porch = 12,
    .vfront_porch = 8,
    .vsync_len = 4,
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
