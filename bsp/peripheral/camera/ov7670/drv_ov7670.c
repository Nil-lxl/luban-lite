/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "ov7670"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of OV7670 */
#define OV7670_DFT_WIN          OV7670_WIN_VGA
#define OV7670_DFT_BUS_TYPE     MEDIA_BUS_PARALLEL
#define OV7670_DFT_MODEL        MODEL_OV7670
#define OV7670_DFT_CODE         0
#define OV7670_DFT_FR           30

/* The clk source is decided by board design */
#define CAMERA_CLK_SRC          CLK_OUT1

#define OV7670_I2C_SLAVE_ID     0x21

#define PLL_FACTOR  4

/* Registers */
#define REG_GAIN    0x00    /* Gain lower 8 bits (rest in vref) */
#define REG_BLUE    0x01    /* blue gain */
#define REG_RED     0x02    /* red gain */
#define REG_VREF    0x03    /* Pieces of GAIN, VSTART, VSTOP */
#define REG_COM1    0x04    /* Control 1 */
#define  COM1_CCIR656     0x40  /* CCIR656 enable */
#define REG_BAVE    0x05    /* U/B Average level */
#define REG_GbAVE   0x06    /* Y/Gb Average level */
#define REG_AECHH   0x07    /* AEC MS 5 bits */
#define REG_RAVE    0x08    /* V/R Average level */
#define REG_COM2    0x09    /* Control 2 */
#define  COM2_SSLEEP      0x10  /* Soft sleep mode */
#define REG_PID     0x0a    /* Product ID MSB */
#define REG_VER     0x0b    /* Product ID LSB */
#define REG_COM3    0x0c    /* Control 3 */
#define  COM3_SWAP    0x40    /* Byte swap */
#define  COM3_SCALEEN     0x08    /* Enable scaling */
#define  COM3_DCWEN   0x04    /* Enable downsamp/crop/window */
#define REG_COM4    0x0d    /* Control 4 */
#define REG_COM5    0x0e    /* All "reserved" */
#define REG_COM6    0x0f    /* Control 6 */
#define REG_AECH    0x10    /* More bits of AEC value */
#define REG_CLKRC   0x11    /* Clocl control */
#define   CLK_EXT     0x40    /* Use external clock directly */
#define   CLK_SCALE   0x3f    /* Mask for internal clock scale */
#define REG_COM7    0x12    /* Control 7 */
#define   COM7_RESET      0x80    /* Register reset */
#define   COM7_FMT_MASK   0x38
#define   COM7_FMT_VGA    0x00
#define   COM7_FMT_CIF    0x20    /* CIF format */
#define   COM7_FMT_QVGA   0x10    /* QVGA format */
#define   COM7_FMT_QCIF   0x08    /* QCIF format */
#define   COM7_RGB    0x04    /* bits 0 and 2 - RGB format */
#define   COM7_YUV    0x00    /* YUV */
#define   COM7_BAYER      0x01    /* Bayer format */
#define   COM7_PBAYER     0x05    /* "Processed bayer" */
#define REG_COM8    0x13    /* Control 8 */
#define   COM8_FASTAEC    0x80    /* Enable fast AGC/AEC */
#define   COM8_AECSTEP    0x40    /* Unlimited AEC step size */
#define   COM8_BFILT      0x20    /* Band filter enable */
#define   COM8_AGC    0x04    /* Auto gain enable */
#define   COM8_AWB    0x02    /* White balance enable */
#define   COM8_AEC    0x01    /* Auto exposure enable */
#define REG_COM9    0x14    /* Control 9  - gain ceiling */
#define REG_COM10   0x15    /* Control 10 */
#define   COM10_HSYNC     0x40    /* HSYNC instead of HREF */
#define   COM10_PCLK_HB   0x20    /* Suppress PCLK on horiz blank */
#define   COM10_HREF_REV  0x08    /* Reverse HREF */
#define   COM10_VS_LEAD   0x04    /* VSYNC on clock leading edge */
#define   COM10_VS_NEG    0x02    /* VSYNC negative */
#define   COM10_HS_NEG    0x01    /* HSYNC negative */
#define REG_HSTART  0x17    /* Horiz start high bits */
#define REG_HSTOP   0x18    /* Horiz stop high bits */
#define REG_VSTART  0x19    /* Vert start high bits */
#define REG_VSTOP   0x1a    /* Vert stop high bits */
#define REG_PSHFT   0x1b    /* Pixel delay after HREF */
#define REG_MIDH    0x1c    /* Manuf. ID high */
#define REG_MIDL    0x1d    /* Manuf. ID low */
#define REG_MVFP    0x1e    /* Mirror / vflip */
#define   MVFP_MIRROR     0x20    /* Mirror image */
#define   MVFP_FLIP   0x10    /* Vertical flip */

