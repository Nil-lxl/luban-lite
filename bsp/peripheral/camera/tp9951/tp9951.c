/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define LOG_TAG     "tp9951"

#include <string.h>
#include <getopt.h>

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

#define DRV_NAME            LOG_TAG

#define DEFAULT_FORMAT      PAL
#define DEFAULT_MEDIA_CODE  MEDIA_BUS_FMT_UYVY8_2X8
#define DEFAULT_BUS_TYPE    MEDIA_BUS_BT656
#define DEFAULT_WIDTH       PAL_WIDTH
#define DEFAULT_HEIGHT      PAL_HEIGHT
#define DEFAULT_IS_HDA      0  /* 0, CVBS; 1, HDA */
#define DEFAULT_IS_INTERLACED
#define DEFAULT_VIN_CH      VIN4

#define TP9951_I2C_SLAVE_ID 0x44
#define TP9951_CHIP_ID      0x2860

enum tp_vin_ch {
    VIN1 = 0,
    VIN2 = 1,
    VIN3 = 2,
    VIN4 = 3,
};

enum tp_std {
    STD_TVI,
    STD_HDA, //AHD
};

enum tp_fmt {
    PAL,
    NTSC,
    HD25,
    HD30,
    HD275,  //720p27.5
    FHD25,
    FHD30,
    FHD275, //1080p27.5
    FHD28,
    UVGA25,  //1280x960p25
    UVGA30,  //1280x960p30
    FHD_X3C,
    F_UVGA30,  //FH 1280x960p30, 1800x1000
    HD50,
    HD60,
    A_UVGA30,  //HDA 1280x960p30
};

struct tp9951_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 pwdn_pin;

    struct mpp_video_fmt fmt;

    bool on;
    bool streaming;
};

static struct tp9951_dev g_tp_dev = {0};

