/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "ov9281"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "camera_inner.h"
#include "drv_camera.h"

/* Format configuration of OV9281 */
#define OV9281_DEFAULT_MODE             OV9281_MODE_640_480
#define OV9281_DEFAULT_BUS_TYPE         MEDIA_BUS_RAW8_MONO
#define OV9281_DEFAULT_CODE             MEDIA_BUS_FMT_Y8_1X8
#define OV9281_DEFAULT_DVP_BUS_WIDTH    8
#define OV9281_DEFAULT_AUTO_EXPOSURE    1
#define OV9281_DEFAULT_AUTO_GAIN        1

// #define OV9281_TEST_MODE

#define OV9281_LINK_FREQ_400MHZ     400000000
#define OV9281_LANES                2

/* pixel rate = link frequency * 2 * lanes / BITS_PER_SAMPLE */
#define OV9281_PIXEL_RATE_10BIT     (OV9281_LINK_FREQ_400MHZ * 2 * \
                     OV9281_LANES / 10)
#define OV9281_PIXEL_RATE_8BIT      (OV9281_LINK_FREQ_400MHZ * 2 * \
                     OV9281_LANES / 8)
#define OV9281_XVCLK_FREQ       24000000

#define OV9281_CHIP_ID          0x9281
#define OV9281_SLAVE_ID         0x60

#define OV9281_REG_CHIP_ID_HIGH         0x300a
#define OV9281_REG_CHIP_ID_LOW          0x300b
#define OV9281_REG_TIMING_FORMAT_1      0x3820
#define OV9281_REG_TIMING_FORMAT_2      0x3821
#define OV9281_FLIP_BIT                 BIT(2)

#define OV9281_REG_CTRL_MODE        0x0100
#define OV9281_MODE_SW_STANDBY      0x0
#define OV9281_MODE_STREAMING       BIT(0)

#define OV9281_REG_EXPOSURE         0x3500
#define OV9281_EXPOSURE_MIN         4
#define OV9281_EXPOSURE_STEP        1
/*
 * Number of lines less than frame length (VTS) that exposure must be.
 * Datasheet states 25, although empirically 5 appears to work.
 */
#define OV9281_EXPOSURE_OFFSET      25

#define OV9281_REG_GAIN_H           0x3508
#define OV9281_REG_GAIN_L           0x3509
#define OV9281_GAIN_H_MASK          0x07
#define OV9281_GAIN_H_SHIFT         8
#define OV9281_GAIN_L_MASK          0xff
#define OV9281_GAIN_MIN             0x10
#define OV9281_GAIN_MAX             0xf8
#define OV9281_GAIN_STEP            1
#define OV9281_GAIN_DEFAULT         0x10

#define OV9281_REG_TEST_PATTERN     0x5e00
#define OV9281_TEST_PATTERN_ENABLE  0x80
#define OV9281_TEST_PATTERN_DISABLE 0x0

#define OV9281_REG_VTS              0x380e
#define OV9281_VTS_MAX              0x7fff

/*
 * OV9281 native and active pixel array size.
 * Datasheet not available to confirm these values, so assume there are no
 * border pixels.
 */
#define OV9281_NATIVE_WIDTH         1280U
#define OV9281_NATIVE_HEIGHT        800U
#define OV9281_PIXEL_ARRAY_LEFT     0U
#define OV9281_PIXEL_ARRAY_TOP      0U
#define OV9281_PIXEL_ARRAY_WIDTH    1280U
#define OV9281_PIXEL_ARRAY_HEIGHT   800U

#define REG_NULL                    0xFFFF

#define OV9281_REG_VALUE_08BIT      1
#define OV9281_REG_VALUE_16BIT      2
#define OV9281_REG_VALUE_24BIT      3

#define OV9281_NAME                 "ov9281"

enum ov9281_mode_id {
    OV9281_MODE_1280_800 = 0,
    OV9281_MODE_1280_720,
    OV9281_MODE_640_480,
    OV9281_MODE_640_400,
    OV9281_MODE_NUM,
};

struct ov9281_mode {
    u32 width;
    u32 height;
    u32 hts_def;
    u32 vts_def;
    u32 exp_def;
    struct mpp_rect crop;
    const struct reg16_info *reg_list;
};

