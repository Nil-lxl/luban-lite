/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "N5"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"
struct n5_dev *sensor;
/* Default format configuration of NextChip N5 */
#define N5_DFT_MODE         AHD_1080P_25HZ
#define N5_DFT_WIDTH        HD_720_WIDTH
#define N5_DFT_HEIGHT       HD_720_HEIGHT
#define N5_DFT_BUS_TYPE     MEDIA_BUS_BT656
#define N5_DFT_CODE         MEDIA_BUS_FMT_UYVY8_2X8
#define N5_DFT_CHAN         0
#define N5_TEST_MODE        0
#define READ_ID_TEST        1
#define N5_USE_RES_DETECT   0
#define N5_I2C_SLAVE_ID     0x32
#define N5_CHIP_ID          0xE0

enum n5_output_channel {
    //CVBS
    N5_OutMode_SD_Input_1_Output_1 =0,
    N5_OutMode_SD_Input_1_Output_2,
    N5_OutMode_SD_Input_2_Output_1,
    N5_OutMode_SD_Input_2_Output_2,
    //720P
    N5_OutMode_HD_Input_1_Output_1,
    N5_OutMode_HD_Input_1_Output_2,
    N5_OutMode_HD_Input_2_Output_1,
    N5_OutMode_HD_Input_2_Output_2,
    //1080P
    N5_OutMode_FHD_Input_1_Output_1,
    N5_OutMode_FHD_Input_1_Output_2,
    N5_OutMode_FHD_Input_2_Output_1,
    N5_OutMode_FHD_Input_2_Output_2
};

enum n5_mode {
    AHD_CVBS_NTSC = 0,
    AHD_CVBS_PAL,
    AHD_720P_25HZ,
    AHD_720P_30HZ,
    AHD_1080P_25HZ,
    AHD_1080P_30HZ,

    AHD_NOSIGNAL = 0xFF
};

#define N5_BRIGHTNESS_REG   0x0C
#define N5_SATURATION_REG   0x3C
#define N5_HUE_REG          0x40
#define N5_CONTRAST_REG     0x10

struct n5_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 rst_pin;
    u32 pwdn_pin;

    struct mpp_video_fmt fmt;

    bool on;
    bool streaming;
};

static struct n5_dev g_n5_dev = {0};

static struct reg8_info n5_common_regs[] = {
    {0xff, 0x00},
    {0x00, 0x10},
    {0x01, 0x10},
    {0x18, 0x3f},
    {0x19, 0x3f},
    {0x22, 0x0b},
    {0x23, 0x41},
    {0x26, 0x0b},
    {0x27, 0x41},
    {0x54, 0x00},
    {0xa0, 0x05},
    {0xa1, 0x05},

    //{0x3C, 0x80},

    {0xff, 0x01},
    {0x97, 0x00},
    {0x97, 0x0f},
    {0x7A, 0x0f},
    {0xB3, 0x0f}, // inv hs/vs  MPP1_INV bit0/MPP2_INV bit1 /MPP3_INV bit2/MPP4_INV bit3

    //0x90: CH1 BT601 Signal Mode though MPP1 and MPP2 pins
    //0xA0: CH2 BT601 Signal Mode though MPP1 and MPP2 pins
    //{0xA8, 0x90},  //h/v0 sync enabled
    //0x90: CH1 BT601 Signal Mode though MPP3 and MPP4 pins
    //0xA0: CH2 BT601 Signal Mode though MPP3 and MPP4 pins
    //{0xA9, 0x90}, //h/v1 sync enabled
    //{0xBC, 0x10},  //h/v0 swap enabled
    //{0xBD, 0x10},
    //{0xBE, 0x10},  //h/v1 swap enabled
    //{0xBF, 0x10},

    {0xff, 0x05},
    {0x00, 0xd0},
    {0x01, 0x62},
    {0x05, 0x04},
    {0x08, 0x55},
    {0x1b, 0x08},
    {0x25, 0xdc},
    {0x28, 0x80},
    {0x2f, 0x00},
    {0x30, 0xe0},
    {0x31, 0x43},
    {0x32, 0xa2},
    {0x57, 0x00},
    {0x58, 0x77},
    {0x5b, 0x41},
    {0x5c, 0x78},
    {0x5f, 0x00},
    {0x7b, 0x11},
    {0x7c, 0x01},
    {0x7d, 0x80},
    {0x80, 0x00},
    {0x90, 0x01},
    {0xa9, 0x00},
    {0xb8, 0x39},
    {0xb9, 0x72},
    {0xd1, 0x00},
    {0xd5, 0x80},

    {0xff, 0x06},
    {0x00, 0xd0},
    {0x01, 0x62},
    {0x05, 0x04},
    {0x08, 0x55},
    {0x1b, 0x08},
    {0x25, 0xdc},
    {0x28, 0x80},
    {0x2f, 0x00},
    {0x30, 0xe0},
    {0x31, 0x43},
    {0x32, 0xa2},
    {0x57, 0x00},
    {0x58, 0x77},
    {0x5b, 0x41},
    {0x5c, 0x78},
    {0x5f, 0x00},
    {0x7b, 0x11},
    {0x7c, 0x01},
    {0x7d, 0x80},
    {0x80, 0x00},
    {0x90, 0x01},
    {0xa9, 0x00},
    {0xb8, 0x39},
    {0xb9, 0x72},
    {0xd1, 0x00},
    {0xd5, 0x80},

