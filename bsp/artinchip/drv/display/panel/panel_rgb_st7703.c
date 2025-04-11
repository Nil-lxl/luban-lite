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
    aic_delay_ms(10);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}
void panel_spi_wr_reg(u8 cmd,u8 data){
    panel_spi_cmd_wr(cmd);
    panel_spi_data_wr(data);
}
static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init();

    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x01);
    panel_spi_wr_reg(0xE3,0x00);
    panel_spi_wr_reg(0x40,0x00);
    panel_spi_wr_reg(0x03,0x40);
    panel_spi_wr_reg(0x04,0x00);
    panel_spi_wr_reg(0x05,0x03);
    panel_spi_wr_reg(0x08,0x00);
    panel_spi_wr_reg(0x09,0x07);
    panel_spi_wr_reg(0x0A,0x01);
    panel_spi_wr_reg(0x0B,0x32);
    panel_spi_wr_reg(0x0C,0x32);
    panel_spi_wr_reg(0x0D,0x0B);
    panel_spi_wr_reg(0x0E,0x00);
    panel_spi_wr_reg(0x23,0xA2);
    panel_spi_wr_reg(0x24,0x0c);
    panel_spi_wr_reg(0x25,0x06);
    panel_spi_wr_reg(0x26,0x14);
    panel_spi_wr_reg(0x27,0x14);
    panel_spi_wr_reg(0x38,0x9C);
    panel_spi_wr_reg(0x39,0xA7);
    panel_spi_wr_reg(0x3A,0x55);
    panel_spi_wr_reg(0x28,0x40);
    panel_spi_wr_reg(0x29,0x01);
    panel_spi_wr_reg(0x2A,0xdf);
    panel_spi_wr_reg(0x49,0x3C);
    panel_spi_wr_reg(0x91,0x57);
    panel_spi_wr_reg(0x92,0x57);
    panel_spi_wr_reg(0xA0,0x55);
    panel_spi_wr_reg(0xA1,0x50);
    panel_spi_wr_reg(0xA4,0x9C);
    panel_spi_wr_reg(0xA7,0x02);
    panel_spi_wr_reg(0xA8,0x01);
    panel_spi_wr_reg(0xA9,0x01);
    panel_spi_wr_reg(0xAA,0xFC);
    panel_spi_wr_reg(0xAB,0x28);
    panel_spi_wr_reg(0xAC,0x06);
    panel_spi_wr_reg(0xAD,0x06);
    panel_spi_wr_reg(0xAE,0x06);
    panel_spi_wr_reg(0xAF,0x03);
    panel_spi_wr_reg(0xB0,0x08);
    panel_spi_wr_reg(0xB1,0x26);
    panel_spi_wr_reg(0xB2,0x28);
    panel_spi_wr_reg(0xB3,0x28);
    panel_spi_wr_reg(0xB4,0x03);
    panel_spi_wr_reg(0xB5,0x08);
    panel_spi_wr_reg(0xB6,0x26);
    panel_spi_wr_reg(0xB7,0x08);
    panel_spi_wr_reg(0xB8,0x26);
    panel_spi_wr_reg(0xF0,0x00);
    panel_spi_wr_reg(0xF6,0xC0);
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x02);
    panel_spi_wr_reg(0xB0,0x0B);
    panel_spi_wr_reg(0xB1,0x16);
    panel_spi_wr_reg(0xB2,0x17);
    panel_spi_wr_reg(0xB3,0x2C);
    panel_spi_wr_reg(0xB4,0x32);
    panel_spi_wr_reg(0xB5,0x3B);
    panel_spi_wr_reg(0xB6,0x29);
    panel_spi_wr_reg(0xB7,0x40);
    panel_spi_wr_reg(0xB8,0x0d);
    panel_spi_wr_reg(0xB9,0x05);
    panel_spi_wr_reg(0xBA,0x12);
    panel_spi_wr_reg(0xBB,0x10);
    panel_spi_wr_reg(0xBC,0x12);
    panel_spi_wr_reg(0xBD,0x15);
    panel_spi_wr_reg(0xBE,0x19);
    panel_spi_wr_reg(0xBF,0x0E);
    panel_spi_wr_reg(0xC0,0x16);
    panel_spi_wr_reg(0xC1,0x0A);
    panel_spi_wr_reg(0xD0,0x0C);
    panel_spi_wr_reg(0xD1,0x17);
    panel_spi_wr_reg(0xD2,0x14);
    panel_spi_wr_reg(0xD3,0x2E);
    panel_spi_wr_reg(0xD4,0x32);
    panel_spi_wr_reg(0xD5,0x3C);
    panel_spi_wr_reg(0xD6,0x22);
    panel_spi_wr_reg(0xD7,0x3D);
    panel_spi_wr_reg(0xD8,0x0D);
    panel_spi_wr_reg(0xD9,0x07);
    panel_spi_wr_reg(0xDA,0x13);
    panel_spi_wr_reg(0xDB,0x13);
    panel_spi_wr_reg(0xDC,0x11);
    panel_spi_wr_reg(0xDD,0x15);
    panel_spi_wr_reg(0xDE,0x19);
    panel_spi_wr_reg(0xDF,0x10);
    panel_spi_wr_reg(0xE0,0x17);
    panel_spi_wr_reg(0xE1,0x0A);
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x03);
    panel_spi_wr_reg(0x00,0x2A);
    panel_spi_wr_reg(0x01,0x2A);
    panel_spi_wr_reg(0x02,0x2A);
    panel_spi_wr_reg(0x03,0x2A);
    panel_spi_wr_reg(0x04,0x61);
    panel_spi_wr_reg(0x05,0x80);
    panel_spi_wr_reg(0x06,0xc7);
    panel_spi_wr_reg(0x07,0x01);
    panel_spi_wr_reg(0x08,0x03);
    panel_spi_wr_reg(0x09,0x04);
    panel_spi_wr_reg(0x70,0x22);
    panel_spi_wr_reg(0x71,0x80);
    panel_spi_wr_reg(0x30,0x2A);
    panel_spi_wr_reg(0x31,0x2A);
    panel_spi_wr_reg(0x32,0x2A);
    panel_spi_wr_reg(0x33,0x2A);
    panel_spi_wr_reg(0x34,0x61);
    panel_spi_wr_reg(0x35,0xc5);
    panel_spi_wr_reg(0x36,0x80);
    panel_spi_wr_reg(0x37,0x23);
    panel_spi_wr_reg(0x40,0x03);
    panel_spi_wr_reg(0x41,0x04);
    panel_spi_wr_reg(0x42,0x05);
    panel_spi_wr_reg(0x43,0x06);
    panel_spi_wr_reg(0x44,0x11);
    panel_spi_wr_reg(0x45,0xe8);
    panel_spi_wr_reg(0x46,0xe9);
    panel_spi_wr_reg(0x47,0x11);
    panel_spi_wr_reg(0x48,0xea);
    panel_spi_wr_reg(0x49,0xeb);
    panel_spi_wr_reg(0x50,0x07);
    panel_spi_wr_reg(0x51,0x08);
    panel_spi_wr_reg(0x52,0x09);
    panel_spi_wr_reg(0x53,0x0a);
    panel_spi_wr_reg(0x54,0x11);
    panel_spi_wr_reg(0x55,0xec);
    panel_spi_wr_reg(0x56,0xed);
    panel_spi_wr_reg(0x57,0x11);
    panel_spi_wr_reg(0x58,0xef);
    panel_spi_wr_reg(0x59,0xf0);
    panel_spi_wr_reg(0xB1,0x01);
    panel_spi_wr_reg(0xB4,0x15);
    panel_spi_wr_reg(0xB5,0x16);
    panel_spi_wr_reg(0xB6,0x09);
    panel_spi_wr_reg(0xB7,0x0f);
    panel_spi_wr_reg(0xB8,0x0d);
    panel_spi_wr_reg(0xB9,0x0b);
    panel_spi_wr_reg(0xBA,0x00);
    panel_spi_wr_reg(0xC7,0x02);
    panel_spi_wr_reg(0xCA,0x17);
    panel_spi_wr_reg(0xCB,0x18);
    panel_spi_wr_reg(0xCC,0x0a);
    panel_spi_wr_reg(0xCD,0x10);
    panel_spi_wr_reg(0xCE,0x0e);
    panel_spi_wr_reg(0xCF,0x0c);
    panel_spi_wr_reg(0xD0,0x00);
    panel_spi_wr_reg(0x81,0x00);
    panel_spi_wr_reg(0x84,0x15);
    panel_spi_wr_reg(0x85,0x16);
    panel_spi_wr_reg(0x86,0x10);
    panel_spi_wr_reg(0x87,0x0a);
    panel_spi_wr_reg(0x88,0x0c);
    panel_spi_wr_reg(0x89,0x0e);
    panel_spi_wr_reg(0x8A,0x02);
    panel_spi_wr_reg(0x97,0x00);
    panel_spi_wr_reg(0x9A,0x17);
    panel_spi_wr_reg(0x9B,0x18);
    panel_spi_wr_reg(0x9C,0x0f);
    panel_spi_wr_reg(0x9D,0x09);
    panel_spi_wr_reg(0x9E,0x0b);
    panel_spi_wr_reg(0x9F,0x0d);
    panel_spi_wr_reg(0xA0,0x01);
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x02);
    panel_spi_wr_reg(0x01,0x01);
    panel_spi_wr_reg(0x02,0xDA);
    panel_spi_wr_reg(0x03,0xBA);
    panel_spi_wr_reg(0x04,0xA8);
    panel_spi_wr_reg(0x05,0x9A);
    panel_spi_wr_reg(0x06,0x70);
    panel_spi_wr_reg(0x07,0xFF);
    panel_spi_wr_reg(0x08,0x91);
    panel_spi_wr_reg(0x09,0x90);
    panel_spi_wr_reg(0x0A,0xFF);
    panel_spi_wr_reg(0x0B,0x8F);
    panel_spi_wr_reg(0x0C,0x60);
    panel_spi_wr_reg(0x0D,0x58);
    panel_spi_wr_reg(0x0E,0x48);
    panel_spi_wr_reg(0x0F,0x38);
    panel_spi_wr_reg(0x10,0x2B);
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x00);
    panel_spi_wr_reg(0x36,0x0a);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(120);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(20);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs st7703_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing st7703_timing = {
    .pixelclock = 20000000,
    .hactive = 640,
    .hfront_porch = 20,
    .hback_porch = 20,
    .hsync_len = 2,
    .vactive = 480,
    .vfront_porch = 12,
    .vback_porch = 6,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_16BIT_HD,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_st7703 = {
    .name = "panel-st7703",
    .timings = &st7703_timing,
    .funcs = &st7703_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

