/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <rtdevice.h>
#include <string.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drv_qspi.h>
#include <lvgl.h>
#include <stdbool.h>
#include <stdio.h>
#include "lv_tpc_run.h"

#define LV_DISP_MODE  0   // 0: single buf, 1: double buf
#define SHOW_SPI_WRITE_TIME 0

#define RST_PIN          "PB.7"
#define BL_PIN           "PD.8"
#define RS_PIN           "PD.9"

#define BL_ACTIVE_LEVEL  PIN_HIGH

#define MY_DISP_HOR_RES  240
#define MY_DISP_VER_RES  240

#define SPI_DRAW_BUF_LINE  10
#define SPI_DRAW_BUF_SIZE  MY_DISP_HOR_RES * SPI_DRAW_BUF_LINE

static rt_base_t rs_pin;
static rt_base_t bl_pin;
static rt_base_t rst_pin;

static struct rt_spi_device *spi_device = NULL;
static struct rt_spi_configuration spi_cfg = { 0 };
static volatile bool disp_flush_enabled = true;

#define SPI_DEV_NAME "spidev"
#define SPI_BUS_NAME "spi1"
#define SPI_MODE     (RT_SPI_MODE_0 | RT_SPI_MSB)
#define SPI_MAX_HZ   100000000

#if LV_DISP_MODE == 1
static int g_wait_last = 0;
#endif

static int screen_pin_init(void)
{
    // rs pin
    rs_pin = rt_pin_get(RS_PIN);
    if (rs_pin < 0) {
        LV_LOG_ERROR("get spi rs pin failed\n");
        return -1;
    }

    rt_pin_mode(rs_pin, PIN_MODE_OUTPUT);
    rt_pin_write(rs_pin, PIN_LOW);

    // bl pin
    bl_pin = rt_pin_get(BL_PIN);
    if (bl_pin < 0) {
        LV_LOG_ERROR("get spi bl pin failed\n");
        return -1;
    }

    rt_pin_mode(bl_pin, PIN_MODE_OUTPUT);
    rt_pin_write(bl_pin, BL_ACTIVE_LEVEL);

    return 0;
}

static int spi_dev_init(void)
{
    int result = 0;
    struct rt_device *dev = NULL;

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL) {
        LV_LOG_ERROR("rt malloc spi device failed.\n");
        return -RT_ERROR;
    }

    result = rt_spi_bus_attach_device(spi_device, SPI_DEV_NAME, SPI_BUS_NAME, NULL);
    if (result != RT_EOK && spi_device != NULL) {
        LV_LOG_ERROR("rt spi bus attach device failed.\n");
        result = -RT_ERROR;
        goto err;
    }

    spi_device = (struct rt_spi_device *)rt_device_find(SPI_DEV_NAME);
    if (!spi_device) {
        LV_LOG_ERROR("Failed to get device in %s\n", SPI_DEV_NAME);
        result = -RT_ERROR;
        goto err;
    }

    dev = (struct rt_device *)spi_device;
    if (dev->type != RT_Device_Class_SPIDevice) {
        spi_device = NULL;
        LV_LOG_ERROR("%s is not SPI device.\n", SPI_DEV_NAME);
        result = -RT_ERROR;
        goto err;
    }

    spi_cfg.mode = SPI_MODE;
    spi_cfg.max_hz = SPI_MAX_HZ;
    result = rt_spi_configure(spi_device, &spi_cfg);
    if (result < 0) {
        LV_LOG_ERROR("qspi configure failure.\n");
        result = -RT_ERROR;
        goto err;
    }

    return result;
err:
    return result;
}

static void spi_write_cmd(struct rt_spi_device *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    int ret;

    rt_spi_take_bus(spi_dev);

    rt_pin_write(rs_pin, PIN_LOW);
    ret = rt_spi_transfer(spi_dev, (u8[]){ cmd }, NULL, 1);
    if (ret != 1)
        LV_LOG_ERROR("Send spi data failed. ret 0x%x\n", (int)ret);

    rt_pin_write(rs_pin, PIN_HIGH);
    if (len != 0) {
        ret = rt_spi_transfer(spi_dev, (void *)data, NULL, len);
        if (ret != len)
            LV_LOG_ERROR("Send spi data failed. ret 0x%x\n", (int)ret);
    }

    rt_spi_release_bus(spi_dev);
}

