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
    panel_spi_wr_reg(0x0A,0x01);
    panel_spi_wr_reg(0x23,0xA2);//RGB MODE
    panel_spi_wr_reg(0x25,0x14);
    panel_spi_wr_reg(0x29,0x02);
    panel_spi_wr_reg(0x2A,0xCF);
    panel_spi_wr_reg(0x38,0x9C);
    panel_spi_wr_reg(0x39,0xA7);
    panel_spi_wr_reg(0x3A,0x33);//VCOM
    panel_spi_wr_reg(0x91,0x77);
    panel_spi_wr_reg(0x92,0x77);
    panel_spi_wr_reg(0x99,0x52);
    panel_spi_wr_reg(0x9B,0x5B);
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
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x02);
    panel_spi_wr_reg(0xB0,0x02);
    panel_spi_wr_reg(0xB1,0x31);
    panel_spi_wr_reg(0xB2,0x24);
    panel_spi_wr_reg(0xB3,0x30);
    panel_spi_wr_reg(0xB4,0x38);
    panel_spi_wr_reg(0xB5,0x3E);
    panel_spi_wr_reg(0xB6,0x26);
    panel_spi_wr_reg(0xB7,0x3E);
    panel_spi_wr_reg(0xB8,0x0a);
    panel_spi_wr_reg(0xB9,0x00);
    panel_spi_wr_reg(0xBA,0x11);
    panel_spi_wr_reg(0xBB,0x11);
    panel_spi_wr_reg(0xBC,0x13);
    panel_spi_wr_reg(0xBD,0x14);
    panel_spi_wr_reg(0xBE,0x18);
    panel_spi_wr_reg(0xBF,0x11);
    panel_spi_wr_reg(0xC0,0x16);
    panel_spi_wr_reg(0xC1,0x00);
    panel_spi_wr_reg(0xD0,0x05);
    panel_spi_wr_reg(0xD1,0x30);
    panel_spi_wr_reg(0xD2,0x25);
    panel_spi_wr_reg(0xD3,0x35);
    panel_spi_wr_reg(0xD4,0x34);
    panel_spi_wr_reg(0xD5,0x3B);
    panel_spi_wr_reg(0xD6,0x26);
    panel_spi_wr_reg(0xD7,0x3D);
    panel_spi_wr_reg(0xD8,0x0a);
    panel_spi_wr_reg(0xD9,0x00);
    panel_spi_wr_reg(0xDA,0x12);
    panel_spi_wr_reg(0xDB,0x10);
    panel_spi_wr_reg(0xDC,0x12);
    panel_spi_wr_reg(0xDD,0x14);
    panel_spi_wr_reg(0xDE,0x18);
    panel_spi_wr_reg(0xDF,0x11);
    panel_spi_wr_reg(0xE0,0x15);
    panel_spi_wr_reg(0xE1,0x00);
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x03);
    panel_spi_wr_reg(0x08,0x09);
    panel_spi_wr_reg(0x09,0x0A);
    panel_spi_wr_reg(0x0A,0x0B);
    panel_spi_wr_reg(0x0B,0x0C);
    panel_spi_wr_reg(0x28,0x22);
    panel_spi_wr_reg(0x2A,0xEC);
    panel_spi_wr_reg(0x2B,0xEC);
    panel_spi_wr_reg(0x30,0x00);
    panel_spi_wr_reg(0x31,0x00);
    panel_spi_wr_reg(0x32,0x00);
    panel_spi_wr_reg(0x33,0x00);
    panel_spi_wr_reg(0x34,0x61);
    panel_spi_wr_reg(0x35,0xD4);
    panel_spi_wr_reg(0x36,0x24);
    panel_spi_wr_reg(0x37,0x03);
    panel_spi_wr_reg(0x40,0x0D);
    panel_spi_wr_reg(0x41,0x0E);
    panel_spi_wr_reg(0x42,0x0F);
    panel_spi_wr_reg(0x43,0x10);
    panel_spi_wr_reg(0x44,0x22);
    panel_spi_wr_reg(0x45,0xE1);
    panel_spi_wr_reg(0x46,0xE2);
    panel_spi_wr_reg(0x47,0x22);
    panel_spi_wr_reg(0x48,0xE3);
    panel_spi_wr_reg(0x49,0xE4);
    panel_spi_wr_reg(0x50,0x11);
    panel_spi_wr_reg(0x51,0x12);
    panel_spi_wr_reg(0x52,0x13);
    panel_spi_wr_reg(0x53,0x14);
    panel_spi_wr_reg(0x54,0x22);
    panel_spi_wr_reg(0x55,0xE5);
    panel_spi_wr_reg(0x56,0xE6);
    panel_spi_wr_reg(0x57,0x22);
    panel_spi_wr_reg(0x58,0xE7);
    panel_spi_wr_reg(0x59,0xE8);
    panel_spi_wr_reg(0x80,0x05);
    panel_spi_wr_reg(0x81,0x1E);
    panel_spi_wr_reg(0x82,0x02);
    panel_spi_wr_reg(0x83,0x04);
    panel_spi_wr_reg(0x84,0x1E);
    panel_spi_wr_reg(0x85,0x1E);
    panel_spi_wr_reg(0x86,0x1f);
    panel_spi_wr_reg(0x87,0x1f);
    panel_spi_wr_reg(0x88,0x0E);
    panel_spi_wr_reg(0x89,0x10);
    panel_spi_wr_reg(0x8A,0x0A);
    panel_spi_wr_reg(0x8B,0x0C);
    panel_spi_wr_reg(0x96,0x05);
    panel_spi_wr_reg(0x97,0x1E);
    panel_spi_wr_reg(0x98,0x01);
    panel_spi_wr_reg(0x99,0x03);
    panel_spi_wr_reg(0x9A,0x1E);
    panel_spi_wr_reg(0x9B,0x1E);
    panel_spi_wr_reg(0x9C,0x1f);
    panel_spi_wr_reg(0x9D,0x1f);
    panel_spi_wr_reg(0x9E,0x0D);
    panel_spi_wr_reg(0x9F,0x0F);
    panel_spi_wr_reg(0xA0,0x09);
    panel_spi_wr_reg(0xA1,0x0B);
    panel_spi_wr_reg(0xB0,0x05);
    panel_spi_wr_reg(0xB1,0x1F);
    panel_spi_wr_reg(0xB2,0x03);
    panel_spi_wr_reg(0xB3,0x01);
    panel_spi_wr_reg(0xB4,0x1E);
    panel_spi_wr_reg(0xB5,0x1E);
    panel_spi_wr_reg(0xB6,0x1f);
    panel_spi_wr_reg(0xB7,0x1E);
    panel_spi_wr_reg(0xB8,0x0B);
    panel_spi_wr_reg(0xB9,0x09);
    panel_spi_wr_reg(0xBA,0x0F);
    panel_spi_wr_reg(0xBB,0x0D);
    panel_spi_wr_reg(0xC6,0x05);
    panel_spi_wr_reg(0xC7,0x1F);
    panel_spi_wr_reg(0xC8,0x04);
    panel_spi_wr_reg(0xC9,0x02);
    panel_spi_wr_reg(0xCA,0x1E);
    panel_spi_wr_reg(0xCB,0x1E);
    panel_spi_wr_reg(0xCC,0x1f);
    panel_spi_wr_reg(0xCD,0x1E);
    panel_spi_wr_reg(0xCE,0x0C);
    panel_spi_wr_reg(0xCF,0x0A);
    panel_spi_wr_reg(0xD0,0x10);
    panel_spi_wr_reg(0xD1,0x0E);
    panel_spi_wr_reg(0xFF,0x30);
    panel_spi_wr_reg(0xFF,0x52);
    panel_spi_wr_reg(0xFF,0x00);
    panel_spi_wr_reg(0x36,0x02);//反扫09
    // panel_spi_wr_reg(0x3A,0x55);//16BIT

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);
    
    panel_spi_cmd_wr(0x29);
    aic_delay_ms(100);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs nv3052_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing nv3052_timing = {
    .pixelclock = 37*1000*1000,
    .hactive = 720,
    .hfront_porch = 46,
    .hback_porch = 44,
    .hsync_len = 2,
    .vactive = 720,
    .vfront_porch = 16,
    .vback_porch = 15,
    .vsync_len = 5,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_16BIT_HD,
    .clock_phase = DEGREE_90,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_nv3052 = {
    .name = "panel-nv3052",
    .timings = &nv3052_timing,
    .funcs = &nv3052_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

