/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>

#include "panel_com.h"

#define LT8911_BLEN     "PE.4"
#define LT8911_RESET    "PA.8"
#define LT8911_I2C_NAME "i2c0"
#define LT8911_I2C_ADDR 0x29

#define _1_Port_    0x80
#define _2_Port_    0x00

#define _JEIDA_     0x40
#define _VESA_      0x00

#define _Sync_Mode_ 0x00
#define _DE_Mode_   0x20

#define LT8911_LVDS_PORT _2_Port_
#define LT8911_LVDS_RX   _VESA_
#define LT8911_LVDS_MODE _DE_Mode_

#define _LANE0_DATA_    0
#define _LANE1_DATA_    1
#define _LANE2_DATA_    2
#define _LANE3_DATA_    3

#define _EDP_DATA3_Select_  (_LANE3_DATA_ << 6)
#define _EDP_DATA2_Select_  (_LANE2_DATA_ << 4)

#define _EDP_DATA1_Select_  (_LANE1_DATA_ << 2)
#define _EDP_DATA0_Select_  (_LANE0_DATA_ << 0)

#define LT8911_EDP_DATA_SEQUENCE (_EDP_DATA3_Select_ + _EDP_DATA2_Select_ + \
                                  _EDP_DATA1_Select_ + _EDP_DATA0_Select_)

// #define LT8911_DEBUG

#ifdef LT8911_DEBUG
#define LT8911_DEV_DEBUG pr_info
#else
#define LT8911_DEV_DEBUG(fmt, ...)
#endif

static struct gpio_desc lt8911_blen;
static struct gpio_desc lt8911_reset;

struct reg_sequence {
    unsigned int reg;
    unsigned int def;
};

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;

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

static void
lt8911_i2c_multi_reg_write(struct rt_i2c_bus_device *i2c,
                        const struct reg_sequence *regs, int num_regs)
{
    int i;

    for (i = 0; i < num_regs; i++)
        lt8911_i2c_reg_write(i2c, regs[i].reg, regs[i].def);

}

static void lt8911_check_id(void)
{
#ifdef LT8911_DEBUG
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x08, 0x7f);

    LT8911_DEV_DEBUG("LT8911EX chip ID: 0x%x 0x%x 0x%x\n",
                            /* 0x17 0x05 0xe0 */
            lt8911_i2c_reg_read(i2c_bus, 0x00),
            lt8911_i2c_reg_read(i2c_bus, 0x01),
            lt8911_i2c_reg_read(i2c_bus, 0x02));
#endif
}

static void lt8911_edp_video_cfg(struct aic_panel *panel)
{
    struct display_timing *timing = panel->timings;
    u32 hactive, htotal, hpw, hfp, hbp;
    u32 vactive, vtotal, vpw, vfp, vbp;
    u32 pixelclock, dessc_m;

    hactive = timing->hactive;
    hfp = timing->hfront_porch;
    hpw = timing->hsync_len;
    hbp = timing->hback_porch;
    htotal = hactive + hfp + hpw + hbp;

    vactive = timing->vactive;
    vfp = timing->vfront_porch;
    vpw = timing->vsync_len;
    vbp = timing->vback_porch;
    vtotal = vactive + vfp + vpw + vbp;

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xa8);

    lt8911_i2c_reg_write(i2c_bus, 0x2d, 0x04);
    lt8911_i2c_reg_write(i2c_bus, 0x05, htotal / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x06, htotal % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x07, (hpw + hbp) / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x08, (hpw + hbp) % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x09, hpw / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x0a, hpw % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x0b, hactive / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x0c, hactive % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x0d, vtotal / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x0e, vtotal % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x11, (vpw + vbp) / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x12, (vpw + vbp) % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x14, vpw % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x15, vactive / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x16, vactive % 256);

    /* LVDS de only mode to regenerate h/v sync. */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xd8);
    lt8911_i2c_reg_write(i2c_bus, 0x20, hfp / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x21, hfp % 256);

    lt8911_i2c_reg_write(i2c_bus, 0x22, hpw / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x23, hpw % 256);

    lt8911_i2c_reg_write(i2c_bus, 0x24, htotal / 256);
    lt8911_i2c_reg_write(i2c_bus, 0x25, htotal % 256);

    lt8911_i2c_reg_write(i2c_bus, 0x26, vfp % 256);
    lt8911_i2c_reg_write(i2c_bus, 0x27, vpw % 256);

    pixelclock = timing->pixelclock;
    dessc_m = (pixelclock / 10 * 1000 * 4) / (25 * 100);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
    lt8911_i2c_reg_write(i2c_bus, 0xaa, dessc_m);
}