struct ov9281_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *iic;
    struct clk          *xvclk;
    u32    reset_gpio;
    u32    pwdn_gpio;

    bool   streaming;
    bool   power_on;
    const struct ov9281_mode *cur_mode;
    u32    code;
    struct mpp_video_fmt fmt;
};

static struct ov9281_dev g_ov9281_dev = {0};

#define to_ov9281(dev) ((struct ov9281_dev *)dev)

/*
 * Xclk 24Mhz
 * max_framerate 120fps for 10 bit, 144fps for 8 bit.
 * mipi_datarate per lane 800Mbps
 */
#if 0
static const struct reg16_info ov9281_common_regs[] = {
    {0x0103, 0x01},
    {0x0302, 0x32},
    {0x030e, 0x02},
    {0x3001, 0x00},
    {0x3004, 0x00},
    {0x3005, 0x00},
    {0x3006, 0x04},
    {0x3011, 0x0a},
    {0x3013, 0x18},
    {0x3022, 0x01},
    {0x3023, 0x00},
    {0x302c, 0x00},
    {0x302f, 0x00},
    {0x3030, 0x04},
    {0x3039, 0x32},
    {0x303a, 0x00},
    {0x303f, 0x01},
    {0x3500, 0x00},
    {0x3501, 0x2a},
    {0x3502, 0x90},
    {0x3503, 0x08},
    {0x3505, 0x8c},
    {0x3507, 0x03},
    {0x3508, 0x00},
    {0x3509, 0x10},
    {0x3610, 0x80},
    {0x3611, 0xa0},
    {0x3620, 0x6f},
    {0x3632, 0x56},
    {0x3633, 0x78},
    {0x3666, 0x00},
    {0x366f, 0x5a},
    {0x3680, 0x84},
    {0x3712, 0x80},
    {0x372d, 0x22},
    {0x3731, 0x80},
    {0x3732, 0x30},
    {0x377d, 0x22},
    {0x3788, 0x02},
    {0x3789, 0xa4},
    {0x378a, 0x00},
    {0x378b, 0x4a},
    {0x3799, 0x20},
    {0x3881, 0x42},
    {0x38b1, 0x00},
    {0x3920, 0xff},
    {0x4010, 0x40},
    {0x4043, 0x40},
    {0x4307, 0x30},
    {0x4317, 0x00},
    {0x4501, 0x00},
    {0x450a, 0x08},
    {0x4601, 0x04},
    {0x470f, 0x00},
    {0x4f07, 0x00},
    {0x4800, 0x00},
    {0x5000, 0x9f},
    {0x5001, 0x00},
    {0x5e00, 0x00},
    {0x5d00, 0x07},
    {0x5d01, 0x00},
    {REG_NULL, 0x00},
};
#endif

static const struct reg16_info ov9281_1280x800_regs[] = {
    {0x0103, 0x01},
    {0x0302, 0x24},
    {0x030d, 0x48},
    {0x030e, 0x06},
    {0x3001, 0x22},
    {0x3004, 0x01},
    {0x3005, 0xff},
    {0x3006, 0xe2},
    {0x3011, 0x0a},
    {0x3013, 0x18},
    {0x3022, 0x07},
    {0x3039, 0x2e},
    {0x303a, 0xf0},
    {0x3500, 0x00},
    {0x3501, 0x2a},
    {0x3502, 0x90},
    {0x3503, 0x00},
    {0x3508, 0x03},
    {0x3509, 0x00},
    {0x3610, 0x80},
    {0x3611, 0xa0},
    {0x3620, 0x78},
    {0x3632, 0x56},
    {0x3633, 0x50},
    {0x3662, 0x05},
    {0x3666, 0x5a},
    {0x366f, 0x7e},
    {0x3712, 0x80},
    {0x372d, 0x22},
    {0x3731, 0x80},
    {0x3732, 0x30},
    {0x3778, 0x00},
    {0x377d, 0x22},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x05},
    {0x3805, 0x0f},
    {0x3806, 0x03},
    {0x3807, 0x2f},
    {0x3808, 0x05},
    {0x3809, 0x00},
    {0x380a, 0x03},
    {0x380b, 0x20},
    {0x380c, 0x02},
    {0x380d, 0xd8},
    {0x380e, 0x03},
    {0x380f, 0x8e},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x40},
    {0x3821, 0x00},
    {0x3881, 0x42},
    {0x4043, 0x40},
    {0x4307, 0x30},
    {0x4317, 0x01},
    {0x4501, 0x00},
    {0x4507, 0x00},
    {0x4509, 0x00},
    {0x4800, 0x00},
    {0x5000, 0x9f},
    {0x5001, 0x00},
    {0x5e00, 0x00},
    {0x0100, 0x01},
    {REG_NULL, 0x00},
};

