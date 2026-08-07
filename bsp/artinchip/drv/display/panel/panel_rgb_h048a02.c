/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "panel_com.h"
#include <aic_hal.h>
#include "disp_gpio.h"

static struct gpio_desc reset_gpio;

static void panel_gpio_init(void) {
    panel_get_gpio(&reset_gpio, RESET_PIN);

    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}
static int panel_enable(struct aic_panel *panel) {
    panel_gpio_init();

    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_cmd_wr(0xF0);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0xAA);
    panel_spi_data_wr(0x52);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xBA);

    aic_delay_ms(30);
    panel_spi_cmd_wr(0xF6);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x87);

    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x3F);

    panel_spi_cmd_wr(0xCD);
    panel_spi_data_wr(0x25);
    panel_spi_cmd_wr(0xC9);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0xF8);
    panel_spi_data_wr(0x8A);
    panel_spi_cmd_wr(0xAC);
    panel_spi_data_wr(0x45);
    panel_spi_cmd_wr(0xA7);
    panel_spi_data_wr(0x47);
    panel_spi_cmd_wr(0xA0);
    panel_spi_data_wr(0xDD);
    panel_spi_cmd_wr(0x86);
    panel_spi_data_wr(0x99);
    panel_spi_data_wr(0xA3);
    panel_spi_data_wr(0xA3);
    panel_spi_data_wr(0x45);
    panel_spi_cmd_wr(0xFA);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x04);
    panel_spi_cmd_wr(0xA3);
    panel_spi_data_wr(0x6E);
    panel_spi_cmd_wr(0xFD);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x9A);
    panel_spi_data_wr(0x9B);
    panel_spi_cmd_wr(0x9B);
    panel_spi_data_wr(0x37);
    panel_spi_cmd_wr(0x82);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0x2B);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0x7A);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x1a);
    panel_spi_cmd_wr(0x7B);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x1a);
    panel_spi_cmd_wr(0x6D);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x60);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_cmd_wr(0x63);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_cmd_wr(0x64);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x61);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x62);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_cmd_wr(0x67);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x63);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x64);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);

    panel_spi_cmd_wr(0x69);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x08);

    panel_spi_cmd_wr(0x6B);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xD1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x6D);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9B);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xD2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x6D);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9B);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xD3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x6D);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9B);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xD4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x6D);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9B);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xD5);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x6D);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9B);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xD6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x6D);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x5A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x9B);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB8);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);
    panel_spi_cmd_wr(0x29);
    aic_delay_ms(200);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h048a02_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h048a02_timing = {
    .pixelclock = 36 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 16,
    .hback_porch = 16,
    .hsync_len = 16,
    .vactive = 1120,
    .vfront_porch = 10,
    .vback_porch = 15,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h048a02 = {
    .name = "panel-h048a02",
    .timings = &h048a02_timing,
    .funcs = &h048a02_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

