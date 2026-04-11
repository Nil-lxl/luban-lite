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

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xF7);
    panel_spi_data_wr(0x49);
    panel_spi_data_wr(0x61);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0x04);
    panel_spi_data_wr(0x81);
    panel_spi_cmd_wr(0x05);
    panel_spi_data_wr(0x00); //05=01,bist
    panel_spi_cmd_wr(0x06);
    panel_spi_data_wr(0x21); //06=60,bist

    panel_spi_cmd_wr(0x09);
    panel_spi_data_wr(0xE0);
    panel_spi_cmd_wr(0x0A);
    panel_spi_data_wr(0x41);
    panel_spi_cmd_wr(0x0B);
    panel_spi_data_wr(0x20);

    panel_spi_cmd_wr(0x29);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x2A);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0x2B);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x2C);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0x00);
    panel_spi_data_wr(0x15);
    panel_spi_cmd_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_cmd_wr(0x03);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0x04);
    panel_spi_data_wr(0x01);
    panel_spi_cmd_wr(0x05);
    panel_spi_data_wr(0x60);
    panel_spi_cmd_wr(0x06);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0x07);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x08);
    panel_spi_data_wr(0x3D);
    panel_spi_cmd_wr(0x09);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0x0A);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0x0B);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x1C);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x11);

    panel_spi_cmd_wr(0x0C);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0x0D);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x1D);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x19);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x11);

    panel_spi_cmd_wr(0x0E);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0x0F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1D);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x11);


    panel_spi_cmd_wr(0x10);
    panel_spi_data_wr(0x16);
    panel_spi_data_wr(0x19);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0x11);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1C);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x11);

    panel_spi_cmd_wr(0x12);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0x14);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x38);

    panel_spi_cmd_wr(0x18);
    panel_spi_data_wr(0x82);

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x06);

    panel_spi_cmd_wr(0x12);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x3D);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x2E);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x1C);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x13);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x3D);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x32);
    panel_spi_data_wr(0x2E);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x1C);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x08); //RGB&LVDS add
    panel_spi_cmd_wr(0x05);
    panel_spi_data_wr(0x01); //RGB&LVDS add
    panel_spi_cmd_wr(0x0C);
    panel_spi_data_wr(0x1A); //RGB&LVDS add
    panel_spi_cmd_wr(0x0D);
    panel_spi_data_wr(0x0E); //RGB&LVDS add

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x0A);
    panel_spi_cmd_wr(0x02);
    panel_spi_data_wr(0x4F);
    panel_spi_cmd_wr(0x0B);
    panel_spi_data_wr(0x40);

    panel_spi_cmd_wr(0x30);
    panel_spi_data_wr(0x0D);
    panel_spi_cmd_wr(0x0D);
    panel_spi_data_wr(0x0C); //mipi 0x04
    panel_spi_cmd_wr(0x10);
    panel_spi_data_wr(0x40);
    panel_spi_cmd_wr(0x11);
    panel_spi_data_wr(0x50);
    panel_spi_cmd_wr(0x12);
    panel_spi_data_wr(0x50);
    panel_spi_cmd_wr(0x13);
    panel_spi_data_wr(0x40);

    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x02);

    aic_delay_ms(100);
    // panel_spi_cmd_wr(0x11);
    // aic_delay_ms(100);

    // panel_spi_cmd_wr(0x29);
    // aic_delay_ms(20);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h050a18_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h050a18_timing = {
    .pixelclock = 32 * 1000 * 1000,
    .hactive = 800,
    .hfront_porch = 112,
    .hback_porch = 64,
    .hsync_len = 24,
    .vactive = 480,
    .vfront_porch = 21,
    .vback_porch = 37,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h050a18 = {
    .name = "panel-h050a18",
    .timings = &h050a18_timing,
    .funcs = &h050a18_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