static const struct reg16_info ov9281_1280x720_regs[] = {
    {0x0103, 0x01},
    {0x0106, 0x00},
    {0x0302, 0x30},
    {0x030d, 0x60},
    {0x030e, 0x06},
    {0x3001, 0x62},
    {0x3004, 0x01},
    {0x3005, 0xff},
    {0x3006, 0xe2},
    {0x3011, 0x0a},
    {0x3013, 0x18},
    {0x301c, 0xf0},
    {0x3022, 0x07},
    {0x3030, 0x10},
    {0x3039, 0x2e},
    {0x303a, 0xf0},
    {0x3500, 0x00},
    {0x3501, 0x2a},
    {0x3502, 0x90},
    {0x3503, 0x08},
    {0x3505, 0x8c},
    {0x3507, 0x03},
    {0x3508, 0x00},
    {0x3509, 0x10},
    {0x3610, 0x80},
    {0x3611, 0xa0},
    {0x3620, 0x6e},
    {0x3632, 0x56},
    {0x3633, 0x78},
    {0x3662, 0x05}, // RAW10
    {0x3666, 0x5a},
    {0x366f, 0x7e},
    {0x3680, 0x84},
    {0x3707, 0x60},
    {0x370d, 0x01},
    {0x370e, 0x00},
    {0x3712, 0x80},
    {0x372d, 0x22},
    {0x3731, 0x80},
    {0x3732, 0x30},
    {0x3778, 0x00},
    {0x377d, 0x22},
    {0x3788, 0x02},
    {0x3789, 0xa4},
    {0x378a, 0x00},
    {0x378b, 0x4a},
    {0x3799, 0x20},
    {0x379c, 0x01},
    {0x3800, 0x00}, // ;
    {0x3801, 0x00}, // ;
    {0x3802, 0x00}, // ;
    {0x3803, 0x28}, // ;
    {0x3804, 0x05},
    {0x3805, 0x0f},
    {0x3806, 0x03},
    {0x3807, 0x2f},
    {0x3808, 0x05}, // X OUTPUT SIZE
    {0x3809, 0x00}, // X OUTPUT SIZE
    {0x380a, 0x02}, // Y OUTPUT SIZE
    {0x380b, 0xd0}, // Y OUTPUT SIZE
    {0x380c, 0x02},
    {0x380d, 0xd8},
    {0x380e, 0x03}, // 72 fps
    {0x380f, 0x8e},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x40},
    {0x3821, 0x00},
    {0x382b, 0x3a},
    {0x382c, 0x06},
    {0x382d, 0xc2},
    {0x389d, 0x00},
    {0x3881, 0x42},
    {0x3882, 0x02},
    {0x3883, 0x12},
    {0x3885, 0x07},
    {0x38a8, 0x02},
    {0x38a9, 0x80},
    {0x38b1, 0x03},
    {0x38b3, 0x07},
    {0x38c4, 0x00},
    {0x38c5, 0xc0},
    {0x38c6, 0x04},
    {0x38c7, 0x80},
    {0x3920, 0xff},
    {0x4003, 0x40},
    {0x4008, 0x04},
    {0x4009, 0x0b},
    {0x400c, 0x01},
    {0x400d, 0x07},
    {0x4010, 0xf0},
    {0x4011, 0x3b},
    {0x4043, 0x40},
    {0x4307, 0x30},
    {0x4317, 0x01},
    {0x4501, 0x00},
    {0x4507, 0x00},
    {0x4509, 0x00},
    {0x450a, 0x08},
    {0x4601, 0x04},
    {0x470f, 0xe0},
    {0x4f07, 0x00},
    {0x4800, 0x00},
    {0x5000, 0x9f},
    {0x5001, 0x00},
    {0x5e00, 0x00},
    {0x5d00, 0x0b},
    {0x5d01, 0x02},
    {0x4f00, 0x0c},
    {0x4f10, 0x00},
    {0x4f11, 0x88},
    {0x4f12, 0x0f},
    {0x4f13, 0xc4},

    {0x3501, 0x27},
    {0x3502, 0x50},

    {0x0100, 0x01},
    {REG_NULL, 0x00},
};

