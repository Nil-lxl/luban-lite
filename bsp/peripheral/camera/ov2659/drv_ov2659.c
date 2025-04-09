/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "ov2659"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of OV2659 */
#define OV2659_DFT_FRAMESIZE    OV2659_SIZE_VGA
#define OV2659_DFT_BUS_TYPE     MEDIA_BUS_PARALLEL
#define OV2659_DFT_CODE         1
#define OV2659_DFT_LINK_FREQ    70000000

/* The clk source is decided by board design */
#define CAMERA_CLK_SRC          CLK_OUT1

#define OV2659_I2C_SLAVE_ID     0x30

/* OV2659 register definitions */
#define REG_SOFTWARE_STANDBY    0x0100
#define REG_SOFTWARE_RESET      0x0103
#define REG_IO_CTRL00           0x3000
#define REG_IO_CTRL01           0x3001
#define REG_IO_CTRL02           0x3002
#define REG_OUTPUT_VALUE00      0x3008
#define REG_OUTPUT_VALUE01      0x3009
#define REG_OUTPUT_VALUE02      0x300d
#define REG_OUTPUT_SELECT00     0x300e
#define REG_OUTPUT_SELECT01     0x300f
#define REG_OUTPUT_SELECT02     0x3010
#define REG_OUTPUT_DRIVE        0x3011
#define REG_INPUT_READOUT00     0x302d
#define REG_INPUT_READOUT01     0x302e
#define REG_INPUT_READOUT02     0x302f

#define REG_SC_PLL_CTRL0        0x3003
#define REG_SC_PLL_CTRL1        0x3004
#define REG_SC_PLL_CTRL2        0x3005
#define REG_SC_PLL_CTRL3        0x3006
#define REG_SC_CHIP_ID_H        0x300a
#define REG_SC_CHIP_ID_L        0x300b
#define REG_SC_PWC              0x3014
#define REG_SC_CLKRST0          0x301a
#define REG_SC_CLKRST1          0x301b
#define REG_SC_CLKRST2          0x301c
#define REG_SC_CLKRST3          0x301d
#define REG_SC_SUB_ID           0x302a
#define REG_SC_SCCB_ID          0x302b

#define REG_GROUP_ADDRESS_00    0x3200
#define REG_GROUP_ADDRESS_01    0x3201
#define REG_GROUP_ADDRESS_02    0x3202
#define REG_GROUP_ADDRESS_03    0x3203
#define REG_GROUP_ACCESS        0x3208

#define REG_AWB_R_GAIN_H        0x3400
#define REG_AWB_R_GAIN_L        0x3401
#define REG_AWB_G_GAIN_H        0x3402
#define REG_AWB_G_GAIN_L        0x3403
#define REG_AWB_B_GAIN_H        0x3404
#define REG_AWB_B_GAIN_L        0x3405
#define REG_AWB_MANUAL_CONTROL  0x3406

#define REG_TIMING_HS_H         0x3800
#define REG_TIMING_HS_L         0x3801
#define REG_TIMING_VS_H         0x3802
#define REG_TIMING_VS_L         0x3803
#define REG_TIMING_HW_H         0x3804
#define REG_TIMING_HW_L         0x3805
#define REG_TIMING_VH_H         0x3806
#define REG_TIMING_VH_L         0x3807
#define REG_TIMING_DVPHO_H      0x3808
#define REG_TIMING_DVPHO_L      0x3809
#define REG_TIMING_DVPVO_H      0x380a
#define REG_TIMING_DVPVO_L      0x380b
#define REG_TIMING_HTS_H        0x380c
#define REG_TIMING_HTS_L        0x380d
#define REG_TIMING_VTS_H        0x380e
#define REG_TIMING_VTS_L        0x380f
#define REG_TIMING_HOFFS_H      0x3810
#define REG_TIMING_HOFFS_L      0x3811
#define REG_TIMING_VOFFS_H      0x3812
#define REG_TIMING_VOFFS_L      0x3813
#define REG_TIMING_XINC         0x3814
#define REG_TIMING_YINC         0x3815
#define REG_TIMING_VERT_FORMAT  0x3820
#define REG_TIMING_HORIZ_FORMAT 0x3821

#define REG_FORMAT_CTRL00       0x4300

#define REG_VFIFO_READ_START_H  0x4608
#define REG_VFIFO_READ_START_L  0x4609

#define REG_DVP_CTRL02          0x4708

#define REG_ISP_CTRL00          0x5000
#define REG_ISP_CTRL01          0x5001
#define REG_ISP_CTRL02          0x5002

