/*
 * Copyright (C) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include "panel_com.h"
#include "panel_dsi.h"

static int panel_enable(struct aic_panel *panel)
{
    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_dcs_send_seq(panel, 0x01);
    aic_delay_ms(20);

    panel_dsi_dcs_send_seq(panel, 0xb0, 0x80);
    aic_delay_ms(10);

    panel_dsi_dcs_send_seq(panel, 0xb1, 0x00);
    aic_delay_ms(10);

    panel_dsi_dcs_send_seq(panel, 0xb2, 0x00);
    aic_delay_ms(10);

    panel_dsi_dcs_send_seq(panel, 0xb3, 0x00);
    aic_delay_ms(10);

    panel_dsi_dcs_send_seq(panel, 0x36, 0x01);
    aic_delay_ms(10);

    panel_dsi_dcs_send_seq(panel, 0x11);
    aic_delay_ms(120);

    panel_dsi_dcs_send_seq(panel, 0x29);
    aic_delay_ms(20);

    panel_dsi_setup_realmode(panel);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);

    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing ek79007ad_timing = {
    .pixelclock = 52 * 1000000,

    .hactive = 1024,
    .hfront_porch = 160,
    .hback_porch = 160,
    .hsync_len = 20,

    .vactive = 600,
    .vfront_porch = 12,
    .vback_porch = 20,
    .vsync_len = 2,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_ek79007ad = {
    .name = "panel-ek79007ad",
    .timings = &ek79007ad_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