static int tp9951_write_reg(u8 reg, u8 val)
{
    if (rt_i2c_write_reg(g_tp_dev.i2c, TP9951_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }
    return 0;
}

unsigned char tp9951_read_reg(u8 reg)
{
    unsigned char val = 0xFF;

    if (rt_i2c_read_reg(g_tp_dev.i2c, TP9951_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return 0xFF;
    }

    return val;
}

void TP9951_dvp_out(unsigned char fmt, unsigned char std)
{
    u8 tmp = 0;

    //mipi setting
    tp9951_write_reg(0x40, 0x08); //select mipi page

    if (FHD30 == fmt || FHD25 == fmt || FHD275 == fmt || FHD28 == fmt || FHD_X3C == fmt || HD50 == fmt  || HD60 == fmt)
    {
        tp9951_write_reg(0x12, 0x54);
        tp9951_write_reg(0x13, 0xef);
        tp9951_write_reg(0x14, 0x41);
        tp9951_write_reg(0x15, 0x02);
        tp9951_write_reg(0x40, 0x00);
    }
    else if (HD30 == fmt || HD25 == fmt || HD275 == fmt)
    {
        tp9951_write_reg(0x12, 0x54);
        tp9951_write_reg(0x13, 0xef);
        tp9951_write_reg(0x14, 0x41);
        tp9951_write_reg(0x15, 0x12);
        tp9951_write_reg(0x40, 0x00);
    }
    else if (NTSC == fmt || PAL == fmt)
    {
        tp9951_write_reg(0x12, 0x54);
        tp9951_write_reg(0x13, 0xef);
        tp9951_write_reg(0x14, 0x41);
        tp9951_write_reg(0x15, 0x12);
        tp9951_write_reg(0x40, 0x00);

    }
    else if (UVGA25 == fmt || UVGA30 == fmt)
    {
        tp9951_write_reg(0x13, 0x0f);
        tp9951_write_reg(0x12, 0x5f);

        tp9951_write_reg(0x14, 0x41);
        tp9951_write_reg(0x15, 0x02);
        tp9951_write_reg(0x40, 0x00);
    }
    else if (F_UVGA30 == fmt)
    {
        tp9951_write_reg(0x13, 0x0f);
        tp9951_write_reg(0x12, 0x5e);

        tp9951_write_reg(0x14, 0x41);
        tp9951_write_reg(0x15, 0x02);
        tp9951_write_reg(0x40, 0x00);
    }
    else if (A_UVGA30 == fmt)
    {
        tp9951_write_reg(0x13, 0x0f);
        tp9951_write_reg(0x12, 0x5a);
        tp9951_write_reg(0x14, 0x41);
        tp9951_write_reg(0x15, 0x02);
    }

    tp9951_write_reg(0x40, 0x00); //back to decoder page
    tmp = tp9951_read_reg(0x06); //PLL reset
    tp9951_write_reg(0x06, 0x80 | tmp);

    tp9951_write_reg(0x40, 0x08); //back to mipi page

    tmp = tp9951_read_reg(0x14); //PLL reset
    tp9951_write_reg(0x14, 0x80 | tmp);
    tp9951_write_reg(0x14, tmp);

    tp9951_write_reg(0x40, 0x00); //back to decoder page
}
/////////////////////////////////
//ch: video channel
//fmt: PAL/NTSC/HD25/HD30
//std: STD_TVI/STD_HDA
//sample: TP9951_sensor_init(VIN1,HD30,STD_TVI); //video is TVI 720p30 from Vin1
////////////////////////////////
void TP9951_sensor_init(unsigned char ch, unsigned char fmt, unsigned char std)
{

    tp9951_write_reg(0x40, 0x00); //select decoder page
    tp9951_write_reg(0x06, 0x12); //default value
    tp9951_write_reg(0x42, 0x00);   //common setting for all format
    tp9951_write_reg(0x4c, 0x43);   //common setting for all format
    tp9951_write_reg(0x4e, 0x1d); //common setting for dvp output
    tp9951_write_reg(0x54, 0x04); //common setting for dvp output

    tp9951_write_reg(0xf6, 0x00);   //common setting for all format
    tp9951_write_reg(0xf7, 0x44); //common setting for dvp output
    if (PAL == fmt || NTSC == fmt)
        tp9951_write_reg(0xfa, 0x04); //common setting for dvp output   CVBS clock/2
    else
        tp9951_write_reg(0xfa, 0x00); //common setting for dvp output
    tp9951_write_reg(0x1b, 0x01); //common setting for dvp output
    tp9951_write_reg(0x41, ch);     //video MUX select

    tp9951_write_reg(0x40, 0x08);   //common setting for all format
    tp9951_write_reg(0x13, 0xef); //common setting for dvp output
    tp9951_write_reg(0x14, 0x41); //common setting for dvp output
    tp9951_write_reg(0x15, 0x02); //common setting for dvp output

    tp9951_write_reg(0x40, 0x00); //select decoder page

    TP9951_dvp_out(fmt, std);

    if (PAL == fmt)
    {
#if CVBS_960H
        tp9951_write_reg(0x02, 0xcf);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x51);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x76);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x17);
        tp9951_write_reg(0x19, 0x20);
        tp9951_write_reg(0x1a, 0x17);
        tp9951_write_reg(0x1c, 0x09);
        tp9951_write_reg(0x1d, 0x48);

        tp9951_write_reg(0x20, 0x48);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x37);
        tp9951_write_reg(0x23, 0x3f);

        tp9951_write_reg(0x2b, 0x70);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x64);
        tp9951_write_reg(0x2e, 0x56);

        tp9951_write_reg(0x30, 0x7a);
        tp9951_write_reg(0x31, 0x4a);
        tp9951_write_reg(0x32, 0x4d);
        tp9951_write_reg(0x33, 0xf0);

        tp9951_write_reg(0x35, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x04);