#if LV_DISP_MODE == 1
static void spi_write_cmd_noblock(struct rt_spi_device *spi_dev, unsigned int cmd, unsigned int len, const u8 *data)
{
    int ret;

    rt_spi_take_bus(spi_dev);

    rt_pin_write(rs_pin, PIN_LOW);
    ret = rt_spi_transfer(spi_dev, (u8[]){ cmd }, NULL, 1);
    if (ret != 1)
        LV_LOG_ERROR("Send spi data failed. ret 0x%x\n", (int)ret);
    rt_pin_write(rs_pin, PIN_HIGH);
    if (len != 0) {
        ret = rt_spi_transfer(spi_dev, (void *)data, NULL, len);
        if (ret != len)
            LV_LOG_ERROR("Send spi data failed. ret 0x%x\n", (int)ret);
    }
}
#endif

#define spi_write_cmd_seq(dev, cmd, seq...)                 \
    do {                                                    \
        static const u8 d[] = { seq };                      \
        spi_write_cmd(dev, cmd, ARRAY_SIZE(d), d);    \
    } while(0);

void gc9a01_enable(struct rt_spi_device *dev)
{
    rst_pin = rt_pin_get(RST_PIN);
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);

    rt_pin_write(rst_pin, 1);
    rt_thread_mdelay(120);

    spi_write_cmd_seq(dev,0xFE);
    spi_write_cmd_seq(dev,0xEF);
    spi_write_cmd_seq(dev,0xEB, 0x14);
    spi_write_cmd_seq(dev,0x84, 0x40);
    spi_write_cmd_seq(dev,0x85, 0xF1);
    spi_write_cmd_seq(dev,0x86, 0x98);
    spi_write_cmd_seq(dev,0x87, 0x28);
    spi_write_cmd_seq(dev,0x88, 0x0A);
    spi_write_cmd_seq(dev,0x89, 0x21);
    spi_write_cmd_seq(dev,0x8A, 0x00);
    spi_write_cmd_seq(dev,0x8B, 0x80);
    spi_write_cmd_seq(dev,0x8C, 0x01);
    spi_write_cmd_seq(dev,0x8D, 0x00);
    spi_write_cmd_seq(dev,0x8E, 0xDF);
    spi_write_cmd_seq(dev,0x8F, 0x52);
    spi_write_cmd_seq(dev,0xB6, 0x20);
    spi_write_cmd_seq(dev,0x36, 0x48);
    spi_write_cmd_seq(dev,0x3A, 0x05); // 0x06
    spi_write_cmd_seq(dev,0x90, 0x08, 0x08, 0x08, 0x08);
    spi_write_cmd_seq(dev,0xBD, 0x06);
    spi_write_cmd_seq(dev,0xBF, 0x1C);
    spi_write_cmd_seq(dev,0xA7, 0x45);
    spi_write_cmd_seq(dev,0xA9, 0xBB);
    spi_write_cmd_seq(dev,0xB8, 0x63);
    spi_write_cmd_seq(dev,0xBC, 0x00);
    spi_write_cmd_seq(dev,0xFF, 0x60, 0x01, 0x04);
    spi_write_cmd_seq(dev,0xC3, 0x17);
    spi_write_cmd_seq(dev,0xC4, 0x17);
    spi_write_cmd_seq(dev,0xC9, 0x25);
    spi_write_cmd_seq(dev,0xBE, 0x11);
    spi_write_cmd_seq(dev,0xE1, 0x10, 0x0E);
    spi_write_cmd_seq(dev,0xDF, 0x21, 0x10, 0x02);
    spi_write_cmd_seq(dev,0xF0, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A);
    spi_write_cmd_seq(dev,0xF1, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F);
    spi_write_cmd_seq(dev,0xF2, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A);
    spi_write_cmd_seq(dev,0xF3, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F);
    spi_write_cmd_seq(dev,0xED, 0x1B, 0x0B);
    spi_write_cmd_seq(dev,0xAC, 0x47);
    spi_write_cmd_seq(dev,0xAE, 0x77);
    spi_write_cmd_seq(dev,0xCB, 0x02);
    spi_write_cmd_seq(dev,0xCD, 0x63);
    spi_write_cmd_seq(dev,0x70, 0x07, 0x09, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03);
    spi_write_cmd_seq(dev,0xE8, 0x34);
    spi_write_cmd_seq(dev,0x62, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70);
    spi_write_cmd_seq(dev,0x63, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70);
    spi_write_cmd_seq(dev,0x64, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07);
    spi_write_cmd_seq(dev,0x66, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00);
    spi_write_cmd_seq(dev,0x67, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98);
    spi_write_cmd_seq(dev,0x74, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00);
    spi_write_cmd_seq(dev,0x35);
    spi_write_cmd_seq(dev,0x21);

    rt_thread_mdelay(120);
    //--------end gamma setting--------------//
    spi_write_cmd_seq(dev,0x11);
    rt_thread_mdelay(120);

    spi_write_cmd_seq(dev,0x29);
    spi_write_cmd_seq(dev,0x2c);
}

