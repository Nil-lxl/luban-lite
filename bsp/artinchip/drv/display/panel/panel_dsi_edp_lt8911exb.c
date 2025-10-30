/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"
#include <drivers/i2c.h>

#ifndef LT9811EXB_RESOLUTION
#define LT9811EXB_RESOLUTION 1
#endif

#define LT8911_I2C_NAME "i2c0"
#define LT8911_I2C_ADDR 0x29

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;

#define LT9811_BL_EN        "PE.3"
#define LT9811_IOVCC18_EN   "PE.4"
#define LT9811_RESET_EN     "PC.1"

static struct gpio_desc iovcc18;
static struct gpio_desc bl_en;
static struct gpio_desc reset;

// #define LT8911_UART_DEBUG

// #define _Test_Pattern_    // LT8911 output test pattern
#define  _read_edid_         // read eDP panel EDID
// #define _Msa_Active_Only_
#define _link_train_enable_

#ifdef LT8911_UART_DEBUG
#define LT8911_DEV_DEBUG printf
#else
#define LT8911_DEV_DEBUG(fmt, ...)
#endif

enum {
    hfp = 0,
    hs,
    hbp,
    hact,
    htotal,
    vfp,
    vs,
    vbp,
    vact,
    vtotal,
    pclk_10khz
};

enum
{
    _Level0_ = 0,                              // 27.8 mA  0x83/0x00
    _Level1_,                                  // 26.2 mA  0x82/0xe0
    _Level2_,                                  // 24.6 mA  0x82/0xc0
    _Level3_,                                  // 23 mA    0x82/0xa0
    _Level4_,                                  // 21.4 mA  0x82/0x80
    _Level5_,                                  // 18.2 mA  0x82/0x40
    _Level6_,                                  // 16.6 mA  0x82/0x20
    _Level7_,                                  // 15mA     0x82/0x00  // level 1
    _Level8_,                                  // 12.8mA   0x81/0x00  // level 2
    _Level9_,                                  // 11.2mA   0x80/0xe0  // level 3
    _Level10_,                                 // 9.6mA    0x80/0xc0  // level 4
    _Level11_,                                 // 8mA      0x80/0xa0  // level 5
    _Level12_,                                 // 6mA      0x80/0x80  // level 6
};

static u8 Level = _Level7_;  // normal

static u8 Swing_Setting1[] = { 0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x80, 0x80 };
static u8 Swing_Setting2[] = { 0x00, 0xe0, 0xc0, 0xa0, 0x80, 0x40, 0x20, 0x00, 0x00, 0xe0, 0xc0, 0xa0, 0x80 };

static bool ScrambleMode = 0;

#ifdef _read_edid_
static u8 EDID_DATA[128] = { 0 };
static u16 EDID_Timing[11] = { 0 };
static bool EDID_Reply = 0;
#endif

/*************************** LT8911EXB Config *********************************/

#define _eDP_2G7_
//#define _eDP_1G62_

#if (LT9811EXB_RESOLUTION == 0)
#define _1920x1200_eDP_Panel_
#elif (LT9811EXB_RESOLUTION == 1)
#define _1080P_eDP_Panel_
#elif (LT9811EXB_RESOLUTION == 2)
#define _1366x768_eDP_Panel_
#endif

#define _MIPI_Lane_ 4

#define _MIPI_data_PN_Swap_En   0xF0
#define _MIPI_data_PN_Swap_Dis  0x00

#define _MIPI_data_PN_ _MIPI_data_PN_Swap_Dis

#define _No_swap_           0x00    // 3210 default
#define _MIPI_data_3210_    0       // default
#define _MIPI_data_0123_    21
#define _MIPI_data_2103_    20

#define _MIPI_data_sequence_ _No_swap_


/*
 * LT8911EXB pin    MIPI RX
 * D3 (37、38)      D3  2   3   2   1   1   3   2   3   2   0   0   3   0   3   0   1   1   2   0   2   0   1   1
 * D2 (40、41)      D2  3   1   1   2   3   2   3   0   0   2   3   0   3   1   1   0   3   0   2   1   1   0   2
 * D1 (44、45)      D1  1   2   3   3   2   0   1   2   3   3   2   1   1   0   3   3   0   1   1   0   2   2   0
 * D0 (47、48)      D0  0   0   0   0   0   1   0   1   1   1   1   2   2   2   2   2   2   3   3   3   3   3   3
 *
 * 0xD003 Reg value 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23
 */

#define _eDP_data_PN_Swap_En    0xF0 // Please refer to the notes below
#define _eDP_data_PN_Swap_Dis   0x00

#define _eDP_data_PN_ _eDP_data_PN_Swap_Dis // default disable

/*
 * eDP data P/N polarity swap
 * bit7	RGD_MLCTRL_LANE3_RVSD_EN
 *         1 = data of lane3 polarity swap;
 *         0 = normal.
 *
 * bit6	RGD_MLCTRL_LANE2_RVSD_EN
 *         1 = data of lane2 polarity swap;
 *         0 = normal.
 *
 * bit5	RGD_MLCTRL_LANE1_RVSD_EN
 *         1 = data of lane1 polarity swap;
 *         0 = normal.
 *
 * bit4	RGD_MLCTRL_LANE0_RVSD_EN
 *         1 = data of lane0 polarity swap;
 *         0 = normal.
 */

#define _Lane0_data_    0
#define _Lane1_data_    1
#define _Lane2_data_    2
#define _Lane3_data_    3

#define _eDP_data_No_swap_  0xe4 // default

#define _eDP_data3_select_  (_Lane3_data_ << 6) // default; _Lane3_data_select_ is lane3
#define _eDP_data2_select_  (_Lane2_data_ << 4) // default; _Lane2_data_select_ is lane2

#define _eDP_data1_select_  (_Lane1_data_ << 2)	// default; _Lane1_data_select_ is lane1
#define _eDP_data0_select_  (_Lane0_data_ << 0) // default; _Lane0_data_select_ is lane0

// example:lane1 and lane0 swap
//#define _eDP_data1_select_    (_Lane0_data_ << 2)	// default _Lane1_data_select_ is lane0
//#define _eDP_data0_select_    (_Lane1_data_ << 0) // default _Lane0_data_select_ is lane1