static void lt8911_init(struct aic_panel *panel)
{
    int i, val;
    /* init */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x49, 0xff);

    /* Txpll 2.7G */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
    lt8911_i2c_reg_write(i2c_bus, 0x19, 0x31);
    lt8911_i2c_reg_write(i2c_bus, 0x1a, 0x1b);
    lt8911_i2c_reg_write(i2c_bus, 0x1b, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0x1c, 0x00);

    /* txpll Analog */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x82);
    lt8911_i2c_reg_write(i2c_bus, 0x09, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0x02, 0x42);
    lt8911_i2c_reg_write(i2c_bus, 0x03, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0x03, 0x01);
    lt8911_i2c_reg_write(i2c_bus, 0x0a, 0x1b);
    lt8911_i2c_reg_write(i2c_bus, 0x04, 0x2a);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
    lt8911_i2c_reg_write(i2c_bus, 0x0c, 0x10);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x09, 0xfc);
    lt8911_i2c_reg_write(i2c_bus, 0x09, 0xfd);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
    lt8911_i2c_reg_write(i2c_bus, 0x0c, 0x11);

    /* ssc */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
    lt8911_i2c_reg_write(i2c_bus, 0x13, 0x83);
    lt8911_i2c_reg_write(i2c_bus, 0x14, 0x41);
    lt8911_i2c_reg_write(i2c_bus, 0x16, 0x0a);
    lt8911_i2c_reg_write(i2c_bus, 0x18, 0x0a);
    lt8911_i2c_reg_write(i2c_bus, 0x19, 0x33);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);

    /* Check Tx PLL */
    for (i = 0; i < 5; i++) {
        aic_delay_ms(10);
        val = lt8911_i2c_reg_read(i2c_bus, 0x37);
        if (val & 0x02) {
            LT8911_DEV_DEBUG("LT8911 tx pll locked\n");
            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
            lt8911_i2c_reg_write(i2c_bus, 0x1a, 0x36);
            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x82);
            lt8911_i2c_reg_write(i2c_bus, 0x0a, 0x36);
            lt8911_i2c_reg_write(i2c_bus, 0x04, 0x3a);
            break;
        } else {
            LT8911_DEV_DEBUG("LT8911 tx pll unlocked\n");
            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
            lt8911_i2c_reg_write(i2c_bus, 0x09, 0xfc);
            lt8911_i2c_reg_write(i2c_bus, 0x09, 0xfd);

            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
            lt8911_i2c_reg_write(i2c_bus, 0x0c, 0x10);
            lt8911_i2c_reg_write(i2c_bus, 0x0c, 0x11);
        }
    }

    /* LVDS Rx analog */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x82);
    lt8911_i2c_reg_write(i2c_bus, 0x3e, 0xa1);

    /* digital top */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
    lt8911_i2c_reg_write(i2c_bus, 0xb0, 0x00);

    /* lvds digital */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
    lt8911_i2c_reg_write(i2c_bus, 0xc3, 0x20);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x05, 0xae);
    lt8911_i2c_reg_write(i2c_bus, 0x05, 0xfe);

    /* AUX reset */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x07, 0xfe);
    lt8911_i2c_reg_write(i2c_bus, 0x07, 0xff);
    lt8911_i2c_reg_write(i2c_bus, 0x0a, 0xfc);
    lt8911_i2c_reg_write(i2c_bus, 0x0a, 0xfe);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xd8);

    lt8911_i2c_reg_write(i2c_bus, 0x10, LT8911_LVDS_PORT);
    lt8911_i2c_reg_write(i2c_bus, 0x11, LT8911_LVDS_RX);
    lt8911_i2c_reg_write(i2c_bus, 0x1f, LT8911_LVDS_MODE);

     /* eDP out swap config: */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xac);
    lt8911_i2c_reg_write(i2c_bus, 0x15, LT8911_EDP_DATA_SEQUENCE);
    lt8911_i2c_reg_write(i2c_bus, 0x16, 0x00);

    /* tx phy */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x82);
    lt8911_i2c_reg_write(i2c_bus, 0x11, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0x13, 0x10);
    lt8911_i2c_reg_write(i2c_bus, 0x14, 0x0c);
    lt8911_i2c_reg_write(i2c_bus, 0x14, 0x08);
    lt8911_i2c_reg_write(i2c_bus, 0x13, 0x20);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x82);
    lt8911_i2c_reg_write(i2c_bus, 0x0e, 0x35);

    lt8911_i2c_reg_write(i2c_bus, 0x12, 0x33);

    /* eDP Tx Digital */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xa8);
    lt8911_i2c_reg_write(i2c_bus, 0x27, 0x10);

    /* _8_bit_eDP_ */
    lt8911_i2c_reg_write(i2c_bus, 0x17, 0x10);
    lt8911_i2c_reg_write(i2c_bus, 0x18, 0x20);

    /* nvid */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xa0);
    lt8911_i2c_reg_write(i2c_bus, 0x00, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0x01, 0x80);

    /* gpio init */
    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xd8);
    lt8911_i2c_reg_write(i2c_bus, 0x16, 0x01);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
    lt8911_i2c_reg_write(i2c_bus, 0x1b, 0x05);
    lt8911_i2c_reg_write(i2c_bus, 0xc0, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0xc1, 0x24);

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x82);
    lt8911_i2c_reg_write(i2c_bus, 0x5a, 0x01);
    lt8911_i2c_reg_write(i2c_bus, 0x5b, 0xc0);

    /* gpio_sync_output */
    lt8911_i2c_reg_write(i2c_bus, 0x60, 0x00);
    lt8911_i2c_reg_write(i2c_bus, 0x61, 0x00);
}