#define REG_AEW     0x24    /* AGC upper limit */
#define REG_AEB     0x25    /* AGC lower limit */
#define REG_VPT     0x26    /* AGC/AEC fast mode op region */
#define REG_HSYST   0x30    /* HSYNC rising edge delay */
#define REG_HSYEN   0x31    /* HSYNC falling edge delay */
#define REG_HREF    0x32    /* HREF pieces */
#define REG_TSLB    0x3a    /* lots of stuff */
#define   TSLB_YLAST      0x04    /* UYVY or VYUY - see com13 */
#define REG_COM11   0x3b    /* Control 11 */
#define   COM11_NIGHT     0x80    /* NIght mode enable */
#define   COM11_NMFR      0x60    /* Two bit NM frame rate */
#define   COM11_HZAUTO    0x10    /* Auto detect 50/60 Hz */
#define   COM11_50HZ      0x08    /* Manual 50Hz select */
#define   COM11_EXP   0x02
#define REG_COM12   0x3c    /* Control 12 */
#define   COM12_HREF      0x80    /* HREF always */
#define REG_COM13   0x3d    /* Control 13 */
#define   COM13_GAMMA     0x80    /* Gamma enable */
#define   COM13_UVSAT     0x40    /* UV saturation auto adjustment */
#define   COM13_UVSWAP    0x01    /* V before U - w/TSLB */
#define REG_COM14   0x3e    /* Control 14 */
#define   COM14_DCWEN     0x10    /* DCW/PCLK-scale enable */
#define REG_EDGE    0x3f    /* Edge enhancement factor */
#define REG_COM15   0x40    /* Control 15 */
#define   COM15_R10F0     0x00    /* Data range 10 to F0 */
#define   COM15_R01FE     0x80    /*            01 to FE */
#define   COM15_R00FF     0xc0    /*            00 to FF */
#define   COM15_RGB565    0x10    /* RGB565 output */
#define   COM15_RGB555    0x30    /* RGB555 output */
#define REG_COM16   0x41    /* Control 16 */
#define   COM16_AWBGAIN   0x08    /* AWB gain enable */
#define REG_COM17   0x42    /* Control 17 */
#define   COM17_AECWIN    0xc0    /* AEC window - must match COM4 */
#define   COM17_CBAR      0x08    /* DSP Color bar */

/*
 * This matrix defines how the colors are generated, must be
 * tweaked to adjust hue and saturation.
 *
 * Order: v-red, v-green, v-blue, u-red, u-green, u-blue
 *
 * They are nine-bit signed quantities, with the sign bit
 * stored in 0x58.  Sign for v-red is bit 0, and up from there.
 */
#define REG_CMATRIX_BASE 0x4f
#define   CMATRIX_LEN 6
#define REG_CMATRIX_SIGN 0x58

#define REG_BRIGHT  0x55    /* Brightness */
#define REG_CONTRAS 0x56    /* Contrast control */

#define REG_GFIX    0x69    /* Fix gain control */

#define REG_DBLV    0x6b    /* PLL control an debugging */
#define   DBLV_BYPASS     0x0a    /* Bypass PLL */
#define   DBLV_X4     0x4a    /* clock x4 */
#define   DBLV_X6     0x8a    /* clock x6 */
#define   DBLV_X8     0xca    /* clock x8 */

#define REG_SCALING_XSC 0x70    /* Test pattern and horizontal scale factor */
#define   TEST_PATTTERN_0 0x80
#define REG_SCALING_YSC 0x71    /* Test pattern and vertical scale factor */
#define   TEST_PATTTERN_1 0x80

#define REG_REG76   0x76    /* OV's name */
#define   R76_BLKPCOR     0x80    /* Black pixel correction enable */
#define   R76_WHTPCOR     0x40    /* White pixel correction enable */

#define REG_RGB444  0x8c    /* RGB 444 control */
#define   R444_ENABLE     0x02    /* Turn on RGB444, overrides 5x5 */
#define   R444_RGBX   0x01    /* Empty nibble at end */

#define REG_HAECC1  0x9f    /* Hist AEC/AGC control 1 */
#define REG_HAECC2  0xa0    /* Hist AEC/AGC control 2 */

#define REG_BD50MAX 0xa5    /* 50hz banding step limit */
#define REG_HAECC3  0xa6    /* Hist AEC/AGC control 3 */
#define REG_HAECC4  0xa7    /* Hist AEC/AGC control 4 */
#define REG_HAECC5  0xa8    /* Hist AEC/AGC control 5 */
#define REG_HAECC6  0xa9    /* Hist AEC/AGC control 6 */
#define REG_HAECC7  0xaa    /* Hist AEC/AGC control 7 */
#define REG_BD60MAX 0xab    /* 60hz banding step limit */

enum ov7670_win_type {
    OV7670_WIN_VGA,
    OV7670_WIN_CIF,
    OV7670_WIN_QVGA,
    OV7670_WIN_QCIF
};