/* _eDP_data_sequence_ default _eDP_data_No_swap_ */
#define _eDP_data_sequence_ (_eDP_data3_select_ + _eDP_data2_select_ + _eDP_data1_select_ + _eDP_data0_select_)

#define _Nvid 0         // 0: 0x0080,default
static int Nvid_Val[] = { 0x0080, 0x0800 };

// #define _6bit_
#define _8bit_

#define PCR_PLL_PREDIV  0x40

#ifdef _1920x1200_eDP_Panel_
#define eDP_lane        2
#endif

#ifdef _1080P_eDP_Panel_
#define eDP_lane        2
#endif

#ifdef _1366x768_eDP_Panel_
#define eDP_lane        1
#endif

static unsigned char
lt8911_i2c_reg_read(struct rt_i2c_bus_device *i2c, unsigned char reg)
{
    rt_uint8_t buffer[1];
    unsigned char cmd[1];
    struct rt_i2c_msg msgs[2];

    cmd[0] = reg;

    msgs[0].addr  = LT8911_I2C_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = cmd;
    msgs[0].len   = 1;

    msgs[1].addr  = LT8911_I2C_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buffer;
    msgs[1].len   = 1;

    if (rt_i2c_transfer(i2c_bus, msgs, 2) == 2)
    {
        return buffer[0];
    }
    else
    {
        pr_err("i2c read reg 0x%#x fail\n", reg);
        return 0;
    }
}

static void
lt8911_i2c_reg_write(struct rt_i2c_bus_device *i2c, unsigned char reg, unsigned char val)
{
    unsigned char buf[2];
    struct rt_i2c_msg msgs;

    buf[0] = reg;
    buf[1] = val;

    msgs.addr  = LT8911_I2C_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = buf;
    msgs.len   = 2;

    if (rt_i2c_transfer(i2c, &msgs, 1) == 1)
    {
        return;
    }
    else
    {
        pr_err("i2c write reg %#x fail\n", reg);
    }
}

static inline void lt8911exb_i2c_write_byte(unsigned char reg, unsigned char val)
{
    lt8911_i2c_reg_write(i2c_bus, reg, val);
}

static inline unsigned char lt8911exb_i2c_read_byte(unsigned char reg)
{
    return lt8911_i2c_reg_read(i2c_bus, reg);
}

static void lt8911_iic_setup(void)
{
    i2c_bus = rt_i2c_bus_device_find(LT8911_I2C_NAME);
    if (i2c_bus == RT_NULL)
    {
        pr_err("can't find %s device\n", LT8911_I2C_NAME);
    }
}

void lt8911exb_read_chipid(void)
{
    /* register bank */
    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x08, 0x7f);

#ifdef LT8911_UART_DEBUG
    /* LT8911EXB Chip ID: 0x17 0x05 0xE0 */
    LT8911_DEV_DEBUG("LT8911EXB Chip ID: 0x%x\n", lt8911exb_i2c_read_byte(0x00));
    LT8911_DEV_DEBUG("0x%x, \n", lt8911exb_i2c_read_byte(0x01));
    LT8911_DEV_DEBUG("0x%x, \n", lt8911exb_i2c_read_byte(0x02));
#endif
}

void lt8911exb_read_edid(void)
{
#ifdef _read_edid_
    u8 reg, i, j;
    /* bool	aux_reply, aux_ack, aux_nack, aux_defer */
    lt8911exb_i2c_write_byte(0xff, 0xac);
    lt8911exb_i2c_write_byte(0x00, 0x20); //Soft Link train
    lt8911exb_i2c_write_byte(0xff, 0xa6);
    lt8911exb_i2c_write_byte(0x2a, 0x01);

    /* set edid offset addr */
    lt8911exb_i2c_write_byte(0x2b, 0x40); //CMD
    lt8911exb_i2c_write_byte(0x2b, 0x00); //addr[15:8]
    lt8911exb_i2c_write_byte(0x2b, 0x50); //addr[7:0]
    lt8911exb_i2c_write_byte(0x2b, 0x00); //data lenth
    lt8911exb_i2c_write_byte(0x2b, 0x00); //data lenth
    lt8911exb_i2c_write_byte(0x2c, 0x00); //start Aux read edid

#ifdef LT8911_UART_DEBUG
    LT8911_DEV_DEBUG("\n");
    LT8911_DEV_DEBUG("\nRead eDP EDID......");
#endif

    /* more than 10ms */
    aicos_mdelay(20);
    reg = lt8911exb_i2c_read_byte(0x25);
    if ((reg & 0x0f) == 0x0c)
    {
        for (j = 0; j < 8; j++)
        {
            if (j == 7)
            {
                lt8911exb_i2c_write_byte(0x2b, 0x10); //MOT
            }
            else
            {
                lt8911exb_i2c_write_byte(0x2b, 0x50);
            }

            lt8911exb_i2c_write_byte(0x2b, 0x00);
            lt8911exb_i2c_write_byte(0x2b, 0x50);
            lt8911exb_i2c_write_byte(0x2b, 0x0f);
            lt8911exb_i2c_write_byte(0x2c, 0x00); //start Aux read edid
            /* more than 50ms */
            aicos_mdelay(50);

            if (lt8911exb_i2c_read_byte(0x39) == 0x31)
            {
                lt8911exb_i2c_read_byte(0x2b);
                for (i = 0; i < 16; i++)
                {
                    EDID_DATA[j * 16 + i] = lt8911exb_i2c_read_byte(0x2b);
                }

                EDID_Reply = 1;
            }
            else
            {
                EDID_Reply = 0;
#ifdef LT8911_UART_DEBUG
                LT8911_DEV_DEBUG("\nno_reply");
                LT8911_DEV_DEBUG("\n");
#endif
                return;
            }
        }

#ifdef LT8911_UART_DEBUG

        for (i = 0; i < 128; i++)
        {
            if ((i % 16) == 0)
            {
                LT8911_DEV_DEBUG("\n");
            }
            LT8911_DEV_DEBUG("0x%x, ", EDID_DATA[i]);
        }

        LT8911_DEV_DEBUG("\n");
        LT8911_DEV_DEBUG("\neDP Timing = { H_FP / H_pluse / H_BP / H_act / H_tol / V_FP / V_pluse / V_BP / V_act / V_tol / D_CLK };");
        LT8911_DEV_DEBUG("\neDP Timing = { ");
        EDID_Timing[hfp] = ((EDID_DATA[0x41] & 0xC0) * 4 + EDID_DATA[0x3e]);
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[hfp]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[hs] = ((EDID_DATA[0x41] & 0x30) * 16 + EDID_DATA[0x3f]);
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[hs]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[hbp] = (((EDID_DATA[0x3a] & 0x0f) * 0x100 + EDID_DATA[0x39]) - ((EDID_DATA[0x41] & 0x30) * 16 + EDID_DATA[0x3f]) - ((EDID_DATA[0x41] & 0xC0) * 4 + EDID_DATA[0x3e]));
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[hbp]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[hact] = ((EDID_DATA[0x3a] & 0xf0) * 16 + EDID_DATA[0x38]);
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[hact]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[htotal] = ((EDID_DATA[0x3a] & 0xf0) * 16 + EDID_DATA[0x38] + ((EDID_DATA[0x3a] & 0x0f) * 0x100 + EDID_DATA[0x39]));
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[htotal]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[vfp] = ((EDID_DATA[0x41] & 0x0c) * 4 + (EDID_DATA[0x40] & 0xf0) / 16);
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[vfp]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[vs] = ((EDID_DATA[0x41] & 0x03) * 16 + (EDID_DATA[0x40] & 0x0f));
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[vs]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[vbp] = (((EDID_DATA[0x3d] & 0x03) * 0x100 + EDID_DATA[0x3c]) - ((EDID_DATA[0x41] & 0x03) * 16 + (EDID_DATA[0x40] & 0x0f)) - ((EDID_DATA[0x41] & 0x0c) * 4 + (EDID_DATA[0x40] & 0xf0) / 16));
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[vbp]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[vact] = ((EDID_DATA[0x3d] & 0xf0) * 16 + EDID_DATA[0x3b]);
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[vact]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[vtotal] = ((EDID_DATA[0x3d] & 0xf0) * 16 + EDID_DATA[0x3b] + ((EDID_DATA[0x3d] & 0x03) * 0x100 + EDID_DATA[0x3c]));
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[vtotal]);
        LT8911_DEV_DEBUG(", ");

        EDID_Timing[pclk_10khz] = (EDID_DATA[0x37] * 0x100 + EDID_DATA[0x36]);
        LT8911_DEV_DEBUG("%d", (u32)EDID_Timing[pclk_10khz]);
        LT8911_DEV_DEBUG(" };");
        LT8911_DEV_DEBUG("\n");
#endif
    }

    return;