#define REG_LENC_RED_X0_H       0x500c
#define REG_LENC_RED_X0_L       0x500d
#define REG_LENC_RED_Y0_H       0x500e
#define REG_LENC_RED_Y0_L       0x500f
#define REG_LENC_RED_A1         0x5010
#define REG_LENC_RED_B1         0x5011
#define REG_LENC_RED_A2_B2      0x5012
#define REG_LENC_GREEN_X0_H     0x5013
#define REG_LENC_GREEN_X0_L     0x5014
#define REG_LENC_GREEN_Y0_H     0x5015
#define REG_LENC_GREEN_Y0_L     0x5016
#define REG_LENC_GREEN_A1       0x5017
#define REG_LENC_GREEN_B1       0x5018
#define REG_LENC_GREEN_A2_B2    0x5019
#define REG_LENC_BLUE_X0_H      0x501a
#define REG_LENC_BLUE_X0_L      0x501b
#define REG_LENC_BLUE_Y0_H      0x501c
#define REG_LENC_BLUE_Y0_L      0x501d
#define REG_LENC_BLUE_A1        0x501e
#define REG_LENC_BLUE_B1        0x501f
#define REG_LENC_BLUE_A2_B2     0x5020

#define REG_AWB_CTRL00          0x5035
#define REG_AWB_CTRL01          0x5036
#define REG_AWB_CTRL02          0x5037
#define REG_AWB_CTRL03          0x5038
#define REG_AWB_CTRL04          0x5039
#define REG_AWB_LOCAL_LIMIT     0x503a
#define REG_AWB_CTRL12          0x5049
#define REG_AWB_CTRL13          0x504a
#define REG_AWB_CTRL14          0x504b

#define REG_SHARPENMT_THRESH1   0x5064
#define REG_SHARPENMT_THRESH2   0x5065
#define REG_SHARPENMT_OFFSET1   0x5066
#define REG_SHARPENMT_OFFSET2   0x5067
#define REG_DENOISE_THRESH1     0x5068
#define REG_DENOISE_THRESH2     0x5069
#define REG_DENOISE_OFFSET1     0x506a
#define REG_DENOISE_OFFSET2     0x506b
#define REG_SHARPEN_THRESH1     0x506c
#define REG_SHARPEN_THRESH2     0x506d
#define REG_CIP_CTRL00          0x506e
#define REG_CIP_CTRL01          0x506f

#define REG_CMX_SIGN            0x5079
#define REG_CMX_MISC_CTRL       0x507a

#define REG_PRE_ISP_CTRL00      0x50a0
#define TEST_PATTERN_ENABLE     BIT(7)
#define VERTICAL_COLOR_BAR_MASK 0x53

#define REG_NULL                0x0000  /* Array end token */

#define OV265X_ID(_msb, _lsb)   ((_msb) << 8 | (_lsb))
#define OV2659_ID               0x2656

enum ov2659_size {
    OV2659_SIZE_QVGA,
    OV2659_SIZE_VGA,
    OV2659_SIZE_SVGA,
    OV2659_SIZE_XGA,
    OV2659_SIZE_720P,
    OV2659_SIZE_SXGA,
    OV2659_SIZE_UXGA,
};

struct ov2659_framesize {
    u16 width;
    u16 height;
    u16 max_exp_lines;
    const struct reg16_info *regs;
};

struct ov2659_pll_ctrl {
    u8 ctrl1;
    u8 ctrl2;
    u8 ctrl3;
};

struct ov2659_pixfmt {
    u32 code;
    /* Output format Register Value (REG_FORMAT_CTRL00) */
    struct reg16_info *format_ctrl_regs;
};

struct pll_ctrl_reg {
    unsigned int div;
    unsigned char reg;
};

struct ov2659_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 xclk_freq;
    u32 rst_pin;
    u32 pwdn_pin;

    u32 link_freq;
    struct mpp_video_fmt fmt;
    const struct ov2659_framesize *frame_size;
    struct reg16_info *format_ctrl_regs;
    struct ov2659_pll_ctrl pll;

    bool streaming;
};

static struct ov2659_dev g_ov2659_dev = {0};