enum ov7670_model {
    MODEL_OV7670 = 0,
    MODEL_OV7675,
};

struct ov7670_win_size {
    int width;
    int height;
    unsigned char com7_bit;
    int hstart;     /* Start/stop values for the camera.  Note */
    int hstop;      /* that they do not always make complete */
    int vstart;     /* sense to humans, but evidently the sensor */
    int vstop;      /* will do the right thing... */
    struct reg8_info *regs; /* Regs to tweak */
};

struct ov7670_format_struct;  /* coming later */
struct ov7670_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 rst_pin;
    u32 pwdn_pin;

    struct ov7670_format_struct *ov_fmt;  /* Current format */
    struct mpp_video_fmt fmt;
    struct ov7670_win_size *wsize;
    struct clk *clk;
    int on;

    unsigned int mbus_config;   /* Media bus configuration flags */
    int min_width;          /* Filter out smaller sizes */
    int min_height;         /* Filter out smaller sizes */
    int clock_speed;        /* External clock speed (MHz) */
    u8 clkrc;           /* Clock divider value */
    bool use_smbus;         /* Use smbus I/O instead of I2C */
    bool pll_bypass;
    bool pclk_hb_disable;
    const struct ov7670_devtype *devtype; /* Device specifics */
};

struct ov7670_devtype {
    /* formats supported for each model */
    struct ov7670_win_size *win_sizes;
    unsigned int n_win_sizes;
    int (*set_framerate)(struct ov7670_dev *sensor, u32 fr);
};

static struct ov7670_dev g_ov7670_dev = {0};

static struct reg8_info ov7670_default_regs[] = {
    { REG_COM7, COM7_RESET },
/*
 * Clock scale: 3 = 15fps
 *              2 = 20fps
 *              1 = 30fps
 */
    { REG_CLKRC, 0x1 }, /* OV: clock scale (30 fps) */
    { REG_TSLB,  0x04 },    /* OV */
    { REG_COM7, 0 },    /* VGA */
    /*
     * Set the hardware window.  These values from OV don't entirely
     * make sense - hstop is less than hstart.  But they work...
     */
    { REG_HSTART, 0x13 },   { REG_HSTOP, 0x01 },
    { REG_HREF, 0xb6 }, { REG_VSTART, 0x02 },
    { REG_VSTOP, 0x7a },    { REG_VREF, 0x0a },

    { REG_COM3, 0 },    { REG_COM14, 0 },
    /* Mystery scaling numbers */
    { REG_SCALING_XSC, 0x3a },
    { REG_SCALING_YSC, 0x35 },
    { 0x72, 0x11 },     { 0x73, 0xf0 },
    { 0xa2, 0x02 },     { REG_COM10, 0x0 },

    /* Gamma curve values */
    { 0x7a, 0x20 },     { 0x7b, 0x10 },
    { 0x7c, 0x1e },     { 0x7d, 0x35 },
    { 0x7e, 0x5a },     { 0x7f, 0x69 },
    { 0x80, 0x76 },     { 0x81, 0x80 },
    { 0x82, 0x88 },     { 0x83, 0x8f },
    { 0x84, 0x96 },     { 0x85, 0xa3 },
    { 0x86, 0xaf },     { 0x87, 0xc4 },
    { 0x88, 0xd7 },     { 0x89, 0xe8 },

    /* AGC and AEC parameters.  Note we start by disabling those features,
       then turn them only after tweaking the values. */
    { REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT },
    { REG_GAIN, 0 },    { REG_AECH, 0 },
    { REG_COM4, 0x40 }, /* magic reserved bit */
    { REG_COM9, 0x18 }, /* 4x gain + magic rsvd bit */
    { REG_BD50MAX, 0x05 },  { REG_BD60MAX, 0x07 },
    { REG_AEW, 0x95 },  { REG_AEB, 0x33 },
    { REG_VPT, 0xe3 },  { REG_HAECC1, 0x78 },
    { REG_HAECC2, 0x68 },   { 0xa1, 0x03 }, /* magic */
    { REG_HAECC3, 0xd8 },   { REG_HAECC4, 0xd8 },
    { REG_HAECC5, 0xf0 },   { REG_HAECC6, 0x90 },
    { REG_HAECC7, 0x94 },
    { REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC },

    /* Almost all of these are magic "reserved" values.  */
    { REG_COM5, 0x61 }, { REG_COM6, 0x4b },
    { 0x16, 0x02 },     { REG_MVFP, 0x07 },
    { 0x21, 0x02 },     { 0x22, 0x91 },
    { 0x29, 0x07 },     { 0x33, 0x0b },
    { 0x35, 0x0b },     { 0x37, 0x1d },
    { 0x38, 0x71 },     { 0x39, 0x2a },
    { REG_COM12, 0x78 },    { 0x4d, 0x40 },
    { 0x4e, 0x20 },     { REG_GFIX, 0 },
    { 0x6b, 0x4a },     { 0x74, 0x10 },
    { 0x8d, 0x4f },     { 0x8e, 0 },
    { 0x8f, 0 },        { 0x90, 0 },
    { 0x91, 0 },        { 0x96, 0 },
    { 0x9a, 0 },        { 0xb0, 0x84 },
    { 0xb1, 0x0c },     { 0xb2, 0x0e },
    { 0xb3, 0x82 },     { 0xb8, 0x0a },

