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

    spi_init_seq_single(0xFF, 0x30);
    spi_init_seq_single(0xFF, 0x52);
    spi_init_seq_single(0xFF, 0x01);
    spi_init_seq_single(0xE3, 0x00);
    spi_init_seq_single(0x0A, 0x01);
    spi_init_seq_single(0x23, 0xA2);//RGB MODE
    spi_init_seq_single(0x25, 0x14);
    spi_init_seq_single(0x29, 0x02);
    spi_init_seq_single(0x2A, 0xCF);
    spi_init_seq_single(0x38, 0x9C);
    spi_init_seq_single(0x39, 0xA7);
    spi_init_seq_single(0x3A, 0x33);//VCOM
    spi_init_seq_single(0x91, 0x77);
    spi_init_seq_single(0x92, 0x77);
    spi_init_seq_single(0x99, 0x52);
    spi_init_seq_single(0x9B, 0x5B);
    spi_init_seq_single(0xA0, 0x55);
    spi_init_seq_single(0xA1, 0x50);
    spi_init_seq_single(0xA4, 0x9C);
    spi_init_seq_single(0xA7, 0x02);
    spi_init_seq_single(0xA8, 0x01);
    spi_init_seq_single(0xA9, 0x01);
    spi_init_seq_single(0xAA, 0xFC);
    spi_init_seq_single(0xAB, 0x28);
    spi_init_seq_single(0xAC, 0x06);
    spi_init_seq_single(0xAD, 0x06);
    spi_init_seq_single(0xAE, 0x06);
    spi_init_seq_single(0xAF, 0x03);
    spi_init_seq_single(0xB0, 0x08);
    spi_init_seq_single(0xB1, 0x26);
    spi_init_seq_single(0xB2, 0x28);
    spi_init_seq_single(0xB3, 0x28);
    spi_init_seq_single(0xB4, 0x03);
    spi_init_seq_single(0xB5, 0x08);
    spi_init_seq_single(0xB6, 0x26);
    spi_init_seq_single(0xB7, 0x08);
    spi_init_seq_single(0xB8, 0x26);
    spi_init_seq_single(0xFF, 0x30);
    spi_init_seq_single(0xFF, 0x52);
    spi_init_seq_single(0xFF, 0x02);
    spi_init_seq_single(0xB0, 0x02);
    spi_init_seq_single(0xB1, 0x31);
    spi_init_seq_single(0xB2, 0x24);
    spi_init_seq_single(0xB3, 0x30);
    spi_init_seq_single(0xB4, 0x38);
    spi_init_seq_single(0xB5, 0x3E);
    spi_init_seq_single(0xB6, 0x26);
    spi_init_seq_single(0xB7, 0x3E);
    spi_init_seq_single(0xB8, 0x0a);
    spi_init_seq_single(0xB9, 0x00);
    spi_init_seq_single(0xBA, 0x11);
    spi_init_seq_single(0xBB, 0x11);
    spi_init_seq_single(0xBC, 0x13);
    spi_init_seq_single(0xBD, 0x14);
    spi_init_seq_single(0xBE, 0x18);
    spi_init_seq_single(0xBF, 0x11);
    spi_init_seq_single(0xC0, 0x16);
    spi_init_seq_single(0xC1, 0x00);
    spi_init_seq_single(0xD0, 0x05);
    spi_init_seq_single(0xD1, 0x30);
    spi_init_seq_single(0xD2, 0x25);
    spi_init_seq_single(0xD3, 0x35);
    spi_init_seq_single(0xD4, 0x34);
    spi_init_seq_single(0xD5, 0x3B);
    spi_init_seq_single(0xD6, 0x26);
    spi_init_seq_single(0xD7, 0x3D);
    spi_init_seq_single(0xD8, 0x0a);
    spi_init_seq_single(0xD9, 0x00);
    spi_init_seq_single(0xDA, 0x12);
    spi_init_seq_single(0xDB, 0x10);
    spi_init_seq_single(0xDC, 0x12);
    spi_init_seq_single(0xDD, 0x14);
    spi_init_seq_single(0xDE, 0x18);
    spi_init_seq_single(0xDF, 0x11);
    spi_init_seq_single(0xE0, 0x15);
    spi_init_seq_single(0xE1, 0x00);
    spi_init_seq_single(0xFF, 0x30);
    spi_init_seq_single(0xFF, 0x52);
    spi_init_seq_single(0xFF, 0x03);
    spi_init_seq_single(0x08, 0x09);
    spi_init_seq_single(0x09, 0x0A);
    spi_init_seq_single(0x0A, 0x0B);
    spi_init_seq_single(0x0B, 0x0C);
    spi_init_seq_single(0x28, 0x22);
    spi_init_seq_single(0x2A, 0xEC);
    spi_init_seq_single(0x2B, 0xEC);
    spi_init_seq_single(0x30, 0x00);
    spi_init_seq_single(0x31, 0x00);
    spi_init_seq_single(0x32, 0x00);
    spi_init_seq_single(0x33, 0x00);
    spi_init_seq_single(0x34, 0x61);
    spi_init_seq_single(0x35, 0xD4);
    spi_init_seq_single(0x36, 0x24);
    spi_init_seq_single(0x37, 0x03);
    spi_init_seq_single(0x40, 0x0D);
    spi_init_seq_single(0x41, 0x0E);
    spi_init_seq_single(0x42, 0x0F);
    spi_init_seq_single(0x43, 0x10);
    spi_init_seq_single(0x44, 0x22);
    spi_init_seq_single(0x45, 0xE1);
    spi_init_seq_single(0x46, 0xE2);
    spi_init_seq_single(0x47, 0x22);
    spi_init_seq_single(0x48, 0xE3);
    spi_init_seq_single(0x49, 0xE4);
    spi_init_seq_single(0x50, 0x11);
    spi_init_seq_single(0x51, 0x12);
    spi_init_seq_single(0x52, 0x13);
    spi_init_seq_single(0x53, 0x14);
    spi_init_seq_single(0x54, 0x22);
    spi_init_seq_single(0x55, 0xE5);
    spi_init_seq_single(0x56, 0xE6);
    spi_init_seq_single(0x57, 0x22);
    spi_init_seq_single(0x58, 0xE7);
    spi_init_seq_single(0x59, 0xE8);
    spi_init_seq_single(0x80, 0x05);
    spi_init_seq_single(0x81, 0x1E);
    spi_init_seq_single(0x82, 0x02);
    spi_init_seq_single(0x83, 0x04);
    spi_init_seq_single(0x84, 0x1E);
    spi_init_seq_single(0x85, 0x1E);
    spi_init_seq_single(0x86, 0x1f);
    spi_init_seq_single(0x87, 0x1f);
    spi_init_seq_single(0x88, 0x0E);
    spi_init_seq_single(0x89, 0x10);
    spi_init_seq_single(0x8A, 0x0A);
    spi_init_seq_single(0x8B, 0x0C);
    spi_init_seq_single(0x96, 0x05);
    spi_init_seq_single(0x97, 0x1E);
    spi_init_seq_single(0x98, 0x01);
    spi_init_seq_single(0x99, 0x03);
    spi_init_seq_single(0x9A, 0x1E);
    spi_init_seq_single(0x9B, 0x1E);
    spi_init_seq_single(0x9C, 0x1f);
    spi_init_seq_single(0x9D, 0x1f);
    spi_init_seq_single(0x9E, 0x0D);
    spi_init_seq_single(0x9F, 0x0F);
    spi_init_seq_single(0xA0, 0x09);
    spi_init_seq_single(0xA1, 0x0B);
    spi_init_seq_single(0xB0, 0x05);
    spi_init_seq_single(0xB1, 0x1F);
    spi_init_seq_single(0xB2, 0x03);
    spi_init_seq_single(0xB3, 0x01);
    spi_init_seq_single(0xB4, 0x1E);
    spi_init_seq_single(0xB5, 0x1E);
    spi_init_seq_single(0xB6, 0x1f);
    spi_init_seq_single(0xB7, 0x1E);
    spi_init_seq_single(0xB8, 0x0B);
    spi_init_seq_single(0xB9, 0x09);
    spi_init_seq_single(0xBA, 0x0F);
    spi_init_seq_single(0xBB, 0x0D);
    spi_init_seq_single(0xC6, 0x05);
    spi_init_seq_single(0xC7, 0x1F);
    spi_init_seq_single(0xC8, 0x04);
    spi_init_seq_single(0xC9, 0x02);
    spi_init_seq_single(0xCA, 0x1E);
    spi_init_seq_single(0xCB, 0x1E);
    spi_init_seq_single(0xCC, 0x1f);
    spi_init_seq_single(0xCD, 0x1E);
    spi_init_seq_single(0xCE, 0x0C);
    spi_init_seq_single(0xCF, 0x0A);
    spi_init_seq_single(0xD0, 0x10);
    spi_init_seq_single(0xD1, 0x0E);
    spi_init_seq_single(0xFF, 0x30);
    spi_init_seq_single(0xFF, 0x52);
    spi_init_seq_single(0xFF, 0x00);
    spi_init_seq_single(0x36, 0x02);//反扫09
    spi_init_seq_single(0x3A, 0x77);//18BIT
    spi_init_seq_single(0x11, 0x00);
    aic_delay_ms(200);
    spi_init_seq_single(0x29, 0x00);
    aic_delay_ms(100);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h040a20_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h040a20_timing = {
    .pixelclock = 37 * 1000 * 1000,
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
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h040a20 = {
    .name = "panel-h040a20",
    .timings = &h040a20_timing,
    .funcs = &h040a20_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