#endif
}

void lt8911exb_mipi_video_timing(const struct display_timing *timing)
{
    u32 hactive = timing->hactive;
    u32 hfp = timing->hfront_porch;
    u32 hs = timing->hsync_len;
    u32 hbp = timing->hback_porch;
    u32 htotal = hactive + hfp + hs + hbp;

    u32 vactive = timing->vactive;
    u32 vfp = timing->vfront_porch;
    u32 vs = timing->vsync_len;
    u32 vbp = timing->vback_porch;
    u32 vtotal = vactive + vfp + vs + vbp;

    lt8911exb_i2c_write_byte(0xff, 0xd0);
    lt8911exb_i2c_write_byte(0x0d, (u8)(vtotal / 256));
    lt8911exb_i2c_write_byte(0x0e, (u8)(vtotal % 256));
    lt8911exb_i2c_write_byte(0x0f, (u8)(vactive / 256));
    lt8911exb_i2c_write_byte(0x10, (u8)(vactive % 256));

    lt8911exb_i2c_write_byte(0x11, (u8)(htotal / 256));
    lt8911exb_i2c_write_byte(0x12, (u8)(htotal % 256));
    lt8911exb_i2c_write_byte(0x13, (u8)(hactive / 256));
    lt8911exb_i2c_write_byte(0x14, (u8)(hactive % 256));

    lt8911exb_i2c_write_byte(0x15, (u8)(vs % 256));
    lt8911exb_i2c_write_byte(0x16, (u8)(hs % 256));
    lt8911exb_i2c_write_byte(0x17, (u8)(vfp / 256));
    lt8911exb_i2c_write_byte(0x18, (u8)(vfp % 256));

    lt8911exb_i2c_write_byte(0x19, (u8)(hfp / 256));
    lt8911exb_i2c_write_byte(0x1a, (u8)(hfp % 256));
}

void lt8911exb_edo_video_cfg(const struct display_timing *timing)
{
    u32 hactive = timing->hactive;
    u32 hfp = timing->hfront_porch;
    u32 hs = timing->hsync_len;
    u32 hbp = timing->hback_porch;
    u32 htotal = hactive + hfp + hs + hbp;

    u32 vactive = timing->vactive;
    u32 vfp = timing->vfront_porch;
    u32 vs = timing->vsync_len;
    u32 vbp = timing->vback_porch;
    u32 vtotal = vactive + vfp + vs + vbp;

    lt8911exb_i2c_write_byte(0xff, 0xa8);
    lt8911exb_i2c_write_byte(0x2d, 0x88); // MSA from register

#ifdef _Msa_Active_Only_
    lt8911exb_i2c_write_byte(0x05, 0x00);
    lt8911exb_i2c_write_byte(0x06, 0x00);
    lt8911exb_i2c_write_byte(0x07, 0x00);
    lt8911exb_i2c_write_byte(0x08, 0x00);

    lt8911exb_i2c_write_byte(0x09, 0x00);
    lt8911exb_i2c_write_byte(0x0a, 0x00);
    lt8911exb_i2c_write_byte(0x0b, (u8)(hactive / 256));
    lt8911exb_i2c_write_byte(0x0c, (u8)(hactive % 256));

    lt8911exb_i2c_write_byte(0x0d, 0x00);
    lt8911exb_i2c_write_byte(0x0e, 0x00);

    lt8911exb_i2c_write_byte(0x11, 0x00);
    lt8911exb_i2c_write_byte(0x12, 0x00);
    lt8911exb_i2c_write_byte(0x14, 0x00);
    lt8911exb_i2c_write_byte(0x15, (u8)(vactive / 256));
    lt8911exb_i2c_write_byte(0x16, (u8)(vactive % 256));
#else
    lt8911exb_i2c_write_byte(0x05, (u8)(htotal / 256));
    lt8911exb_i2c_write_byte(0x06, (u8)(htotal % 256));
    lt8911exb_i2c_write_byte(0x07, (u8)((hs + hbp) / 256));
    lt8911exb_i2c_write_byte(0x08, (u8)((hs + hbp) % 256));
    lt8911exb_i2c_write_byte(0x09, (u8)(hs / 256));
    lt8911exb_i2c_write_byte(0x0a, (u8)(hs % 256));
    lt8911exb_i2c_write_byte(0x0b, (u8)(hactive / 256));
    lt8911exb_i2c_write_byte(0x0c, (u8)(hactive % 256));
    lt8911exb_i2c_write_byte(0x0d, (u8)(vtotal / 256));
    lt8911exb_i2c_write_byte(0x0e, (u8)(vtotal % 256));
    lt8911exb_i2c_write_byte(0x11, (u8)((vs + vbp) / 256));
    lt8911exb_i2c_write_byte(0x12, (u8)((vs + vbp) % 256));
    lt8911exb_i2c_write_byte(0x14, (u8)(vs % 256));
    lt8911exb_i2c_write_byte(0x15, (u8)(vactive / 256));
    lt8911exb_i2c_write_byte(0x16, (u8)(vactive % 256));
#endif
}