static const struct reg16_info ov9281_640x480_regs[] = {
    {0x0103, 0x01},
    {0x0106, 0x00},
    {0x0302, 0x30},
    {0x030d, 0x60},
    {0x030e, 0x06},
    {0x3001, 0x62},
    {0x3004, 0x01},
    {0x3005, 0xff},
    {0x3006, 0xe2},
    {0x3011, 0x0a},
    {0x3013, 0x18},
    {0x301c, 0xf0},
    {0x3022, 0x07},
    {0x3030, 0x10},
    {0x3039, 0x2e},
    {0x303a, 0xf0},
    {0x3500, 0x00},
    {0x3501, 0x2a},
    {0x3502, 0x90},
    {0x3503, 0x08},
    {0x3505, 0x8c},
    {0x3507, 0x03},
    {0x3508, 0x00},
    {0x3509, 0x10},
    {0x3610, 0x80},
    {0x3611, 0xa0},
    {0x3620, 0x6e},
    {0x3632, 0x56},
    {0x3633, 0x78},
    {0x3662, 0x05}, // RAW10
    {0x3666, 0x5a},
    {0x366f, 0x7e},
    {0x3680, 0x84},
    {0x3707, 0x60},
    {0x370d, 0x01},
    {0x370e, 0x00},
    {0x3712, 0x80},
    {0x372d, 0x22},
    {0x3731, 0x80},
    {0x3732, 0x30},
    {0x3778, 0x00},
    {0x377d, 0x22},
    {0x3788, 0x02},
    {0x3789, 0xa4},
    {0x378a, 0x00},
    {0x378b, 0x4a},
    {0x3799, 0x20},
    {0x379c, 0x01},
    {0x3800, 0x01},
    {0x3801, 0x40},
    {0x3802, 0x00},
    {0x3803, 0xa0},
    {0x3804, 0x05},
    {0x3805, 0x0f},
    {0x3806, 0x03},
    {0x3807, 0x2f},
    {0x3808, 0x02}, // X OUTPUT SIZE
    {0x3809, 0x80}, // X OUTPUT SIZE
    {0x380a, 0x01}, // Y OUTPUT SIZE
    {0x380b, 0xe0}, // Y OUTPUT SIZE
    {0x380c, 0x02},
    {0x380d, 0xd8},
    {0x380e, 0x03},
    {0x380f, 0x8e},
    {0x3810, 0x00},
    {0x3811, 0x08},
    {0x3812, 0x00},
    {0x3813, 0x08},
    {0x3814, 0x11},
    {0x3815, 0x11},
    {0x3820, 0x40},
    {0x3821, 0x00},
    {0x382b, 0x3a},
    {0x382c, 0x06},
    {0x382d, 0xc2},
    {0x389d, 0x00},
    {0x3881, 0x42},
    {0x3882, 0x02},
    {0x3883, 0x12},
    {0x3885, 0x07},
    {0x38a8, 0x02},
    {0x38a9, 0x80},
    {0x38b1, 0x03},
    {0x38b3, 0x07},
    {0x38c4, 0x00},
    {0x38c5, 0xc0},
    {0x38c6, 0x04},
    {0x38c7, 0x80},
    {0x3920, 0xff},
    {0x4003, 0x40},
    {0x4008, 0x04},
    {0x4009, 0x0b},
    {0x400c, 0x01},
    {0x400d, 0x07},
    {0x4010, 0xf0},
    {0x4011, 0x3b},
    {0x4043, 0x40},
    {0x4307, 0x30},
    {0x4317, 0x01},
    {0x4501, 0x00},
    {0x4507, 0x00},
    {0x4509, 0x00},
    {0x450a, 0x08},
    {0x4601, 0x04},
    {0x470f, 0xe0},
    {0x4f07, 0x00},
    {0x4800, 0x00},
    {0x5000, 0x9f},
    {0x5001, 0x00},
    {0x5e00, 0x00},
    {0x5d00, 0x0b},
    {0x5d01, 0x02},
    {0x4f00, 0x0c},
    {0x4f10, 0x00},
    {0x4f11, 0x88},
    {0x4f12, 0x0f},
    {0x4f13, 0xc4},

    {0x3501, 0x27},
    {0x3502, 0x50},

    {0x0100, 0x01},
    {REG_NULL, 0x00},
};

