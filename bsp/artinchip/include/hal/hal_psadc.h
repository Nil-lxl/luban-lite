/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_PSADC_H_
#define _ARTINCHIP_HAL_PSADC_H_

#include "aic_osal.h"

#define AIC_PSADC_FIFO1_NUM_BITS    20
#define AIC_PSADC_FIFO2_NUM_BITS    12
#define AIC_PSADC_TIMEOUT           1000 /* 1000 ms */
#define AIC_PSADC_POLL_READ_TIMEOUT 1000 /* 1000 times */
#define AIC_PSADC_QUEUE_LENGTH      8

typedef void (*dma_callback)(void *arg);
struct aic_dma_chan;

enum aic_psadc_mode {
    AIC_PSADC_MODE_SINGLE = 0,
    AIC_PSADC_MODE_PERIOD = 1
};

enum aic_psadc_queue_type {
    AIC_PSADC_QC = 0,
    AIC_PSADC_Q1 = 1,
    AIC_PSADC_Q2 = 2,
    AIC_PSADC_Q_NUM
};

struct aic_psadc_ch {
    u8 id;
    u8 available;
    u8 fifo_depth;
    enum aic_psadc_mode mode;
};

struct aic_psadc_dma_info {
    void *buf;
    int buf_size;
    void *callback_param;
    dma_callback callback;
};

struct aic_psadc_dev {
    u8 id;
    u8 nodes_num;
    int type;
    struct aic_psadc_ch *chan;
    u8 chan_num;
    aicos_sem_t complete;
#ifdef AIC_PSADC_DRV_DMA
    struct aic_dma_chan *dma_chan;
    struct aic_psadc_dma_info dma_info;
#endif
};

void hal_psadc_enable(int enable);
void hal_psadc_single_queue_mode(int enable);
void hal_psadc_qc_irq_enable(bool enable);
int hal_psadc_init(struct aic_psadc_dev *queue);
void hal_psadc_deinit(struct aic_psadc_dev *queue);
irqreturn_t hal_psadc_isr(int irq, void *arg);
void hal_psadc_sw_trigger(void);
int hal_psadc_read(struct aic_psadc_dev *queue, u32 *val, u32 timeout);
int hal_psadc_read_poll(struct aic_psadc_dev *queue, u32 *val, u32 timeout);
struct aic_psadc_ch *hal_psadc_ch_is_valid(u32 ch);
void hal_psadc_set_ch_num(u32 num);
void aich_psadc_status_show(void);
int hal_psadc_set_queue_node(int queue, int ch, int node_ordinal);
#ifdef AIC_PSADC_DRV_DMA
int hal_psadc_active_dma(struct aic_psadc_dev *queue, struct aic_psadc_dma_info *dma_info);
#endif

#endif
