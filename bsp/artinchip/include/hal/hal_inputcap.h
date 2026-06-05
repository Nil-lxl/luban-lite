/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_INPUTCAP_H_
#define _ARTINCHIP_HAL_INPUTCAP_H_

#include "aic_common.h"
#include "hal_dma.h"
#include "aic_dma_id.h"

#ifdef FPGA_BOARD_ARTINCHIP
#define INPUTCAP_CLK_RATE          48000000 /* 48 MHz */
#else
#define INPUTCAP_CLK_RATE          200000000 /* 200 MHz */
#endif

/* CAP_FLG */
#define CAP_INT_FLG                BIT(0)
#define CAP_EVNT0_FLG              BIT(1)
#define CAP_EVNT1_FLG              BIT(2)
#define CAP_CNT_OVFL_FLG           BIT(5)
#define CAP_CNT_PRD_FLG            BIT(6)
#define CAP_CNT_CMP_FLG            BIT(7)
#define CAP_FIFO0_FLG              BIT(8)
#define CAP_FIFO0_OVFL_FLG         BIT(9)
#define CAP_FIFO0_UDFL_FLG         BIT(10)
#define CAP_FIFO1_FLG              BIT(11)
#define CAP_FIFO1_OVFL_FLG         BIT(12)
#define CAP_FIFO1_UDFL_FLG         BIT(13)

enum inputcap_polarity {
    INPUTCAP_RISING_EDGE,
    INPUTCAP_FALLING_EDGE,
    INPUTCAP_RISING_FALLING_EDGE,
};

struct aic_inputcap_buf_info
{
    uint32_t *buf;
    uint32_t buf_len;
};

struct aic_inputcap_transfer_info
{
    struct aic_dma_chan *dma_chan;
    struct aic_inputcap_buf_info buf_info;
};

struct aic_inputcap_pdata {
    u8 id;
    u8 isr_event;
    enum inputcap_polarity event0_pol;
    enum inputcap_polarity event1_pol;
    u32 cur_cnt;
    u32 last_cnt;
    struct aic_inputcap_transfer_info t_info0;
    struct aic_inputcap_transfer_info t_info1;
};

void hal_inputcap_evnt_en(u32 i, u32 event, u32 enable);
void hal_inputcap_pol_set(u32 i, u32 event, enum inputcap_polarity event_pol);
void hal_inputcap_cnt_en(u32 i, u32 enable);
u32 hal_inputcap_get_cnt(u32 i);
void hal_inputcap_set_fifo(u32 i, u32 fifo, u32 enable);
u32 hal_inputcap_int_stat(u32 i);
void hal_inputcap_int_clr(u32 i, u32 mask);
u32 hal_inputcap_get_event_fifo(u32 i, u32 fifo);
void hal_inputcap_fifo_flush(u32 i, u32 fifo);
#ifdef AIC_DMA_DRV
void hal_inputcap_dma_config(struct aic_inputcap_pdata *chan, u32 event,
                                        dma_async_callback callback, void *callback_param);
#endif

#endif // end of _ARTINCHIP_HAL_INPUTCAP_H_