static void lt8911ex_tx_swingpreset(void)
{
    const struct reg_sequence seq[] = {
        { 0xFF, 0x82 },
        { 0x22, 0x82 },
        { 0x23, 0x00 },
        { 0x24, 0x80 },
        { 0x25, 0x00 },
        { 0x26, 0x82 },
        { 0x27, 0x00 },
        { 0x28, 0x80 },
        { 0x29, 0x00 },
    };
    lt8911_i2c_multi_reg_write(i2c_bus, seq, ARRAY_SIZE(seq));
}

static void lt8911_link_train(void)
{
    const struct reg_sequence seq[] = {
        { 0x06, 0xff },
        { 0xff, 0x85 },
        { 0xa1, 0x01 },

        /* Aux setup */
        { 0xff, 0xac },
        { 0x00, 0x60 },
        { 0xff, 0xa6 },
        { 0x2a, 0x00 },
        { 0xff, 0x81 },
        { 0x07, 0xfe },
        { 0x07, 0xff },
        { 0x0a, 0xfc },
        { 0x0a, 0xfe },
        { 0xff, 0xa8 },
        { 0x2d, 0x00 },
        { 0xff, 0xa8 },
        { 0x2d, 0x00 },

        /* link train */
        { 0xff, 0x85 },
        { 0x1a, 0x02 },
        { 0xff, 0xac },
        { 0x00, 0x64 },
        { 0x01, 0x0a },
        { 0x0c, 0x85 },
        { 0x0c, 0xc5 },
    };

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x06, 0xdf);
    aic_delay_ms(10);

    lt8911_i2c_multi_reg_write(i2c_bus, seq, ARRAY_SIZE(seq));
}

static void lt8911_lvds_clock_check(void)
{
#ifdef LT8911_DEBUG
    u32 fm_value;

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
    lt8911_i2c_reg_write(i2c_bus, 0x1d, 0x03);
    aic_delay_ms(200);
    fm_value   = 0;
    fm_value   = lt8911_i2c_reg_read(i2c_bus, 0x4d);
    fm_value   = fm_value * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x4e);
    fm_value   = fm_value * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x4f);
    LT8911_DEV_DEBUG("LVDS portA clock: %d * 10K\n", fm_value);

    lt8911_i2c_reg_write(i2c_bus, 0x1d, 0x04);
    aic_delay_ms(200);
    fm_value   = 0;
    fm_value   = lt8911_i2c_reg_read(i2c_bus, 0x4d);
    fm_value   = fm_value * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x4e);
    fm_value   = fm_value * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x4f);
    LT8911_DEV_DEBUG("LVDS portB clock: %d * 10K\n", fm_value);

    lt8911_i2c_reg_write(i2c_bus, 0x1d, 0x08);
    aic_delay_ms(200);
    fm_value   = 0;
    fm_value   = lt8911_i2c_reg_read(i2c_bus, 0x4d);
    fm_value   = fm_value * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x4e);
    fm_value   = fm_value * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x4f);

    LT8911_DEV_DEBUG("LVDS pixel clock: %d * 10K\n", fm_value / 10);
