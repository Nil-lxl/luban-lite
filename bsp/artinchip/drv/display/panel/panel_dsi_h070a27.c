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

    panel_dsi_generic_send_seq(panel, 0x00, 0x68);//Page 6
    panel_dsi_generic_send_seq(panel, 0x0F, 0xAA);
    panel_dsi_generic_send_seq(panel, 0x00, 0x08);//page 0
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0x00);
    panel_dsi_generic_send_seq(panel, 0x03, 0x67);
    panel_dsi_generic_send_seq(panel, 0x04, 0x10);
    panel_dsi_generic_send_seq(panel, 0x05, 0x40);
    panel_dsi_generic_send_seq(panel, 0x06, 0x38);
    panel_dsi_generic_send_seq(panel, 0x07, 0x40);
    panel_dsi_generic_send_seq(panel, 0x08, 0x0E);
    panel_dsi_generic_send_seq(panel, 0x09, 0x14);
    panel_dsi_generic_send_seq(panel, 0x0a, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x18);
    panel_dsi_generic_send_seq(panel, 0x0c, 0x18);
    panel_dsi_generic_send_seq(panel, 0x0d, 0xAA);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x00);
    panel_dsi_generic_send_seq(panel, 0x10, 0x4E);
    panel_dsi_generic_send_seq(panel, 0x11, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x12, 0x00);
    panel_dsi_generic_send_seq(panel, 0x13, 0x00);
    panel_dsi_generic_send_seq(panel, 0x14, 0x60);
    panel_dsi_generic_send_seq(panel, 0x15, 0x46);
    panel_dsi_generic_send_seq(panel, 0x16, 0x80);
    panel_dsi_generic_send_seq(panel, 0x17, 0x00);
    panel_dsi_generic_send_seq(panel, 0x18, 0x80);
    panel_dsi_generic_send_seq(panel, 0x19, 0x00);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x80);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x1c, 0x24);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x09);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x18);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x00);
    panel_dsi_generic_send_seq(panel, 0x20, 0x47);
    panel_dsi_generic_send_seq(panel, 0x21, 0x00);
    panel_dsi_generic_send_seq(panel, 0x22, 0x00);
    panel_dsi_generic_send_seq(panel, 0x23, 0x00);
    panel_dsi_generic_send_seq(panel, 0x00, 0x18);//page 1
    panel_dsi_generic_send_seq(panel, 0x01, 0x66);
    panel_dsi_generic_send_seq(panel, 0x02, 0x02);
    panel_dsi_generic_send_seq(panel, 0x03, 0x45);
    panel_dsi_generic_send_seq(panel, 0x04, 0xCA);
    panel_dsi_generic_send_seq(panel, 0x05, 0x24);
    panel_dsi_generic_send_seq(panel, 0x06, 0x18);
    panel_dsi_generic_send_seq(panel, 0x07, 0x79);
    panel_dsi_generic_send_seq(panel, 0x08, 0xB6);
    panel_dsi_generic_send_seq(panel, 0x09, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x7D);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x40);
    panel_dsi_generic_send_seq(panel, 0x0c, 0x91);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x24);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x50);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x00);
    panel_dsi_generic_send_seq(panel, 0x10, 0x0A);
    panel_dsi_generic_send_seq(panel, 0x11, 0x00);
    panel_dsi_generic_send_seq(panel, 0x12, 0x00);
    panel_dsi_generic_send_seq(panel, 0x13, 0x00);
    panel_dsi_generic_send_seq(panel, 0x14, 0x00);
    panel_dsi_generic_send_seq(panel, 0x15, 0x00);
    panel_dsi_generic_send_seq(panel, 0x16, 0x00);
    panel_dsi_generic_send_seq(panel, 0x17, 0x04);
    panel_dsi_generic_send_seq(panel, 0x18, 0x10);
    panel_dsi_generic_send_seq(panel, 0x19, 0x40);
    panel_dsi_generic_send_seq(panel, 0x1a, 0xC1);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x1c, 0x00);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x4F);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x07);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x63);
    panel_dsi_generic_send_seq(panel, 0x20, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x21, 0x20);
    panel_dsi_generic_send_seq(panel, 0x22, 0x2D);
    panel_dsi_generic_send_seq(panel, 0x23, 0x2F);
    panel_dsi_generic_send_seq(panel, 0x24, 0x01);
    panel_dsi_generic_send_seq(panel, 0x25, 0x04);
    panel_dsi_generic_send_seq(panel, 0x26, 0x00);
    panel_dsi_generic_send_seq(panel, 0x27, 0x63);
    panel_dsi_generic_send_seq(panel, 0x28, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2a, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2b, 0x3C);
    panel_dsi_generic_send_seq(panel, 0x2c, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2d, 0x0E);
    panel_dsi_generic_send_seq(panel, 0x2e, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2f, 0x37);
    panel_dsi_generic_send_seq(panel, 0x30, 0x00);
    panel_dsi_generic_send_seq(panel, 0x31, 0x1E);
    panel_dsi_generic_send_seq(panel, 0x32, 0x10);
    panel_dsi_generic_send_seq(panel, 0x33, 0x0F);
    panel_dsi_generic_send_seq(panel, 0x46, 0xC0);//page 1
    panel_dsi_generic_send_seq(panel, 0x47, 0x00);
    panel_dsi_generic_send_seq(panel, 0x48, 0x05);
    panel_dsi_generic_send_seq(panel, 0x49, 0x42);
    panel_dsi_generic_send_seq(panel, 0x4a, 0x02);
    panel_dsi_generic_send_seq(panel, 0x4b, 0x20);
    panel_dsi_generic_send_seq(panel, 0x4c, 0x82);
    panel_dsi_generic_send_seq(panel, 0x4d, 0x02);
    panel_dsi_generic_send_seq(panel, 0x4e, 0x02);
    panel_dsi_generic_send_seq(panel, 0x4f, 0x5A);
    panel_dsi_generic_send_seq(panel, 0x50, 0x15);
    panel_dsi_generic_send_seq(panel, 0x51, 0xC1);
    panel_dsi_generic_send_seq(panel, 0x52, 0x05);
    panel_dsi_generic_send_seq(panel, 0x00, 0x28);//page 2
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0x00);
    panel_dsi_generic_send_seq(panel, 0x03, 0x00);
    panel_dsi_generic_send_seq(panel, 0x04, 0x40);
    panel_dsi_generic_send_seq(panel, 0x05, 0x00);
    panel_dsi_generic_send_seq(panel, 0x06, 0x6A);
    panel_dsi_generic_send_seq(panel, 0x07, 0x00);
    panel_dsi_generic_send_seq(panel, 0x08, 0x8B);
    panel_dsi_generic_send_seq(panel, 0x09, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0a, 0xA7);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0c, 0xF9);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x01);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x34);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x01);
    panel_dsi_generic_send_seq(panel, 0x10, 0x62);
    panel_dsi_generic_send_seq(panel, 0x11, 0x01);
    panel_dsi_generic_send_seq(panel, 0x12, 0xAB);
    panel_dsi_generic_send_seq(panel, 0x13, 0x01);
    panel_dsi_generic_send_seq(panel, 0x14, 0xE9);
    panel_dsi_generic_send_seq(panel, 0x15, 0x02);
    panel_dsi_generic_send_seq(panel, 0x16, 0x24);
    panel_dsi_generic_send_seq(panel, 0x17, 0x02);
    panel_dsi_generic_send_seq(panel, 0x18, 0x6A);
    panel_dsi_generic_send_seq(panel, 0x19, 0x02);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x97);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x02);
    panel_dsi_generic_send_seq(panel, 0x1c, 0xD5);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x03);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x34);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x03);
    panel_dsi_generic_send_seq(panel, 0x20, 0x58);
    panel_dsi_generic_send_seq(panel, 0x21, 0x03);
    panel_dsi_generic_send_seq(panel, 0x22, 0x88);
    panel_dsi_generic_send_seq(panel, 0x23, 0x03);
    panel_dsi_generic_send_seq(panel, 0x24, 0xCB);
    panel_dsi_generic_send_seq(panel, 0x25, 0x03);
    panel_dsi_generic_send_seq(panel, 0x26, 0xFC);
    panel_dsi_generic_send_seq(panel, 0x00, 0x38);//page 3
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0x00);
    panel_dsi_generic_send_seq(panel, 0x03, 0x00);
    panel_dsi_generic_send_seq(panel, 0x04, 0x40);
    panel_dsi_generic_send_seq(panel, 0x05, 0x00);
    panel_dsi_generic_send_seq(panel, 0x06, 0x6A);
    panel_dsi_generic_send_seq(panel, 0x07, 0x00);
    panel_dsi_generic_send_seq(panel, 0x08, 0x8B);
    panel_dsi_generic_send_seq(panel, 0x09, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0a, 0xA7);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0c, 0xF9);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x01);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x34);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x01);
    panel_dsi_generic_send_seq(panel, 0x10, 0x62);
    panel_dsi_generic_send_seq(panel, 0x11, 0x01);
    panel_dsi_generic_send_seq(panel, 0x12, 0xAB);
    panel_dsi_generic_send_seq(panel, 0x13, 0x01);
    panel_dsi_generic_send_seq(panel, 0x14, 0xE9);
    panel_dsi_generic_send_seq(panel, 0x15, 0x02);
    panel_dsi_generic_send_seq(panel, 0x16, 0x24);
    panel_dsi_generic_send_seq(panel, 0x17, 0x02);
    panel_dsi_generic_send_seq(panel, 0x18, 0x6A);
    panel_dsi_generic_send_seq(panel, 0x19, 0x02);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x97);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x02);
    panel_dsi_generic_send_seq(panel, 0x1c, 0xD5);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x03);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x34);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x03);
    panel_dsi_generic_send_seq(panel, 0x20, 0x58);
    panel_dsi_generic_send_seq(panel, 0x21, 0x03);
    panel_dsi_generic_send_seq(panel, 0x22, 0x88);
    panel_dsi_generic_send_seq(panel, 0x23, 0x03);
    panel_dsi_generic_send_seq(panel, 0x24, 0xCB);
    panel_dsi_generic_send_seq(panel, 0x25, 0x03);
    panel_dsi_generic_send_seq(panel, 0x26, 0xFC);
    panel_dsi_generic_send_seq(panel, 0x00, 0x48);//page 4
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0x00);
    panel_dsi_generic_send_seq(panel, 0x03, 0x00);
    panel_dsi_generic_send_seq(panel, 0x04, 0x40);
    panel_dsi_generic_send_seq(panel, 0x05, 0x00);
    panel_dsi_generic_send_seq(panel, 0x06, 0x6A);
    panel_dsi_generic_send_seq(panel, 0x07, 0x00);
    panel_dsi_generic_send_seq(panel, 0x08, 0x8B);
    panel_dsi_generic_send_seq(panel, 0x09, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0a, 0xA7);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0c, 0xF9);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x01);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x34);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x01);
    panel_dsi_generic_send_seq(panel, 0x10, 0x62);
    panel_dsi_generic_send_seq(panel, 0x11, 0x01);
    panel_dsi_generic_send_seq(panel, 0x12, 0xAB);
    panel_dsi_generic_send_seq(panel, 0x13, 0x01);
    panel_dsi_generic_send_seq(panel, 0x14, 0xE9);
    panel_dsi_generic_send_seq(panel, 0x15, 0x02);
    panel_dsi_generic_send_seq(panel, 0x16, 0x24);
    panel_dsi_generic_send_seq(panel, 0x17, 0x02);
    panel_dsi_generic_send_seq(panel, 0x18, 0x6A);
    panel_dsi_generic_send_seq(panel, 0x19, 0x02);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x97);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x02);
    panel_dsi_generic_send_seq(panel, 0x1c, 0xD5);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x03);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x34);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x03);
    panel_dsi_generic_send_seq(panel, 0x20, 0x58);
    panel_dsi_generic_send_seq(panel, 0x21, 0x03);
    panel_dsi_generic_send_seq(panel, 0x22, 0x88);
    panel_dsi_generic_send_seq(panel, 0x23, 0x03);
    panel_dsi_generic_send_seq(panel, 0x24, 0xCB);
    panel_dsi_generic_send_seq(panel, 0x25, 0x03);
    panel_dsi_generic_send_seq(panel, 0x26, 0xFC);
    panel_dsi_generic_send_seq(panel, 0x00, 0x58);//page 5
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0xD9);
    panel_dsi_generic_send_seq(panel, 0x03, 0xD6);
    panel_dsi_generic_send_seq(panel, 0x04, 0x5F);
    panel_dsi_generic_send_seq(panel, 0x05, 0xD1);
    panel_dsi_generic_send_seq(panel, 0x06, 0x54);
    panel_dsi_generic_send_seq(panel, 0x07, 0x57);
    panel_dsi_generic_send_seq(panel, 0x08, 0x41);
    panel_dsi_generic_send_seq(panel, 0x09, 0x81);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x36);
    panel_dsi_generic_send_seq(panel, 0x0b, 0xB6);
    panel_dsi_generic_send_seq(panel, 0x0c, 0xDD);
    panel_dsi_generic_send_seq(panel, 0x0d, 0xAC);
    panel_dsi_generic_send_seq(panel, 0x0e, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x0f, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x10, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x11, 0x68);
    panel_dsi_generic_send_seq(panel, 0x12, 0x00);
    panel_dsi_generic_send_seq(panel, 0x13, 0x0A);
    panel_dsi_generic_send_seq(panel, 0x14, 0x0A);
    panel_dsi_generic_send_seq(panel, 0x15, 0x18);
    panel_dsi_generic_send_seq(panel, 0x16, 0x06);
    panel_dsi_generic_send_seq(panel, 0x17, 0x00);
    panel_dsi_generic_send_seq(panel, 0x18, 0x03);
    panel_dsi_generic_send_seq(panel, 0x19, 0x05);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x69);
    panel_dsi_generic_send_seq(panel, 0x1b, 0xD8);
    panel_dsi_generic_send_seq(panel, 0x1c, 0x3C);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x00);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x1E);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x10);
    panel_dsi_generic_send_seq(panel, 0x20, 0x00);
    panel_dsi_generic_send_seq(panel, 0x21, 0x00);
    panel_dsi_generic_send_seq(panel, 0x22, 0x00);
    panel_dsi_generic_send_seq(panel, 0x23, 0x00);
    panel_dsi_generic_send_seq(panel, 0x24, 0x00);
    panel_dsi_generic_send_seq(panel, 0x25, 0x00);
    panel_dsi_generic_send_seq(panel, 0x26, 0x00);
    panel_dsi_generic_send_seq(panel, 0x27, 0x00);
    panel_dsi_generic_send_seq(panel, 0x28, 0x00);
    panel_dsi_generic_send_seq(panel, 0x29, 0x0A);
    panel_dsi_generic_send_seq(panel, 0x2a, 0x0A);
    panel_dsi_generic_send_seq(panel, 0x2b, 0xAA);
    panel_dsi_generic_send_seq(panel, 0x2c, 0x01);
    panel_dsi_generic_send_seq(panel, 0x2d, 0x22);
    panel_dsi_generic_send_seq(panel, 0x2e, 0x00);
    panel_dsi_generic_send_seq(panel, 0x30, 0xCA);
    panel_dsi_generic_send_seq(panel, 0x31, 0x00);
    panel_dsi_generic_send_seq(panel, 0x32, 0x00);
    panel_dsi_generic_send_seq(panel, 0x00, 0x78);//page 7
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0x98);
    panel_dsi_generic_send_seq(panel, 0x03, 0xC6);
    panel_dsi_generic_send_seq(panel, 0x04, 0x5E);
    panel_dsi_generic_send_seq(panel, 0x05, 0x90);
    panel_dsi_generic_send_seq(panel, 0x06, 0x44);
    panel_dsi_generic_send_seq(panel, 0x07, 0x56);
    panel_dsi_generic_send_seq(panel, 0x08, 0x00);
    panel_dsi_generic_send_seq(panel, 0x09, 0x81);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x36);
    panel_dsi_generic_send_seq(panel, 0x0b, 0xB6);
    panel_dsi_generic_send_seq(panel, 0x0c, 0xDD);
    panel_dsi_generic_send_seq(panel, 0x0d, 0xAC);
    panel_dsi_generic_send_seq(panel, 0x0e, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x0f, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x10, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x11, 0x68);
    panel_dsi_generic_send_seq(panel, 0x12, 0x00);
    panel_dsi_generic_send_seq(panel, 0x13, 0xDD);
    panel_dsi_generic_send_seq(panel, 0x14, 0x00);
    panel_dsi_generic_send_seq(panel, 0x15, 0xDD);
    panel_dsi_generic_send_seq(panel, 0x16, 0x11);
    panel_dsi_generic_send_seq(panel, 0x17, 0x2D);
    panel_dsi_generic_send_seq(panel, 0x18, 0x2E);
    panel_dsi_generic_send_seq(panel, 0x19, 0xAD);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x4C);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x43);
    panel_dsi_generic_send_seq(panel, 0x1c, 0x04);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x10);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x33);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x33);
    panel_dsi_generic_send_seq(panel, 0x20, 0x00);
    panel_dsi_generic_send_seq(panel, 0x21, 0x00);
    panel_dsi_generic_send_seq(panel, 0x22, 0x00);
    panel_dsi_generic_send_seq(panel, 0x23, 0x03);
    panel_dsi_generic_send_seq(panel, 0x24, 0x03);
    panel_dsi_generic_send_seq(panel, 0x25, 0xB4);
    panel_dsi_generic_send_seq(panel, 0x26, 0x44);
    panel_dsi_generic_send_seq(panel, 0x27, 0xD2);
    panel_dsi_generic_send_seq(panel, 0x28, 0x0F);
    panel_dsi_generic_send_seq(panel, 0x29, 0x84);
    panel_dsi_generic_send_seq(panel, 0x00, 0x88);//page 8
    panel_dsi_generic_send_seq(panel, 0x09, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x18);
    panel_dsi_generic_send_seq(panel, 0x20, 0x03);
    panel_dsi_generic_send_seq(panel, 0x21, 0x28);
    panel_dsi_generic_send_seq(panel, 0x22, 0x02);
    panel_dsi_generic_send_seq(panel, 0x23, 0x82);
    panel_dsi_generic_send_seq(panel, 0x24, 0x33);
    panel_dsi_generic_send_seq(panel, 0x25, 0x03);
    panel_dsi_generic_send_seq(panel, 0x26, 0x14);
    panel_dsi_generic_send_seq(panel, 0x27, 0x86);
    panel_dsi_generic_send_seq(panel, 0x28, 0xFF);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2a, 0x00);
    panel_dsi_generic_send_seq(panel, 0x2B, 0x20);
    panel_dsi_generic_send_seq(panel, 0x2c, 0x30);
    panel_dsi_generic_send_seq(panel, 0x2d, 0xAA);
    panel_dsi_generic_send_seq(panel, 0x00, 0x98);//page 9
    panel_dsi_generic_send_seq(panel, 0x01, 0x89);
    panel_dsi_generic_send_seq(panel, 0x02, 0x04);
    panel_dsi_generic_send_seq(panel, 0x03, 0x09);
    panel_dsi_generic_send_seq(panel, 0x04, 0x0B);
    panel_dsi_generic_send_seq(panel, 0x05, 0x60);
    panel_dsi_generic_send_seq(panel, 0x06, 0xF1);
    panel_dsi_generic_send_seq(panel, 0x07, 0x89);
    panel_dsi_generic_send_seq(panel, 0x08, 0x05);
    panel_dsi_generic_send_seq(panel, 0x09, 0x80);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x00);
    panel_dsi_generic_send_seq(panel, 0x0c, 0x0F);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x0F);
    panel_dsi_generic_send_seq(panel, 0x00, 0xA8);//page A
    panel_dsi_generic_send_seq(panel, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0x02, 0x02);
    panel_dsi_generic_send_seq(panel, 0x03, 0x04);
    panel_dsi_generic_send_seq(panel, 0x04, 0x08);
    panel_dsi_generic_send_seq(panel, 0x05, 0x10);
    panel_dsi_generic_send_seq(panel, 0x06, 0x18);
    panel_dsi_generic_send_seq(panel, 0x07, 0x00);
    panel_dsi_generic_send_seq(panel, 0x08, 0x10);
    panel_dsi_generic_send_seq(panel, 0x09, 0x10);
    panel_dsi_generic_send_seq(panel, 0x0a, 0x40);
    panel_dsi_generic_send_seq(panel, 0x0b, 0x70);
    panel_dsi_generic_send_seq(panel, 0x0c, 0x70);
    panel_dsi_generic_send_seq(panel, 0x0d, 0x70);
    panel_dsi_generic_send_seq(panel, 0x0e, 0x68);
    panel_dsi_generic_send_seq(panel, 0x0f, 0x70);
    panel_dsi_generic_send_seq(panel, 0x10, 0x78);
    panel_dsi_generic_send_seq(panel, 0x11, 0x7C);
    panel_dsi_generic_send_seq(panel, 0x12, 0x7E);
    panel_dsi_generic_send_seq(panel, 0x13, 0x7F);
    panel_dsi_generic_send_seq(panel, 0x14, 0x00);
    panel_dsi_generic_send_seq(panel, 0x15, 0x02);
    panel_dsi_generic_send_seq(panel, 0x16, 0x04);
    panel_dsi_generic_send_seq(panel, 0x17, 0x08);
    panel_dsi_generic_send_seq(panel, 0x18, 0x10);
    panel_dsi_generic_send_seq(panel, 0x19, 0x18);
    panel_dsi_generic_send_seq(panel, 0x1a, 0x00);
    panel_dsi_generic_send_seq(panel, 0x1b, 0x10);
    panel_dsi_generic_send_seq(panel, 0x1c, 0x10);
    panel_dsi_generic_send_seq(panel, 0x1d, 0x40);
    panel_dsi_generic_send_seq(panel, 0x1e, 0x70);
    panel_dsi_generic_send_seq(panel, 0x1f, 0x70);
    panel_dsi_generic_send_seq(panel, 0x20, 0x70);
    panel_dsi_generic_send_seq(panel, 0x21, 0x68);
    panel_dsi_generic_send_seq(panel, 0x22, 0x70);
    panel_dsi_generic_send_seq(panel, 0x23, 0x78);
    panel_dsi_generic_send_seq(panel, 0x24, 0x7C);
    panel_dsi_generic_send_seq(panel, 0x25, 0x7E);
    panel_dsi_generic_send_seq(panel, 0x26, 0x7F);

    panel_dsi_generic_send_seq(panel, 0x11, 0x00);
    aic_delay_ms(100);
    panel_dsi_generic_send_seq(panel, 0x29, 0x00);
    aic_delay_ms(100);

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

static struct display_timing h070a27_timing = {
    .pixelclock = 84 * 1000 * 1000,
    .hactive = 1080,
    .hfront_porch = 100,
    .hback_porch = 30,
    .hsync_len = 10,
    .vactive = 1080,
    .vfront_porch = 30,
    .vback_porch = 20,
    .vsync_len = 10,
};

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_PULSE,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_h070a27 = {
    .name = "panel-h070a27",
    .timings = &h070a27_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};

