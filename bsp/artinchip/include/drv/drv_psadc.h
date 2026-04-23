/*
 * Copyright (C) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  deqiang.lin <deqiang.lin@artinchip.com>
 */

#ifndef _AIC_DRV_PSADC_H_
#define _AIC_DRV_PSADC_H_

typedef void (*drv_dma_cb)(void *dma_param);

struct drv_psadc_dma_info
{
    void *buf;
    int smp_cnt;
    int buf_size;
    void *callback_param;
    drv_dma_cb callback;
};

#endif /* _AIC_DRV_PSADC_H_ */