static const struct reg16_info ov2659_init_regs[] = {
    { REG_IO_CTRL00, 0x03 },
    { REG_IO_CTRL01, 0xff },
    { REG_IO_CTRL02, 0xe0 },
    { 0x3633, 0x3d },
    { 0x3620, 0x02 },
    { 0x3631, 0x11 },
    { 0x3612, 0x04 },
    { 0x3630, 0x20 },
    { 0x4702, 0x02 },
    { 0x370c, 0x34 },
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xb7 },
    { REG_TIMING_DVPHO_H, 0x03 },
    { REG_TIMING_DVPHO_L, 0x20 },
    { REG_TIMING_DVPVO_H, 0x02 },
    { REG_TIMING_DVPVO_L, 0x58 },
    { REG_TIMING_HTS_H, 0x05 },
    { REG_TIMING_HTS_L, 0x14 },
    { REG_TIMING_VTS_H, 0x02 },
    { REG_TIMING_VTS_L, 0x68 },
    { REG_TIMING_HOFFS_L, 0x08 },
    { REG_TIMING_VOFFS_L, 0x02 },
    { REG_TIMING_XINC, 0x31 },
    { REG_TIMING_YINC, 0x31 },
    { 0x3a02, 0x02 },
    { 0x3a03, 0x68 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0x5c },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x4d },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x02 },
    { 0x3a15, 0x28 },
    { REG_DVP_CTRL02, 0x01 },
    { 0x3623, 0x00 },
    { 0x3634, 0x76 },
    { 0x3701, 0x44 },
    { 0x3702, 0x18 },
    { 0x3703, 0x24 },
    { 0x3704, 0x24 },
    { 0x3705, 0x0c },
    { REG_TIMING_VERT_FORMAT, 0x81 },
    { REG_TIMING_HORIZ_FORMAT, 0x01 },
    { 0x370a, 0x52 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0x80 },
    { REG_FORMAT_CTRL00, 0x30 },
    { 0x5086, 0x02 },
    { REG_ISP_CTRL00, 0xfb },
    { REG_ISP_CTRL01, 0x1f },
    { REG_ISP_CTRL02, 0x00 },
    { 0x5025, 0x0e },
    { 0x5026, 0x18 },
    { 0x5027, 0x34 },
    { 0x5028, 0x4c },
    { 0x5029, 0x62 },
    { 0x502a, 0x74 },
    { 0x502b, 0x85 },
    { 0x502c, 0x92 },
    { 0x502d, 0x9e },
    { 0x502e, 0xb2 },
    { 0x502f, 0xc0 },
    { 0x5030, 0xcc },
    { 0x5031, 0xe0 },
    { 0x5032, 0xee },
    { 0x5033, 0xf6 },
    { 0x5034, 0x11 },
    { 0x5070, 0x1c },
    { 0x5071, 0x5b },
    { 0x5072, 0x05 },
    { 0x5073, 0x20 },
    { 0x5074, 0x94 },
    { 0x5075, 0xb4 },
    { 0x5076, 0xb4 },
    { 0x5077, 0xaf },
    { 0x5078, 0x05 },
    { REG_CMX_SIGN, 0x98 },
    { REG_CMX_MISC_CTRL, 0x21 },
    { REG_AWB_CTRL00, 0x6a },
    { REG_AWB_CTRL01, 0x11 },
    { REG_AWB_CTRL02, 0x92 },
    { REG_AWB_CTRL03, 0x21 },
    { REG_AWB_CTRL04, 0xe1 },
    { REG_AWB_LOCAL_LIMIT, 0x01 },
    { 0x503c, 0x05 },
    { 0x503d, 0x08 },
    { 0x503e, 0x08 },
    { 0x503f, 0x64 },
    { 0x5040, 0x58 },
    { 0x5041, 0x2a },
    { 0x5042, 0xc5 },
    { 0x5043, 0x2e },
    { 0x5044, 0x3a },
    { 0x5045, 0x3c },
    { 0x5046, 0x44 },
    { 0x5047, 0xf8 },
    { 0x5048, 0x08 },
    { REG_AWB_CTRL12, 0x70 },
    { REG_AWB_CTRL13, 0xf0 },
    { REG_AWB_CTRL14, 0xf0 },
    { REG_LENC_RED_X0_H, 0x03 },
    { REG_LENC_RED_X0_L, 0x20 },
    { REG_LENC_RED_Y0_H, 0x02 },
    { REG_LENC_RED_Y0_L, 0x5c },
    { REG_LENC_RED_A1, 0x48 },
    { REG_LENC_RED_B1, 0x00 },
    { REG_LENC_RED_A2_B2, 0x66 },
    { REG_LENC_GREEN_X0_H, 0x03 },
    { REG_LENC_GREEN_X0_L, 0x30 },
    { REG_LENC_GREEN_Y0_H, 0x02 },
    { REG_LENC_GREEN_Y0_L, 0x7c },
    { REG_LENC_GREEN_A1, 0x40 },
    { REG_LENC_GREEN_B1, 0x00 },
    { REG_LENC_GREEN_A2_B2, 0x66 },
    { REG_LENC_BLUE_X0_H, 0x03 },
    { REG_LENC_BLUE_X0_L, 0x10 },
    { REG_LENC_BLUE_Y0_H, 0x02 },
    { REG_LENC_BLUE_Y0_L, 0x7c },
    { REG_LENC_BLUE_A1, 0x3a },
    { REG_LENC_BLUE_B1, 0x00 },
    { REG_LENC_BLUE_A2_B2, 0x66 },
    { REG_CIP_CTRL00, 0x44 },
    { REG_SHARPENMT_THRESH1, 0x08 },
    { REG_SHARPENMT_THRESH2, 0x10 },
    { REG_SHARPENMT_OFFSET1, 0x12 },
    { REG_SHARPENMT_OFFSET2, 0x02 },
    { REG_SHARPEN_THRESH1, 0x08 },
    { REG_SHARPEN_THRESH2, 0x10 },
    { REG_CIP_CTRL01, 0xa6 },
    { REG_DENOISE_THRESH1, 0x08 },
    { REG_DENOISE_THRESH2, 0x10 },
    { REG_DENOISE_OFFSET1, 0x04 },
    { REG_DENOISE_OFFSET2, 0x12 },
    { 0x507e, 0x40 },
    { 0x507f, 0x20 },
    { 0x507b, 0x02 },
    { REG_CMX_MISC_CTRL, 0x01 },
    { 0x5084, 0x0c },
    { 0x5085, 0x3e },
    { 0x5005, 0x80 },
    { 0x3a0f, 0x30 },
    { 0x3a10, 0x28 },
    { 0x3a1b, 0x32 },
    { 0x3a1e, 0x26 },
    { 0x3a11, 0x60 },
    { 0x3a1f, 0x14 },
    { 0x5060, 0x69 },
    { 0x5061, 0x7d },
    { 0x5062, 0x7d },
    { 0x5063, 0x69 },
    { REG_NULL, 0x00 },
};

