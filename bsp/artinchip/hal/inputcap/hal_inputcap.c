/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#include <string.h>
#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_inputcap.h"

#define INPUTCAP_BASE(i)            (CAP_BASE + i * 0x100)

#define CAP_CNT_V                   0x000
#define CAP_CNT_PRDV                0x008
#define CAP_CNT_CMPV                0x00C
#define CAP_CNT_PRDV_SH             0x010
#define CAP_CNT_CMPV_SH             0x014
#define CAP_CONF1                   0x018
#define CAP_CONF2                   0x01C
#define CAP_INT_EN                  0x020
#define CAP_FLG                     0x024
#define CAP_FLG_CLR                 0x028
#define CAP_IN_FLT                  0x030
#define CAP_FIFO0_CTL               0x038
#define CAP_FIFO1_CTL               0x03C
#define CAP_EVNT0_FIFO              0x040
#define CAP_EVNT1_FIFO              0x044
#define CAP_VER                     0x0FC

/* CAP_CONF1 */
#define CAP_EVENT0_POL_MASK         GENMASK(1, 0)
#define CAP_EVENT0_POL_SHIFT        0
#define CAP_EVENT0_EN_MASK          GENMASK(2, 2)
#define CAP_EVENT0_EN_SHIFT         2
#define CAP_EVENT1_POL_MASK         GENMASK(5, 4)
#define CAP_EVENT1_POL_SHIFT        4
#define CAP_EVENT1_EN_MASK          GENMASK(6, 6)
#define CAP_EVENT1_EN_SHIFT         6

/* CAP_CONF2 */
#define CAP_CNT_EN_MASK             GENMASK(4, 4)
#define CAP_CNT_EN_SHIFT            4

/* CAP_FIFO0_CTL */
#define CAP_FIFO0_DMA_EN_MASK       GENMASK(3, 3)
#define CAP_FIFO0_DMA_EN_SHIFT      3
#define CAP_FIFO0_TH_MASK           GENMASK(6, 4)
#define CAP_FIFO0_TH_SHIFT          4
#define CAP_FIFO0_FLUSH_MASK        GENMASK(1, 1)
#define CAP_FIFO0_FLUSH_SHIFT       1
#define CAP_FIFO0_EN_MASK           GENMASK(0, 0)
#define CAP_FIFO0_EN_SHIFT          0

/* CAP_FIFO1_CTL */
#define CAP_FIFO1_DMA_EN_MASK       GENMASK(3, 3)
#define CAP_FIFO1_DMA_EN_SHIFT      3
#define CAP_FIFO1_TH_MASK           GENMASK(6, 4)
#define CAP_FIFO1_TH_SHIFT          4
#define CAP_FIFO1_FLUSH_MASK        GENMASK(1, 1)
#define CAP_FIFO1_FLUSH_SHIFT       1
#define CAP_FIFO1_EN_MASK           GENMASK(0, 0)
#define CAP_FIFO1_EN_SHIFT          0

/* CAP_INT_EN */
#define CAP_EVNT0_INT_EN_MASK       GENMASK(1, 1)
#define CAP_EVNT0_INT_EN_SHIFT      1
#define CAP_EVNT1_INT_EN_MASK       GENMASK(2, 2)
#define CAP_EVNT1_INT_EN_SHIFT      2
#define CAP_FIFO0_INT_EN_MASK       GENMASK(8, 8)
#define CAP_FIFO0_INT_EN_SHIFT      8
#define CAP_FIFO1_INT_EN_MASK       GENMASK(11, 11)
#define CAP_FIFO1_INT_EN_SHIFT      11

#define CAP_FIFO_DEFAULT_TH         8
#define CAP_SRC_MAXBURST            CAP_FIFO_DEFAULT_TH
#define CAP_DST_MAXBURST            16

u32 hal_inputcap_int_stat(u32 i)
{
    return readl((INPUTCAP_BASE(i) + CAP_FLG));
}

void hal_inputcap_int_clr(u32 i, u32 mask)
{
    writel(mask, (INPUTCAP_BASE(i) + CAP_FLG_CLR));
}

static inline void inputcap_reg_config(u32 i, u32 offset, u32 mask, u32 shift, u32 val)
{
    u32 cur;
    cur = readl((INPUTCAP_BASE(i) + offset));
    setbits(val, mask, shift, cur);
    writel(cur, (INPUTCAP_BASE(i) + offset));
}

void hal_inputcap_evnt_en(u32 i, u32 event, u32 enable)
{
    if (event == 0)
        inputcap_reg_config(i, CAP_CONF1, CAP_EVENT0_EN_MASK, CAP_EVENT0_EN_SHIFT, enable);
    else
        inputcap_reg_config(i, CAP_CONF1, CAP_EVENT1_EN_MASK, CAP_EVENT1_EN_SHIFT, enable);

}

