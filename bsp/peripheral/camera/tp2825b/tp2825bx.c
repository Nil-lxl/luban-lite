/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define LOG_TAG     "tp2825"

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
#include "tp2802_def.h"
#include "tp2825b.h"

#define DRV_NAME            "tp2825"

#define DEFAULT_FORMAT      TP2802_PAL
#define DEFAULT_MEDIA_CODE  MEDIA_BUS_FMT_UYVY8_2X8
#define DEFAULT_BUS_TYPE    MEDIA_BUS_BT656
#define DEFAULT_WIDTH       PAL_WIDTH
#define DEFAULT_HEIGHT      PAL_HEIGHT
#define DEFAULT_USE_CVBS    1   /* 0, HDA; 1, CVBS */
#define DEFAULT_FRAMERATE   25
#define DEFAULT_VIN_CH      0   /* 0 or 1 channel */

#define DEBUG               0 // debug information on/off
#define TEST_MODE           0
#define WATCHDOG_EXIT       0
#define WATCHDOG_RUNNING    0
#define WDT                 0

#if WDT
static int HDC_enable = 1;
#endif
static int mode = DEFAULT_FORMAT;
static int chips = 1;
static int output[] = {
//  EMB422_16BIT, //for TP2825B
    SEP656_8BIT,  //for TP2850
};

static unsigned int id[MAX_CHIPS] = {0};

#define TP2825B_I2C_A   0x44
#define TP2825B_I2C_B   0x45

unsigned char tp2802_i2c_addr[] = {TP2825B_I2C_A, TP2825B_I2C_B};

typedef struct {
    unsigned int  count[CHANNELS_PER_CHIP];
    unsigned int  mode[CHANNELS_PER_CHIP];
    unsigned int  scan[CHANNELS_PER_CHIP];
    unsigned int  gain[CHANNELS_PER_CHIP][4];
    unsigned int  std[CHANNELS_PER_CHIP];
    unsigned int  state[CHANNELS_PER_CHIP];
    unsigned int  force[CHANNELS_PER_CHIP];
    unsigned char addr;
} tp2802wd_info;

static tp2802wd_info watchdog_info[MAX_CHIPS] = {{{0}}};
volatile static unsigned int watchdog_state = 0;
struct task_struct *task_watchdog_deamon = NULL;

struct tp2825_dev {
    struct rt_device dev;
    struct rt_i2c_bus_device *i2c;
    u32 pwdn_pin;

    struct mpp_video_fmt fmt;

    /* lock to protect all members below */
    bool streaming;
};

static struct tp2825_dev g_tp2825_dev = {0};
struct rt_i2c_bus_device *tp28xx_client0 = NULL;

int  TP2802_watchdog_init(void);
void TP2802_watchdog_exit(void);
static void TP2825B_PTZ_mode(unsigned char, unsigned char, unsigned char);
static int tp2802_set_video_mode(unsigned char addr, unsigned char mode,
                                 unsigned char ch, unsigned char std);
static void tp2802_set_reg_page(unsigned char addr, unsigned char ch);

unsigned int ConvertACPV1Data(unsigned char dat)
{
    unsigned int i, tmp = 0;
    for (i = 0; i < 8; i++) {
        tmp <<= 3;

        if (0x01 & dat)
            tmp |= 0x06;
        else
            tmp |= 0x04;

        dat >>= 1;
    }
    return tmp;
}

void tp28xx_byte_write(u8 chip, u8 reg, u8 val)
{
    if (rt_i2c_write_reg(tp28xx_client0, tp2802_i2c_addr[chip], reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return;
    }

    aicos_udelay(300);
}

unsigned char tp28xx_byte_read(u8 chip, u8 reg)
{
    unsigned char val = 0xFF;

    if (rt_i2c_read_reg(tp28xx_client0, tp2802_i2c_addr[chip], reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return 0xFF;
    }

    return val;
}

static void tp2802_write_table(unsigned char chip,
                               unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
    unsigned char i = 0;
    for (i = 0; i < tbl_cnt; i++)
        tp28xx_byte_write(chip, (addr + i), *(tbl_ptr + i));
}

static void tp28xx_show_reg(u8 chip, u32 cnt)
{
#define REG_DUMP_STEP   16
    u8 val = 0;
    s32 i, j;

    for (i = 0; i < cnt / REG_DUMP_STEP; i++) {
        printf("0x%02x:", i * REG_DUMP_STEP);
        for (j = 0; j < REG_DUMP_STEP; j++) {
            val = tp28xx_byte_read(chip, i * REG_DUMP_STEP + j);
            printf(" %02x", val);

            if (j == 7)
                printf("   ");
        }
        printf("\n");
    }
}

static void tp28xx_dump_reg(u8 chip)
{
    printf("Chip%d MIPI:\n", chip);

    tp2802_set_reg_page(chip, MIPI_PAGE);
    tp28xx_show_reg(chip, 0x40);

    printf("\nChip%d VIDEO:\n", chip);

    tp2802_set_reg_page(chip, VIDEO_PAGE);
    tp28xx_show_reg(chip, 0x100);
}

unsigned char ReverseByte(unsigned char dat)
{
    static const unsigned char BitReverseTable256[] = {
        0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
        0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
        0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
        0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
        0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
        0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
        0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
        0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
        0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
        0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
        0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
        0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
        0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
        0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
        0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
        0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
    };
    return BitReverseTable256[dat];
}
void HDC_QHD_SetData(unsigned char chip, unsigned char reg, unsigned int dat)
{
    unsigned int i;
    unsigned char ret = 0;
    unsigned char crc = 0;

    if (dat > 0xff) {
        tp28xx_byte_write(chip, reg + 0, 0x00);
        tp28xx_byte_write(chip, reg + 1, 0x07);
        tp28xx_byte_write(chip, reg + 2, 0xff);
        tp28xx_byte_write(chip, reg + 3, 0xff);
        tp28xx_byte_write(chip, reg + 4, 0xfc);
    } else {
        for (i = 0; i < 8; i++) {
            ret >>= 1;
            if (0x80 & dat) {
                ret |= 0x80;
                crc += 0x80;
            }
            dat <<= 1;
        }

        tp28xx_byte_write(chip, reg + 0, 0x00);
        tp28xx_byte_write(chip, reg + 1, 0x06);
        tp28xx_byte_write(chip, reg + 2, ret);
        tp28xx_byte_write(chip, reg + 3, 0x7f | crc);
        tp28xx_byte_write(chip, reg + 4, 0xfc);
    }

}
void HDC_SetData(unsigned char chip, unsigned char reg, unsigned int dat)
{
    unsigned int i;
    unsigned char ret = 0;
    unsigned char crc = 0;

    if (dat > 0xff) {
        tp28xx_byte_write(chip, reg + 0, 0x07);
        tp28xx_byte_write(chip, reg + 1, 0xff);
        tp28xx_byte_write(chip, reg + 2, 0xff);
        tp28xx_byte_write(chip, reg + 3, 0xff);
        tp28xx_byte_write(chip, reg + 4, 0xfc);
    } else {
        for (i = 0; i < 8; i++) {
            ret >>= 1;
            if (0x80 & dat) {
                ret |= 0x80;
                crc += 0x80;
            }
            dat <<= 1;
        }

        tp28xx_byte_write(chip, reg + 0, 0x06);
        tp28xx_byte_write(chip, reg + 1, ret);
        tp28xx_byte_write(chip, reg + 2, 0x7f | crc);
        tp28xx_byte_write(chip, reg + 3, 0xff);
        tp28xx_byte_write(chip, reg + 4, 0xfc);
    }

}
void HDA_SetACPV2Data(unsigned char chip, unsigned char reg, unsigned char dat)
{
    unsigned int i;
    unsigned int PTZ_pelco = 0;

    for (i = 0; i < 8; i++) {
        PTZ_pelco <<= 3;

        if (0x80 & dat)
            PTZ_pelco |= 0x06;
        else
            PTZ_pelco |= 0x04;

        dat <<= 1;
    }

    tp28xx_byte_write(chip, reg + 0, (PTZ_pelco >> 16) & 0xff);
    tp28xx_byte_write(chip, reg + 1, (PTZ_pelco >> 8) & 0xff);
    tp28xx_byte_write(chip, reg + 2, (PTZ_pelco) & 0xff);
}
void HDA_SetACPV1Data(unsigned char chip, unsigned char reg, unsigned char dat)
{
    unsigned int i;
    unsigned int PTZ_pelco = 0;

    for (i = 0; i < 8; i++) {
        PTZ_pelco <<= 3;

        if (0x01 & dat)
            PTZ_pelco |= 0x06;
        else
            PTZ_pelco |= 0x04;

        dat >>= 1;
    }

    tp28xx_byte_write(chip, reg + 0, (PTZ_pelco >> 16) & 0xff);
    tp28xx_byte_write(chip, reg + 1, (PTZ_pelco >> 8) & 0xff);
    tp28xx_byte_write(chip, reg + 2, (PTZ_pelco) & 0xff);

}

static void tp2802_set_work_mode_1080p25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_1080p25_raster, 9);
}

static void tp2802_set_work_mode_1080p30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_1080p30_raster, 9);
}

static void tp2802_set_work_mode_720p25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p25_raster, 9);
}

static void tp2802_set_work_mode_720p30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p30_raster, 9);
}

static void tp2802_set_work_mode_720p50(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p50_raster, 9);
}

static void tp2802_set_work_mode_720p60(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p60_raster, 9);
}

static void tp2802_set_work_mode_PAL(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_PAL_raster, 9);
}

static void tp2802_set_work_mode_NTSC(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_NTSC_raster, 9);
}
static void tp2802_set_work_mode_3M(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_3M_raster, 9);
}