    {0xff, 0x09},
    {0x50, 0x30},
    {0x51, 0x6f},
    {0x52, 0x67},
    {0x53, 0x48},
    {0x54, 0x30},
    {0x55, 0x6f},
    {0x56, 0x67},
    {0x57, 0x48},
    {0x96, 0x00},
    {0x9e, 0x00},
    {0xb6, 0x00},
    {0xbe, 0x00},

    {0xff, 0x0a},
    {0x25, 0x10},
    {0x27, 0x1e},
    {0x30, 0xac},
    {0x31, 0x78},
    {0x32, 0x17},
    {0x33, 0xc1},
    {0x34, 0x40},
    {0x35, 0x00},
    {0x36, 0xc3},
    {0x37, 0x0a},
    {0x38, 0x00},
    {0x39, 0x02},
    {0x3a, 0x00},
    {0x3b, 0xb2},
    {0xa5, 0x10},
    {0xa7, 0x1e},
    {0xb0, 0xac},
    {0xb1, 0x78},
    {0xb2, 0x17},
    {0xb3, 0xc1},
    {0xb4, 0x40},
    {0xb5, 0x00},
    {0xb6, 0xc3},
    {0xb7, 0x0a},
    {0xb8, 0x00},
    {0xb9, 0x02},
    {0xba, 0x00},
    {0xbb, 0xb2},
    {0x77, 0x8F},
    {0xF7, 0x8F},

    // {0xff, 0x13},
    // {0x07, 0x47},
    // {0x12, 0x04},
    // {0x1e, 0x1f},
    // {0x1f, 0x27},
    // {0x2e, 0x10},
    // {0x2f, 0xc8},
    // {0x30, 0x00},
    // {0x31, 0xff},
    // {0x32, 0x00},
    // {0x33, 0x00},
    // {0x3a, 0xff},
    // {0x3b, 0xff},
    // {0x3c, 0xff},
    // {0x3d, 0xff},
    // {0x3e, 0xff},
    // {0x3f, 0x0f},
    // {0x70, 0x00},
    // {0x72, 0x05},
    // {0x7A, 0xf0},

    {0xff, 0x13},
    {0x05, 0xA0},
    {0x07, 0x47},
    {0x0d, 0x40},
    {0x0f, 0x40},
    {0x12, 0x04},
    {0x1e, 0x1f},
    {0x1f, 0x27},
    {0x2e, 0x10},
    {0x2f, 0xc8},
    {0x30, 0x00},
    {0x31, 0xff},
    {0x32, 0x00},
    {0x33, 0x00},
    {0x3a, 0x00},
    {0x3b, 0x00},
    {0x3c, 0x00},
    {0x3d, 0x00},
    {0x3e, 0x00},
    {0x3f, 0x00},
    {0x70, 0x00},
    {0x72, 0x05},
    {0x73, 0x23},
    {0x7A, 0xf0},


    {0xff, 0x00}, //8x8 color block test pattern
    {0x78, 0xaa},//When No-Video, BackGround Color
#if N5_TEST_MODE
    // Test mode
    {0xff, 0x05},
    {0x2c, 0x08},
    {0x6a, 0x80},
    {0xff, 0x06},
    {0x2c, 0x08},
    {0x6a, 0x80},
#endif
};

