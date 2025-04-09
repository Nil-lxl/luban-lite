/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

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

struct aic_psadc_queue aic_psadc_queues[] = {
    {
        .id = 0,
        .type = AIC_PSADC_QC,
    },
};