#endif
}

/* LVDS should be ready before configuring below video check setting */
static bool lt8911_lvds_signal_check(struct aic_panel *panel)
{
#ifdef LT8911_DEBUG
    u16 h_fp, h_sync, h_bp, h_total;
    u16 v_fp, v_sync, v_bp, v_total;
    u8 sync_pol;
#endif
    struct display_timing *timing = panel->timings;
    u16 h_active, v_active;

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
    lt8911_i2c_reg_write(i2c_bus, 0xa1, 0x01);

    v_active = lt8911_i2c_reg_read(i2c_bus, 0x88);
    v_active = v_active * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x89);

    h_active = lt8911_i2c_reg_read(i2c_bus, 0x8a);
    h_active = h_active * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x8b);

#ifdef LT8911_DEBUG
    sync_pol = lt8911_i2c_reg_read(i2c_bus, 0x7a);
    v_sync   = lt8911_i2c_reg_read(i2c_bus, 0x7b);

    h_sync   = lt8911_i2c_reg_read(i2c_bus, 0x7c);
    h_sync   = h_sync * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x7d);

    v_bp     = lt8911_i2c_reg_read(i2c_bus, 0x7e);
    v_fp     = lt8911_i2c_reg_read(i2c_bus, 0x7f);

    h_bp     = lt8911_i2c_reg_read(i2c_bus, 0x80);
    h_bp     = h_bp * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x81);

    h_fp     = lt8911_i2c_reg_read(i2c_bus, 0x82);
    h_fp     = h_fp * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x83);

    v_total  = lt8911_i2c_reg_read(i2c_bus, 0x84);
    v_total  = v_total * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x85);

    h_total  = lt8911_i2c_reg_read(i2c_bus, 0x86);
    h_total  = h_total * 0x100 + lt8911_i2c_reg_read(i2c_bus, 0x87);

    LT8911_DEV_DEBUG("sync_pol = %x\n", sync_pol);

    LT8911_DEV_DEBUG("hfp, hs, hbp, hact, htotal = %d, %d, %d, %d, %d\n",
            h_fp, h_sync, h_bp , h_active, h_total);

    LT8911_DEV_DEBUG("vfp, vs, vbp, vact, vtotal = %d, %d, %d, %d, %d\n",
            v_fp , v_sync, v_bp, v_active ,v_total);
#endif

    return (h_active != timing->hactive) || (v_active != timing->vactive);
}

static void lt8911ex_link_train_check(void)
{
    u8 val, i;

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0xac);
    for (i = 0; i < 5; i++)
    {
        val = lt8911_i2c_reg_read(i2c_bus, 0x82);
        if (val & 0x20) {
            if ((val & 0x1f) == 0x1e) {
                LT8911_DEV_DEBUG("eDP link train successed: %d\n", val);
                return;
            } else {
                LT8911_DEV_DEBUG("eDP link train failed: %d\n", val);

                lt8911_i2c_reg_write(i2c_bus, 0xff, 0xac);
                lt8911_i2c_reg_write(i2c_bus, 0x00, 0x00);
                lt8911_i2c_reg_write(i2c_bus, 0x01, 0x0a);
                lt8911_i2c_reg_write(i2c_bus, 0x14, 0x80);
                lt8911_i2c_reg_write(i2c_bus, 0x14, 0x81);
                aic_delay_ms(50);
                lt8911_i2c_reg_write(i2c_bus, 0x14, 0x84);
                aic_delay_ms(50);
                lt8911_i2c_reg_write(i2c_bus, 0x14, 0xc0);
            }

            val = lt8911_i2c_reg_read(i2c_bus, 0x83);
            LT8911_DEV_DEBUG("panel link rate: %d", val);

            val = lt8911_i2c_reg_read(i2c_bus, 0x84);
            LT8911_DEV_DEBUG("panel link count: %d", val);

            aic_delay_ms(100);
        } else {
            aic_delay_ms(100);
        }
    }
}


