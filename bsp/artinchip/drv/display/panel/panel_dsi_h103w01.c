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
    aic_delay_ms(120);
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
    // panel_dsi_dcs_send_seq(panel,0xF0,0x00);//0E BIST
    // panel_dsi_dcs_send_seq(panel,0xF6,0xC0);

    // panel_dsi_dcs_send_seq(panel, 0x20, 0xA0);//3LANE
    panel_dsi_dcs_send_seq(panel, 0x25, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x28, 0x8F);
    panel_dsi_dcs_send_seq(panel, 0x29, 0xC5);
    panel_dsi_dcs_send_seq(panel, 0x2a, 0x9F);
    panel_dsi_dcs_send_seq(panel, 0x37, 0xAC);
    panel_dsi_dcs_send_seq(panel, 0x38, 0xB7);
    panel_dsi_dcs_send_seq(panel, 0x39, 0x20);//VCOM
    panel_dsi_dcs_send_seq(panel, 0x44, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x49, 0x1C);
    panel_dsi_dcs_send_seq(panel, 0x59, 0xfe);
    panel_dsi_dcs_send_seq(panel, 0x5c, 0x00);

    // panel_dsi_dcs_send_seq(panel,0x80,0x20);//2power
    // panel_dsi_dcs_send_seq(panel,0x91,0x77);
    // panel_dsi_dcs_send_seq(panel,0x92,0x77);

    panel_dsi_dcs_send_seq(panel, 0x99, 0x51);
    panel_dsi_dcs_send_seq(panel, 0x9B, 0x59);
    panel_dsi_dcs_send_seq(panel, 0xA0, 0x55);
    panel_dsi_dcs_send_seq(panel, 0xA1, 0x50);
    panel_dsi_dcs_send_seq(panel, 0xA3, 0x58);
    panel_dsi_dcs_send_seq(panel, 0xA4, 0x9C);
    panel_dsi_dcs_send_seq(panel, 0xA7, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xA8, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xA9, 0x21);
    panel_dsi_dcs_send_seq(panel, 0xAA, 0xFC);
    panel_dsi_dcs_send_seq(panel, 0xAB, 0x28);
    panel_dsi_dcs_send_seq(panel, 0xAC, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xAD, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xAE, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xAF, 0x03);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x28);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x28);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x03);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xC3, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x2C, 0x22);

    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0xD1, 0x15);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x32);
    panel_dsi_dcs_send_seq(panel, 0xD4, 0x2E);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xD2, 0x15);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x32);
    panel_dsi_dcs_send_seq(panel, 0xD3, 0x2E);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0x23);
    panel_dsi_dcs_send_seq(panel, 0xD6, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x41);
    panel_dsi_dcs_send_seq(panel, 0xD7, 0x3B);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x07);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0x07);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xD8, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0xB9, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xD9, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xBD, 0x13);
    panel_dsi_dcs_send_seq(panel, 0xDD, 0x15);
    panel_dsi_dcs_send_seq(panel, 0xBC, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xDC, 0x14);
    panel_dsi_dcs_send_seq(panel, 0xBB, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xDB, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xBA, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xDA, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xBE, 0x16);
    panel_dsi_dcs_send_seq(panel, 0xDE, 0x18);
    panel_dsi_dcs_send_seq(panel, 0xBF, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xDF, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x15);
    panel_dsi_dcs_send_seq(panel, 0xE0, 0x17);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x37);
    panel_dsi_dcs_send_seq(panel, 0xD5, 0x32);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xD0, 0x02);

    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x07, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x08, 0x8A);
    panel_dsi_dcs_send_seq(panel, 0x09, 0x8B);
    panel_dsi_dcs_send_seq(panel, 0x0a, 0x88);
    panel_dsi_dcs_send_seq(panel, 0x0b, 0x89);
    panel_dsi_dcs_send_seq(panel, 0x2A, 0xA3);
    panel_dsi_dcs_send_seq(panel, 0x2B, 0xA3);
    panel_dsi_dcs_send_seq(panel, 0x34, 0x61);
    panel_dsi_dcs_send_seq(panel, 0x35, 0xD4);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x24);
    panel_dsi_dcs_send_seq(panel, 0x37, 0x13);
    panel_dsi_dcs_send_seq(panel, 0x40, 0x86);
    panel_dsi_dcs_send_seq(panel, 0x41, 0x87);
    panel_dsi_dcs_send_seq(panel, 0x42, 0x84);
    panel_dsi_dcs_send_seq(panel, 0x43, 0x85);
    panel_dsi_dcs_send_seq(panel, 0x45, 0x9E);
    panel_dsi_dcs_send_seq(panel, 0x46, 0x9D);
    panel_dsi_dcs_send_seq(panel, 0x48, 0xA0);
    panel_dsi_dcs_send_seq(panel, 0x49, 0x9F);
    panel_dsi_dcs_send_seq(panel, 0x50, 0x82);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x83);
    panel_dsi_dcs_send_seq(panel, 0x52, 0x80);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x81);
    panel_dsi_dcs_send_seq(panel, 0x55, 0xa2);
    panel_dsi_dcs_send_seq(panel, 0x56, 0xa1);
    panel_dsi_dcs_send_seq(panel, 0x58, 0xa4);
    panel_dsi_dcs_send_seq(panel, 0x59, 0xa3);
    panel_dsi_dcs_send_seq(panel, 0x7E, 0x78);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0x81, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0x82, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0x83, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0x84, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x85, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x86, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x87, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x88, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x89, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x8A, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x8B, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x8C, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x8D, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x8E, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x8F, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0x90, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0x91, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0x92, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0x93, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x94, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x96, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x97, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0x98, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0x99, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0x9a, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x9B, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x9C, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x9D, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x9E, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x9F, 0x04);
    panel_dsi_dcs_send_seq(panel, 0xA0, 0x04);
    panel_dsi_dcs_send_seq(panel, 0xA1, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xA2, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xA3, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0xA4, 0x0f);
    panel_dsi_dcs_send_seq(panel, 0xA5, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0xA6, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0xA7, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0xa8, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0xa9, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xaA, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xE0, 0x02);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xff, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xff, 0x52);
    panel_dsi_dcs_send_seq(panel, 0xff, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x02);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x2c);

    panel_dsi_dcs_send_seq(panel, 0x11);
    aic_delay_ms(120);

    panel_dsi_dcs_send_seq(panel, 0x29);
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

static struct display_timing h103w01_timing = {
    .pixelclock = 72 * 1000 * 1000,
    .hactive = 540,
    .hfront_porch = 160,
    .hback_porch = 60,
    .hsync_len = 40,
    .vactive = 1440,
    .vfront_porch = 40,
    .vback_porch = 10,
    .vsync_len = 2,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h103w01 = {
    .name = "panel-h103w01",
    .timings = &h103w01_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