static int n5_write_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(i2c, N5_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static u8 n5_read_reg(struct rt_i2c_bus_device *i2c, u8 reg)
{
    u8 val = 0;

    if (rt_i2c_read_reg(i2c, N5_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return val;
}
#if READ_ID_TEST
void nvp6158_dump_bank(int bank)
{
    #if 1
    u32 reg_addr=0;
    u32 reg_value2[16];
    int j,i;

    printf("--------------------[BANK = 0x%02x]----------------------\n",bank);
    printf("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    printf("----------------------------------------------------\n");

    n5_write_reg(sensor->i2c, 0xFF, bank);

    for(j = 0 ; j < 16; j++) {
        for(i = 0; i < 16; i++) {
            reg_addr  = i+(0x10*j);
            reg_value2[i] = n5_read_reg(sensor->i2c, reg_addr);
            if (i==15) {
                printf("%02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", j*0x10, \
                        reg_value2[0], reg_value2[1], reg_value2[2], reg_value2[3], reg_value2[4], reg_value2[5], reg_value2[6], reg_value2[7], \
                        reg_value2[8], reg_value2[9], reg_value2[10], reg_value2[11], reg_value2[12], reg_value2[13], reg_value2[14], reg_value2[15]);
            }
        }
    }
    printf("\n");
    #endif
}
#endif
static void n5_selchannel_portmode(struct n5_dev *sensor, u8 ch, u8 MuxMode)
{
    u8 val_1xc8, val_1xca, val_0x54;
    u8 clk_freq_array[4] = {0x89,0x09,0x49,0x69}; //clk_freq: 0~3:37.125M/74.25M/148.5M/297M

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    val_0x54 = n5_read_reg(sensor->i2c, 0x54);
    val_0x54 &= 0xFE;
    n5_write_reg(sensor->i2c, 0x54, val_0x54);
    n5_write_reg(sensor->i2c, 0xff, 0x01);
    val_1xc8 = n5_read_reg(sensor->i2c, 0xc8);

    n5_write_reg(sensor->i2c, 0xA0, 0x00);
    n5_write_reg(sensor->i2c, 0xA1, 0x00);

    switch(MuxMode)
    {
        // CVBS
        case N5_OutMode_SD_Input_1_Output_1:
            n5_write_reg(sensor->i2c, 0xC0, 0x00);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[0]);
            break;

        case N5_OutMode_SD_Input_1_Output_2:
            n5_write_reg(sensor->i2c, 0xC2, 0x00);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[0]);
            break;

        case N5_OutMode_SD_Input_2_Output_1:
            n5_write_reg(sensor->i2c, 0xC0, 0x11);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[0]);
            break;

        case N5_OutMode_SD_Input_2_Output_2:
            n5_write_reg(sensor->i2c, 0xC2, 0x11);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[0]);
            break;

        //720P
        case N5_OutMode_HD_Input_1_Output_1:
            n5_write_reg(sensor->i2c, 0xC0, 0x00);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[1]);
            break;

        case N5_OutMode_HD_Input_1_Output_2:
            n5_write_reg(sensor->i2c, 0xC2, 0x00);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[1]);
            break;

        case N5_OutMode_HD_Input_2_Output_1:
            n5_write_reg(sensor->i2c, 0xC0, 0x11);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[1]);
            break;

        case N5_OutMode_HD_Input_2_Output_2:
            n5_write_reg(sensor->i2c, 0xC2, 0x11);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[1]);
            break;

        // 1080P
        case N5_OutMode_FHD_Input_1_Output_1:
            n5_write_reg(sensor->i2c, 0xC0, 0x00);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[2]);
            break;

        case N5_OutMode_FHD_Input_1_Output_2:
            n5_write_reg(sensor->i2c, 0xC2, 0x00);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[2]);
            break;

        case N5_OutMode_FHD_Input_2_Output_1:
            n5_write_reg(sensor->i2c, 0xC0, 0x11);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[2]);
            break;

        case N5_OutMode_FHD_Input_2_Output_2:
            n5_write_reg(sensor->i2c, 0xC2, 0x11);
            n5_write_reg(sensor->i2c, 0xCC + ch, clk_freq_array[2]);
            break;
    }
    val_1xc8 &= (1 == ch? 0x0F : 0xF0);
    n5_write_reg(sensor->i2c, 0xC8, val_1xc8);
    val_1xca = n5_read_reg(sensor->i2c, 0xCA);
    val_1xca |= (0x11 << ch); //enable port
    n5_write_reg(sensor->i2c, 0xCA, val_1xca);
}

static void n5_set_chn_720h_ntsc(struct n5_dev *sensor, u8 ch)
{
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x08 + ch, 0xa0);
    n5_write_reg(sensor->i2c, 0x34 + ch, 0x00);
    n5_write_reg(sensor->i2c, 0x81 + ch, 0x60);
    n5_write_reg(sensor->i2c, 0x85 + ch, 0x00);
    n5_write_reg(sensor->i2c, 0x54, n5_read_reg(sensor->i2c, 0x54) | (0x10 << ch));
    n5_write_reg(sensor->i2c, 0x64 + ch, 0xa1);
    n5_write_reg(sensor->i2c, 0x89 + ch, 0x10);
    n5_write_reg(sensor->i2c, ch+0x8e, 0x2f);
    n5_write_reg(sensor->i2c, 0x30 + ch, 0x12);
    n5_write_reg(sensor->i2c, 0xa0 + ch, 0x05);

    n5_write_reg(sensor->i2c, 0xff, 0x01);
    n5_write_reg(sensor->i2c, 0x84 + ch, 0x06);
    n5_write_reg(sensor->i2c, 0x8c + ch, 0x86);
    n5_write_reg(sensor->i2c, 0xed, n5_read_reg(sensor->i2c, 0xed) & (~(0x01<<ch)));

    n5_write_reg(sensor->i2c, 0xff, 0x05 + ch);
    n5_write_reg(sensor->i2c, 0x25, 0xdc);
    n5_write_reg(sensor->i2c, 0x2b, 0x78);
    n5_write_reg(sensor->i2c, 0x47, 0x04);
    n5_write_reg(sensor->i2c, 0x50, 0x84);
    n5_write_reg(sensor->i2c, 0x69, 0x00);
    n5_write_reg(sensor->i2c, 0x7b, 0x00);
    n5_write_reg(sensor->i2c, 0xb8, 0xb9);

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x0c + ch, 0x00); //bright
    n5_write_reg(sensor->i2c, 0x10 + ch, 0x80); //contrast
    n5_write_reg(sensor->i2c, 0x40 + ch, 0x00); //hue
    n5_write_reg(sensor->i2c, 0x44 + ch, 0x00); //uGain
    n5_write_reg(sensor->i2c, 0x48 + ch, 0x10); //vGain
    n5_write_reg(sensor->i2c, 0x4C + ch, 0x00); //uoffset
    n5_write_reg(sensor->i2c, 0x50 + ch, 0x00); //voffset
    n5_write_reg(sensor->i2c, 0x18 + ch, 0x08); //Y_Peak
    n5_write_reg(sensor->i2c, 0x58 + ch, 0x90); //H_Delay
    n5_write_reg(sensor->i2c, 0x5c + ch, 0xbe); //V_Delay

    n5_write_reg(sensor->i2c, 0xff, 0x05 + ch);
    n5_write_reg(sensor->i2c, 0x20, 0x90);
    n5_write_reg(sensor->i2c, 0x28, 0x80);
    n5_write_reg(sensor->i2c, 0x58, 0x70);
    n5_write_reg(sensor->i2c, 0x5C, 0x78);
    n5_write_reg(sensor->i2c, 0x01, 0x62);
}

