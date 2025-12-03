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
    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(100);
}

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    // panel_dsi_dcs_send_seq(panel, 0xF0, 0xC3);
    // panel_dsi_dcs_send_seq(panel, 0xF0, 0x96);
    // panel_dsi_dcs_send_seq(panel, 0xF0, 0xA5);
    // panel_dsi_dcs_send_seq(panel, 0xC1, 0x00, 0x08, 0xA5, 0x13);
    // panel_dsi_dcs_send_seq(panel, 0xC2, 0x00, 0x08, 0xA5, 0x13);
    // panel_dsi_dcs_send_seq(panel, 0xC3, 0x44, 0x04, 0x44, 0x04);
    // panel_dsi_dcs_send_seq(panel, 0xC4, 0x44, 0x04, 0x44, 0x04);
    // panel_dsi_dcs_send_seq(panel, 0xC5, 0x6A);
    // panel_dsi_dcs_send_seq(panel, 0xD6, 0x00);
    // panel_dsi_dcs_send_seq(panel, 0xD7, 0x00);
    // panel_dsi_dcs_send_seq(panel, 0xE0, 0xD2, 0x09, 0x0C, 0x07, 0x06, 0x23,
    //     0x2E, 0x43, 0x46, 0x17, 0x13, 0x13, 0x2D, 0x33);
    // panel_dsi_dcs_send_seq(panel, 0xE1, 0xD2, 0x09, 0x0C, 0x07, 0x05, 0x23,
    //     0x2E, 0x33, 0x46, 0x17, 0x13, 0x13, 0x2D, 0x33);
    // panel_dsi_dcs_send_seq(panel, 0xE5, 0xB5, 0xF5, 0xC9, 0x33, 0x22, 0x25,
    //     0x10, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
    // panel_dsi_dcs_send_seq(panel, 0xE6, 0xB5, 0xF5, 0xC9, 0x33, 0x22, 0x25,
    //     0x10, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
    // panel_dsi_dcs_send_seq(panel, 0xEC, 0x00, 0x55, 0x00, 0x00, 0x00, 0x08);
    // panel_dsi_dcs_send_seq(panel, 0x36, 0x0C);
    // panel_dsi_dcs_send_seq(panel, 0x3A, 0x07);
    // panel_dsi_dcs_send_seq(panel, 0xB2, 0x09);
    // panel_dsi_dcs_send_seq(panel, 0xB3, 0x01);
    // panel_dsi_dcs_send_seq(panel, 0xB4, 0x01);
    // panel_dsi_dcs_send_seq(panel, 0xB5, 0x00, 0x08, 0x00, 0x08);
    // panel_dsi_dcs_send_seq(panel, 0xB6, 0xC7, 0x31);
    // panel_dsi_dcs_send_seq(panel, 0xA4, 0xC0, 0x63);
    // panel_dsi_dcs_send_seq(panel, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15,
    //     0x2A, 0xBA, 0x02);
    // panel_dsi_dcs_send_seq(panel, 0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15,
    //     0x2A, 0xBA, 0x02);
    // panel_dsi_dcs_send_seq(panel, 0xBA, 0x1A, 0x0A, 0x45, 0x00, 0x23, 0x01,
    //     0x00);
    // panel_dsi_dcs_send_seq(panel, 0xBB, 0x00, 0x24, 0x00, 0x2F, 0x83, 0x07,
    //     0x18, 0x00);
    // panel_dsi_dcs_send_seq(panel, 0xBC, 0x00, 0x24, 0x00, 0x2F, 0x83, 0x07,
    //     0x18, 0x00);
    // panel_dsi_dcs_send_seq(panel, 0xBD, 0x22, 0x11, 0xFF, 0xFF, 0x55, 0x44,
    //     0x77, 0x66, 0xFF, 0xFF, 0x0F);
    // panel_dsi_dcs_send_seq(panel, 0xED, 0xC3);
    // panel_dsi_dcs_send_seq(panel, 0xE4, 0x40, 0x08);
    // panel_dsi_dcs_send_seq(panel, 0x35, 0x00);
    // panel_dsi_dcs_send_seq(panel, 0x21);

    panel_dsi_dcs_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(120);

    panel_dsi_dcs_send_seq(panel, 0x36, 0x5E);
    panel_dsi_dcs_send_seq(panel, 0x3A, 0x66);
    panel_dsi_dcs_send_seq(panel, 0xF0, 0xC3);
    panel_dsi_dcs_send_seq(panel, 0xF0, 0x96);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x80);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0xA0, 0x02, 0x3b);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x1D);
    panel_dsi_dcs_send_seq(panel, 0xC2, 0xA7);
    panel_dsi_dcs_send_seq(panel, 0xC5, 0x23);
    panel_dsi_dcs_send_seq(panel, 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xE0, 0xF0, 0x03, 0x0A, 0x12, 0x15, 0x1D, 0x42, 0x44, 0x50, 0x28, 0x16, 0x15, 0x20, 0x21);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0xF0, 0x03, 0x0A, 0x12, 0x15, 0x1C, 0x42, 0x44, 0x52, 0x28, 0x16, 0x15, 0x20, 0x23);
    panel_dsi_dcs_send_seq(panel, 0xF0, 0x3C);
    panel_dsi_dcs_send_seq(panel, 0xF0, 0x69);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x21, 0x00);
    aic_delay_ms(120);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x00);
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

static struct display_timing st7797_timing = {
    .pixelclock = 10 * 1000 * 1000,
    .hactive = 320,
    .hfront_porch = 40,
    .hback_porch = 10,
    .hsync_len = 10,
    .vactive = 480,
    .vfront_porch = 30,
    .vback_porch = 10,
    .vsync_len = 10,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB666,
    .lane_num = 1,
};

struct aic_panel dsi_st7797 = {
    .name = "panel-st7797",
    .timings = &st7797_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

