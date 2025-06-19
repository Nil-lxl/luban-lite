/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include <aic_hal.h>

#ifdef AIC_USING_D213ECV_EzUIX1_DEMO
#define RESET_PIN  "PA.3"

#define CS         "PE.17"
#define SCL        "PE.16"
#define SDI        "PE.18"
#endif
#ifdef AIC_USING_JYX68_RGB01
#define RESET_PIN  "PB.1"
#define CS         "PE.13"
#define SCL        "PE.12"
#define SDI        "PE.14"
#endif
static struct gpio_desc reset_gpio;
// static struct gpio_desc sleep_gpio;

static void panel_gpio_init(void)
{
    panel_get_gpio(&reset_gpio, RESET_PIN);

    aic_delay_ms(1);
    panel_gpio_set_value(&reset_gpio, 0);
    aic_delay_ms(20);
    panel_gpio_set_value(&reset_gpio, 1);
    aic_delay_ms(120);
}

static int panel_enable(struct aic_panel *panel)
{
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

    // panel_spi_cmd_wr(0x3A);
    // panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0C);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x14);
    panel_spi_cmd_wr(0xCC);
    panel_spi_data_wr(0x10);
    panel_spi_cmd_wr(0xB0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x2F);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0xB1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x13);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x27);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x1F);
    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x11);
    panel_spi_cmd_wr(0xB0);//VOP
    panel_spi_data_wr(0x4D);//
    panel_spi_cmd_wr(0xB1);//VCOM
    panel_spi_data_wr(0x4D);
    panel_spi_cmd_wr(0xB2);//VGH
    panel_spi_data_wr(0x87);
    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x80);
    panel_spi_cmd_wr(0xB5);//VGL
    panel_spi_data_wr(0x45);
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x85);
    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x20);
    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x09);
    panel_spi_cmd_wr(0xC1);
    panel_spi_data_wr(0x78);
    panel_spi_cmd_wr(0xC2);
    panel_spi_data_wr(0x78);
    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x88);
    aic_delay_ms (100);
    
    panel_spi_cmd_wr(0xE0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x02);
    panel_spi_cmd_wr(0xE1);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE2);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x96);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x96);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xE3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x22);
    panel_spi_cmd_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE5);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x90);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x92);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x8C);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x8E);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_cmd_wr(0xE6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x22);
    panel_spi_data_wr(0x22);
    panel_spi_cmd_wr(0xE7);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x44);
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x91);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x93);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x8D);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x8F);
    panel_spi_data_wr(0xB0);
    panel_spi_data_wr(0xA0);
    panel_spi_cmd_wr(0xE9);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_cmd_wr(0xEB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x44);
    panel_spi_data_wr(0x88);
    panel_spi_data_wr(0x40);
    panel_spi_cmd_wr(0xED);
    panel_spi_data_wr(0xC1);
    panel_spi_data_wr(0xA2);
    panel_spi_data_wr(0xBF);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x67);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0xFF);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0xF0);
    panel_spi_data_wr(0xFB);
    panel_spi_data_wr(0x2A);
    panel_spi_data_wr(0x1C);
    panel_spi_cmd_wr(0xEF);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x1F);
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
    aic_delay_ms(50);
    
    panel_spi_cmd_wr(0xE8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xFF);
    panel_spi_data_wr(0x77);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(120);

    // panel_spi_cmd_wr(0x11);
    // aic_delay_ms(120);
    
    // panel_spi_cmd_wr(0x35);
    // panel_spi_data_wr(0x00);
    // aic_delay_ms(120);

    // panel_spi_cmd_wr(0x29);
    // aic_delay_ms(20);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h024a13_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h024a13_timing = {
    .pixelclock = 30 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 80,
    .hback_porch = 70,
    .hsync_len = 5,
    .vactive = 640,
    .vfront_porch = 30,
    .vback_porch = 40,
    .vsync_len = 15,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_h024a13 = {
    .name = "panel-h024a13",
    .timings = &h024a13_timing,
    .funcs = &h024a13_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