static void n5_set_chn_720h_pal(struct n5_dev *sensor, u8 ch)
{
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x08 + ch, 0xdd);
    n5_write_reg(sensor->i2c, 0x34 + ch, 0x00);
    n5_write_reg(sensor->i2c, 0x81 + ch, 0x70);
    n5_write_reg(sensor->i2c, 0x85 + ch, 0x00);
    n5_write_reg(sensor->i2c, 0x54, n5_read_reg(sensor->i2c, 0x54)&(~(0x10<<ch)));
    n5_write_reg(sensor->i2c, 0x64 + ch, 0xa0);
    n5_write_reg(sensor->i2c, 0x89 + ch, 0x10);
    n5_write_reg(sensor->i2c, ch+0x8e, 0x2e);
    n5_write_reg(sensor->i2c, 0x30 + ch, 0x12);
    n5_write_reg(sensor->i2c, 0xa0 + ch, 0x05);

    n5_write_reg(sensor->i2c, 0xff, 0x01);
    n5_write_reg(sensor->i2c, 0x84 + ch, 0x06);
    n5_write_reg(sensor->i2c, 0x8c + ch, 0x86);
    n5_write_reg(sensor->i2c, 0xed, n5_read_reg(sensor->i2c, 0xed)&(~(0x01<<ch)));

    n5_write_reg(sensor->i2c, 0xff, 0x05 + ch);
    n5_write_reg(sensor->i2c, 0x25, 0xcc);
    n5_write_reg(sensor->i2c, 0x2b, 0x78);
    n5_write_reg(sensor->i2c, 0x47, 0x04);
    n5_write_reg(sensor->i2c, 0x50, 0x84);
    n5_write_reg(sensor->i2c, 0x69, 0x00);
    n5_write_reg(sensor->i2c, 0x7b, 0x00);
    n5_write_reg(sensor->i2c, 0xb8, 0xb9);

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x0c + ch, 0x00); //bright
    n5_write_reg(sensor->i2c, 0x10 + ch, 0x80); //contrast
    n5_write_reg(sensor->i2c, 0x40 + ch, 0x00); //hue
    n5_write_reg(sensor->i2c, 0x44 + ch, 0x00); //uGain
    n5_write_reg(sensor->i2c, 0x48 + ch, 0x10); //vGain
    n5_write_reg(sensor->i2c, 0x4C + ch, 0x00); //uoffset
    n5_write_reg(sensor->i2c, 0x50 + ch, 0x00); //voffset
    n5_write_reg(sensor->i2c, 0x18 + ch, 0x3f); //Y_Peak
    n5_write_reg(sensor->i2c, 0x58 + ch, 0x80); //H_Delay
    n5_write_reg(sensor->i2c, 0x5c + ch, 0xbe); //V_Delay

    n5_write_reg(sensor->i2c, 0xff, 0x05 + ch);
    n5_write_reg(sensor->i2c, 0x20, 0x90);
    n5_write_reg(sensor->i2c, 0x28, 0x80);
    n5_write_reg(sensor->i2c, 0x58, 0x70);
    n5_write_reg(sensor->i2c, 0x5C, 0x78);
    n5_write_reg(sensor->i2c, 0x01, 0x62);
}

