/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui <huahui.mai@artinchip.com>
 */

#include <rtdevice.h>
#include "lv_aic_spi.h"

#define ST77912_RST_PIN  "PD.19"

void lv_spi_panel_enable(struct lv_spi_dev *dev)
{
    u32 rst_pin;

    rst_pin = rt_pin_get(ST77912_RST_PIN);
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);

    rt_pin_write(rst_pin, 1);
    rt_thread_mdelay(120);

    lv_spi_write_seq(dev, 0xF0, 0x01);
    lv_spi_write_seq(dev, 0xF1, 0x01);
    lv_spi_write_seq(dev, 0x7A, 0x83);
    lv_spi_write_seq(dev, 0xB0, 0x5E);
    lv_spi_write_seq(dev, 0xB1, 0x55);
    lv_spi_write_seq(dev, 0xB2, 0x24);
    lv_spi_write_seq(dev, 0xB4, 0xA7);
    lv_spi_write_seq(dev, 0xB5, 0x54);
    lv_spi_write_seq(dev, 0xB6, 0x8B);
    lv_spi_write_seq(dev, 0xB7, 0x50);
    lv_spi_write_seq(dev, 0xBA, 0x00);
    lv_spi_write_seq(dev, 0xBB, 0x08);
    lv_spi_write_seq(dev, 0xBC, 0x08);
    lv_spi_write_seq(dev, 0xBD, 0x00);
    lv_spi_write_seq(dev, 0xC0, 0x80);
    lv_spi_write_seq(dev, 0xC1, 0x08);
    lv_spi_write_seq(dev, 0xC2, 0x54);
    lv_spi_write_seq(dev, 0xC3, 0x80);
    lv_spi_write_seq(dev, 0xC4, 0x08);
    lv_spi_write_seq(dev, 0xC5, 0x54);
    lv_spi_write_seq(dev, 0xC6, 0xA9);
    lv_spi_write_seq(dev, 0xC7, 0x41);
    lv_spi_write_seq(dev, 0xC8, 0x51);
    lv_spi_write_seq(dev, 0xC9, 0xA9);
    lv_spi_write_seq(dev, 0xCA, 0x41);
    lv_spi_write_seq(dev, 0xCB, 0x51);
    lv_spi_write_seq(dev, 0xD0, 0x80);
    lv_spi_write_seq(dev, 0xD1, 0xF0);
    lv_spi_write_seq(dev, 0xD2, 0xF0);

    lv_spi_write_seq(dev, 0xF5, 0x00, 0xA5);

    lv_spi_write_seq(dev, 0xDD, 0x36);
    lv_spi_write_seq(dev, 0xDE, 0x36);
    lv_spi_write_seq(dev, 0xF0, 0x02);
    lv_spi_write_seq(dev, 0xF1, 0x01);

    lv_spi_write_seq(dev, 0xE0, 0xF0, 0x16, 0x1C, 0x0A, 0x0A, 0x06, 0x3E, 0x33, 0x53, 0x07, 0x14, 0x13, 0x31, 0x35);
    lv_spi_write_seq(dev, 0xE1, 0xF0, 0x16, 0x1C, 0x0A, 0x0A, 0x06, 0x3E, 0x33, 0x53, 0x07, 0x14, 0x13, 0x31, 0x35);

    lv_spi_write_seq(dev, 0xF0,  0x10);
    lv_spi_write_seq(dev, 0xF3,  0x10);
    lv_spi_write_seq(dev, 0xE0,  0x0B);
    lv_spi_write_seq(dev, 0xE1,  0x00);
    lv_spi_write_seq(dev, 0xE2,  0x00);
    lv_spi_write_seq(dev, 0xE3,  0x00);
    lv_spi_write_seq(dev, 0xE4,  0xE0);
    lv_spi_write_seq(dev, 0xE5,  0x06);
    lv_spi_write_seq(dev, 0xE6,  0x21);
    lv_spi_write_seq(dev, 0xE7,  0x80);
    lv_spi_write_seq(dev, 0xE8,  0x0A);
    lv_spi_write_seq(dev, 0xE9,  0x00);
    lv_spi_write_seq(dev, 0xEA,  0x04);
    lv_spi_write_seq(dev, 0xEB,  0x00);
    lv_spi_write_seq(dev, 0xEC,  0x00);
    lv_spi_write_seq(dev, 0xED,  0x24);
    lv_spi_write_seq(dev, 0xEE,  0x00);
    lv_spi_write_seq(dev, 0xEF,  0x00);
    lv_spi_write_seq(dev, 0xF8,  0xFF);
    lv_spi_write_seq(dev, 0xF9,  0x00);
    lv_spi_write_seq(dev, 0xFA,  0x00);
    lv_spi_write_seq(dev, 0xFB,  0x30);
    lv_spi_write_seq(dev, 0xFC,  0x00);
    lv_spi_write_seq(dev, 0xFD,  0x00);
    lv_spi_write_seq(dev, 0xFE,  0x00);
    lv_spi_write_seq(dev, 0xFF,  0x00);
    lv_spi_write_seq(dev, 0x60,  0x40);
    lv_spi_write_seq(dev, 0x61,  0x08);
    lv_spi_write_seq(dev, 0x62,  0x00);
    lv_spi_write_seq(dev, 0x63,  0x41);
    lv_spi_write_seq(dev, 0x64,  0xED);
    lv_spi_write_seq(dev, 0x65,  0x00);
    lv_spi_write_seq(dev, 0x66,  0x40);
    lv_spi_write_seq(dev, 0x67,  0x00);
    lv_spi_write_seq(dev, 0x68,  0x00);
    lv_spi_write_seq(dev, 0x69,  0x40);
    lv_spi_write_seq(dev, 0x6A,  0x00);
    lv_spi_write_seq(dev, 0x6B,  0x00);
    lv_spi_write_seq(dev, 0x70,  0x40);
    lv_spi_write_seq(dev, 0x71,  0x07);
    lv_spi_write_seq(dev, 0x72,  0x00);
    lv_spi_write_seq(dev, 0x73,  0x41);
    lv_spi_write_seq(dev, 0x74,  0xEC);
    lv_spi_write_seq(dev, 0x75,  0x00);
    lv_spi_write_seq(dev, 0x76,  0x40);
    lv_spi_write_seq(dev, 0x77,  0x00);
    lv_spi_write_seq(dev, 0x78,  0x00);
    lv_spi_write_seq(dev, 0x79,  0x40);
    lv_spi_write_seq(dev, 0x7A,  0x00);
    lv_spi_write_seq(dev, 0x7B,  0x00);
    lv_spi_write_seq(dev, 0x80,  0x48);
    lv_spi_write_seq(dev, 0x81,  0x00);
    lv_spi_write_seq(dev, 0x82,  0x0A);
    lv_spi_write_seq(dev, 0x83,  0x01);
    lv_spi_write_seq(dev, 0x84,  0xEA);
    lv_spi_write_seq(dev, 0x85,  0x00);
    lv_spi_write_seq(dev, 0x86,  0x00);
    lv_spi_write_seq(dev, 0x87,  0x00);
    lv_spi_write_seq(dev, 0x88,  0x48);
    lv_spi_write_seq(dev, 0x89,  0x00);
    lv_spi_write_seq(dev, 0x8A,  0x0C);
    lv_spi_write_seq(dev, 0x8B,  0x01);
    lv_spi_write_seq(dev, 0x8C,  0xEC);
    lv_spi_write_seq(dev, 0x8D,  0x00);
    lv_spi_write_seq(dev, 0x8E,  0x00);
    lv_spi_write_seq(dev, 0x8F,  0x00);
    lv_spi_write_seq(dev, 0x90,  0x48);
    lv_spi_write_seq(dev, 0x91,  0x00);
    lv_spi_write_seq(dev, 0x92,  0x0E);
    lv_spi_write_seq(dev, 0x93,  0x01);
    lv_spi_write_seq(dev, 0x94,  0xEE);
    lv_spi_write_seq(dev, 0x95,  0x00);
    lv_spi_write_seq(dev, 0x96,  0x00);
    lv_spi_write_seq(dev, 0x97,  0x00);
    lv_spi_write_seq(dev, 0x98,  0x48);
    lv_spi_write_seq(dev, 0x99,  0x00);
    lv_spi_write_seq(dev, 0x9A,  0x10);
    lv_spi_write_seq(dev, 0x9B,  0x01);
    lv_spi_write_seq(dev, 0x9C,  0xF0);
    lv_spi_write_seq(dev, 0x9D,  0x00);
    lv_spi_write_seq(dev, 0x9E,  0x00);
    lv_spi_write_seq(dev, 0x9F,  0x00);
    lv_spi_write_seq(dev, 0xA0,  0x48);
    lv_spi_write_seq(dev, 0xA1,  0x00);
    lv_spi_write_seq(dev, 0xA2,  0x09);
    lv_spi_write_seq(dev, 0xA3,  0x01);
    lv_spi_write_seq(dev, 0xA4,  0xE9);
    lv_spi_write_seq(dev, 0xA5,  0x00);
    lv_spi_write_seq(dev, 0xA6,  0x00);
    lv_spi_write_seq(dev, 0xA7,  0x00);
    lv_spi_write_seq(dev, 0xA8,  0x48);
    lv_spi_write_seq(dev, 0xA9,  0x00);
    lv_spi_write_seq(dev, 0xAA,  0x0B);
    lv_spi_write_seq(dev, 0xAB,  0x01);
    lv_spi_write_seq(dev, 0xAC,  0xEB);
    lv_spi_write_seq(dev, 0xAD,  0x00);
    lv_spi_write_seq(dev, 0xAE,  0x00);
    lv_spi_write_seq(dev, 0xAF,  0x00);
    lv_spi_write_seq(dev, 0xB0,  0x48);
    lv_spi_write_seq(dev, 0xB1,  0x00);
    lv_spi_write_seq(dev, 0xB2,  0x0D);
    lv_spi_write_seq(dev, 0xB3,  0x01);
    lv_spi_write_seq(dev, 0xB4,  0xED);
    lv_spi_write_seq(dev, 0xB5,  0x00);
    lv_spi_write_seq(dev, 0xB6,  0x00);
    lv_spi_write_seq(dev, 0xB7,  0x00);
    lv_spi_write_seq(dev, 0xB8,  0x48);
    lv_spi_write_seq(dev, 0xB9,  0x00);
    lv_spi_write_seq(dev, 0xBA,  0x0F);
    lv_spi_write_seq(dev, 0xBB,  0x01);
    lv_spi_write_seq(dev, 0xBC,  0xEF);
    lv_spi_write_seq(dev, 0xBD,  0x00);
    lv_spi_write_seq(dev, 0xBE,  0x00);
    lv_spi_write_seq(dev, 0xBF,  0x00);
    lv_spi_write_seq(dev, 0xC0,  0x88);
    lv_spi_write_seq(dev, 0xC1,  0x99);
    lv_spi_write_seq(dev, 0xC2,  0x01);
    lv_spi_write_seq(dev, 0xC3,  0xAA);
    lv_spi_write_seq(dev, 0xC4,  0xBB);
    lv_spi_write_seq(dev, 0xC5,  0x74);
    lv_spi_write_seq(dev, 0xC6,  0x65);
    lv_spi_write_seq(dev, 0xC7,  0x56);
    lv_spi_write_seq(dev, 0xC8,  0x47);
    lv_spi_write_seq(dev, 0xC9,  0x10);
    lv_spi_write_seq(dev, 0xD0,  0x88);
    lv_spi_write_seq(dev, 0xD1,  0x99);
    lv_spi_write_seq(dev, 0xD2,  0x01);
    lv_spi_write_seq(dev, 0xD3,  0xAA);
    lv_spi_write_seq(dev, 0xD4,  0xBB);
    lv_spi_write_seq(dev, 0xD5,  0x74);
    lv_spi_write_seq(dev, 0xD6,  0x65);
    lv_spi_write_seq(dev, 0xD7,  0x56);
    lv_spi_write_seq(dev, 0xD8,  0x47);
    lv_spi_write_seq(dev, 0xD9,  0x10);

    lv_spi_write_seq(dev, 0xF0, 0x08);
    lv_spi_write_seq(dev, 0xF2, 0x08);
    lv_spi_write_seq(dev, 0x71, 0x03);
    lv_spi_write_seq(dev, 0x73, 0x30);
    lv_spi_write_seq(dev, 0x76, 0x00);
    lv_spi_write_seq(dev, 0x78, 0x33);
    lv_spi_write_seq(dev, 0x79, 0x01);
    lv_spi_write_seq(dev, 0x7B, 0xFA);
    lv_spi_write_seq(dev, 0x7E, 0x16);
    lv_spi_write_seq(dev, 0x86, 0x55);
    lv_spi_write_seq(dev, 0x89, 0x61);
    lv_spi_write_seq(dev, 0x8A, 0x00);
    lv_spi_write_seq(dev, 0xF0, 0x01);
    lv_spi_write_seq(dev, 0xF1, 0x01);
    lv_spi_write_seq(dev, 0xA0, 0x0B);

    lv_spi_write_seq(dev, 0xA3, 0x2A);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x2B);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x2C);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x2D);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x2E);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x2F);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x30);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x31);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x32);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA3, 0x33);
    lv_spi_write_seq(dev, 0xA5, 0xC3);

    lv_spi_write_seq(dev, 0x00, 0x1);

    lv_spi_write_seq(dev, 0xA0, 0x09);
    lv_spi_write_seq(dev, 0xF0, 0x00);
    lv_spi_write_seq(dev, 0xF1, 0x10);
    lv_spi_write_seq(dev, 0xF2, 0x84);
    lv_spi_write_seq(dev, 0xF3, 0x01);

    lv_spi_write_seq(dev, 0x3A, 0x05);

    lv_spi_write_seq(dev, 0x21);
    lv_spi_write_seq(dev, 0x11);

    rt_thread_mdelay(120);
    lv_spi_write_seq(dev, 0x29);
    rt_thread_mdelay(20);
}
