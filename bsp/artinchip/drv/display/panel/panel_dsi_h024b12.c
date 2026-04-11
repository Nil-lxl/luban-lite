/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include "disp_gpio.h"

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_dcs_send_seq(panel, 0xDF, 0x98, 0x51, 0xE9);
//---------------- PAGE0 --------------
    panel_dsi_dcs_send_seq(panel, 0xDE, 0x00);
    //VGMP,VGSP,VGMN,VGSN 4.4
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x16, 0x7D, 0x16, 0x3B);
    //Set_R_GAMMA
    panel_dsi_dcs_send_seq(panel, 0xC8, 0x3F, 0x2B, 0x22, 0x21, 0x23, 0x26, 0x21, 0x22, 0x22, 0x22, 0x1E, 0x15, 0x11, 0x0A, 0x09, 0x0E, 0x3F, 0x2B, 0x22, 0x21, 0x24, 0x27, 0x22, 0x22, 0x22, 0x21, 0x1E, 0x14, 0x11, 0x0A, 0x09, 0x0E);
    //POW_CTRL
    panel_dsi_dcs_send_seq(panel, 0xB9, 0x33, 0x08, 0xCC);
    //DCDC_SEL
    panel_dsi_dcs_send_seq(panel, 0xBB, 0x47, 0x7A, 0x30, 0x40, 0x7C, 0x60, 0x70, 0x70);
    //VDDD_CTRL
    panel_dsi_dcs_send_seq(panel, 0xBC, 0x38, 0x3C);
    //SETSTBA
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x31, 0x20);
    //SETPANEL(default)
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x12);
    //SETRGBCYC
    panel_dsi_dcs_send_seq(panel, 0xC3, 0x08, 0x00, 0x0A, 0x10, 0x08, 0x54, 0x45, 0x71, 0x2C);
    //SETRGBCYC(default)
    panel_dsi_dcs_send_seq(panel, 0xC4, 0x00, 0xA0, 0x79, 0x0E, 0x0A, 0x16, 0x79, 0x0E, 0x0A, 0x16, 0x79, 0x0E, 0x0A, 0x16, 0x82, 0x00, 0x03);
    //SET_GD(default)
    panel_dsi_dcs_send_seq(panel, 0xD0, 0x04, 0x0C, 0x6A, 0x0F, 0x00, 0x03);
    //RAMCTRL(default)
    panel_dsi_dcs_send_seq(panel, 0xD7, 0x13, 0x00);
    //---------------- PAGE2 --------------
    panel_dsi_dcs_send_seq(panel, 0xDE, 0x02);
    aic_delay_ms(1);
    //DCDC_SET
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x1D, 0xA0, 0x2F, 0x2C, 0x2B);
    //SETRGBCYC2
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x10, 0x66, 0x66, 0x01);
    //---------------- PAGE0 --------------
    panel_dsi_dcs_send_seq(panel, 0xDE, 0x00);

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

static struct display_timing h024b12_timing = {
    .pixelclock = 10 * 1000 * 1000,
    .hactive = 240,
    .hfront_porch = 40,
    .hback_porch = 20,
    .hsync_len = 4,
    .vactive = 320,
    .vfront_porch = 8,
    .vback_porch = 6,
    .vsync_len = 2,
};

static struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 1,
};

struct aic_panel dsi_h024b12 = {
    .name = "panel-h024b12",
    .timings = &h024b12_timing,
    .funcs = &paenl_funcs,
    .dbi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