#else //PAL 720H

        tp9951_write_reg(0x02, 0xcf);
        tp9951_write_reg(0x06, 0x32);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x51);

        tp9951_write_reg(0x15, 0x03);
        tp9951_write_reg(0x16, 0xf0);
        tp9951_write_reg(0x17, 0xa0);
        tp9951_write_reg(0x18, 0x17);
        tp9951_write_reg(0x19, 0x20);
        tp9951_write_reg(0x1a, 0x15);
        tp9951_write_reg(0x1c, 0x06);
        tp9951_write_reg(0x1d, 0xc0);

        tp9951_write_reg(0x20, 0x48);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x37);
        tp9951_write_reg(0x23, 0x3f);

        tp9951_write_reg(0x2b, 0x70);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x4b);
        tp9951_write_reg(0x2e, 0x56);

        tp9951_write_reg(0x30, 0x7a);
        tp9951_write_reg(0x31, 0x4a);
        tp9951_write_reg(0x32, 0x4d);
        tp9951_write_reg(0x33, 0xfb);

        tp9951_write_reg(0x35, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x04);
#endif
    }
    else if (NTSC == fmt)
    {
#if CVBS_960H
        tp9951_write_reg(0x02, 0xcf);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x60);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x12);
        tp9951_write_reg(0x19, 0xf0);
        tp9951_write_reg(0x1a, 0x07);
        tp9951_write_reg(0x1c, 0x09);
        tp9951_write_reg(0x1d, 0x38);

        tp9951_write_reg(0x20, 0x40);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x70);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x68);
        tp9951_write_reg(0x2e, 0x57);

        tp9951_write_reg(0x30, 0x62);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x96);
        tp9951_write_reg(0x33, 0xc0);

        tp9951_write_reg(0x35, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x04);

#else   //NTSC 720H

        tp9951_write_reg(0x02, 0xcf);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x03);
        tp9951_write_reg(0x16, 0xd6);
        tp9951_write_reg(0x17, 0xa0);
        tp9951_write_reg(0x18, 0x12);
        tp9951_write_reg(0x19, 0xf0);
        tp9951_write_reg(0x1a, 0x05);
        tp9951_write_reg(0x1c, 0x06);
        tp9951_write_reg(0x1d, 0xb4);

        tp9951_write_reg(0x20, 0x40);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x70);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x4b);
        tp9951_write_reg(0x2e, 0x57);

        tp9951_write_reg(0x30, 0x62);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x96);
        tp9951_write_reg(0x33, 0xcb);

        tp9951_write_reg(0x35, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x04);