static void n5_set_chn_720p_30(struct n5_dev *sensor, u8 ch)
{
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x08+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x34+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x81+ch, 0x06);
    n5_write_reg(sensor->i2c, 0x85+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x54, n5_read_reg(sensor->i2c, 0x54)&(~(0x10<<ch)));
    n5_write_reg(sensor->i2c, 0x64+ch, 0x01);
    n5_write_reg(sensor->i2c, 0x89+ch, 0x00);
    n5_write_reg(sensor->i2c, ch+0x8e, 0x00);
    n5_write_reg(sensor->i2c, 0x30+ch, 0x12);
    n5_write_reg(sensor->i2c, 0xa0+ch, 0x05);

    n5_write_reg(sensor->i2c, 0xff, 0x01);
    n5_write_reg(sensor->i2c, 0x84+ch, 0x08);
    n5_write_reg(sensor->i2c, 0x8c+ch, 0x08);
    n5_write_reg(sensor->i2c, 0xed, n5_read_reg(sensor->i2c, 0xed)&(~(0x01<<ch)));

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x25, 0xdc);
    n5_write_reg(sensor->i2c, 0x2b, 0xa8);
    n5_write_reg(sensor->i2c, 0x47, 0xee);
    n5_write_reg(sensor->i2c, 0x50, 0xc6);
    n5_write_reg(sensor->i2c, 0x69, 0x00);
    n5_write_reg(sensor->i2c, 0x7b, 0x11);
    n5_write_reg(sensor->i2c, 0xb8, 0x39);

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x0c+ch, 0x00); //bright
    n5_write_reg(sensor->i2c, 0x10+ch, 0x80); //contrast
    n5_write_reg(sensor->i2c, 0x40+ch, 0x00); //hue
    n5_write_reg(sensor->i2c, 0x44+ch, 0x00); //uGain
    n5_write_reg(sensor->i2c, 0x48+ch, 0x10); //vGain
    n5_write_reg(sensor->i2c, 0x4C+ch, 0x00); //uoffset
    n5_write_reg(sensor->i2c, 0x50+ch, 0x00); //voffset
    n5_write_reg(sensor->i2c, 0x18+ch, 0x3f); //Y_Peak
    n5_write_reg(sensor->i2c, 0x58+ch, 0x80); //H_Delay
    n5_write_reg(sensor->i2c, 0x5c+ch, 0x80); //V_Delay

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x20, 0x80);
    n5_write_reg(sensor->i2c, 0x28, 0x80);
    n5_write_reg(sensor->i2c, 0x58, 0x70);
    n5_write_reg(sensor->i2c, 0x5C, 0x78);
    n5_write_reg(sensor->i2c, 0x01, 0x62);
}

static void n5_set_chn_720p_25(struct n5_dev *sensor, u8 ch)
{
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x08+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x34+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x81+ch, 0x07);
    n5_write_reg(sensor->i2c, 0x85+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x54, n5_read_reg(sensor->i2c, 0x54)&(~(0x10<<ch)));
    n5_write_reg(sensor->i2c, 0x64+ch, 0x01);
    n5_write_reg(sensor->i2c, 0x89+ch, 0x00);
    n5_write_reg(sensor->i2c, ch+0x8e, 0x00);
    n5_write_reg(sensor->i2c, 0x30+ch, 0x12);
    n5_write_reg(sensor->i2c, 0xa0+ch, 0x05);

    n5_write_reg(sensor->i2c, 0xff, 0x01);
    n5_write_reg(sensor->i2c, 0x84+ch, 0x08);
    n5_write_reg(sensor->i2c, 0x8c+ch, 0x08);
    n5_write_reg(sensor->i2c, 0xed, n5_read_reg(sensor->i2c, 0xed)&(~(0x01<<ch)));

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x25, 0xdc);
    n5_write_reg(sensor->i2c, 0x2b, 0xa8);
    n5_write_reg(sensor->i2c, 0x47, 0xee);
    n5_write_reg(sensor->i2c, 0x50, 0xc6);
    n5_write_reg(sensor->i2c, 0x69, 0x00);
    n5_write_reg(sensor->i2c, 0x7b, 0x11);
    n5_write_reg(sensor->i2c, 0xb8, 0x39);

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x0c+ch, 0x00); //bright
    n5_write_reg(sensor->i2c, 0x10+ch, 0x80); //contrast
    n5_write_reg(sensor->i2c, 0x40+ch, 0x00); //hue
    n5_write_reg(sensor->i2c, 0x44+ch, 0x00); //uGain
    n5_write_reg(sensor->i2c, 0x48+ch, 0x10); //vGain
    n5_write_reg(sensor->i2c, 0x4C+ch, 0x00); //uoffset
    n5_write_reg(sensor->i2c, 0x50+ch, 0x00); //voffset
    n5_write_reg(sensor->i2c, 0x18+ch, 0x3f); //Y_Peak
    n5_write_reg(sensor->i2c, 0x58+ch, 0x80); //H_Delay
    n5_write_reg(sensor->i2c, 0x5c+ch, 0x80); //V_Delay

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x20, 0x78);
    n5_write_reg(sensor->i2c, 0x28, 0x80);
    n5_write_reg(sensor->i2c, 0x58, 0x70);
    n5_write_reg(sensor->i2c, 0x5C, 0x78);
    n5_write_reg(sensor->i2c, 0x01, 0x62);
}

