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
    panel_spi_data_wr(0x63);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x1e);
    panel_spi_data_wr(0x0C);

    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x07);

    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x38);

    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x1F);

    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x1F);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);


    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x95);


    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x63);

    panel_spi_cmd_wr(0xB2);
    panel_spi_data_wr(0x87);

    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);

    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x4E);

    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);

    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x31);

    panel_spi_cmd_wr(0xB9);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x1F);

    panel_spi_cmd_wr(0xBB);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xBC);
    panel_spi_data_wr(0x3E);

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
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);

    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x0f);
    panel_spi_data_wr(0xD8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0f);
    panel_spi_data_wr(0xD8);
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
    panel_spi_data_wr(0x1a);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x1c);
    panel_spi_data_wr(0x42);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x3c);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x3e);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);

    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x33);

    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x1b);
    panel_spi_data_wr(0x41);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x1d);
    panel_spi_data_wr(0x43);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x17);
    panel_spi_data_wr(0x3d);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x19);
    panel_spi_data_wr(0x3f);
    panel_spi_data_wr(0xff);
    panel_spi_data_wr(0xB4);


    panel_spi_cmd_wr(0xE9);
    panel_spi_data_wr(0xf6);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x4E);
    panel_spi_data_wr(0x4E);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x88);
    panel_spi_data_wr(0x40);

    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0xc1);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xAA);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0xF6);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x5F);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x47);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0xAA);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x1c);
    panel_spi_data_wr(0x2B);

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
    panel_spi_data_wr(0x13);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0E);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x11);

    aic_delay_ms(120);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);

    aic_delay_ms(10);

    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x13);

    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x7C);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(120);

    // panel_spi_cmd_wr(0x3A);
    // panel_spi_data_wr(0x50);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h040a30_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h040a30_timing = {
    .pixelclock = 26 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 15,
    .hback_porch = 2,
    .hsync_len = 15,
    .vactive = 800,
    .vfront_porch = 20,
    .vback_porch = 20,
    .vsync_len = 10,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h040a30 = {
    .name = "panel-h040a30",
    .timings = &h040a30_timing,
    .funcs = &h040a30_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

