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

    panel_spi_cmd_wr(0xDF);
    panel_spi_data_wr(0x90);
    panel_spi_data_wr(0x85);
    panel_spi_data_wr(0x15);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x70);
    panel_spi_data_wr(0x70);

    panel_spi_cmd_wr(0xB6);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x5F);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0xC0);
    panel_spi_data_wr(0xEE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x5F);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x8F);

    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x9C);
    panel_spi_data_wr(0x9C);

    panel_spi_cmd_wr(0xBB);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0x2C);

    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xC3);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x78);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0xE8);

    panel_spi_cmd_wr(0xC4);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x22);

    panel_spi_cmd_wr(0xC8);
    panel_spi_data_wr(0x6E);
    panel_spi_data_wr(0x60);
    panel_spi_data_wr(0x57);
    panel_spi_data_wr(0x46);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x27);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0x2E);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x49);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x42);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0x2A);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x27);
    panel_spi_data_wr(0x6E);
    panel_spi_data_wr(0x60);
    panel_spi_data_wr(0x57);
    panel_spi_data_wr(0x46);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x27);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0x2E);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x49);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x42);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0x2A);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x27);

    panel_spi_cmd_wr(0xCB);
    panel_spi_data_wr(0x71);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x30);

    panel_spi_cmd_wr(0xCE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFC);

    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFC);
    panel_spi_data_wr(0x02);

    panel_spi_cmd_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x7C);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x7C);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x7C);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x7C);

    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xD2);
    panel_spi_data_wr(0xD0);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDD);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0xC0);
    panel_spi_data_wr(0xDB);
    panel_spi_data_wr(0xD9);
    panel_spi_data_wr(0xCE);
    panel_spi_data_wr(0xCC);
    panel_spi_data_wr(0xCA);
    panel_spi_data_wr(0xC8);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0xC4);

    panel_spi_cmd_wr(0xD1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xD3);
    panel_spi_data_wr(0xD1);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDF);
    panel_spi_data_wr(0xDD);
    panel_spi_data_wr(0xC3);
    panel_spi_data_wr(0xC1);
    panel_spi_data_wr(0xDC);
    panel_spi_data_wr(0xDA);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0xCD);
    panel_spi_data_wr(0xCB);
    panel_spi_data_wr(0xC9);
    panel_spi_data_wr(0xC7);
    panel_spi_data_wr(0xC5);

    panel_spi_cmd_wr(0xD2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1D);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x1A);
    panel_spi_data_wr(0x1C);

    panel_spi_cmd_wr(0xD3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1D);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x19);
    panel_spi_data_wr(0x1B);

    panel_spi_cmd_wr(0xD4);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xBA);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xBA);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x80);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC0);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xBB);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xBB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x84);
    panel_spi_data_wr(0xCC);
    panel_spi_data_wr(0xC0);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xCA);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0F);

    panel_spi_cmd_wr(0xD5);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF7);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0xF7);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0x0F);

    panel_spi_cmd_wr(0xD6);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x04);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x02);

    panel_spi_cmd_wr(0xC5);
    panel_spi_data_wr(0x4A);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x4A);
    panel_spi_data_wr(0xEC);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x59);
    panel_spi_data_wr(0x8D);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x4A);
    panel_spi_data_wr(0xEC);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x59);
    panel_spi_data_wr(0x8D);

    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0xDD);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0xDD);
    panel_spi_data_wr(0xEA);
    panel_spi_data_wr(0x86);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0xDC);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x41);
    panel_spi_data_wr(0xEA);
    panel_spi_data_wr(0x86);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0xDC);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x41);

    panel_spi_cmd_wr(0xD2);
    panel_spi_data_wr(0xF7); //42

    panel_spi_cmd_wr(0xD6);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x48);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x04);

    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x31);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x48);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(100);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h070a36_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h070a36_timing = {
    .pixelclock = 51 * 1000 * 1000,
    .hactive = 1024,
    .hfront_porch = 100,
    .hback_porch = 11,
    .hsync_len = 4,
    .vactive = 600,
    .vfront_porch = 120,
    .vback_porch = 23,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_h070a36 = {
    .name = "panel-h070a36",
    .timings = &h070a36_timing,
    .funcs = &h070a36_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

