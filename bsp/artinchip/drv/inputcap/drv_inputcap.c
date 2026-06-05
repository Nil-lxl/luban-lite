/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#include <string.h>
#include <drivers/rt_inputcapture.h>
#include <drivers/pm.h>

#define LOG_TAG         "INPUTCAP"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_inputcap.h"

struct aic_inputcap {
    struct rt_inputcapture_device rtdev;
    struct aic_inputcap_pdata *data;
};

#ifdef AIC_DMA_DRV
static void inputcap_fifo0_dma_callback(void *arg)
{
    struct aic_inputcap *aic_capture;
    u8 fifo_flag = 0;

    aic_capture = (struct aic_inputcap *)arg;
    aic_capture->rtdev.parent.user_data = &fifo_flag;

    if (aic_capture->rtdev.parent.rx_indicate != RT_NULL)
        aic_capture->rtdev.parent.rx_indicate(&aic_capture->rtdev.parent, aic_capture->data->t_info0.buf_info.buf_len);
}
static void inputcap_fifo1_dma_callback(void *arg)
{
    struct aic_inputcap *aic_capture;
    u8 fifo_flag = 1;

    aic_capture = (struct aic_inputcap *)arg;
    aic_capture->rtdev.parent.user_data = &fifo_flag;

    if (aic_capture->rtdev.parent.rx_indicate != RT_NULL)
        aic_capture->rtdev.parent.rx_indicate(&aic_capture->rtdev.parent, aic_capture->data->t_info1.buf_info.buf_len);
}
#endif

static rt_err_t aic_inputcap_init(struct rt_inputcapture_device *inputcapture)
{
    return RT_EOK;
}

#ifdef AIC_DMA_DRV
static rt_err_t aic_inputcap_setbuf(struct rt_inputcapture_device *inputcapture, struct rt_inputcapture_fifo_buf *ptr)
{
    struct aic_inputcap *aic_capture;

    RT_ASSERT(inputcapture != RT_NULL);

    aic_capture = (struct aic_inputcap *)inputcapture;

    aic_capture->data->t_info0.buf_info.buf = ptr->event0_buf;
    aic_capture->data->t_info0.buf_info.buf_len = ptr->event0_buflen;
    aic_capture->data->t_info1.buf_info.buf = ptr->event1_buf;
    aic_capture->data->t_info1.buf_info.buf_len = ptr->event1_buflen;

    return RT_EOK;
}
#endif

static rt_err_t aic_inputcap_open(struct rt_inputcapture_device *inputcapture)
{
    struct aic_inputcap *aic_capture;

    RT_ASSERT(inputcapture != RT_NULL);

    aic_capture = (struct aic_inputcap *)inputcapture;

    if (aic_capture->data->event0_pol != 0) {
        hal_inputcap_pol_set(aic_capture->data->id, 0, (aic_capture->data->event0_pol - 1));
        hal_inputcap_set_fifo(aic_capture->data->id, 0, 1);
        hal_inputcap_evnt_en(aic_capture->data->id, 0, 1);
#ifdef AIC_DMA_DRV
        hal_inputcap_dma_config(aic_capture->data, 0, inputcap_fifo0_dma_callback, (void *)aic_capture);
#endif
    }

    if (aic_capture->data->event1_pol != 0) {
        hal_inputcap_pol_set(aic_capture->data->id, 1, (aic_capture->data->event1_pol - 1));
        hal_inputcap_set_fifo(aic_capture->data->id, 1, 1);
        hal_inputcap_evnt_en(aic_capture->data->id, 1, 1);
#ifdef AIC_DMA_DRV
        hal_inputcap_dma_config(aic_capture->data, 1, inputcap_fifo1_dma_callback, (void *)aic_capture);
#endif
    }
    hal_inputcap_cnt_en(aic_capture->data->id, 1);

    return RT_EOK;
}

static rt_err_t aic_inputcap_close(struct rt_inputcapture_device *inputcapture)
{
    struct aic_inputcap *aic_capture;

    RT_ASSERT(inputcapture != RT_NULL);

    aic_capture = (struct aic_inputcap *)inputcapture;

    if (aic_capture->data->event0_pol != 0) {
        hal_inputcap_set_fifo(aic_capture->data->id, 0, 0);
        hal_inputcap_evnt_en(aic_capture->data->id, 0, 0);
#ifdef AIC_DMA_DRV
        hal_inputcap_fifo_flush(aic_capture->data->id, 0);
        hal_dma_chan_stop(aic_capture->data->t_info0.dma_chan);
        hal_release_dma_chan(aic_capture->data->t_info0.dma_chan);
#endif
    }
    if (aic_capture->data->event1_pol != 0) {
        hal_inputcap_set_fifo(aic_capture->data->id, 1, 0);
        hal_inputcap_evnt_en(aic_capture->data->id, 1, 0);
#ifdef AIC_DMA_DRV
        hal_inputcap_fifo_flush(aic_capture->data->id, 1);
        hal_dma_chan_stop(aic_capture->data->t_info1.dma_chan);
        hal_release_dma_chan(aic_capture->data->t_info1.dma_chan);
#endif
    }
    hal_inputcap_cnt_en(aic_capture->data->id, 0);

    return RT_EOK;
}

