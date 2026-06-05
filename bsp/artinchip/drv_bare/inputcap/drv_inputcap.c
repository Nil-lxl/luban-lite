/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#define LOG_TAG         "INPUTCAP"
#include "aic_core.h"
#include "aic_hal_clk.h"
#include "hal_inputcap.h"
#include "inputcap.h"

#ifndef AIC_DMA_DRV
static u32 g_event0_pluse[AIC_INPUTCAP_CH_NUM][INPUTCAP_WATER_MARK] = {0};
static u32 g_event1_pluse[AIC_INPUTCAP_CH_NUM][INPUTCAP_WATER_MARK] = {0};
#endif

static struct inputcap_cb g_inputcap_cb[AIC_INPUTCAP_CH_NUM] = {0};

static struct aic_inputcap_pdata g_inputcap_pdata[] = {
#ifdef AIC_USING_INPUTCAP0
    {
        .id = 0,
#ifdef AIC_INPUTCAP0_EVENT0
        .event0_pol = AIC_INPUTCAP0_EVENT0,
#endif
#ifdef AIC_INPUTCAP0_EVENT1
        .event1_pol = AIC_INPUTCAP0_EVENT1,
#endif
    },
#endif
#ifdef AIC_USING_INPUTCAP1
    {
        .id = 1,
#ifdef AIC_INPUTCAP1_EVENT0
        .event0_pol = AIC_INPUTCAP1_EVENT0,
#endif
#ifdef AIC_INPUTCAP1_EVENT1
        .event1_pol = AIC_INPUTCAP1_EVENT1,
#endif
    },
#endif
#ifdef AIC_USING_INPUTCAP2
    {
        .id = 2,
#ifdef AIC_INPUTCAP2_EVENT0
        .event0_pol = AIC_INPUTCAP2_EVENT0,
#endif
#ifdef AIC_INPUTCAP2_EVENT1
        .event1_pol = AIC_INPUTCAP2_EVENT1,
#endif
    },
#endif
#ifdef AIC_USING_INPUTCAP3
    {
        .id = 3,
#ifdef AIC_INPUTCAP3_EVENT0
        .event0_pol = AIC_INPUTCAP3_EVENT0,
#endif
#ifdef AIC_INPUTCAP3_EVENT1
        .event1_pol = AIC_INPUTCAP3_EVENT1,
#endif
    },
#endif
#ifdef AIC_USING_INPUTCAP4
    {
        .id = 4,
#ifdef AIC_INPUTCAP4_EVENT0
        .event0_pol = AIC_INPUTCAP4_EVENT0,
#endif
#ifdef AIC_INPUTCAP4_EVENT1
        .event1_pol = AIC_INPUTCAP4_EVENT1,
#endif
    },
#endif
};

static struct aic_inputcap_pdata *_get_inputcap_priv(u32 ch)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(g_inputcap_pdata); i++) {
        if (g_inputcap_pdata[i].id == ch)
            return &g_inputcap_pdata[i];
    }

    return NULL;
}

#ifdef AIC_DMA_DRV
static void inputcap_fifo0_dma_callback(void *arg)
{
    struct aic_inputcap_pdata *aic_capture = (struct aic_inputcap_pdata *)arg;

    if (g_inputcap_cb[aic_capture->id].func_event0 != NULL)
        g_inputcap_cb[aic_capture->id].func_event0((void *)(&aic_capture->id));

}
static void inputcap_fifo1_dma_callback(void *arg)
{
    struct aic_inputcap_pdata *aic_capture = (struct aic_inputcap_pdata *)arg;

    if (g_inputcap_cb[aic_capture->id].func_event1 != NULL)
        g_inputcap_cb[aic_capture->id].func_event1((void *)(&aic_capture->id));
}
#endif

#ifdef AIC_DMA_DRV
int aic_inputcap_setbuf(u32 ch, struct inputcap_fifo_buf *buf)
{
    struct aic_inputcap_pdata *aic_capture = _get_inputcap_priv(ch);

    if (aic_capture == NULL) {
        pr_err("invalid ch: %d\n", ch);
        return -EINVAL;
    }

    aic_capture->t_info0.buf_info.buf = buf->event0_buf;
    aic_capture->t_info0.buf_info.buf_len = buf->event0_buflen;
    aic_capture->t_info1.buf_info.buf = buf->event1_buf;
    aic_capture->t_info1.buf_info.buf_len = buf->event1_buflen;

    return EOK;
}
#endif

