/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */

#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include "aic_core.h"
#include "hal_gpai.h"
#include "hal_adcim.h"

extern struct aic_gpai_ch aic_gpai_chs[];
extern const int aic_gpai_chs_size;

void drv_gpai_init(void)
{
    hal_gpai_clk_init();
    hal_gpai_set_ch_num(aic_gpai_chs_size);
#if defined(AIC_GPAI_DRV_V21)
    aich_gpai_adc_sel_enable(AIC_GPAI_ADC_ACC);
#endif
    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);
    aich_gpai_enable(1);

    printf("ArtInChip GPAI loaded\n");
    return;
}

int drv_gpai_chan_init(int ch)
{
    struct aic_gpai_ch *chan;

    chan = hal_gpai_ch_is_valid(ch);
    if (!chan)
        return -EINVAL;

    hal_gpai_clk_get(chan);
    aich_gpai_ch_init(chan, chan->pclk_rate);
    return EOK;
}

int drv_gpai_get_mode(int ch)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    if (!chan)
        return -EINVAL;

    switch (chan->mode) {
    case AIC_GPAI_MODE_SINGLE:
        printf("Starting gpai single mode\n");
        break;
    case AIC_GPAI_MODE_PERIOD:
        printf("Starting gpai period mode\n");
        break;
    default:
        printf("Unknown mode: %d\n", chan->mode);
        return -EINVAL;
        break;
    }

    return chan->mode;
}

int drv_gpai_get_data_mode(int ch)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    if (!chan)
        return -EINVAL;

    switch (chan->obtain_data_mode) {
    case AIC_GPAI_OBTAIN_DATA_BY_CPU:
        printf("Starting CPU interrupt mode\n");
        break;
#ifdef AIC_GPAI_DRV_DMA
    case AIC_GPAI_OBTAIN_DATA_BY_DMA:
        printf("Starting DMA mode\n");
        break;
#endif
    case AIC_GPAI_OBTAIN_DATA_BY_POLL:
        printf("Starting polling mode\n");
        break;
    default:
        printf("The current mode%d is not supported\n", chan->obtain_data_mode);
        return -EINVAL;
        break;
    }

    return chan->obtain_data_mode;
}

int drv_gpai_irq_callback(struct aic_gpai_irq_info *irq_info)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(irq_info->chan_id);

    if (!chan)
        return -EINVAL;

    chan->irq_info.callback = irq_info->callback;
    chan->irq_info.callback_param = irq_info->callback_param;

    return EOK;
}

int drv_gpai_enabled(int ch, bool enabled)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    hal_gpai_clk_get(chan);
    if (!chan)
        return -EINVAL;

    if (enabled) {
        aich_gpai_ch_init(chan, chan->pclk_rate);
        chan->irq_count = 0;
        if (chan->mode == AIC_GPAI_MODE_SINGLE) {
            chan->irq_count++;
            chan->complete = aicos_sem_create(0);
        }
    } else {
        aich_gpai_ch_enable(chan->id, 0);
        if (chan->mode == AIC_GPAI_MODE_SINGLE) {
            aicos_sem_delete(chan->complete);
            chan->complete = NULL;
        }
    }

    return EOK;
}

int drv_gpai_get_ch_info(struct aic_gpai_ch_info *chan_info)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(chan_info->chan_id);

    hal_gpai_get_data(chan, chan_info->adc_values, AIC_GPAI_TIMEOUT);
    chan_info->fifo_valid_cnt = chan->fifo_valid_cnt;

    return EOK;
}

#ifdef AIC_GPAI_DRV_DMA
int drv_gpai_config_dma(struct aic_dma_transfer_info *dma_info)
{
    if (!dma_info)
        return -EINVAL;

    struct aic_dma_transfer_info *chan_info;
    chan_info = (struct aic_dma_transfer_info *)dma_info;
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(chan_info->chan_id);

    if (!chan || chan->obtain_data_mode == AIC_GPAI_OBTAIN_DATA_BY_CPU)
        return -EINVAL;

    chan->dma_rx_info.buf = chan_info->buf;
    chan->dma_rx_info.buf_size = chan_info->buf_size;
    chan->dma_rx_info.callback = chan_info->callback;
    chan->dma_rx_info.callback_param = chan_info->callback_param;
    hal_gpai_config_dma(chan);

    return EOK;
}

int drv_gpai_get_dma_data(int ch)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    hal_gpai_start_dma(chan);

    return EOK;
}

int drv_gpai_stop_dma(int ch)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    hal_gpai_stop_dma(chan);

    return EOK;
}
#endif
