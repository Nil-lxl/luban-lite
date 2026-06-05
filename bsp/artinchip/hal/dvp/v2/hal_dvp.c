/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip <ArtInChip@artinchip.com>
 */

#include "aic_core.h"
#include "mpp_vin.h"

#include "hal_dvp.h"

static inline void dvp_writel(u32 val, int reg)
{
    writel(val, DVP_BASE + reg);
}

static inline u32 dvp_readl(int reg)
{
    return readl(DVP_BASE + reg);
}

static void hal_dvp_reg_enable(int offset, int bit, int enable)
{
    int tmp;

    tmp = dvp_readl(offset);
    tmp &= ~bit;
    if (enable)
        tmp |= bit;

    dvp_writel(tmp, offset);
}

void hal_dvp_enable(struct aic_dvp_config *cfg, int enable)
{
    hal_dvp_reg_enable(DVP_CTL, DVP_CTL_EN, enable);
}

void hal_dvp_demux_en(int enable)
{
    hal_dvp_reg_enable(DVP_CTL, DVP_CTL_DEMUX_EN, enable);
}

void hal_dvp_clr_mode(void)
{
    dvp_writel(DVP_CTL_CLR_MODE, DVP_CTL);
}

void hal_dvp_hist_en(void)
{
    dvp_writel(DVP_CTL_HIST_EN, DVP_CTL);
}

void hal_dvp_ch_id_cfg(u32 id0, u32 id1)
{
    u32 val = dvp_readl(DVP_CH_REF_ID);

    val &= ~(DVP_CH_REF_ID0_MASK | DVP_CH_REF_ID1_MASK);
    val |= DVP_CH_REF_ID0_CFG(id0) | DVP_CH_REF_ID1_CFG(id1);
    val |= DVP_CH_REF_ID_EN;

    dvp_writel(val, DVP_CH_REF_ID);
}

u32 hal_dvp_ch_irq_sta_get(void)
{
    u32 val = dvp_readl(DVP_CH_IRQ_STA);

    if (val)
        dvp_writel(val, DVP_CH_IRQ_STA);
    return val;
}

u32 hal_dvp_irq_sta_get(u32 ch)
{
    return dvp_readl(DVP_IRQ_STA(ch));
}

void hal_dvp_capture_start(u32 ch)
{
    dvp_writel(DVP_OUT_CTL_CAP_ON, DVP_OUT_CTL(ch));
}

void hal_dvp_capture_stop(u32 ch)
{
    dvp_writel(0, DVP_OUT_CTL(ch));
}

void hal_dvp_clr_fifo(void)
{
    hal_dvp_reg_enable(DVP_CTL, DVP_CTL_CLR, 1);
}

int hal_dvp_clr_int(u32 ch)
{
    int sta = dvp_readl(DVP_IRQ_STA(ch));

    if (sta)
        dvp_writel(sta, DVP_IRQ_STA(ch));
    return sta;
}

bool hal_dvp_int_is_enabled(u32 ch)
{
    if (dvp_readl(DVP_IRQ_EN(ch)) & (DVP_IRQ_EN_FRAME_DONE | DVP_IRQ_EN_HNUM))
        return true;
    else
        return false;
}

void hal_dvp_enable_int(struct aic_dvp_config *cfg, u32 ch, int enable)
{
    int hnum = cfg->height / 4;

    if (cfg->interlaced)
        hnum = hnum / 2;

    /* When HNUM IRQ happened, so we can update the address */
    if (enable)
        dvp_writel(hnum << DVP_IRQ_CFG_HNUM_SHIFT, DVP_IRQ_CFG(ch));

    hal_dvp_reg_enable(DVP_IRQ_EN(ch),
                       DVP_IRQ_EN_FRAME_DONE | DVP_IRQ_EN_HNUM, enable);
}

void hal_dvp_set_pol(u32 flags, u32 ch)
{
    u32 href_pol, pclk_pol, vref_pol, field_pol;

    field_pol = flags & MEDIA_SIGNAL_FIELD_ACTIVE_HIGH ?
                0 : DVP_IN_CFG_FILED_POL_ACTIVE_LOW;
    vref_pol = flags & MEDIA_SIGNAL_VSYNC_ACTIVE_LOW ?
                DVP_IN_CFG_VSYNC_POL_FALLING : 0;
    href_pol = flags & MEDIA_SIGNAL_HSYNC_ACTIVE_HIGH ?
                DVP_IN_CFG_HREF_POL_ACTIVE_HIGH : 0;
    pclk_pol = flags & MEDIA_SIGNAL_PCLK_SAMPLE_FALLING ?
                DVP_IN_CFG_PCLK_POL_FALLING : 0;
    dvp_writel(href_pol | vref_pol | pclk_pol | field_pol, DVP_IN_CFG(ch));
}