void lt8911exb_mipi_rx_sot_get()
{
    u8 set0 = 0;
    u8 set1 = 0;
    u8 set2 = 0;
    u8 set3 = 0;
    u8 sot0 = 0;
    u8 sot1 = 0;
    u8 sot2 = 0;
    u8 sot3 = 0;

    lt8911exb_i2c_write_byte(0xff,0xd0);
    set0 = lt8911exb_i2c_read_byte(0x88);
    set1 = lt8911exb_i2c_read_byte(0x8a);
    set2 = lt8911exb_i2c_read_byte(0x8c);
    set3 = lt8911exb_i2c_read_byte(0x8e);

    sot0 = lt8911exb_i2c_read_byte(0x89);
    sot1 = lt8911exb_i2c_read_byte(0x8b);
    sot2 = lt8911exb_i2c_read_byte(0x8d);
    sot3 = lt8911exb_i2c_read_byte(0x8f);

    LT8911_DEV_DEBUG("\nSet Num = 0x%x, 0x%x, 0x%x, 0x%x", set0,set1,set2,set3);
    LT8911_DEV_DEBUG("\nSot Dta = 0x%x, 0x%x, 0x%x, 0x%x", sot0,sot1,sot2,sot3);
}

void lt8911exb_mipi_rx_hs_settle_set(void)
{
    lt8911exb_i2c_write_byte(0xff,0xd0);
    if ((lt8911exb_i2c_read_byte(0x88) > 0x10) && (lt8911exb_i2c_read_byte(0x88) < 0x50))
    {
        LT8911_DEV_DEBUG("\n Set Mipi Rx Settle: 0x%x", (lt8911exb_i2c_read_byte(0x88) - 5));
        lt8911exb_i2c_write_byte(0xff,0xd0);
        lt8911exb_i2c_write_byte(0x02,(lt8911exb_i2c_read_byte(0x88) - 5));
    }
    else
    {
        LT8911_DEV_DEBUG("\n Set Mipi Rx Settle: 0x08"); //mipi rx cts test need settle 0x0e
        lt8911exb_i2c_write_byte(0xff,0xd0);
        lt8911exb_i2c_write_byte(0x02,0x08);
    }
}

void lt8911exb_init(struct aic_panel *panel)
{
    const struct display_timing *timing = panel->timings;
    u16 pclk_10khz = timing->pixelclock / 10000;
    u8 pcr_pll_postdiv, pcr_m, i;

    /* init */
    lt8911exb_i2c_write_byte(0xff, 0x81); // Change Reg bank
    lt8911exb_i2c_write_byte(0x08, 0x7f); // i2c over aux issue
    lt8911exb_i2c_write_byte(0x49, 0xff); // enable 0x87xx

    lt8911exb_i2c_write_byte(0xff, 0x82); // Change Reg bank
    lt8911exb_i2c_write_byte(0x5a, 0x0e); // GPIO test output

    /* for power consumption */
    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x05, 0x06);
    lt8911exb_i2c_write_byte(0x43, 0x00);
    lt8911exb_i2c_write_byte(0x44, 0x1f);
    lt8911exb_i2c_write_byte(0x45, 0xf7);
    lt8911exb_i2c_write_byte(0x46, 0xf6);
    lt8911exb_i2c_write_byte(0x49, 0x7f);

    lt8911exb_i2c_write_byte(0xff, 0x82);
#if (eDP_lane == 2)
    {
        lt8911exb_i2c_write_byte(0x12, 0x33);
    }
#elif (eDP_lane == 1)
    {
        lt8911exb_i2c_write_byte(0x12, 0x11);
    }
#endif

    /* mipi Rx analog */
    lt8911exb_i2c_write_byte(0xff, 0x82); // Change Reg bank
    lt8911exb_i2c_write_byte(0x32, 0x51);
    lt8911exb_i2c_write_byte(0x35, 0x22); //EQ current 0x22/0x42/0x62/0x82/0xA2/0xC2/0xe2
    lt8911exb_i2c_write_byte(0x3a, 0x77); //EQ 12.5db
    lt8911exb_i2c_write_byte(0x3b, 0x77); //EQ 12.5db

    lt8911exb_i2c_write_byte(0x4c, 0x0c);
    lt8911exb_i2c_write_byte(0x4d, 0x00);

    /* dessc_pcr  pll analog */
    lt8911exb_i2c_write_byte(0xff, 0x82); // Change Reg bank
    lt8911exb_i2c_write_byte(0x6a, 0x40);
    lt8911exb_i2c_write_byte(0x6b, PCR_PLL_PREDIV);

    if (pclk_10khz < 8800)
    {
        lt8911exb_i2c_write_byte(0x6e, 0x82); //0x44:pre-div = 2 ,pixel_clk = 44~ 88MHz
        pcr_pll_postdiv = 0x08;
    }
    else if (pclk_10khz < 17600)
    {
        lt8911exb_i2c_write_byte(0x6e, 0x81); //0x40:pre-div = 1, pixel_clk = 88~176MHz
        pcr_pll_postdiv = 0x04;
    }
    else
    {
        lt8911exb_i2c_write_byte(0x6e, 0x80); //0x40:pre-div = 0, pixel_clk = 176~200MHz
        pcr_pll_postdiv = 0x02;
    }

    pcr_m = (u8)(pclk_10khz * pcr_pll_postdiv / 25 / 100);

    /* dessc pll digital */
    lt8911exb_i2c_write_byte(0xff, 0x85);     // Change Reg bank
    lt8911exb_i2c_write_byte(0xa9, 0x31);
    lt8911exb_i2c_write_byte(0xaa, 0x17);
    lt8911exb_i2c_write_byte(0xab, 0xba);
    lt8911exb_i2c_write_byte(0xac, 0xe1);
    lt8911exb_i2c_write_byte(0xad, 0x47);
    lt8911exb_i2c_write_byte(0xae, 0x01);
    lt8911exb_i2c_write_byte(0xae, 0x11);

    /* Digital Top */
    lt8911exb_i2c_write_byte(0xff, 0x85);                             // Change Reg bank
    lt8911exb_i2c_write_byte(0xc0, 0x01);                             //select mipi Rx