void st77912_enable(struct rt_spi_device *dev)
{
    rst_pin = rt_pin_get(RST_PIN);
    rt_pin_mode(rst_pin, PIN_MODE_OUTPUT);

    rt_pin_write(rst_pin, 1);
    rt_thread_mdelay(120);

    spi_write_cmd_seq(dev, 0xF0, 0x01);
    spi_write_cmd_seq(dev, 0xF1, 0x01);
    spi_write_cmd_seq(dev, 0x7A, 0x83);
    spi_write_cmd_seq(dev, 0xB0, 0x5E);
    spi_write_cmd_seq(dev, 0xB1, 0x55);
    spi_write_cmd_seq(dev, 0xB2, 0x24);
    spi_write_cmd_seq(dev, 0xB4, 0xA7);
    spi_write_cmd_seq(dev, 0xB5, 0x54);
    spi_write_cmd_seq(dev, 0xB6, 0x8B);
    spi_write_cmd_seq(dev, 0xB7, 0x50);
    spi_write_cmd_seq(dev, 0xBA, 0x00);
    spi_write_cmd_seq(dev, 0xBB, 0x08);
    spi_write_cmd_seq(dev, 0xBC, 0x08);
    spi_write_cmd_seq(dev, 0xBD, 0x00);
    spi_write_cmd_seq(dev, 0xC0, 0x80);
    spi_write_cmd_seq(dev, 0xC1, 0x08);
    spi_write_cmd_seq(dev, 0xC2, 0x54);
    spi_write_cmd_seq(dev, 0xC3, 0x80);
    spi_write_cmd_seq(dev, 0xC4, 0x08);
    spi_write_cmd_seq(dev, 0xC5, 0x54);
    spi_write_cmd_seq(dev, 0xC6, 0xA9);
    spi_write_cmd_seq(dev, 0xC7, 0x41);
    spi_write_cmd_seq(dev, 0xC8, 0x51);
    spi_write_cmd_seq(dev, 0xC9, 0xA9);
    spi_write_cmd_seq(dev, 0xCA, 0x41);
    spi_write_cmd_seq(dev, 0xCB, 0x51);
    spi_write_cmd_seq(dev, 0xD0, 0x80);
    spi_write_cmd_seq(dev, 0xD1, 0xF0);
    spi_write_cmd_seq(dev, 0xD2, 0xF0);

    spi_write_cmd_seq(dev, 0xF5, 0x00, 0xA5);

    spi_write_cmd_seq(dev, 0xDD, 0x36);
    spi_write_cmd_seq(dev, 0xDE, 0x36);
    spi_write_cmd_seq(dev, 0xF0, 0x02);
    spi_write_cmd_seq(dev, 0xF1, 0x01);

    spi_write_cmd_seq(dev, 0xE0, 0xF0, 0x16, 0x1C, 0x0A, 0x0A, 0x06, 0x3E, 0x33, 0x53, 0x07, 0x14, 0x13, 0x31, 0x35);
    spi_write_cmd_seq(dev, 0xE1, 0xF0, 0x16, 0x1C, 0x0A, 0x0A, 0x06, 0x3E, 0x33, 0x53, 0x07, 0x14, 0x13, 0x31, 0x35);

    spi_write_cmd_seq(dev, 0xF0,  0x10);
    spi_write_cmd_seq(dev, 0xF3,  0x10);
    spi_write_cmd_seq(dev, 0xE0,  0x0B);
    spi_write_cmd_seq(dev, 0xE1,  0x00);
    spi_write_cmd_seq(dev, 0xE2,  0x00);
    spi_write_cmd_seq(dev, 0xE3,  0x00);
    spi_write_cmd_seq(dev, 0xE4,  0xE0);
    spi_write_cmd_seq(dev, 0xE5,  0x06);
    spi_write_cmd_seq(dev, 0xE6,  0x21);
    spi_write_cmd_seq(dev, 0xE7,  0x80);
    spi_write_cmd_seq(dev, 0xE8,  0x0A);
    spi_write_cmd_seq(dev, 0xE9,  0x00);
    spi_write_cmd_seq(dev, 0xEA,  0x04);
    spi_write_cmd_seq(dev, 0xEB,  0x00);
    spi_write_cmd_seq(dev, 0xEC,  0x00);
    spi_write_cmd_seq(dev, 0xED,  0x24);
    spi_write_cmd_seq(dev, 0xEE,  0x00);
    spi_write_cmd_seq(dev, 0xEF,  0x00);
    spi_write_cmd_seq(dev, 0xF8,  0xFF);
    spi_write_cmd_seq(dev, 0xF9,  0x00);
    spi_write_cmd_seq(dev, 0xFA,  0x00);
    spi_write_cmd_seq(dev, 0xFB,  0x30);
    spi_write_cmd_seq(dev, 0xFC,  0x00);
    spi_write_cmd_seq(dev, 0xFD,  0x00);
    spi_write_cmd_seq(dev, 0xFE,  0x00);
    spi_write_cmd_seq(dev, 0xFF,  0x00);
    spi_write_cmd_seq(dev, 0x60,  0x40);
    spi_write_cmd_seq(dev, 0x61,  0x08);
    spi_write_cmd_seq(dev, 0x62,  0x00);
    spi_write_cmd_seq(dev, 0x63,  0x41);
    spi_write_cmd_seq(dev, 0x64,  0xED);
    spi_write_cmd_seq(dev, 0x65,  0x00);
    spi_write_cmd_seq(dev, 0x66,  0x40);
    spi_write_cmd_seq(dev, 0x67,  0x00);
    spi_write_cmd_seq(dev, 0x68,  0x00);
    spi_write_cmd_seq(dev, 0x69,  0x40);
    spi_write_cmd_seq(dev, 0x6A,  0x00);
    spi_write_cmd_seq(dev, 0x6B,  0x00);
    spi_write_cmd_seq(dev, 0x70,  0x40);
    spi_write_cmd_seq(dev, 0x71,  0x07);
    spi_write_cmd_seq(dev, 0x72,  0x00);
    spi_write_cmd_seq(dev, 0x73,  0x41);
    spi_write_cmd_seq(dev, 0x74,  0xEC);
    spi_write_cmd_seq(dev, 0x75,  0x00);
    spi_write_cmd_seq(dev, 0x76,  0x40);
    spi_write_cmd_seq(dev, 0x77,  0x00);
    spi_write_cmd_seq(dev, 0x78,  0x00);
    spi_write_cmd_seq(dev, 0x79,  0x40);
    spi_write_cmd_seq(dev, 0x7A,  0x00);
    spi_write_cmd_seq(dev, 0x7B,  0x00);
    spi_write_cmd_seq(dev, 0x80,  0x48);
    spi_write_cmd_seq(dev, 0x81,  0x00);
    spi_write_cmd_seq(dev, 0x82,  0x0A);
    spi_write_cmd_seq(dev, 0x83,  0x01);
    spi_write_cmd_seq(dev, 0x84,  0xEA);
    spi_write_cmd_seq(dev, 0x85,  0x00);
    spi_write_cmd_seq(dev, 0x86,  0x00);
    spi_write_cmd_seq(dev, 0x87,  0x00);
    spi_write_cmd_seq(dev, 0x88,  0x48);
    spi_write_cmd_seq(dev, 0x89,  0x00);
    spi_write_cmd_seq(dev, 0x8A,  0x0C);
    spi_write_cmd_seq(dev, 0x8B,  0x01);
    spi_write_cmd_seq(dev, 0x8C,  0xEC);
    spi_write_cmd_seq(dev, 0x8D,  0x00);
    spi_write_cmd_seq(dev, 0x8E,  0x00);
    spi_write_cmd_seq(dev, 0x8F,  0x00);
    spi_write_cmd_seq(dev, 0x90,  0x48);
    spi_write_cmd_seq(dev, 0x91,  0x00);
    spi_write_cmd_seq(dev, 0x92,  0x0E);
    spi_write_cmd_seq(dev, 0x93,  0x01);
    spi_write_cmd_seq(dev, 0x94,  0xEE);
    spi_write_cmd_seq(dev, 0x95,  0x00);
    spi_write_cmd_seq(dev, 0x96,  0x00);
    spi_write_cmd_seq(dev, 0x97,  0x00);
    spi_write_cmd_seq(dev, 0x98,  0x48);
    spi_write_cmd_seq(dev, 0x99,  0x00);
    spi_write_cmd_seq(dev, 0x9A,  0x10);
    spi_write_cmd_seq(dev, 0x9B,  0x01);
    spi_write_cmd_seq(dev, 0x9C,  0xF0);
    spi_write_cmd_seq(dev, 0x9D,  0x00);
    spi_write_cmd_seq(dev, 0x9E,  0x00);
    spi_write_cmd_seq(dev, 0x9F,  0x00);
    spi_write_cmd_seq(dev, 0xA0,  0x48);
    spi_write_cmd_seq(dev, 0xA1,  0x00);
    spi_write_cmd_seq(dev, 0xA2,  0x09);
    spi_write_cmd_seq(dev, 0xA3,  0x01);
    spi_write_cmd_seq(dev, 0xA4,  0xE9);
    spi_write_cmd_seq(dev, 0xA5,  0x00);
    spi_write_cmd_seq(dev, 0xA6,  0x00);
    spi_write_cmd_seq(dev, 0xA7,  0x00);
    spi_write_cmd_seq(dev, 0xA8,  0x48);
    spi_write_cmd_seq(dev, 0xA9,  0x00);
    spi_write_cmd_seq(dev, 0xAA,  0x0B);
    spi_write_cmd_seq(dev, 0xAB,  0x01);
    spi_write_cmd_seq(dev, 0xAC,  0xEB);
    spi_write_cmd_seq(dev, 0xAD,  0x00);
    spi_write_cmd_seq(dev, 0xAE,  0x00);
    spi_write_cmd_seq(dev, 0xAF,  0x00);
    spi_write_cmd_seq(dev, 0xB0,  0x48);
    spi_write_cmd_seq(dev, 0xB1,  0x00);
    spi_write_cmd_seq(dev, 0xB2,  0x0D);
    spi_write_cmd_seq(dev, 0xB3,  0x01);
    spi_write_cmd_seq(dev, 0xB4,  0xED);
    spi_write_cmd_seq(dev, 0xB5,  0x00);
    spi_write_cmd_seq(dev, 0xB6,  0x00);
    spi_write_cmd_seq(dev, 0xB7,  0x00);
    spi_write_cmd_seq(dev, 0xB8,  0x48);
    spi_write_cmd_seq(dev, 0xB9,  0x00);
    spi_write_cmd_seq(dev, 0xBA,  0x0F);
    spi_write_cmd_seq(dev, 0xBB,  0x01);
    spi_write_cmd_seq(dev, 0xBC,  0xEF);
    spi_write_cmd_seq(dev, 0xBD,  0x00);
    spi_write_cmd_seq(dev, 0xBE,  0x00);
    spi_write_cmd_seq(dev, 0xBF,  0x00);
    spi_write_cmd_seq(dev, 0xC0,  0x88);
    spi_write_cmd_seq(dev, 0xC1,  0x99);
    spi_write_cmd_seq(dev, 0xC2,  0x01);
    spi_write_cmd_seq(dev, 0xC3,  0xAA);
    spi_write_cmd_seq(dev, 0xC4,  0xBB);
    spi_write_cmd_seq(dev, 0xC5,  0x74);
    spi_write_cmd_seq(dev, 0xC6,  0x65);
    spi_write_cmd_seq(dev, 0xC7,  0x56);
    spi_write_cmd_seq(dev, 0xC8,  0x47);
    spi_write_cmd_seq(dev, 0xC9,  0x10);
    spi_write_cmd_seq(dev, 0xD0,  0x88);
    spi_write_cmd_seq(dev, 0xD1,  0x99);
    spi_write_cmd_seq(dev, 0xD2,  0x01);
    spi_write_cmd_seq(dev, 0xD3,  0xAA);
    spi_write_cmd_seq(dev, 0xD4,  0xBB);
    spi_write_cmd_seq(dev, 0xD5,  0x74);
    spi_write_cmd_seq(dev, 0xD6,  0x65);
    spi_write_cmd_seq(dev, 0xD7,  0x56);
    spi_write_cmd_seq(dev, 0xD8,  0x47);
    spi_write_cmd_seq(dev, 0xD9,  0x10);

    spi_write_cmd_seq(dev, 0xF0, 0x08);
    spi_write_cmd_seq(dev, 0xF2, 0x08);
    spi_write_cmd_seq(dev, 0x71, 0x03);
    spi_write_cmd_seq(dev, 0x73, 0x30);
    spi_write_cmd_seq(dev, 0x76, 0x00);
    spi_write_cmd_seq(dev, 0x78, 0x33);
    spi_write_cmd_seq(dev, 0x79, 0x01);
    spi_write_cmd_seq(dev, 0x7B, 0xFA);
    spi_write_cmd_seq(dev, 0x7E, 0x16);
    spi_write_cmd_seq(dev, 0x86, 0x55);
    spi_write_cmd_seq(dev, 0x89, 0x61);
    spi_write_cmd_seq(dev, 0x8A, 0x00);
    spi_write_cmd_seq(dev, 0xF0, 0x01);
    spi_write_cmd_seq(dev, 0xF1, 0x01);
    spi_write_cmd_seq(dev, 0xA0, 0x0B);

    spi_write_cmd_seq(dev, 0xA3, 0x2A);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x2B);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x2C);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x2D);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x2E);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x2F);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x30);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x31);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x32);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA3, 0x33);
    spi_write_cmd_seq(dev, 0xA5, 0xC3);

    spi_write_cmd_seq(dev, 0x00, 0x1);

    spi_write_cmd_seq(dev, 0xA0, 0x09);
    spi_write_cmd_seq(dev, 0xF0, 0x00);
    spi_write_cmd_seq(dev, 0xF1, 0x10);
    spi_write_cmd_seq(dev, 0xF2, 0x84);
    spi_write_cmd_seq(dev, 0xF3, 0x01);

    spi_write_cmd_seq(dev, 0x3A, 0x05);

    spi_write_cmd_seq(dev, 0x21);
    spi_write_cmd_seq(dev, 0x11);

    rt_thread_mdelay(120);
    spi_write_cmd_seq(dev, 0x29);
    rt_thread_mdelay(20);
}