#endif
    }
    else if (HD25 == fmt)
    {
        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x15);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x19);
        tp9951_write_reg(0x19, 0xd0);
        tp9951_write_reg(0x1a, 0x25);
        tp9951_write_reg(0x1c, 0x07);  //1280*720, 25fps
        tp9951_write_reg(0x1d, 0xbc);  //1280*720, 25fps

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x25);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x18);

        if (STD_HDA == std) //AHD720p25 extra
        {
            tp9951_write_reg(0x02, 0xce);

            tp9951_write_reg(0x0d, 0x71);

            tp9951_write_reg(0x18, 0x1b);

            tp9951_write_reg(0x20, 0x40);
            tp9951_write_reg(0x21, 0x46);

            tp9951_write_reg(0x25, 0xfe);
            tp9951_write_reg(0x26, 0x01);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x5a);
            tp9951_write_reg(0x2e, 0x40);

            tp9951_write_reg(0x30, 0x9e);
            tp9951_write_reg(0x31, 0x20);
            tp9951_write_reg(0x32, 0x10);
            tp9951_write_reg(0x33, 0x90);
        }
    }
    else if (HD30 == fmt)
    {
        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x15);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x19);
        tp9951_write_reg(0x19, 0xd0);
        tp9951_write_reg(0x1a, 0x25);
        tp9951_write_reg(0x1c, 0x06);  //1280*720, 30fps
        tp9951_write_reg(0x1d, 0x72);  //1280*720, 30fps

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x25);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x18);

        if (STD_HDA == std) //AHD720p30 extra
        {
            tp9951_write_reg(0x02, 0xce);

            tp9951_write_reg(0x0d, 0x70);

            tp9951_write_reg(0x18, 0x1b);

            tp9951_write_reg(0x20, 0x40);
            tp9951_write_reg(0x21, 0x46);

            tp9951_write_reg(0x25, 0xfe);
            tp9951_write_reg(0x26, 0x01);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x5a);
            tp9951_write_reg(0x2e, 0x40);

            tp9951_write_reg(0x30, 0x9d);
            tp9951_write_reg(0x31, 0xca);
            tp9951_write_reg(0x32, 0x01);
            tp9951_write_reg(0x33, 0xd0);
        }
    }
    else if (HD275 == fmt)      //720P27.5
    {
        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x15);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x19);
        tp9951_write_reg(0x19, 0xd0);
        tp9951_write_reg(0x1a, 0x25);
        tp9951_write_reg(0x1c, 0x07);  //1280*720, 27.5fps
        tp9951_write_reg(0x1d, 0x08);  //1280*720, 27.5fps

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x25);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x18);

        if (STD_HDA == std) //AHD720p30 extra
        {

        }
    }
    else if (FHD30 == fmt)
    {
        tp9951_write_reg(0x02, 0xc8);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x03);
        tp9951_write_reg(0x16, 0xd2);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x29);
        tp9951_write_reg(0x19, 0x38);
        tp9951_write_reg(0x1a, 0x47);
        tp9951_write_reg(0x1c, 0x08);  //1920*1080, 30fps
        tp9951_write_reg(0x1d, 0x98);  //

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x05);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1C);

        if (STD_HDA == std) //AHD1080p30 extra
        {
            tp9951_write_reg(0x02, 0xcc);

            tp9951_write_reg(0x0d, 0x72);

            tp9951_write_reg(0x15, 0x01);
            tp9951_write_reg(0x16, 0xf0);
            tp9951_write_reg(0x18, 0x2a);

            tp9951_write_reg(0x20, 0x38);
            tp9951_write_reg(0x21, 0x46);

            tp9951_write_reg(0x25, 0xfe);
            tp9951_write_reg(0x26, 0x0d);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x54);
            tp9951_write_reg(0x2e, 0x40);

            tp9951_write_reg(0x30, 0xa5);
            tp9951_write_reg(0x31, 0x95);
            tp9951_write_reg(0x32, 0xe0);
            tp9951_write_reg(0x33, 0x60);
        }
    }
    else if (FHD25 == fmt)
    {
        tp9951_write_reg(0x02, 0xc8);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x03);
        tp9951_write_reg(0x16, 0xd2);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x29);
        tp9951_write_reg(0x19, 0x38);
        tp9951_write_reg(0x1a, 0x47);

        tp9951_write_reg(0x1c, 0x0a);  //1920*1080, 25fps
        tp9951_write_reg(0x1d, 0x50);  //

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x05);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1C);

        if (STD_HDA == std) //AHD1080p25 extra
        {
            tp9951_write_reg(0x02, 0xcc);

            tp9951_write_reg(0x0d, 0x73);

            tp9951_write_reg(0x15, 0x01);
            tp9951_write_reg(0x16, 0xf0);
            tp9951_write_reg(0x18, 0x2a);

            tp9951_write_reg(0x20, 0x3c);
            tp9951_write_reg(0x21, 0x46);

            tp9951_write_reg(0x25, 0xfe);
            tp9951_write_reg(0x26, 0x0d);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x54);
            tp9951_write_reg(0x2e, 0x40);

            tp9951_write_reg(0x30, 0xa5);
            tp9951_write_reg(0x31, 0x86);
            tp9951_write_reg(0x32, 0xfb);
            tp9951_write_reg(0x33, 0x60);
        }
    }
    else if (FHD275 == fmt) //TVI 1080p27.5
    {
        tp9951_write_reg(0x02, 0xc8);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x88);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x29);
        tp9951_write_reg(0x19, 0x38);
        tp9951_write_reg(0x1a, 0x47);

        tp9951_write_reg(0x1c, 0x09);  //1920*1080, 27.5fps
        tp9951_write_reg(0x1d, 0x60);  //

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x05);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1C);
        if (STD_HDA == std)
        {
#if 0 // op2
            tp9951_write_reg(0x02, 0xcc);

            tp9951_write_reg(0x0d, 0x73);

            tp9951_write_reg(0x15, 0x11);
            tp9951_write_reg(0x16, 0xd2);
            tp9951_write_reg(0x18, 0x2a);

            tp9951_write_reg(0x20, 0x38);
            tp9951_write_reg(0x21, 0x46);

            tp9951_write_reg(0x25, 0xfe);
            tp9951_write_reg(0x26, 0x0d);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x54);
            tp9951_write_reg(0x2e, 0x40);

            tp9951_write_reg(0x30, 0xa6);
            tp9951_write_reg(0x31, 0x14);
            tp9951_write_reg(0x32, 0x7a);
            tp9951_write_reg(0x33, 0xe0);