#ifdef _6bit_
    lt8911exb_i2c_write_byte(0xb0, 0xd0);                             //enable dither
#else
    lt8911exb_i2c_write_byte(0xb0, 0x00);                             // disable dither
#endif

    /* mipi Rx Digital */
    lt8911exb_i2c_write_byte(0xff, 0xd0);                             // Change Reg bank
    lt8911exb_i2c_write_byte(0x00, _MIPI_data_PN_ + _MIPI_Lane_ % 4); // 0: 4 Lane / 1: 1 Lane / 2 : 2 Lane / 3: 3 Lane

    lt8911exb_mipi_rx_sot_get();
    lt8911exb_mipi_rx_hs_settle_set();

//  lt8911exb_i2c_write_byte(0x02, 0x08);                             //settle
    lt8911exb_i2c_write_byte(0x03, _MIPI_data_sequence_);             // default is 0x00
    lt8911exb_i2c_write_byte(0x08, 0x00);
//  lt8911exb_i2c_write_byte(0x0a, 0x12);                             //pcr mode

    lt8911exb_i2c_write_byte(0x0c, 0x80);                             //fifo position
    lt8911exb_i2c_write_byte(0x1c, 0x80);                             //fifo position

//  lt8911exb_i2c_write_byte(0x1e, 0x10);

    lt8911exb_i2c_write_byte(0x24, 0x70);                             // 0x30  [3:0]  line limit

    lt8911exb_i2c_write_byte(0x31, 0x0a);

    /* stage1 hs mode */
    lt8911exb_i2c_write_byte(0x25, 0x90);                    // 0x80    line limit
    lt8911exb_i2c_write_byte(0x2a, 0x3a);                    // 0x04    step in limit
    lt8911exb_i2c_write_byte(0x21, 0x4f);                    // hs_step
    lt8911exb_i2c_write_byte(0x22, 0xff);

    /* stage2 de mode */
    lt8911exb_i2c_write_byte(0x0a, 0x02);                    // de adjust pre line
    lt8911exb_i2c_write_byte(0x38, 0x02);                    // de_threshold 1
    lt8911exb_i2c_write_byte(0x39, 0x04);                    // de_threshold 2
    lt8911exb_i2c_write_byte(0x3a, 0x08);                    // de_threshold 3
    lt8911exb_i2c_write_byte(0x3b, 0x10);                    // de_threshold 4

    lt8911exb_i2c_write_byte(0x3f, 0x04);                    // de_step 1
    lt8911exb_i2c_write_byte(0x40, 0x08);                    // de_step 2
    lt8911exb_i2c_write_byte(0x41, 0x10);                    // de_step 3
    lt8911exb_i2c_write_byte(0x42, 0x60);                    // de_step 4

    /* stage2 hs mode */
    lt8911exb_i2c_write_byte(0x1e, 0x01);                    // 0x11
    lt8911exb_i2c_write_byte(0x23, 0xf0);                    // 0x80

    lt8911exb_i2c_write_byte(0x2b, 0x80);                    // 0xa0

#ifdef _Test_Pattern_
    lt8911exb_i2c_write_byte(0x26, (pcr_m | 0x80));
#else

    lt8911exb_i2c_write_byte(0x26, pcr_m);

//  lt8911exb_i2c_write_byte(0x27, Read_0xD095);
//  lt8911exb_i2c_write_byte(0x28, Read_0xD096);
#endif

    lt8911exb_mipi_video_timing(timing);         // defualt setting is 1080P

    lt8911exb_i2c_write_byte(0xff, 0x81);       // Change Reg bank
    lt8911exb_i2c_write_byte(0x03, 0x7b);       // PCR reset
    lt8911exb_i2c_write_byte(0x03, 0xff);

#ifdef _eDP_2G7_
    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x19, 0x31);
//  lt8911exb_i2c_write_byte(0x1a, 0x36); // sync m
    lt8911exb_i2c_write_byte(0x1a, 0x1b);
    lt8911exb_i2c_write_byte(0x1b, 0x00); // sync_k [7:0]
    lt8911exb_i2c_write_byte(0x1c, 0x00); // sync_k [13:8]

    /* txpll Analog */
    lt8911exb_i2c_write_byte(0xff, 0x82);
    lt8911exb_i2c_write_byte(0x09, 0x00); // div hardware mode, for ssc.

//  lt8911exb_i2c_write_byte(0x01, 0x18); // default : 0x18
    lt8911exb_i2c_write_byte(0x02, 0x42);
    lt8911exb_i2c_write_byte(0x03, 0x00); // txpll en = 0
    lt8911exb_i2c_write_byte(0x03, 0x01); // txpll en = 1
//  lt8911exb_i2c_write_byte(0x04, 0x3a); // default : 0x3A
    lt8911exb_i2c_write_byte(0x0a,0x1b);
    lt8911exb_i2c_write_byte(0x04,0x2a);

    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x0c, 0x10); // cal en = 0

    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x09, 0xfc);
    lt8911exb_i2c_write_byte(0x09, 0xfd);

    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x0c, 0x11); // cal en = 1

    /* ssc */
    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x13, 0x83);
    lt8911exb_i2c_write_byte(0x14, 0x41);
    lt8911exb_i2c_write_byte(0x16, 0x0a);
    lt8911exb_i2c_write_byte(0x18, 0x0a);
    lt8911exb_i2c_write_byte(0x19, 0x33);
