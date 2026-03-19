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

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xDF);
    panel_spi_data_wr(0x91);
    panel_spi_data_wr(0x63);
    panel_spi_data_wr(0xF4);

    panel_spi_cmd_wr(0xB6);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xBD);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6D);

    panel_spi_cmd_wr(0xBE);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x8D);

    panel_spi_cmd_wr(0xC0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0xBB);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x68);
    panel_spi_data_wr(0x26);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x54);
    panel_spi_data_wr(0x54);

    panel_spi_cmd_wr(0xC3);
    panel_spi_data_wr(0x2B);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x12);
    panel_spi_data_wr(0x6B);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x6B);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x15);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x67);

    panel_spi_cmd_wr(0xC5);
    panel_spi_data_wr(0x20);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x78);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x25);

    panel_spi_cmd_wr(0xC6);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x10);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0xCB);
    panel_spi_data_wr(0x7F);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0x58);
    panel_spi_data_wr(0x4A);
    panel_spi_data_wr(0x46);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x25);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x5E);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x24);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x7F);
    panel_spi_data_wr(0x66);
    panel_spi_data_wr(0x58);
    panel_spi_data_wr(0x4A);
    panel_spi_data_wr(0x46);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x3B);
    panel_spi_data_wr(0x25);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x5E);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x45);
    panel_spi_data_wr(0x38);
    panel_spi_data_wr(0x24);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xCE);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x14);
    panel_spi_data_wr(0x14);

    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xD1);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x3C);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xD2);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x06);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xD3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x07);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x1F);
    panel_spi_data_wr(0x55);
    panel_spi_data_wr(0x40);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xD4);
    panel_spi_data_wr(0x10);//1
    panel_spi_data_wr(0x00);//2
    panel_spi_data_wr(0x00);//3
    panel_spi_data_wr(0x0B);//4
    panel_spi_data_wr(0x00);//5
    panel_spi_data_wr(0x00);//6
    panel_spi_data_wr(0x00);//7
    panel_spi_data_wr(0x00);//8
    panel_spi_data_wr(0x00);//9
    panel_spi_data_wr(0x00);//10
    panel_spi_data_wr(0x00);//11
    panel_spi_data_wr(0x02);//12
    panel_spi_data_wr(0x02);//13
    panel_spi_data_wr(0x00);//14
    panel_spi_data_wr(0x00);//15
    panel_spi_data_wr(0x24);//16
    panel_spi_data_wr(0x30);//17
    panel_spi_data_wr(0x00);//18
    panel_spi_data_wr(0x00);//19
    panel_spi_data_wr(0x01);//20
    panel_spi_data_wr(0x01);//21
    panel_spi_data_wr(0x00);//22
    panel_spi_data_wr(0x01);//23
    panel_spi_data_wr(0x00);//24
    panel_spi_data_wr(0x04);//25
    panel_spi_data_wr(0x01);//26
    panel_spi_data_wr(0x01);//27
    panel_spi_data_wr(0x11);//28
    panel_spi_data_wr(0x60);//29
    panel_spi_data_wr(0x03);//30
    panel_spi_data_wr(0x00);//31
    panel_spi_data_wr(0x05);//32
    panel_spi_data_wr(0x00);//33
    panel_spi_data_wr(0x00);//34
    panel_spi_data_wr(0x00);//35
    panel_spi_data_wr(0x00);//36
    panel_spi_data_wr(0x00);//37
    panel_spi_data_wr(0x04);//38
    panel_spi_data_wr(0x0A);//39
    panel_spi_data_wr(0x04);//40
    panel_spi_data_wr(0x07);//41
    panel_spi_data_wr(0x00);//42
    panel_spi_data_wr(0x0F);//43
    panel_spi_data_wr(0x00);//44
    panel_spi_data_wr(0x00);//45
    panel_spi_data_wr(0x00);//46
    panel_spi_data_wr(0x00);//47
    panel_spi_data_wr(0x00);//48
    panel_spi_data_wr(0x00);//49
    panel_spi_data_wr(0x00);//50
    panel_spi_data_wr(0x00);//51
    panel_spi_data_wr(0x00);//52
    panel_spi_data_wr(0x00);//53
    panel_spi_data_wr(0x00);//54
    panel_spi_data_wr(0x00);//55
    panel_spi_data_wr(0x00);//56
    panel_spi_data_wr(0x00);//57
    panel_spi_data_wr(0x00);//58
    panel_spi_data_wr(0x00);//59
    panel_spi_data_wr(0x00);//60
    panel_spi_data_wr(0x00);//61
    panel_spi_data_wr(0x00);//62
    panel_spi_data_wr(0x00);//63

    panel_spi_cmd_wr(0xD5);
    panel_spi_data_wr(0x01);//1
    panel_spi_data_wr(0x10);//2
    panel_spi_data_wr(0x01);//3
    panel_spi_data_wr(0x00);//4
    panel_spi_data_wr(0x00);//5
    panel_spi_data_wr(0x00);//6
    panel_spi_data_wr(0x00);//7
    panel_spi_data_wr(0x00);//8
    panel_spi_data_wr(0x00);//9
    panel_spi_data_wr(0x43);//10
    panel_spi_data_wr(0x00);//11
    panel_spi_data_wr(0x00);//12
    panel_spi_data_wr(0x00);//13
    panel_spi_data_wr(0x07);//14
    panel_spi_data_wr(0x32);//15
    panel_spi_data_wr(0x5A);//16
    panel_spi_data_wr(0x00);//17
    panel_spi_data_wr(0x40);//18
    panel_spi_data_wr(0x05);//19
    panel_spi_data_wr(0x00);//20
    panel_spi_data_wr(0x01);//21
    panel_spi_data_wr(0x00);//22
    panel_spi_data_wr(0x30);//23
    panel_spi_data_wr(0x32);//24
    panel_spi_data_wr(0x00);//25
    panel_spi_data_wr(0x0B);//26
    panel_spi_data_wr(0x08);//27
    panel_spi_data_wr(0x00);//28
    panel_spi_data_wr(0x72);//29
    panel_spi_data_wr(0x00);//30
    panel_spi_data_wr(0x09);//31
    panel_spi_data_wr(0x03);//32
    panel_spi_data_wr(0x04);//33
    panel_spi_data_wr(0x06);//34
    panel_spi_data_wr(0x00);//35
    panel_spi_data_wr(0x00);//36
    panel_spi_data_wr(0x00);//37
    panel_spi_data_wr(0x00);//38
    panel_spi_data_wr(0xF0);//39
    panel_spi_data_wr(0xF0);//40
    panel_spi_data_wr(0x00);//41
    panel_spi_data_wr(0x00);//42

    panel_spi_cmd_wr(0xD7);
    panel_spi_data_wr(0x00);//1
    panel_spi_data_wr(0x74);//2
    panel_spi_data_wr(0x74);//3
    panel_spi_data_wr(0x74);//4
    panel_spi_data_wr(0x74);//5
    panel_spi_data_wr(0x02);//6
    panel_spi_data_wr(0x30);//7
    panel_spi_data_wr(0x03);//8
    panel_spi_data_wr(0x30);//9
    panel_spi_data_wr(0x06);//10
    panel_spi_data_wr(0x30);//11
    panel_spi_data_wr(0x06);//12
    panel_spi_data_wr(0x30);//13
    panel_spi_data_wr(0x74);//14
    panel_spi_data_wr(0x74);//15
    panel_spi_data_wr(0x40);//16
    panel_spi_data_wr(0x56);//17
    panel_spi_data_wr(0x10);//18
    panel_spi_data_wr(0x10);//19

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x01);

    panel_spi_cmd_wr(0xC5);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xCD);
    panel_spi_data_wr(0x41);
    panel_spi_data_wr(0x31);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x02);

    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x18);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0xBE);
    panel_spi_data_wr(0x4B);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x04);

    panel_spi_cmd_wr(0xD3);
    panel_spi_data_wr(0x34);
    panel_spi_data_wr(0x31);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x35);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);

    panel_spi_cmd_wr(0xB9);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF8);
    panel_spi_data_wr(0x53);
    panel_spi_data_wr(0x42);

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x02);

    panel_spi_cmd_wr(0xC6);
    panel_spi_data_wr(0x05);//1
    panel_spi_data_wr(0x10);//2
    panel_spi_data_wr(0x29);//3
    panel_spi_data_wr(0x00);//4

    panel_spi_cmd_wr(0xDE);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(150);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h048a02_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h048a02_timing = {
    .pixelclock = 36 * 1000 * 1000,
    .hactive = 480,
    .hfront_porch = 16,
    .hback_porch = 16,
    .hsync_len = 16,
    .vactive = 1120,
    .vfront_porch = 10,
    .vback_porch = 15,
    .vsync_len = 2,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_90,
    .data_order = RGB,
    .data_mirror = 0,
};

struct aic_panel rgb_h048a02 = {
    .name = "panel-h048a02",
    .timings = &h048a02_timing,
    .funcs = &h048a02_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