#else   //op1
            tp9951_write_reg(0x02, 0xc8);

            tp9951_write_reg(0x0d, 0x50);

            tp9951_write_reg(0x15, 0x11);
            tp9951_write_reg(0x16, 0xd2);
            tp9951_write_reg(0x18, 0x2a);

            tp9951_write_reg(0x20, 0x38);
            tp9951_write_reg(0x21, 0x46);

            tp9951_write_reg(0x25, 0xfe);
            tp9951_write_reg(0x26, 0x0d);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x54);
            tp9951_write_reg(0x2e, 0x40);

            tp9951_write_reg(0x30, 0x29);
            tp9951_write_reg(0x31, 0x85);
            tp9951_write_reg(0x32, 0x1e);
            tp9951_write_reg(0x33, 0xb0);

#endif
        }

    }
    else if (FHD28 == fmt)  //TVI 1080p28
    {
        tp9951_write_reg(0x02, 0xc8);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x03);
        tp9951_write_reg(0x16, 0xd2);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x79);
        tp9951_write_reg(0x19, 0x38);
        tp9951_write_reg(0x1a, 0x47);

        tp9951_write_reg(0x1c, 0x08);  //1920*1080, 27.5fps
        tp9951_write_reg(0x1d, 0x98);  //

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x14);
        tp9951_write_reg(0x36, 0xb5);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1C);


    }
    else if (FHD_X3C == fmt) //TVI 1080p25  2475x1205
    {
        tp9951_write_reg(0x02, 0xc8);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0xE8);
        tp9951_write_reg(0x17, 0x80);
        tp9951_write_reg(0x18, 0x54);
        tp9951_write_reg(0x19, 0x38);
        tp9951_write_reg(0x1a, 0x47);

        tp9951_write_reg(0x1c, 0x09);
        tp9951_write_reg(0x1d, 0xAB);

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x14);
        tp9951_write_reg(0x36, 0xb5);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1C);


    }
    else if (UVGA25 == fmt) //TVI960P25
    {
        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x16);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0xa0);
        tp9951_write_reg(0x19, 0xc0);
        tp9951_write_reg(0x1a, 0x35);
        tp9951_write_reg(0x1c, 0x07);  //
        tp9951_write_reg(0x1d, 0xbc);  //

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x26, 0x01);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x0a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xba);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x14);
        tp9951_write_reg(0x36, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1c);
    }
    else if (UVGA30 == fmt) //TVI960P30
    {

        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x16);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0xa0);
        tp9951_write_reg(0x19, 0xc0);
        tp9951_write_reg(0x1a, 0x35);
        tp9951_write_reg(0x1c, 0x06);  //
        tp9951_write_reg(0x1d, 0x72);  //

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x26, 0x01);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x0a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x43);
        tp9951_write_reg(0x31, 0x3b);
        tp9951_write_reg(0x32, 0x79);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x14);
        tp9951_write_reg(0x36, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1c);
    }
    else if (F_UVGA30 == fmt) //FH 960P30
    {
        tp9951_write_reg(0x02, 0xcc);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x76);
        tp9951_write_reg(0x0e, 0x16);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x8f);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x23);
        tp9951_write_reg(0x19, 0xc0);
        tp9951_write_reg(0x1a, 0x35);
        tp9951_write_reg(0x1c, 0x07);  //
        tp9951_write_reg(0x1d, 0x08);  //

        tp9951_write_reg(0x20, 0x60);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x26, 0x05);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x70);
        tp9951_write_reg(0x2e, 0x50);

        tp9951_write_reg(0x30, 0x7f);
        tp9951_write_reg(0x31, 0x49);
        tp9951_write_reg(0x32, 0xf4);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x13);
        tp9951_write_reg(0x36, 0xe8);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x88);
    }
    else if (HD50 == fmt)
    {
        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x15);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x19);
        tp9951_write_reg(0x19, 0xd0);
        tp9951_write_reg(0x1a, 0x25);
        tp9951_write_reg(0x1c, 0x07);  //1280*720,
        tp9951_write_reg(0x1d, 0xbc);  //1280*720, 50fps

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x05);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1c);

        if (STD_HDA == std)     //subcarrier=24M
        {
            tp9951_write_reg(0x02, 0xce);
            tp9951_write_reg(0x05, 0x01);
            tp9951_write_reg(0x0d, 0x76);
            tp9951_write_reg(0x0e, 0x0a);
            tp9951_write_reg(0x14, 0x00);
            tp9951_write_reg(0x15, 0x13);
            tp9951_write_reg(0x16, 0x1a);
            tp9951_write_reg(0x18, 0x1b);

            tp9951_write_reg(0x20, 0x40);

            tp9951_write_reg(0x26, 0x01);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x54);
            tp9951_write_reg(0x2e, 0x50);

            tp9951_write_reg(0x30, 0xa5);
            tp9951_write_reg(0x31, 0x9f);
            tp9951_write_reg(0x32, 0xce);
            tp9951_write_reg(0x33, 0x60);
        }


    }
    else if (HD60 == fmt)
    {
        tp9951_write_reg(0x02, 0xca);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x13);
        tp9951_write_reg(0x0d, 0x50);

        tp9951_write_reg(0x15, 0x13);
        tp9951_write_reg(0x16, 0x15);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x19);
        tp9951_write_reg(0x19, 0xd0);
        tp9951_write_reg(0x1a, 0x25);
        tp9951_write_reg(0x1c, 0x06);  //1280*720,
        tp9951_write_reg(0x1d, 0x72);  //1280*720, 60fps

        tp9951_write_reg(0x20, 0x30);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x30);
        tp9951_write_reg(0x2e, 0x70);

        tp9951_write_reg(0x30, 0x48);
        tp9951_write_reg(0x31, 0xbb);
        tp9951_write_reg(0x32, 0x2e);
        tp9951_write_reg(0x33, 0x90);

        tp9951_write_reg(0x35, 0x05);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x1c);
