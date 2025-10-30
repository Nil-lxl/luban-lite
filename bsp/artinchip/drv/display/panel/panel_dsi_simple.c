/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"

#if PANEL_RESET
static struct gpio_desc reset;
#endif

static int panel_enable(struct aic_panel *panel)
{
#if PANEL_RESET
    panel_get_gpio(&reset, AIC_PANEL_RESET_GPIO);

    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);
    panel_gpio_set_value(&reset, 0);
    aic_delay_ms(120);
    panel_gpio_set_value(&reset, 1);
    aic_delay_ms(120);
#endif

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    PANEL_DSI_SIMEP_SEND_SEQ;

    panel_dsi_setup_realmode(panel);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static int panel_disable(struct aic_panel *panel)
{
    panel_default_disable(panel);
#if PANEL_RESET
    panel_gpio_set_value(&reset, 0);
#endif
    return 0;
}

static struct aic_panel_funcs dsi_simple_funcs = {
    .prepare           = panel_default_prepare,
    .enable            = panel_enable,
    .disable           = panel_disable,
    .unprepare         = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing panel_timing = {
    .pixelclock   = PANEL_PIXELCLOCK * 1000000,

    .hactive      = PANEL_HACTIVE,
    .hback_porch  = PANEL_HBP,
    .hfront_porch = PANEL_HFP,
    .hsync_len    = PANEL_HSW,

    .vactive      = PANEL_VACTIVE,
    .vback_porch  = PANEL_VBP,
    .vfront_porch = PANEL_VFP,
    .vsync_len    = PANEL_VSW,
};

static struct panel_dsi dsi = {
    .mode      = AIC_MIPI_DSI_MODE,
    .format    = AIC_MIPI_DSI_FORMAT,
    .lane_num  = AIC_MIPI_DSI_LINE_NUM,
    .vc_num    = VIRTUAL_CHANNEL,
    .ln_polrs  = LANE_POLARITIES,
    .dc_inv    = CLK_INVERSE,
    .ln_assign = LANE_ASSIGNMENTS,
};

struct aic_panel dsi_simple = {
    .name           = "panel-dsi-simple",
    .timings        = &panel_timing,
    .funcs          = &dsi_simple_funcs,
    .dsi            = &dsi,
    .connector_type = AIC_MIPI_COM,
};
