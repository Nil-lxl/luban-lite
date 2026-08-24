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
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}
static int panel_enable(struct aic_panel *panel) {
    panel_gpio_init();

    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x52);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0xE3);
    panel_spi_data_wr(0x00);
    //panel_spi_cmd_wr(0xF6);
    //panel_spi_data_wr(0xC0);
    //panel_spi_cmd_wr(0xF0);
    //panel_spi_data_wr(0x00);//BIST
    panel_spi_cmd_wr(0x0a);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0x23);
    panel_spi_data_wr(0xa2);
    panel_spi_cmd_wr(0x24);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0x25);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0x26);
    panel_spi_data_wr(0x2e);
    panel_spi_cmd_wr(0x27);
    panel_spi_data_wr(0x2E);
    panel_spi_cmd_wr(0x28);
    panel_spi_data_wr(0x64);
    panel_spi_cmd_wr(0x38);
    panel_spi_data_wr(0x84);
    panel_spi_cmd_wr(0x39);
    panel_spi_data_wr(0x8f);
    panel_spi_cmd_wr(0x3A);
    panel_spi_data_wr(0x2C);//VCOM
    //panel_spi_cmd_wr(0x40);
    //panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0x80);
    panel_spi_data_wr(0x60);

    panel_spi_cmd_wr(0x49);
    panel_spi_data_wr(0x3C);
    panel_spi_cmd_wr(0x91);
    panel_spi_data_wr(0x67);
    panel_spi_cmd_wr(0x92);
    panel_spi_data_wr(0x67);
    panel_spi_cmd_wr(0x99);
    panel_spi_data_wr(0x54);
    panel_spi_cmd_wr(0x9B);
    panel_spi_data_wr(0x56);
    panel_spi_cmd_wr(0xA0);
    panel_spi_data_wr(0x55);
    panel_spi_cmd_wr(0xA1);
    panel_spi_data_wr(0x50);
    panel_spi_cmd_wr(0xA4);
    panel_spi_data_wr(0x9C);
    panel_spi_cmd_wr(0xA7);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xA8);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0xA9);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0xAA);
    panel_spi_data_wr(0xFC);
    panel_spi_cmd_wr(0xAB);
    panel_spi_data_wr(0x28);
    panel_spi_cmd_wr(0xAC);
    panel_spi_data_wr(0x06);
    panel_spi_cmd_wr(0xAD);
    panel_spi_data_wr(0x06);
    panel_spi_cmd_wr(0xAE);
    panel_spi_data_wr(0x06);
    panel_spi_cmd_wr(0xAF);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x26);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x28);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x28);
    panel_spi_cmd_wr(0xB4);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0xB6);
    panel_spi_data_wr(0x26);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x26);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x52);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x07);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x2E);
    panel_spi_cmd_wr(0xB4);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x37);
    panel_spi_cmd_wr(0xB6);
    panel_spi_data_wr(0x17);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x37);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x0D);
    panel_spi_cmd_wr(0xB9);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xBA);
    panel_spi_data_wr(0x12);
    panel_spi_cmd_wr(0xBB);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xBC);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xBD);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0xBE);
    panel_spi_data_wr(0x16);
    panel_spi_cmd_wr(0xBF);
    panel_spi_data_wr(0x0D);
    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x16);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x06);
    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x05);
    panel_spi_cmd_wr(0xD1);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xD2);
    panel_spi_data_wr(0x0B);
    panel_spi_cmd_wr(0xD3);
    panel_spi_data_wr(0x2C);
    panel_spi_cmd_wr(0xD4);
    panel_spi_data_wr(0x2E);
    panel_spi_cmd_wr(0xD5);
    panel_spi_data_wr(0x32);
    panel_spi_cmd_wr(0xD6);
    panel_spi_data_wr(0x15);
    panel_spi_cmd_wr(0xD7);
    panel_spi_data_wr(0x37);
    panel_spi_cmd_wr(0xD8);
    panel_spi_data_wr(0x0B);
    panel_spi_cmd_wr(0xD9);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xDA);
    panel_spi_data_wr(0x12);
    panel_spi_cmd_wr(0xDB);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xDC);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xDD);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x18);
    panel_spi_cmd_wr(0xDF);
    panel_spi_data_wr(0x0F);
    panel_spi_cmd_wr(0xE0);
    panel_spi_data_wr(0x18);
    panel_spi_cmd_wr(0xE1);
    panel_spi_data_wr(0x06);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x52);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x02);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x03);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x08);
    panel_spi_data_wr(0x09);
    panel_spi_cmd_wr(0x09);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x0A);
    panel_spi_data_wr(0x0B);
    panel_spi_cmd_wr(0x0B);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0x2A);
    panel_spi_data_wr(0x19);
    panel_spi_cmd_wr(0x2B);
    panel_spi_data_wr(0x19);
    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x31);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x32);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x33);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x34);
    panel_spi_data_wr(0xa1);
    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x26);
    panel_spi_cmd_wr(0x40);
    panel_spi_data_wr(0x0D);
    panel_spi_cmd_wr(0x41);
    panel_spi_data_wr(0x0E);
    panel_spi_cmd_wr(0x42);
    panel_spi_data_wr(0x0F);
    panel_spi_cmd_wr(0x43);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0x45);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0x46);
    panel_spi_data_wr(0x15);
    panel_spi_cmd_wr(0x48);
    panel_spi_data_wr(0x16);
    panel_spi_cmd_wr(0x49);
    panel_spi_data_wr(0x17);
    panel_spi_cmd_wr(0x50);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0x51);
    panel_spi_data_wr(0x12);
    panel_spi_cmd_wr(0x52);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0x53);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0x55);
    panel_spi_data_wr(0x18);
    panel_spi_cmd_wr(0x56);
    panel_spi_data_wr(0x19);
    panel_spi_cmd_wr(0x58);
    panel_spi_data_wr(0x1a);
    panel_spi_cmd_wr(0x59);
    panel_spi_data_wr(0x1b);
    panel_spi_cmd_wr(0x60);
    panel_spi_data_wr(0x05);
    panel_spi_cmd_wr(0x61);
    panel_spi_data_wr(0x05);
    panel_spi_cmd_wr(0x65);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x66);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x80);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x81);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0x82);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x83);
    panel_spi_data_wr(0x12);
    panel_spi_cmd_wr(0x84);
    panel_spi_data_wr(0x0e);
    panel_spi_cmd_wr(0x85);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0x86);
    panel_spi_data_wr(0x0a);
    panel_spi_cmd_wr(0x87);
    panel_spi_data_wr(0x0c);
    panel_spi_cmd_wr(0x88);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x89);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0x8A);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0x8B);
    panel_spi_data_wr(0x04);
    panel_spi_cmd_wr(0x96);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x97);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0x98);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x99);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0x9A);
    panel_spi_data_wr(0x0d);
    panel_spi_cmd_wr(0x9B);
    panel_spi_data_wr(0x0f);
    panel_spi_cmd_wr(0x9C);
    panel_spi_data_wr(0x09);
    panel_spi_cmd_wr(0x9D);
    panel_spi_data_wr(0x0b);
    panel_spi_cmd_wr(0x9E);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x9F);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0xA0);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0xA1);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xb0);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0xb1);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xb2);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xb3);
    panel_spi_data_wr(0x12);
    panel_spi_cmd_wr(0xb4);
    panel_spi_data_wr(0x0b);
    panel_spi_cmd_wr(0xb5);
    panel_spi_data_wr(0x09);
    panel_spi_cmd_wr(0xb6);
    panel_spi_data_wr(0x0f);
    panel_spi_cmd_wr(0xb7);
    panel_spi_data_wr(0x0d);
    panel_spi_cmd_wr(0xb8);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xb9);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0xbA);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xbB);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0xc6);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0xc7);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xc8);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xc9);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xcA);
    panel_spi_data_wr(0x0c);
    panel_spi_cmd_wr(0xcB);
    panel_spi_data_wr(0x0a);
    panel_spi_cmd_wr(0xcC);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0xcD);
    panel_spi_data_wr(0x0e);
    panel_spi_cmd_wr(0xcE);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xcF);
    panel_spi_data_wr(0x1f);
    panel_spi_cmd_wr(0xd0);
    panel_spi_data_wr(0x04);
    panel_spi_cmd_wr(0xd1);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x52);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x02);	//0A

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(120);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h078a03_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h078a03_timing = {
    .pixelclock = 51 * 1000 * 1000,
    .hactive = 540,
    .hfront_porch = 46,
    .hback_porch = 44,
    .hsync_len = 2,
    .vactive = 1280,
    .vfront_porch = 20,
    .vback_porch = 18,
    .vsync_len = 3,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_h078a03 = {
    .name = "panel-h078a03",
    .timings = &h078a03_timing,
    .funcs = &h078a03_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