#if 0
        if (STD_HDA == std)     ////subcarrier=11M
        {
            tp9951_write_reg(0x02, 0xce);
            tp9951_write_reg(0x05, 0xf9);
            tp9951_write_reg(0x0d, 0x76);
            tp9951_write_reg(0x0e, 0x03);
            tp9951_write_reg(0x14, 0x00);
            tp9951_write_reg(0x15, 0x13);
            tp9951_write_reg(0x16, 0x41);
            tp9951_write_reg(0x18, 0x1b);

            tp9951_write_reg(0x20, 0x50);
            tp9951_write_reg(0x21, 0x84);

            tp9951_write_reg(0x25, 0xff);
            tp9951_write_reg(0x26, 0x0d);

            tp9951_write_reg(0x2c, 0x3a);
            tp9951_write_reg(0x2d, 0x68);
            tp9951_write_reg(0x2e, 0x60);

            tp9951_write_reg(0x30, 0x4e);
            tp9951_write_reg(0x31, 0xf8);
            tp9951_write_reg(0x32, 0xdc);
            tp9951_write_reg(0x33, 0xf0);
        }
#else
        if (STD_HDA == std)     ////subcarrier=22M
        {
            tp9951_write_reg(0x02, 0xce);
            tp9951_write_reg(0x05, 0x55);
            tp9951_write_reg(0x0d, 0x76);
            tp9951_write_reg(0x0e, 0x08);
            tp9951_write_reg(0x14, 0x00);
            tp9951_write_reg(0x15, 0x13);
            tp9951_write_reg(0x16, 0x25);
            tp9951_write_reg(0x18, 0x1b);

            tp9951_write_reg(0x20, 0x50);
            tp9951_write_reg(0x21, 0x84);

            tp9951_write_reg(0x25, 0xff);
            tp9951_write_reg(0x26, 0x01);

            tp9951_write_reg(0x2c, 0x2a);
            tp9951_write_reg(0x2d, 0x54);
            tp9951_write_reg(0x2e, 0x60);

            tp9951_write_reg(0x30, 0xa5);
            tp9951_write_reg(0x31, 0x8b);
            tp9951_write_reg(0x32, 0xf2);
            tp9951_write_reg(0x33, 0x60);
        }
