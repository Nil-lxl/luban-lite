/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#define LOG_TAG     "ov2640"

#include <drivers/i2c.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "mpp_types.h"
#include "mpp_img_size.h"
#include "mpp_vin.h"

#include "drv_camera.h"
#include "camera_inner.h"

/* Default format configuration of OV2640 */
#define OV2640_DFT_WIN          OV2640_WIN_VGA
#define OV2640_DFT_BUS_TYPE     MEDIA_BUS_PARALLEL
#define OV2640_DFT_CODE         MEDIA_BUS_FMT_UYVY8_2X8

/* The clk source is decided by board design */
#define CAMERA_CLK_SRC          CLK_OUT1

#define OV2640_I2C_SLAVE_ID     0x30

#define VAL_SET(x, mask, rshift, lshift)    ((((x) >> rshift) & mask) << lshift)
/*
 * DSP registers
 * register offset for BANK_SEL == BANK_SEL_DSP
 */
#define R_BYPASS    0x05 /* Bypass DSP */
#define   R_BYPASS_DSP_BYPAS    0x01 /* Bypass DSP, sensor out directly */
#define   R_BYPASS_USE_DSP      0x00 /* Use the internal DSP */
#define QS          0x44 /* Quantization Scale Factor */
#define CTRLI       0x50
#define   CTRLI_LP_DP           0x80
#define   CTRLI_ROUND           0x40
#define   CTRLI_V_DIV_SET(x)    VAL_SET(x, 0x3, 0, 3)
#define   CTRLI_H_DIV_SET(x)    VAL_SET(x, 0x3, 0, 0)
#define HSIZE       0x51 /* H_SIZE[7:0] (real/4) */
#define   HSIZE_SET(x)          VAL_SET(x, 0xFF, 2, 0)
#define VSIZE       0x52 /* V_SIZE[7:0] (real/4) */
#define   VSIZE_SET(x)          VAL_SET(x, 0xFF, 2, 0)
#define XOFFL       0x53 /* OFFSET_X[7:0] */
#define   XOFFL_SET(x)          VAL_SET(x, 0xFF, 0, 0)
#define YOFFL       0x54 /* OFFSET_Y[7:0] */
#define   YOFFL_SET(x)          VAL_SET(x, 0xFF, 0, 0)
#define VHYX        0x55 /* Offset and size completion */
#define   VHYX_VSIZE_SET(x)     VAL_SET(x, 0x1, (8+2), 7)
#define   VHYX_HSIZE_SET(x)     VAL_SET(x, 0x1, (8+2), 3)
#define   VHYX_YOFF_SET(x)      VAL_SET(x, 0x3, 8, 4)
#define   VHYX_XOFF_SET(x)      VAL_SET(x, 0x3, 8, 0)
#define DPRP        0x56
#define TEST        0x57 /* Horizontal size completion */
#define   TEST_HSIZE_SET(x)     VAL_SET(x, 0x1, (9+2), 7)
#define ZMOW        0x5A /* Zoom: Out Width  OUTW[7:0] (real/4) */
#define   ZMOW_OUTW_SET(x)      VAL_SET(x, 0xFF, 2, 0)
#define ZMOH        0x5B /* Zoom: Out Height OUTH[7:0] (real/4) */
#define   ZMOH_OUTH_SET(x)      VAL_SET(x, 0xFF, 2, 0)
#define ZMHH        0x5C /* Zoom: Speed and H&W completion */
#define   ZMHH_ZSPEED_SET(x)    VAL_SET(x, 0x0F, 0, 4)
#define   ZMHH_OUTH_SET(x)      VAL_SET(x, 0x1, (8+2), 2)
#define   ZMHH_OUTW_SET(x)      VAL_SET(x, 0x3, (8+2), 0)
#define BPADDR      0x7C /* SDE Indirect Register Access: Address */
#define BPDATA      0x7D /* SDE Indirect Register Access: Data */
#define CTRL2       0x86 /* DSP Module enable 2 */
#define   CTRL2_DCW_EN          0x20
#define   CTRL2_SDE_EN          0x10
#define   CTRL2_UV_ADJ_EN       0x08
#define   CTRL2_UV_AVG_EN       0x04
#define   CTRL2_CMX_EN          0x01
#define CTRL3       0x87 /* DSP Module enable 3 */
#define   CTRL3_BPC_EN          0x80
#define   CTRL3_WPC_EN          0x40
#define SIZEL       0x8C /* Image Size Completion */
#define   SIZEL_HSIZE8_11_SET(x) VAL_SET(x, 0x1, 11, 6)
#define   SIZEL_HSIZE8_SET(x)    VAL_SET(x, 0x7, 0, 3)
#define   SIZEL_VSIZE8_SET(x)    VAL_SET(x, 0x7, 0, 0)
#define HSIZE8      0xC0 /* Image Horizontal Size HSIZE[10:3] */
#define   HSIZE8_SET(x)         VAL_SET(x, 0xFF, 3, 0)
#define VSIZE8      0xC1 /* Image Vertical Size VSIZE[10:3] */
#define   VSIZE8_SET(x)         VAL_SET(x, 0xFF, 3, 0)
#define CTRL0       0xC2 /* DSP Module enable 0 */
#define   CTRL0_AEC_EN       0x80
#define   CTRL0_AEC_SEL      0x40
#define   CTRL0_STAT_SEL     0x20
#define   CTRL0_VFIRST       0x10
#define   CTRL0_YUV422       0x08
#define   CTRL0_YUV_EN       0x04
#define   CTRL0_RGB_EN       0x02
#define   CTRL0_RAW_EN       0x01
#define CTRL1       0xC3 /* DSP Module enable 1 */
#define   CTRL1_CIP          0x80
#define   CTRL1_DMY          0x40
#define   CTRL1_RAW_GMA      0x20
#define   CTRL1_DG           0x10
#define   CTRL1_AWB          0x08
#define   CTRL1_AWB_GAIN     0x04
#define   CTRL1_LENC         0x02
#define   CTRL1_PRE          0x01
/*      REG 0xC7 (unknown name): affects Auto White Balance (AWB)
 *    AWB_OFF            0x40
 *    AWB_SIMPLE         0x10
 *    AWB_ON             0x00   (Advanced AWB ?) */