/* 1280X720 720p */
static struct reg16_info ov2659_720p[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0xa0 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0xf0 },
    { REG_TIMING_HW_H, 0x05 },
    { REG_TIMING_HW_L, 0xbf },
    { REG_TIMING_VH_H, 0x03 },
    { REG_TIMING_VH_L, 0xcb },
    { REG_TIMING_DVPHO_H, 0x05 },
    { REG_TIMING_DVPHO_L, 0x00 },
    { REG_TIMING_DVPVO_H, 0x02 },
    { REG_TIMING_DVPVO_L, 0xd0 },
    { REG_TIMING_HTS_H, 0x06 },
    { REG_TIMING_HTS_L, 0x4c },
    { REG_TIMING_VTS_H, 0x02 },
    { REG_TIMING_VTS_L, 0xe8 },
    { REG_TIMING_HOFFS_L, 0x10 },
    { REG_TIMING_VOFFS_L, 0x06 },
    { REG_TIMING_XINC, 0x11 },
    { REG_TIMING_YINC, 0x11 },
    { REG_TIMING_VERT_FORMAT, 0x80 },
    { REG_TIMING_HORIZ_FORMAT, 0x00 },
    { 0x370a, 0x12 },
    { 0x3a03, 0xe8 },
    { 0x3a09, 0x6f },
    { 0x3a0b, 0x5d },
    { 0x3a15, 0x9a },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0x80 },
    { REG_ISP_CTRL02, 0x00 },
    { REG_NULL, 0x00 },
};

/* 1600X1200 UXGA */
static struct reg16_info ov2659_uxga[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xbb },
    { REG_TIMING_DVPHO_H, 0x06 },
    { REG_TIMING_DVPHO_L, 0x40 },
    { REG_TIMING_DVPVO_H, 0x04 },
    { REG_TIMING_DVPVO_L, 0xb0 },
    { REG_TIMING_HTS_H, 0x07 },
    { REG_TIMING_HTS_L, 0x9f },
    { REG_TIMING_VTS_H, 0x04 },
    { REG_TIMING_VTS_L, 0xd0 },
    { REG_TIMING_HOFFS_L, 0x10 },
    { REG_TIMING_VOFFS_L, 0x06 },
    { REG_TIMING_XINC, 0x11 },
    { REG_TIMING_YINC, 0x11 },
    { 0x3a02, 0x04 },
    { 0x3a03, 0xd0 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0xb8 },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x9a },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x04 },
    { 0x3a15, 0x50 },
    { 0x3623, 0x00 },
    { 0x3634, 0x44 },
    { 0x3701, 0x44 },
    { 0x3702, 0x30 },
    { 0x3703, 0x48 },
    { 0x3704, 0x48 },
    { 0x3705, 0x18 },
    { REG_TIMING_VERT_FORMAT, 0x80 },
    { REG_TIMING_HORIZ_FORMAT, 0x00 },
    { 0x370a, 0x12 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0x80 },
    { REG_ISP_CTRL02, 0x00 },
    { REG_NULL, 0x00 },
};

/* 1280X1024 SXGA */
static struct reg16_info ov2659_sxga[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xb7 },
    { REG_TIMING_DVPHO_H, 0x05 },
    { REG_TIMING_DVPHO_L, 0x00 },
    { REG_TIMING_DVPVO_H, 0x04 },
    { REG_TIMING_DVPVO_L, 0x00 },
    { REG_TIMING_HTS_H, 0x07 },
    { REG_TIMING_HTS_L, 0x9c },
    { REG_TIMING_VTS_H, 0x04 },
    { REG_TIMING_VTS_L, 0xd0 },
    { REG_TIMING_HOFFS_L, 0x10 },
    { REG_TIMING_VOFFS_L, 0x06 },
    { REG_TIMING_XINC, 0x11 },
    { REG_TIMING_YINC, 0x11 },
    { 0x3a02, 0x02 },
    { 0x3a03, 0x68 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0x5c },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x4d },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x02 },
    { 0x3a15, 0x28 },
    { 0x3623, 0x00 },
    { 0x3634, 0x76 },
    { 0x3701, 0x44 },
    { 0x3702, 0x18 },
    { 0x3703, 0x24 },
    { 0x3704, 0x24 },
    { 0x3705, 0x0c },
    { REG_TIMING_VERT_FORMAT, 0x80 },
    { REG_TIMING_HORIZ_FORMAT, 0x00 },
    { 0x370a, 0x52 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0x80 },
    { REG_ISP_CTRL02, 0x00 },
    { REG_NULL, 0x00 },
};