static rt_err_t aic_inputcap_get_pulsewidth(struct rt_inputcapture_device *inputcapture, rt_uint32_t *pulsewidth_us)
{
    struct aic_inputcap *aic_capture;
    u32 temp_cnt;

    RT_ASSERT(inputcapture != RT_NULL);

    aic_capture = (struct aic_inputcap *)inputcapture;

    if (aic_capture->data->isr_event == 0) {
        aic_capture->data->last_cnt = hal_inputcap_get_event_fifo(aic_capture->data->id, 0);
        aic_capture->data->cur_cnt = hal_inputcap_get_event_fifo(aic_capture->data->id, 0);
    } else {
        aic_capture->data->last_cnt = hal_inputcap_get_event_fifo(aic_capture->data->id, 1);
        aic_capture->data->cur_cnt = hal_inputcap_get_event_fifo(aic_capture->data->id, 1);
    }

    if (aic_capture->data->cur_cnt > aic_capture->data->last_cnt)
        temp_cnt = aic_capture->data->cur_cnt - aic_capture->data->last_cnt;
    else
        temp_cnt = aic_capture->data->cur_cnt + ((0xFFFFFFFF - aic_capture->data->last_cnt) + 1);

    *pulsewidth_us = temp_cnt / (INPUTCAP_CLK_RATE / 1000000);

    return RT_EOK;

}

static struct rt_inputcapture_ops aic_input_ops =
{
    .init   =   aic_inputcap_init,
    .open   =   aic_inputcap_open,
    .close  =   aic_inputcap_close,
    .get_pulsewidth =   aic_inputcap_get_pulsewidth,
#ifdef AIC_DMA_DRV
    .set_buf =   aic_inputcap_setbuf,
#endif
};

static struct aic_inputcap_pdata inputcap_pdata[] = {
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

#ifndef AIC_DMA_DRV
irqreturn_t aic_inputcap_irq(int irq, void *arg)
{
    struct aic_inputcap *inputcap = (struct aic_inputcap *)arg;
    u32 incap = irq - CAP0_IRQn;
    u32 stat;

    stat = hal_inputcap_int_stat(incap);
    if((stat & CAP_FIFO0_FLG)) {
        inputcap->data->isr_event = 0;
        hal_inputcap_int_clr(incap, CAP_FIFO0_FLG);
        rt_hw_inputcapture_isr(&inputcap->rtdev, inputcap->data->event0_pol);
    }
    if((stat & CAP_FIFO1_FLG)) {
        inputcap->data->isr_event = 1;
        hal_inputcap_int_clr(incap, CAP_FIFO1_FLG);
        rt_hw_inputcapture_isr(&inputcap->rtdev, inputcap->data->event1_pol);
    }

    return IRQ_HANDLED;
}
#endif

static rt_err_t aic_inputcap_probe(struct aic_inputcap_pdata *pdata)
{
    struct aic_inputcap *inputcap;
    char aic_inputcap_device_name[10] = "";

    inputcap = (struct aic_inputcap *)malloc(sizeof(struct aic_inputcap));
    if (!inputcap) {
        LOG_E("Failed to malloc(%d)\n", (u32)sizeof(struct aic_inputcap));
        goto err;
    }

    inputcap->data = pdata;
    inputcap->rtdev.ops = &aic_input_ops;

    if (hal_clk_enable_deassertrst(CLK_CAP0 + inputcap->data->id) < 0) {
        hal_log_err("Failed to reset INPUTCAP deassert\n");
        goto err;
    }

#ifndef AIC_DMA_DRV
    aicos_request_irq(CAP0_IRQn + inputcap->data->id, aic_inputcap_irq, 0, NULL, inputcap);
#endif

    snprintf(aic_inputcap_device_name, 10, "incap%d", inputcap->data->id);

    rt_device_inputcapture_register(&inputcap->rtdev, aic_inputcap_device_name, NULL);

    LOG_I("ArtInChip %s loaded", aic_inputcap_device_name);

    return RT_EOK;
err:
    if (inputcap)
        free(inputcap);

    return -EINVAL;
}

static int drv_inputcap_init(void)
{
    rt_err_t ret = RT_EOK;
    int i;

    if (hal_clk_set_freq(CLK_CAP_SDFM, INPUTCAP_CLK_RATE) < 0) {
        hal_log_err("Failed to set INPUTCAP clk %d\n", INPUTCAP_CLK_RATE);
        return -RT_ERROR;
    }

    for (i = 0; i < ARRAY_SIZE(inputcap_pdata); i++) {
        ret = aic_inputcap_probe(&inputcap_pdata[i]);
        if (ret)
            return ret;
    }

    return 0;
}
INIT_DEVICE_EXPORT(drv_inputcap_init);
