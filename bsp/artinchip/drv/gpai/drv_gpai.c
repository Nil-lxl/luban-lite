/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <drivers/adc.h>

#define LOG_TAG            "GPAI"
#include "aic_core.h"
#include "hal_gpai.h"

extern struct aic_gpai_ch aic_gpai_chs[];
extern const int aic_gpai_chs_size;
#define AIC_GPAI_NAME      "gpai"

struct aic_gpai_dev {
    struct rt_adc_device *dev;
    struct aic_gpai_ch *chan;
};

static rt_err_t drv_gpai_enabled(struct rt_adc_device *dev,
                                 rt_uint32_t ch, rt_bool_t enabled)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    hal_gpai_clk_get(chan);
    if (!chan)
        return -RT_EINVAL;

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

    return RT_EOK;
}

static rt_err_t drv_gpai_convert(struct rt_adc_device *dev, rt_uint32_t ch,
                                 rt_uint32_t *value)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    *value = 0;

    if (!chan)
        return -RT_EINVAL;

    return hal_gpai_get_data(chan, (u16 *)value, AIC_GPAI_TIMEOUT);
}

static rt_err_t drv_gpai_get_ch_info(struct rt_adc_device *dev, void *chan_info)
{
    struct aic_gpai_ch_info *info = (struct aic_gpai_ch_info *)chan_info;
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(info->chan_id);

    hal_gpai_get_data(chan, info->adc_values, AIC_GPAI_TIMEOUT);
    info->fifo_valid_cnt = chan->fifo_valid_cnt;

    return RT_EOK;
}

static rt_uint8_t drv_gpai_resolution(struct rt_adc_device *dev)
{
    return 12;
}

static rt_err_t drv_gpai_get_mode(struct rt_adc_device *dev,
                                     void *chan_info)
{
    struct aic_gpai_ch_info *info = (struct aic_gpai_ch_info *)chan_info;
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(info->chan_id);

    if (!chan)
        return -RT_EINVAL;

    info->mode = chan->mode;

    return RT_EOK;
}

static rt_uint32_t drv_gpai_obtain_data_mode(struct rt_adc_device *dev,
                                             rt_uint32_t channel)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(channel);

    if (!chan)
        return -RT_EINVAL;

    return chan->obtain_data_mode;
}

static rt_err_t drv_gpai_irq_callback(struct rt_adc_device *dev,
                                      void *chan_irq_info)
{
    struct aic_gpai_irq_info *irq_info;
    irq_info = (struct aic_gpai_irq_info *)chan_irq_info;
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(irq_info->chan_id);

    chan->irq_info.callback = irq_info->callback;
    chan->irq_info.callback_param = irq_info->callback_param;

    return RT_EOK;
}

#ifdef AIC_GPAI_DRV_DMA
static rt_err_t drv_gpai_config_dma(struct rt_adc_device *dev, void *dma_info)
{
    if (!dma_info)
        return -RT_EINVAL;

    struct aic_dma_transfer_info *chan_info;
    chan_info = (struct aic_dma_transfer_info *)dma_info;
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(chan_info->chan_id);

    if (!chan || chan->obtain_data_mode == AIC_GPAI_OBTAIN_DATA_BY_CPU)
        return -RT_EINVAL;

    chan->dma_rx_info.buf = chan_info->buf;
    chan->dma_rx_info.buf_size = chan_info->buf_size;
    chan->dma_rx_info.callback = chan_info->callback;
    chan->dma_rx_info.callback_param = chan_info->callback_param;
    hal_gpai_config_dma(chan);

    return RT_EOK;
}

static rt_err_t drv_gpai_get_dma_data(struct rt_adc_device *dev,
                                      rt_uint32_t channel)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(channel);

    hal_gpai_start_dma(chan);

    return RT_EOK;
}

static rt_err_t drv_gpai_stop_dma(struct rt_adc_device *dev, rt_uint32_t channel)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(channel);

    hal_gpai_stop_dma(chan);

    return RT_EOK;
}
#endif

static const struct rt_adc_ops aic_adc_ops =
{
    .enabled = drv_gpai_enabled,
    .convert = drv_gpai_convert,
#ifdef AIC_GPAI_DRV_DMA
    .config_dma = drv_gpai_config_dma,
    .get_dma_data = drv_gpai_get_dma_data,
    .stop_dma = drv_gpai_stop_dma,
#endif
    .get_resolution = drv_gpai_resolution,
    .get_obtaining_data_mode = drv_gpai_obtain_data_mode,
    .irq_callback = drv_gpai_irq_callback,
    .get_ch_info = drv_gpai_get_ch_info,
    .get_mode = drv_gpai_get_mode,
};

static int drv_gpai_init(void)
{
    struct rt_adc_device *dev = NULL;
    s32 ret = 0;

    if (hal_gpai_clk_init())
        return -RT_ERROR;

#ifndef AIC_GPAI_DRV_POLL
    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);
#endif
#if defined(AIC_GPAI_DRV_V21)
    aich_gpai_adc_sel_enable(AIC_GPAI_ADC_ACC);
#endif
    aich_gpai_enable(1);
    hal_gpai_set_ch_num(aic_gpai_chs_size);

    dev = aicos_malloc(0, sizeof(struct rt_adc_device));
    if (!dev) {
        LOG_E("Failed to malloc(%d)", sizeof(struct rt_adc_device));
        return -RT_ERROR;
    }
    memset(dev, 0, sizeof(struct rt_adc_device));

    ret = rt_hw_adc_register(dev, AIC_GPAI_NAME, &aic_adc_ops, NULL);
    if (ret) {
        LOG_E("Failed to register ADC. ret %d", ret);
        return ret;
    }
    return 0;
}
INIT_BOARD_EXPORT(drv_gpai_init);