static void n5_set_chn_1080p_30(struct n5_dev *sensor, u8 ch)
{
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x08+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x34+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x81+ch, 0x02);
    n5_write_reg(sensor->i2c, 0x85+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x54, n5_read_reg(sensor->i2c, 0x54)&(~(0x10<<ch)));
    n5_write_reg(sensor->i2c, 0x64+ch, 0x01);
    n5_write_reg(sensor->i2c, 0x89+ch, 0x10);
    n5_write_reg(sensor->i2c, ch+0x8e, 0x00);
    n5_write_reg(sensor->i2c, 0x30+ch, 0x12);
    n5_write_reg(sensor->i2c, 0xa0+ch, 0x05);

    n5_write_reg(sensor->i2c, 0xff, 0x01);
    n5_write_reg(sensor->i2c, 0x84+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x8c+ch, 0x40);
    n5_write_reg(sensor->i2c, 0xed, n5_read_reg(sensor->i2c, 0xed)&(~(0x01<<ch)));

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x25, 0xdc);
    n5_write_reg(sensor->i2c, 0x2b, 0xa8);
    n5_write_reg(sensor->i2c, 0x47, 0xee);
    n5_write_reg(sensor->i2c, 0x50, 0xc6);
    n5_write_reg(sensor->i2c, 0x69, 0x00);
    n5_write_reg(sensor->i2c, 0x7b, 0x11);
    n5_write_reg(sensor->i2c, 0xb8, 0x39);

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x0c+ch, 0x00); //bright
    n5_write_reg(sensor->i2c, 0x10+ch, 0x80); //contrast
    n5_write_reg(sensor->i2c, 0x40+ch, 0x00); //hue
    n5_write_reg(sensor->i2c, 0x44+ch, 0x00); //uGain
    n5_write_reg(sensor->i2c, 0x48+ch, 0x10); //vGain
    n5_write_reg(sensor->i2c, 0x4C+ch, 0x00); //uoffset
    n5_write_reg(sensor->i2c, 0x50+ch, 0x00); //voffset
    n5_write_reg(sensor->i2c, 0x18+ch, 0x3f); //Y_Peak
    n5_write_reg(sensor->i2c, 0x58+ch, 0x82); //H_Delay
    n5_write_reg(sensor->i2c, 0x5c+ch, 0x80); //V_Delay

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x20, 0x80);
    n5_write_reg(sensor->i2c, 0x28, 0x80);
    n5_write_reg(sensor->i2c, 0x58, 0x70);
    n5_write_reg(sensor->i2c, 0x5C, 0x78);
    n5_write_reg(sensor->i2c, 0x01, 0x62);
}

static void n5_set_chn_1080p_25(struct n5_dev *sensor, u8 ch)
{
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x08+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x34+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x81+ch, 0x03);
    n5_write_reg(sensor->i2c, 0x85+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x54, n5_read_reg(sensor->i2c, 0x54)&(~(0x10<<ch)));
    n5_write_reg(sensor->i2c, 0x64+ch, 0x01);
    n5_write_reg(sensor->i2c, 0x89+ch, 0x10);
    n5_write_reg(sensor->i2c, ch+0x8e, 0x00);
    n5_write_reg(sensor->i2c, 0x30+ch, 0x12);
    n5_write_reg(sensor->i2c, 0xa0+ch, 0x05);

    n5_write_reg(sensor->i2c, 0xff, 0x01);
    n5_write_reg(sensor->i2c, 0x84+ch, 0x00);
    n5_write_reg(sensor->i2c, 0x8c+ch, 0x40);
    n5_write_reg(sensor->i2c, 0xed, n5_read_reg(sensor->i2c, 0xed)&(~(0x01<<ch)));

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x25, 0xdc);
    n5_write_reg(sensor->i2c, 0x2b, 0xa8);
    n5_write_reg(sensor->i2c, 0x47, 0xee);
    n5_write_reg(sensor->i2c, 0x50, 0xc6);
    n5_write_reg(sensor->i2c, 0x69, 0x00);
    n5_write_reg(sensor->i2c, 0x7b, 0x11);
    n5_write_reg(sensor->i2c, 0xb8, 0x39);

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x0c+ch, 0x00); //bright
    n5_write_reg(sensor->i2c, 0x10+ch, 0x80); //contrast
    n5_write_reg(sensor->i2c, 0x40+ch, 0x00); //hue
    n5_write_reg(sensor->i2c, 0x44+ch, 0x00); //uGain
    n5_write_reg(sensor->i2c, 0x48+ch, 0x10); //vGain
    n5_write_reg(sensor->i2c, 0x4C+ch, 0x00); //uoffset
    n5_write_reg(sensor->i2c, 0x50+ch, 0x00); //voffset
    n5_write_reg(sensor->i2c, 0x18+ch, 0x3f); //Y_Peak
    n5_write_reg(sensor->i2c, 0x58+ch, 0x82); //H_Delay
    n5_write_reg(sensor->i2c, 0x5c+ch, 0x80); //V_Delay

    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0x20, 0x80); //0x84//0xA0//0x84 //0x70 // black level
    n5_write_reg(sensor->i2c, 0x28, 0x80);
    n5_write_reg(sensor->i2c, 0x58, 0x70);
    n5_write_reg(sensor->i2c, 0x5C, 0x78);
    n5_write_reg(sensor->i2c, 0x01, 0x62);
}