#endif
    }
    else if (A_UVGA30 == fmt) //AHD 960P30 1400X1125
    {

        tp9951_write_reg(0x02, 0xc8);
        tp9951_write_reg(0x05, 0x01);
        tp9951_write_reg(0x07, 0xc0);
        tp9951_write_reg(0x0b, 0xc0);
        tp9951_write_reg(0x0c, 0x03);
        tp9951_write_reg(0x0d, 0x76);
        tp9951_write_reg(0x0e, 0x12);

        tp9951_write_reg(0x15, 0x03);
        tp9951_write_reg(0x16, 0x5f);
        tp9951_write_reg(0x17, 0x00);
        tp9951_write_reg(0x18, 0x9c);
        tp9951_write_reg(0x19, 0xc0);
        tp9951_write_reg(0x1a, 0x35);
        tp9951_write_reg(0x1c, 0x85);  //
        tp9951_write_reg(0x1d, 0x78);  //

        tp9951_write_reg(0x20, 0x14);
        tp9951_write_reg(0x21, 0x84);
        tp9951_write_reg(0x22, 0x36);
        tp9951_write_reg(0x23, 0x3c);

        tp9951_write_reg(0x26, 0x0d);

        tp9951_write_reg(0x2b, 0x60);
        tp9951_write_reg(0x2c, 0x2a);
        tp9951_write_reg(0x2d, 0x1e);
        tp9951_write_reg(0x2e, 0x50);

        tp9951_write_reg(0x30, 0x29);
        tp9951_write_reg(0x31, 0x01);
        tp9951_write_reg(0x32, 0x76);
        tp9951_write_reg(0x33, 0x80);

        tp9951_write_reg(0x35, 0x14);
        tp9951_write_reg(0x36, 0x65);
        tp9951_write_reg(0x38, 0x00);
        tp9951_write_reg(0x39, 0x88);
    }
}

static int tp9951_chipid_check(struct tp9951_dev *sensor)
{
    u8 id_h = 0, id_l = 0;

    id_h = tp9951_read_reg(0xfe);
    id_l = tp9951_read_reg(0xff);

    if ((id_h << 8 | id_l) != TP9951_CHIP_ID) {
        LOG_E("Invalid Chip ID: 0x%02x%02x\n", id_h, id_l);
        // return -1;
    }

    return 0;
}

static bool tp9951_is_open(struct tp9951_dev *sensor)
{
    return sensor->on;
}