/* 1024X768 SXGA */
static struct reg16_info ov2659_xga[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xb7 },
    { REG_TIMING_DVPHO_H, 0x04 },
    { REG_TIMING_DVPHO_L, 0x00 },
    { REG_TIMING_DVPVO_H, 0x03 },
    { REG_TIMING_DVPVO_L, 0x00 },
    { REG_TIMING_HTS_H, 0x07 },
    { REG_TIMING_HTS_L, 0x9c },
    { REG_TIMING_VTS_H, 0x04 },
    { REG_TIMING_VTS_L, 0xd0 },
    { REG_TIMING_HOFFS_L, 0x10 },
    { REG_TIMING_VOFFS_L, 0x06 },
    { REG_TIMING_XINC, 0x11 },
    { REG_TIMING_YINC, 0x11 },
    { 0x3a02, 0x02 },
    { 0x3a03, 0x68 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0x5c },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x4d },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x02 },
    { 0x3a15, 0x28 },
    { 0x3623, 0x00 },
    { 0x3634, 0x76 },
    { 0x3701, 0x44 },
    { 0x3702, 0x18 },
    { 0x3703, 0x24 },
    { 0x3704, 0x24 },
    { 0x3705, 0x0c },
    { REG_TIMING_VERT_FORMAT, 0x80 },
    { REG_TIMING_HORIZ_FORMAT, 0x00 },
    { 0x370a, 0x52 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0x80 },
    { REG_ISP_CTRL02, 0x00 },
    { REG_NULL, 0x00 },
};

/* 800X600 SVGA */
static struct reg16_info ov2659_svga[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xb7 },
    { REG_TIMING_DVPHO_H, 0x03 },
    { REG_TIMING_DVPHO_L, 0x20 },
    { REG_TIMING_DVPVO_H, 0x02 },
    { REG_TIMING_DVPVO_L, 0x58 },
    { REG_TIMING_HTS_H, 0x05 },
    { REG_TIMING_HTS_L, 0x14 },
    { REG_TIMING_VTS_H, 0x02 },
    { REG_TIMING_VTS_L, 0x68 },
    { REG_TIMING_HOFFS_L, 0x08 },
    { REG_TIMING_VOFFS_L, 0x02 },
    { REG_TIMING_XINC, 0x31 },
    { REG_TIMING_YINC, 0x31 },
    { 0x3a02, 0x02 },
    { 0x3a03, 0x68 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0x5c },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x4d },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x02 },
    { 0x3a15, 0x28 },
    { 0x3623, 0x00 },
    { 0x3634, 0x76 },
    { 0x3701, 0x44 },
    { 0x3702, 0x18 },
    { 0x3703, 0x24 },
    { 0x3704, 0x24 },
    { 0x3705, 0x0c },
    { REG_TIMING_VERT_FORMAT, 0x81 },
    { REG_TIMING_HORIZ_FORMAT, 0x01 },
    { 0x370a, 0x52 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0x80 },
    { REG_ISP_CTRL02, 0x00 },
    { REG_NULL, 0x00 },
};

/* 640X480 VGA */
static struct reg16_info ov2659_vga[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xb7 },
    { REG_TIMING_DVPHO_H, 0x02 },
    { REG_TIMING_DVPHO_L, 0x80 },
    { REG_TIMING_DVPVO_H, 0x01 },
    { REG_TIMING_DVPVO_L, 0xe0 },
    { REG_TIMING_HTS_H, 0x05 },
    { REG_TIMING_HTS_L, 0x14 },
    { REG_TIMING_VTS_H, 0x02 },
    { REG_TIMING_VTS_L, 0x68 },
    { REG_TIMING_HOFFS_L, 0x08 },
    { REG_TIMING_VOFFS_L, 0x02 },
    { REG_TIMING_XINC, 0x31 },
    { REG_TIMING_YINC, 0x31 },
    { 0x3a02, 0x02 },
    { 0x3a03, 0x68 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0x5c },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x4d },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x02 },
    { 0x3a15, 0x28 },
    { 0x3623, 0x00 },
    { 0x3634, 0x76 },
    { 0x3701, 0x44 },
    { 0x3702, 0x18 },
    { 0x3703, 0x24 },
    { 0x3704, 0x24 },
    { 0x3705, 0x0c },
    { REG_TIMING_VERT_FORMAT, 0x81 },
    { REG_TIMING_HORIZ_FORMAT, 0x01 },
    { 0x370a, 0x52 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0xa0 },
    { REG_ISP_CTRL02, 0x10 },
    { REG_NULL, 0x00 },
};