static void n5_setoutchannel_mode(struct n5_dev *sensor, u8 ch, enum n5_mode mode)
{
    switch (mode) {
    case AHD_CVBS_NTSC:
        LOG_I(" >>> NTSC");
        n5_set_chn_720h_ntsc(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_SD_Input_2_Output_1:N5_OutMode_SD_Input_1_Output_1);
        break;

    case AHD_CVBS_PAL:
        LOG_I(" >>> PAL");
        n5_set_chn_720h_pal(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_SD_Input_2_Output_1:N5_OutMode_SD_Input_1_Output_1);
        break;

    case AHD_720P_25HZ:
        LOG_I(" >>> AHD_720P_25PFS");
        n5_set_chn_720p_25(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_HD_Input_2_Output_1:N5_OutMode_HD_Input_1_Output_1);
        break;

    case AHD_NOSIGNAL:
        LOG_I(" >>> AHD_720P_25PFS");
        n5_set_chn_720p_25(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_HD_Input_2_Output_1:N5_OutMode_HD_Input_1_Output_1);
        break;

    case AHD_720P_30HZ:
        LOG_I(" >>> AHD_720P_30PFS");
        n5_set_chn_720p_30(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_HD_Input_2_Output_1:N5_OutMode_HD_Input_1_Output_1);
        break;

    case AHD_1080P_25HZ:
        LOG_I(" >>> AHD_1080P_25PFS");
        n5_set_chn_1080p_25(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_FHD_Input_2_Output_1:N5_OutMode_FHD_Input_1_Output_1);
        break;

    case AHD_1080P_30HZ:
        LOG_I(" >>> AHD_1080P_30PFS");
        n5_set_chn_1080p_30(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_FHD_Input_2_Output_1:N5_OutMode_FHD_Input_1_Output_1);
        break;

    default:
        LOG_I(" >>> Default AHD_1080P_25PFS");
        n5_set_chn_1080p_25(sensor, ch);
        n5_selchannel_portmode(sensor, 0, ch?N5_OutMode_FHD_Input_2_Output_1:N5_OutMode_FHD_Input_1_Output_1);
        break;
    }
}

static void n5_auto_detect_mode(struct n5_dev *sensor)
{
    n5_write_reg(sensor->i2c, 0xff, 0x13);
    n5_write_reg(sensor->i2c, 0x30, 0x7f);
    n5_write_reg(sensor->i2c, 0x70, 0x30);
    n5_write_reg(sensor->i2c, 0xff, 0x00);
    n5_write_reg(sensor->i2c, 0x00, 0x18);
    n5_write_reg(sensor->i2c, 0x01, 0x18);
    n5_write_reg(sensor->i2c, 0x02, 0x18);
    n5_write_reg(sensor->i2c, 0x03, 0x18);
    n5_write_reg(sensor->i2c, 0x00, 0x10);
    n5_write_reg(sensor->i2c, 0x01, 0x10);
    n5_write_reg(sensor->i2c, 0x02, 0x10);
    n5_write_reg(sensor->i2c, 0x03, 0x10);
}

static void n5_video_format_unknown(struct n5_dev *sensor, u32 ch)
{
    n5_setoutchannel_mode(sensor, ch, AHD_1080P_25HZ);
    n5_write_reg(sensor->i2c, 0xff, 0x05+ch);
    n5_write_reg(sensor->i2c, 0xB8, 0xB8);
    n5_write_reg(sensor->i2c, 0xff, 0x13);
    n5_write_reg(sensor->i2c, 0x70, n5_read_reg(sensor->i2c, 0x70)&(~(0x01<<ch)));
}

static void n5_init_common(struct n5_dev *sensor)
{
    struct reg8_info *info = n5_common_regs;
    int i;

    for (i = 0; i < ARRAY_SIZE(n5_common_regs); i++, info++)
        n5_write_reg(sensor->i2c, info->reg, info->val);
}
#if N5_USE_RES_DETECT
static u8 n5_resolutiondetect(struct n5_dev *sensor, u8 ch)
{
    u8 detCamera = 0;
    u8 detFormat = 0;

    n5_write_reg(sensor->i2c, 0xff, 0x00);
    detCamera = (n5_read_reg(sensor->i2c, 0xa8) & (0x01 << ch));
    LOG_I("detCamera = %x",detCamera);
    if (detCamera == 0x00) {
        n5_write_reg(sensor->i2c, 0xff, 0x05 + ch%2);
        detFormat = n5_read_reg(sensor->i2c, 0xf0);
        LOG_I("detFormat = %x", detFormat);

        if (detFormat == 0x00) // NTSC
        {
            LOG_I("AHD_CVBS_NTSC");
            return AHD_CVBS_NTSC;
        }
        else if (detFormat == 0x10) //PAL
        {
            LOG_I("AHD_CVBS_PAL");
            return AHD_CVBS_PAL;
        }
        else if (detFormat == 0x21) //720p_25fps
        {
            LOG_I("AHD_720P_25HZ");
            return AHD_720P_25HZ;
        }
        else if (detFormat == 0x20) //720P_30fps
        {
            LOG_I("AHD_720P_30HZ");
            return AHD_720P_30HZ;
        }
        else if (detFormat == 0x31) // 1080P_25fps
        {
            LOG_I("AHD_1080P_25HZ");
            return AHD_1080P_25HZ;
        }
        else if (detFormat == 0x30) //1080P_30fps
        {
            LOG_I("AHD_1080P_30HZ");
            return AHD_1080P_30HZ;
        }
    } else {
        LOG_I("No camera found");
    }
    return AHD_NOSIGNAL;
}
#endif
static bool n5_is_open(struct n5_dev *sensor)
{
    return sensor->on;
}

