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
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel) {
    int ret;

    panel_gpio_init(panel);

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);



    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30); //Register, Parameters
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x49);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x01);
    panel_dsi_dcs_send_seq(panel, 0xe2, 0x00);
    //panel_dsi_dcs_send_seq(panel,0xF1,0x0E);//BIST
    panel_dsi_dcs_send_seq(panel, 0x14, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x11, 0x10);
    //lvd
    panel_dsi_dcs_send_seq(panel, 0x3b, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x40, 0x0b);
    panel_dsi_dcs_send_seq(panel, 0x41, 0x32);
    panel_dsi_dcs_send_seq(panel, 0x42, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x43, 0x40);
    panel_dsi_dcs_send_seq(panel, 0x45, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x46, 0xE0);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x3C);
    panel_dsi_dcs_send_seq(panel, 0x52, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x53, 0x22);
    panel_dsi_dcs_send_seq(panel, 0x54, 0x6F);
    panel_dsi_dcs_send_seq(panel, 0x55, 0x6F);
    panel_dsi_dcs_send_seq(panel, 0x56, 0x10);
    panel_dsi_dcs_send_seq(panel, 0x57, 0x37);
    panel_dsi_dcs_send_seq(panel, 0x59, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x5A, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x5b, 0x82);
    panel_dsi_dcs_send_seq(panel, 0x5C, 0x6C);
    panel_dsi_dcs_send_seq(panel, 0x5D, 0x80);
    panel_dsi_dcs_send_seq(panel, 0x79, 0xfe);
    panel_dsi_dcs_send_seq(panel, 0x7D, 0x01);
    panel_dsi_dcs_send_seq(panel, 0x90, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x91, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x94, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x96, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xc8, 0x37);
    panel_dsi_dcs_send_seq(panel, 0xca, 0x8C);
    panel_dsi_dcs_send_seq(panel, 0xcc, 0x21);//VCOM
    panel_dsi_dcs_send_seq(panel, 0xA0, 0x39);
    panel_dsi_dcs_send_seq(panel, 0xA1, 0x29);
    panel_dsi_dcs_send_seq(panel, 0xA2, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xA8, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xA9, 0x19);
    panel_dsi_dcs_send_seq(panel, 0xAA, 0x17);
    panel_dsi_dcs_send_seq(panel, 0xAB, 0x14);
    panel_dsi_dcs_send_seq(panel, 0xA3, 0x41);
    panel_dsi_dcs_send_seq(panel, 0xAC, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xAD, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xA4, 0x41);
    panel_dsi_dcs_send_seq(panel, 0xAE, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xAF, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xA5, 0x17);
    panel_dsi_dcs_send_seq(panel, 0xA6, 0x09);
    panel_dsi_dcs_send_seq(panel, 0xA7, 0x03);
    panel_dsi_dcs_send_seq(panel, 0xB4, 0x39);
    panel_dsi_dcs_send_seq(panel, 0xB5, 0x2A);
    panel_dsi_dcs_send_seq(panel, 0xB6, 0x26);
    panel_dsi_dcs_send_seq(panel, 0xBC, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xBD, 0x1E);
    panel_dsi_dcs_send_seq(panel, 0xBE, 0x1D);
    panel_dsi_dcs_send_seq(panel, 0xBF, 0x19);
    panel_dsi_dcs_send_seq(panel, 0xB7, 0x46);
    panel_dsi_dcs_send_seq(panel, 0xC0, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xC1, 0x14);
    panel_dsi_dcs_send_seq(panel, 0xB8, 0x46);
    panel_dsi_dcs_send_seq(panel, 0xC2, 0x12);
    panel_dsi_dcs_send_seq(panel, 0xC3, 0x13);
    panel_dsi_dcs_send_seq(panel, 0xC4, 0x16);
    panel_dsi_dcs_send_seq(panel, 0xC5, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0xB9, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xBA, 0x14);
    panel_dsi_dcs_send_seq(panel, 0xBB, 0x03);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x49);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x03);
    panel_dsi_dcs_send_seq(panel, 0x00, 0x11);
    panel_dsi_dcs_send_seq(panel, 0x01, 0xE9);
    panel_dsi_dcs_send_seq(panel, 0x02, 0xEC);
    panel_dsi_dcs_send_seq(panel, 0x27, 0x88);
    panel_dsi_dcs_send_seq(panel, 0x28, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0x29, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x36, 0x88);
    panel_dsi_dcs_send_seq(panel, 0x37, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0x38, 0x07);
    panel_dsi_dcs_send_seq(panel, 0x3C, 0x88);
    panel_dsi_dcs_send_seq(panel, 0x3D, 0x09);
    panel_dsi_dcs_send_seq(panel, 0x3E, 0x06);
    panel_dsi_dcs_send_seq(panel, 0x48, 0x88);
    panel_dsi_dcs_send_seq(panel, 0x49, 0x08);
    panel_dsi_dcs_send_seq(panel, 0x4A, 0x05);
    panel_dsi_dcs_send_seq(panel, 0x4E, 0x11);
    panel_dsi_dcs_send_seq(panel, 0x4F, 0xE9);
    panel_dsi_dcs_send_seq(panel, 0x50, 0xED);
    panel_dsi_dcs_send_seq(panel, 0x5D, 0x11);
    panel_dsi_dcs_send_seq(panel, 0x5E, 0xE9);
    panel_dsi_dcs_send_seq(panel, 0x5F, 0xED);
    panel_dsi_dcs_send_seq(panel, 0x51, 0x04);
    panel_dsi_dcs_send_seq(panel, 0x7B, 0x30);
    panel_dsi_dcs_send_seq(panel, 0x80, 0x87);
    panel_dsi_dcs_send_seq(panel, 0x81, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x82, 0xE1);
    panel_dsi_dcs_send_seq(panel, 0x83, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x84, 0x70);
    panel_dsi_dcs_send_seq(panel, 0x85, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x88, 0x86);
    panel_dsi_dcs_send_seq(panel, 0x89, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x8A, 0xE2);
    panel_dsi_dcs_send_seq(panel, 0x8B, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x8C, 0x70);
    panel_dsi_dcs_send_seq(panel, 0x8D, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x8E, 0x85);
    panel_dsi_dcs_send_seq(panel, 0x8F, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x90, 0xE3);
    panel_dsi_dcs_send_seq(panel, 0x91, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x92, 0x70);
    panel_dsi_dcs_send_seq(panel, 0x93, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x94, 0x84);
    panel_dsi_dcs_send_seq(panel, 0x95, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x96, 0xE4);
    panel_dsi_dcs_send_seq(panel, 0x97, 0x33);
    panel_dsi_dcs_send_seq(panel, 0x98, 0x70);
    panel_dsi_dcs_send_seq(panel, 0x99, 0x00);
    panel_dsi_dcs_send_seq(panel, 0x9A, 0x83);
    panel_dsi_dcs_send_seq(panel, 0x9B, 0x31);
    panel_dsi_dcs_send_seq(panel, 0x9C, 0xE5);
    panel_dsi_dcs_send_seq(panel, 0x9D, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0x9E, 0x70);
    panel_dsi_dcs_send_seq(panel, 0x9F, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xA2, 0x82);
    panel_dsi_dcs_send_seq(panel, 0xA3, 0x31);
    panel_dsi_dcs_send_seq(panel, 0xA4, 0xE6);
    panel_dsi_dcs_send_seq(panel, 0xA5, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xA6, 0x70);
    panel_dsi_dcs_send_seq(panel, 0xA7, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xA8, 0x81);
    panel_dsi_dcs_send_seq(panel, 0xA9, 0x31);
    panel_dsi_dcs_send_seq(panel, 0xAA, 0xE7);
    panel_dsi_dcs_send_seq(panel, 0xAB, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xAC, 0x70);
    panel_dsi_dcs_send_seq(panel, 0xAD, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xAE, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xAF, 0x31);
    panel_dsi_dcs_send_seq(panel, 0xB0, 0xE8);
    panel_dsi_dcs_send_seq(panel, 0xB1, 0x33);
    panel_dsi_dcs_send_seq(panel, 0xB2, 0x70);
    panel_dsi_dcs_send_seq(panel, 0xB3, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xF7, 0x40);
    panel_dsi_dcs_send_seq(panel, 0xF8, 0x40);
    panel_dsi_dcs_send_seq(panel, 0xD0, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xD1, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xD2, 0x0F);
    panel_dsi_dcs_send_seq(panel, 0xD3, 0x11);
    panel_dsi_dcs_send_seq(panel, 0xD4, 0x0B);
    panel_dsi_dcs_send_seq(panel, 0xD5, 0x0D);
    panel_dsi_dcs_send_seq(panel, 0xD6, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xD7, 0x09);
    panel_dsi_dcs_send_seq(panel, 0xD8, 0x05);
    panel_dsi_dcs_send_seq(panel, 0xD9, 0x07);
    panel_dsi_dcs_send_seq(panel, 0xDA, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xDB, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xDC, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0xDD, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xDE, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xDF, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xE0, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xE1, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xE2, 0x1B);
    panel_dsi_dcs_send_seq(panel, 0xE3, 0x1A);
    panel_dsi_dcs_send_seq(panel, 0xE4, 0x08);
    panel_dsi_dcs_send_seq(panel, 0xE5, 0x00);
    panel_dsi_dcs_send_seq(panel, 0xE6, 0x06);
    panel_dsi_dcs_send_seq(panel, 0xE7, 0x04);
    panel_dsi_dcs_send_seq(panel, 0xE8, 0x09);
    panel_dsi_dcs_send_seq(panel, 0xE9, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xEA, 0x0C);
    panel_dsi_dcs_send_seq(panel, 0xEB, 0x0A);
    panel_dsi_dcs_send_seq(panel, 0xEC, 0x10);
    panel_dsi_dcs_send_seq(panel, 0xED, 0x0E);
    panel_dsi_dcs_send_seq(panel, 0xEE, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xEF, 0x1F);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x30);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x49);
    panel_dsi_dcs_send_seq(panel, 0xFF, 0x00);
    ////panel_dsi_dcs_send_seq(panel,0xF1,0xd2);//testen
    
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

static struct display_timing h034a03_timing = {
    .pixelclock = 18 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 44,
    .hback_porch = 46,
    .hsync_len = 2,
    .vactive = 480,
    .vfront_porch = 16,
    .vback_porch = 14,
    .vsync_len = 2,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 2,
};

struct aic_panel dsi_h034a03 = {
    .name = "panel-h034a03",
    .timings = &h034a03_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