/* 320X240 QVGA */
static  struct reg16_info ov2659_qvga[] = {
    { REG_TIMING_HS_H, 0x00 },
    { REG_TIMING_HS_L, 0x00 },
    { REG_TIMING_VS_H, 0x00 },
    { REG_TIMING_VS_L, 0x00 },
    { REG_TIMING_HW_H, 0x06 },
    { REG_TIMING_HW_L, 0x5f },
    { REG_TIMING_VH_H, 0x04 },
    { REG_TIMING_VH_L, 0xb7 },
    { REG_TIMING_DVPHO_H, 0x01 },
    { REG_TIMING_DVPHO_L, 0x40 },
    { REG_TIMING_DVPVO_H, 0x00 },
    { REG_TIMING_DVPVO_L, 0xf0 },
    { REG_TIMING_HTS_H, 0x05 },
    { REG_TIMING_HTS_L, 0x14 },
    { REG_TIMING_VTS_H, 0x02 },
    { REG_TIMING_VTS_L, 0x68 },
    { REG_TIMING_HOFFS_L, 0x08 },
    { REG_TIMING_VOFFS_L, 0x02 },
    { REG_TIMING_XINC, 0x31 },
    { REG_TIMING_YINC, 0x31 },
    { 0x3a02, 0x02 },
    { 0x3a03, 0x68 },
    { 0x3a08, 0x00 },
    { 0x3a09, 0x5c },
    { 0x3a0a, 0x00 },
    { 0x3a0b, 0x4d },
    { 0x3a0d, 0x08 },
    { 0x3a0e, 0x06 },
    { 0x3a14, 0x02 },
    { 0x3a15, 0x28 },
    { 0x3623, 0x00 },
    { 0x3634, 0x76 },
    { 0x3701, 0x44 },
    { 0x3702, 0x18 },
    { 0x3703, 0x24 },
    { 0x3704, 0x24 },
    { 0x3705, 0x0c },
    { REG_TIMING_VERT_FORMAT, 0x81 },
    { REG_TIMING_HORIZ_FORMAT, 0x01 },
    { 0x370a, 0x52 },
    { REG_VFIFO_READ_START_H, 0x00 },
    { REG_VFIFO_READ_START_L, 0xa0 },
    { REG_ISP_CTRL02, 0x10 },
    { REG_NULL, 0x00 },
};

static const struct pll_ctrl_reg ctrl3[] = {
    { 1, 0x00 },
    { 2, 0x02 },
    { 3, 0x03 },
    { 4, 0x06 },
    { 6, 0x0d },
    { 8, 0x0e },
    { 12, 0x0f },
    { 16, 0x12 },
    { 24, 0x13 },
    { 32, 0x16 },
    { 48, 0x1b },
    { 64, 0x1e },
    { 96, 0x1f },
    { 0, 0x00 },
};

static const struct pll_ctrl_reg ctrl1[] = {
    { 2, 0x10 },
    { 4, 0x20 },
    { 6, 0x30 },
    { 8, 0x40 },
    { 10, 0x50 },
    { 12, 0x60 },
    { 14, 0x70 },
    { 16, 0x80 },
    { 18, 0x90 },
    { 20, 0xa0 },
    { 22, 0xb0 },
    { 24, 0xc0 },
    { 26, 0xd0 },
    { 28, 0xe0 },
    { 30, 0xf0 },
    { 0, 0x00 },
};

static const struct ov2659_framesize ov2659_framesizes[] = {
    { /* QVGA */
        .width      = 320,
        .height     = 240,
        .regs       = ov2659_qvga,
        .max_exp_lines  = 248,
    }, { /* VGA */
        .width      = 640,
        .height     = 480,
        .regs       = ov2659_vga,
        .max_exp_lines  = 498,
    }, { /* SVGA */
        .width      = 800,
        .height     = 600,
        .regs       = ov2659_svga,
        .max_exp_lines  = 498,
    }, { /* XGA */
        .width      = 1024,
        .height     = 768,
        .regs       = ov2659_xga,
        .max_exp_lines  = 498,
    }, { /* 720P */
        .width      = 1280,
        .height     = 720,
        .regs       = ov2659_720p,
        .max_exp_lines  = 498,
    }, { /* SXGA */
        .width      = 1280,
        .height     = 1024,
        .regs       = ov2659_sxga,
        .max_exp_lines  = 1048,
    }, { /* UXGA */
        .width      = 1600,
        .height     = 1200,
        .regs       = ov2659_uxga,
        .max_exp_lines  = 498,
    },
};

/* YUV422 YUYV*/
static struct reg16_info ov2659_format_yuyv[] = {
    { REG_FORMAT_CTRL00, 0x30 },
    { REG_NULL, 0x0 },
};

/* YUV422 UYVY  */
static struct reg16_info ov2659_format_uyvy[] = {
    { REG_FORMAT_CTRL00, 0x32 },
    { REG_NULL, 0x0 },
};

/* Raw Bayer BGGR */
static struct reg16_info ov2659_format_bggr[] = {
    { REG_FORMAT_CTRL00, 0x00 },
    { REG_NULL, 0x0 },
};