static void tp9951_power_on(struct tp9951_dev *sensor)
{
    if (sensor->on)
        return;

    camera_pin_set_high(sensor->pwdn_pin);
    LOG_I("Power on");
    sensor->on = true;
}

static void tp9951_power_off(struct tp9951_dev *sensor)
{
    if (!sensor->on)
        return;

    camera_pin_set_low(sensor->pwdn_pin);
    LOG_I("Power off");
    sensor->on = false;
}

static rt_err_t tp9951_init(rt_device_t dev)
{
    struct tp9951_dev *sensor = (struct tp9951_dev *)dev;
    struct mpp_video_fmt *fmt = &sensor->fmt;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    /* request optional power down pin */
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->pwdn_pin)
        return -RT_EINVAL;

    fmt->code = DEFAULT_MEDIA_CODE;
    fmt->width = DEFAULT_WIDTH;
    fmt->height = DEFAULT_HEIGHT;
    fmt->bus_type = DEFAULT_BUS_TYPE;
    fmt->flags = MEDIA_SIGNAL_FIELD_ACTIVE_LOW |
                 MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH |
                 MEDIA_SIGNAL_HSYNC_ACTIVE_LOW |
                 MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

#ifdef DEFAULT_IS_INTERLACED
    LOG_I("The input signal is interlace mode\n");
    fmt->flags |= MEDIA_SIGNAL_INTERLACED_MODE;
#endif

    return RT_EOK;
}

static rt_err_t tp9951_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct tp9951_dev *sensor = (struct tp9951_dev *)dev;

    if (tp9951_is_open(sensor))
        return RT_EOK;

    tp9951_power_on(sensor);

    if (tp9951_chipid_check(sensor))
        return -RT_ERROR;

    TP9951_sensor_init(DEFAULT_VIN_CH, DEFAULT_FORMAT, DEFAULT_IS_HDA);

    LOG_I("%s inited\n", DRV_NAME);
    return RT_EOK;
}

static rt_err_t tp9951_close(rt_device_t dev)
{
    struct tp9951_dev *sensor = (struct tp9951_dev *)dev;

    if (!tp9951_is_open(sensor))
        return -RT_ERROR;

    tp9951_power_off(sensor);
    return RT_EOK;
}

static int tp9951_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct tp9951_dev *sensor = (struct tp9951_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int tp9951_start(rt_device_t dev)
{
    return 0;
}

static int tp9951_stop(rt_device_t dev)
{
    return 0;
}

static u8 g_tp_data_ctrl = 0;

static int tp9951_pause(rt_device_t dev)
{
    g_tp_data_ctrl = tp9951_read_reg(0x4e);
    return tp9951_write_reg(0x4e, 0x0);
}

static int tp9951_resume(rt_device_t dev)
{
    if (g_tp_data_ctrl)
        return tp9951_write_reg(0x4e, g_tp_data_ctrl);

    LOG_W("Invalid clock ctrl: 0x%x\n", g_tp_data_ctrl);
    return -1;
}

static rt_err_t tp9951_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return tp9951_start(dev);
    case CAMERA_CMD_STOP:
        return tp9951_stop(dev);
    case CAMERA_CMD_PAUSE:
        return tp9951_pause(dev);
    case CAMERA_CMD_RESUME:
        return tp9951_resume(dev);
    case CAMERA_CMD_GET_FMT:
        return tp9951_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops tp9951_ops =
{
    .init = tp9951_init,
    .open = tp9951_open,
    .close = tp9951_close,
    .control = tp9951_control,
};
#endif

int rt_hw_tp9951_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_tp_dev.dev.ops = &tp9951_ops;
#else
    g_tp_dev.dev.init = tp9951_init;
    g_tp_dev.dev.open = tp9951_open;
    g_tp_dev.dev.close = tp9951_close;
    g_tp_dev.dev.control = tp9951_control;
#endif
    g_tp_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_tp_dev.dev, CAMERA_DEV_NAME, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_tp9951_init);
