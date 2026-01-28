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
    aic_delay_ms(200);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(200);
}

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x28);
    panel_dsi_generic_send_seq(panel, 0xF2, 0x00, 0x28);
    panel_dsi_generic_send_seq(panel, 0x73, 0x00, 0xF0);
    panel_dsi_generic_send_seq(panel, 0x7C, 0x00, 0xD1);
    panel_dsi_generic_send_seq(panel, 0x83, 0x00, 0xE0);
    panel_dsi_generic_send_seq(panel, 0x84, 0x00, 0x61);
    panel_dsi_generic_send_seq(panel, 0xF2, 0x00, 0x82);
    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0xF1, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0xB0, 0x00, 0x50);
    panel_dsi_generic_send_seq(panel, 0xB1, 0x00, 0x23);
    panel_dsi_generic_send_seq(panel, 0xB2, 0x00, 0x36);
    panel_dsi_generic_send_seq(panel, 0xB3, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0xB4, 0x00, 0x06);
    panel_dsi_generic_send_seq(panel, 0xB5, 0x00, 0x24);
    panel_dsi_generic_send_seq(panel, 0xB6, 0x00, 0xA5);
    panel_dsi_generic_send_seq(panel, 0xB7, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xB8, 0x00, 0x8C);
    panel_dsi_generic_send_seq(panel, 0xB9, 0x00, 0x15);
    panel_dsi_generic_send_seq(panel, 0xBA, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xBB, 0x00, 0x08);
    panel_dsi_generic_send_seq(panel, 0xBC, 0x00, 0x08);
    panel_dsi_generic_send_seq(panel, 0xBD, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xBE, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xBF, 0x00, 0x07);
    panel_dsi_generic_send_seq(panel, 0xC0, 0x00, 0x80);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xC2, 0x00, 0x37);
    panel_dsi_generic_send_seq(panel, 0xC3, 0x00, 0x80);
    panel_dsi_generic_send_seq(panel, 0xC4, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xC5, 0x00, 0x37);
    panel_dsi_generic_send_seq(panel, 0xC6, 0x00, 0xA9);
    panel_dsi_generic_send_seq(panel, 0xC7, 0x00, 0x41);
    panel_dsi_generic_send_seq(panel, 0xC8, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0xC9, 0x00, 0xA9);
    panel_dsi_generic_send_seq(panel, 0xCA, 0x00, 0x41);
    panel_dsi_generic_send_seq(panel, 0xCB, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0xCC, 0x00, 0x7F);
    panel_dsi_generic_send_seq(panel, 0xCD, 0x00, 0x7F);
    panel_dsi_generic_send_seq(panel, 0xCE, 0x00, 0xFF);
    panel_dsi_generic_send_seq(panel, 0xD0, 0x00, 0x91);
    panel_dsi_generic_send_seq(panel, 0xD1, 0x00, 0x68);
    panel_dsi_generic_send_seq(panel, 0xD2, 0x00, 0x68);
    panel_dsi_generic_send_seq(panel, 0xF5, 0x00, 0x00, 0xA5);
    panel_dsi_generic_send_seq(panel, 0xF1, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0xe0, 0x00, 0xf0, 0x00, 0x0B, 0x00, 0x12, 0x00, 0x0B, 0x00, 0x0A, 0x00, 0x06, 0x00, 0x39, 0x00, 0x43, 0x00, 0x4F, 0x00, 0x07, 0x00, 0x14, 0x00, 0x14, 0x00, 0x2f, 0x00, 0x34);
    panel_dsi_generic_send_seq(panel, 0xe1, 0x00, 0xf0, 0x00, 0x0B, 0x00, 0x11, 0x00, 0x0A, 0x00, 0x09, 0x00, 0x05, 0x00, 0x32, 0x00, 0x33, 0x00, 0x48, 0x00, 0x07, 0x00, 0x13, 0x00, 0x13, 0x00, 0x2C, 0x00, 0x33);
    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xF3, 0x00, 0x10);
    panel_dsi_generic_send_seq(panel, 0xE0, 0x00, 0x0A);
    panel_dsi_generic_send_seq(panel, 0xE1, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE2, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE3, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE4, 0x00, 0xE0);
    panel_dsi_generic_send_seq(panel, 0xE5, 0x00, 0x06);
    panel_dsi_generic_send_seq(panel, 0xE6, 0x00, 0x21);
    panel_dsi_generic_send_seq(panel, 0xE7, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE8, 0x00, 0x05);
    panel_dsi_generic_send_seq(panel, 0xE9, 0x00, 0xF2);
    panel_dsi_generic_send_seq(panel, 0xEA, 0x00, 0xDF);
    panel_dsi_generic_send_seq(panel, 0xEB, 0x00, 0x80);
    panel_dsi_generic_send_seq(panel, 0xEC, 0x00, 0x20);
    panel_dsi_generic_send_seq(panel, 0xED, 0x00, 0x14);
    panel_dsi_generic_send_seq(panel, 0xEE, 0x00, 0xFF);
    panel_dsi_generic_send_seq(panel, 0xEF, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xF8, 0x00, 0xFF);
    panel_dsi_generic_send_seq(panel, 0xF9, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFA, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFB, 0x00, 0x30);
    panel_dsi_generic_send_seq(panel, 0xFC, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFD, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFE, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xFF, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x60, 0x00, 0x42);
    panel_dsi_generic_send_seq(panel, 0x61, 0x00, 0xE0);
    panel_dsi_generic_send_seq(panel, 0x62, 0x00, 0x40);
    panel_dsi_generic_send_seq(panel, 0x63, 0x00, 0x40);
    panel_dsi_generic_send_seq(panel, 0x64, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x65, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x66, 0x00, 0x40);
    panel_dsi_generic_send_seq(panel, 0x67, 0x00, 0x03);
    panel_dsi_generic_send_seq(panel, 0x68, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x69, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x6A, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x6B, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x70, 0x00, 0x42);
    panel_dsi_generic_send_seq(panel, 0x71, 0x00, 0xE0);
    panel_dsi_generic_send_seq(panel, 0x72, 0x00, 0x40);
    panel_dsi_generic_send_seq(panel, 0x73, 0x00, 0x40);
    panel_dsi_generic_send_seq(panel, 0x74, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x75, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x76, 0x00, 0x40);
    panel_dsi_generic_send_seq(panel, 0x77, 0x00, 0x03);
    panel_dsi_generic_send_seq(panel, 0x78, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x79, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x7A, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x7B, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x80, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0x81, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x82, 0x00, 0x05);
    panel_dsi_generic_send_seq(panel, 0x83, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x84, 0x00, 0xDD);
    panel_dsi_generic_send_seq(panel, 0x85, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x86, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x87, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x88, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0x89, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x8A, 0x00, 0x07);
    panel_dsi_generic_send_seq(panel, 0x8B, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x8C, 0x00, 0xDF);
    panel_dsi_generic_send_seq(panel, 0x8D, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x8E, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x8F, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x90, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0x91, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x92, 0x00, 0x09);
    panel_dsi_generic_send_seq(panel, 0x93, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x94, 0x00, 0xE1);
    panel_dsi_generic_send_seq(panel, 0x95, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x96, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x97, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x98, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0x99, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x9A, 0x00, 0x0B);
    panel_dsi_generic_send_seq(panel, 0x9B, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0x9C, 0x00, 0xE3);
    panel_dsi_generic_send_seq(panel, 0x9D, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x9E, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0x9F, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xA0, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0xA1, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xA2, 0x00, 0x04);
    panel_dsi_generic_send_seq(panel, 0xA3, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0xA4, 0x00, 0xDC);
    panel_dsi_generic_send_seq(panel, 0xA5, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xA6, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xA7, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xA8, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0xA9, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xAA, 0x00, 0x06);
    panel_dsi_generic_send_seq(panel, 0xAB, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0xAC, 0x00, 0xDE);
    panel_dsi_generic_send_seq(panel, 0xAD, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xAE, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xAF, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xB0, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0xB1, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xB2, 0x00, 0x08);
    panel_dsi_generic_send_seq(panel, 0xB3, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0xB4, 0x00, 0xE0);
    panel_dsi_generic_send_seq(panel, 0xB5, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xB6, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xB7, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xB8, 0x00, 0x48);
    panel_dsi_generic_send_seq(panel, 0xB9, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xBA, 0x00, 0x0A);
    panel_dsi_generic_send_seq(panel, 0xBB, 0x00, 0x02);
    panel_dsi_generic_send_seq(panel, 0xBC, 0x00, 0xE2);
    panel_dsi_generic_send_seq(panel, 0xBD, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xBE, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xBF, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xC0, 0x00, 0x22);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x00, 0x98);
    panel_dsi_generic_send_seq(panel, 0xC2, 0x00, 0x65);
    panel_dsi_generic_send_seq(panel, 0xC3, 0x00, 0x74);
    panel_dsi_generic_send_seq(panel, 0xC4, 0x00, 0x47);
    panel_dsi_generic_send_seq(panel, 0xC5, 0x00, 0x56);
    panel_dsi_generic_send_seq(panel, 0xC6, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xC7, 0x00, 0xBA);
    panel_dsi_generic_send_seq(panel, 0xC8, 0x00, 0xAB);
    panel_dsi_generic_send_seq(panel, 0xC9, 0x00, 0x33);
    panel_dsi_generic_send_seq(panel, 0xD0, 0x00, 0x11);
    panel_dsi_generic_send_seq(panel, 0xD1, 0x00, 0x98);
    panel_dsi_generic_send_seq(panel, 0xD2, 0x00, 0x65);
    panel_dsi_generic_send_seq(panel, 0xD3, 0x00, 0x74);
    panel_dsi_generic_send_seq(panel, 0xD4, 0x00, 0x47);
    panel_dsi_generic_send_seq(panel, 0xD5, 0x00, 0x56);
    panel_dsi_generic_send_seq(panel, 0xD6, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xD7, 0x00, 0xBA);
    panel_dsi_generic_send_seq(panel, 0xD8, 0x00, 0xAB);
    panel_dsi_generic_send_seq(panel, 0xD9, 0x00, 0x33);
    panel_dsi_generic_send_seq(panel, 0xF3, 0x00, 0x01);
    panel_dsi_generic_send_seq(panel, 0xF0, 0x00, 0x00);

    panel_dsi_generic_send_seq(panel, 0x21, 0x00);

    panel_dsi_generic_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(120);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    aic_delay_ms(50);
    // panel_dsi_generic_send_seq(panel, 0x1C, 0x00);
    // aic_delay_ms(50);

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

static struct display_timing h013a08_timing = {
    .pixelclock = 10 * 1000 * 1000,
    .hactive = 360,
    .hfront_porch = 10,
    .hback_porch = 10,
    .hsync_len = 10,
    .vactive = 360,
    .vfront_porch = 20,
    .vback_porch = 30,
    .vsync_len = 30,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 1,
};

struct aic_panel dsi_h013a08 = {
    .name = "panel-h013a08",
    .timings = &h013a08_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