#define R_DVP_SP    0xD3 /* DVP output speed control */
#define   R_DVP_SP_AUTO_MODE 0x80
 /* DVP PCLK = sysclk (48)/[6:0] (YUV0);
  *          = sysclk (48)/(2*[6:0]) (RAW);*/
#define   R_DVP_SP_DVP_MASK  0x3F
#define IMAGE_MODE  0xDA /* Image Output Format Select */
#define   IMAGE_MODE_Y8_DVP_EN   0x40
#define   IMAGE_MODE_JPEG_EN     0x10
#define   IMAGE_MODE_YUV422      0x00
#define   IMAGE_MODE_RAW10       0x04 /* (DVP) */
#define   IMAGE_MODE_RGB565      0x08
/* HREF timing select in DVP JPEG output mode (0 for HREF is same as sensor) */
#define   IMAGE_MODE_HREF_VSYNC  0x02
/* Byte swap enable for DVP
 * 1: Low byte first UYVY (C2[4] =0)
 *     VYUY (C2[4] =1)
 * 0: High byte first YUYV (C2[4]=0)
 *     YVYU (C2[4] = 1) */
#define   IMAGE_MODE_LBYTE_FIRST 0x01
#define RESET       0xE0 /* Reset */
#define   RESET_MICROC       0x40
#define   RESET_SCCB         0x20
#define   RESET_JPEG         0x10
#define   RESET_DVP          0x04
#define   RESET_IPU          0x02
#define   RESET_CIF          0x01
#define REGED       0xED /* Register ED */
#define   REGED_CLK_OUT_DIS  0x10
#define MS_SP       0xF0 /* SCCB Master Speed */
#define SS_ID       0xF7 /* SCCB Slave ID */
#define SS_CTRL     0xF8 /* SCCB Slave Control */
#define   SS_CTRL_ADD_AUTO_INC  0x20
#define   SS_CTRL_EN            0x08
#define   SS_CTRL_DELAY_CLK     0x04
#define   SS_CTRL_ACC_EN        0x02
#define   SS_CTRL_SEN_PASS_THR  0x01
#define MC_BIST     0xF9 /* Microcontroller misc register */
#define   MC_BIST_RESET           0x80 /* Microcontroller Reset */
#define   MC_BIST_BOOT_ROM_SEL    0x40
#define   MC_BIST_12KB_SEL        0x20
#define   MC_BIST_12KB_MASK       0x30
#define   MC_BIST_512KB_SEL       0x08
#define   MC_BIST_512KB_MASK      0x0C
#define   MC_BIST_BUSY_BIT_R      0x02
#define   MC_BIST_MC_RES_ONE_SH_W 0x02
#define   MC_BIST_LAUNCH          0x01
#define BANK_SEL    0xFF /* Register Bank Select */
#define   BANK_SEL_DSP     0x00
#define   BANK_SEL_SENS    0x01

/*
 * Sensor registers
 * register offset for BANK_SEL == BANK_SEL_SENS
 */
#define GAIN        0x00 /* AGC - Gain control gain setting */
#define COM1        0x03 /* Common control 1 */
#define   COM1_1_DUMMY_FR          0x40
#define   COM1_3_DUMMY_FR          0x80
#define   COM1_7_DUMMY_FR          0xC0
#define   COM1_VWIN_LSB_UXGA       0x0F
#define   COM1_VWIN_LSB_SVGA       0x0A
#define   COM1_VWIN_LSB_CIF        0x06
#define REG04       0x04 /* Register 04 */
#define   REG04_DEF             0x20 /* Always set */
#define   REG04_HFLIP_IMG       0x80 /* Horizontal mirror image ON/OFF */
#define   REG04_VFLIP_IMG       0x40 /* Vertical flip image ON/OFF */
#define   REG04_VREF_EN         0x10
#define   REG04_HREF_EN         0x08
#define   REG04_AEC_SET(x)      VAL_SET(x, 0x3, 0, 0)
#define REG08       0x08 /* Frame Exposure One-pin Control Pre-charge Row Num */
#define COM2        0x09 /* Common control 2 */
#define   COM2_SOFT_SLEEP_MODE  0x10 /* Soft sleep mode */
                     /* Output drive capability */