static const struct reg16_info ov9281_640x400_regs[] = {
    {0x0103, 0x01},
    {0x0302, 0x24},
    {0x030d, 0x48},
    {0x030e, 0x06},
    {0x3001, 0x22},
    {0x3004, 0x01},
    {0x3005, 0xff},
    {0x3006, 0xe2},
    {0x3011, 0x0a},
    {0x3013, 0x18},
    {0x3022, 0x07},
    {0x3039, 0x2e},
    {0x303a, 0xf0},
    {0x3500, 0x00},
    {0x3501, 0x01},
    {0x3502, 0xf4},
    {0x3503, 0x00},
    {0x3508, 0x07},
    {0x3509, 0x80},
    {0x3610, 0x80},
    {0x3611, 0xa0},
    {0x3620, 0x78},
    {0x3632, 0x56},
    {0x3633, 0x50},
    {0x3662, 0x05},
    {0x3666, 0x5a},
    {0x366f, 0x7e},
    {0x3712, 0x80},
    {0x372d, 0x22},
    {0x3731, 0x80},
    {0x3732, 0x30},
    {0x3778, 0x10},
    {0x377d, 0x22},
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x05},
    {0x3805, 0x0f},
    {0x3806, 0x03},
    {0x3807, 0x2f},
    {0x3808, 0x02}, // 640
    {0x3809, 0x80},
    {0x380a, 0x01}, // 400
    {0x380b, 0x90},
    {0x380c, 0x02},
    {0x380d, 0xd8},
    {0x380e, 0x02},
    {0x380f, 0x08},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x04},
    {0x3814, 0x31},
    {0x3815, 0x22},
    {0x3820, 0x60},
    {0x3821, 0x01},
    {0x3881, 0x42},
    {0x4043, 0x40},
    {0x4307, 0x30},
    {0x4317, 0x01}, // DVP
    {0x4501, 0x00},
    {0x4507, 0x03},
    {0x4509, 0x80},
    {0x4800, 0x00},
    {0x5000, 0x9f},
    {0x5001, 0x00},
    {0x5e00, 0x00},
    {0x0100, 0x01},
    {REG_NULL, 0x00},
};

static const struct reg16_info op_8bit[] = {
    {0x030d, 0x60},
    {0x3662, 0x07},
    {REG_NULL, 0x00},
};

static const struct ov9281_mode supported_modes[] = {
    {
        .width = 1280,
        .height = 800,
        .exp_def = 0x0320,
        .hts_def = 0x05b0,  /* 0x2d8*2 */
        .vts_def = 0x038e,
        .crop = {
            .x = 0,
            .y = 0,
            .width = 1280,
            .height = 800
        },
        .reg_list = ov9281_1280x800_regs,
    },
    {
        .width = 1280,
        .height = 720,
        .exp_def = 0x0320,
        .hts_def = 0x05b0, // 1456
        .vts_def = 910,
        .crop = {
            .x = 0,
            .y = 40,
            .width = 1280,
            .height = 720
        },
        .reg_list = ov9281_1280x720_regs,
    },
    {
        .width = 640,
        .height = 480,
        .exp_def = 0x0320,
        .hts_def = 0x05b0,
        .vts_def = 422,
        .crop = {
            .x = 0,
            .y = 0,
            .width = 1280,
            .height = 800
        },
        .reg_list = ov9281_640x480_regs,
    },
    {
        .width = 640,
        .height = 400,
        .exp_def = 0x0320,
        .hts_def = 0x05b0,
        .vts_def = 422,
        .crop = {
            .x = 0,
            .y = 0,
            .width = 1280,
            .height = 800
        },
        .reg_list = ov9281_640x400_regs,
    },
};