#endif

#ifdef _eDP_1G62_
    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x19, 0x31);
    lt8911exb_i2c_write_byte(0x1a, 0x20); // sync m
    lt8911exb_i2c_write_byte(0x1b, 0x19); // sync_k [7:0]
    lt8911exb_i2c_write_byte(0x1c, 0x99); // sync_k [13:8]

    /* txpll Analog */
    lt8911exb_i2c_write_byte(0xff, 0x82);
    lt8911exb_i2c_write_byte(0x09, 0x00); // div hardware mode, for ssc.
//  lt8911exb_i2c_write_byte(0x01, 0x18); // default : 0x18
    lt8911exb_i2c_write_byte(0x02, 0x42);
    lt8911exb_i2c_write_byte(0x03, 0x00); // txpll en = 0
    lt8911exb_i2c_write_byte(0x03, 0x01); // txpll en = 1
//  lt8911exb_i2c_write_byte(0x04, 0x3a); // default : 0x3A

    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x0c, 0x10); // cal en = 0

    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x09, 0xfc);
    lt8911exb_i2c_write_byte(0x09, 0xfd);

    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x0c, 0x11); // cal en = 1

    /* ssc */
    lt8911exb_i2c_write_byte(0xff, 0x87);
    lt8911exb_i2c_write_byte(0x13, 0x83);
    lt8911exb_i2c_write_byte(0x14, 0x41);
    lt8911exb_i2c_write_byte(0x16, 0x0a);
    lt8911exb_i2c_write_byte(0x18, 0x0a);
    lt8911exb_i2c_write_byte(0x19, 0x33);
#endif

    lt8911exb_i2c_write_byte(0xff, 0x87);

    for (i = 0; i < 5; i++)
    {
        aicos_mdelay(5);
        if (lt8911exb_i2c_read_byte(0x37) & 0x02)
        {
            LT8911_DEV_DEBUG("\nLT8911 tx pll locked\n");
            lt8911exb_i2c_write_byte(0xff,0x87);
            lt8911exb_i2c_write_byte(0x1a,0x36);
            lt8911exb_i2c_write_byte(0xff,0x82);
            lt8911exb_i2c_write_byte(0x0a,0x36);
            lt8911exb_i2c_write_byte(0x04,0x3a);
            break;
        }
        else
        {
            LT8911_DEV_DEBUG("LT8911 tx pll unlocked\n");
            lt8911exb_i2c_write_byte(0xff, 0x81);
            lt8911exb_i2c_write_byte(0x09, 0xfc);
            lt8911exb_i2c_write_byte(0x09, 0xfd);

            lt8911exb_i2c_write_byte(0xff, 0x87);
            lt8911exb_i2c_write_byte(0x0c, 0x10);
            lt8911exb_i2c_write_byte(0x0c, 0x11);
        }
    }

    lt8911exb_i2c_write_byte(0xff, 0xac);                   // Change Reg bank
    lt8911exb_i2c_write_byte(0x15, _eDP_data_sequence_);    // eDP data swap
    lt8911exb_i2c_write_byte(0x16, _eDP_data_PN_);          // eDP P / N swap

    /* AUX reset */
    lt8911exb_i2c_write_byte(0xff, 0x81); // Change Reg bank
    lt8911exb_i2c_write_byte(0x07, 0xfe);
    lt8911exb_i2c_write_byte(0x07, 0xff);
    lt8911exb_i2c_write_byte(0x0a, 0xfc);
    lt8911exb_i2c_write_byte(0x0a, 0xfe);

    /* tx phy */
    lt8911exb_i2c_write_byte(0xff, 0x82); // Change Reg bank
    lt8911exb_i2c_write_byte(0x11, 0x00);
    lt8911exb_i2c_write_byte(0x13, 0x10);
    lt8911exb_i2c_write_byte(0x14, 0x0c);
    lt8911exb_i2c_write_byte(0x14, 0x08);
    lt8911exb_i2c_write_byte(0x13, 0x20);

    lt8911exb_i2c_write_byte(0xff, 0x82); // Change Reg bank
    lt8911exb_i2c_write_byte(0x0e, 0x35);
//  lt8911exb_i2c_write_byte(0x12, 0xff);
//  lt8911exb_i2c_write_byte(0xff, 0x80);
//  lt8911exb_i2c_write_byte(0x40, 0x22);

    /* eDP Tx Digital */
    lt8911exb_i2c_write_byte(0xff, 0xa8); // Change Reg bank

#ifdef _Test_Pattern_

    lt8911exb_i2c_write_byte(0x24, 0x50); // bit2 ~ bit 0 : test panttern image mode
    lt8911exb_i2c_write_byte(0x25, 0x70); // bit6 ~ bit 4 : test Pattern color
    lt8911exb_i2c_write_byte(0x27, 0x50); //0x50:Pattern; 0x10:mipi video

//  lt8911exb_i2c_write_byte(0x2d, 0x00); // pure color setting
//  lt8911exb_i2c_write_byte(0x2d, 0x84); // black color
    lt8911exb_i2c_write_byte(0x2d, 0x88); // block

#else
    lt8911exb_i2c_write_byte(0x27, 0x10); //0x50:Pattern; 0x10:mipi video
#endif

#ifdef _6bit_
    lt8911exb_i2c_write_byte(0x17, 0x00);
    lt8911exb_i2c_write_byte(0x18, 0x00);
#else /* _8bit_ */
    lt8911exb_i2c_write_byte(0x17, 0x10);
    lt8911exb_i2c_write_byte(0x18, 0x20);
#endif

    /* nvid */
    lt8911exb_i2c_write_byte(0xff, 0xa0);                            // Change Reg bank
    lt8911exb_i2c_write_byte(0x00, (u8)(Nvid_Val[_Nvid] / 256));    // 0x08
    lt8911exb_i2c_write_byte(0x01, (u8)(Nvid_Val[_Nvid] % 256));    // 0x00
}

