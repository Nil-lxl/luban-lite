/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include <aic_hal.h>

#define RESET_PIN  "PD.13"

#define SCL_PIN    "PC.4"
#define DC_PIN     "PC.5"
#define CS_PIN     "PC.7"
#define SDI_PIN    "PD.16"

#define IM0_PIN    "PD.15"
#define IM1_PIN    "PD.14"

static struct gpio_desc reset_gpio;
static struct gpio_desc im0_gpio;
static struct gpio_desc im1_gpio;

static void panel_gpio_init(void)
{
    panel_get_gpio(&reset_gpio, RESET_PIN);
    panel_get_gpio(&im0_gpio, IM0_PIN);
    panel_get_gpio(&im1_gpio, IM1_PIN);

    panel_get_gpio(&im0_gpio, IM0_PIN);
    panel_get_gpio(&im1_gpio, IM1_PIN);

    panel_gpio_set_value(&im0_gpio, 1);
    panel_gpio_set_value(&im1_gpio, 0);

    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init();

    panel_spi_device_emulation(CS_PIN, SDI_PIN, SCL_PIN, DC_PIN);

    panel_spi_send_seq(0xF0, 0x28);
    panel_spi_send_seq(0xF2, 0x28);
    panel_spi_send_seq(0x73, 0xF0);
    panel_spi_send_seq(0x7C, 0xD1);
    panel_spi_send_seq(0x83, 0xE0);
    panel_spi_send_seq(0x84, 0x61);
    panel_spi_send_seq(0xF2, 0x82);
    panel_spi_send_seq(0xF0, 0x00);
    panel_spi_send_seq(0xF0, 0x01);
    panel_spi_send_seq(0xF1, 0x01);
    panel_spi_send_seq(0xB0, 0x65);
    panel_spi_send_seq(0xB1, 0x3E);
    panel_spi_send_seq(0xB2, 0x33);
    panel_spi_send_seq(0xB3, 0x01);
    panel_spi_send_seq(0xB4, 0x69);
    panel_spi_send_seq(0xB5, 0x45);
    panel_spi_send_seq(0xB6, 0x8B);
    panel_spi_send_seq(0xB7, 0x41);
    panel_spi_send_seq(0xB8, 0x86);
    panel_spi_send_seq(0xB9, 0x15);
    panel_spi_send_seq(0xBA, 0x00);
    panel_spi_send_seq(0xBB, 0x08);
    panel_spi_send_seq(0xBC, 0x08);
    panel_spi_send_seq(0xBD, 0x00);
    panel_spi_send_seq(0xBE, 0x00);
    panel_spi_send_seq(0xBF, 0x07);
    panel_spi_send_seq(0xC0, 0x80);
    panel_spi_send_seq(0xC1, 0x10);
    panel_spi_send_seq(0xC2, 0x37);
    panel_spi_send_seq(0xC3, 0x80);
    panel_spi_send_seq(0xC4, 0x10);
    panel_spi_send_seq(0xC5, 0x37);
    panel_spi_send_seq(0xC6, 0xA9);
    panel_spi_send_seq(0xC7, 0x41);
    panel_spi_send_seq(0xC8, 0x01);
    panel_spi_send_seq(0xC9, 0xA9);
    panel_spi_send_seq(0xCA, 0x41);
    panel_spi_send_seq(0xCB, 0x01);
    panel_spi_send_seq(0xCC, 0x7F);
    panel_spi_send_seq(0xCD, 0x7F);
    panel_spi_send_seq(0xCE, 0xFF);
    panel_spi_send_seq(0xD0, 0x91);
    panel_spi_send_seq(0xD1, 0x68);
    panel_spi_send_seq(0xD2, 0x68);
    panel_spi_send_seq(0xF5, 0x00, 0xA5);
    panel_spi_send_seq(0xF1, 0x10);
    panel_spi_send_seq(0xF0, 0x00);
    panel_spi_send_seq(0xF0, 0x02);
    panel_spi_send_seq(0xE0, 0xF0, 0x08, 0x0E, 0x0C, 0x0C, 0x18,
    0x36, 0x44, 0x4D, 0x19, 0x17, 0x16, 0x32, 0x35);
    panel_spi_send_seq(0xE1, 0xF0, 0x07, 0x0D, 0x0B, 0x0B, 0x07,
    0x35, 0x33, 0x4D, 0x39, 0x16, 0x16, 0x31, 0x35);
    panel_spi_send_seq(0xF0, 0x10);
    panel_spi_send_seq(0xF3, 0x10);
    panel_spi_send_seq(0xE0, 0x08);
    panel_spi_send_seq(0xE1, 0x00);
    panel_spi_send_seq(0xE2, 0x00);
    panel_spi_send_seq(0xE3, 0x00);
    panel_spi_send_seq(0xE4, 0xE0);
    panel_spi_send_seq(0xE5, 0x06);
    panel_spi_send_seq(0xE6, 0x21);
    panel_spi_send_seq(0xE7, 0x03);
    panel_spi_send_seq(0xE8, 0x05);
    panel_spi_send_seq(0xE9, 0x02);
    panel_spi_send_seq(0xEA, 0xE9);
    panel_spi_send_seq(0xEB, 0x00);
    panel_spi_send_seq(0xEC, 0x00);
    panel_spi_send_seq(0xED, 0x14);
    panel_spi_send_seq(0xEE, 0xFF);
    panel_spi_send_seq(0xEF, 0x00);
    panel_spi_send_seq(0xF8, 0xFF);
    panel_spi_send_seq(0xF9, 0x00);
    panel_spi_send_seq(0xFA, 0x00);
    panel_spi_send_seq(0xFB, 0x30);
    panel_spi_send_seq(0xFC, 0x00);
    panel_spi_send_seq(0xFD, 0x00);
    panel_spi_send_seq(0xFE, 0x00);
    panel_spi_send_seq(0xFF, 0x00);

    panel_spi_send_seq(0x60, 0x40);
    panel_spi_send_seq(0x61, 0x05);
    panel_spi_send_seq(0x62, 0x00);
    panel_spi_send_seq(0x63, 0x42);
    panel_spi_send_seq(0x64, 0xDA);
    panel_spi_send_seq(0x65, 0x00);
    panel_spi_send_seq(0x66, 0x00);
    panel_spi_send_seq(0x67, 0x00);
    panel_spi_send_seq(0x68, 0x00);
    panel_spi_send_seq(0x69, 0x00);
    panel_spi_send_seq(0x6A, 0x00);
    panel_spi_send_seq(0x6B, 0x00);
    panel_spi_send_seq(0x70, 0x40);
    panel_spi_send_seq(0x71, 0x04);
    panel_spi_send_seq(0x72, 0x00);
    panel_spi_send_seq(0x73, 0x42);
    panel_spi_send_seq(0x74, 0xD9);
    panel_spi_send_seq(0x75, 0x00);
    panel_spi_send_seq(0x76, 0x00);
    panel_spi_send_seq(0x77, 0x00);
    panel_spi_send_seq(0x78, 0x00);
    panel_spi_send_seq(0x79, 0x00);
    panel_spi_send_seq(0x7A, 0x00);
    panel_spi_send_seq(0x7B, 0x00);
    panel_spi_send_seq(0x80, 0x48);
    panel_spi_send_seq(0x81, 0x00);
    panel_spi_send_seq(0x82, 0x07);
    panel_spi_send_seq(0x83, 0x02);
    panel_spi_send_seq(0x84, 0xD7);
    panel_spi_send_seq(0x85, 0x04);
    panel_spi_send_seq(0x86, 0x00);
    panel_spi_send_seq(0x87, 0x00);
    panel_spi_send_seq(0x88, 0x48);
    panel_spi_send_seq(0x89, 0x00);
    panel_spi_send_seq(0x8A, 0x09);
    panel_spi_send_seq(0x8B, 0x02);
    panel_spi_send_seq(0x8C, 0xD9);
    panel_spi_send_seq(0x8D, 0x04);
    panel_spi_send_seq(0x8E, 0x00);
    panel_spi_send_seq(0x8F, 0x00);
    panel_spi_send_seq(0x90, 0x48);
    panel_spi_send_seq(0x91, 0x00);
    panel_spi_send_seq(0x92, 0x0B);
    panel_spi_send_seq(0x93, 0x02);
    panel_spi_send_seq(0x94, 0xDB);
    panel_spi_send_seq(0x95, 0x04);
    panel_spi_send_seq(0x96, 0x00);
    panel_spi_send_seq(0x97, 0x00);
    panel_spi_send_seq(0x98, 0x48);
    panel_spi_send_seq(0x99, 0x00);
    panel_spi_send_seq(0x9A, 0x0D);
    panel_spi_send_seq(0x9B, 0x02);
    panel_spi_send_seq(0x9C, 0xDD);
    panel_spi_send_seq(0x9D, 0x04);
    panel_spi_send_seq(0x9E, 0x00);
    panel_spi_send_seq(0x9F, 0x00);
    panel_spi_send_seq(0xA0, 0x48);
    panel_spi_send_seq(0xA1, 0x00);
    panel_spi_send_seq(0xA2, 0x06);
    panel_spi_send_seq(0xA3, 0x02);
    panel_spi_send_seq(0xA4, 0xD6);
    panel_spi_send_seq(0xA5, 0x04);
    panel_spi_send_seq(0xA6, 0x00);
    panel_spi_send_seq(0xA7, 0x00);
    panel_spi_send_seq(0xA8, 0x48);
    panel_spi_send_seq(0xA9, 0x00);
    panel_spi_send_seq(0xAA, 0x08);
    panel_spi_send_seq(0xAB, 0x02);
    panel_spi_send_seq(0xAC, 0xD8);
    panel_spi_send_seq(0xAD, 0x04);
    panel_spi_send_seq(0xAE, 0x00);
    panel_spi_send_seq(0xAF, 0x00);
    panel_spi_send_seq(0xB0, 0x48);
    panel_spi_send_seq(0xB1, 0x00);
    panel_spi_send_seq(0xB2, 0x0A);
    panel_spi_send_seq(0xB3, 0x02);
    panel_spi_send_seq(0xB4, 0xDA);
    panel_spi_send_seq(0xB5, 0x04);
    panel_spi_send_seq(0xB6, 0x00);
    panel_spi_send_seq(0xB7, 0x00);
    panel_spi_send_seq(0xB8, 0x48);
    panel_spi_send_seq(0xB9, 0x00);
    panel_spi_send_seq(0xBA, 0x0C);
    panel_spi_send_seq(0xBB, 0x02);
    panel_spi_send_seq(0xBC, 0xDC);
    panel_spi_send_seq(0xBD, 0x04);
    panel_spi_send_seq(0xBE, 0x00);
    panel_spi_send_seq(0xBF, 0x00);
    panel_spi_send_seq(0xC0, 0x10);
    panel_spi_send_seq(0xC1, 0x47);
    panel_spi_send_seq(0xC2, 0x56);
    panel_spi_send_seq(0xC3, 0x65);
    panel_spi_send_seq(0xC4, 0x74);
    panel_spi_send_seq(0xC5, 0x88);
    panel_spi_send_seq(0xC6, 0x99);
    panel_spi_send_seq(0xC7, 0x01);
    panel_spi_send_seq(0xC8, 0xBB);
    panel_spi_send_seq(0xC9, 0xAA);
    panel_spi_send_seq(0xD0, 0x10);
    panel_spi_send_seq(0xD1, 0x47);
    panel_spi_send_seq(0xD2, 0x56);
    panel_spi_send_seq(0xD3, 0x65);
    panel_spi_send_seq(0xD4, 0x74);
    panel_spi_send_seq(0xD5, 0x88);
    panel_spi_send_seq(0xD6, 0x99);
    panel_spi_send_seq(0xD7, 0x01);
    panel_spi_send_seq(0xD8, 0xBB);
    panel_spi_send_seq(0xD9, 0xAA);
    panel_spi_send_seq(0xF3, 0x01);
    panel_spi_send_seq(0xF0, 0x00);
    panel_spi_send_seq(0x3A, 0x55);
    panel_spi_send_seq(0x36, 0x00);
    panel_spi_send_seq(0x21);
    panel_spi_send_seq(0x11);
    aic_delay_ms(120);
    panel_spi_send_seq(0x29);
    aic_delay_ms(10);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);

    return 0;
}

static struct aic_panel_funcs st77916_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing st77916_timing = {
    .pixelclock = 8000000,
    .hactive = 360,
    .hfront_porch = 6,
    .hback_porch = 12,
    .hsync_len = 6,
    .vactive = 360,
    .vfront_porch = 2,
    .vback_porch = 2,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = SRGB,
    .format = SRGB_8BIT,
    .clock_phase = DEGREE_0,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_st77916 = {
    .name = "panel-st77916",
    .timings = &st77916_timing,
    .funcs = &st77916_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