/* Write one byte at a time */
static int ov9281_write_reg(struct ov9281_dev *sensor, u16 reg, u8 val)
{
    if (rt_i2c_write_reg16(sensor->iic, OV9281_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int ov9281_write_array(struct ov9281_dev *sensor,
                              const struct reg16_info *regs)
{
    u32 i;
    int ret = 0;

    for (i = 0; ret == 0 && regs[i].reg != REG_NULL; i++)
        ret = ov9281_write_reg(sensor, regs[i].reg, regs[i].val);

    return ret;
}

/* Read one byte at a time */
static int ov9281_read_reg(struct ov9281_dev *sensor, u16 reg, u8 *val)
{
    if (rt_i2c_read_reg16(sensor->iic, OV9281_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static u16 ov9281_read_u16(struct ov9281_dev *sensor, u16 reg_h, u16 reg_l)
{
    u8 val_h = 0, val_l = 0;

    if (ov9281_read_reg(sensor, reg_h, &val_h) ||
        ov9281_read_reg(sensor, reg_l, &val_l))
        return 0;

    return val_h << 8 | val_l;
}

#ifdef OV9281_TEST_MODE
static void ov9281_test_mode(struct ov9281_dev *sensor)
{
    ov9281_write_reg(sensor, OV9281_REG_TEST_PATTERN, 0x84);
}
#endif

static int ov9281_set_fps(struct ov9281_dev *sensor, u32 fps)
{
    u16 cur = 0, val = 0;
    u32 base[] = {0x222, 0x222, 0x222, 0x186};

    if (fps > 210)
        fps = 210;
    if (fps < 10)
        fps = 10;

    cur = ov9281_read_u16(sensor, 0x380e, 0x380f);
    if (!cur) {
        LOG_E("Failed to read FPS\n");
        return -1;
    }

    if (!base[OV9281_DEFAULT_MODE])
        return -1;

    val = base[OV9281_DEFAULT_MODE] * 120 / fps;

    LOG_I("Set FPS %d[0x%03x] -> %d[0x%03x]\n",
          120 * base[OV9281_DEFAULT_MODE] / cur, cur,
          120 * base[OV9281_DEFAULT_MODE] / val, val);

    if (cur == val)
        return 0;

    if (ov9281_write_reg(sensor, 0x380e, val >> 8) ||
        ov9281_write_reg(sensor, 0x380f, val & 0xFF))
        return -1;
    else
        return 0;
}

static int ov9281_set_aec(struct ov9281_dev *sensor, u32 percent)
{
    u16 frame_len = 0, cur = 0, val = 0;
    u8 val_h = 0, val_l = 0;

    frame_len = ov9281_read_u16(sensor, 0x380e, 0x380f);
    if (!frame_len) {
        LOG_E("Failed to get frame length\n");
        return -1;
    }
    if (frame_len - 25 > 0xFFF) {
        LOG_W("Frame length %d is too large\n", frame_len);
        frame_len = 0xFFF + 25;
    }

    if (ov9281_read_reg(sensor, 0x3501, &val_h)
        || ov9281_read_reg(sensor, 0x3502, &val_l)) {
        LOG_E("Failed to get current AEC\n");
        return -1;
    }
    cur = (val_h << 4) | (val_l >> 4); // {16'h3501,16'h3502[7:4]}

    val = PERCENT_TO_INT(1, frame_len - 25, percent);
    LOG_I("Set AEC %d[0x%03x] -> %d[0x%03x] (Frame length %d[0x%03x])\n",
          cur, cur, val, val, frame_len, frame_len);

    if (cur == val)
        return 0;

    if (ov9281_write_reg(sensor, 0x3501, (val >> 4) & 0xFF) ||
        ov9281_write_reg(sensor, 0x3502, (val & 0xF) << 4))
        return -1;
    else
        return 0;
}

static int ov9281_set_agc(struct ov9281_dev *sensor, u32 percent)
{
    u8 mode = 0;
    u8 cur = 0, gain = PERCENT_TO_INT(1, 0xFF, percent);

    if (ov9281_read_reg(sensor, 0x3503, &mode))
        return -1;

    mode &= ~0x4;
    if (ov9281_write_reg(sensor, 0x3503, mode))
        return -1;

    if (ov9281_read_reg(sensor, 0x3509, &cur))
        return -1;

    LOG_I("Set AGC %d[0x%02x] -> %d[0x%02x]\n", cur, cur, gain, gain);

    if (ov9281_write_reg(sensor, 0x3509, gain))
        return -1;
    else
        return 0;
}

static int ov9281_enable_flip(struct ov9281_dev *sensor, u16 reg,
                              bool enable, char *name)
{
    u8 cur = 0, mask = 0x4, shift = 2;

    if (ov9281_read_reg(sensor, reg, &cur)) {
        LOG_E("Failed to get current flip\n");
        return -1;
    }

    LOG_I("Set %s flip %d -> %d\n", name, (cur & mask) >> shift, enable);
    if ((cur & mask) >> shift == (u8)enable)
        return 0;

    if (enable)
        return ov9281_write_reg(sensor, reg, cur | mask);
    else
        return ov9281_write_reg(sensor, reg, cur & ~mask);
}

static int ov9281_enable_h_flip(struct ov9281_dev *sensor, bool enable)
{
    return ov9281_enable_flip(sensor, 0x3821, enable, "H");
}

static int ov9281_enable_v_flip(struct ov9281_dev *sensor, bool enable)
{
    return ov9281_enable_flip(sensor, 0x3820, enable, "V");
}

static int ov9281_start_stream(struct ov9281_dev *sensor)
{
    int ret;

#if 0
    ret = ov9281_write_array(sensor, ov9281_common_regs);
    if (ret)
        return ret;
#endif
    ret = ov9281_write_array(sensor, sensor->cur_mode->reg_list);
    if (ret)
        return ret;

    /* Only support 8bit */
    ret = ov9281_write_array(sensor, op_8bit);
    if (ret)
        return ret;

#ifdef OV9281_TEST_MODE
    ov9281_test_mode(sensor);
#endif
    ov9281_set_aec(sensor, 80);
    ov9281_set_agc(sensor, 100);

    return ov9281_write_reg(sensor, OV9281_REG_CTRL_MODE,
                            OV9281_MODE_STREAMING);
}

static int ov9281_stop_stream(struct ov9281_dev *sensor)
{
    return ov9281_write_reg(sensor, OV9281_REG_CTRL_MODE,
                            OV9281_MODE_SW_STANDBY);
}

static int ov9281_s_stream(struct ov9281_dev *sensor, int on)
{
    int ret = 0;

    on = !!on;
    if (on == sensor->streaming)
        goto unlock_and_return;

    if (on) {
        ret = ov9281_start_stream(sensor);
        if (ret) {
            LOG_E("start stream failed while write regs\n");
            goto unlock_and_return;
        }
    } else {
        ov9281_stop_stream(sensor);
    }

    sensor->streaming = on;

unlock_and_return:
    return ret;
}

/* Calculate the delay in us by clock rate and clock cycles */
static inline u32 ov9281_cal_delay(u32 cycles)
{
    return DIV_ROUND_UP(cycles, OV9281_XVCLK_FREQ / 1000 / 1000);
}

static int ov9281_power_on(struct ov9281_dev *sensor)
{
    u32 delay_us;

    camera_pin_set_low(sensor->reset_gpio);
    aicos_msleep(10);
    camera_pin_set_high(sensor->reset_gpio);

    camera_pin_set_high(sensor->pwdn_gpio);

    /* 8192 cycles prior to first SCCB transaction */
    delay_us = ov9281_cal_delay(8192);
    aicos_udelay(delay_us * 2);

    return 0;
}

static void ov9281_power_off(struct ov9281_dev *sensor)
{
    camera_pin_set_low(sensor->pwdn_gpio);
    camera_pin_set_low(sensor->reset_gpio);
}

static bool ov9281_is_open(struct ov9281_dev *sensor)
{
    return sensor->power_on;
}

static int ov9281_s_power(struct ov9281_dev *sensor, int on)
{
    /* If the power state is not modified - no work to do. */
    if (sensor->power_on == !!on)
        return 0;

    if (on) {
        ov9281_power_on(sensor);
        LOG_I("Power on");
        sensor->power_on = true;
    } else {
        ov9281_power_off(sensor);
        LOG_I("Power off");
        sensor->power_on = false;
    }

    return 0;
}

static int ov9281_check_sensor_id(struct ov9281_dev *sensor)
{
    u8 id_h = 0, id_l = 0;

    if (ov9281_read_reg(sensor, OV9281_REG_CHIP_ID_HIGH, &id_h)
        || ov9281_read_reg(sensor, OV9281_REG_CHIP_ID_LOW, &id_l))
        return -1;

    if ((id_h << 8 | id_l) != OV9281_CHIP_ID) {
        LOG_E("Unexpected sensor id: %02x %02x\n", id_h, id_l);
        return -ENODEV;
    }
    return 0;
}

static rt_err_t ov9281_init(rt_device_t dev)
{
    struct ov9281_dev *sensor = to_ov9281(dev);

    sensor->iic = camera_i2c_get();
    if (!sensor->iic)
        return -RT_EINVAL;

    sensor->reset_gpio = camera_rst_pin_get();
    sensor->pwdn_gpio  = camera_pwdn_pin_get();
    if (!sensor->reset_gpio || !sensor->pwdn_gpio)
        return -RT_EINVAL;

    sensor->cur_mode = &supported_modes[OV9281_DEFAULT_MODE];
    sensor->fmt.code   = OV9281_DEFAULT_CODE;
    sensor->fmt.width  = sensor->cur_mode->width;
    sensor->fmt.height = sensor->cur_mode->height;
    sensor->fmt.bus_type = OV9281_DEFAULT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    return 0;
}

static rt_err_t ov9281_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct ov9281_dev *sensor = to_ov9281(dev);

    if (ov9281_is_open(sensor))
        return RT_EOK;

    ov9281_s_power(sensor, 1);

    if (ov9281_check_sensor_id(sensor)) {
        ov9281_power_off(sensor);
        return -RT_ERROR;
    }
    LOG_I("OV9281 Inited");
    return RT_EOK;
}

static rt_err_t ov9281_close(rt_device_t dev)
{
    struct ov9281_dev *sensor = (struct ov9281_dev *)dev;

    if (!ov9281_is_open(sensor))
        return -RT_ERROR;

    ov9281_s_power(sensor, 0);
    LOG_D("OV9281 Close");
    return RT_EOK;
}

static int ov9281_get_fmt(struct ov9281_dev *sensor, struct mpp_video_fmt *cfg)
{

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int ov9281_start(struct ov9281_dev *sensor)
{
    return ov9281_s_stream(sensor, 1);
}

static int ov9281_stop(struct ov9281_dev *sensor)
{
    return ov9281_s_stream(sensor, 0);
}

static int ov9281_pause(rt_device_t dev)
{
    struct ov9281_dev *sensor = (struct ov9281_dev *)dev;

    return ov9281_write_reg(sensor, OV9281_REG_CTRL_MODE, OV9281_MODE_SW_STANDBY);
}

static int ov9281_resume(rt_device_t dev)
{
    struct ov9281_dev *sensor = (struct ov9281_dev *)dev;

    return ov9281_write_reg(sensor, OV9281_REG_CTRL_MODE, OV9281_MODE_STREAMING);
}

static rt_err_t ov9281_control(rt_device_t dev, int cmd, void *args)
{
    struct ov9281_dev *sensor = (struct ov9281_dev *)dev;

    switch (cmd) {
    case CAMERA_CMD_START:
        return ov9281_start(sensor);
    case CAMERA_CMD_STOP:
        return ov9281_stop(sensor);
    case CAMERA_CMD_PAUSE:
        return ov9281_pause(dev);
    case CAMERA_CMD_RESUME:
        return ov9281_resume(dev);
    case CAMERA_CMD_GET_FMT:
        return ov9281_get_fmt(sensor, (struct mpp_video_fmt *)args);
    case CAMERA_CMD_SET_FPS:
        return ov9281_set_fps(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_AEC_VAL:
        return ov9281_set_aec(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_AUTOGAIN:
        return ov9281_set_agc(sensor, *(u32 *)args);
    case CAMERA_CMD_SET_H_FLIP:
        return ov9281_enable_h_flip(sensor, *(bool *)args);
    case CAMERA_CMD_SET_V_FLIP:
        return ov9281_enable_v_flip(sensor, *(bool *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops ov9281_ops =
{
    .init = ov9281_init,
    .open = ov9281_open,
    .close = ov9281_close,
    .control = ov9281_control,
};
#endif

int rt_hw_ov9281_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_ov9281_dev.dev.ops = &ov9281_ops;
#else
    g_ov9281_dev.dev.init = ov9281_init;
    g_ov9281_dev.dev.open = ov9281_open;
    g_ov9281_dev.dev.close = ov9281_close;
    g_ov9281_dev.dev.control = ov9281_control;
#endif
    g_ov9281_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_ov9281_dev.dev, CAMERA_DEV_NAME, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_ov9281_init);