static void spi_draw_lcd(int x_start, int y_start, int x_end, int y_end, uint8_t *data)
{
    struct rt_spi_device *spi = spi_device;
    u8 cmd[4] = { 0 };
    int width = x_end - x_start + 1;
    int height = y_end - y_start + 1;
    int length = width * height * 2;  //RGB565
    uint16_t *cur_data = (uint16_t *)data;

#if LV_DISP_MODE == 1
    if (g_wait_last) {
        while (rt_spi_get_transfer_status(spi) != 0) {
        }
        rt_spi_release_bus(spi);
    }
    rt_spi_nonblock_set(spi, 0);
#endif

    //swap data
    for (int j = 0; j < width * height; j++) {
        uint16_t temp_data = cur_data[j];
        cur_data[j] = (temp_data >> 8) | ((temp_data & 0xFF) << 8);
    }

    // set x postion
    cmd[0] = x_start >> 8;
    cmd[1] = x_start & 0xFF;
    cmd[2] = (x_end) >> 8;
    cmd[3] = (x_end) & 0xFF;


    spi_write_cmd(spi, 0x2A, 4, cmd);

    // set y postion
    cmd[0] = y_start >> 8;
    cmd[1] = y_start & 0xFF;
    cmd[2] = (y_end) >> 8;
    cmd[3] = (y_end) & 0xFF;

    spi_write_cmd(spi, 0x2B, 4, cmd);
#if LV_DISP_MODE == 1
    rt_spi_nonblock_set(spi, 1);
    // set pixel data
    spi_write_cmd_noblock(spi, 0x2C, length, data);
    g_wait_last = 1;
#else

#if SHOW_SPI_WRITE_TIME == 1
    unsigned long start_us = aic_get_time_us();
#endif
    spi_write_cmd(spi, 0x2C, length, data);
#if SHOW_SPI_WRITE_TIME == 1
    printf("spi write time:%.2f\n", (240 * 240 * 2)/((float)(aic_get_time_us() - start_us) / 1000000.0f));
#endif

#endif

    return;
}

