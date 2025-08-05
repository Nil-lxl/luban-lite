/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    //CMD PAGE 3
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x98, 0x81, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x01, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x02, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x03, 0x73);
    panel_dsi_dcs_send_seq(panel, 0x04, 0x13);
    panel_dsi_dcs_send_seq(panel, 0x05, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x06, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x07, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x08, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x09, 0x28);
    panel_dsi_dcs_send_seq(panel, 0x0a, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x0b, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x0c, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x0d, 0x28);
    panel_dsi_dcs_send_seq(panel, 0x0e, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x0f, 0x28);
    panel_dsi_dcs_send_seq(panel, 0x10, 0x28);
    panel_dsi_dcs_send_seq(panel, 0x11, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x12, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x13, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x14, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x15, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x16, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x17, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x18, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x19, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1a, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1b, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1c, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1d, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x1e, 0x40);
    panel_dsi_dcs_send_seq(panel, 0x1f, 0x80);
    panel_dsi_dcs_send_seq(panel, 0x20, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x21, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x22, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x23, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x24, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x25, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x26, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x27, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x28, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x2a, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x2b, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x2c, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x2d, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x2e, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x2f, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x30, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x31, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x32, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x33, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x34, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x37, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x38, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x39, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3a, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3b, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3c, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3d, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3e, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3f, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x40, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x41, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x42, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x43, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x44, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x50, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x23);
    panel_dsi_dcs_send_seq(panel, 0x52, 0x44);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x67);
    panel_dsi_dcs_send_seq(panel, 0x54, 0x89);
    panel_dsi_dcs_send_seq(panel, 0x55, 0xab);
    panel_dsi_dcs_send_seq(panel, 0x56, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x57, 0x23);
    panel_dsi_dcs_send_seq(panel, 0x58, 0x45);
    panel_dsi_dcs_send_seq(panel, 0x59, 0x67);
    panel_dsi_dcs_send_seq(panel, 0x5a, 0x89);
    panel_dsi_dcs_send_seq(panel, 0x5b, 0xab);
    panel_dsi_dcs_send_seq(panel, 0x5c, 0xcd);
    panel_dsi_dcs_send_seq(panel, 0x5d, 0xef);
    panel_dsi_dcs_send_seq(panel, 0x5e, 0x11);
    panel_dsi_dcs_send_seq(panel, 0x5f, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x60, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x61, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0x62, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x63, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0x64, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0x65, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x66, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x67, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x68, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x69, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x6a, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x6b, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x6c, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x6d, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x6e, 0x16);
    panel_dsi_dcs_send_seq(panel, 0x6f, 0x16);
    panel_dsi_dcs_send_seq(panel, 0x70, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x71, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x72, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x73, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x74, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x75, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x76, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x77, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0x78, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x79, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0x7a, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0x7b, 0x17);
    panel_dsi_dcs_send_seq(panel, 0x7c, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x7d, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x7e, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x7f, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x81, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x82, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x83, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x84, 0x16);
    panel_dsi_dcs_send_seq(panel, 0x85, 0x16);
    panel_dsi_dcs_send_seq(panel, 0x86, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x87, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x88, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x89, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x8A, 0x02);
    //CMD PAGE 4
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x98, 0x81, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x00, 0x80);  //MIPI_LANE_SEL
    panel_dsi_dcs_send_seq(panel, 0x6E, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0x6F, 0x37);
    panel_dsi_dcs_send_seq(panel, 0x3A, 0xA4);
    panel_dsi_dcs_send_seq(panel, 0x8D, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0x87, 0xBA);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0xD1);
    panel_dsi_dcs_send_seq(panel, 0x88, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0x38, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x39, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x31, 0x25);
    panel_dsi_dcs_send_seq(panel, 0x3B, 0x98);
    //CMD PAGE 1
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x98, 0x81, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x22, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x31, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x3D);
    panel_dsi_dcs_send_seq(panel, 0x55, 0x3D);
    panel_dsi_dcs_send_seq(panel, 0x50, 0xA0);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x9C);
    panel_dsi_dcs_send_seq(panel, 0x60, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x62, 0x20);
    panel_dsi_dcs_send_seq(panel, 0xA0, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xA1, 0x21);
    panel_dsi_dcs_send_seq(panel, 0xA2, 0x35);
    panel_dsi_dcs_send_seq(panel, 0xA3, 0x19);
    panel_dsi_dcs_send_seq(panel, 0xA4, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0xA5, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xA6, 0x27);
    panel_dsi_dcs_send_seq(panel, 0xA7, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xA8, 0xAF);
    panel_dsi_dcs_send_seq(panel, 0xA9, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xAA, 0x27);
    panel_dsi_dcs_send_seq(panel, 0xAB, 0x8D);
    panel_dsi_dcs_send_seq(panel, 0xAC, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0xAD, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xAE, 0x50);
    panel_dsi_dcs_send_seq(panel, 0xAF, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x2B);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x54);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x23);

    // panel_dsi_dcs_send_seq(panel, 0xB6, 0x90);  //software polarity
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x02);  //MIPI_LANE num

    panel_dsi_dcs_send_seq(panel, 0xC0, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x21);
    panel_dsi_dcs_send_seq(panel, 0xC2, 0x35);
    panel_dsi_dcs_send_seq(panel, 0xC3, 0x19);
    panel_dsi_dcs_send_seq(panel, 0xC4, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0xC5, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xC6, 0x27);
    panel_dsi_dcs_send_seq(panel, 0xC7, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xC8, 0xAF);
    panel_dsi_dcs_send_seq(panel, 0xC9, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xCA, 0x27);
    panel_dsi_dcs_send_seq(panel, 0xCB, 0x8D);
    panel_dsi_dcs_send_seq(panel, 0xCC, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0xCD, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xCE, 0x50);
    panel_dsi_dcs_send_seq(panel, 0xCF, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xD0, 0x2B);
    panel_dsi_dcs_send_seq(panel, 0xD1, 0x54);
    panel_dsi_dcs_send_seq(panel, 0xD2, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0xD3, 0x23);
    //CMD PAGE 0
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x98, 0x81, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x00);

    panel_dsi_dcs_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(120);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x00);
    aic_delay_ms(5);

    panel_dsi_setup_realmode(panel);

    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs paenl_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h070b13_timing = {
    .pixelclock = 70 * 1000 * 1000,
    .hactive = 800,
    .hfront_porch = 20,
    .hback_porch = 20,
    .hsync_len = 20,
    .vactive = 1280,
    .vfront_porch = 20,
    .vback_porch = 30,
    .vsync_len = 2,
};

static struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h070b13 = {
    .name = "panel-h070b13",
    .timings = &h070b13_timing,
    .funcs = &paenl_funcs,
    .dbi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
