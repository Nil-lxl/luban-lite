/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <aic_hal.h>
#include "disp_gpio.h"

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

    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
    panel_dsi_generic_send_seq(panel, 0xEF, 0x08);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xC0, 0x63, 0x00);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x0A, 0x0C);
    panel_dsi_generic_send_seq(panel, 0xC2, 0x31, 0x08);
    panel_dsi_generic_send_seq(panel, 0xCC, 0x18);
    panel_dsi_generic_send_seq(panel, 0xB0, 0x00, 0x08, 0x10, 0x0E, 0x11, 0x07, 0x08, 0x08, 0x08, 0x25, 0x04, 0x12, 0x0F, 0x2C, 0x30, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xB1, 0x00, 0x11, 0x18, 0x0C, 0x10, 0x05, 0x07, 0x09, 0x08, 0x24, 0x04, 0x11, 0x10, 0x2B, 0x30, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
    panel_dsi_generic_send_seq(panel, 0xB0, 0x4D);
    panel_dsi_generic_send_seq(panel, 0xB1, 0x39);
    panel_dsi_generic_send_seq(panel, 0xB2, 0x87);
    panel_dsi_generic_send_seq(panel, 0xB3, 0x80);
    panel_dsi_generic_send_seq(panel, 0xB5, 0x47);
    panel_dsi_generic_send_seq(panel, 0xB7, 0x8A);
    panel_dsi_generic_send_seq(panel, 0xB8, 0x20);
    panel_dsi_generic_send_seq(panel, 0xB9, 0x10, 0x13);
    panel_dsi_generic_send_seq(panel, 0xC0, 0x09);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x78);
    panel_dsi_generic_send_seq(panel, 0xC2, 0x78);
    panel_dsi_generic_send_seq(panel, 0xD0, 0x88);
    aic_delay_ms(120);
    panel_dsi_generic_send_seq(panel, 0xE0, 0x00, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0xE1, 0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20);
    panel_dsi_generic_send_seq(panel, 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE3, 0x00, 0x00, 0x33, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE4, 0x22, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE5, 0x04, 0x34, 0xAA, 0xAA, 0x06, 0x34, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE6, 0x00, 0x00, 0x33, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE7, 0x22, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE8, 0x05, 0x34, 0xAA, 0xAA, 0x07, 0x34, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xEB, 0x02, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xEC, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xED, 0xFA, 0x45, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0x54, 0xAF);
    panel_dsi_generic_send_seq(panel, 0xEF, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
    panel_dsi_generic_send_seq(panel, 0xE8, 0x00, 0x0E);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x11);
    aic_delay_ms(120);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
    panel_dsi_generic_send_seq(panel, 0xE8, 0x00, 0x0C);
    aic_delay_ms(60);
    panel_dsi_generic_send_seq(panel, 0xE8, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xC7, 0x04); //00

    panel_dsi_generic_send_seq(panel, 0x29);
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

static struct display_timing h043a8_timing = {
    .pixelclock = 30 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 48,
    .hback_porch = 48,
    .hsync_len = 10,
    .vactive = 800,
    .vfront_porch = 20,
    .vback_porch = 20,
    .vsync_len = 10,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 2,
};

struct aic_panel dsi_h043a8 = {
    .name = "panel-h043a8",
    .timings = &h043a8_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