#define   COM2_OCAP_Nx_SET(N)   (((N) - 1) & 0x03) /* N = [1x .. 4x] */
#define PID         0x0A /* Product ID Number MSB */
#define VER         0x0B /* Product ID Number LSB */
#define COM3        0x0C /* Common control 3 */
#define   COM3_BAND_50H        0x04 /* 0 For Banding at 60H */
#define   COM3_BAND_AUTO       0x02 /* Auto Banding */
/* 0 For enable live video output after the snapshot sequence*/
#define   COM3_SING_FR_SNAPSH  0x01
#define AEC         0x10 /* AEC[9:2] Exposure Value */
#define CLKRC       0x11 /* Internal clock */
#define   CLKRC_EN             0x80
#define   CLKRC_DIV_SET(x)     (((x) - 1) & 0x1F) /* CLK = XVCLK/(x) */
#define COM7        0x12 /* Common control 7 */
/* Initiates system reset. All registers are set to factory default values
 * after which the chip resumes normal operation */
#define   COM7_SRST            0x80
#define   COM7_RES_UXGA        0x00 /* Resolution selectors for UXGA */
#define   COM7_RES_SVGA        0x40 /* SVGA */
#define   COM7_RES_CIF         0x20 /* CIF */
#define   COM7_ZOOM_EN         0x04 /* Enable Zoom mode */
#define   COM7_COLOR_BAR_TEST  0x02 /* Enable Color Bar Test Pattern */
#define COM8        0x13 /* Common control 8 */
#define   COM8_DEF             0xC0
#define   COM8_BNDF_EN         0x20 /* Banding filter ON/OFF */
#define   COM8_AGC_EN          0x04 /* AGC Auto/Manual control selection */
#define   COM8_AEC_EN          0x01 /* Auto/Manual Exposure control */
/* Common control 9 Automatic gain ceiling - maximum AGC value [7:5]*/
#define COM9        0x14
#define   COM9_AGC_GAIN_2x     0x00 /* 000 :   2x */
#define   COM9_AGC_GAIN_4x     0x20 /* 001 :   4x */
#define   COM9_AGC_GAIN_8x     0x40 /* 010 :   8x */
#define   COM9_AGC_GAIN_16x    0x60 /* 011 :  16x */
#define   COM9_AGC_GAIN_32x    0x80 /* 100 :  32x */
#define   COM9_AGC_GAIN_64x    0xA0 /* 101 :  64x */
#define   COM9_AGC_GAIN_128x   0xC0 /* 110 : 128x */
#define COM10       0x15 /* Common control 10 */
#define   COM10_PCLK_HREF      0x20 /* PCLK output qualified by HREF */
/* Data is updated at the rising edge of PCLK
 * (user can latch data at the next falling edge of PCLK) 0 otherwise. */
#define   COM10_PCLK_RISE      0x10
/* Invert HREF polarity: HREF negative for valid data */
#define   COM10_HREF_INV       0x08
#define   COM10_VSINC_INV      0x02 /* Invert VSYNC polarity */
#define HSTART      0x17 /* Horizontal Window start MSB 8 bit */
#define HEND        0x18 /* Horizontal Window end MSB 8 bit */
#define VSTART      0x19 /* Vertical Window start MSB 8 bit */
#define VEND        0x1A /* Vertical Window end MSB 8 bit */
#define MIDH        0x1C /* Manufacturer ID byte - high */
#define MIDL        0x1D /* Manufacturer ID byte - low  */
#define AEW         0x24 /* AGC/AEC - Stable operating region (upper limit) */
#define AEB         0x25 /* AGC/AEC - Stable operating region (lower limit) */
#define VV          0x26 /* AGC/AEC Fast mode operating region */
#define   VV_HIGH_TH_SET(x)      VAL_SET(x, 0xF, 0, 4)
#define   VV_LOW_TH_SET(x)       VAL_SET(x, 0xF, 0, 0)
#define REG2A       0x2A /* Dummy pixel insert MSB */
#define FRARL       0x2B /* Dummy pixel insert LSB */
#define ADDVFL      0x2D /* LSB of insert dummy lines in Vertical direction */
#define ADDVFH      0x2E /* MSB of insert dummy lines in Vertical direction */
#define YAVG        0x2F /* Y/G Channel Average value */
#define REG32       0x32 /* Common Control 32 */
#define   REG32_PCLK_DIV_2    0x80 /* PCLK freq divided by 2 */
#define   REG32_PCLK_DIV_4    0xC0 /* PCLK freq divided by 4 */
#define ARCOM2      0x34 /* Zoom: Horizontal start point */
#define REG45       0x45 /* Register 45 */
#define FLL         0x46 /* Frame Length Adjustment LSBs */
#define FLH         0x47 /* Frame Length Adjustment MSBs */
#define COM19       0x48 /* Zoom: Vertical start point */
#define ZOOMS       0x49 /* Zoom: Vertical start point */
#define COM22       0x4B /* Flash light control */
#define COM25       0x4E /* For Banding operations */
#define   COM25_50HZ_BANDING_AEC_MSBS_MASK      0xC0 /* 50Hz Bd. AEC 2 MSBs */
#define   COM25_60HZ_BANDING_AEC_MSBS_MASK      0x30 /* 60Hz Bd. AEC 2 MSBs */
#define   COM25_50HZ_BANDING_AEC_MSBS_SET(x)    VAL_SET(x, 0x3, 8, 6)
#define   COM25_60HZ_BANDING_AEC_MSBS_SET(x)    VAL_SET(x, 0x3, 8, 4)
#define BD50        0x4F /* 50Hz Banding AEC 8 LSBs */
#define   BD50_50HZ_BANDING_AEC_LSBS_SET(x)     VAL_SET(x, 0xFF, 0, 0)
#define BD60        0x50 /* 60Hz Banding AEC 8 LSBs */
#define   BD60_60HZ_BANDING_AEC_LSBS_SET(x)     VAL_SET(x, 0xFF, 0, 0)
#define REG5A       0x5A /* 50/60Hz Banding Maximum AEC Step */
#define   BD50_MAX_AEC_STEP_MASK         0xF0 /* 50Hz Banding Max. AEC Step */
#define   BD60_MAX_AEC_STEP_MASK         0x0F /* 60Hz Banding Max. AEC Step */
#define   BD50_MAX_AEC_STEP_SET(x)       VAL_SET((x - 1), 0x0F, 0, 4)
#define   BD60_MAX_AEC_STEP_SET(x)       VAL_SET((x - 1), 0x0F, 0, 0)
#define REG5D       0x5D /* AVGsel[7:0],   16-zone average weight option */
#define REG5E       0x5E /* AVGsel[15:8],  16-zone average weight option */
#define REG5F       0x5F /* AVGsel[23:16], 16-zone average weight option */
#define REG60       0x60 /* AVGsel[31:24], 16-zone average weight option */
#define HISTO_LOW   0x61 /* Histogram Algorithm Low Level */
#define HISTO_HIGH  0x62 /* Histogram Algorithm High Level */

