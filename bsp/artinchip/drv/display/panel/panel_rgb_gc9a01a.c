/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"

#define RESET_PIN  "PC.3"

#define CS         "PD.1"
#define SCL        "PD.0"
#define SDI        "PD.2"

static struct gpio_desc reset_gpio;

static void panel_gpio_init(struct aic_panel *panel)
{
    panel_get_gpio(&reset_gpio, RESET_PIN);

    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(50);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init(panel);

    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_cmd_wr(0xFE);
    panel_spi_cmd_wr(0xEF);

    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x14);

    panel_spi_cmd_wr(0x84);
    panel_spi_data_wr(0x61);

    panel_spi_cmd_wr(0x85);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x86);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x87);
    panel_spi_data_wr(0xFF);
    panel_spi_cmd_wr(0x8E);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x8F);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x88);
    panel_spi_data_wr(0x0A);

    panel_spi_cmd_wr(0x89);
    panel_spi_data_wr(0x23);

    panel_spi_cmd_wr(0x8A);
    panel_spi_data_wr(0x40);
    panel_spi_cmd_wr(0x8B);
    panel_spi_data_wr(0x80);

    panel_spi_cmd_wr(0x8C);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0x8D);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xB6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x48);

    panel_spi_cmd_wr(0x3A);
    panel_spi_data_wr(0x55);

    panel_spi_cmd_wr(0xb0);
    panel_spi_data_wr(0x40);

    panel_spi_cmd_wr(0xF6);
    panel_spi_data_wr(0xc6);

    panel_spi_cmd_wr(0xb5);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x14);

    panel_spi_cmd_wr(0x90);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);

    panel_spi_cmd_wr(0xBA);
    panel_spi_data_wr(0x0A);

    panel_spi_cmd_wr(0xBD);
    panel_spi_data_wr(0x06);

    panel_spi_cmd_wr(0xBC);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x60);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x04);

    panel_spi_cmd_wr(0xC3);
    panel_spi_data_wr(0x13);
    panel_spi_cmd_wr(0xC4);
    panel_spi_data_wr(0x13);

    panel_spi_cmd_wr(0xC9);
    panel_spi_data_wr(0x22);

    panel_spi_cmd_wr(0xBE);
    panel_spi_data_wr(0x11);

    panel_spi_cmd_wr(0xE1);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0E);

    panel_spi_cmd_wr(0xDF);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x0c);
    panel_spi_data_wr(0x02);

    panel_spi_cmd_wr(0xF0);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x2A);

    panel_spi_cmd_wr(0xF1);
    panel_spi_data_wr(0x43);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x72);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x6F);

    panel_spi_cmd_wr(0xF2);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x2A);

    panel_spi_cmd_wr(0xF3);
    panel_spi_data_wr(0x43);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x72);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x6F);

    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0x1B);
    panel_spi_data_wr(0x0B);

    panel_spi_cmd_wr(0xAE);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0xCD);
    panel_spi_data_wr(0x63);

    panel_spi_cmd_wr(0x70);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x14);

    panel_spi_cmd_wr(0x62);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x71);
    panel_spi_data_wr(0xED);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x71);
    panel_spi_data_wr(0xEF);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);

    panel_spi_cmd_wr(0x63);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x71);
    panel_spi_data_wr(0xF1);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x71);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);

    panel_spi_cmd_wr(0x64);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0xF1);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x07);

    panel_spi_cmd_wr(0x66);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCD);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x67);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x98);

    panel_spi_cmd_wr(0x74);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x94);
    panel_spi_data_wr(0x80);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x4E);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x98);
    panel_spi_data_wr(0x3e);
    panel_spi_data_wr(0x07);

    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x21);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(320);
    panel_spi_cmd_wr(0x29);
    aic_delay_ms(120);
    panel_spi_cmd_wr(0x2C);
    aic_delay_ms(120);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs gc9a01a_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing gc9a01a_timing = {
    .pixelclock = 4000000,
    .hactive = 240,
    .hfront_porch = 10,
    .hback_porch = 20,
    .hsync_len = 10,
    .vactive = 240,
    .vfront_porch = 4,
    .vback_porch = 2,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_16BIT_LD,
    .clock_phase = DEGREE_0,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_gc9a01a = {
    .name = "panel-gc9a01a",
    .timings = &gc9a01a_timing,
    .funcs = &gc9a01a_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

