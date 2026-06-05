/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dbi.h"

static int panel_enable(struct aic_panel *panel)
{
    struct panel_dbi *dbi = panel->dbi;
    struct spi_cfg *spi = dbi->spi;

    panel_di_enable(panel, 0);

    /* Page 1 (GIP Setting) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x00, 0x0D);
    panel_dbi_dcs_send_seq(panel, 0x01, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x02, 0x83);
    panel_dbi_dcs_send_seq(panel, 0x03, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x04, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x05, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x06, 0xC9);
    panel_dbi_dcs_send_seq(panel, 0x08, 0xC3);
    panel_dbi_dcs_send_seq(panel, 0x09, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x0A, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x0B, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x0D, 0x05);
    panel_dbi_dcs_send_seq(panel, 0x0E, 0x82);
    panel_dbi_dcs_send_seq(panel, 0x0F, 0x72);
    panel_dbi_dcs_send_seq(panel, 0x10, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x11, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x12, 0x14);
    panel_dbi_dcs_send_seq(panel, 0x13, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x14, 0x14);
    panel_dbi_dcs_send_seq(panel, 0x15, 0x00);
    panel_dbi_dcs_send_seq(panel, 0xB0, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x80, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x2D, 0xA8);
    panel_dbi_dcs_send_seq(panel, 0x2E, 0x08);

    /* FW_R */
    panel_dbi_dcs_send_seq(panel, 0xB0, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x50, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x51, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x52, 0x0D);
    panel_dbi_dcs_send_seq(panel, 0x53, 0x0B);
    panel_dbi_dcs_send_seq(panel, 0x54, 0x15);
    panel_dbi_dcs_send_seq(panel, 0x55, 0x13);
    panel_dbi_dcs_send_seq(panel, 0x56, 0x11);
    panel_dbi_dcs_send_seq(panel, 0x57, 0x0F);
    panel_dbi_dcs_send_seq(panel, 0x58, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x59, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x5A, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x5B, 0x07);
    panel_dbi_dcs_send_seq(panel, 0xA0, 0x04);
    panel_dbi_dcs_send_seq(panel, 0xA1, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xA2, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xA3, 0x05);

    /* FW_L */
    panel_dbi_dcs_send_seq(panel, 0x5C, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x5D, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x5E, 0x0C);
    panel_dbi_dcs_send_seq(panel, 0x5F, 0x0A);
    panel_dbi_dcs_send_seq(panel, 0x60, 0x14);
    panel_dbi_dcs_send_seq(panel, 0x61, 0x12);
    panel_dbi_dcs_send_seq(panel, 0x62, 0x10);
    panel_dbi_dcs_send_seq(panel, 0x63, 0x0E);
    panel_dbi_dcs_send_seq(panel, 0x64, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x65, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x66, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x67, 0x06);
    panel_dbi_dcs_send_seq(panel, 0xA4, 0x04);
    panel_dbi_dcs_send_seq(panel, 0xA5, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xA6, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xA7, 0x05);

    /* BW_R */
    panel_dbi_dcs_send_seq(panel, 0x68, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x69, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x6A, 0x0A);
    panel_dbi_dcs_send_seq(panel, 0x6B, 0x0C);
    panel_dbi_dcs_send_seq(panel, 0x6C, 0x0E);
    panel_dbi_dcs_send_seq(panel, 0x6D, 0x10);
    panel_dbi_dcs_send_seq(panel, 0x6E, 0x12);
    panel_dbi_dcs_send_seq(panel, 0x6F, 0x14);
    panel_dbi_dcs_send_seq(panel, 0x70, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x71, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x72, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x73, 0x06);
    panel_dbi_dcs_send_seq(panel, 0xA8, 0x04);
    panel_dbi_dcs_send_seq(panel, 0xA9, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xAA, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xAB, 0x05);

    /* BW_L */
    panel_dbi_dcs_send_seq(panel, 0x74, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x75, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x76, 0x0B);
    panel_dbi_dcs_send_seq(panel, 0x77, 0x0D);
    panel_dbi_dcs_send_seq(panel, 0x78, 0x0F);
    panel_dbi_dcs_send_seq(panel, 0x79, 0x11);
    panel_dbi_dcs_send_seq(panel, 0x7A, 0x13);
    panel_dbi_dcs_send_seq(panel, 0x7B, 0x15);
    panel_dbi_dcs_send_seq(panel, 0x7C, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x7D, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x7E, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x7F, 0x07);
    panel_dbi_dcs_send_seq(panel, 0xAC, 0x04);
    panel_dbi_dcs_send_seq(panel, 0xAD, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xAE, 0x05);
    panel_dbi_dcs_send_seq(panel, 0xAF, 0x05);

    /* Page 2 (Internal VBP, VFP) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x02);
    panel_dbi_dcs_send_seq(panel, 0x0C, 0x2C);
    panel_dbi_dcs_send_seq(panel, 0x0F, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x10, 0x58);
    panel_dbi_dcs_send_seq(panel, 0x11, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x12, 0xD7);
    panel_dbi_dcs_send_seq(panel, 0x19, 0x20);
    panel_dbi_dcs_send_seq(panel, 0x1B, 0x11);
    panel_dbi_dcs_send_seq(panel, 0x52, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x44, 0x0F);
    panel_dbi_dcs_send_seq(panel, 0x55, 0x0C);
    panel_dbi_dcs_send_seq(panel, 0x56, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x57, 0x09);
    panel_dbi_dcs_send_seq(panel, 0x58, 0x1B);
    panel_dbi_dcs_send_seq(panel, 0x40, 0x46);
    panel_dbi_dcs_send_seq(panel, 0x59, 0x01);

    /* Page 4 (Dither Enable) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x04);
    panel_dbi_dcs_send_seq(panel, 0xF0, 0x01);

    /* Page 5 (Power Setting) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x05);
    panel_dbi_dcs_send_seq(panel, 0x00, 0x11);
    panel_dbi_dcs_send_seq(panel, 0x01, 0x17);
    panel_dbi_dcs_send_seq(panel, 0x04, 0xD1);
    panel_dbi_dcs_send_seq(panel, 0x07, 0x95);
    panel_dbi_dcs_send_seq(panel, 0x08, 0x7F);
    panel_dbi_dcs_send_seq(panel, 0x09, 0x97);
    panel_dbi_dcs_send_seq(panel, 0x0A, 0x97);
    panel_dbi_dcs_send_seq(panel, 0x17, 0x21);
    panel_dbi_dcs_send_seq(panel, 0x0F, 0x5E);
    panel_dbi_dcs_send_seq(panel, 0x10, 0xFF);
    panel_dbi_dcs_send_seq(panel, 0x11, 0xF5);
    panel_dbi_dcs_send_seq(panel, 0x12, 0xEF);
    panel_dbi_dcs_send_seq(panel, 0x13, 0xFB);
    panel_dbi_dcs_send_seq(panel, 0x14, 0x5F);
    panel_dbi_dcs_send_seq(panel, 0x42, 0x06);
    panel_dbi_dcs_send_seq(panel, 0x43, 0x06);
    panel_dbi_dcs_send_seq(panel, 0x40, 0x17);
    panel_dbi_dcs_send_seq(panel, 0x46, 0x13);
    panel_dbi_dcs_send_seq(panel, 0x47, 0x14);
    panel_dbi_dcs_send_seq(panel, 0x41, 0x56);
    panel_dbi_dcs_send_seq(panel, 0x44, 0x09);
    panel_dbi_dcs_send_seq(panel, 0x45, 0x55);
    panel_dbi_dcs_send_seq(panel, 0x2C, 0x03);

    /* OSC Auto Trim (still in Page 5) */
    panel_dbi_dcs_send_seq(panel, 0x9A, 0x86);
    panel_dbi_dcs_send_seq(panel, 0x9B, 0xC0);
    panel_dbi_dcs_send_seq(panel, 0x9C, 0x05);
    panel_dbi_dcs_send_seq(panel, 0x9D, 0x05);
    panel_dbi_dcs_send_seq(panel, 0x9E, 0xFF);
    panel_dbi_dcs_send_seq(panel, 0x9F, 0xFF);
    panel_dbi_dcs_send_seq(panel, 0xAA, 0x02);
    panel_dbi_dcs_send_seq(panel, 0xAB, 0xC0);
    panel_dbi_dcs_send_seq(panel, 0xAC, 0x7F);
    panel_dbi_dcs_send_seq(panel, 0xAD, 0xBF);

    /* Page 6 (Resolution) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x06);
    panel_dbi_dcs_send_seq(panel, 0x2E, 0x07);
    panel_dbi_dcs_send_seq(panel, 0xC0, 0x67);
    panel_dbi_dcs_send_seq(panel, 0xC1, 0x01);
    panel_dbi_dcs_send_seq(panel, 0xCC, 0x67);
    panel_dbi_dcs_send_seq(panel, 0xCD, 0x01);
    panel_dbi_dcs_send_seq(panel, 0x09, 0x28);
    panel_dbi_dcs_send_seq(panel, 0xAA, 0x00);
    panel_dbi_dcs_send_seq(panel, 0xAB, 0x00);
    panel_dbi_dcs_send_seq(panel, 0xAC, 0x00);
    panel_dbi_dcs_send_seq(panel, 0xAD, 0x00);

    /* Page 7 (Gamma) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x07);
    panel_dbi_dcs_send_seq(panel, 0xE0, 0x90, 0x06, 0x0B, 0x0A,
                           0x0A, 0x06, 0x32, 0x43, 0x4A, 0x06,
                           0x17, 0x1A, 0x36, 0x38);
    panel_dbi_dcs_send_seq(panel, 0xE1, 0x90, 0x06, 0x0B, 0x0A,
                           0x0A, 0x06, 0x32, 0x43, 0x4A, 0x06,
                           0x17, 0x1A, 0x36, 0x38);

    /* Page 9 (Vcom) */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x09);
    panel_dbi_dcs_send_seq(panel, 0x2C, 0x00);
    panel_dbi_dcs_send_seq(panel, 0x2D, 0x66);
    panel_dbi_dcs_send_seq(panel, 0x13, 0x23);

    /* Back to Page 0 */
    panel_dbi_dcs_send_seq(panel, 0xFF, 0x83, 0x88, 0x00);
    panel_dbi_dcs_send_seq(panel, 0xC4, 0x80);
    panel_dbi_dcs_send_seq(panel, 0x3A, 0x55);
    panel_dbi_dcs_send_seq(panel, 0x2A, 0x00, 0x00, 0x01, 0x67);
    panel_dbi_dcs_send_seq(panel, 0x2B, 0x00, 0x00, 0x01, 0x67);
    panel_dbi_dcs_send_seq(panel, 0x35, 0x00);

    aic_delay_ms(115);
    panel_dbi_dcs_send_seq(panel, 0x11);
    aic_delay_ms(120);
    panel_dbi_dcs_send_seq(panel, 0x29);
    aic_delay_ms(20);

    spi->code[0] = 0x32;
    panel_di_enable(panel, 0);

    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs il79400a_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing il79400a_timing = {
    .pixelclock = 15000000,

    .hactive = 360,
    .hback_porch = 10,
    .hfront_porch = 10,
    .hsync_len = 10,

    .vactive = 360,
    .vback_porch = 10,
    .vfront_porch = 10,
    .vsync_len = 10,
};

static struct spi_cfg spi = {
    .qspi_mode = 0,
    .code = {0x02, 0x00, 0x00},
};

static struct panel_dbi dbi = {
    .type = SPI,
    .format = SPI_4SDA_RGB565,
    .spi = &spi,
};

struct aic_panel dbi_il79400a = {
    .name = "panel-il79400a",
    .timings = &il79400a_timing,
    .funcs = &il79400a_funcs,
    .dbi = &dbi,
    .connector_type = AIC_DBI_COM,
};