    /* More reserved magic, some of which tweaks white balance */
    { 0x43, 0x0a },     { 0x44, 0xf0 },
    { 0x45, 0x34 },     { 0x46, 0x58 },
    { 0x47, 0x28 },     { 0x48, 0x3a },
    { 0x59, 0x88 },     { 0x5a, 0x88 },
    { 0x5b, 0x44 },     { 0x5c, 0x67 },
    { 0x5d, 0x49 },     { 0x5e, 0x0e },
    { 0x6c, 0x0a },     { 0x6d, 0x55 },
    { 0x6e, 0x11 },     { 0x6f, 0x9f }, /* "9e for advance AWB" */
    { 0x6a, 0x40 },     { REG_BLUE, 0x40 },
    { REG_RED, 0x60 },
    { REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC|COM8_AWB },

    /* Matrix coefficients */
    { 0x4f, 0x80 },     { 0x50, 0x80 },
    { 0x51, 0 },        { 0x52, 0x22 },
    { 0x53, 0x5e },     { 0x54, 0x80 },
    { 0x58, 0x9e },

    { REG_COM16, COM16_AWBGAIN },   { REG_EDGE, 0 },
    { 0x75, 0x05 },     { 0x76, 0xe1 },
    { 0x4c, 0 },        { 0x77, 0x01 },
    { REG_COM13, 0xc3 },    { 0x4b, 0x09 },
    { 0xc9, 0x60 },     { REG_COM16, 0x38 },
    { 0x56, 0x40 },

    { 0x34, 0x11 },     { REG_COM11, COM11_EXP|COM11_HZAUTO },
    { 0xa4, 0x88 },     { 0x96, 0 },
    { 0x97, 0x30 },     { 0x98, 0x20 },
    { 0x99, 0x30 },     { 0x9a, 0x84 },
    { 0x9b, 0x29 },     { 0x9c, 0x03 },
    { 0x9d, 0x4c },     { 0x9e, 0x3f },
    { 0x78, 0x04 },

    /* Extra-weird stuff.  Some sort of multiplexor register */
    { 0x79, 0x01 },     { 0xc8, 0xf0 },
    { 0x79, 0x0f },     { 0xc8, 0x00 },
    { 0x79, 0x10 },     { 0xc8, 0x7e },
    { 0x79, 0x0a },     { 0xc8, 0x80 },
    { 0x79, 0x0b },     { 0xc8, 0x01 },
    { 0x79, 0x0c },     { 0xc8, 0x0f },
    { 0x79, 0x0d },     { 0xc8, 0x20 },
    { 0x79, 0x09 },     { 0xc8, 0x80 },
    { 0x79, 0x02 },     { 0xc8, 0xc0 },
    { 0x79, 0x03 },     { 0xc8, 0x40 },
    { 0x79, 0x05 },     { 0xc8, 0x30 },
    { 0x79, 0x26 },

