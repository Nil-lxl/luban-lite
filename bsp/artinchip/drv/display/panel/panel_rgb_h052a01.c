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

    panel_spi_cmd_wr(0xFF); //Change to page
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xEF); //reset
    panel_spi_data_wr(0x08); //tearing on
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xFF); //Change to page
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x10);
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x1f);
    panel_spi_data_wr(0x10); //Output SDA
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0C); //"00" =DE MODE;"01" = SYNC MODE
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x02);
    //DE=1 Active
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xC6);
    panel_spi_data_wr(0x21); //Normally Black
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x30); //VFP
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x1F);
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x17);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x27);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x1F);
    aic_delay_ms(5);
    panel_spi_cmd_wr(0xFF); //Change to page
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x4D);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x40);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x81);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x4E);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x21);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x78);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x78);
    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x88);
    panel_spi_cmd_wr(0xE0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xE1);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xE3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x55);
    panel_spi_cmd_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x17);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x55);
    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE5);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0xA0);
    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0xCC);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xEC);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x54);
    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x7F);
    panel_spi_data_wr(0x89);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x42);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFC);
    panel_spi_data_wr(0x24);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x98);
    panel_spi_data_wr(0xF7);
    panel_spi_data_wr(0x65);
    panel_spi_data_wr(0xFF);
    panel_spi_cmd_wr(0xFF); //Change to page
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xE8); //Change to page0
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0E);
    panel_spi_cmd_wr(0xFF); //Change to page0
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);

    //aic_delay_ms 120ms
    panel_spi_cmd_wr(0xFF); //Change to page
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xFF); //Changetopage
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x3A);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(200);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h052a01_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h052a01_timing = {
    .pixelclock = 34 * 1000 * 1000,
    .hactive = 448,
    .hfront_porch = 20,
    .hback_porch = 20,
    .hsync_len = 20,
    .vactive = 1280,
    .vfront_porch = 20,
    .vback_porch = 20,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h052a01 = {
    .name = "panel-h052a01",
    .timings = &h052a01_timing,
    .funcs = &h052a01_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

