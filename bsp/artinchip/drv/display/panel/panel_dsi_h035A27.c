/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <aic_hal.h>

#if defined AIC_USING_D213ECV_EzUIX1_DEMO_V1
#define RESET_PIN  "PB.6"
#elif defined AIC_USING_D213ECV_EzUIX1_DEMO_V0
#define RESET_PIN  "PA.3"
#elif defined AIC_USING_JYX68_MIPI01
#define RESET_PIN    "PD.17"
#endif

static struct gpio_desc reset_gpio;

static void panel_gpio_init(struct aic_panel *panel) {
    panel_get_gpio(&reset_gpio, RESET_PIN);

    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_generic_send_seq(panel, 0xf0, 0xc3);
    panel_dsi_generic_send_seq(panel, 0xf0, 0x96);
    panel_dsi_generic_send_seq(panel, 0x36, 0x48);
    panel_dsi_generic_send_seq(panel, 0xB4, 0x01);
    panel_dsi_generic_send_seq(panel, 0xc5, 0x2E);

    panel_dsi_generic_send_seq(panel, 0xe8, 0x40, 0x84, 0x1B, 0x1B, 0x10, 0x03, 0xB8, 0x33);
    panel_dsi_generic_send_seq(panel, 0xe0, 0xf0, 0x07, 0x0E, 0x0A, 0x08, 0x25, 0x38, 0x43, 0x51, 0x38, 0x14, 0x12, 0x32, 0x3F);
    panel_dsi_generic_send_seq(panel, 0xe1, 0xf0, 0x08, 0x0D, 0x09, 0x09, 0x26, 0x39, 0x45, 0x52, 0x07, 0x13, 0x16, 0x32, 0x3F);

    panel_dsi_generic_send_seq(panel, 0x2a, 0x00, 0x00, 0x01, 0x3f);
    panel_dsi_generic_send_seq(panel, 0x2b, 0x00, 0x00, 0x01, 0xdf);

    panel_dsi_generic_send_seq(panel, 0xf0, 0x3c);
    panel_dsi_generic_send_seq(panel, 0xf0, 0x69);
    panel_dsi_generic_send_seq(panel, 0X3A, 0x77);

    panel_dsi_generic_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(120);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    aic_delay_ms(200);

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

static struct display_timing h035a27_timing = {
    .pixelclock = 11 * 1000 * 1000,
    .hactive = 320,
    .hfront_porch = 20,
    .hback_porch = 20,
    .hsync_len = 12,
    .vactive = 480,
    .vfront_porch = 12,
    .vback_porch = 12,
    .vsync_len = 6,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 1,
};

struct aic_panel dsi_h035a27 = {
    .name = "panel-h035a27",
    .timings = &h035a27_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

