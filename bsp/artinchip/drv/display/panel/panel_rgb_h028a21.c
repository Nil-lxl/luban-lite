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
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x98);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x23);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x52);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x97);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x51);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0x0F);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x5D);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x2D);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x81);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x4E);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x20);
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
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0xF4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xE3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE5);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0xF7);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0xF1);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0xF4);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xF2);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0xF0);
    panel_spi_cmd_wr(0xE9);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x88);
    panel_spi_data_wr(0x40);
    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0xAF);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFC);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0xFA);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xFF);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x54);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);
    panel_spi_cmd_wr(0x3A);
    panel_spi_data_wr(0x77);
    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x29);
    aic_delay_ms(50);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h028a21_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h028a21_timing = {
    .pixelclock = 30 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 80,
    .hback_porch = 70,
    .hsync_len = 8,
    .vactive = 480,
    .vfront_porch = 25,
    .vback_porch = 10,
    .vsync_len = 10,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h028a21 = {
    .name = "panel-h028a21",
    .timings = &h028a21_timing,
    .funcs = &h028a21_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

