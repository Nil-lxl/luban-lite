/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
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
#include "aic_hal_clk.h"

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

    if (!chan)
        return -RT_EINVAL;

    hal_gpai_clk_get(chan);

    if (enabled) {
        aich_gpai_ch_init(chan, chan->pclk_rate);
        chan->irq_count = 0;
        if (chan->mode == AIC_GPAI_MODE_SINGLE) {
            chan->irq_count++;
            chan->complete = aicos_sem_create(0);
        }
    } else {
        aich_gpai_ch_deinit(chan);
        if (chan->mode == AIC_GPAI_MODE_SINGLE) {
            aicos_sem_delete(chan->complete);
            chan->complete = NULL;
        }
#ifdef AIC_GPAI_DRV_DMA
        if (chan->dma_rx_info.buf != NULL) {
            aicos_free_align(MEM_DEFAULT, chan->dma_rx_info.buf);
            chan->dma_rx_info.buf = NULL;
            chan->dma_rx_info.buf_size = 0;
        }
#endif
    }

    return RT_EOK;
}

static rt_err_t drv_gpai_convert(struct rt_adc_device *dev, rt_uint32_t ch,
                                 rt_uint32_t *value)
{
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(ch);

    if (!chan)
        return -RT_EINVAL;

    *value = 0;

    return hal_gpai_get_data(chan, (u16 *)value, AIC_GPAI_TIMEOUT);
}

static rt_err_t drv_gpai_get_ch_info(struct rt_adc_device *dev, void *chan_info)
{
    struct aic_gpai_ch_info *info = (struct aic_gpai_ch_info *)chan_info;
    struct aic_gpai_ch *chan = hal_gpai_ch_is_valid(info->chan_id);

    if (!chan)
        return -RT_EINVAL;

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

    if (!chan)
        return -RT_EINVAL;

    chan->irq_info.callback = irq_info->callback;
    chan->irq_info.callback_param = irq_info->callback_param;

    return RT_EOK;
}

#ifdef AIC_GPAI_DRV_DMA
static rt_err_t drv_gpai_config_dma(struct rt_adc_device *dev, void *dma_info)
{
    struct aic_dma_transfer_info *chan_info = NULL;
    struct aic_gpai_ch *chan = NULL;
    int buf_size = 0;

    if (!dma_info)
        return -RT_EINVAL;

    chan_info = (struct aic_dma_transfer_info *)dma_info;
    chan = hal_gpai_ch_is_valid(chan_info->chan_id);

    if (!chan || chan->obtain_data_mode == AIC_GPAI_OBTAIN_DATA_BY_CPU)
        return -RT_EINVAL;

    if (chan->mode == AIC_GPAI_MODE_SINGLE) {
        if (chan_info->smp_cnt != 1)
            hal_log_warn("Single mode only support one sample\n");
        chan_info->smp_cnt = 1;
    }
    chan_info->buf_size = chan_info->smp_cnt * sizeof(u32);
    buf_size = ALIGN_UP(chan_info->buf_size, CACHE_LINE_SIZE);
    if (chan->dma_rx_info.buf_size != buf_size) {
        if (chan->dma_rx_info.buf != NULL) {
            aicos_free_align(MEM_DEFAULT, chan->dma_rx_info.buf);
            chan->dma_rx_info.buf = NULL;
            chan->dma_rx_info.buf_size = 0;
        }

        chan_info->buf = aicos_malloc_align(MEM_DEFAULT, buf_size, CACHE_LINE_SIZE);
        if (!chan_info->buf) {
            hal_log_err("Failed to malloc dma buffer\n");
            return -ENOMEM;
        }
        chan_info->buf_size = buf_size;
        if (chan->mode == AIC_GPAI_MODE_PERIOD)
            chan_info->smp_cnt = buf_size / sizeof(u32);

        chan->dma_rx_info.buf = chan_info->buf;
        chan->dma_rx_info.buf_size = buf_size;
        chan->irq_info.callback = chan_info->callback;
        chan->irq_info.callback_param = chan_info->callback_param;
    }
    hal_gpai_config_dma(chan);

    return RT_EOK;
}

