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

void panel_spi_wr_reg(u8 cmd, u8 data) {
    panel_spi_cmd_wr(cmd);
    panel_spi_data_wr(data);
}

static int panel_enable(struct aic_panel *panel) {

    panel_gpio_init();

    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x01);
    panel_spi_wr_reg(0xE3, 0x00);
    panel_spi_wr_reg(0x0A, 0x01);
    panel_spi_wr_reg(0x23, 0xA2);
    panel_spi_wr_reg(0x24, 0x10);
    panel_spi_wr_reg(0x25, 0x08);
    panel_spi_wr_reg(0x28, 0x50);
    panel_spi_wr_reg(0x29, 0x04);
    panel_spi_wr_reg(0x2a, 0xFf);
    panel_spi_wr_reg(0x38, 0x9C);
    panel_spi_wr_reg(0x39, 0xA7);
    panel_spi_wr_reg(0x3A, 0x4A);
    panel_spi_wr_reg(0x91, 0x67);
    panel_spi_wr_reg(0x98, 0xCA);
    panel_spi_wr_reg(0x99, 0x15);
    panel_spi_wr_reg(0x9A, 0xC1);
    panel_spi_wr_reg(0x9B, 0x1C);
    panel_spi_wr_reg(0xA0, 0x55);
    panel_spi_wr_reg(0xA1, 0x50);
    panel_spi_wr_reg(0xA4, 0x9C);
    panel_spi_wr_reg(0xA7, 0x02);
    panel_spi_wr_reg(0xA8, 0x01);
    panel_spi_wr_reg(0xA9, 0x01);
    panel_spi_wr_reg(0xAA, 0xFC);
    panel_spi_wr_reg(0xAB, 0x28);
    panel_spi_wr_reg(0xAC, 0x06);
    panel_spi_wr_reg(0xAD, 0x06);
    panel_spi_wr_reg(0xAE, 0x06);
    panel_spi_wr_reg(0xAF, 0x03);
    panel_spi_wr_reg(0xB0, 0x08);
    panel_spi_wr_reg(0xB1, 0x26);
    panel_spi_wr_reg(0xB2, 0x28);
    panel_spi_wr_reg(0xB3, 0x28);
    panel_spi_wr_reg(0xB4, 0x03);
    panel_spi_wr_reg(0xB5, 0x08);
    panel_spi_wr_reg(0xB6, 0x26);
    panel_spi_wr_reg(0xB7, 0x08);
    panel_spi_wr_reg(0xB8, 0x26);
    panel_spi_wr_reg(0xF6, 0xC0);
    panel_spi_wr_reg(0xF0, 0x00);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x02);
    panel_spi_wr_reg(0xB5, 0x3E);
    panel_spi_wr_reg(0xD5, 0x3C);
    panel_spi_wr_reg(0xB0, 0x08);
    panel_spi_wr_reg(0xD0, 0x0A);
    panel_spi_wr_reg(0xB4, 0x2F);
    panel_spi_wr_reg(0xD4, 0x2C);
    panel_spi_wr_reg(0xB1, 0x0B);
    panel_spi_wr_reg(0xD1, 0x08);
    panel_spi_wr_reg(0xB3, 0x2F);
    panel_spi_wr_reg(0xD3, 0x2D);
    panel_spi_wr_reg(0xB2, 0x05);
    panel_spi_wr_reg(0xD2, 0x05);
    panel_spi_wr_reg(0xB7, 0x26);
    panel_spi_wr_reg(0xD7, 0x25);
    panel_spi_wr_reg(0xB6, 0x02);
    panel_spi_wr_reg(0xD6, 0x01);
    panel_spi_wr_reg(0xC0, 0x17);
    panel_spi_wr_reg(0xE0, 0x19);
    panel_spi_wr_reg(0xC1, 0x04);
    panel_spi_wr_reg(0xE1, 0x04);
    panel_spi_wr_reg(0xBF, 0x11);
    panel_spi_wr_reg(0xDF, 0x11);
    panel_spi_wr_reg(0xB8, 0x0A);
    panel_spi_wr_reg(0xD8, 0x0A);
    panel_spi_wr_reg(0xBE, 0x19);
    panel_spi_wr_reg(0xDE, 0x19);
    panel_spi_wr_reg(0xB9, 0x00);
    panel_spi_wr_reg(0xD9, 0x01);
    panel_spi_wr_reg(0xBD, 0x0F);
    panel_spi_wr_reg(0xDD, 0x0F);
    panel_spi_wr_reg(0xBA, 0x0E);
    panel_spi_wr_reg(0xDA, 0x0D);
    panel_spi_wr_reg(0xBC, 0x0D);
    panel_spi_wr_reg(0xDC, 0x0B);
    panel_spi_wr_reg(0xBB, 0x0B);
    panel_spi_wr_reg(0xDB, 0x0B);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x03);
    panel_spi_wr_reg(0x07, 0x03);
    panel_spi_wr_reg(0x08, 0x00);
    panel_spi_wr_reg(0x09, 0x01);
    panel_spi_wr_reg(0x27, 0x03);
    panel_spi_wr_reg(0x2a, 0x08);
    panel_spi_wr_reg(0x2b, 0x08);
    panel_spi_wr_reg(0x34, 0x61);
    panel_spi_wr_reg(0x35, 0xD4);
    panel_spi_wr_reg(0x36, 0x24);
    panel_spi_wr_reg(0x37, 0x03);
    panel_spi_wr_reg(0x40, 0x01);
    panel_spi_wr_reg(0x41, 0x02);
    panel_spi_wr_reg(0x42, 0x03);
    panel_spi_wr_reg(0x43, 0x04);
    panel_spi_wr_reg(0x45, 0x06);
    panel_spi_wr_reg(0x46, 0x07);
    panel_spi_wr_reg(0x48, 0x08);
    panel_spi_wr_reg(0x49, 0x09);
    panel_spi_wr_reg(0x50, 0x05);
    panel_spi_wr_reg(0x51, 0x06);
    panel_spi_wr_reg(0x52, 0x07);
    panel_spi_wr_reg(0x53, 0x08);
    panel_spi_wr_reg(0x55, 0x0A);
    panel_spi_wr_reg(0x56, 0x0B);
    panel_spi_wr_reg(0x58, 0x0C);
    panel_spi_wr_reg(0x59, 0x0D);
    panel_spi_wr_reg(0x7E, 0x78);
    panel_spi_wr_reg(0x7F, 0x78);
    panel_spi_wr_reg(0x80, 0x05);
    panel_spi_wr_reg(0x81, 0x00);
    panel_spi_wr_reg(0x82, 0x01);
    panel_spi_wr_reg(0x83, 0x00);
    panel_spi_wr_reg(0x84, 0x1F);
    panel_spi_wr_reg(0x85, 0x15);
    panel_spi_wr_reg(0x86, 0x16);
    panel_spi_wr_reg(0x87, 0x0B);
    panel_spi_wr_reg(0x88, 0x09);
    panel_spi_wr_reg(0x89, 0x0F);
    panel_spi_wr_reg(0x8A, 0x0D);
    panel_spi_wr_reg(0x96, 0x06);
    panel_spi_wr_reg(0x97, 0x00);
    panel_spi_wr_reg(0x98, 0x02);
    panel_spi_wr_reg(0x99, 0x00);
    panel_spi_wr_reg(0x9A, 0x1F);
    panel_spi_wr_reg(0x9B, 0x15);
    panel_spi_wr_reg(0x9C, 0x16);
    panel_spi_wr_reg(0x9D, 0x0C);
    panel_spi_wr_reg(0x9E, 0x0A);
    panel_spi_wr_reg(0x9F, 0x10);
    panel_spi_wr_reg(0xA0, 0x0E);
    panel_spi_wr_reg(0xB0, 0x05);
    panel_spi_wr_reg(0xB1, 0x1F);
    panel_spi_wr_reg(0xB2, 0x01);
    panel_spi_wr_reg(0xB3, 0x00);
    panel_spi_wr_reg(0xB4, 0x00);
    panel_spi_wr_reg(0xB5, 0x15);
    panel_spi_wr_reg(0xB6, 0x16);
    panel_spi_wr_reg(0xB7, 0x0E);
    panel_spi_wr_reg(0xB8, 0x10);
    panel_spi_wr_reg(0xB9, 0x0A);
    panel_spi_wr_reg(0xBA, 0x0C);
    panel_spi_wr_reg(0xC6, 0x06);
    panel_spi_wr_reg(0xC7, 0x1F);
    panel_spi_wr_reg(0xC8, 0x02);
    panel_spi_wr_reg(0xC9, 0x00);
    panel_spi_wr_reg(0xCA, 0x00);
    panel_spi_wr_reg(0xCB, 0x15);
    panel_spi_wr_reg(0xCC, 0x16);
    panel_spi_wr_reg(0xCD, 0x0D);
    panel_spi_wr_reg(0xCE, 0x0F);
    panel_spi_wr_reg(0xCF, 0x09);
    panel_spi_wr_reg(0xD0, 0x0B);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x00);
    panel_spi_wr_reg(0x36, 0x0A);

    panel_spi_wr_reg(0x11, 0x00);
    aic_delay_ms(200);
    panel_spi_wr_reg(0x29, 0x00);
    aic_delay_ms(120);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h068a04_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h068a04_timing = {
    .pixelclock = 54 * 1000 * 1000,
    .hactive = 600,
    .hfront_porch = 46,
    .hback_porch = 44,
    .hsync_len = 2,
    .vactive = 1280,
    .vfront_porch = 16,
    .vback_porch = 6,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h068a04 = {
    .name = "panel-h068a04",
    .timings = &h068a04_timing,
    .funcs = &h068a04_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