int aic_inputcap_open(u32 ch, struct inputcap_cb cb)
{
    struct aic_inputcap_pdata *aic_capture = _get_inputcap_priv(ch);

    if (aic_capture == NULL) {
        pr_err("invalid ch: %d\n", ch);
        return -EINVAL;
    }

    if (cb.func_event0 != NULL)
        g_inputcap_cb[aic_capture->id].func_event0 = cb.func_event0;

    if (cb.func_event1 != NULL)
        g_inputcap_cb[aic_capture->id].func_event1 = cb.func_event1;

    if (aic_capture->event0_pol != 0) {
#ifndef AIC_DMA_DRV
        memset(g_event0_pluse[aic_capture->id], 0, INPUTCAP_WATER_MARK * sizeof(u32));
#endif
        hal_inputcap_pol_set(aic_capture->id, 0, (aic_capture->event0_pol - 1));
        hal_inputcap_set_fifo(aic_capture->id, 0, 1);
        hal_inputcap_evnt_en(aic_capture->id, 0, 1);
#ifdef AIC_DMA_DRV
        hal_inputcap_dma_config(aic_capture, 0, inputcap_fifo0_dma_callback, (void *)aic_capture);
#endif
    }

    if (aic_capture->event1_pol != 0) {
#ifndef AIC_DMA_DRV
        memset(g_event1_pluse[aic_capture->id], 0, INPUTCAP_WATER_MARK * sizeof(u32));
#endif
        hal_inputcap_pol_set(aic_capture->id, 1, (aic_capture->event1_pol - 1));
        hal_inputcap_set_fifo(aic_capture->id, 1, 1);
        hal_inputcap_evnt_en(aic_capture->id, 1, 1);
#ifdef AIC_DMA_DRV
        hal_inputcap_dma_config(aic_capture, 1, inputcap_fifo1_dma_callback, (void *)aic_capture);
#endif
    }
    hal_inputcap_cnt_en(aic_capture->id, 1);

    return EOK;
}

int aic_inputcap_close(u32 ch)
{
    struct aic_inputcap_pdata *aic_capture = _get_inputcap_priv(ch);

    if (aic_capture == NULL) {
        pr_err("invalid ch: %d\n", ch);
        return -EINVAL;
    }

    if (aic_capture->event0_pol != 0) {
        hal_inputcap_set_fifo(aic_capture->id, 0, 0);
        hal_inputcap_evnt_en(aic_capture->id, 0, 0);
#ifdef AIC_DMA_DRV
        hal_inputcap_fifo_flush(aic_capture->id, 0);
        hal_dma_chan_stop(aic_capture->t_info0.dma_chan);
        hal_release_dma_chan(aic_capture->t_info0.dma_chan);
#endif
    }
    if (aic_capture->event1_pol != 0) {
        hal_inputcap_set_fifo(aic_capture->id, 1, 0);
        hal_inputcap_evnt_en(aic_capture->id, 1, 0);
#ifdef AIC_DMA_DRV
        hal_inputcap_fifo_flush(aic_capture->id, 1);
        hal_dma_chan_stop(aic_capture->t_info1.dma_chan);
        hal_release_dma_chan(aic_capture->t_info1.dma_chan);
#endif
    }
    hal_inputcap_cnt_en(aic_capture->id, 0);

    return EOK;
}

#ifndef AIC_DMA_DRV
static u32 aic_inputcap_get_pulsewidth(u32 ch)
{
    struct aic_inputcap_pdata *aic_capture = _get_inputcap_priv(ch);

    if (aic_capture == NULL) {
        pr_err("invalid ch: %d\n", ch);
        return 0;
    }

    u32 temp_cnt;
    u32 pulsewidth_us = 0;

    if (aic_capture->isr_event == 0) {
        aic_capture->last_cnt = hal_inputcap_get_event_fifo(aic_capture->id, 0);
        aic_capture->cur_cnt = hal_inputcap_get_event_fifo(aic_capture->id, 0);
    } else {
        aic_capture->last_cnt = hal_inputcap_get_event_fifo(aic_capture->id, 1);
        aic_capture->cur_cnt = hal_inputcap_get_event_fifo(aic_capture->id, 1);
    }

    if (aic_capture->cur_cnt > aic_capture->last_cnt)
        temp_cnt = aic_capture->cur_cnt - aic_capture->last_cnt;
    else
        temp_cnt = aic_capture->cur_cnt + ((0xFFFFFFFF - aic_capture->last_cnt) + 1);

    pulsewidth_us = temp_cnt / (INPUTCAP_CLK_RATE / 1000000);

    return pulsewidth_us;

}