/* ID */
#define MANUFACTURER_ID 0x7FA2
#define PID_OV2640  0x2642
#define VERSION(pid, ver) ((pid << 8) | (ver & 0xFF))

enum ov2640_win_type {
    OV2640_WIN_QCIF,
    OV2640_WIN_QVGA,
    OV2640_WIN_CIF,
    OV2640_WIN_VGA,
    OV2640_WIN_SVGA,
    OV2640_WIN_XGA,
    OV2640_WIN_SXGA,
    OV2640_WIN_UXGA,
};

struct ov2640_win_size {
    char           *name;
    u32             width;
    u32             height;
    const struct reg8_info *regs;
};

struct ov2640_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 xclk_freq;
    u32 rst_pin;
    u32 pwdn_pin;

    struct mpp_video_fmt fmt;
    u32 cfmt_code;
    const struct ov2640_win_size    *win;

    bool streaming;
    int power_count;
};

static struct ov2640_dev g_ov2640_dev = {0};

/* Registers settings */

#define ENDMARKER { 0xff, 0xff }

static const struct reg8_info ov2640_init_regs[] = {
    { BANK_SEL, BANK_SEL_DSP },
    { 0x2c,   0xff },
    { 0x2e,   0xdf },
    { BANK_SEL, BANK_SEL_SENS },
    { 0x3c,   0x32 },
    { CLKRC,  CLKRC_DIV_SET(1) },
    { COM2,   COM2_OCAP_Nx_SET(3) },
    { REG04,  REG04_DEF | REG04_HREF_EN },
    { COM8,   COM8_DEF | COM8_BNDF_EN | COM8_AGC_EN | COM8_AEC_EN },
    { COM9,   COM9_AGC_GAIN_8x | 0x08 },
    { 0x2c,   0x0c },
    { 0x33,   0x78 },
    { 0x3a,   0x33 },
    { 0x3b,   0xfb },
    { 0x3e,   0x00 },
    { 0x43,   0x11 },
    { 0x16,   0x10 },
    { 0x39,   0x02 },
    { 0x35,   0x88 },
    { 0x22,   0x0a },
    { 0x37,   0x40 },
    { 0x23,   0x00 },
    { ARCOM2, 0xa0 },
    { 0x06,   0x02 },
    { 0x06,   0x88 },
    { 0x07,   0xc0 },
    { 0x0d,   0xb7 },
    { 0x0e,   0x01 },
    { 0x4c,   0x00 },
    { 0x4a,   0x81 },
    { 0x21,   0x99 },
    { AEW,    0x40 },
    { AEB,    0x38 },
    { VV,     VV_HIGH_TH_SET(0x08) | VV_LOW_TH_SET(0x02) },
    { 0x5c,   0x00 },
    { 0x63,   0x00 },
    { FLL,    0x22 },
    { COM3,   0x38 | COM3_BAND_AUTO },
    { REG5D,  0x55 },
    { REG5E,  0x7d },
    { REG5F,  0x7d },
    { REG60,  0x55 },
    { HISTO_LOW,   0x70 },
    { HISTO_HIGH,  0x80 },
    { 0x7c,   0x05 },
    { 0x20,   0x80 },
    { 0x28,   0x30 },
    { 0x6c,   0x00 },
    { 0x6d,   0x80 },
    { 0x6e,   0x00 },
    { 0x70,   0x02 },
    { 0x71,   0x94 },
    { 0x73,   0xc1 },
    { 0x3d,   0x34 },
    { COM7,   COM7_RES_UXGA | COM7_ZOOM_EN },
    { REG5A,  BD50_MAX_AEC_STEP_SET(6)
           | BD60_MAX_AEC_STEP_SET(8) },        /* 0x57 */
    { COM25,  COM25_50HZ_BANDING_AEC_MSBS_SET(0x0bb)
           | COM25_60HZ_BANDING_AEC_MSBS_SET(0x09c) },  /* 0x00 */
    { BD50,   BD50_50HZ_BANDING_AEC_LSBS_SET(0x0bb) },  /* 0xbb */
    { BD60,   BD60_60HZ_BANDING_AEC_LSBS_SET(0x09c) },  /* 0x9c */
    { BANK_SEL,  BANK_SEL_DSP },
    { 0xe5,   0x7f },
    { MC_BIST,  MC_BIST_RESET | MC_BIST_BOOT_ROM_SEL },
    { 0x41,   0x24 },
    { RESET,  RESET_JPEG | RESET_DVP },
    { 0x76,   0xff },
    { 0x33,   0xa0 },
    { 0x42,   0x20 },
    { 0x43,   0x18 },
    { 0x4c,   0x00 },
    { CTRL3,  CTRL3_BPC_EN | CTRL3_WPC_EN | 0x10 },
    { 0x88,   0x3f },
    { 0xd7,   0x03 },
    { 0xd9,   0x10 },
    { R_DVP_SP,  R_DVP_SP_AUTO_MODE | 0x2 },
    { 0xc8,   0x08 },
    { 0xc9,   0x80 },
    { BPADDR, 0x00 },
    { BPDATA, 0x00 },
    { BPADDR, 0x03 },
    { BPDATA, 0x48 },
    { BPDATA, 0x48 },
    { BPADDR, 0x08 },
    { BPDATA, 0x20 },
    { BPDATA, 0x10 },
    { BPDATA, 0x0e },
    { 0x90,   0x00 },
    { 0x91,   0x0e },
    { 0x91,   0x1a },
    { 0x91,   0x31 },
    { 0x91,   0x5a },
    { 0x91,   0x69 },
    { 0x91,   0x75 },
    { 0x91,   0x7e },
    { 0x91,   0x88 },
    { 0x91,   0x8f },
    { 0x91,   0x96 },
    { 0x91,   0xa3 },
    { 0x91,   0xaf },
    { 0x91,   0xc4 },
    { 0x91,   0xd7 },
    { 0x91,   0xe8 },
    { 0x91,   0x20 },
    { 0x92,   0x00 },
    { 0x93,   0x06 },
    { 0x93,   0xe3 },
    { 0x93,   0x03 },
    { 0x93,   0x03 },
    { 0x93,   0x00 },
    { 0x93,   0x02 },
    { 0x93,   0x00 },
    { 0x93,   0x00 },
    { 0x93,   0x00 },
    { 0x93,   0x00 },
    { 0x93,   0x00 },
    { 0x93,   0x00 },
    { 0x93,   0x00 },
    { 0x96,   0x00 },
    { 0x97,   0x08 },
    { 0x97,   0x19 },
    { 0x97,   0x02 },
    { 0x97,   0x0c },
    { 0x97,   0x24 },
    { 0x97,   0x30 },
    { 0x97,   0x28 },
    { 0x97,   0x26 },
    { 0x97,   0x02 },
    { 0x97,   0x98 },
    { 0x97,   0x80 },
    { 0x97,   0x00 },
    { 0x97,   0x00 },
    { 0xa4,   0x00 },
    { 0xa8,   0x00 },
    { 0xc5,   0x11 },
    { 0xc6,   0x51 },
    { 0xbf,   0x80 },
    { 0xc7,   0x10 },   /* simple AWB */
    { 0xb6,   0x66 },
    { 0xb8,   0xA5 },
    { 0xb7,   0x64 },
    { 0xb9,   0x7C },
    { 0xb3,   0xaf },
    { 0xb4,   0x97 },
    { 0xb5,   0xFF },
    { 0xb0,   0xC5 },
    { 0xb1,   0x94 },
    { 0xb2,   0x0f },
    { 0xc4,   0x5c },
    { 0xa6,   0x00 },
    { 0xa7,   0x20 },
    { 0xa7,   0xd8 },
    { 0xa7,   0x1b },
    { 0xa7,   0x31 },
    { 0xa7,   0x00 },
    { 0xa7,   0x18 },
    { 0xa7,   0x20 },
    { 0xa7,   0xd8 },
    { 0xa7,   0x19 },
    { 0xa7,   0x31 },
    { 0xa7,   0x00 },
    { 0xa7,   0x18 },
    { 0xa7,   0x20 },
    { 0xa7,   0xd8 },
    { 0xa7,   0x19 },
    { 0xa7,   0x31 },
    { 0xa7,   0x00 },
    { 0xa7,   0x18 },
    { 0x7f,   0x00 },
    { 0xe5,   0x1f },
    { 0xe1,   0x77 },
    { 0xdd,   0x7f },
    { CTRL0,  CTRL0_YUV422 | CTRL0_YUV_EN | CTRL0_RGB_EN },
    ENDMARKER,
};