static void tp2802_set_work_mode_5M(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_5M_raster, 9);
}
static void tp2802_set_work_mode_4M(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M_raster, 9);
}
static void tp2802_set_work_mode_3M20(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_3M20_raster, 9);
}
#if 0
static void tp2802_set_work_mode_4M12(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M12_raster, 9);
}
#endif
static void tp2802_set_work_mode_6M10(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_6M10_raster, 9);
}
#if 0
static void tp2802_set_work_mode_QHDH30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHDH30_raster, 9);
}
static void tp2802_set_work_mode_QHDH25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHDH25_raster, 9);
}
#endif
static void tp2802_set_work_mode_QHD15(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHD15_raster, 9);
}
#if 0
static void tp2802_set_work_mode_QXGAH30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QXGAH30_raster, 9);
}
static void tp2802_set_work_mode_QXGAH25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QXGAH25_raster, 9);
}
#endif
static void tp2802_set_work_mode_QHD30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHD30_raster, 9);
}
static void tp2802_set_work_mode_QHD25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHD25_raster, 9);
}
static void tp2802_set_work_mode_QXGA30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QXGA30_raster, 9);
}
static void tp2802_set_work_mode_QXGA25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QXGA25_raster, 9);
}
/*
static void tp2802_set_work_mode_4M30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M30_raster, 9);
}
static void tp2802_set_work_mode_4M25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M25_raster, 9);
}
*/
static void tp2802_set_work_mode_5M20(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_5M20_raster, 9);
}
/*
static void tp2802_set_work_mode_5MH20(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_5MH20_raster, 9);
}
static void tp2802_set_work_mode_4MH30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4MH30_raster, 9);
}
static void tp2802_set_work_mode_4MH25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4MH25_raster, 9);
}
*/
static void tp2802_set_work_mode_8M15(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_8M15_raster, 9);
}
#if 0
static void tp2802_set_work_mode_8MH15(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_8MH15_raster, 9);
}
#endif
static void tp2802_set_work_mode_8M12(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_8M12_raster, 9);
}
#if 0
static void tp2802_set_work_mode_8MH12(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_8MH12_raster, 9);
}
#endif
static void tp2802_set_work_mode_720p30HDR(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p30HDR_raster, 9);
}
static void tp2802_set_work_mode_6M20(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_6M20_raster, 9);
}

//#define AMEND
#include "tp2825b_tbl.c"

static void tp2825_get_v_loss(tp2802_video_loss *loss)
{
    u8 tmp = 0;
    unsigned int chip = 0;

    //if (loss->ch >= CHANNELS_PER_CHIP)  return FAILURE;
    if (loss->ch >= CHANNELS_PER_CHIP)
        loss->ch = 0;

#if (WDT)
    loss->is_lost = (VIDEO_LOCKED == watchdog_info[loss->chip].state[loss->ch]) ? 0 : 1;
    if (loss->is_lost)
        loss->is_lost = (VIDEO_UNLOCK == watchdog_info[loss->chip].state[loss->ch]) ? 0 : 1;
#else

    chip = loss->chip;

    tp2802_set_reg_page(chip, loss->ch);

    tmp = tp28xx_byte_read(chip, 0x01);
    tmp = (tmp & 0x80) >> 7;
    if (!tmp) {
        if (0x08 == tp28xx_byte_read(chip, 0x2f)) {
            tmp = tp28xx_byte_read(chip, 0x04);
            if (tmp < 0x30)
                tmp = 0;
            else
                tmp = 1;
        }
    }
    loss->is_lost = tmp;   /* [7] - VDLOSS */

#endif
}

static int tp2825_set_ptz(tp2802_PTZ_data *ptz)
{
    unsigned int i, chip = 0;

    //if (ptz->ch > PTZ_PIN_PTZ2)  return FAILURE;

    chip = ptz->chip;

    tp28xx_byte_write(chip, 0x40,  0x0); //bank switch
    TP2825B_PTZ_mode(chip, ptz->ch, ptz->mode);

    for (i = 0; i < 24; i++)
        tp28xx_byte_write(chip, 0x55 + i, 0x00);

    if (PTZ_HDC == ptz->mode || PTZ_HDC_8M12 == ptz->mode) { //HDC 1080p
        HDC_SetData(chip, 0x56, ptz->data[0]);
        HDC_SetData(chip, 0x5c, ptz->data[1]);
        HDC_SetData(chip, 0x62, ptz->data[2]);
        HDC_SetData(chip, 0x68, ptz->data[3]);
        TP2825B_StartTX(chip, ptz->ch);
        HDC_SetData(chip, 0x56, ptz->data[4]);
        HDC_SetData(chip, 0x5c, ptz->data[5]);
        HDC_SetData(chip, 0x62, ptz->data[6]);
        HDC_SetData(chip, 0x68, 0xffff);
        TP2825B_StartTX(chip, ptz->ch);
    } else if (PTZ_HDC_FIFO == ptz->mode) { //HDC 1080p FIFO mode
        for (i = 0; i < 7; i++)
            tp28xx_byte_write(chip, 0x6e, ptz->data[i]);

        TP2825B_StartTX(chip, ptz->ch);
    } else if (PTZ_HDC_QHD == ptz->mode || PTZ_HDC_8M15 == ptz->mode) { //HDC QHD
        HDC_QHD_SetData(chip, 0x56, ptz->data[0]);
        HDC_QHD_SetData(chip, 0x5c, ptz->data[1]);
        HDC_QHD_SetData(chip, 0x62, ptz->data[2]);
        HDC_QHD_SetData(chip, 0x68, ptz->data[3]);
        TP2825B_StartTX(chip, ptz->ch);
        HDC_QHD_SetData(chip, 0x56, ptz->data[4]);
        HDC_QHD_SetData(chip, 0x5c, ptz->data[5]);
        HDC_QHD_SetData(chip, 0x62, ptz->data[6]);
        HDC_QHD_SetData(chip, 0x68, 0xffff);
        TP2825B_StartTX(chip, ptz->ch);
    } else if (PTZ_HDA_4M25 == ptz->mode || PTZ_HDA_4M15 == ptz->mode) { //HDA QHD
        for (i = 0; i < 8; i++)
            tp28xx_byte_write(chip, 0x6e, 0x00);

        TP2825B_StartTX(chip, ptz->ch);

        for (i = 0; i < 8; i++)
            tp28xx_byte_write(chip, 0x6e, ReverseByte(ptz->data[i]));

        TP2825B_StartTX(chip, ptz->ch);
    } else if (PTZ_HDA_1080P == ptz->mode || PTZ_HDA_3M18 == ptz->mode || PTZ_HDA_3M25 == ptz->mode) { //HDA 1080p
        HDA_SetACPV2Data(chip, 0x58, 0x00);
        HDA_SetACPV2Data(chip, 0x5e, 0x00);
        HDA_SetACPV2Data(chip, 0x64, 0x00);
        HDA_SetACPV2Data(chip, 0x6a, 0x00);
        TP2825B_StartTX(chip, ptz->ch);
        HDA_SetACPV2Data(chip, 0x58, ptz->data[0]);
        HDA_SetACPV2Data(chip, 0x5e, ptz->data[1]);
        HDA_SetACPV2Data(chip, 0x64, ptz->data[2]);
        HDA_SetACPV2Data(chip, 0x6a, ptz->data[3]);
        TP2825B_StartTX(chip, ptz->ch);
    } else if (PTZ_HDA_720P == ptz->mode) { //HDA 720p
        HDA_SetACPV1Data(chip, 0x55, 0x00);
        HDA_SetACPV1Data(chip, 0x58, 0x00);
        HDA_SetACPV1Data(chip, 0x5b, 0x00);
        HDA_SetACPV1Data(chip, 0x5e, 0x00);
        TP2825B_StartTX(chip, ptz->ch);
        HDA_SetACPV1Data(chip, 0x55, ptz->data[0]);
        HDA_SetACPV1Data(chip, 0x58, ptz->data[1]);
        HDA_SetACPV1Data(chip, 0x5b, ptz->data[2]);
        HDA_SetACPV1Data(chip, 0x5e, ptz->data[3]);
        TP2825B_StartTX(chip, ptz->ch);
    } else if (PTZ_HDA_CVBS == ptz->mode) {//HDA 960H
        HDA_SetACPV1Data(chip, 0x55, 0x00);
        HDA_SetACPV1Data(chip, 0x58, 0x00);
        HDA_SetACPV1Data(chip, 0x5b, 0x00);
        HDA_SetACPV1Data(chip, 0x5e, 0x00);
        //TP2825B_StartTX(chip, ptz->ch);
        TP2825B_StartTX(chip, ptz->ch);
        HDA_SetACPV1Data(chip, 0x55, ptz->data[0]);
        HDA_SetACPV1Data(chip, 0x58, ptz->data[1]);
        HDA_SetACPV1Data(chip, 0x5b, ptz->data[2]);
        HDA_SetACPV1Data(chip, 0x5e, ptz->data[3]);
        //TP2825B_StartTX(chip, ptz->ch);
        TP2825B_StartTX(chip, ptz->ch);
    } else { //TVI
        // line1
        tp28xx_byte_write(chip, 0x56, 0x02);
        tp28xx_byte_write(chip, 0x57, ptz->data[0]);
        tp28xx_byte_write(chip, 0x58, ptz->data[1]);
        tp28xx_byte_write(chip, 0x59, ptz->data[2]);
        tp28xx_byte_write(chip, 0x5A, ptz->data[3]);
        // line2
        tp28xx_byte_write(chip, 0x5C, 0x02);
        tp28xx_byte_write(chip, 0x5D, ptz->data[4]);
        tp28xx_byte_write(chip, 0x5E, ptz->data[5]);
        tp28xx_byte_write(chip, 0x5F, ptz->data[6]);
        tp28xx_byte_write(chip, 0x60, ptz->data[7]);

        // line3
        tp28xx_byte_write(chip, 0x62, 0x02);
        tp28xx_byte_write(chip, 0x63, ptz->data[0]);
        tp28xx_byte_write(chip, 0x64, ptz->data[1]);
        tp28xx_byte_write(chip, 0x65, ptz->data[2]);
        tp28xx_byte_write(chip, 0x66, ptz->data[3]);
        // line4
        tp28xx_byte_write(chip, 0x68, 0x02);
        tp28xx_byte_write(chip, 0x69, ptz->data[4]);
        tp28xx_byte_write(chip, 0x6A, ptz->data[5]);
        tp28xx_byte_write(chip, 0x6B, ptz->data[6]);
        tp28xx_byte_write(chip, 0x6C, ptz->data[7]);

        TP2825B_StartTX(chip, ptz->ch);
    }

    return SUCCESS;
}

