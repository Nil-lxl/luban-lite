/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
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

#include "hal_psadc.h"

#define AIC_PSADC_NAME      "psadc"

struct aic_psadc_dev {
    struct rt_adc_device *dev;
    struct aic_psadc_ch *chan;
    struct aic_psadc_queue *queue;
};

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
struct aic_psadc_queue aic_psadc_queues[] = {
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
    struct aic_psadc_queue *queue = NULL;

    CHECK_QUEUE_VALID(queue_type, -EINVAL);
    queue = &aic_psadc_queues[queue_type];

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

        hal_psadc_ch_init();
    } else {
        hal_psadc_qc_irq_enable(false);
        if (queue->complete) {
            aicos_sem_delete(queue->complete);
            queue->complete = NULL;
        }
    }

    return RT_EOK;
}

#ifdef AIC_PSADC_DRV_POLL
static rt_err_t drv_psadc_get_adc_values_poll(struct rt_adc_device *dev,
                                              void *values)
{
    return hal_psadc_read_poll(values, AIC_PSADC_POLL_READ_TIMEOUT);
}
#else
static rt_err_t drv_psadc_get_adc_values(struct rt_adc_device *dev,
                                         void *values)
{
    return hal_psadc_read(values, AIC_PSADC_TIMEOUT);
}
#endif

static rt_uint32_t drv_psadc_get_chan_count(struct rt_adc_device *dev)
{
    return aic_psadc_queues[AIC_PSADC_QC].nodes_num;
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
#ifdef AIC_PSADC_DRV_POLL
    .get_adc_values_poll = drv_psadc_get_adc_values_poll,
#else
    .get_adc_values = drv_psadc_get_adc_values,
#endif
    .get_chan_count = drv_psadc_get_chan_count,
};

static int drv_psadc_init(void)
{
    struct rt_adc_device *dev = NULL;
    s32 ret = 0;

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
    ret = aicos_request_irq(PSADC_IRQn, hal_psadc_isr, 0, NULL, NULL);
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

    ret = rt_hw_adc_register(dev, AIC_PSADC_NAME, &aic_adc_ops, NULL);
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