/*
 * Register settings for window size
 * The preamble, setup the internal DSP to input an UXGA (1600x1200) image.
 * Then the different zooming configurations will setup the output image size.
 */
static const struct reg8_info ov2640_size_change_preamble_regs[] = {
    { BANK_SEL, BANK_SEL_DSP },
    { RESET, RESET_DVP },
    { SIZEL, SIZEL_HSIZE8_11_SET(UXGA_WIDTH) |
         SIZEL_HSIZE8_SET(UXGA_WIDTH) |
         SIZEL_VSIZE8_SET(UXGA_HEIGHT) },
    { HSIZE8, HSIZE8_SET(UXGA_WIDTH) },
    { VSIZE8, VSIZE8_SET(UXGA_HEIGHT) },
    { CTRL2, CTRL2_DCW_EN | CTRL2_SDE_EN |
         CTRL2_UV_AVG_EN | CTRL2_CMX_EN | CTRL2_UV_ADJ_EN },
    { HSIZE, HSIZE_SET(UXGA_WIDTH) },
    { VSIZE, VSIZE_SET(UXGA_HEIGHT) },
    { XOFFL, XOFFL_SET(0) },
    { YOFFL, YOFFL_SET(0) },
    { VHYX, VHYX_HSIZE_SET(UXGA_WIDTH) | VHYX_VSIZE_SET(UXGA_HEIGHT) |
        VHYX_XOFF_SET(0) | VHYX_YOFF_SET(0)},
    { TEST, TEST_HSIZE_SET(UXGA_WIDTH) },
    ENDMARKER,
};

