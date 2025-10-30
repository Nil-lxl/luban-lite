/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui Mai <huahui.mai@artinchip.com>
 */

#include "panel_com.h"
#include "panel_dsi.h"

static int panel_enable(struct aic_panel *panel)
{
    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    /* Out sleep */
    panel_dsi_dcs_send_seq(panel, 0x11);

    aic_delay_ms(120);

    /* Display Control Setting */
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x63, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x11, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xC2, 0x37, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xCC, 0x18);
    panel_dsi_dcs_send_seq(panel, 0xC7, 0x00);

    /* GAMMA Set */
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x40, 0xC9, 0x91, 0x07, 0x02, 0x09, 0x09, 0x1F, 0x04, 0x50);
    panel_dsi_dcs_send_seq(panel, 0x0F, 0xE4, 0x29, 0xDF);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x40, 0xCB, 0xD0, 0x11, 0x92, 0x07, 0x00, 0x08, 0x07, 0x1C);
    panel_dsi_dcs_send_seq(panel, 0x06, 0x53, 0x12, 0x63, 0xEB, 0xDF);

    /* Power Control Registers Initial */
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x65);

    /* Vcom Setting */
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x6A);

    /* End Vcom Setting */
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x87);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x80);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x49);
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x85);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x20);
    panel_dsi_dcs_send_seq(panel, 0xB9, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x78);
    panel_dsi_dcs_send_seq(panel, 0xC2, 0x78);
    panel_dsi_dcs_send_seq(panel, 0xD0, 0x88);
    aic_delay_ms(100);

    /* GIP Set  */
    panel_dsi_dcs_send_seq(panel, 0xE0, 0x00, 0x00, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0x08, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00, 0x33,
                                  0x33);
    panel_dsi_dcs_send_seq(panel, 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xE3, 0x00, 0x00, 0x33, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xE4, 0x44, 0x44);
    panel_dsi_dcs_send_seq(panel, 0xE5, 0x0E, 0x60, 0xA0, 0xA0, 0x10, 0x60, 0xA0, 0xA0, 0x0A, 0x60,
                                  0xA0, 0xA0, 0x0C, 0x60, 0xA0, 0xA0);
    panel_dsi_dcs_send_seq(panel, 0xE6, 0x00, 0x00, 0x33, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xE7, 0x44, 0x44);
    panel_dsi_dcs_send_seq(panel, 0xE8, 0x0D, 0x60, 0xA0, 0xA0, 0x0F, 0x60, 0xA0, 0xA0, 0x09, 0x60,
                                  0xA0, 0xA0, 0x0B, 0x60, 0xA0, 0xA0);
    panel_dsi_dcs_send_seq(panel, 0xEB, 0x02, 0x01, 0xE4, 0xE4, 0x44, 0x00, 0x40);
    panel_dsi_dcs_send_seq(panel, 0xEC, 0x02, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xED, 0xAB, 0x89, 0x76, 0x54, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                  0xFF, 0x10, 0x45, 0x67, 0x98, 0xBA);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00);

    panel_dsi_dcs_send_seq(panel, 0x36, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x3A, 0x70);
    panel_dsi_dcs_send_seq(panel, 0x29);

    aic_delay_ms(120);

    panel_dsi_setup_realmode(panel);
    panel_de_timing_enable(panel, 0);

    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing st7701s_timing = {
    .pixelclock = 35000000,
    .hactive = 480,
    .hfront_porch = 20,
    .hback_porch = 200,
    .hsync_len = 4,
    .vactive = 800,
    .vfront_porch = 10,
    .vback_porch = 20,
    .vsync_len = 4,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 2,
};

struct aic_panel dsi_st7701s = {
    .name = "panel-st7701s",
    .timings = &st7701s_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
