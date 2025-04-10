/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include <aic_hal.h>

// #define SLEEP_PIN  "PE.1"
#define RESET_PIN  "PB.1"

#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.14"

static struct gpio_desc reset_gpio;
// static struct gpio_desc sleep_gpio;

static void panel_gpio_init(void)
{
    panel_get_gpio(&reset_gpio, RESET_PIN);
    // panel_get_gpio(&sleep_gpio, SLEEP_PIN);

    // panel_gpio_set_value(&sleep_gpio, 1);
    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
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
    panel_spi_data_wr(0xE9);
    panel_spi_data_wr(0x03);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x0A);

    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x10);

    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x9C);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0x2D);
    panel_spi_data_wr(0x1F);

    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x17);
    panel_spi_data_wr(0x9E);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x51);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x6A);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x1B);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x30);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x57);
    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x84);
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
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x6E);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6E);
    panel_spi_data_wr(0xA0);
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
    panel_spi_data_wr(0x69);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x6B);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x65);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
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
    panel_spi_data_wr(0x68);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x6A);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x64);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0xA0);

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
    panel_spi_data_wr(0x40);

    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0xFA);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFC);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0xAF);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0xFF);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x1F);
    
    // panel_spi_cmd_wr(0xFF);
    // panel_spi_data_wr(0x77);
    // panel_spi_data_wr(0x01);
    // panel_spi_data_wr(0x00);
    // panel_spi_data_wr(0x00);
    // panel_spi_data_wr(0x13);
    // panel_spi_cmd_wr(0xE8);
    // panel_spi_data_wr(0x00);
    // panel_spi_data_wr(0x0E);

    // panel_spi_cmd_wr(0xE8);
    // panel_spi_data_wr(0x00);
    // panel_spi_data_wr(0x0C);

    // aic_delay_ms(20);

    // panel_spi_cmd_wr(0xE8);
    // panel_spi_data_wr(0x40);
    // panel_spi_data_wr(0x00);
    // panel_spi_cmd_wr(0xE6);
    // panel_spi_data_wr(0x16);
    // panel_spi_data_wr(0x7C);
    // panel_spi_cmd_wr(0xFF);
    // panel_spi_data_wr(0x77);
    // panel_spi_data_wr(0x01);
    // panel_spi_data_wr(0x00);
    // panel_spi_data_wr(0x00);
    // panel_spi_data_wr(0x00);
    // panel_spi_cmd_wr(0x36);
    // panel_spi_data_wr(0x00);
    // panel_spi_cmd_wr(0x35);
    // panel_spi_data_wr(0x00);

    // panel_spi_cmd_wr(0x3a);
    // panel_spi_data_wr(0x66);

#ifdef BIST_MODE
    panel_spi_cmd_wr (0xFF);
    panel_spi_data_wr (0x77);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0x00);
    panel_spi_data_wr (0x00);
    panel_spi_data_wr (0x12);
    panel_spi_cmd_wr (0xD1);
    panel_spi_data_wr (0x81);
    panel_spi_data_wr (0x08);
    panel_spi_data_wr (0x03);
    panel_spi_data_wr (0x20);
    panel_spi_data_wr (0x08);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0xA0);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0xE0);
    panel_spi_data_wr (0xA0);
    panel_spi_data_wr (0x01);
    panel_spi_data_wr (0xE0);
    panel_spi_data_wr (0x03);
    panel_spi_data_wr (0x20);
    panel_spi_cmd_wr (0xD2);
    /* 0x08: colorbar, 0X02: red  etc... */
    panel_spi_data_wr (0x08);
#endif

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);
    
    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);
    aic_delay_ms(120);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(20);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs st7701s_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing st7701s_timing = {
    .pixelclock = 31000000,
    .hactive = 480,
    .hfront_porch = 43,
    .hback_porch = 10,
    .hsync_len = 10,
    .vactive = 854,
    .vfront_porch = 41,
    .vback_porch = 20,
    .vsync_len = 20,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_16BIT_HD,
    .clock_phase = DEGREE_0,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_st7701s = {
    .name = "panel-st7701s",
    .timings = &st7701s_timing,
    .funcs = &st7701s_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