void lt8911exb_txswing_preset(void)
{
    lt8911exb_i2c_write_byte(0xFF, 0x82);
    lt8911exb_i2c_write_byte(0x22, Swing_Setting1[Level]);    //lane 0 tap0
    lt8911exb_i2c_write_byte(0x23, Swing_Setting2[Level]);
    lt8911exb_i2c_write_byte(0x24, 0x80);                     //lane 0 tap1
    lt8911exb_i2c_write_byte(0x25, 0x00);

#if (eDP_lane == 2)
    lt8911exb_i2c_write_byte(0x26, Swing_Setting1[Level]);    //lane 1 tap0
    lt8911exb_i2c_write_byte(0x27, Swing_Setting2[Level]);
    lt8911exb_i2c_write_byte(0x28, 0x80);                     //lane 1 tap1
    lt8911exb_i2c_write_byte(0x29, 0x00);
#endif
}

void dpcd_write(u32 address, u8 data)
{
    /*
     * Pay attention to the Big-Endian and Little-Endian!
     * The default mode is Big-Endian here.
     */
    u8 address_h = 0x0f & (address >> 16);
    u8 address_m = 0xff & (address >> 8);
    u8 address_l = 0xff & address;

    u8 reg;

    lt8911exb_i2c_write_byte(0xff, 0xa6);
    lt8911exb_i2c_write_byte(0x2b, (0x80 | address_h));    //CMD
    lt8911exb_i2c_write_byte(0x2b, address_m);             //addr[15:8]
    lt8911exb_i2c_write_byte(0x2b, address_l);             //addr[7:0]
    lt8911exb_i2c_write_byte(0x2b, 0x00);                  //data lenth
    lt8911exb_i2c_write_byte(0x2b, data);                  //data
    lt8911exb_i2c_write_byte(0x2c, 0x00);                  //start Aux

    /* more than 10ms */
    aicos_mdelay(20);
    reg = lt8911exb_i2c_read_byte(0x25);

    if ((reg & 0x0f) == 0x0c)
    {
        return;
    }
}

void lt8911exb_link_train(void)
{
    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x06, 0xdf); // rset VID TX
    lt8911exb_i2c_write_byte(0x06, 0xff);

    lt8911exb_i2c_write_byte(0xff, 0x85);

//  lt8911exb_i2c_write_byte(0x17, 0xf0); // turn off scramble

    if (ScrambleMode)
    {
        lt8911exb_i2c_write_byte(0xa1, 0x82); // eDP scramble mode;

        /* Aux operater init */
        lt8911exb_i2c_write_byte(0xff, 0xac);
        lt8911exb_i2c_write_byte(0x00, 0x20); //Soft Link train
        lt8911exb_i2c_write_byte(0xff, 0xa6);
        lt8911exb_i2c_write_byte(0x2a, 0x01);

        dpcd_write(0x010a, 0x01);
        aicos_mdelay(10);
        dpcd_write(0x0102, 0x00);
        aicos_mdelay(10);
        dpcd_write(0x010a, 0x01);

        aicos_mdelay(200);
    }
    else
    {
        lt8911exb_i2c_write_byte(0xa1, 0x02); // DP scramble mode;
    }

    /* Aux setup */
    lt8911exb_i2c_write_byte(0xff, 0xac);
    lt8911exb_i2c_write_byte(0x00, 0x60); //Soft Link train
    lt8911exb_i2c_write_byte(0xff, 0xa6);
    lt8911exb_i2c_write_byte(0x2a, 0x00);

    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x07, 0xfe);
    lt8911exb_i2c_write_byte(0x07, 0xff);
    lt8911exb_i2c_write_byte(0x0a, 0xfc);
    lt8911exb_i2c_write_byte(0x0a, 0xfe);

    /* link train */

    lt8911exb_i2c_write_byte(0xff, 0x85);
    lt8911exb_i2c_write_byte(0x1a, eDP_lane);

#ifdef _link_train_enable_
    lt8911exb_i2c_write_byte(0xff, 0xac);
    lt8911exb_i2c_write_byte(0x00, 0x64);
    lt8911exb_i2c_write_byte(0x01, 0x0a);
    lt8911exb_i2c_write_byte(0x0c, 0x85);
    lt8911exb_i2c_write_byte(0x0c, 0xc5);
#else
    lt8911exb_i2c_write_byte(0xff, 0xac);
    lt8911exb_i2c_write_byte(0x00, 0x00);
    lt8911exb_i2c_write_byte(0x01, 0x0a);
    lt8911exb_i2c_write_byte(0x14, 0x80);
    lt8911exb_i2c_write_byte(0x14, 0x81);
    aicos_mdelay(50);
    lt8911exb_i2c_write_byte(0x14, 0x84);
    aicos_mdelay(50);
    lt8911exb_i2c_write_byte(0x14, 0xc0);
#endif
}

void lt8911exb_link_train_result_check(void)
{
#ifdef _link_train_enable_
    u8 i, val;

    lt8911exb_i2c_write_byte(0xff, 0xac);
    for (i = 0; i < 10; i++)
    {
        val = lt8911exb_i2c_read_byte(0x82);
        if (val & 0x20)
        {
            if ((val & 0x1f) == 0x1e)
            {
                LT8911_DEV_DEBUG("edp link train successed.\n");
                return;
            }
            else
            {
                LT8911_DEV_DEBUG("edp link train failed.\n");
                lt8911exb_i2c_write_byte(0xff, 0xac);
                lt8911exb_i2c_write_byte(0x00, 0x00);
                lt8911exb_i2c_write_byte(0x01, 0x0a);
                lt8911exb_i2c_write_byte(0x14, 0x80);
                lt8911exb_i2c_write_byte(0x14, 0x81);
                aicos_mdelay(50);
                lt8911exb_i2c_write_byte(0x14, 0x84);
                aicos_mdelay(50);
                lt8911exb_i2c_write_byte(0x14, 0xc0);
            }
#ifdef LT8911_UART_DEBUG
            val = lt8911exb_i2c_read_byte(0x83);
            LT8911_DEV_DEBUG("panel link rate: %d\n", val);

            val = lt8911exb_i2c_read_byte(0x84);
            LT8911_DEV_DEBUG("panel link count: %d\n", val);
#endif
            aicos_mdelay(100);
        }
        else
        {
            LT8911_DEV_DEBUG("link trian on going...\n");
            aicos_mdelay(100);
        }
    }
#endif
}