#define PER_SIZE_REG_SEQ(x, y, v_div, h_div, pclk_div)  \
    { CTRLI, CTRLI_LP_DP | CTRLI_V_DIV_SET(v_div) | \
         CTRLI_H_DIV_SET(h_div)},       \
    { ZMOW, ZMOW_OUTW_SET(x) },         \
    { ZMOH, ZMOH_OUTH_SET(y) },         \
    { ZMHH, ZMHH_OUTW_SET(x) | ZMHH_OUTH_SET(y) },  \
    { R_DVP_SP, pclk_div },             \
    { RESET, 0x00 }

static const struct reg8_info ov2640_qcif_regs[] = {
    PER_SIZE_REG_SEQ(QCIF_WIDTH, QCIF_HEIGHT, 3, 3, 4),
    ENDMARKER,
};

static const struct reg8_info ov2640_qvga_regs[] = {
    PER_SIZE_REG_SEQ(QVGA_WIDTH, QVGA_HEIGHT, 2, 2, 4),
    ENDMARKER,
};

static const struct reg8_info ov2640_cif_regs[] = {
    PER_SIZE_REG_SEQ(CIF_WIDTH, CIF_HEIGHT, 2, 2, 8),
    ENDMARKER,
};

static const struct reg8_info ov2640_vga_regs[] = {
    PER_SIZE_REG_SEQ(VGA_WIDTH, VGA_HEIGHT, 0, 0, 2),
    ENDMARKER,
};

static const struct reg8_info ov2640_svga_regs[] = {
    PER_SIZE_REG_SEQ(SVGA_WIDTH, SVGA_HEIGHT, 1, 1, 2),
    ENDMARKER,
};

static const struct reg8_info ov2640_xga_regs[] = {
    PER_SIZE_REG_SEQ(XGA_WIDTH, XGA_HEIGHT, 0, 0, 2),
    { CTRLI,    0x00 },
    ENDMARKER,
};

static const struct reg8_info ov2640_sxga_regs[] = {
    PER_SIZE_REG_SEQ(SXGA_WIDTH, SXGA_HEIGHT, 0, 0, 2),
    { CTRLI,    0x00 },
    { R_DVP_SP, 2 | R_DVP_SP_AUTO_MODE },
    ENDMARKER,
};

static const struct reg8_info ov2640_uxga_regs[] = {
    PER_SIZE_REG_SEQ(UXGA_WIDTH, UXGA_HEIGHT, 0, 0, 0),
    { CTRLI,    0x00 },
    { R_DVP_SP, 0 | R_DVP_SP_AUTO_MODE },
    ENDMARKER,
};

#define OV2640_SIZE(n, w, h, r) \
    { .name = n, .width = w , .height = h, .regs = r }

static const struct ov2640_win_size ov2640_supported_win_sizes[] = {
    OV2640_SIZE("QCIF", QCIF_WIDTH, QCIF_HEIGHT, ov2640_qcif_regs),
    OV2640_SIZE("QVGA", QVGA_WIDTH, QVGA_HEIGHT, ov2640_qvga_regs),
    OV2640_SIZE("CIF", CIF_WIDTH, CIF_HEIGHT, ov2640_cif_regs),
    OV2640_SIZE("VGA", VGA_WIDTH, VGA_HEIGHT, ov2640_vga_regs),
    OV2640_SIZE("SVGA", SVGA_WIDTH, SVGA_HEIGHT, ov2640_svga_regs),
    OV2640_SIZE("XGA", XGA_WIDTH, XGA_HEIGHT, ov2640_xga_regs),
    OV2640_SIZE("SXGA", SXGA_WIDTH, SXGA_HEIGHT, ov2640_sxga_regs),
    OV2640_SIZE("UXGA", UXGA_WIDTH, UXGA_HEIGHT, ov2640_uxga_regs),
};

/* Register settings for pixel formats */
static const struct reg8_info ov2640_format_change_preamble_regs[] = {
    { BANK_SEL, BANK_SEL_DSP },
    { R_BYPASS, R_BYPASS_USE_DSP },
    ENDMARKER,
};

static const struct reg8_info ov2640_yuyv_regs[] = {
    { IMAGE_MODE, IMAGE_MODE_YUV422 },
    { 0xd7, 0x03 },
    { 0x33, 0xa0 },
    { 0xe5, 0x1f },
    { 0xe1, 0x67 },
    { RESET,  0x00 },
    { R_BYPASS, R_BYPASS_USE_DSP },
    ENDMARKER,
};

