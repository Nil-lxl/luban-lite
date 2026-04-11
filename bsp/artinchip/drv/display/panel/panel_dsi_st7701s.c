/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Huahui Mai <huahui.mai@artinchip.com>
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <aic_hal.h>
#include "disp_gpio.h"

static struct gpio_desc reset_gpio;

static void panel_gpio_init(struct aic_panel *panel) {
    panel_get_gpio(&reset_gpio, RESET_PIN);

    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel) {

    panel_gpio_init(panel);
    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
    panel_dsi_generic_send_seq(panel, 0xEF, 0x08);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xC0, 0x63, 0x00);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x0D, 0x0D);
    panel_dsi_generic_send_seq(panel, 0xC2, 0x07, 0x03);
    panel_dsi_generic_send_seq(panel, 0xB0, 0x00, 0x0B, 0x12, 0x0E, 0x11, 0x06, 0x02, 0x08, 0x08, 0x1F, 0x06, 0x14, 0x12, 0x29, 0x31, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xB1, 0x00, 0x0C, 0x13, 0x0C, 0x10, 0x06, 0x01, 0x07, 0x06, 0x1F, 0x03, 0x12, 0x10, 0x29, 0x32, 0x1F);
    panel_dsi_generic_send_seq(panel, 0x36, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
    panel_dsi_generic_send_seq(panel, 0xB0, 0x65);
    panel_dsi_generic_send_seq(panel, 0xB1, 0x5A);
    panel_dsi_generic_send_seq(panel, 0xB2, 0x87);
    panel_dsi_generic_send_seq(panel, 0xB3, 0x80);
    panel_dsi_generic_send_seq(panel, 0xB5, 0x46);
    panel_dsi_generic_send_seq(panel, 0xB7, 0x85);
    panel_dsi_generic_send_seq(panel, 0xB8, 0x31);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x78);
    panel_dsi_generic_send_seq(panel, 0xC2, 0x78);
    panel_dsi_generic_send_seq(panel, 0xE0, 0x00, 0x29, 0x02);
    panel_dsi_generic_send_seq(panel, 0xE1, 0x08, 0xAE, 0x00, 0x00, 0x07, 0xAE, 0x00, 0x00, 0x00, 0x43, 0x43);
    panel_dsi_generic_send_seq(panel, 0xE2, 0x30, 0x30, 0x40, 0x40, 0x2D, 0xAE, 0x00, 0x00, 0x2C, 0xAE, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE3, 0x00, 0x00, 0x33, 0x33);
    panel_dsi_generic_send_seq(panel, 0xE4, 0x44, 0x44);
    panel_dsi_generic_send_seq(panel, 0xE5, 0x0A, 0x30, 0x0E, 0xAE, 0x0C, 0x32, 0x0E, 0xAE, 0x0E, 0x34, 0x0E, 0xAE, 0x10, 0x36, 0x0E, 0xAE);
    panel_dsi_generic_send_seq(panel, 0xE6, 0x00, 0x00, 0x33, 0x33);
    panel_dsi_generic_send_seq(panel, 0xE7, 0x44, 0x44);
    panel_dsi_generic_send_seq(panel, 0xE8, 0x09, 0x2F, 0x0E, 0xAE, 0x0B, 0x31, 0x0E, 0xAE, 0x0D, 0x33, 0x0E, 0xAE, 0x0F, 0x35, 0x0E, 0xAE);
    panel_dsi_generic_send_seq(panel, 0xEB, 0x00, 0x01, 0xE4, 0xE4, 0x99, 0x33, 0x40);
    panel_dsi_generic_send_seq(panel, 0xEC, 0x3D, 0x00);
    panel_dsi_generic_send_seq(panel, 0xED, 0x20, 0x76, 0x54, 0x89, 0xBA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAB, 0x98, 0x45, 0x67, 0x02);
    panel_dsi_generic_send_seq(panel, 0x35, 0x00);
    panel_dsi_generic_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(120);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    aic_delay_ms(120);

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

static struct display_timing st7701s_timing = {
    .pixelclock = 35 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 20,
    .hback_porch = 20,
    .hsync_len = 8,
    .vactive = 800,
    .vfront_porch = 20,
    .vback_porch = 20,
    .vsync_len = 8,
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