/* RGB565 */
static struct reg16_info ov2659_format_rgb565[] = {
    { REG_FORMAT_CTRL00, 0x60 },
    { REG_NULL, 0x0 },
};

static const struct ov2659_pixfmt ov2659_formats[] = {
    {
        .code = MEDIA_BUS_FMT_YUYV8_2X8,
        .format_ctrl_regs = ov2659_format_yuyv,
    }, {
        .code = MEDIA_BUS_FMT_UYVY8_2X8,
        .format_ctrl_regs = ov2659_format_uyvy,
    }, {
        .code = MEDIA_BUS_FMT_RGB565_2X8_BE,
        .format_ctrl_regs = ov2659_format_rgb565,
    }, {
        .code = MEDIA_BUS_FMT_SBGGR8_1X8,
        .format_ctrl_regs = ov2659_format_bggr,
    },
};

static int ov2659_write_reg(struct rt_i2c_bus_device *i2c, u16 reg, u8 val)
{
    if (rt_i2c_write_reg16(i2c, OV2659_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int ov2659_read_reg(struct rt_i2c_bus_device *i2c, u16 reg, u8 *val)
{
    if (rt_i2c_read_reg16(i2c, OV2659_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static int ov2659_write_array(struct rt_i2c_bus_device *i2c,
                              const struct reg16_info *regs)
{
    int i, ret = 0;

    for (i = 0; ret == 0 && regs[i].reg; i++)
        ret = ov2659_write_reg(i2c, regs[i].reg, regs[i].val);

    return ret;
}

static void ov2659_pll_calc_params(struct ov2659_dev *sensor)
{
    u8 ctrl1_reg = 0, ctrl2_reg = 0, ctrl3_reg = 0;
    unsigned int desired = sensor->link_freq;
    u32 prediv, postdiv, mult;
    u32 bestdelta = -1;
    u32 delta, actual;
    int i, j;

    for (i = 0; ctrl1[i].div != 0; i++) {
        postdiv = ctrl1[i].div;
        for (j = 0; ctrl3[j].div != 0; j++) {
            prediv = ctrl3[j].div;
            for (mult = 1; mult <= 63; mult++) {
                actual  = sensor->xclk_freq;
                actual *= mult;
                actual /= prediv;
                actual /= postdiv;
                delta = actual - desired;
                delta = abs(delta);

                if ((delta < bestdelta) || (bestdelta == -1)) {
                    bestdelta = delta;
                    ctrl1_reg = ctrl1[i].reg;
                    ctrl2_reg = mult;
                    ctrl3_reg = ctrl3[j].reg;
                }
            }
        }
    }

    sensor->pll.ctrl1 = ctrl1_reg;
    sensor->pll.ctrl2 = ctrl2_reg;
    sensor->pll.ctrl3 = ctrl3_reg;

    LOG_I("Actual reg config: ctrl1: %02x ctrl2: %02x ctrl3: %02x\n",
          ctrl1_reg, ctrl2_reg, ctrl3_reg);
}

static int ov2659_set_pixel_clock(struct ov2659_dev *sensor)
{
    struct reg16_info pll_regs[] = {
        {REG_SC_PLL_CTRL1, sensor->pll.ctrl1},
        {REG_SC_PLL_CTRL2, sensor->pll.ctrl2},
        {REG_SC_PLL_CTRL3, sensor->pll.ctrl3},
        {REG_NULL, 0x00},
    };

    return ov2659_write_array(sensor->i2c, pll_regs);
};

static void ov2659_get_default_format(struct mpp_video_fmt *fmt)
{
    fmt->width      = ov2659_framesizes[OV2659_DFT_FRAMESIZE].width;
    fmt->height     = ov2659_framesizes[OV2659_DFT_FRAMESIZE].height;
    fmt->code       = ov2659_formats[OV2659_DFT_CODE].code;
    fmt->bus_type   = OV2659_DFT_BUS_TYPE;
    fmt->flags      = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;
}

static void ov2659_set_streaming(struct ov2659_dev *sensor, int on)
{
    int ret;

    on = !!on;

    LOG_D("%s: on: %d\n", __func__, on);
    ret = ov2659_write_reg(sensor->i2c, REG_SOFTWARE_STANDBY, on);
    if (ret)
        LOG_E("ov2659 soft standby failed\n");
}

static int ov2659_base_init(struct ov2659_dev *sensor)
{
    return ov2659_write_array(sensor->i2c, ov2659_init_regs);
}

static int ov2659_set_frame_size(struct ov2659_dev *sensor)
{
    return ov2659_write_array(sensor->i2c, sensor->frame_size->regs);
}

static int ov2659_set_format(struct ov2659_dev *sensor)
{
    return ov2659_write_array(sensor->i2c, sensor->format_ctrl_regs);
}

static int ov2659_s_stream(struct ov2659_dev *sensor, int on)
{
    int ret = 0;

    LOG_D("%s: on: %d\n", __func__, on);

    on = !!on;

    if (sensor->streaming == on)
        goto out;

    if (!on) {
        /* Stop Streaming Sequence */
        ov2659_set_streaming(sensor, 0);
        sensor->streaming = on;
        goto out;
    }

    ret = ov2659_base_init(sensor);
    if (!ret)
        ret = ov2659_set_pixel_clock(sensor);
    if (!ret)
        ret = ov2659_set_frame_size(sensor);
    if (!ret)
        ret = ov2659_set_format(sensor);
    if (!ret) {
        ov2659_set_streaming(sensor, 1);
        sensor->streaming = on;
    }

out:
    return ret;
}

static int ov2659_power_off(struct ov2659_dev *sensor)
{
    camera_pin_set_high(sensor->pwdn_pin);
    return 0;
}

static int ov2659_power_on(struct ov2659_dev *sensor)
{
    camera_pin_set_low(sensor->pwdn_pin);

    if (sensor->rst_pin) {
        camera_pin_set_low(sensor->rst_pin);
        rt_thread_mdelay(1);
        camera_pin_set_high(sensor->rst_pin);
        rt_thread_mdelay(3);
    }

    return 0;
}

static int ov2659_detect(struct ov2659_dev *sensor)
{
    u8 pid = 0;
    u8 ver = 0;
    int ret = 0;

    ret = ov2659_write_reg(sensor->i2c, REG_SOFTWARE_RESET, 0x01);
    if (ret != 0) {
        LOG_E("Sensor soft reset failed\n");
        return -ENODEV;
    }
    aicos_msleep(1);

    /* Check sensor revision */
    ret = ov2659_read_reg(sensor->i2c, REG_SC_CHIP_ID_H, &pid);
    if (!ret)
        ret = ov2659_read_reg(sensor->i2c, REG_SC_CHIP_ID_L, &ver);

    if (!ret) {
        unsigned short id;

        id = OV265X_ID(pid, ver);
        if (id != OV2659_ID) {
            LOG_E("Sensor detection failed (%04X, %d)\n", id, ret);
            ret = -ENODEV;
        } else {
            LOG_I("Found OV%04X sensor\n", id);
        }
    }

    return ret;
}

static rt_err_t ov2659_init(rt_device_t dev)
{
    int ret = 0;
    struct ov2659_dev *sensor = &g_ov2659_dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->xclk_freq = camera_xclk_rate_get();
    if (!sensor->xclk_freq) {
        LOG_E("Must set XCLK freq first!\n");
        return -RT_EINVAL;
    }

    sensor->rst_pin = camera_rst_pin_get();
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->rst_pin || !sensor->pwdn_pin)
        return -RT_EINVAL;

    ov2659_get_default_format(&sensor->fmt);
    sensor->frame_size = &ov2659_framesizes[OV2659_DFT_FRAMESIZE];
    sensor->format_ctrl_regs = ov2659_formats[OV2659_DFT_CODE].format_ctrl_regs;
    sensor->link_freq = OV2659_DFT_LINK_FREQ;

    ov2659_power_on(sensor);
    ret = ov2659_detect(sensor);
    if (ret < 0)
        goto error;

    /* Calculate the PLL register value needed */
    ov2659_pll_calc_params(sensor);

    LOG_I("OV2659 inited");
error:
    ov2659_power_off(sensor);
    return RT_EOK;
}

static rt_err_t ov2659_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct ov2659_dev *sensor = (struct ov2659_dev *)dev;

    ov2659_power_on(sensor);
    LOG_I("OV2659 Open");
    return RT_EOK;
}

static rt_err_t ov2659_close(rt_device_t dev)
{
    struct ov2659_dev *sensor = (struct ov2659_dev *)dev;

    ov2659_power_off(sensor);
    LOG_I("OV2659 Close");
    return RT_EOK;
}

static int ov2659_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct ov2659_dev *sensor = (struct ov2659_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int ov2659_start(rt_device_t dev)
{
    return ov2659_s_stream((struct ov2659_dev *)dev, 1);
}

static int ov2659_stop(rt_device_t dev)
{
    return ov2659_s_stream((struct ov2659_dev *)dev, 0);
}

static rt_err_t ov2659_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return ov2659_start(dev);
    case CAMERA_CMD_STOP:
        return ov2659_stop(dev);
    case CAMERA_CMD_GET_FMT:
        return ov2659_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops ov2659_ops =
{
    .init = ov2659_init,
    .open = ov2659_open,
    .close = ov2659_close,
    .control = ov2659_control,
};
#endif

int rt_hw_ov2659_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_ov2659_dev.dev.ops = &ov2659_ops;
#else
    g_ov2659_dev.dev.init = ov2659_init;
    g_ov2659_dev.dev.open = ov2659_open;
    g_ov2659_dev.dev.close = ov2659_close;
    g_ov2659_dev.dev.control = ov2659_control;
#endif
    g_ov2659_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_ov2659_dev.dev, CAMERA_DEV_NAME, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_ov2659_init);