static int tp2825_get_ptz(tp2802_PTZ_data *ptz)
{
    unsigned int chip = 0;

    if (ptz->ch >= CHANNELS_PER_CHIP)
        return FAILURE;

    chip = ptz->chip;

    tp28xx_byte_write(chip, 0x40, 0x0); //bank switch
    // line1
    ptz->data[0] = tp28xx_byte_read(chip, 0x8C);
    ptz->data[1] = tp28xx_byte_read(chip, 0x8D);
    ptz->data[2] = tp28xx_byte_read(chip, 0x8E);
    ptz->data[3] = tp28xx_byte_read(chip, 0x8F);
    //line2
    ptz->data[4] = tp28xx_byte_read(chip, 0x92);
    ptz->data[5] = tp28xx_byte_read(chip, 0x93);
    ptz->data[6] = tp28xx_byte_read(chip, 0x94);
    ptz->data[7] = tp28xx_byte_read(chip, 0x95);
    // line3
    ptz->data[8] = tp28xx_byte_read(chip, 0x98);
    ptz->data[9] = tp28xx_byte_read(chip, 0x99);
    ptz->data[10] = tp28xx_byte_read(chip, 0x9a);
    ptz->data[11] = tp28xx_byte_read(chip, 0x9b);
    //line4
    ptz->data[12] = tp28xx_byte_read(chip, 0x9e);
    ptz->data[13] = tp28xx_byte_read(chip, 0x9f);
    ptz->data[14] = tp28xx_byte_read(chip, 0xa0);
    ptz->data[15] = tp28xx_byte_read(chip, 0xa1);
    return SUCCESS;
}

static int tp2825_set_v_input(tp2802_register *reg)
{
    u8 tmp = 0;
    unsigned int chip = 0;

    if (reg->value > DIFF_VIN34)
        return FAILURE;

    chip = reg->chip;
    tp28xx_byte_write(chip, 0x40, 0x0); //bank switch
    tmp = tp28xx_byte_read(chip, 0x41);
    tmp &= 0xf0;
    tmp |= reg->value;
    tp28xx_byte_write(chip, 0x41, tmp);

    if (reg->value < DIFF_VIN12) {
        //single
        tp28xx_byte_write(chip, 0x38, 0x40);
        tp28xx_byte_write(chip, 0x3d, 0x60);
        //tp28xx_byte_write(chip, 0x3e, 0x00);
    } else {
        //differential
        tp28xx_byte_write(chip, 0x38, 0x4e);
        tp28xx_byte_write(chip, 0x3d, 0x40);
        //tp28xx_byte_write(chip, 0x3e, 0x80);
    }

    watchdog_info[chip].force[0] = 1; //reset when Vin is changed
    return SUCCESS;
}

static int tp2825_set_fifo_data(tp2802_PTZ_data *ptz)
{
    unsigned int i, chip = 0;

    if (ptz->ch >= CHANNELS_PER_CHIP)
        return FAILURE;

    chip = ptz->chip;

    //TP2854_PTZ_mode(chip, ptz->ch, ptz->mode);
    /*
        if (ptz->mode < 128) {
            while((128 - tp28xx_byte_read(chip, 0xde)) < ptz->mode) aicos_msleep(2);
            //while(tp28xx_byte_read(chip, 0xde)) aicos_msleep(2);
        }
        else
     */
    {
        while ((255 - tp28xx_byte_read(chip, 0xde)) < ptz->mode)
            aicos_msleep(2);
        //while(tp28xx_byte_read(chip, 0xde)) aicos_msleep(2);
    }
    //LOG_I("ptz_mode %x\n", ptz->mode);
    for (i = 0; i < ptz->mode; i++)
        tp28xx_byte_write(chip, 0xdf, ptz->data[i]);

    return SUCCESS;
}

long tp2802_ioctl(struct tp2825_dev *sensor,
                  unsigned int cmd, unsigned long arg)
{
    unsigned int i, chip, tmp, ret = 0;

    tp2802_register     *dev_register = NULL;
    tp2802_image_adjust *image_adjust = NULL;
    tp2802_work_mode    *work_mode    = NULL;
    tp2802_video_mode   *video_mode   = NULL;
    tp2802_PTZ_data     *PTZ_data     = NULL;

    switch (cmd) {
    case TP2802_READ_REG:
    {
        dev_register = (tp2802_register *)arg;

        chip = dev_register->chip;
        tp2802_set_reg_page(chip, dev_register->ch);
        dev_register->value = tp28xx_byte_read(chip, dev_register->reg_addr);
        break;
    }

    case TP2802_WRITE_REG:
    {
        dev_register = (tp2802_register *)arg;

        chip = dev_register->chip;
        tp2802_set_reg_page(chip, dev_register->ch);
        tp28xx_byte_write(chip, dev_register->reg_addr, dev_register->value);
        break;
    }

    case TP2802_SET_VIDEO_MODE:
    {
        video_mode = (tp2802_video_mode *)arg;
        if (video_mode->ch >= CHANNELS_PER_CHIP)
            return FAILURE;

        ret = tp2802_set_video_mode(video_mode->chip, video_mode->mode, video_mode->ch, video_mode->std);

        if (!(ret)) {
            watchdog_info[video_mode->chip].mode[video_mode->ch] = video_mode->mode;
            watchdog_info[video_mode->chip].std[video_mode->ch] = video_mode->std;
            return SUCCESS;
        } else {
            LOG_E("Invalid mode:%d\n", video_mode->mode);
            return FAILURE;
        }

        break;
    }

    case TP2802_GET_VIDEO_MODE:
    {
        video_mode = (tp2802_video_mode *)arg;

        //if (video_mode->ch >= CHANNELS_PER_CHIP)  return FAILURE;
        if (video_mode->ch >= CHANNELS_PER_CHIP)
            video_mode->ch = 0;
#if (WDT)
        video_mode->mode = watchdog_info[video_mode->chip].mode[video_mode->ch];
        video_mode->std = watchdog_info[video_mode->chip].std[video_mode->ch];
#else

        chip = video_mode->chip;

        tp2802_set_reg_page(chip, video_mode->ch);

        tmp = tp28xx_byte_read(chip, 0x03);
        tmp &= 0x7; /* [2:0] - CVSTD */
        video_mode->mode = tmp;

#endif
        break;
    }

    case TP2802_GET_VIDEO_LOSS:/* get video loss state */
    {
        tp2825_get_v_loss((tp2802_video_loss *)arg);
        break;
    }

    case TP2802_SET_IMAGE_ADJUST:
    {
        image_adjust = (tp2802_image_adjust *)arg;
        if (image_adjust->ch >= CHANNELS_PER_CHIP)
            return FAILURE;

        chip = image_adjust->chip;

        tp2802_set_reg_page(chip, image_adjust->ch);

        // Set Brightness
        tp28xx_byte_write(chip, BRIGHTNESS, image_adjust->brightness);

        // Set Contrast
        tp28xx_byte_write(chip, CONTRAST, image_adjust->contrast);

        // Set Saturation
        tp28xx_byte_write(chip, SATURATION, image_adjust->saturation);

        // Set Hue
        tp28xx_byte_write(chip, HUE, image_adjust->hue);

        // Set Sharpness
        tmp = tp28xx_byte_read(chip, SHARPNESS);
        tmp &= 0xe0;
        tmp |= (image_adjust->sharpness & 0x1F);
        tp28xx_byte_write(chip, SHARPNESS, tmp);

        break;
    }

    case TP2802_GET_IMAGE_ADJUST:
    {
        image_adjust = (tp2802_image_adjust *)arg;
        if (image_adjust->ch >= CHANNELS_PER_CHIP)
            return FAILURE;

        chip = image_adjust->chip;

        tp2802_set_reg_page(chip, image_adjust->ch);

        // Get Brightness
        image_adjust->brightness = tp28xx_byte_read(chip, BRIGHTNESS);

        // Get Contrast
        image_adjust->contrast = tp28xx_byte_read(chip, CONTRAST);

        // Get Saturation
        image_adjust->saturation = tp28xx_byte_read(chip, SATURATION);

        // Get Hue
        image_adjust->hue = tp28xx_byte_read(chip, HUE);

        // Get Sharpness
        image_adjust->sharpness = 0x1F & tp28xx_byte_read(chip, SHARPNESS);

        break;
    }
    case TP2802_SET_PTZ_DATA:
    {
        return tp2825_set_ptz((tp2802_PTZ_data *)arg);
    }
    case TP2802_GET_PTZ_DATA:
    {
        return tp2825_get_ptz((tp2802_PTZ_data *)arg);
    }
    case TP2802_SET_SCAN_MODE:
    {
        work_mode = (tp2802_work_mode *)arg;
        if (work_mode->ch >= CHANNELS_PER_CHIP) {
            for (i = 0; i < CHANNELS_PER_CHIP; i++)
                watchdog_info[work_mode->chip].scan[i] = work_mode->mode;
        } else {
            watchdog_info[work_mode->chip].scan[work_mode->ch] = work_mode->mode;
        }
        break;
    }
    case TP2802_DUMP_REG:
    {
        dev_register = (tp2802_register *)arg;

        tp28xx_dump_reg(dev_register->chip);
        break;
    }
    case TP2802_FORCE_DETECT:
    {
        work_mode = (tp2802_work_mode *)arg;
        if (work_mode->ch >= CHANNELS_PER_CHIP) {
            for (i = 0; i < CHANNELS_PER_CHIP; i++)
                watchdog_info[work_mode->chip].force[i] = 1;
        } else {
            watchdog_info[work_mode->chip].force[work_mode->ch] = 1;
        }
        break;
    }

    case TP2802_SET_BURST_DATA:
    {
        PTZ_data = (tp2802_PTZ_data *)arg;

        //if (PTZ_data->ch >= CHANNELS_PER_CHIP)  return FAILURE;
        chip = PTZ_data->chip;

        tp28xx_byte_write(chip, 0x40,  0x0); //bank switch
        TP2825B_PTZ_mode(chip, PTZ_data->ch, PTZ_data->mode);

        tp28xx_byte_write(chip, 0x55, 0x00);
        tp28xx_byte_write(chip, 0x5b, 0x00);

        //line1
        tp28xx_byte_write(chip, 0x56, 0x03);
        tp28xx_byte_write(chip, 0x57, PTZ_data->data[0]);
        tp28xx_byte_write(chip, 0x58, PTZ_data->data[1]);
        tp28xx_byte_write(chip, 0x59, PTZ_data->data[2]);
        tp28xx_byte_write(chip, 0x5A, PTZ_data->data[3]);
        //line2
        tp28xx_byte_write(chip, 0x5C, 0x03);
        tp28xx_byte_write(chip, 0x5D, PTZ_data->data[4]);
        tp28xx_byte_write(chip, 0x5E, PTZ_data->data[5]);
        tp28xx_byte_write(chip, 0x5F, PTZ_data->data[6]);
        tp28xx_byte_write(chip, 0x60, PTZ_data->data[7]);
        //line3
        tp28xx_byte_write(chip, 0x62, 0x03);
        tp28xx_byte_write(chip, 0x63, PTZ_data->data[8]);
        tp28xx_byte_write(chip, 0x64, PTZ_data->data[9]);
        tp28xx_byte_write(chip, 0x65, PTZ_data->data[10]);
        tp28xx_byte_write(chip, 0x66, PTZ_data->data[11]);
        //line4
        tp28xx_byte_write(chip, 0x68, 0x03);
        tp28xx_byte_write(chip, 0x69, PTZ_data->data[12]);
        tp28xx_byte_write(chip, 0x6A, PTZ_data->data[13]);
        tp28xx_byte_write(chip, 0x6B, PTZ_data->data[14]);
        tp28xx_byte_write(chip, 0x6C, PTZ_data->data[15]);

        TP2825B_StartTX(chip, PTZ_data->ch);

        break;
    }

    case TP2802_SET_VIDEO_INPUT:
    {
        return tp2825_set_v_input((tp2802_register *)arg);
    }
    case TP2802_GET_VIDEO_INPUT:
    {
        dev_register = (tp2802_register *)arg;

        tp28xx_byte_write(dev_register->chip, 0x40,  0x0); //bank switch
        dev_register->value = tp28xx_byte_read(dev_register->chip, 0x41) & 0xF;
        break;
    }
    case TP2802_SET_PTZ_MODE:
    {
        PTZ_data = (tp2802_PTZ_data *)arg;
        if (PTZ_data->ch > PTZ_PIN_PTZ2)
            return FAILURE;

        chip = PTZ_data->chip;

        //if (TP2825B == id[chip])
        {
            TP2825B_PTZ_mode(chip, PTZ_data->ch, PTZ_data->mode);
        }
        break;
    }
    case TP2802_SET_RX_MODE:
    {
        PTZ_data = (tp2802_PTZ_data *)arg;
        chip = PTZ_data->chip;

        tp28xx_byte_write(chip, 0x40, 0x0); //bank switch

        TP2825B_RX_init(chip, PTZ_data->mode);
        break;
    }

    case TP2802_SET_FIFO_DATA:
    {
        return tp2825_set_fifo_data((tp2802_PTZ_data *)arg);
    }
    default:
    {
        LOG_E("Invalid tp2802 ioctl cmd!\n");
        ret = -1;
        break;
    }
    }

    return ret;
}