/* mipi should be ready before configuring below video check setting */
void lt8911exb_video_check(void)
{
    u32 reg = 0x00;
    /* mipi byte clk check*/
    lt8911exb_i2c_write_byte(0xff, 0x85); // Change Reg bank
    lt8911exb_i2c_write_byte(0x1d, 0x00); // FM select byte clk
    lt8911exb_i2c_write_byte(0x40, 0xf7);
    lt8911exb_i2c_write_byte(0x41, 0x30);

    if (ScrambleMode)
    {
        lt8911exb_i2c_write_byte(0xa1, 0x82); // eDP scramble mode;
    }
    else
    {
        lt8911exb_i2c_write_byte(0xa1, 0x02); // DP scramble mode;
    }

//  lt8911exb_i2c_write_byte(0x17, 0xf0); // 0xf0:Close scramble; 0xD0 : Open scramble

    lt8911exb_i2c_write_byte(0xff, 0x81);
    lt8911exb_i2c_write_byte(0x09, 0x7d);
    lt8911exb_i2c_write_byte(0x09, 0xfd);

    lt8911exb_i2c_write_byte(0xff, 0x85);
    aicos_mdelay(200);
    if (lt8911exb_i2c_read_byte(0x50) == 0x03)
    {
        reg = lt8911exb_i2c_read_byte(0x4d);
        reg = reg * 256 + lt8911exb_i2c_read_byte(0x4e);
        reg = reg * 256 + lt8911exb_i2c_read_byte(0x4f);

        LT8911_DEV_DEBUG("video check: mipi byteclk =  %d x1000\n", reg);
    }
    else
    {
        LT8911_DEV_DEBUG("video check: mipi clk unstable\n");
    }

    reg = lt8911exb_i2c_read_byte(0x76);
    reg = reg * 256 + lt8911exb_i2c_read_byte(0x77);
    LT8911_DEV_DEBUG("video check: Vtotal =  %d\n", reg);

    lt8911exb_i2c_write_byte(0xff, 0xd0);
    reg = lt8911exb_i2c_read_byte(0x82);
    reg = reg * 256 + lt8911exb_i2c_read_byte(0x83);
    reg = reg / 3;
    LT8911_DEV_DEBUG("video check: Hact(word counter) =  %d\n", reg);

    reg = lt8911exb_i2c_read_byte(0x85);
    reg = reg * 256 + lt8911exb_i2c_read_byte(0x86);
    LT8911_DEV_DEBUG("video check: Vact = %d\n", reg);
}

void lt8911exb_pcr_clk_status_check(void)
{
#ifdef LT8911_UART_DEBUG
    u8 reg;

    lt8911exb_i2c_write_byte(0xff, 0xd0);
    reg = lt8911exb_i2c_read_byte(0x87);

    LT8911_DEV_DEBUG("Reg 0xD087 = 0x%02x\n", reg);

    if (reg & 0x10)
    {
        LT8911_DEV_DEBUG("PCR Clock stable\n");
    }
    else
    {
        LT8911_DEV_DEBUG("PCR Clock unstable\n");
    }
    LT8911_DEV_DEBUG("\n");
#endif
}

static void panel_gpio_init(void)
{
    panel_get_gpio(&iovcc18, LT9811_IOVCC18_EN);
    panel_get_gpio(&reset, LT9811_RESET_EN);

    panel_gpio_set_value(&iovcc18, 1);
    aicos_mdelay(120);

    panel_gpio_set_value(&reset, 1);
    aicos_mdelay(120);
    panel_gpio_set_value(&reset, 0);
    aicos_mdelay(120);
    panel_gpio_set_value(&reset, 1);
    aicos_mdelay(120);
}

static void panel_bl_enable(void)
{
    panel_get_gpio(&bl_en, LT9811_BL_EN);
    panel_gpio_set_value(&bl_en, 1);
}

static int panel_enable(struct aic_panel *panel)
{
    panel_gpio_init();

    /* ArtInChip output mipi video single first */
    panel_di_enable(panel, 0);
    panel_dsi_setup_realmode(panel);
    panel_de_timing_enable(panel, 0);

    lt8911_iic_setup();
    /* read lt8911 chipid to check i2c */
    lt8911exb_read_chipid();

    lt8911exb_edo_video_cfg(panel->timings);
    lt8911exb_init(panel);

    /* read eDP panel edid */
    lt8911exb_read_edid();

    lt8911exb_txswing_preset();
    lt8911exb_link_train();

    /* debug eDP Link Train and mipi video signal */
    lt8911exb_link_train_result_check();
    lt8911exb_video_check();
    lt8911exb_pcr_clk_status_check();

    panel_bl_enable();
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

#ifdef _1920x1200_eDP_Panel_
static struct display_timing lt9811_timing = {
    .pixelclock   = 155 * 1000 * 1000,

    .hactive      = 1920,
    .hback_porch  = 80,
    .hfront_porch = 48,
    .hsync_len    = 32,

    .vactive      = 1200,
    .vback_porch  = 20,
    .vfront_porch = 5,
    .vsync_len    = 5,
};
#endif

#ifdef _1080P_eDP_Panel_
static struct display_timing lt9811_timing = {
    .pixelclock   = 150 * 1000 * 1000,

    .hactive      = 1920,
    .hback_porch  = 148,
    .hfront_porch = 188,
    .hsync_len    = 44,

    .vactive      = 1080,
    .vback_porch  = 36,
    .vfront_porch = 4,
    .vsync_len    = 5,

};
#endif

#ifdef _1366x768_eDP_Panel_
static struct display_timing lt9811_timing = {
    .pixelclock   = 72000000,

    .hactive      = 1366,
    .hback_porch  = 64,
    .hfront_porch = 14,
    .hsync_len    = 56,

    .vactive      = 768,
    .vback_porch  = 28,
    .vfront_porch = 1,
    .vsync_len    = 3,
};
#endif

struct panel_dsi dsi = {
    .mode = DSI_MOD_VID_EVENT,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

struct aic_panel dsi_edp_lt9811exb = {
    .name = "panel-dsi-eDP-lt9811exb",
    .timings = &lt9811_timing,
    .funcs = &panel_funcs,
    .dsi = &dsi,
    .connector_type = AIC_MIPI_COM,
};