void hal_inputcap_pol_set(u32 i, u32 event, enum inputcap_polarity event_pol)
{
    if (event == 0)
        inputcap_reg_config(i, CAP_CONF1, CAP_EVENT0_POL_MASK, CAP_EVENT0_POL_SHIFT, (u32)event_pol);
    else
        inputcap_reg_config(i, CAP_CONF1, CAP_EVENT1_POL_MASK, CAP_EVENT1_POL_SHIFT, (u32)event_pol);
}

void hal_inputcap_cnt_en(u32 i, u32 enable)
{
    inputcap_reg_config(i, CAP_CONF2, CAP_CNT_EN_MASK, CAP_CNT_EN_SHIFT, enable);
}

u32 hal_inputcap_get_cnt(u32 i)
{
    return readl((INPUTCAP_BASE(i) + CAP_CNT_V));
}

u32 hal_inputcap_get_event_fifo(u32 i, u32 fifo)
{
    if (fifo == 0)
        return readl((INPUTCAP_BASE(i) + CAP_EVNT0_FIFO));
    else
        return readl((INPUTCAP_BASE(i) + CAP_EVNT1_FIFO));

    return 0;
}

void hal_inputcap_fifo_flush(u32 i, u32 fifo)
{
    if (fifo == 0)
        inputcap_reg_config(i, CAP_FIFO0_CTL, CAP_FIFO0_FLUSH_MASK, CAP_FIFO0_FLUSH_SHIFT, 1);
    else
        inputcap_reg_config(i, CAP_FIFO1_CTL, CAP_FIFO1_FLUSH_MASK, CAP_FIFO1_FLUSH_SHIFT, 1);
}

void hal_inputcap_set_fifo(u32 i, u32 fifo, u32 enable)
{
    u32 th = CAP_FIFO_DEFAULT_TH;

    if (fifo == 0) {
        inputcap_reg_config(i, CAP_FIFO0_CTL, CAP_FIFO0_TH_MASK, CAP_FIFO0_TH_SHIFT, th);
        inputcap_reg_config(i, CAP_FIFO0_CTL, CAP_FIFO0_EN_MASK, CAP_FIFO0_EN_SHIFT, enable);
#ifndef AIC_DMA_DRV
        inputcap_reg_config(i, CAP_INT_EN, CAP_FIFO0_INT_EN_MASK, CAP_FIFO0_INT_EN_SHIFT, enable);
#else
        inputcap_reg_config(i, CAP_FIFO0_CTL, CAP_FIFO0_DMA_EN_MASK, CAP_FIFO0_DMA_EN_SHIFT, enable);
#endif
    } else {
        inputcap_reg_config(i, CAP_FIFO1_CTL, CAP_FIFO1_TH_MASK, CAP_FIFO1_TH_SHIFT, th);
        inputcap_reg_config(i, CAP_FIFO1_CTL, CAP_FIFO1_EN_MASK, CAP_FIFO1_EN_SHIFT, enable);
#ifndef AIC_DMA_DRV
        inputcap_reg_config(i, CAP_INT_EN, CAP_FIFO1_INT_EN_MASK, CAP_FIFO1_INT_EN_SHIFT, enable);
#else
        inputcap_reg_config(i, CAP_FIFO1_CTL, CAP_FIFO1_DMA_EN_MASK, CAP_FIFO1_DMA_EN_SHIFT, enable);
#endif
    }
}

#ifdef AIC_DMA_DRV
void hal_inputcap_dma_config(struct aic_inputcap_pdata *chan, u32 event,
                                        dma_async_callback callback, void *callback_param)
{
    struct dma_slave_config config;
    struct aic_inputcap_transfer_info *info;

    config.direction = DMA_DEV_TO_MEM;
    config.src_maxburst = CAP_SRC_MAXBURST;
    config.dst_maxburst = CAP_DST_MAXBURST;
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;

    if (event == 0) {
        config.src_addr = INPUTCAP_BASE(chan->id) + CAP_EVNT0_FIFO;
        config.slave_id = DMA_ID_CAP00 + chan->id * 2;
        info = &chan->t_info0;
    } else {
        config.src_addr = INPUTCAP_BASE(chan->id) + CAP_EVNT1_FIFO;
        config.slave_id = DMA_ID_CAP01 + chan->id * 2;
        info = &chan->t_info1;
    }

    info->dma_chan = hal_request_dma_chan();
    if (!info->dma_chan) {
        hal_log_err("Inputcap request dma channel error\n");
        return;
    }

    hal_dma_chan_config(info->dma_chan, &config);
    hal_dma_chan_register_cb(info->dma_chan, callback, callback_param);
    hal_dma_chan_prep_device(info->dma_chan, (ulong)info->buf_info.buf, config.src_addr,
                    info->buf_info.buf_len, DMA_DEV_TO_MEM);

    hal_dma_chan_start(info->dma_chan);

    return;
}
#endif