static void lt8911_video_on(struct aic_panel *panel)
{
    bool lvds_signal_off, txpll_unlock = true;
    int i = 0;

    lt8911_check_id();
    lt8911_edp_video_cfg(panel);
    lt8911_init(panel);

    do {
        /* register bank */
        lt8911_i2c_reg_write(i2c_bus, 0xff, 0x85);
        /* clock stable */
        if ((lt8911_i2c_reg_read(i2c_bus, 0x12) & 0xc0) == 0x40) {
            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
            /* pll unlocked */
            if ((lt8911_i2c_reg_read(i2c_bus, 0x37) & 0x04) == 0x00) {
                /* rxpll reset */
                lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
                lt8911_i2c_reg_write(i2c_bus, 0x04, 0xfd);

                aic_delay_ms(10);
                lt8911_i2c_reg_write(i2c_bus, 0x04, 0xff);

                LT8911_DEV_DEBUG("lvds rxpll unlocked, reset rxpll\n");
            } else {
                LT8911_DEV_DEBUG("lvds rxpll locked\n");
            }

            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x87);
            /* txpll unlocked */
            if ((lt8911_i2c_reg_read(i2c_bus, 0x37) & 0x02) == 0x00) {
                lt8911_check_id();
                lt8911_edp_video_cfg(panel);
                lt8911_init(panel);

                txpll_unlock = true;
                LT8911_DEV_DEBUG("lvds txpll unlocked, reset 8911EX\n");
            } else {
                txpll_unlock = false;
                LT8911_DEV_DEBUG("TxPLL locked\n");
            }
        }

        aic_delay_ms(200);
        lt8911_lvds_clock_check();

        lvds_signal_off = lt8911_lvds_signal_check(panel);
        if (lvds_signal_off) {
            lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
            lt8911_i2c_reg_write(i2c_bus, 0x05, 0xae);

            aic_delay_ms(10);
            lt8911_i2c_reg_write(i2c_bus, 0x05, 0xfe);

            LT8911_DEV_DEBUG("No LVDS Signal Input\n");
        }

        i++;
    } while ((txpll_unlock || lvds_signal_off) && (i < 5));

    if (i < 5)
        LT8911_DEV_DEBUG("LVDS Signal is OK\n");
    else
        return;

    lt8911_i2c_reg_write(i2c_bus, 0xff, 0x81);
    lt8911_i2c_reg_write(i2c_bus, 0x06, 0xdf);

    aic_delay_ms(10);
    lt8911_i2c_reg_write(i2c_bus, 0x06, 0xff);

    lt8911ex_tx_swingpreset();

    lt8911_link_train();

    lt8911ex_link_train_check();
}

static void lt8911_hard_reset(void)
{
    panel_get_gpio(&lt8911_reset, LT8911_RESET);
    panel_gpio_set_value(&lt8911_reset, 0);
    aic_delay_ms(120);
    panel_gpio_set_value(&lt8911_reset, 1);
}

static void lt8911_video_setup(struct aic_panel *panel)
{
    i2c_bus = rt_i2c_bus_device_find(LT8911_I2C_NAME);
    if (i2c_bus == RT_NULL)
    {
        pr_err("can't find %s device\n", LT8911_I2C_NAME);
        return;
    }

    lt8911_video_on(panel);
}

static int panel_enable(struct aic_panel *panel)
{
    lt8911_hard_reset();

    panel_di_enable(panel, 0);
    panel_de_timing_enable(panel, 0);

    lt8911_video_setup(panel);

    panel_get_gpio(&lt8911_blen, LT8911_BLEN);
    panel_gpio_set_value(&lt8911_blen, 1);
    panel_backlight_enable(panel, 0);
    return 0;
}

static int panel_disable(struct aic_panel *panel)
{
    panel_backlight_disable(panel, 0);
    panel_di_disable(panel, 0);
    panel_de_timing_disable(panel, 0);

    panel_gpio_set_value(&lt8911_reset, 0);
    return 0;
}

static struct aic_panel_funcs simple_funcs = {
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .disable = panel_disable,
    .unprepare = panel_default_unprepare,
    .register_callback = panel_register_callback,
};

static struct display_timing panel_timing = {
    .pixelclock   = 148 * 1000000,

    .hactive      = 1920,
    .hback_porch  = 148,
    .hfront_porch = 88,
    .hsync_len    = 44,

    .vactive      = 1080,
    .vback_porch  = 36,
    .vfront_porch = 4,
    .vsync_len    = 5,

    .flags        = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW,
};

static struct panel_lvds lvds = {
    .mode      = NS,
    .link_mode = DUAL_LINK,
};

struct aic_panel bridge_lt8911 = {
    .name = "panel-bridge-lt8911",
    .timings = &panel_timing,
    .funcs = &simple_funcs,
    .lvds = &lvds,
    .connector_type = AIC_LVDS_COM,
};

