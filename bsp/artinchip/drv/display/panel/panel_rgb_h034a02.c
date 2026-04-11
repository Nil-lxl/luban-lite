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
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x25);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x1D);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x24);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x4D);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x52);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x81);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x4E);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x33);
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
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0xD5);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xD5);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xE3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE5);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0xD3);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0xD5);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0xCE);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xD0);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x8C);
    panel_spi_cmd_wr(0xE9);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x88);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xF7);
    panel_spi_data_wr(0x65);
    panel_spi_data_wr(0x4C);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xF2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC4);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x7F);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);
    panel_spi_cmd_wr(0x29);
    aic_delay_ms(200);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h034a02_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h034a02_timing = {
    .pixelclock = 29 * 1000 * 1000,
    .hactive = 412,
    .hfront_porch = 30,
    .hback_porch = 30,
    .hsync_len = 5,
    .vactive = 960,
    .vfront_porch = 20,
    .vback_porch = 20,
    .vsync_len = 5,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h034a02 = {
    .name = "panel-h034a02",
    .timings = &h034a02_timing,
    .funcs = &h034a02_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

