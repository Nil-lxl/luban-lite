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
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(200);
}

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xE3, 0x00);
    // panel_dsi_dcs_send_seq(panel, 0x20, 0x90); //默认4L  90:2Lane  A0:3Lane
    panel_dsi_dcs_send_seq(panel, 0x25, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x28, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x2a, 0xFF);
    panel_dsi_dcs_send_seq(panel, 0x30, 0x58); //V1
    panel_dsi_dcs_send_seq(panel, 0x37, 0x9C);
    panel_dsi_dcs_send_seq(panel, 0x38, 0xA7);
    panel_dsi_dcs_send_seq(panel, 0x39, 0x27);//VCOM
    panel_dsi_dcs_send_seq(panel, 0x44, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x49, 0x3C);
    panel_dsi_dcs_send_seq(panel, 0x59, 0xFE);
    panel_dsi_dcs_send_seq(panel, 0x5c, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x20);
    panel_dsi_dcs_send_seq(panel, 0x91, 0x77);
    panel_dsi_dcs_send_seq(panel, 0x92, 0x77);
    panel_dsi_dcs_send_seq(panel, 0x99, 0x54);
    panel_dsi_dcs_send_seq(panel, 0x9B, 0x56);
    panel_dsi_dcs_send_seq(panel, 0xA0, 0x55);
    panel_dsi_dcs_send_seq(panel, 0xA1, 0x50);
    panel_dsi_dcs_send_seq(panel, 0xA3, 0x58); //V1
    panel_dsi_dcs_send_seq(panel, 0xA4, 0x9C);
    panel_dsi_dcs_send_seq(panel, 0xA7, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xA8, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xA9, 0x21);
    panel_dsi_dcs_send_seq(panel, 0xAA, 0xfc);
    panel_dsi_dcs_send_seq(panel, 0xAB, 0x28);
    panel_dsi_dcs_send_seq(panel, 0xAC, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xAD, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xAE, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xAF, 0x03);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x28);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x28);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xc0, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xc1, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xc3, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0x2c, 0x22);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x2B);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x2D);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x37);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0x18);
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x34);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0xB9, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xBA, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xBB, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xBC, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xBD, 0x13);
    panel_dsi_dcs_send_seq(panel, 0xBE, 0x18);
    panel_dsi_dcs_send_seq(panel, 0xBF, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x17);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xD0, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xD1, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xD2, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xD3, 0x25);
    panel_dsi_dcs_send_seq(panel, 0xD4, 0x29);
    panel_dsi_dcs_send_seq(panel, 0xD5, 0x32);
    panel_dsi_dcs_send_seq(panel, 0xD6, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xD7, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xD8, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0xD9, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xDA, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xDB, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xDC, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xDD, 0x13);
    panel_dsi_dcs_send_seq(panel, 0xDE, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0xDF, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xE0, 0x19);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x08, 0x85);
    panel_dsi_dcs_send_seq(panel, 0x09, 0x86);
    panel_dsi_dcs_send_seq(panel, 0x25, 0x32);
    panel_dsi_dcs_send_seq(panel, 0x27, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x2A, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x2B, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x7E, 0x3C);
    panel_dsi_dcs_send_seq(panel, 0x34, 0xb1);
    panel_dsi_dcs_send_seq(panel, 0x35, 0x12);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x37, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x40, 0x82);
    panel_dsi_dcs_send_seq(panel, 0x41, 0x83);
    panel_dsi_dcs_send_seq(panel, 0x42, 0x80);
    panel_dsi_dcs_send_seq(panel, 0x43, 0x81);
    panel_dsi_dcs_send_seq(panel, 0x45, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x46, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x48, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x49, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x50, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x52, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x55, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x56, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x58, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x59, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x0e);
    panel_dsi_dcs_send_seq(panel, 0x81, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0x82, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0x83, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0x84, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x85, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x86, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x87, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x88, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x8F, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x96, 0x0e);
    panel_dsi_dcs_send_seq(panel, 0x97, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0x98, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0x99, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0x9A, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x9B, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x9C, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x9D, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x9E, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xA5, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x2C);

    panel_dsi_dcs_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(200);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x00);
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

static struct display_timing h080b7_timing = {
    .pixelclock = 68 * 1000 * 1000,
    .hactive = 800,
    .hfront_porch = 80,
    .hback_porch = 40,
    .hsync_len = 10,
    .vactive = 1280,
    .vfront_porch = 16,
    .vback_porch = 14,
    .vsync_len = 2,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h080b7 = {
    .name = "panel-h080b7",
    .timings = &h080b7_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