static void disp_init(void);

static lv_color16_t buf_1[ALIGN_UP(SPI_DRAW_BUF_SIZE, CACHE_LINE_SIZE)] __attribute__((aligned(CACHE_LINE_SIZE)));

#if LV_DISP_MODE == 1
static lv_color16_t buf_2[ALIGN_UP(SPI_DRAW_BUF_SIZE, CACHE_LINE_SIZE)] __attribute__((aligned(CACHE_LINE_SIZE)));
#endif

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t *px_map);

void lv_port_disp_init(void)
{
    disp_init();

    lv_display_t *disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
#if LV_DISP_MODE == 1
    lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
#else
    lv_display_set_buffers(disp, buf_1, NULL, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

#ifdef AIC_TOUCH_PANEL_NAME
    int result = tpc_run(AIC_TOUCH_PANEL_NAME, MY_DISP_HOR_RES, MY_DISP_VER_RES);
    if (result) {
        LV_LOG_INFO("can't find touch panel\n");
    }
    return;
#endif
}

static void disp_init(void)
{
    int y_start = 0;
    int y_end = 0;
    int line_step = 0;

    screen_pin_init();
    if (spi_dev_init() != 0) {
        printf("spi init error\n");
        return;
    }

    gc9a01_enable(spi_device);

    while (y_start < MY_DISP_VER_RES) {
        if (y_start + SPI_DRAW_BUF_LINE < MY_DISP_VER_RES) {
            line_step = SPI_DRAW_BUF_LINE;

        } else {
            line_step = MY_DISP_VER_RES - y_start;
        }

        y_end = y_start + line_step - 1;
        spi_draw_lcd(0, y_start, MY_DISP_HOR_RES - 1, y_end, (uint8_t *)buf_1);
        y_start += line_step;
    }
}

void disp_enable_update(void)
{
    disp_flush_enabled = true;
}
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    if (disp_flush_enabled) {
        spi_draw_lcd((int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2, px_map);
    }

    /*Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp_drv);
}