static const struct reg8_info ov2640_uyvy_regs[] = {
    { IMAGE_MODE, IMAGE_MODE_LBYTE_FIRST | IMAGE_MODE_YUV422 },
    { 0xd7, 0x01 },
    { 0x33, 0xa0 },
    { 0xe1, 0x67 },
    { RESET,  0x00 },
    { R_BYPASS, R_BYPASS_USE_DSP },
    ENDMARKER,
};

static const struct reg8_info ov2640_rgb565_be_regs[] = {
    { IMAGE_MODE, IMAGE_MODE_RGB565 },
    { 0xd7, 0x03 },
    { RESET,  0x00 },
    { R_BYPASS, R_BYPASS_USE_DSP },
    ENDMARKER,
};

static const struct reg8_info ov2640_rgb565_le_regs[] = {
    { IMAGE_MODE, IMAGE_MODE_LBYTE_FIRST | IMAGE_MODE_RGB565 },
    { 0xd7, 0x03 },
    { RESET,  0x00 },
    { R_BYPASS, R_BYPASS_USE_DSP },
    ENDMARKER,
};

static int ov2640_write_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(i2c, OV2640_I2C_SLAVE_ID, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static int ov2640_read_reg(struct rt_i2c_bus_device *i2c, u8 reg, u8 *val)
{
    if (rt_i2c_read_reg(i2c, OV2640_I2C_SLAVE_ID, reg, val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static int ov2640_write_array(struct rt_i2c_bus_device *i2c,
                              const struct reg8_info *vals)
{
    int ret;

    while ((vals->reg != 0xff) || (vals->val != 0xff)) {
        ret = ov2640_write_reg(i2c, vals->reg, vals->val);
        LOG_D("array: 0x%02x, 0x%02x", vals->reg, vals->val);

        if (ret < 0)
            return ret;
        vals++;
    }
    return 0;
}

static int ov2640_mask_set(struct rt_i2c_bus_device *i2c,
                           u8 reg, u8 mask, u8 set)
{
    u8 val = 0;
    s32 ret = 0;

    ret = ov2640_read_reg(i2c, reg, &val);
    if (ret < 0)
        return ret;

    val &= ~mask;
    val |= set & mask;
    LOG_D("masks: 0x%02x, 0x%02x", reg, val);

    return ov2640_write_reg(i2c, reg, val);
}

static int ov2640_reset(struct rt_i2c_bus_device *i2c)
{
    int ret = 0;
    static const struct reg8_info reset_seq[] = {
        {BANK_SEL, BANK_SEL_SENS},
        {COM7, COM7_SRST},
        ENDMARKER,
    };

    ret = ov2640_write_array(i2c, reset_seq);
    if (ret)
        goto err;

    aicos_msleep(5);
err:
    LOG_D("%s: (ret %d)", __func__, ret);
    return ret;
}

static void ov2640_set_power(struct ov2640_dev *sensor, int on)
{
    if (sensor->power_count == on)
        return;

    if (on)
        camera_pin_set_low(sensor->pwdn_pin);
    else
        camera_pin_set_high(sensor->pwdn_pin);

    if (on && sensor->rst_pin) {
        /* Active the reset pin to perform a reset pulse */
        camera_pin_set_low(sensor->rst_pin);
        rt_thread_mdelay(5);
        camera_pin_set_high(sensor->rst_pin);
    }

    sensor->power_count += on ? 1 : -1;
    if (sensor->power_count < 0)
        LOG_E("Invalid power count: %d\n", sensor->power_count);
}

static int ov2640_set_params(struct rt_i2c_bus_device *i2c,
                             const struct ov2640_win_size *win, u32 code)
{
    const struct reg8_info *selected_cfmt_regs;
    u8 val;
    int ret;

    switch (code) {
    case MEDIA_BUS_FMT_RGB565_2X8_BE:
        LOG_D("%s: Selected cfmt RGB565 BE", __func__);
        selected_cfmt_regs = ov2640_rgb565_be_regs;
        break;
    case MEDIA_BUS_FMT_RGB565_2X8_LE:
        LOG_D("%s: Selected cfmt RGB565 LE", __func__);
        selected_cfmt_regs = ov2640_rgb565_le_regs;
        break;
    case MEDIA_BUS_FMT_YUYV8_2X8:
        LOG_D("%s: Selected cfmt YUYV (YUV422)", __func__);
        selected_cfmt_regs = ov2640_yuyv_regs;
        break;
    case MEDIA_BUS_FMT_UYVY8_2X8:
    default:
        LOG_D("%s: Selected cfmt UYVY", __func__);
        selected_cfmt_regs = ov2640_uyvy_regs;
        break;
    case MEDIA_BUS_FMT_YVYU8_2X8:
        LOG_D("%s: Selected cfmt YVYU", __func__);
        selected_cfmt_regs = ov2640_yuyv_regs;
        break;
    case MEDIA_BUS_FMT_VYUY8_2X8:
        LOG_D("%s: Selected cfmt VYUY", __func__);
        selected_cfmt_regs = ov2640_uyvy_regs;
        break;
    }

    /* reset hardware */
    ov2640_reset(i2c);

    /* initialize the sensor with default data */
    LOG_D("%s: Init default", __func__);
    ret = ov2640_write_array(i2c, ov2640_init_regs);
    if (ret < 0)
        goto err;

    /* select preamble */
    LOG_D("%s: Set size to %s", __func__, win->name);
    ret = ov2640_write_array(i2c, ov2640_size_change_preamble_regs);
    if (ret < 0)
        goto err;

    /* set size win */
    ret = ov2640_write_array(i2c, win->regs);
    if (ret < 0)
        goto err;

    /* cfmt preamble */
    LOG_D("%s: Set cfmt", __func__);
    ret = ov2640_write_array(i2c, ov2640_format_change_preamble_regs);
    if (ret < 0)
        goto err;

    /* set cfmt */
    ret = ov2640_write_array(i2c, selected_cfmt_regs);
    if (ret < 0)
        goto err;
    val = (code == MEDIA_BUS_FMT_YVYU8_2X8)
          || (code == MEDIA_BUS_FMT_VYUY8_2X8) ? CTRL0_VFIRST : 0x00;
    ret = ov2640_mask_set(i2c, CTRL0, CTRL0_VFIRST, val);
    if (ret < 0)
        goto err;

    return 0;

err:
    LOG_E("%s: Error %d", __func__, ret);
    ov2640_reset(i2c);

    return ret;
}

static int ov2640_s_stream(struct ov2640_dev *sensor, int on)
{
    int ret = 0;

    if ((sensor->streaming == !on) && on) {
        ret = ov2640_set_params(sensor->i2c, sensor->win, sensor->cfmt_code);
    }
    if (!ret)
        sensor->streaming = on;

    return ret;
}

static int ov2640_video_probe(struct ov2640_dev *sensor)
{
    u8 pid = 0, ver = 0, midh = 0, midl = 0;
    const char *devname;
    int ret = 0;

    ov2640_set_power(sensor, 1);

    /* check and show product ID and manufacturer ID */
    ov2640_write_reg(sensor->i2c, BANK_SEL, BANK_SEL_SENS);
    ov2640_read_reg(sensor->i2c, PID, &pid);
    ov2640_read_reg(sensor->i2c, VER, &ver);
    ov2640_read_reg(sensor->i2c, MIDH, &midh);
    ov2640_read_reg(sensor->i2c, MIDL, &midl);

    switch (VERSION(pid, ver)) {
    case PID_OV2640:
        devname = "ov2640";
        break;
    default:
        LOG_E("Product ID error %x:%x\n", pid, ver);
        ret = -ENODEV;
        goto done;
    }

    LOG_I("%s Product ID %0x:%0x Manufacturer ID %x:%x\n",
            devname, pid, ver, midh, midl);

done:
    ov2640_set_power(sensor, 0);
    return ret;
}

static rt_err_t ov2640_init(rt_device_t dev)
{
    int ret = 0;
    struct ov2640_dev *sensor = &g_ov2640_dev;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    sensor->xclk_freq = camera_xclk_rate_get();
    if (!sensor->xclk_freq) {
        LOG_E("Must set XCLK freq first!\n");
        return -RT_EINVAL;
    }

    sensor->win = &ov2640_supported_win_sizes[OV2640_DFT_WIN];
    sensor->cfmt_code  = OV2640_DFT_CODE;
    sensor->fmt.code   = OV2640_DFT_CODE;
    sensor->fmt.width  = sensor->win->width;
    sensor->fmt.height = sensor->win->height;
    sensor->fmt.bus_type = OV2640_DFT_BUS_TYPE;
    sensor->fmt.flags = MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH |
                        MEDIA_SIGNAL_VSYNC_ACTIVE_LOW |
                        MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    sensor->rst_pin = camera_rst_pin_get();
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->rst_pin || !sensor->pwdn_pin)
        return -RT_EINVAL;

    ret = ov2640_video_probe(sensor);
    if (ret)
        return -RT_ERROR;

    LOG_I("OV2640 inited");
    return RT_EOK;
}

static rt_err_t ov2640_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct ov2640_dev *sensor = (struct ov2640_dev *)dev;

    ov2640_set_power(sensor, 1);
    LOG_I("OV2640 Open");
    return RT_EOK;
}

static rt_err_t ov2640_close(rt_device_t dev)
{
    struct ov2640_dev *sensor = (struct ov2640_dev *)dev;

    ov2640_set_power(sensor, 0);
    LOG_I("OV2640 Close");
    return RT_EOK;
}

static int ov2640_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct ov2640_dev *sensor = (struct ov2640_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int ov2640_start(rt_device_t dev)
{
    return ov2640_s_stream((struct ov2640_dev *)dev, 1);
}

static int ov2640_stop(rt_device_t dev)
{
    return ov2640_s_stream((struct ov2640_dev *)dev, 0);
}

static rt_err_t ov2640_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return ov2640_start(dev);
    case CAMERA_CMD_STOP:
        return ov2640_stop(dev);
    case CAMERA_CMD_GET_FMT:
        return ov2640_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops ov2640_ops =
{
    .init = ov2640_init,
    .open = ov2640_open,
    .close = ov2640_close,
    .control = ov2640_control,
};
#endif

int rt_hw_ov2640_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_ov2640_dev.dev.ops = &ov2640_ops;
#else
    g_ov2640_dev.dev.init = ov2640_init;
    g_ov2640_dev.dev.open = ov2640_open;
    g_ov2640_dev.dev.close = ov2640_close;
    g_ov2640_dev.dev.control = ov2640_control;
#endif
    g_ov2640_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_ov2640_dev.dev, CAMERA_DEV_NAME, 0);
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_ov2640_init);