    { 0xff, 0xff }, /* END MARKER */
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 * RGB656 and YUV422 come from OV; RGB444 is homebrewed.
 *
 * IMPORTANT RULE: the first entry must be for COM7, see ov7670_s_fmt for why.
 */

static struct reg8_info ov7670_fmt_yuv422[] = {
    { REG_COM7, 0x0 },  /* Selects YUV mode */
    { REG_RGB444, 0 },  /* No RGB444 please */
    { REG_COM1, 0 },    /* CCIR601 */
    { REG_COM15, COM15_R00FF },
    { REG_COM9, 0x48 }, /* 32x gain ceiling; 0x8 is reserved bit */
    { 0x4f, 0x80 },     /* "matrix coefficient 1" */
    { 0x50, 0x80 },     /* "matrix coefficient 2" */
    { 0x51,    0 },     /* vb */
    { 0x52, 0x22 },     /* "matrix coefficient 4" */
    { 0x53, 0x5e },     /* "matrix coefficient 5" */
    { 0x54, 0x80 },     /* "matrix coefficient 6" */
    { REG_COM13, COM13_GAMMA|COM13_UVSAT },
    { 0xff, 0xff },
};

static struct reg8_info ov7670_fmt_rgb565[] = {
    { REG_COM7, COM7_RGB }, /* Selects RGB mode */
    { REG_RGB444, 0 },  /* No RGB444 please */
    { REG_COM1, 0x0 },  /* CCIR601 */
    { REG_COM15, COM15_RGB565 },
    { REG_COM9, 0x38 }, /* 16x gain ceiling; 0x8 is reserved bit */
    { 0x4f, 0xb3 },     /* "matrix coefficient 1" */
    { 0x50, 0xb3 },     /* "matrix coefficient 2" */
    { 0x51,    0 },     /* vb */
    { 0x52, 0x3d },     /* "matrix coefficient 4" */
    { 0x53, 0xa7 },     /* "matrix coefficient 5" */
    { 0x54, 0xe4 },     /* "matrix coefficient 6" */
    { REG_COM13, COM13_GAMMA|COM13_UVSAT },
    { 0xff, 0xff },
};

static struct reg8_info ov7670_fmt_rgb444[] = {
    { REG_COM7, COM7_RGB }, /* Selects RGB mode */
    { REG_RGB444, R444_ENABLE },    /* Enable xxxxrrrr ggggbbbb */
    { REG_COM1, 0x0 },  /* CCIR601 */
    { REG_COM15, COM15_R01FE|COM15_RGB565 }, /* Data range needed? */
    { REG_COM9, 0x38 }, /* 16x gain ceiling; 0x8 is reserved bit */
    { 0x4f, 0xb3 },     /* "matrix coefficient 1" */
    { 0x50, 0xb3 },     /* "matrix coefficient 2" */
    { 0x51,    0 },     /* vb */
    { 0x52, 0x3d },     /* "matrix coefficient 4" */
    { 0x53, 0xa7 },     /* "matrix coefficient 5" */
    { 0x54, 0xe4 },     /* "matrix coefficient 6" */
    { REG_COM13, COM13_GAMMA|COM13_UVSAT|0x2 },  /* Magic rsvd bit */
    { 0xff, 0xff },
};

static struct reg8_info ov7670_fmt_raw[] = {
    { REG_COM7, COM7_BAYER },
    { REG_COM13, 0x08 }, /* No gamma, magic rsvd bit */
    { REG_COM16, 0x3d }, /* Edge enhancement, denoise */
    { REG_REG76, 0xe1 }, /* Pix correction, magic rsvd */
    { 0xff, 0xff },
};

/*
 * Store information about the video data format.  The color matrix
 * is deeply tied into the format, so keep the relevant values here.
 * The magic matrix numbers come from OmniVision.
 */
static struct ov7670_format_struct {
    u32 mbus_code;
    struct reg8_info *regs;
    int cmatrix[CMATRIX_LEN];
} ov7670_formats[] = {
    {
        .mbus_code  = MEDIA_BUS_FMT_YUYV8_2X8,
        .regs       = ov7670_fmt_yuv422,
        .cmatrix    = { 128, -128, 0, -34, -94, 128 },
    },
    {
        .mbus_code  = MEDIA_BUS_FMT_RGB444_2X8_PADHI_LE,
        .regs       = ov7670_fmt_rgb444,
        .cmatrix    = { 179, -179, 0, -61, -176, 228 },
    },
    {
        .mbus_code  = MEDIA_BUS_FMT_RGB565_2X8_LE,
        .regs       = ov7670_fmt_rgb565,
        .cmatrix    = { 179, -179, 0, -61, -176, 228 },
    },
    {
        .mbus_code  = MEDIA_BUS_FMT_SBGGR8_1X8,
        .regs       = ov7670_fmt_raw,
        .cmatrix    = { 0, 0, 0, 0, 0, 0 },
    },
};
#define N_OV7670_FMTS ARRAY_SIZE(ov7670_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

/*
 * QCIF mode is done (by OV) in a very strange way - it actually looks like
 * VGA with weird scaling options - they do *not* use the canned QCIF mode
 * which is allegedly provided by the sensor.  So here's the weird register
 * settings.
 */
static struct reg8_info ov7670_qcif_regs[] = {
    { REG_COM3, COM3_SCALEEN|COM3_DCWEN },
    { REG_COM3, COM3_DCWEN },
    { REG_COM14, COM14_DCWEN | 0x01 },
    { 0x73, 0xf1 },
    { 0xa2, 0x52 },
    { 0x7b, 0x1c },
    { 0x7c, 0x28 },
    { 0x7d, 0x3c },
    { 0x7f, 0x69 },
    { REG_COM9, 0x38 },
    { 0xa1, 0x0b },
    { 0x74, 0x19 },
    { 0x9a, 0x80 },
    { 0x43, 0x14 },
    { REG_COM13, 0xc0 },
    { 0xff, 0xff },
};

static struct ov7670_win_size ov7670_win_sizes[] = {
    /* VGA */
    {
        .width      = VGA_WIDTH,
        .height     = VGA_HEIGHT,
        .com7_bit   = COM7_FMT_VGA,
        .hstart     = 158,  /* These values from */
        .hstop      =  14,  /* Omnivision */
        .vstart     =  10,
        .vstop      = 490,
        .regs       = NULL,
    },
    /* CIF */
    {
        .width      = CIF_WIDTH,
        .height     = CIF_HEIGHT,
        .com7_bit   = COM7_FMT_CIF,
        .hstart     = 170,  /* Empirically determined */
        .hstop      =  90,
        .vstart     =  14,
        .vstop      = 494,
        .regs       = NULL,
    },
    /* QVGA */
    {
        .width      = QVGA_WIDTH,
        .height     = QVGA_HEIGHT,
        .com7_bit   = COM7_FMT_QVGA,
        .hstart     = 168,  /* Empirically determined */
        .hstop      =  24,
        .vstart     =  12,
        .vstop      = 492,
        .regs       = NULL,
    },
    /* QCIF */
    {
        .width      = QCIF_WIDTH,
        .height     = QCIF_HEIGHT,
        .com7_bit   = COM7_FMT_VGA, /* see comment above */
        .hstart     = 456,  /* Empirically determined */
        .hstop      =  24,
        .vstart     =  14,
        .vstop      = 494,
        .regs       = ov7670_qcif_regs,
    }
};

static struct ov7670_win_size ov7675_win_sizes[] = {
    /*
     * Currently, only VGA is supported. Theoretically it could be possible
     * to support CIF, QVGA and QCIF too. Taking values for ov7670 as a
     * base and tweak them empirically could be required.
     */
    {
        .width      = VGA_WIDTH,
        .height     = VGA_HEIGHT,
        .com7_bit   = COM7_FMT_VGA,
        .hstart     = 158,  /* These values from */
        .hstop      =  14,  /* Omnivision */
        .vstart     =  14,  /* Empirically determined */
        .vstop      = 494,
        .regs       = NULL,
    }
};

static int ov7670_read_reg(struct rt_i2c_bus_device *i2c, unsigned char reg,
                           unsigned char *val)
{
    if (rt_i2c_read_reg(i2c, OV7670_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static int ov7670_write_reg(struct rt_i2c_bus_device *i2c, unsigned char reg,
                            unsigned char val)
{
    if (rt_i2c_write_reg(i2c, OV7670_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    if (reg == REG_COM7 && (val & COM7_RESET))
        aicos_msleep(5);  /* Wait for reset to run */

    return 0;
}

/* Write a list of register settings; ff/ff stops the process. */
static int ov7670_write_array(struct ov7670_dev *sensor, struct reg8_info *vals)
{
    while (vals->reg != 0xff || vals->val != 0xff) {
        if (ov7670_write_reg(sensor->i2c, vals->reg, vals->val))
            return -1;
        vals++;
    }
    return 0;
}

static int ov7670_base_init(struct ov7670_dev *sensor)
{
    return ov7670_write_array(sensor, ov7670_default_regs);
}

static int ov7670_detect(struct ov7670_dev *sensor)
{
    unsigned char v = 0;
    int ret = 0;

    ret = ov7670_base_init(sensor);
    if (ret < 0)
        return ret;
    ret = ov7670_read_reg(sensor->i2c, REG_MIDH, &v);
    if (ret < 0)
        return ret;
    if (v != 0x7f) /* OV manuf. id. */
        return -ENODEV;
    ret = ov7670_read_reg(sensor->i2c, REG_MIDL, &v);
    if (ret < 0)
        return ret;
    if (v != 0xa2)
        return -ENODEV;
    /*
     * OK, we know we have an OmniVision chip...but which one?
     */
    ret = ov7670_read_reg(sensor->i2c, REG_PID, &v);
    if (ret < 0)
        return ret;
    if (v != 0x76)  /* PID + VER = 0x76 / 0x73 */
        return -ENODEV;
    ret = ov7670_read_reg(sensor->i2c, REG_VER, &v);
    if (ret < 0)
        return ret;
    if (v != 0x73)  /* PID + VER = 0x76 / 0x73 */
        return -ENODEV;
    return 0;
}

static int ov7675_apply_framerate(struct ov7670_dev *sensor)
{
    int ret = 0;

    ret = ov7670_write_reg(sensor->i2c, REG_CLKRC, sensor->clkrc);
    if (ret < 0)
        return ret;

    return ov7670_write_reg(sensor->i2c, REG_DBLV,
                            sensor->pll_bypass ? DBLV_BYPASS : DBLV_X4);
}

/* Store a set of start/stop values into the camera. */
static int ov7670_set_hw(struct ov7670_dev *sensor, int hstart, int hstop,
                         int vstart, int vstop)
{
    int ret = 0;
    unsigned char v = 0;
/*
 * Horizontal: 11 bits, top 8 live in hstart and hstop.  Bottom 3 of
 * hstart are in href[2:0], bottom 3 of hstop in href[5:3].  There is
 * a mystery "edge offset" value in the top two bits of href.
 */
    ret =  ov7670_write_reg(sensor->i2c, REG_HSTART, (hstart >> 3) & 0xff);
    ret += ov7670_write_reg(sensor->i2c, REG_HSTOP, (hstop >> 3) & 0xff);
    ret += ov7670_read_reg(sensor->i2c, REG_HREF, &v);
    v = (v & 0xc0) | ((hstop & 0x7) << 3) | (hstart & 0x7);
    aicos_msleep(10);
    ret += ov7670_write_reg(sensor->i2c, REG_HREF, v);
/*
 * Vertical: similar arrangement, but only 10 bits.
 */
    ret += ov7670_write_reg(sensor->i2c, REG_VSTART, (vstart >> 2) & 0xff);
    ret += ov7670_write_reg(sensor->i2c, REG_VSTOP, (vstop >> 2) & 0xff);
    ret += ov7670_read_reg(sensor->i2c, REG_VREF, &v);
    v = (v & 0xf0) | ((vstop & 0x3) << 2) | (vstart & 0x3);
    aicos_msleep(10);
    ret += ov7670_write_reg(sensor->i2c, REG_VREF, v);
    return ret;
}

static int ov7670_apply_fmt(struct ov7670_dev *sensor)
{
    struct ov7670_win_size *wsize = sensor->wsize;
    unsigned char com7 = 0, com10 = 0;
    int ret = 0;

    /*
     * COM7 is a pain in the ass, it doesn't like to be read then
     * quickly written afterward.  But we have everything we need
     * to set it absolutely here, as long as the format-specific
     * register sets list it first.
     */
    com7 = sensor->ov_fmt->regs[0].val;
    com7 |= wsize->com7_bit;
    ret = ov7670_write_reg(sensor->i2c, REG_COM7, com7);
    if (ret)
        return ret;

    /*
     * Configure the media bus through COM10 register
     */
    if (sensor->mbus_config & MEDIA_SIGNAL_VSYNC_ACTIVE_LOW)
        com10 |= COM10_VS_NEG;
    if (sensor->mbus_config & MEDIA_SIGNAL_HSYNC_ACTIVE_LOW)
        com10 |= COM10_HREF_REV;
    if (sensor->pclk_hb_disable)
        com10 |= COM10_PCLK_HB;
    ret = ov7670_write_reg(sensor->i2c, REG_COM10, com10);
    if (ret)
        return ret;

    /*
     * Now write the rest of the array.  Also store start/stops
     */
    ret = ov7670_write_array(sensor, sensor->ov_fmt->regs + 1);
    if (ret)
        return ret;

    ret = ov7670_set_hw(sensor, wsize->hstart, wsize->hstop, wsize->vstart,
                wsize->vstop);
    if (ret)
        return ret;

    if (wsize->regs) {
        ret = ov7670_write_array(sensor, wsize->regs);
        if (ret)
            return ret;
    }

    /*
     * If we're running RGB565, we must rewrite clkrc after setting
     * the other parameters or the image looks poor.  If we're *not*
     * doing RGB565, we must not rewrite clkrc or the image looks
     * *really* poor.
     *
     * (Update) Now that we retain clkrc state, we should be able
     * to write it unconditionally, and that will make the frame
     * rate persistent too.
     */
    ret = ov7670_write_reg(sensor->i2c, REG_CLKRC, sensor->clkrc);
    if (ret)
        return ret;

    return 0;
}

static int ov7670_set_framerate_legacy(struct ov7670_dev *sensor, u32 fr)
{
    int div = 1;

    sensor->clkrc = (sensor->clkrc & 0x80) | div;
    if (sensor->on)
        return ov7670_write_reg(sensor->i2c, REG_CLKRC, sensor->clkrc);

    return 0;
}

static int ov7675_set_framerate(struct ov7670_dev *sensor, u32 fr)
{
    u32 clkrc = 0;
    int pll_factor = 0;

    /*
     * The formula is fps = 5/4*pixclk for YUV/RGB and
     * fps = 5/2*pixclk for RAW.
     *
     * pixclk = clock_speed / (clkrc + 1) * PLLfactor
     *
     */
    pll_factor = sensor->pll_bypass ? 1 : PLL_FACTOR;
    clkrc = (5 * pll_factor * sensor->clock_speed * fr) / 4;
    if (sensor->ov_fmt->mbus_code == MEDIA_BUS_FMT_SBGGR8_1X8)
        clkrc = (clkrc << 1);
    clkrc--;

    /*
     * The datasheet claims that clkrc = 0 will divide the input clock by 1
     * but we've checked with an oscilloscope that it divides by 2 instead.
     * So, if clkrc = 0 just bypass the divider.
     */
    if (clkrc <= 0)
        clkrc = CLK_EXT;
    else if (clkrc > CLK_SCALE)
        clkrc = CLK_SCALE;
    sensor->clkrc = clkrc;

    /*
     * If the device is not powered up by the host driver do
     * not apply any changes to H/W at this time. Instead
     * the framerate will be restored right after power-up.
     */
    if (sensor->on)
        return ov7675_apply_framerate(sensor);

    return 0;
}

static void ov7670_power_on(struct ov7670_dev *sensor)
{
    if (sensor->on)
        return;

    if (sensor->pwdn_pin)
        camera_pin_set_low(sensor->pwdn_pin);
    if (sensor->rst_pin) {
        camera_pin_set_low(sensor->rst_pin);
        aicos_msleep(1);
        camera_pin_set_high(sensor->rst_pin);
    }
    if (sensor->pwdn_pin || sensor->rst_pin || sensor->clk)
        aicos_msleep(3);

    sensor->on = true;
}

static void ov7670_power_off(struct ov7670_dev *sensor)
{
    if (!sensor->on)
        return;

    if (sensor->pwdn_pin)
        camera_pin_set_high(sensor->pwdn_pin);

    sensor->on = false;
}

static const struct ov7670_devtype ov7670_devdata[] = {
    [MODEL_OV7670] = {
        .win_sizes = ov7670_win_sizes,
        .n_win_sizes = ARRAY_SIZE(ov7670_win_sizes),
        .set_framerate = ov7670_set_framerate_legacy,
    },
    [MODEL_OV7675] = {
        .win_sizes = ov7675_win_sizes,
        .n_win_sizes = ARRAY_SIZE(ov7675_win_sizes),
        .set_framerate = ov7675_set_framerate,
    },
};

static void ov7670_get_default_format(struct ov7670_dev *sensor)
{
    sensor->devtype = &ov7670_devdata[OV7670_DFT_MODEL];
    sensor->ov_fmt  = &ov7670_formats[OV7670_DFT_CODE];
    sensor->wsize   = &sensor->devtype->win_sizes[OV7670_DFT_WIN];
    sensor->clkrc   = 0;

    sensor->fmt.code     = sensor->ov_fmt->mbus_code;
    sensor->fmt.width    = sensor->wsize->width;
    sensor->fmt.height   = sensor->wsize->height;
    sensor->fmt.bus_type = OV7670_DFT_BUS_TYPE;
    sensor->fmt.flags    = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                           MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                           MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;
}

static rt_err_t ov7670_init(rt_device_t dev)
{
    int ret = 0;
    struct ov7670_dev *sensor = &g_ov7670_dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->clock_speed = camera_xclk_rate_get() / 1000000;
    if (sensor->clock_speed < 10 || sensor->clock_speed > 48) {
        LOG_E("XCLK rate %d is out of range", sensor->clock_speed);
        return -EINVAL;
    }

    ov7670_get_default_format(sensor);

    sensor->rst_pin = camera_rst_pin_get();
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->rst_pin || !sensor->pwdn_pin)
        return -RT_EINVAL;

    ov7670_power_on(sensor);
    ret = ov7670_detect(sensor);
    if (ret) {
        LOG_E("chip found @ 0x%x (i2c%d) is not an ov7670 chip.\n",
              OV7670_I2C_SLAVE_ID, AIC_CAMERA_I2C_CHAN);
        goto power_off;
    }

    sensor->devtype->set_framerate(sensor, OV7670_DFT_FR);

    LOG_I("OV7670 inited");

power_off:
    ov7670_power_off(sensor);
    return ret;
}

static rt_err_t ov7670_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct ov7670_dev *sensor = (struct ov7670_dev *)dev;

    ov7670_power_on(sensor);
    ov7670_base_init(sensor);
    ov7670_apply_fmt(sensor);
    ov7675_apply_framerate(sensor);

    LOG_I("OV7670 Open");
    return RT_EOK;
}

static rt_err_t ov7670_close(rt_device_t dev)
{
    struct ov7670_dev *sensor = (struct ov7670_dev *)dev;

    ov7670_power_off(sensor);
    LOG_I("OV7670 Close");
    return RT_EOK;
}

static int ov7670_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct ov7670_dev *sensor = (struct ov7670_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int ov7670_start(rt_device_t dev)
{
    return 0;
}

static int ov7670_stop(rt_device_t dev)
{
    return 0;
}

static rt_err_t ov7670_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return ov7670_start(dev);
    case CAMERA_CMD_STOP:
        return ov7670_stop(dev);
    case CAMERA_CMD_GET_FMT:
        return ov7670_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops ov7670_ops =
{
    .init = ov7670_init,
    .open = ov7670_open,
    .close = ov7670_close,
    .control = ov7670_control,
};
#endif

int rt_hw_ov7670_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_ov7670_dev.dev.ops = &ov7670_ops;
#else
    g_ov7670_dev.dev.init = ov7670_init;
    g_ov7670_dev.dev.open = ov7670_open;
    g_ov7670_dev.dev.close = ov7670_close;
    g_ov7670_dev.dev.control = ov7670_control;
#endif
    g_ov7670_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_ov7670_dev.dev, CAMERA_DEV_NAME, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_ov7670_init);
