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

    panel_spi_cmd_wr(0xD7);

    panel_spi_data_wr(0x61);

    panel_spi_data_wr(0x74);

    panel_spi_data_wr(0x97);


    panel_spi_cmd_wr(0xD0);

    panel_spi_data_wr(0x10);

    //page0

    panel_spi_cmd_wr(0x60);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x61);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x62);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x63);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x64);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x65);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x66);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x67);

    panel_spi_data_wr(0xff);


    panel_spi_cmd_wr(0x68);

    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x69);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6A);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6B);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6C);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6D);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6E);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6F);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0xA9);
    panel_spi_data_wr(0x03);

    panel_spi_cmd_wr(0xA8);
    panel_spi_data_wr(0x09);

    panel_spi_cmd_wr(0xF5);
    panel_spi_data_wr(0xB6);

    panel_spi_cmd_wr(0xFD);
    panel_spi_data_wr(0x6D);

    panel_spi_cmd_wr(0x9D);
    panel_spi_data_wr(0xA6);

    panel_spi_cmd_wr(0x9F);
    panel_spi_data_wr(0x0C);

    panel_spi_cmd_wr(0xA5);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xA6);
    panel_spi_data_wr(0xC0);
    panel_spi_data_wr(0x80);

    panel_spi_cmd_wr(0x7A);
    panel_spi_data_wr(0x25);

    panel_spi_cmd_wr(0x7B);
    panel_spi_data_wr(0x76);

    panel_spi_cmd_wr(0x88);
    panel_spi_data_wr(0x32);

    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0x04);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xB6);
    panel_spi_data_wr(0x22);//Forward:0x22 Backward:0x11
    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x30);

    panel_spi_cmd_wr(0x92);
    panel_spi_data_wr(0x2C);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x58);
    panel_spi_data_wr(0x50);

    panel_spi_cmd_wr(0xB5);
    panel_spi_data_wr(0x80);

    panel_spi_cmd_wr(0xAD);
    panel_spi_data_wr(0x88);

    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x11);

    panel_spi_cmd_wr(0x60);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x61);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x62);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x63);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x64);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x65);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x66);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x67);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x68);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x69);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6A);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6B);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6C);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0x6D);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0xB8);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0xB9);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0xBA);
    panel_spi_data_wr(0xff);

    panel_spi_cmd_wr(0xB3);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x34);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x0A);
    panel_spi_data_wr(0x08);
    panel_spi_data_wr(0x0E);
    panel_spi_data_wr(0x0C);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3E);
    panel_spi_data_wr(0x36);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x34);
    panel_spi_data_wr(0x37);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x28);
    panel_spi_data_wr(0x0B);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x0F);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);
    panel_spi_data_wr(0x3F);

    panel_spi_cmd_wr(0xB7);
    panel_spi_data_wr(0x07);

    panel_spi_cmd_wr(0xBC);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x08);

    panel_spi_cmd_wr(0xA0);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x4F);

    panel_spi_cmd_wr(0xA1);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x30);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x4F);

    panel_spi_cmd_wr(0xA3);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x0D);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x4D);

    panel_spi_cmd_wr(0xA7);
    panel_spi_data_wr(0x33);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x4D);
    panel_spi_data_wr(0x4D);

    panel_spi_cmd_wr(0xA8);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x09);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xA4);
    panel_spi_data_wr(0x59);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x1E);
    panel_spi_data_wr(0x0a);
    panel_spi_data_wr(0x4D);

    panel_spi_cmd_wr(0xC3);
    panel_spi_data_wr(0x1A);

    panel_spi_cmd_wr(0xE5);
    panel_spi_data_wr(0x05);
    panel_spi_data_wr(0x0A);

    panel_spi_cmd_wr(0xF8);
    panel_spi_data_wr(0x29);

    panel_spi_cmd_wr(0xAC);
    panel_spi_data_wr(0xA0);

    panel_spi_cmd_wr(0xF4);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xF5);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xF6);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0xF7);
    panel_spi_data_wr(0x00);

    panel_spi_cmd_wr(0xE4);
    panel_spi_data_wr(0xA6);

    panel_spi_cmd_wr(0xD0);
    panel_spi_data_wr(0x12);

    panel_spi_cmd_wr(0x80);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF9);

    panel_spi_cmd_wr(0x81);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x82);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB1);

    panel_spi_cmd_wr(0x83);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xBC);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x84);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF9);

    panel_spi_cmd_wr(0x85);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x86);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB1);

    panel_spi_cmd_wr(0x87);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xBC);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x88);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF9);

    panel_spi_cmd_wr(0x89);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x8A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB1);

    panel_spi_cmd_wr(0x8B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xBC);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x90);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF9);

    panel_spi_cmd_wr(0x91);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x92);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB1);

    panel_spi_cmd_wr(0x93);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xBC);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x94);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF9);

    panel_spi_cmd_wr(0x95);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x96);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB1);

    panel_spi_cmd_wr(0x97);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xBC);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x98);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x29);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x35);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x6F);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0x95);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xB4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xCF);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xE4);
    panel_spi_data_wr(0x00);
    panel_spi_data_wr(0xF9);

    panel_spi_cmd_wr(0x99);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x11);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x4F);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0x82);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xC2);
    panel_spi_data_wr(0x01);
    panel_spi_data_wr(0xF3);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x39);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x76);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0x77);

    panel_spi_cmd_wr(0x9A);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xB2);
    panel_spi_data_wr(0x02);
    panel_spi_data_wr(0xF5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x21);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x56);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x74);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0x9A);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xA5);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xB1);

    panel_spi_cmd_wr(0x9B);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xBC);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xC6);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xD4);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE0);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xE7);
    panel_spi_data_wr(0x03);
    panel_spi_data_wr(0xFF);

    panel_spi_cmd_wr(0x36);
    panel_spi_data_wr(0x08);

    panel_spi_cmd_wr(0x11);
    aic_delay_ms(200);

    panel_spi_cmd_wr(0x29);
    aic_delay_ms(100);

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs h068b06_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing h068b06_timing = {
    .pixelclock = 63 * 1000 * 1000,
    .hactive = 600,
    .hfront_porch = 80,
    .hback_porch = 80,
    .hsync_len = 8,
    .vactive = 1280,
    .vfront_porch = 40,
    .vback_porch = 40,
    .vsync_len = 4,
};

static struct panel_rgb rgb = {
    .mode = PRGB,
    .format = PRGB_24BIT,
    .clock_phase = DEGREE_0,
    .data_order = BGR,
    .data_mirror = 0,
};

struct aic_panel rgb_h068b06 = {
    .name = "panel-h068b06",
    .timings = &h068b06_timing,
    .funcs = &h068b06_funcs,
    .rgb = &rgb,
    .connector_type = AIC_RGB_COM,
};

