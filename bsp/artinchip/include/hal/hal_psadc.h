/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
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

enum aic_psadc_mode {
    AIC_PSADC_MODE_SINGLE = 0,
    AIC_PSADC_MODE_PERIOD = 1
};

enum aic_psadc_queue_type {
    AIC_PSADC_QC = 0,
    AIC_PSADC_Q1 = 1,
    AIC_PSADC_Q2 = 2
};

struct aic_psadc_ch {
    u8 id;
    u8 available;
    enum aic_psadc_mode mode;
    u8 fifo_depth;
};

struct aic_psadc_queue {
    u8 id;
    int type;
    u8 nodes_num;
    aicos_sem_t complete;
};

void hal_psadc_enable(int enable);
void hal_psadc_single_queue_mode(int enable);
void hal_psadc_qc_irq_enable(int enable);
int hal_psadc_ch_init();
irqreturn_t hal_psadc_isr(int irq, void *arg);
int hal_psadc_read(u32 *val, u32 timeout);
int hal_psadc_read_poll(u32 *val, u32 timeout);
struct aic_psadc_ch *hal_psadc_ch_is_valid(u32 ch);
void hal_psadc_set_ch_num(u32 num);
void aich_psadc_status_show(void);
int hal_psadc_set_queue_node(int queue, int ch, int node_ordinal);

#endif