void hal_dvp_set_cfg(struct aic_dvp_config *cfg, u32 ch)
{
    u32 height = 0, stride0 = 0, stride1 = 0, val = 0;

    if (cfg->stride[0] == 0) {
        hal_log_err("Invalid stride0: 0x%x\n", cfg->stride[0]);
        return;
    }

    if ((cfg->output != DVP_OUT_RAW_PASSTHROUGH) &&
        (cfg->output != DVP_OUT_Y_ONLY) && (cfg->stride[1] == 0)) {
        hal_log_err("Invalid stride1: 0x%x\n", cfg->stride[1]);
        return;
    }

    if (cfg->interlaced) {
        height  = cfg->height / 2;
#ifdef DVP_SFIELD_MODE
        stride0 = cfg->stride[0];
        stride1 = cfg->stride[1];
#else
        stride0 = cfg->stride[0] * 2;
        stride1 = cfg->stride[1] * 2;
#endif
    } else {
        height  = cfg->height;
        stride0 = cfg->stride[0];
        stride1 = cfg->stride[1];
    }

    if (cfg->stitch_mode == MPP_STITCH_H_MODE) {
        stride0 *= 2;
        stride1 *= 2;
    }

    val = DVP_CTL_IN_FMT(cfg->input)
            | DVP_CTL_IN_SEQ(cfg->input_seq)
            | DVP_CTL_OUT_FMT(cfg->output)
            | DVP_CTL_VIDEO_CONTINUE_EN
            | DVP_CTL_EN;
    if (!cfg->interlaced)
        val |= DVP_CTL_DROP_FRAME_EN;
    if (cfg->mux > 1) {
        val |= DVP_CTL_DEMUX_EN;
        hal_dvp_ch_id_cfg(DVP_DEMUX_CH0_ID, DVP_DEMUX_CH1_ID);
    }
    dvp_writel(val, DVP_CTL);

    if (cfg->input == DVP_IN_RAW)
        val = DVP_OUT_HOR_NUM_RAW(cfg->width) | DVP_OUT_HOR_BEG_RAW(cfg->crop_x);
    else
        val = DVP_OUT_HOR_NUM(cfg->width) | DVP_OUT_HOR_BEG(cfg->crop_x);
    dvp_writel(val, DVP_OUT_HOR_SIZE(ch));

    dvp_writel(DVP_OUT_VER_NUM(height) | DVP_OUT_VER_BEG(cfg->crop_y), DVP_OUT_VER_SIZE(ch));

    dvp_writel(stride0, DVP_OUT_LINE_STRIDE0(ch));
    dvp_writel(stride1, DVP_OUT_LINE_STRIDE1(ch));
}

void hal_dvp_update_buf_addr(dma_addr_t y, dma_addr_t uv, u32 ch,
                u32 y_offset, u32 uv_offset)
{
    if (y == 0) {
        hal_log_err("Invalid DMA address: Y 0x%x, UV 0x%x\n", (u32)y, (u32)uv);
        return;
    }
    dvp_writel(y + y_offset, DVP_OUT_ADDR_BUF(ch, 0));
    dvp_writel(uv + uv_offset, DVP_OUT_ADDR_BUF(ch, 1));
}

void hal_dvp_update_ctl(u32 ch)
{
    dvp_writel(1, DVP_OUT_UPDATE_CTL(ch));
}

void hal_dvp_record_mode(u32 ch)
{
    dvp_writel(0x80000003, DVP_OUT_FRA_NUM(ch));
}

void hal_dvp_set_frame_offset(u32 num, u32 ch)
{
    u32 val = 0;

    if (!num || num > 0xFF) {
        pr_warn("Invalid frame offset %d\n", num);
        return;
    }

    val = dvp_readl(DVP_OUT_FRA_NUM(ch));
    setbits(num, DVP_OUT_FRA_NUM_MASK, 0, val);
    dvp_writel(val, DVP_OUT_FRA_NUM(ch));
}

void hal_dvp_qos_cfg(u32 high, u32 low, u32 inc_thd, u32 dec_thd)
{
    u32 val = DVP_QOS_CUSTOM;

    val |= (inc_thd << DVP_QOS_INC_THR_SHIFT) & DVP_QOS_INC_THR_MASK;
    val |= (dec_thd << DVP_QOS_DEC_THR_SHIFT) & DVP_QOS_DEC_THR_MASK;
    val |= (high << DVP_QOS_HIGH_SHIFT) & DVP_QOS_HIGH_MASK;
    val |= low & DVP_QOS_LOW_MASK;
    hal_log_info("DVP QoS is enable: 0x%x\n", val);
    dvp_writel(val, DVP_QOS_CFG);
}

static bool g_top_field[DVP_MAX_CH_NUM];

u32 hal_dvp_get_current_xy(u32 ch)
{
    u32 val = dvp_readl(DVP_IN_HOR_SIZE(ch));

    g_top_field[ch] = val & DVP_IN_HOR_SIZE_XY_CODE_F ? 0 : 1;
    hal_log_debug("XY: %#x, is %s\n",
                  (u32)(val & DVP_IN_HOR_SIZE_XY_CODE_MASK),
                  g_top_field ? "Top" : "Bottom");

    return g_top_field[ch];
}

u32 hal_dvp_is_top_field(u32 ch)
{
    return g_top_field[ch];
}

u32 hal_dvp_is_bottom_field(u32 ch)
{
    return !g_top_field[ch];
}

void hal_dvp_field_tag_clr(u32 ch)
{
    memset(g_top_field, 0, sizeof(g_top_field));
}
