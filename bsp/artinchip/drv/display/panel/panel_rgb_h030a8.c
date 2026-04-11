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
void panel_spi_wr_reg(u8 cmd, u8 data) {
    panel_spi_cmd_wr(cmd);
    panel_spi_data_wr(data);
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
    panel_spi_data_wr(0x63);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x0D);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x31);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x65);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x5A);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x87);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x46);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x31);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x78);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x78);
    panel_spi_cmd_wr(0xE0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xE1);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x43);
    panel_spi_data_wr(0x43);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x2D);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0xAE);
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
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x34);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);
    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x31);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0xAE);
    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x99);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x40);
    panel_spi_cmd_wr(0xEC);
    panel_spi_data_wr(0x3D);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0x89);
    panel_spi_data_wr(0xBA);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xAB);
    panel_spi_data_wr(0x98);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);

    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(50);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h030a8_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h030a8_timing = {
    .pixelclock = 28 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 35,
    .hback_porch = 24,
    .hsync_len = 10,
    .vactive = 800,
    .vfront_porch = 25,
    .vback_porch = 14,
    .vsync_len = 5,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_h030a8 = {
    .name = "panel-h030a8",
    .timings = &h030a8_timing,
    .funcs = &h030a8_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

