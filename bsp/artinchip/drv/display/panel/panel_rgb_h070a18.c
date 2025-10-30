/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include <aic_hal.h>

#if defined AIC_USING_D213ECV_EzUIX1_DEMO_V1
#define RESET_PIN  "PB.6"
#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.18"
#elif defined AIC_USING_D213ECV_EzUIX1_DEMO_V0
#define RESET_PIN  "PA.3"
#define CS         "PE.17"
#define SCL        "PE.16"
#define SDI        "PE.18"
#elif defined AIC_USING_H215_DEMO_A02_V0
#define RESET_PIN  "PF.0"
#define CS         "PC.6"
#define SCL        "PF.15"
#define SDI        "PF.14"
#elif defined AIC_USING_JYX68_RGB01
#define RESET_PIN  "PB.1"
#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.14"
#endif

#ifdef RESET_PIN
static struct gpio_desc reset_gpio;
static void panel_gpio_init(void) {
    panel_get_gpio(&reset_gpio, RESET_PIN);
    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}
#endif

void panel_spi_wr_reg(u8 cmd, u8 data) {
    panel_spi_cmd_wr(cmd);
    panel_spi_data_wr(data);
}

static int panel_enable(struct aic_panel *panel) {
#ifdef RESET_PIN
    panel_gpio_init();
#endif
    panel_spi_device_emulation(CS, SDI, SCL);

    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x01);
    panel_spi_wr_reg(0xE3, 0x00);
    panel_spi_wr_reg(0x08, 0x00);
    panel_spi_wr_reg(0x09, 0x07);
    panel_spi_wr_reg(0x0A, 0xfd);
    panel_spi_wr_reg(0x0B, 0x32);
    panel_spi_wr_reg(0x0C, 0x32);
    panel_spi_wr_reg(0x0D, 0x0B);
    panel_spi_wr_reg(0x0E, 0x00);
    panel_spi_wr_reg(0x24, 0x10);
    panel_spi_wr_reg(0x25, 0x0a);
    panel_spi_wr_reg(0x20, 0xA0);
    panel_spi_wr_reg(0x28, 0x50);
    panel_spi_wr_reg(0x29, 0xc5);
    panel_spi_wr_reg(0x2a, 0x90);
    panel_spi_wr_reg(0x38, 0x9C);
    panel_spi_wr_reg(0x39, 0xA7);
    panel_spi_wr_reg(0x3A, 0x37);
    panel_spi_wr_reg(0x49, 0x3C);
    panel_spi_wr_reg(0x6D, 0x00);
    panel_spi_wr_reg(0x6E, 0x00);
    panel_spi_wr_reg(0x80, 0x20);
    panel_spi_wr_reg(0x91, 0x67);
    panel_spi_wr_reg(0x92, 0x67);
    panel_spi_wr_reg(0x99, 0x51);
    panel_spi_wr_reg(0x9b, 0x59);
    panel_spi_wr_reg(0xA0, 0x55);
    panel_spi_wr_reg(0xA1, 0x50);
    panel_spi_wr_reg(0xA3, 0xD8);
    panel_spi_wr_reg(0xA4, 0x9C);
    panel_spi_wr_reg(0xA7, 0x02);
    panel_spi_wr_reg(0xA8, 0x01);
    panel_spi_wr_reg(0xA9, 0x01);
    panel_spi_wr_reg(0xAA, 0xA8);
    panel_spi_wr_reg(0xAB, 0x28);
    panel_spi_wr_reg(0xAC, 0xE0);
    panel_spi_wr_reg(0xAD, 0xE2);
    panel_spi_wr_reg(0xAE, 0xE2);
    panel_spi_wr_reg(0xAF, 0x02);
    panel_spi_wr_reg(0xB0, 0xE2);
    panel_spi_wr_reg(0xB1, 0x26);
    panel_spi_wr_reg(0xB2, 0x28);
    panel_spi_wr_reg(0xB3, 0x28);
    panel_spi_wr_reg(0xB4, 0x22);
    panel_spi_wr_reg(0xB5, 0xE2);
    panel_spi_wr_reg(0xB6, 0x26);
    panel_spi_wr_reg(0xB7, 0xE2);
    panel_spi_wr_reg(0xB8, 0x26);
    panel_spi_wr_reg(0xF6, 0xC0);
    panel_spi_wr_reg(0xF0, 0x00);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x02);
    panel_spi_wr_reg(0xB1, 0x0B);
    panel_spi_wr_reg(0xD1, 0x0B);
    panel_spi_wr_reg(0xB4, 0x31);
    panel_spi_wr_reg(0xD4, 0x2F);
    panel_spi_wr_reg(0xB2, 0x09);
    panel_spi_wr_reg(0xD2, 0x09);
    panel_spi_wr_reg(0xB3, 0x2F);
    panel_spi_wr_reg(0xD3, 0x2D);
    panel_spi_wr_reg(0xB6, 0x14);
    panel_spi_wr_reg(0xD6, 0x13);
    panel_spi_wr_reg(0xB7, 0x37);
    panel_spi_wr_reg(0xD7, 0x36);
    panel_spi_wr_reg(0xC1, 0x08);
    panel_spi_wr_reg(0xE1, 0x07);
    panel_spi_wr_reg(0xB8, 0x0C);
    panel_spi_wr_reg(0xD8, 0x0C);
    panel_spi_wr_reg(0xB9, 0x03);
    panel_spi_wr_reg(0xD9, 0x02);
    panel_spi_wr_reg(0xBD, 0x13);
    panel_spi_wr_reg(0xDD, 0x13);
    panel_spi_wr_reg(0xBC, 0x11);
    panel_spi_wr_reg(0xDC, 0x11);
    panel_spi_wr_reg(0xBB, 0x10);
    panel_spi_wr_reg(0xDB, 0x0F);
    panel_spi_wr_reg(0xBA, 0x10);
    panel_spi_wr_reg(0xDA, 0x10);
    panel_spi_wr_reg(0xBE, 0x18);
    panel_spi_wr_reg(0xDE, 0x1A);
    panel_spi_wr_reg(0xBF, 0x0F);
    panel_spi_wr_reg(0xDF, 0x11);
    panel_spi_wr_reg(0xC0, 0x16);
    panel_spi_wr_reg(0xE0, 0x18);
    panel_spi_wr_reg(0xB5, 0x37);
    panel_spi_wr_reg(0xD5, 0x32);
    panel_spi_wr_reg(0xB0, 0x02);
    panel_spi_wr_reg(0xD0, 0x05);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x03);
    panel_spi_wr_reg(0x04, 0x51);
    panel_spi_wr_reg(0x05, 0x50);
    panel_spi_wr_reg(0x06, 0x50);
    panel_spi_wr_reg(0x07, 0x03);
    panel_spi_wr_reg(0x08, 0x04);
    panel_spi_wr_reg(0x09, 0x05);
    panel_spi_wr_reg(0x0A, 0x06);
    panel_spi_wr_reg(0x0B, 0x07);
    panel_spi_wr_reg(0x24, 0x51);
    panel_spi_wr_reg(0x25, 0x50);
    panel_spi_wr_reg(0x26, 0x50);
    panel_spi_wr_reg(0x27, 0x03);
    panel_spi_wr_reg(0x28, 0x55);
    panel_spi_wr_reg(0x29, 0x55);
    panel_spi_wr_reg(0x2A, 0xa7);
    panel_spi_wr_reg(0x2B, 0xab);
    panel_spi_wr_reg(0x2c, 0xab);
    panel_spi_wr_reg(0x34, 0x51);
    panel_spi_wr_reg(0x35, 0x50);
    panel_spi_wr_reg(0x36, 0x50);
    panel_spi_wr_reg(0x37, 0x03);
    panel_spi_wr_reg(0x40, 0x04);
    panel_spi_wr_reg(0x41, 0x05);
    panel_spi_wr_reg(0x42, 0x06);
    panel_spi_wr_reg(0x43, 0x07);
    panel_spi_wr_reg(0x44, 0x55);
    panel_spi_wr_reg(0x45, 0x9f);
    panel_spi_wr_reg(0x46, 0xa0);
    panel_spi_wr_reg(0x47, 0x55);
    panel_spi_wr_reg(0x48, 0xa1);
    panel_spi_wr_reg(0x49, 0xa2);
    panel_spi_wr_reg(0x50, 0x08);
    panel_spi_wr_reg(0x51, 0x09);
    panel_spi_wr_reg(0x52, 0x0a);
    panel_spi_wr_reg(0x53, 0x0b);
    panel_spi_wr_reg(0x54, 0x55);
    panel_spi_wr_reg(0x55, 0xa3);
    panel_spi_wr_reg(0x56, 0xa4);
    panel_spi_wr_reg(0x57, 0x55);
    panel_spi_wr_reg(0x58, 0xa5);
    panel_spi_wr_reg(0x59, 0xa6);
    panel_spi_wr_reg(0x60, 0x03);
    panel_spi_wr_reg(0x61, 0x03);
    panel_spi_wr_reg(0x64, 0x00);
    panel_spi_wr_reg(0x65, 0x0a);
    panel_spi_wr_reg(0x66, 0x0a);
    panel_spi_wr_reg(0x80, 0x06);
    panel_spi_wr_reg(0x81, 0x11);
    panel_spi_wr_reg(0x82, 0x07);
    panel_spi_wr_reg(0x83, 0x03);
    panel_spi_wr_reg(0x84, 0x1f);
    panel_spi_wr_reg(0x85, 0x1e);
    panel_spi_wr_reg(0x86, 0x01);
    panel_spi_wr_reg(0x87, 0x1e);
    panel_spi_wr_reg(0x88, 0x09);
    panel_spi_wr_reg(0x89, 0x0b);
    panel_spi_wr_reg(0x8A, 0x0d);
    panel_spi_wr_reg(0x8B, 0x0f);
    panel_spi_wr_reg(0x96, 0x06);
    panel_spi_wr_reg(0x97, 0x11);
    panel_spi_wr_reg(0x98, 0x07);
    panel_spi_wr_reg(0x99, 0x04);
    panel_spi_wr_reg(0x9a, 0x1f);
    panel_spi_wr_reg(0x9b, 0x1e);
    panel_spi_wr_reg(0x9c, 0x02);
    panel_spi_wr_reg(0x9d, 0x1e);
    panel_spi_wr_reg(0x9e, 0x0a);
    panel_spi_wr_reg(0x9f, 0x0c);
    panel_spi_wr_reg(0xA0, 0x0e);
    panel_spi_wr_reg(0xA1, 0x10);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x02);
    panel_spi_wr_reg(0x28, 0x0B);
    panel_spi_wr_reg(0x29, 0x07);
    panel_spi_wr_reg(0x2A, 0x81);
    panel_spi_wr_reg(0x01, 0x01);
    panel_spi_wr_reg(0x02, 0xDA);
    panel_spi_wr_reg(0x03, 0xBA);
    panel_spi_wr_reg(0x04, 0xA8);
    panel_spi_wr_reg(0x05, 0x9A);
    panel_spi_wr_reg(0x06, 0x70);
    panel_spi_wr_reg(0x07, 0xFF);
    panel_spi_wr_reg(0x08, 0x91);
    panel_spi_wr_reg(0x09, 0x90);
    panel_spi_wr_reg(0x0A, 0xFF);
    panel_spi_wr_reg(0x0B, 0x8F);
    panel_spi_wr_reg(0x0C, 0x60);
    panel_spi_wr_reg(0x0D, 0x58);
    panel_spi_wr_reg(0x0E, 0x48);
    panel_spi_wr_reg(0x0F, 0x38);
    panel_spi_wr_reg(0x10, 0x2B);
    panel_spi_wr_reg(0xFF, 0x30);
    panel_spi_wr_reg(0xFF, 0x52);
    panel_spi_wr_reg(0xFF, 0x00);
    panel_spi_wr_reg(0x36, 0x0A);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(100);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h070a18_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h070a18_timing = {
    .pixelclock = 60 * 1000 * 1000,
    .hactive = 600,
    .hfront_porch = 30,
    .hback_porch = 30,
    .hsync_len = 10,
    .vactive = 1424,
    .vfront_porch = 40,
    .vback_porch = 10,
    .vsync_len = 1,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h070a18 = {
    .name = "panel-h070a18",
    .timings = &h070a18_timing,
    .funcs = &h070a18_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