static rt_err_t drv_gpai_active_dma(struct rt_adc_device *dev,
                                    void *arg)
{
    rt_uint32_t channel = (rt_uint32_t)arg;
    struct aic_gpai_ch *chan = NULL;

    chan = hal_gpai_ch_is_valid(channel);
    if (!chan)
        return -RT_EINVAL;
#ifdef RT_USING_PM
    rt_pm_module_request(PM_NONE_ID, PM_SLEEP_MODE_NONE);
#endif

    hal_gpai_start_dma(chan);

    return RT_EOK;
}
#endif

#ifdef RT_USING_PM
#ifdef AIC_PM_DRV_V15
static void aic_gpai_resume_chan(void)
{
    struct aic_gpai_ch *chan = NULL;
    rt_uint8_t i = 0;

    for (i = 0; i < AIC_GPAI_CH_NUM; i++) {
        chan = hal_gpai_ch_is_valid(i);
        if (chan && chan->enabled) {
            aich_gpai_ch_init(chan, chan->pclk_rate);
#ifdef AIC_GPAI_DRV_DMA
            if (chan->dma_rx_info.buf)
                hal_gpai_config_dma(chan);
#endif
        }
    }
}
#endif
static int aic_gpai_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_gpai_deinit();
#else
        hal_clk_disable(CLK_GPAI);
#endif
        break;
    default:
        break;
    }

    return 0;
}

static void aic_gpai_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_gpai_init();
        aich_gpai_enable(1);
        aic_gpai_resume_chan();
#else
        hal_clk_enable(CLK_GPAI);
#endif
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_gpai_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_gpai_suspend, aic_gpai_resume)
    NULL,
};
#endif

static const struct rt_adc_ops aic_adc_ops =
{
    .enabled = drv_gpai_enabled,
    .convert = drv_gpai_convert,
#ifdef AIC_GPAI_DRV_DMA
    .config_dma = drv_gpai_config_dma,
    .active_dma = drv_gpai_active_dma,
#endif
    .get_resolution = drv_gpai_resolution,
    .get_obtaining_data_mode = drv_gpai_obtain_data_mode,
    .irq_callback = drv_gpai_irq_callback,
    .get_ch_info = drv_gpai_get_ch_info,
    .get_mode = drv_gpai_get_mode,
};

static void drv_gpai_event_cb(struct aic_gpai_ch *chan, enum aic_gpai_event ev)
{
    irq_callback user_cb = NULL;
    void *user_data = NULL;

    if (!chan)
        return;
    user_cb = chan->irq_info.callback;
    user_data = chan->irq_info.callback_param;
    if (user_cb)
        user_cb(user_data);
#ifdef AIC_GPAI_DRV_DMA
#ifdef RT_USING_PM
    rt_pm_module_release(PM_NONE_ID, PM_SLEEP_MODE_NONE);
#endif
#endif
}

static int drv_gpai_init(void)
{
    struct rt_adc_device *dev = NULL;
    struct aic_gpai_ch *chan = NULL;
    s32 ret = 0;

    if (hal_gpai_init())
        return -RT_ERROR;

#ifndef AIC_GPAI_DRV_POLL
    aicos_request_irq(GPAI_IRQn, aich_gpai_isr, 0, NULL, NULL);
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
        aicos_free(0, dev);
        return ret;
    }
    for (int i = 0; i < AIC_GPAI_CH_NUM; i++) {
        chan = hal_gpai_ch_is_valid(i);
        if (chan)
            chan->ev_cb = drv_gpai_event_cb;
    }
#ifdef RT_USING_PM
    rt_pm_device_register(&dev->parent, &aic_gpai_pm_ops);
#endif
    return 0;
}
INIT_DEVICE_EXPORT(drv_gpai_init);