int aic_inputcap_int_dis(u32 ch, u8 event)
{
    struct aic_inputcap_pdata *aic_capture = _get_inputcap_priv(ch);

    if (aic_capture == NULL) {
        pr_err("invalid ch: %d\n", ch);
        return -EINVAL;
    }

    hal_inputcap_set_fifo(aic_capture->id, event, 0);
    hal_inputcap_evnt_en(aic_capture->id, event, 0);

    return EOK;
}

u32* aic_get_inputcap_data(u32 ch, u8 event)
{
    struct aic_inputcap_pdata *aic_capture = _get_inputcap_priv(ch);

    if (aic_capture == NULL) {
        pr_err("invalid ch: %d\n", ch);
        return NULL;
    }

    if (event == 0)
        return g_event0_pluse[ch];

    if (event == 1)
        return g_event1_pluse[ch];

    return NULL;
}
#endif

#ifndef AIC_DMA_DRV
irqreturn_t aic_inputcap_irq(int irq, void *arg)
{
    struct aic_inputcap_pdata *aic_capture = (struct aic_inputcap_pdata *)arg;
    u32 stat;
    static u32 event0_count = 0;
    static u32 event1_count = 0;

    stat = hal_inputcap_int_stat(aic_capture->id);
    if((stat & CAP_FIFO0_FLG)) {
        aic_capture->isr_event = 0;
        hal_inputcap_int_clr(aic_capture->id, CAP_FIFO0_FLG);
        g_event0_pluse[aic_capture->id][event0_count++] = aic_inputcap_get_pulsewidth(aic_capture->id);
        if (event0_count >= INPUTCAP_WATER_MARK) {
            event0_count = 0;
            if (g_inputcap_cb[aic_capture->id].func_event0 != NULL)
                g_inputcap_cb[aic_capture->id].func_event0((void *)(&aic_capture->id));
        }
    }
    if((stat & CAP_FIFO1_FLG)) {
        aic_capture->isr_event = 1;
        hal_inputcap_int_clr(aic_capture->id, CAP_FIFO1_FLG);
        g_event1_pluse[aic_capture->id][event1_count++] = aic_inputcap_get_pulsewidth(aic_capture->id);
        if (event1_count >= INPUTCAP_WATER_MARK) {
            event1_count = 0;
            if (g_inputcap_cb[aic_capture->id].func_event1 != NULL)
                g_inputcap_cb[aic_capture->id].func_event1((void *)(&aic_capture->id));
        }
    }

    return IRQ_HANDLED;
}
#endif

static int aic_inputcap_probe(struct aic_inputcap_pdata *pdata)
{
    if (hal_clk_enable_deassertrst(CLK_CAP0 + pdata->id) < 0) {
        pr_err("Failed to reset INPUTCAP deassert\n");
        return -EINVAL;
    }

#ifndef AIC_DMA_DRV
    aicos_request_irq(CAP0_IRQn + pdata->id, aic_inputcap_irq, 0, NULL, pdata);
#endif

    pr_info("ArtInChip incap%d loaded", pdata->id);

    return EOK;
}

int drv_inputcap_init(void)
{
    int ret = 0;
    int i;

    if (hal_clk_set_freq(CLK_CAP_SDFM, INPUTCAP_CLK_RATE) < 0) {
        pr_err("Failed to set INPUTCAP clk %d\n", INPUTCAP_CLK_RATE);
        return -EINVAL;
    }

    for (i = 0; i < ARRAY_SIZE(g_inputcap_pdata); i++) {
        ret = aic_inputcap_probe(&g_inputcap_pdata[i]);
        if (ret)
            return ret;
    }

    return EOK;
}