static void n5_power_on(struct n5_dev *sensor)
{
    if (sensor->on)
        return;

    camera_pin_set_high(sensor->rst_pin);
    aicos_msleep(300);
    camera_pin_set_low(sensor->rst_pin);
    aicos_msleep(300);
    camera_pin_set_high(sensor->rst_pin);
    aicos_msleep(300);

    LOG_I("Power on");
    sensor->on = true;
}

static void n5_power_off(struct n5_dev *sensor)
{
    if (!sensor->on)
        return;

    if (sensor->rst_pin)
        camera_pin_set_low(sensor->rst_pin);

    LOG_I("Power off");
    sensor->on = false;
}

static int n5_probe(struct n5_dev *sensor)
{
    u8 id = 0;
    int Chn = 0;

    n5_power_on(sensor);
    n5_write_reg(sensor->i2c, 0xff, 0);
    id = n5_read_reg(sensor->i2c, 0xf4);
    if (id != N5_CHIP_ID) {
        LOG_E("Invalid chip ID: %02x\n", id);
        return -1;
    }

    n5_init_common(sensor);
    n5_video_format_unknown(sensor, 0);
    n5_video_format_unknown(sensor, 1);
    n5_auto_detect_mode(sensor);
    for(Chn = 0; Chn < 2; Chn++)
    {
        n5_write_reg(sensor->i2c, 0xff, 0x13);
        n5_write_reg(sensor->i2c, 0x70, n5_read_reg(sensor->i2c, 0x70)|(0x01<<Chn));
        n5_write_reg(sensor->i2c, 0x71, n5_read_reg(sensor->i2c, 0x71)|(0x01<<Chn));
        n5_set_chn_1080p_25(sensor, Chn);
    }
#if N5_USE_RES_DETECT
    if (n5_resolutiondetect(sensor, N5_DFT_CHAN) == AHD_NOSIGNAL)
        return -1;
#endif
    aicos_msleep(200);
    n5_selchannel_portmode(sensor, 0, N5_OutMode_FHD_Input_1_Output_1);
    aic_mdelay(200);
    return 0;
}

static rt_err_t n5_init(rt_device_t dev)
{
    struct n5_dev *sensor = (struct n5_dev *)dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->fmt.code   = N5_DFT_CODE;
    sensor->fmt.width  = N5_DFT_WIDTH;
    sensor->fmt.height = N5_DFT_HEIGHT;
    sensor->fmt.bus_type = N5_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;//|
                        //MEDIA_SIGNAL_INTERLACED_MODE;

    sensor->rst_pin = camera_rst_pin_get();
    if (!sensor->rst_pin)
        return -RT_EINVAL;

    return RT_EOK;
}

static rt_err_t n5_open(rt_device_t dev, rt_uint16_t oflag)
{
    sensor = (struct n5_dev *)dev;
    if (n5_is_open(sensor))
        return RT_EOK;

    if (n5_probe(sensor))
        return -RT_ERROR;

    LOG_I("Open N5");
    return RT_EOK;
}

static rt_err_t n5_close(rt_device_t dev)
{
    struct n5_dev *sensor = (struct n5_dev *)dev;

    if (!n5_is_open(sensor))
        return -RT_ERROR;

    n5_power_off(sensor);
    LOG_D("Close N5");
    return RT_EOK;
}

static int n5_get_fmt(struct n5_dev *sensor, struct mpp_video_fmt *cfg)
{
    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int n5_start(rt_device_t dev)
{
    return 0;
}

static int n5_stop(rt_device_t dev)
{
    return 0;
}

static int n5_pause(rt_device_t dev)
{
    return 0;
}

static int n5_resume(rt_device_t dev)
{
    return 0;
}

static int n5_set_brightness(struct n5_dev *sensor, u32 percent)
{
    return 0;
}

static int n5_set_saturation(struct n5_dev *sensor, u32 percent)
{
    return 0;
}

static int n5_set_hue(struct n5_dev *sensor, u32 percent)
{
    return 0;
}

static int n5_set_contrast(struct n5_dev *sensor, u32 percent)
{
    return 0;
}

static rt_err_t n5_control(rt_device_t dev, int cmd, void *args)
{
    struct n5_dev *sensor = (struct n5_dev *)dev;

    switch (cmd) {
    case CAMERA_CMD_START:
        return n5_start(dev);
    case CAMERA_CMD_STOP:
        return n5_stop(dev);
    case CAMERA_CMD_PAUSE:
        return n5_pause(dev);
    case CAMERA_CMD_RESUME:
        return n5_resume(dev);
    case CAMERA_CMD_GET_FMT:
        return n5_get_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_CONTRAST:
        return n5_set_contrast(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_BRIGHTNESS:
        return n5_set_brightness(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_HUE:
        return n5_set_hue(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_SATURATION:
        return n5_set_saturation(sensor, *(u32 *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops n5_ops =
{
    .init = n5_init,
    .open = n5_open,
    .close = n5_close,
    .control = n5_control,
};
#endif

int rt_hw_n5_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_n5_dev.dev.ops = &n5_ops;
#else
    g_n5_dev.dev.init = n5_init;
    g_n5_dev.dev.open = n5_open;
    g_n5_dev.dev.close = n5_close;
    g_n5_dev.dev.control = n5_control;
#endif
    g_n5_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_n5_dev.dev, CAMERA_DEV_NAME, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_n5_init);
