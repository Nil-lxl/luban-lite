/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"

#define NT35560_CS         "PC.0"
#define NT35560_SCL        "PC.5"
#define NT35560_SDI        "PC.6"

static int panel_enable(struct aic_panel *panel)
{
    panel_spi_device_emulation(NT35560_CS, NT35560_SDI, NT35560_SCL);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(20);

    panel_spi_cmd_wr(0x3A);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0xC0);
    aic_delay_ms(120);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(120);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs nt35560_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing nt35560_timing = {
    .pixelclock   = 26000000,

    .hactive      = 480,
    .hfront_porch = 26,
    .hback_porch  = 26,
    .hsync_len    = 8,

    .vactive      = 800,
    .vfront_porch = 8,
    .vback_porch  = 8,
    .vsync_len    = 8,
};

static struct panel_rgb rgb = {
    .mode        = PRGB,
    .format      = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order  = RGB,
    .data_mirror = 1,
};

struct aic_panel rgb_nt35560 = {
    .name = "panel-nt35560",
    .timings = &nt35560_timing,
    .funcs = &nt35560_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

