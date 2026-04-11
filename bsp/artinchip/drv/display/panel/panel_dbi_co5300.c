/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

#define CO5300_RESET "PD.27"

static struct gpio_desc reset;

/* Init sequence, each line consists of command, count of data, data... */
static const u8 co5300_commands[] = {
    0xFE,  1,   0x00,
    0xC4,  1,   0x80,
    0x3A,  1,   0x77,
    0X35,  1,   0x00,
    0X53,  1,   0x20,
    0x51,  1,   0xFF,
    0x63,  1,   0xFF,
    0x2A,  4,   0x00,   0x00,   0x01,   0x0F,
    0x2B,  4,   0x00,   0x00,   0x01,   0xD1,
    0x11,  0,
    0x00,  1,   60,
    0x29,  0,
};

static int panel_prepare(void)
{
    panel_get_gpio(&reset, CO5300_RESET);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(120);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);

    return 0;
}

int panel_enable(struct aic_panel *panel)
{
    struct panel_dbi_commands *commands = &panel->dbi->commands;
    struct spi_cfg *spi = panel->dbi->spi;

    panel_di_enable(panel, 0);
    panel_dbi_commands_execute(panel, commands);

    spi->code[0] = 0x32;
    panel_di_enable(panel, 0);

    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs co5300_funcs = {
    .prepare = panel_prepare,
    .enable = panel_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,

};

static struct display_timing co5300_timing = {
    .pixelclock   = 9000000,

    .hactive      = 272,
    .hback_porch  = 10,
    .hfront_porch = 10,
    .hsync_len    = 10,

    .vactive      = 460,
    .vback_porch  = 10,
    .vfront_porch = 10,
    .vsync_len    = 5,
};

static struct spi_cfg spi = {
    .qspi_mode = 0,
    .vbp_num   = 0,
    .code1_cfg = 0,
    .code      = { 0x02, 0x00, 0x00 },
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4SDA_RGB888,
    .first_line = 0x2c,
    .other_line = 0x3c,
    .spi = &spi,
    .commands = {
        .buf = co5300_commands,
        .len = ARRAY_SIZE(co5300_commands),
    }
};

struct aic_panel dbi_co5300 = {
    .name = "panel-co5300",
    .timings = &co5300_timing,
    .funcs = &co5300_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