static void TP2825B_Set_REG0X02(unsigned char chip, unsigned char data)
{
    if (MUX656_8BIT == output[chip] || SEP656_8BIT == output[chip])
        tp28xx_byte_write(chip, 0x02, data);
    else
        tp28xx_byte_write(chip, 0x02, data & 0x7f);
}
//////////////////////////////////////////////////////////////////////////////////////////////

static void TP2826_C1080P25_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x50);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x54);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa2);

}
static void TP2826_C720P25_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x3a);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2d, 0x36);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x67);
    tp28xx_byte_write(chip, 0x32, 0x6f);
    tp28xx_byte_write(chip, 0x33, 0x33);

}
static void TP2826_C720P50_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x3a);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x42);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa3);

}
static void TP2826_C1080P30_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x3c);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x4c);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa4);

}
static void TP2826_C720P30_DataSet(unsigned char chip)
{
    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x30);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x37);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x67);
    tp28xx_byte_write(chip, 0x32, 0x6f);
    tp28xx_byte_write(chip, 0x33, 0x30);
}
static void TP2826_C720P60_DataSet(unsigned char chip)
{
    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x30);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x37);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa0);
}
//////////////////////////////////////////////////////////////////////////////
static int tp2802_set_video_mode(unsigned char chip, unsigned char mode,
                                 unsigned char ch, unsigned char std)
{
    int err = 0;
    unsigned int tmp;

    pr_debug("%s() Set Chip%d ch%d mode %#x std %d\n",
             __func__, chip, ch, mode, std);
    switch (mode) {
    case TP2802_1080P25:
        tp2802_set_work_mode_1080p25(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        if (STD_HDA == std) {
            if (TP2860 == id[chip]) {
                TP2860_A1080P25_DataSet(chip);
                //TP2860_144_A1080P25_DataSet(chip);
            } else {
                TP2825B_A1080P25_DataSet(chip);
            }

        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2826_C1080P25_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x15, 0x13);
                tp28xx_byte_write(chip, 0x16, 0x60);
                tp28xx_byte_write(chip, 0x17, 0x80);
                tp28xx_byte_write(chip, 0x18, 0x29);
                tp28xx_byte_write(chip, 0x19, 0x38);
                tp28xx_byte_write(chip, 0x1A, 0x47);
                tp28xx_byte_write(chip, 0x1C, 0x09);
                tp28xx_byte_write(chip, 0x1D, 0x60);
            }
        }

        if (STD_HDA == std && TP2860 == id[chip]) {
            TP2860_SYSCLK_A1080P(chip);
            //TP2860_SYSCLK_144M(chip);
        } else {
            TP2825B_SYSCLK_V1(chip);
        }
        break;

    case TP2802_1080P30:
        tp2802_set_work_mode_1080p30(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        if (STD_HDA == std) {
            if (TP2860 == id[chip]) {
                TP2860_A1080P30_DataSet(chip);
                //TP2860_144_A1080P30_DataSet(chip);
            } else {
                TP2825B_A1080P30_DataSet(chip);
            }
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2826_C1080P30_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x15, 0x13);
                tp28xx_byte_write(chip, 0x16, 0x60);
                tp28xx_byte_write(chip, 0x17, 0x80);
                tp28xx_byte_write(chip, 0x18, 0x29);
                tp28xx_byte_write(chip, 0x19, 0x38);
                tp28xx_byte_write(chip, 0x1A, 0x47);
                tp28xx_byte_write(chip, 0x1C, 0x09);
                tp28xx_byte_write(chip, 0x1D, 0x60);
            }
        }

        if (STD_HDA == std && TP2860 == id[chip]) {
            TP2860_SYSCLK_A1080P(chip);
            //TP2860_SYSCLK_144M(chip);
        } else {
            TP2825B_SYSCLK_V1(chip);
        }
        break;

    case TP2802_720P25:
        tp2802_set_work_mode_720p25(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V1_DataSet(chip);
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_720P30:
        tp2802_set_work_mode_720p30(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V1_DataSet(chip);
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_720P50:
        tp2802_set_work_mode_720p50(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V1_DataSet(chip);
        if (STD_HDA == std) {

        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2826_C720P50_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x15, 0x13);
                tp28xx_byte_write(chip, 0x16, 0x0a);
                tp28xx_byte_write(chip, 0x17, 0x00);
                tp28xx_byte_write(chip, 0x18, 0x19);
                tp28xx_byte_write(chip, 0x19, 0xd0);
                tp28xx_byte_write(chip, 0x1A, 0x25);
                tp28xx_byte_write(chip, 0x1C, 0x06);
                tp28xx_byte_write(chip, 0x1D, 0x7a);
            }
        }
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_720P60:
        tp2802_set_work_mode_720p60(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V1_DataSet(chip);
        if (STD_HDA == std) {
//            TP2826_A720P60_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2826_C720P60_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x15, 0x13);
                tp28xx_byte_write(chip, 0x16, 0x08);
                tp28xx_byte_write(chip, 0x17, 0x00);
                tp28xx_byte_write(chip, 0x18, 0x19);
                tp28xx_byte_write(chip, 0x19, 0xd0);
                tp28xx_byte_write(chip, 0x1A, 0x25);
                tp28xx_byte_write(chip, 0x1C, 0x06);
                tp28xx_byte_write(chip, 0x1D, 0x72);
            }
        }
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_720P30V2:
        tp2802_set_work_mode_720p60(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V2_DataSet(chip);
        if (STD_HDA == std) {
            if (TP2860 == id[chip]) {
                TP2860_A720P30_DataSet(chip);
            } else {
                TP2825B_A720P30_DataSet(chip);
            }
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2826_C720P30_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x15, 0x13);
                tp28xx_byte_write(chip, 0x16, 0x08);
                tp28xx_byte_write(chip, 0x17, 0x00);
                tp28xx_byte_write(chip, 0x18, 0x19);
                tp28xx_byte_write(chip, 0x19, 0xd0);
                tp28xx_byte_write(chip, 0x1A, 0x25);
                tp28xx_byte_write(chip, 0x1C, 0x06);
                tp28xx_byte_write(chip, 0x1D, 0x72);
            }
        }

        if (STD_HDA == std && TP2860 == id[chip])
            TP2860_SYSCLK_A720P(chip);
        else
            TP2825B_SYSCLK_V2(chip);
        break;

    case TP2802_720P25V2:
        tp2802_set_work_mode_720p50(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V2_DataSet(chip);
        if (STD_HDA == std) {
            if (TP2860 == id[chip])
                TP2860_A720P25_DataSet(chip);
            else
                TP2825B_A720P25_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2826_C720P25_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x15, 0x13);
                tp28xx_byte_write(chip, 0x16, 0x0a);
                tp28xx_byte_write(chip, 0x17, 0x00);
                tp28xx_byte_write(chip, 0x18, 0x19);
                tp28xx_byte_write(chip, 0x19, 0xd0);
                tp28xx_byte_write(chip, 0x1A, 0x25);
                tp28xx_byte_write(chip, 0x1C, 0x06);
                tp28xx_byte_write(chip, 0x1D, 0x7a);
            }
        }

        if (STD_HDA == std && TP2860 == id[chip])
            TP2860_SYSCLK_A720P(chip);
        else
            TP2825B_SYSCLK_V2(chip);
        break;

    case TP2802_PAL:
        tp2802_set_work_mode_PAL(chip);
        TP2825B_Set_REG0X02(chip, 0xCF);
        TP2825B_PAL_DataSet(chip);
        TP2825B_SYSCLK_CVBS(chip);
        break;

    case TP2802_NTSC:
        tp2802_set_work_mode_NTSC(chip);
        TP2825B_Set_REG0X02(chip, 0xCF);
        TP2825B_NTSC_DataSet(chip);
        TP2825B_SYSCLK_CVBS(chip);
        break;

    case TP2802_3M18:
        tp2802_set_work_mode_3M(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x30);
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_5M12:
        tp2802_set_work_mode_5M(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xd0);
        if (STD_HDA == std) {
            TP2825B_A5MP12_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_4M15:
        tp2802_set_work_mode_4M(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_3M20:
        tp2802_set_work_mode_3M20(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        tp28xx_byte_write(chip, 0x2d, 0x26);
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_6M10:
        tp2802_set_work_mode_6M10(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xbc);
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_QHD30:
        tp2802_set_work_mode_QHD30(chip);
        TP2825B_Set_REG0X02(chip, 0xD8);
        TP2825B_QHDP30_25_DataSet(chip);
        if (STD_HDA == std) {
            TP2825B_AQHDP30_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2825B_CQHDP30_DataSet(chip);
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_QHD25:
        tp2802_set_work_mode_QHD25(chip);
        TP2825B_Set_REG0X02(chip, 0xD8);
        TP2825B_QHDP30_25_DataSet(chip);
        if (STD_HDA == std) {
            TP2825B_AQHDP25_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2825B_CQHDP25_DataSet(chip);
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_QHD15:
        tp2802_set_work_mode_QHD15(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x15);
        tp28xx_byte_write(chip, 0x36, 0xdc);
        if (STD_HDA == std) {
            TP2825B_AQHDP15_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_QXGA18:
        tp2802_set_work_mode_3M(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_V1_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        if (STD_HDA == std) {
            TP2825B_AQXGAP18_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_QXGA25:
        tp2802_set_work_mode_QXGA25(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        if (STD_HDA == std) {
            TP2825B_AQXGAP25_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_QXGA30:
        tp2802_set_work_mode_QXGA30(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        if (STD_HDA == std) {
            TP2825B_AQXGAP30_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_5M20:
        tp2802_set_work_mode_5M20(chip);
        TP2825B_Set_REG0X02(chip, 0xD8);
        TP2825B_5MP20_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xbc);
        if (STD_HDA == std) {
            TP2825B_A5MP20_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2825B_C5MP20_DataSet(chip);
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_8M15:
        tp2802_set_work_mode_8M15(chip);
        TP2825B_Set_REG0X02(chip, 0xD8);
        TP2825B_8MP15_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x18);
        tp28xx_byte_write(chip, 0x36, 0xca);
        if (STD_HDA == std) {
            TP2825B_A8MP15_DataSet(chip);
        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2825B_C8MP15_DataSet(chip);
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_8M12:
        tp2802_set_work_mode_8M12(chip);
        TP2825B_Set_REG0X02(chip, 0xD8);
        TP2825B_8MP15_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x18);
        tp28xx_byte_write(chip, 0x36, 0xca);
        if (STD_HDA == std) {

        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2825B_C8MP12_DataSet(chip);
            if (STD_HDC == std) {
                tp28xx_byte_write(chip, 0x1c, 0x13);
                tp28xx_byte_write(chip, 0x1d, 0x10);
            }

        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_1080P60:
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_1080P60_DataSet(chip);
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_1080P50:
        TP2825B_Set_REG0X02(chip, 0xC8);
        TP2825B_1080P50_DataSet(chip);
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_720P14:
        tp2802_set_work_mode_720p30(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V2_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x33);
        tp28xx_byte_write(chip, 0x36, 0x20);
        TP2825B_SYSCLK_V2(chip);
        break;

    case TP2802_720P30HDR:
        tp2802_set_work_mode_720p30HDR(chip);
        TP2825B_Set_REG0X02(chip, 0xCA);
        TP2825B_V2_DataSet(chip);
        tp28xx_byte_write(chip, 0x35, 0x33);
        tp28xx_byte_write(chip, 0x36, 0x39);
        tp28xx_byte_write(chip, 0x2d, 0x28);
        TP2825B_SYSCLK_V2(chip);
        break;

    case TP2802_6M20:
        tp2802_set_work_mode_6M20(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xd0);
        if (STD_HDA == std) {

        } else if (STD_HDC == std || STD_HDC_DEFAULT == std) {
            TP2825B_C6MP20_DataSet(chip);
            if (STD_HDC == std) {
            }
        }
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_960P30:
        if (STD_HDA == std) {
            TP2825B_Set_REG0X02(chip, 0xC8);

            TP2860_A960P30_DataSet(chip);

            TP2860_SYSCLK_94500K(chip);
        } else {
            TP2825B_Set_REG0X02(chip, 0xC8);
            TP2860_960P30_DataSet(chip);
            TP2860_SYSCLK_111375K(chip);
        }
        break;

//    case TP2802_960P25:
//        TP2825B_Set_REG0X02(chip, 0xC8);
//        TP2860_960P25_DataSet(chip);
//        TP2860_SYSCLK_111375K(chip);
//        break;
    case TP2802_8M15V2:
        TP2825B_Set_REG0X02(chip, 0xC8);

        TP2831_8MP15V2_DataSet(chip);
        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_1080P15:
        tp28xx_byte_write(chip, 0x35, 0x25);
        tp2802_set_work_mode_1080p30(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);

        TP2825B_V2_DataSet(chip);

        if (STD_HDA == std)
            TP2825B_A1080P30_DataSet(chip);

        TP2825B_SYSCLK_V2(chip);
        break;

    case TP2802_1080P12:
        tp28xx_byte_write(chip, 0x35, 0x25);
        tp2802_set_work_mode_1080p25(chip);
        TP2825B_Set_REG0X02(chip, 0xC8);

        TP2825B_V2_DataSet(chip);

        if (STD_HDA == std)
            TP2825B_A1080P25_DataSet(chip);

        TP2825B_SYSCLK_V2(chip);
        break;

    case TP2802_5M20V2:
        TP2825B_Set_REG0X02(chip, 0xC8);

        TP2831_5MP20V2_DataSet(chip);

        TP2825B_SYSCLK_V3(chip);
        break;

    case TP2802_5M12V2:
        TP2825B_Set_REG0X02(chip, 0xC8);

        TP2831_5MP12V2_DataSet(chip);

        TP2825B_SYSCLK_V1(chip);
        break;

    case TP2802_960P25:
        if (STD_HDA == std) {
            TP2825B_Set_REG0X02(chip, 0xC8);

            TP2860_A960P25_DataSet(chip);

            TP2860_SYSCLK_A1080P(chip);
        } else {
            TP2825B_Set_REG0X02(chip, 0xC8);
            TP2860_960P25_DataSet(chip);
            TP2860_SYSCLK_111375K(chip);
        }

        break;
    default:
        err = -1;
        break;
    }

    if (TP2802_PAL == mode || TP2802_NTSC == mode) { //960x480/960x576
        tmp = tp28xx_byte_read(chip, 0x35);
        tmp |= 0x40;
        tp28xx_byte_write(chip, 0x35, tmp);
    }
    tmp = tp28xx_byte_read(chip, 0x06);
    tmp |= 0x80;
    tp28xx_byte_write(chip, 0x06, tmp);

    /*
        if (MUX656_8BIT == output[chip] || SEP656_8BIT == output[chip]) {
            if (TP2802_PAL == mode || TP2802_NTSC == mode) //960x480/960x576 {
    #if (CVBS_960H)
                tp28xx_byte_write(chip, 0xfa, 0x0b);

                tmp = tp28xx_byte_read(chip, 0x35);
                tmp |= 0x40;
                tp28xx_byte_write(chip, 0x35, tmp);
    #endif
            } else {
                tp28xx_byte_write(chip, 0xfa, 0x08);
            }
        } else { //16bit output
                tp28xx_byte_write(chip, 0xfa, 0x0b);
        }
    */
    return err;
}


static void tp2802_set_reg_page(unsigned char chip, unsigned char ch)
{
    switch (ch) {
    case MIPI_PAGE:
        tp28xx_byte_write(chip, 0x40, 0x08);
        break;
    default:
        tp28xx_byte_write(chip, 0x40, 0x00);
        break;
    }
}
/*
static void tp2802_manual_agc(unsigned char chip, unsigned char ch)
{
    unsigned int agc, tmp;

    tp28xx_byte_write(chip, 0x2F, 0x02);
    agc = tp28xx_byte_read(chip, 0x04);
    LOG_D("AGC=0x%04x ch%02x\r\n", agc, ch);
    agc += tp28xx_byte_read(chip, 0x04);
    agc += tp28xx_byte_read(chip, 0x04);
    agc += tp28xx_byte_read(chip, 0x04);
    agc &= 0x3f0;
    agc >>=1;
    if (agc > 0x1ff)
        agc = 0x1ff;
#if (DEBUG)
    LOG_I("AGC=0x%04x ch%02x\r\n", agc, ch);
#endif
    tp28xx_byte_write(chip, 0x08, agc&0xff);
    tmp = tp28xx_byte_read(chip, 0x06);
    tmp &=0xf9;
    tmp |=(agc>>7)&0x02;
    tmp |=0x04;
    tp28xx_byte_write(chip, 0x06,tmp);
}
*/

#if WDT
static void TP28xx_reset_default(int chip, unsigned char ch)
{
    TP2825B_reset_default(chip, ch);
}
#endif

////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static int tp2825_is_interlaced(void)
{
    return tp28xx_byte_read(0, 0x01) & 0x2 ? 0 : 1;
}

static void tp2802_comm_init(int chip)
{
    if (TP2825B == id[chip]) {
        tp28xx_byte_write(chip, 0x40, 0x00); //default Vin1
        tp28xx_byte_write(chip, 0x35, 0x25);
        tp28xx_byte_write(chip, 0xfd, 0x80);
        tp28xx_byte_write(chip, 0xfa, 0x03);

        TP2825B_reset_default(chip, VIDEO_PAGE);

        if (DEFAULT_USE_CVBS)
            tp2802_set_video_mode(chip, mode, VIDEO_PAGE, STD_TVI);
        else
            tp2802_set_video_mode(chip, mode, VIDEO_PAGE, STD_HDA);

#if (WDT)
        tp28xx_byte_write(chip, 0x26, 0x04);
#endif

        //MUX output
        TP2825B_output(chip);

        TP2825B_RX_init(chip, PTZ_RX_TVI_CMD);

    } else if (TP2850 == id[chip]) {
        tp28xx_byte_write(chip, 0x40, 0x00);
        tp28xx_byte_write(chip, 0x41, DEFAULT_VIN_CH);
        tp28xx_byte_write(chip, 0x35, 0x25);
        tp28xx_byte_write(chip, 0xfd, 0x80);
        tp28xx_byte_write(chip, 0xf0, 0x10); //default PTZ1
        tp28xx_byte_write(chip, 0xfa, 0x83);

        TP2825B_reset_default(chip, VIDEO_PAGE);

        tp2802_set_video_mode(chip, mode, VIDEO_PAGE, STD_TVI);

#if (WDT)
        tp28xx_byte_write(chip, 0x26, 0x04);
#endif

        //MUX output
        TP2850_output(chip);

        TP2825B_RX_init(chip, PTZ_RX_TVI_CMD);
    } else if (TP2860 == id[chip]) {
        tp28xx_byte_write(chip, 0x40, 0x00); //default Vin1
        tp28xx_byte_write(chip, 0x42, 0x00);
        tp28xx_byte_write(chip, 0xfd, 0x80);
        tp28xx_byte_write(chip, 0xf0, 0x10); //default PTZ1
        tp28xx_byte_write(chip, 0x72, 0x10);

        tp28xx_byte_write(chip, 0xd9, 0x68);
        tp28xx_byte_write(chip, 0xda, 0xc0);
        tp28xx_byte_write(chip, 0xdb, 0x07);
        tp28xx_byte_write(chip, 0xdc, 0x10);
        tp28xx_byte_write(chip, 0xdd, 0x80);
        tp28xx_byte_write(chip, 0xd8, 0x85);

        TP2825B_reset_default(chip, VIDEO_PAGE);

        tp2802_set_video_mode(chip, mode, VIDEO_PAGE, STD_TVI);

#if (WDT)
        tp28xx_byte_write(chip, 0x26, 0x04);
#endif

        //MUX output
        TP2860_output(chip);

        TP2825B_RX_init(chip, PTZ_RX_TVI_CMD);
    }
}

static int tp2802_module_init(void)
{
#if WDT
    int ret = 0;
#endif
    int i = 0, val = 0;
//    unsigned char chip;
    /*
        // 1st check the module parameters
        if ((mode < TP2802_1080P25) || (mode > TP2802_720P60))
        {
            LOG_E("TP2802 module param 'mode' Invalid!\n");
            return FAILURE;
        }
    */
    LOG_I("TP2825B driver version %d.%d.%d loaded\n",
          (TP2825_VERSION_CODE >> 16) & 0xff,
          (TP2825_VERSION_CODE >>  8) & 0xff,
          TP2825_VERSION_CODE & 0xff);

    if (chips <= 0 || chips > MAX_CHIPS) {
        LOG_E("TP2825B module param 'chips' invalid value:%d\n", chips);
        return FAILURE;
    }

    /* initize each tp2802*/
    for (i = 0; i < chips; i++) {
        val = tp28xx_byte_read(i, 0xfe);
        val <<= 8;
        val += tp28xx_byte_read(i, 0xff);

        if (TP2825B == val) {
            LOG_I("Chip%d: Detected TP2825B\n", i);
        } else if (TP2850 == val) {
            LOG_I("Chip%d: Detected TP2850/TP9950\n", i);
        } else if (TP2860 == val) {
            LOG_I("Chip%d: Detected TP2860\n", i);
        } else {
            LOG_I("Chip%d: Invalid chip %2x\n", i, val);
            return FAILURE;
        }

        id[i] = val;
//        tp28xx_byte_write(chip, 0x26, 0x04);
        tp2802_comm_init(i);
    }

#if (WDT)
    ret = TP2802_watchdog_init();
    if (ret) {
        LOG_E("ERROR: could not create watchdog\n");
        return ret;
    }
#endif

    LOG_I("TP2825B Driver Init Successful!\n");
    return SUCCESS;
}

#if 0
static void tp2802_module_exit(void)
{
#if (WDT)
    TP2802_watchdog_exit();
#endif
}
#endif

/////////////////////////////////////////////////////////////////
unsigned char TP2825B_read_egain(unsigned char chip)
{
    unsigned char gain;

    gain = tp28xx_byte_read(chip, 0x03);
    gain >>= 4;

    return gain;
}
/////////////////////////////////////////////////////////////////
unsigned char tp28xx_read_egain(unsigned char chip)
{
    unsigned char gain;


    tp28xx_byte_write(chip, 0x2f, 0x00);
    gain = tp28xx_byte_read(chip, 0x04);

    return gain;
}

#if WDT
//////////////////////////////////////////////////////////////////
/******************************************************************************
 *
 * TP2802_watchdog_deamon()

 *
 ******************************************************************************/
static int TP2802_watchdog_deamon(void *data)
{
    //unsigned long flags;
    int iChip, i = 0;
    struct sched_param param = { .sched_priority = 99 };
    tp2802wd_info *wdi = NULL;

    //struct timeval start, end;
    //int interval;
    unsigned char status, cvstd, gain, agc, tmp, flag_locked;
    unsigned char rx1, rx2;

    LOG_I("TP2802_watchdog_deamon: start!\n");

    sched_setscheduler(current, SCHED_FIFO, &param);
    current->flags |= PF_NOFREEZE;

    set_current_state(TASK_INTERRUPTIBLE);

    while (watchdog_state != WATCHDOG_EXIT) {
        //do_gettimeofday(&start);

        for (iChip = 0; iChip < chips; iChip++) {
            wdi = &watchdog_info[iChip];

            for (i = 0; i < CHANNELS_PER_CHIP; i++) { //scan four inputs:
                if (SCAN_DISABLE == wdi->scan[i])
                    continue;

                tp2802_set_reg_page(iChip, i);

                status = tp28xx_byte_read(iChip, 0x01);

                //state machine for video checking
                if (status & FLAG_LOSS) { //no video
                    if (VIDEO_UNPLUG != wdi->state[i]) { //switch to no video
                        wdi->state[i] = VIDEO_UNPLUG;
                        wdi->count[i] = 0;
                        if (SCAN_MANUAL != wdi->scan[i])
                            wdi->mode[i] = INVALID_FORMAT;
#if (DEBUG)
                            LOG_D("video loss ch%02x chip%2x\r\n", i, iChip);
#endif
                    }

                    if (0 == wdi->count[i]) { //first time into no video
                        if (SCAN_MANUAL != wdi->scan[i]) {
                            //tp2802_set_video_mode(iChip, DEFAULT_FORMAT, i, STD_TVI);
                            TP28xx_reset_default(iChip, i);
                        }
                        wdi->count[i]++;
                    } else {
                        if (wdi->count[i] < MAX_COUNT)
                            wdi->count[i]++;
                        continue;
                    }
                } else { //there is video
                    if (TP2802_PAL == wdi->mode[i] || TP2802_NTSC == wdi->mode[i])
                        flag_locked = FLAG_HV_LOCKED;
                    else
                        flag_locked = FLAG_HV_LOCKED;

                    if (flag_locked == (status & flag_locked)) { //video locked
                        if (VIDEO_LOCKED == wdi->state[i]) {
                            if (wdi->count[i] < MAX_COUNT)
                                wdi->count[i]++;
                        } else if (VIDEO_UNPLUG == wdi->state[i]) {
                            wdi->state[i] = VIDEO_IN;
                            wdi->count[i] = 0;
#if (DEBUG)
                            LOG_D("1video in ch%02x chip%2x\r\n", i, iChip);
#endif
                        } else if (wdi->mode[i] != INVALID_FORMAT) {
                            //if (FLAG_HV_LOCKED == (FLAG_HV_LOCKED & status))//H&V locked
                            {
                                wdi->state[i] = VIDEO_LOCKED;
                                wdi->count[i] = 0;
#if (DEBUG)
                                LOG_D("video locked %02x ch%02x chip%2x\r\n", status, i, iChip);
#endif
                            }

                        }
                    } else { //video in but unlocked
                        if (VIDEO_UNPLUG == wdi->state[i]) {
                            wdi->state[i] = VIDEO_IN;
                            wdi->count[i] = 0;
#if (DEBUG)
                            LOG_D("2video in ch%02x chip%2x\r\n", i, iChip);
#endif
                        } else if (VIDEO_LOCKED == wdi->state[i]) {
                            wdi->state[i] = VIDEO_UNLOCK;
                            wdi->count[i] = 0;
#if (DEBUG)
                            LOG_D("video unstable ch%02x chip%2x\r\n", i, iChip);
#endif
                        } else {
                            if (wdi->count[i] < MAX_COUNT)
                                wdi->count[i]++;
                            if (VIDEO_UNLOCK == wdi->state[i] && wdi->count[i] > 2) {
                                wdi->state[i] = VIDEO_IN;
                                wdi->count[i] = 0;
                                if (SCAN_MANUAL != wdi->scan[i])
                                    TP28xx_reset_default(iChip, i);
#if (DEBUG)
                                LOG_D("video unlocked ch%02x chip%2x\r\n", i, iChip);
#endif
                            }
                        }
                    }

                    if (wdi->force[i]) { //manual reset for V1/2 switching
                        wdi->state[i] = VIDEO_UNPLUG;
                        wdi->count[i] = 0;
                        wdi->mode[i] = INVALID_FORMAT;
                        wdi->force[i] = 0;
                        TP28xx_reset_default(iChip, i);
                        //tp2802_set_video_mode(iChip, DEFAULT_FORMAT, i);
                    }
                }

                //LOG_D("video state %2x detected ch%02x count %4x\r\n", wdi->state[i], i, wdi->count[i]);
                if (VIDEO_IN == wdi->state[i]) {
                    if (SCAN_MANUAL != wdi->scan[i]) {
                        //tp28xx_byte_write(iChip, 0x40, 0x08);
                        //tp28xx_byte_write(iChip, 0x13, 0x24);
                        //tp28xx_byte_write(iChip, 0x40, 0x00);
                        //tp28xx_byte_write(iChip, 0x35, 0x25);
                        cvstd = tp28xx_byte_read(iChip, 0x03);
#if (DEBUG)
                        LOG_D("video format %2x detected ch%02x chip%2x count%2x\r\n", cvstd, i, iChip, wdi->count[i]);
#endif
                        cvstd &= 0x0f;

                        wdi->std[i] = STD_TVI;
                        if (SCAN_HDA == wdi->scan[i])
                            wdi->std[i] = STD_HDA;
                        else if (SCAN_HDC == wdi->scan[i])
                            wdi->std[i] = STD_HDC;
                        /*
                        if (TP2802_SD == (cvstd&0x07)) {
                            if (wdi->count[i] & 0x01) {
                                wdi->mode[i] = TP2802_PAL;
                            } else {
                                wdi->mode[i] = TP2802_NTSC;
                            }
                            tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                        } else if ((cvstd&0x07) < 6)
                        */
                        /* bob 20210825
                        if ((cvstd&0x07) < 6)
                        {

                        if (TP2802_720P25 == (cvstd&0x07))
                        {
                            wdi->mode[i] = TP2802_720P25V2;
                        }
                        else if (TP2802_720P30 == (cvstd&0x07))
                        {
                            if (wdi->count[i] & 1)
                                wdi->mode[i] = TP2802_QHD15;
                            else
                                wdi->mode[i] = TP2802_720P30V2;
                        }
                        else if (TP2802_720P60 == (cvstd&0x07))
                        {
                            if (wdi->count[i] & 1)
                                wdi->mode[i] = TP2802_QHD30;
                            else
                                wdi->mode[i] = TP2802_720P60;
                        }
                        else if (TP2802_720P50 == (cvstd&0x07))
                        {
                        if (wdi->count[i] & 1)
                            wdi->mode[i] = TP2802_QHD25;
                        else
                            wdi->mode[i] = TP2802_720P50;
                        }
                        else if (TP2802_1080P30 == (cvstd&0x07))
                        {
                        if (wdi->count[i] & 1)
                            wdi->mode[i] = TP2802_8M15;
                        else
                            wdi->mode[i] = TP2802_1080P30;
                        }
                        else if (TP2802_1080P25 == (cvstd&0x07))
                        {
                        if (wdi->count[i] & 1)
                           wdi->mode[i] = TP2802_8M12;
                        else
                            wdi->mode[i] = TP2802_1080P25;
                        }
                        tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);

                        }
                        else //format is 7
                        */
                        {

                            tp28xx_byte_write(iChip, 0x2f, 0x09);
                            tmp = tp28xx_byte_read(iChip, 0x04);
#if (DEBUG)
                            LOG_D("detection %02x  ch%02x chip%2x\r\n", tmp, i, iChip);
#endif

                            if (0x67 == tmp) {
                                if (wdi->count[i] & 1)
                                    wdi->mode[i] = TP2802_QHD15;
                                else
                                    wdi->mode[i] = TP2802_720P30V2;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x7b == tmp) {
                                wdi->mode[i] = TP2802_720P25V2;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x44 == tmp) {
                                if (wdi->count[i] & 1)
                                    wdi->mode[i] = TP2802_8M15;
                                else
                                    wdi->mode[i] = TP2802_1080P30;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x52 == tmp) {
                                if (wdi->count[i] & 1)
                                    wdi->mode[i] = TP2802_8M12;
                                else
                                    wdi->mode[i] = TP2802_1080P25;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x33 == tmp) {
                                if (wdi->count[i] & 1)
                                    wdi->mode[i] = TP2802_QHD30;
                                else
                                    wdi->mode[i] = TP2802_720P60;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x3d == tmp) {
                                if (wdi->count[i] & 1)
                                    wdi->mode[i] = TP2802_QHD25;
                                else
                                    wdi->mode[i] = TP2802_720P50;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x4e == tmp) {
                                if (SCAN_HDA == wdi->scan[i])
                                    wdi->mode[i] = TP2802_QXGA18;
                                else if (SCAN_AUTO == wdi->scan[i] && wdi->count[i] < 3)
                                    wdi->mode[i] = TP2802_QXGA18;
                                else
                                    wdi->mode[i] = TP2802_3M18;

                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x5d == tmp) {
                                if ((wdi->count[i] % 3) == 0) {
                                    wdi->mode[i] = TP2802_5M12;
                                    wdi->std[i] = STD_HDA;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i,  wdi->std[i]);
                                } else if ((wdi->count[i] % 3) == 1) {
                                    wdi->mode[i] = TP2802_4M15;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                } else {
                                    wdi->mode[i] = TP2802_720P30HDR;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                }
                            } else if (0x5c == tmp) {
                                wdi->mode[i] = TP2802_5M12;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x75 == tmp) {
                                wdi->mode[i] = TP2802_6M10;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x38 == tmp) {
                                wdi->mode[i] = TP2802_QXGA25; //current only HDA
                                wdi->std[i] = STD_HDA;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x2e == tmp) {
                                wdi->mode[i] = TP2802_QXGA30; //current only HDA
                                wdi->std[i] = STD_HDA;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x3a == tmp) {
                                if (TP2802_5M20 != wdi->mode[i]) {
                                    wdi->mode[i] = TP2802_5M20;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                    //soft reset
                                    agc = tp28xx_byte_read(iChip, 0x06);
                                    agc |= 0x80;
                                    tp28xx_byte_write(iChip, 0x06, agc);
                                }
                            } else if (0x39 == tmp) {
                                if (TP2802_6M20 != wdi->mode[i]) {
                                    wdi->mode[i] = TP2802_6M20;
                                    wdi->std[i] = STD_HDC;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                }
                            } else if (0x89 == tmp) {
                                wdi->mode[i] = TP2802_1080P15;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x22 == tmp) {
                                wdi->mode[i] = TP2802_1080P60;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x29 == tmp) {
                                wdi->mode[i] = TP2802_1080P50;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x93 == tmp) {
                                wdi->mode[i] = TP2802_NTSC;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x94 == tmp) {
                                wdi->mode[i] = TP2802_PAL;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x7d == tmp) {
                                wdi->mode[i] = TP2802_8M7;
                                wdi->std[i] = STD_HDA;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0xa4 == tmp) {
                                wdi->mode[i] = TP2802_1080P12;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x6a == tmp) {
                                wdi->mode[i] = TP2802_5M12V2;
                                tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                            } else if (0x54 == tmp) {
                                if (TP2802_960P25 != wdi->mode[i]) {
                                    wdi->mode[i] = TP2802_960P25;
                                    wdi->std[i] = STD_HDA;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                }
                            }
                        }

                    }

                }

#define EQ_COUNT 10
                if (VIDEO_LOCKED == wdi->state[i]) { //check signal lock
                    if (0 == wdi->count[i]) {
                        tmp = tp28xx_byte_read(iChip, 0x26);
                        tmp |= 0x01;
                        tp28xx_byte_write(iChip, 0x26, tmp);

                        if ((SCAN_AUTO == wdi->scan[i] || SCAN_TVI == wdi->scan[i])) {
                            if ((TP2802_720P30V2 == wdi->mode[i]) || (TP2802_720P25V2 == wdi->mode[i])) {
                                tmp = tp28xx_byte_read(iChip, 0x03);
#if (DEBUG)
                                LOG_D("CVSTD%02x  ch%02x chip%2x\r\n", tmp, i, iChip);
#endif
                                if (!(0x08 & tmp)) {
#if (DEBUG)
                                    LOG_D("720P V1 Detected ch%02x chip%2x\r\n", i, iChip);
#endif
                                    wdi->mode[i] &= 0xf7;
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, STD_TVI); //to speed the switching

                                }
                            }

                            //these code need to keep bottom
                            {
                                tmp = tp28xx_byte_read(iChip, 0xa7);
                                tmp &= 0xfe;
                                tp28xx_byte_write(iChip, 0xa7, tmp);
                                //tp28xx_byte_write(iChip, 0x2f, 0x0f);
                                tp28xx_byte_write(iChip, 0x1f, 0x06);
                                tp28xx_byte_write(iChip, 0x1e, 0x60);
                            }
                        }
                    } else if (1 == wdi->count[i]) {
                        tmp = tp28xx_byte_read(iChip, 0xa7);
                        tmp |= 0x01;
                        tp28xx_byte_write(iChip, 0xa7, tmp);
#if (DEBUG)
                        tmp = tp28xx_byte_read(iChip, 0x01);
                        LOG_D("status%02x  ch%02x\r\n", tmp, i);
                        tmp = tp28xx_byte_read(iChip, 0x03);
                        LOG_D("CVSTD%02x  ch%02x\r\n", tmp, i);
#endif
                    } else if (wdi->count[i] < EQ_COUNT - 3) {
                        if (SCAN_AUTO == wdi->scan[i]) {
                            if (STD_TVI == wdi->std[i]) {
                                tmp = tp28xx_byte_read(iChip, 0x01);

                                if ((TP2802_PAL == wdi->mode[i]) || (TP2802_NTSC == wdi->mode[i])) {
                                    //nothing to do
                                } else if (TP2802_QXGA18 == wdi->mode[i]) {
                                    if (0x60 == (tmp & 0x64)) {
                                        wdi->std[i] = STD_HDA; //no CVI QXGA18
                                        tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                    }
                                } else if (TP2802_QHD15 == wdi->mode[i] || TP2802_5M12 == wdi->mode[i]) {
                                    if (0x60 == (tmp & 0x64)) {
                                        wdi->std[i] = STD_HDA; //no CVI QHD15/5M20/5M12.5
                                        tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                    }
                                } else if (TP2802_QHD25 == wdi->mode[i] || TP2802_QHD30 == wdi->mode[i]
                                           || TP2802_8M12 == wdi->mode[i] || TP2802_8M15 == wdi->mode[i]
                                           || TP2802_5M20 == wdi->mode[i]) {
                                    agc = tp28xx_byte_read(iChip, 0x10);
                                    tp28xx_byte_write(iChip, 0x10, 0x00);

                                    tp28xx_byte_write(iChip, 0x2f, 0x0f);
                                    rx1 = tp28xx_byte_read(iChip, 0x04);

                                    tp28xx_byte_write(iChip, 0x10, agc);

                                    if (rx1 > 0x30)
                                        wdi->std[i] = STD_HDA;
                                    else if (0x60 == (tmp & 0x64))
                                        wdi->std[i] = STD_HDC;
#if (DEBUG)
                                    LOG_D("RX1=%02x standard to %02x  ch%02x\r\n", rx1, wdi->std[i], i);
#endif
                                    if (STD_TVI != wdi->std[i])
                                        tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
                                    else if (TP2802_8M12 == wdi->mode[i] || TP2802_8M15 == wdi->mode[i])
                                        tp28xx_byte_write(iChip, 0x20, 0x50); //restore TVI clamping
                                } else if (0x60 == (tmp & 0x64)) {
                                    rx2 = tp28xx_byte_read(iChip, 0x94); //capture line7 to match 3M/4M RT

                                    if (HDC_enable) {
                                        if (0xff == rx2)
                                            wdi->std[i] = STD_HDC;
                                        else if (0x00 == rx2)
                                            wdi->std[i] = STD_HDC_DEFAULT;
                                        else
                                            wdi->std[i] = STD_HDA;
                                    } else {
                                        wdi->std[i] = STD_HDA;
                                    }

                                    if (STD_TVI != wdi->std[i]) {
                                        tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
#if (DEBUG)
                                        LOG_D("RX2=%02x standard to %02x  ch%02x\r\n", rx2, wdi->std[i], i);
#endif
                                    }
                                }
                            }
                        }
                    } else if (wdi->count[i] < EQ_COUNT) {


                    } else if (wdi->count[i] == EQ_COUNT) {
                        gain = tp28xx_read_egain(iChip);

                        if (STD_TVI != wdi->std[i])
                            tp28xx_byte_write(iChip, 0x07, 0x80 | (gain >> 2)); // manual mode

                    } else if (wdi->count[i] == EQ_COUNT + 1) {

                        if (SCAN_AUTO == wdi->scan[i]) {

                            if (HDC_enable) {
                                if (STD_HDC_DEFAULT == wdi->std[i]) {
                                    tp28xx_byte_write(iChip, 0x2f, 0x0c);
                                    tmp = tp28xx_byte_read(iChip, 0x04);
                                    status = tp28xx_byte_read(iChip, 0x01);

                                    //if (0x10 == (0x11 & status) && (tmp < 0x18 || tmp > 0xf0))
                                    if (0x10 == (0x11 & status))
                                        //if ((tmp < 0x18 || tmp > 0xf0))
                                    {
                                        wdi->std[i] = STD_HDC;
                                    } else {
                                        wdi->std[i] = STD_HDA;
                                    }
                                    tp2802_set_video_mode(iChip, wdi->mode[i], i, wdi->std[i]);
#if (DEBUG)
                                    LOG_D("reg01=%02x reg04@2f=0c %02x std%02x ch%02x\r\n", status, tmp, wdi->std[i], i);
#endif
                                }
                            }
                        }
                    } else {
                        if (SCAN_AUTO == wdi->scan[i]) {
                            /*
                            if (wdi->mode[i] < TP2802_3M18) {
                                tmp = tp28xx_byte_read(iChip, 0x03); //
                                tmp &= 0x07;
                                if (tmp != (wdi->mode[i]&0x07) && tmp < TP2802_SD) {
                                    #if (DEBUG)
                                    LOG_I("correct %02x from %02x ch%02x\r\n", tmp, wdi->mode[i], i);
                                    #endif
                                    wdi->force[i] = 1;
                                }
                            }
                            */
                        }


                    }
                }
            }
        }

        //do_gettimeofday(&end);

        //interval = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);

        //LOG_D("WDT elapsed time %d.%dms\n", interval/1000, interval%1000);

        /* sleep 0.5 seconds */
        schedule_timeout_interruptible(msecs_to_jiffies(1000) + 1);
    }

    set_current_state(TASK_RUNNING);
    LOG_I("TP2802_watchdog_deamon: exit!\n");
    return SUCCESS;
}

/******************************************************************************
 *
 * TP2825_watchdog_init()
 *
 ******************************************************************************/
int TP2802_watchdog_init(void)
{
    struct task_struct *p_dog;
    int i, j;
    watchdog_state = WATCHDOG_RUNNING;
    memset(&watchdog_info, 0, sizeof(watchdog_info));

    for (i = 0; i < MAX_CHIPS; i++) {
        watchdog_info[i].addr = tp2802_i2c_addr[i];
        for (j = 0; j < CHANNELS_PER_CHIP; j++) {
            watchdog_info[i].count[j] = 0;
            watchdog_info[i].force[j] = 0;
            //watchdog_info[i].loss[j] = 1;
            watchdog_info[i].mode[j] = INVALID_FORMAT;
            watchdog_info[i].scan[j] = SCAN_AUTO;
            watchdog_info[i].state[j] = VIDEO_UNPLUG;
            watchdog_info[i].std[j] = STD_TVI;
        }
    }

    p_dog = kthread_create(TP2802_watchdog_deamon, NULL, "WatchDog");

    if (IS_ERR(p_dog)) {
        LOG_E("TP2802_watchdog_init: create watchdog_deamon failed!\n");
        return -1;
    }

    wake_up_process(p_dog);

    task_watchdog_deamon = p_dog;

    LOG_I("TP2802_watchdog_init: done!\n");

    return SUCCESS;
}

/******************************************************************************
 *
 * TP2825_watchdog_exit()
 *
 ******************************************************************************/
void TP2802_watchdog_exit(void)
{
    struct task_struct *p_dog = task_watchdog_deamon;
    watchdog_state = WATCHDOG_EXIT;

    if (p_dog == NULL)
        return;

    wake_up_process(p_dog);

    kthread_stop(p_dog);

    yield();

    task_watchdog_deamon = NULL;

    LOG_I("TP2802_watchdog_exit: done!\n");
}
#endif

static int tp2825_s_stream(struct tp2825_dev *sensor, int enable)
{
    LOG_I("Streaming %s\n", enable ? "On" : "Off");
    sensor->streaming = enable;

#if TEST_MODE
    if (enable) {
        LOG_I("Enter test mode\n");
        tp28xx_byte_write(0, 0x2A, 0x3C);
    } else {
        LOG_I("Exit test mode\n");
        tp28xx_byte_write(0, 0x2A, 0x30);
    }
#endif
    return SUCCESS;
}

static rt_err_t tp2825_init(rt_device_t dev)
{
    struct tp2825_dev *sensor = (struct tp2825_dev *)dev;
    struct mpp_video_fmt *fmt = &sensor->fmt;
    int ret = 0;

    sensor->i2c = camera_i2c_get();
    if (!sensor->i2c)
        return -RT_EINVAL;

    tp28xx_client0 = sensor->i2c;

    /* request optional power down pin */
    sensor->pwdn_pin = camera_pwdn_pin_get();
    if (!sensor->pwdn_pin)
        return -RT_EINVAL;

    camera_pin_set_high(sensor->pwdn_pin);

    ret = tp2802_module_init();
    if (ret)
        return -EINVAL;

    fmt->code = DEFAULT_MEDIA_CODE;
    fmt->width = DEFAULT_WIDTH;
    fmt->height = DEFAULT_HEIGHT;
    fmt->bus_type = DEFAULT_BUS_TYPE;
    fmt->flags = MEDIA_SIGNAL_FIELD_ACTIVE_LOW |
                 MEDIA_SIGNAL_VSYNC_ACTIVE_HIGH |
                 MEDIA_SIGNAL_HSYNC_ACTIVE_LOW |
                 MEDIA_SIGNAL_PCLK_SAMPLE_FALLING;

    if (tp2825_is_interlaced()) {
        LOG_I("The input signal is interlace mode\n");
        fmt->flags |= MEDIA_SIGNAL_INTERLACED_MODE;
    }

    LOG_I("Init %s device\n", DRV_NAME);
    return SUCCESS;
}

static rt_err_t tp2825_open(rt_device_t dev, rt_uint16_t oflag)
{
//    struct tp2825_dev *sensor = (struct tp2825_dev *)dev;

    return RT_EOK;
}

static rt_err_t tp2825_close(rt_device_t dev)
{
//    struct tp2825_dev *sensor = (struct tp2825_dev *)dev;

//    tp2802_module_exit();
    return RT_EOK;
}

static int tp2825_get_fmt(rt_device_t dev, struct mpp_video_fmt *cfg)
{
    struct tp2825_dev *sensor = (struct tp2825_dev *)dev;

    cfg->code   = sensor->fmt.code;
    cfg->width  = sensor->fmt.width;
    cfg->height = sensor->fmt.height;
    cfg->flags  = sensor->fmt.flags;
    cfg->bus_type = sensor->fmt.bus_type;
    return RT_EOK;
}

static int tp2825_start(rt_device_t dev)
{
    struct tp2825_dev *sensor = (struct tp2825_dev *)dev;

    return tp2825_s_stream(sensor, 1);
}

static int tp2825_stop(rt_device_t dev)
{
    struct tp2825_dev *sensor = (struct tp2825_dev *)dev;

    return tp2825_s_stream(sensor, 0);
}

static rt_err_t tp2825_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd) {
    case CAMERA_CMD_START:
        return tp2825_start(dev);
    case CAMERA_CMD_STOP:
        return tp2825_stop(dev);
    case CAMERA_CMD_GET_FMT:
        return tp2825_get_fmt(dev, (struct mpp_video_fmt *)args);
    default:
        LOG_I("Unsupported cmd: 0x%x", cmd);
        return -RT_EINVAL;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops tp2825_ops =
{
    .init = tp2825_init,
    .open = tp2825_open,
    .close = tp2825_close,
    .control = tp2825_control,
};
#endif

int rt_hw_tp2825_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_tp2825_dev.dev.ops = &tp2825_ops;
#else
    g_tp2825_dev.dev.init = tp2825_init;
    g_tp2825_dev.dev.open = tp2825_open;
    g_tp2825_dev.dev.close = tp2825_close;
    g_tp2825_dev.dev.control = tp2825_control;
#endif
    g_tp2825_dev.dev.type = RT_Device_Class_CAMERA;

    rt_device_register(&g_tp2825_dev.dev, CAMERA_DEV_NAME, 0);
    return SUCCESS;
}
INIT_DEVICE_EXPORT(rt_hw_tp2825_init);

static void test_tp2825_usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -d, --dump\t\tdump the register of TP2825\n");
    printf("\t -c, --channel\t\tthe number of channel, [0, 1] \n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -d\n", program);
}

static void cmd_test_tp2825(int argc, char **argv)
{
    int c;
    u32 channel = 0;
    tp2802_register tp_reg = {0};
    const char sopts[] = "c:dh";
    const struct option lopts[] = {
        {"channel",       required_argument, NULL, 'c'},
        {"dump",          required_argument, NULL, 'd'},
        {"usage",               no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'd':
            if (tp28xx_client0)
                tp28xx_dump_reg(0);
            else
                LOG_E("Must init tp2825 first!");
            return;

        case 'c':
            channel = atoi(optarg);
            if (channel > 1) {
                LOG_E("Invalid channel number: %s", optarg);
                return;
            }
            tp_reg.value = channel;
            tp2825_set_v_input(&tp_reg);
            break;

        case 'h':
        default:
            test_tp2825_usage(argv[0]);
            return;
        }
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_test_tp2825, test_tp2825, Test TP2825 camera);
