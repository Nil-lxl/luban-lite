/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui <huahui.mai@artinchip.com>
 */

#include <rtdevice.h>
#include "lv_aic_spi.h"

#define ST7789_RESET_PIN "PB.10"

void lv_spi_panel_enable(struct lv_spi_dev *dev)
{
    u32 rst_pin;

    rst_pin = rt_pin_get(ST7789_RESET_PIN);
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);

    rt_pin_write(rst_pin, 1);
    rt_thread_mdelay(120);

    lv_spi_write_seq(dev, 0x11);
    rt_thread_mdelay(120);

    lv_spi_write_seq(dev, 0x36, 0x00);
    lv_spi_write_seq(dev, 0x3a, 0x05);
    lv_spi_write_seq(dev, 0x21);
    lv_spi_write_seq(dev, 0xB2, 0x05, 0x05, 0x00, 0x33, 0x33);
    lv_spi_write_seq(dev, 0xB7, 0x74);
    lv_spi_write_seq(dev, 0xBB, 0x25);
    lv_spi_write_seq(dev, 0xC0, 0x2C);
    lv_spi_write_seq(dev, 0xC2, 0x01);
    lv_spi_write_seq(dev, 0xC3, 0x13);
    lv_spi_write_seq(dev, 0xC4, 0x20);
    lv_spi_write_seq(dev, 0xC6, 0x0F);
    lv_spi_write_seq(dev, 0xD0, 0xA4, 0xA1);
    lv_spi_write_seq(dev, 0xD6, 0xA1);
    lv_spi_write_seq(dev, 0xE0, 0xD0, 0x08, 0x0D, 0x0C, 0x0B, 0x26, 0x30, 0x33, 0x47, 0x36, 0x14,
                          0x14, 0x2A, 0x2E);
    lv_spi_write_seq(dev, 0xE1, 0xD0, 0x05, 0x0A, 0x09, 0x08, 0x04, 0x2E, 0x44, 0x45, 0x39, 0x15,
                          0x16, 0x2C, 0x2F);
    lv_spi_write_seq(dev, 0x2A,0x00, 0x34, 0x00, 0xBB);
    lv_spi_write_seq(dev, 0x2B,0x00, 0x28, 0x01, 0x17);

    lv_spi_write_seq(dev, 0x29);
    rt_thread_mdelay(10);
}
