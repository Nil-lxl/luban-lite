/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */

#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <drivers/adc.h>

#define LOG_TAG            "PSADC"
#include "aic_core.h"
#include "aic_hal_clk.h"

#include "drv_psadc.h"
#include "hal_psadc.h"
#include "hal_dma.h"

#define AIC_PSADC_NAME      "psadc"

#ifdef AIC_PSADC_DRV_V11
#define AIC_PSADC_CLK_RATE    40000000   /* 40MHz */
#endif

struct aic_psadc_ch aic_psadc_chs[] = {
#ifdef AIC_USING_PSADC0
    {
        .id = 0,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC1
    {
        .id = 1,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC2
    {
        .id = 2,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC3
    {
        .id = 3,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC4
    {
        .id = 4,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC5
    {
        .id = 5,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC6
    {
        .id = 6,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC7
    {
        .id = 7,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC8
    {
        .id = 8,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC9
    {
        .id = 9,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC10
    {
        .id = 10,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC11
    {
        .id = 11,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC12
    {
        .id = 12,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC13
    {
        .id = 13,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC14
    {
        .id = 14,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
#ifdef AIC_USING_PSADC15
    {
        .id = 15,
        .available = 1,
        .mode = AIC_PSADC_MODE_SINGLE,
        .fifo_depth = 12,
    },
#endif
};

/* As to now, just only support QC type */
struct aic_psadc_dev aic_psadc_devs[] = {
    {
        .id = 0,
        .type = AIC_PSADC_QC,
    },
};
#define CHECK_QUEUE_VALID(q, ret)    \
    do { \
        if (q != AIC_PSADC_QC) { \
            pr_err("Invalid queue type: %d\n", q); \
            return ret; \
        } \
    } while (0)

static rt_err_t drv_psadc_enabled(struct rt_adc_device *dev,
                                  rt_uint32_t queue_type, rt_bool_t enabled)
{
    rt_device_t parent = (rt_device_t)dev;
    struct aic_psadc_dev *queue;

    queue = (struct aic_psadc_dev *)parent->user_data;

    CHECK_QUEUE_VALID(queue_type, -EINVAL);

    if (enabled) {
        int cnt = 0;
        for (int i = 0; i < AIC_PSADC_CH_NUM; i++) {
            struct aic_psadc_ch *chan = hal_psadc_ch_is_valid(i);
            if (!chan)
                continue;
            if (chan->available && cnt < AIC_PSADC_QUEUE_LENGTH) {
                hal_psadc_set_queue_node(AIC_PSADC_Q1, chan->id, cnt);
                cnt++;
                continue;
            }
            if (chan->available && cnt >= AIC_PSADC_QUEUE_LENGTH) {
                hal_psadc_set_queue_node(AIC_PSADC_Q2, chan->id,
                                         cnt - AIC_PSADC_QUEUE_LENGTH);
                cnt++;
                continue;
            }
        }
        if (!cnt) {
            pr_err("Forget to enable PSADC channel?\n");
            return -RT_EINVAL;
        }

        queue->nodes_num = cnt;
        queue->complete = aicos_sem_create(0);
        if (!queue->complete) {
            pr_err("Failed to create complete\n");
            return -RT_ENOMEM;
        }

        hal_psadc_init(queue);
    } else {
        hal_psadc_deinit(queue);
        if (queue->complete) {
            aicos_sem_delete(queue->complete);
            queue->complete = NULL;
        }
#ifdef AIC_PSADC_DRV_DMA
        if (queue->dma_info.buf != NULL) {
            aicos_free_align(MEM_DEFAULT, queue->dma_info.buf);
            queue->dma_info.buf = NULL;
            queue->dma_info.buf_size = 0;
        }
#endif
    }

    return RT_EOK;
}

#ifdef AIC_PSADC_DRV_DMA
static void drv_psadc_dma_cb(void *arg)
{
    struct aic_psadc_dev *queue = (struct aic_psadc_dev *)arg;
    struct aic_psadc_dma_info *queue_info = NULL;

    queue_info = &queue->dma_info;
    hal_dma_chan_stop(queue->dma_chan);
    aicos_dcache_invalid_range(queue_info->buf, queue_info->buf_size);
    if (queue_info->callback)
        queue_info->callback(queue_info->callback_param);
}

static rt_err_t drv_psadc_active_dma(struct rt_adc_device *dev, void *arg)
{
    struct aic_psadc_dma_info *queue_info = NULL;
    struct aic_psadc_dma_info tmp_info = {0};
    rt_device_t parent = (rt_device_t)dev;
    struct aic_psadc_dev *queue;

    queue = (struct aic_psadc_dev *)parent->user_data;
    queue_info = &queue->dma_info;
#ifdef AIC_PSADC_TRIG_BY_SOFT
    tmp_info.buf_size = queue->nodes_num * sizeof(u32);
#else
    tmp_info.buf_size = queue_info->buf_size;
#endif
    tmp_info.buf = queue_info->buf;
    tmp_info.callback = drv_psadc_dma_cb;
    tmp_info.callback_param = queue;
    return hal_psadc_active_dma(queue, &tmp_info);
}

static rt_err_t drv_psadc_config_dma(struct rt_adc_device *dev, void *dma_info)
{
    struct aic_psadc_dma_info *queue_info = NULL;
    struct drv_psadc_dma_info *drv_info = NULL;
    rt_device_t parent = (rt_device_t)dev;
    struct aic_psadc_dev *queue;
    int buf_size = 0;

    queue = (struct aic_psadc_dev *)parent->user_data;
    queue_info = &queue->dma_info;
    drv_info = (struct drv_psadc_dma_info *)dma_info;
    if (drv_info == NULL)
        return -RT_EINVAL;

#ifdef AIC_PSADC_TRIG_BY_SOFT
    if (drv_info->smp_cnt > 1) {
        pr_warn("Only support one sampling when trigger by software and using dma.\n");
        drv_info->smp_cnt = 1;
    }
#endif
    buf_size = drv_info->smp_cnt * queue->nodes_num * sizeof(u32);
    buf_size = ALIGN_UP(buf_size, CACHE_LINE_SIZE);
#ifndef AIC_PSADC_TRIG_BY_SOFT
    drv_info->smp_cnt = buf_size / (queue->nodes_num * sizeof(u32));
#endif

    if (buf_size != queue_info->buf_size) {
        if (queue_info->buf_size != 0) {
            aicos_free_align(MEM_DEFAULT, queue_info->buf);
            queue_info->buf = NULL;
            queue_info->buf_size = 0;
        }

        drv_info->buf = aicos_malloc_align(MEM_DEFAULT, buf_size, CACHE_LINE_SIZE);
        if (!drv_info->buf) {
            LOG_E("Failed to malloc dma buffer\n");
            return -ENOMEM;
        }
        drv_info->buf_size = buf_size;

        queue_info->buf_size = buf_size;
        queue_info->buf = drv_info->buf;
        queue_info->callback = drv_info->callback;
        queue_info->callback_param = drv_info->callback_param;
    }
    return 0;
}
#endif
#ifdef AIC_PSADC_DRV_POLL
static rt_err_t drv_psadc_get_adc_values_poll(struct rt_adc_device *dev,
                                              void *values)
{
    rt_device_t parent = (rt_device_t)dev;
    struct aic_psadc_dev *queue;

    queue = (struct aic_psadc_dev *)parent->user_data;
    return hal_psadc_read_poll(queue, values,
                               AIC_PSADC_POLL_READ_TIMEOUT);
}
#else
static rt_err_t drv_psadc_get_adc_values(struct rt_adc_device *dev,
                                         void *values)
{
    rt_device_t parent = (rt_device_t)dev;
    struct aic_psadc_dev *queue;

    queue = (struct aic_psadc_dev *)parent->user_data;
    return hal_psadc_read(queue, values, AIC_PSADC_TIMEOUT);
}
#endif

static rt_uint32_t drv_psadc_get_chan_count(struct rt_adc_device *dev)
{
    rt_device_t parent = (rt_device_t)dev;
    struct aic_psadc_dev *queue;

    queue = (struct aic_psadc_dev *)parent->user_data;
    return queue->nodes_num;
}

static rt_uint8_t drv_psadc_resolution(struct rt_adc_device *dev)
{
    return 12;
}

static rt_err_t drv_psadc_convert(struct rt_adc_device *dev, rt_uint32_t ch,
                                 rt_uint32_t *value)
{
    pr_warn("Please call ioctl API to get ADC data\n");
    return -RT_EINVAL;
}

static const struct rt_adc_ops aic_adc_ops =
{
    .enabled = drv_psadc_enabled,
    .convert = drv_psadc_convert,
    .get_resolution = drv_psadc_resolution,
#ifdef AIC_PSADC_DRV_DMA
    .active_dma = drv_psadc_active_dma,
    .config_dma = drv_psadc_config_dma,
#endif
#ifdef AIC_PSADC_DRV_POLL
    .get_adc_values_poll = drv_psadc_get_adc_values_poll,
#else
    .get_adc_values = drv_psadc_get_adc_values,
#endif
    .get_chan_count = drv_psadc_get_chan_count,
};

static int drv_psadc_init(void)
{
    struct aic_psadc_dev *queue = NULL;
    struct rt_adc_device *dev = NULL;
    s32 ret = 0;

    queue = &aic_psadc_devs[0];
#ifdef AIC_PSADC_DRV_V11
    ret = hal_clk_set_freq(CLK_PSADC, AIC_PSADC_CLK_RATE);
    if (ret < 0) {
            LOG_E("PSADC clk freq set failed!");
            return -RT_ERROR;
    }
#endif

    ret = hal_clk_enable_deassertrst(CLK_PSADC);
    if (ret < 0) {
        LOG_E("PSADC reset deassert failed!");
        return -RT_ERROR;
    }

    hal_psadc_single_queue_mode(1);

#ifndef AIC_PSADC_DRV_POLL
    ret = aicos_request_irq(PSADC_IRQn, hal_psadc_isr, 0, NULL, queue);
      if (ret < 0) {
        LOG_E("PSADC irq enable failed!");
        goto err_irq;
    }
#endif

    hal_psadc_enable(1);
    hal_psadc_set_ch_num(ARRAY_SIZE(aic_psadc_chs));

    dev = aicos_malloc(0, sizeof(struct rt_adc_device));
    if (!dev) {
        LOG_E("Failed to malloc(%d)", sizeof(struct rt_adc_device));
        ret = -RT_ERROR;
        goto err_irq;
    }
    memset(dev, 0, sizeof(struct rt_adc_device));

    ret = rt_hw_adc_register(dev, AIC_PSADC_NAME, &aic_adc_ops, queue);
    if (ret) {
        LOG_E("Failed to register ADC. ret %d", ret);
        aicos_free(0, dev);
        goto err_irq;
    }
    LOG_I("ArtInChip PSADC loaded");
    return 0;

err_irq:
    hal_clk_disable_assertrst(CLK_PSADC);

    return ret;
}
INIT_DEVICE_EXPORT(drv_psadc_init);
