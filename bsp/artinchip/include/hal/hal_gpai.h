/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_GPAI_H_
#define _ARTINCHIP_HAL_GPAI_H_

#include "aic_osal.h"

#define AIC_GPAI_NUM_BITS           12
#define AIC_GPAI_TIMEOUT            1000 /* 1000 ms */
#define AIC_GPAI_FIFO_MAX_DEPTH     32

enum aic_gpai_mode {
    AIC_GPAI_MODE_SINGLE = 0,
    AIC_GPAI_MODE_PERIOD = 1
};

enum aic_gpai_obtain_data_mode {
    AIC_GPAI_OBTAIN_DATA_BY_CPU = 1,
    AIC_GPAI_OBTAIN_DATA_BY_DMA = 2,
    AIC_GPAI_OBTAIN_DATA_BY_POLL = 3,
    AIC_GPAI_OBTAIN_DATA_BY_DO = 4
};

#if defined(AIC_GPAI_DRV_V21)
enum adc_acc_sel_type_t {
	AIC_GPAI_ADC_12BIT = 0,
	AIC_GPAI_ADC_14BIT = 1
};
#endif

typedef void (*dma_callback)(void *dma_param);
typedef void (*irq_callback)(void *cb_param);

struct aic_dma_transfer_info
{
    u32 chan_id;
    struct aic_dma_chan *dma_chan;

    void *buf;
    int buf_size;
    void *callback_param;
    dma_callback callback;
};

struct aic_gpai_irq_info
{
    u32 chan_id;
    void *callback_param;
    irq_callback callback;
};

struct aic_gpai_ch_info
{
    u32 chan_id;
    u16 adc_values[AIC_GPAI_FIFO_MAX_DEPTH];
    u8 fifo_valid_cnt;
    enum aic_gpai_mode mode;
};

struct aic_gpai_ch {
    u8 id;
    u8 available;
    u8 adc_acq;
    enum aic_gpai_mode mode;
    enum aic_gpai_obtain_data_mode obtain_data_mode;
    u32 smp_period;
    u32 pclk_rate;
    u16 avg_data;
    u16 fifo_data[AIC_GPAI_FIFO_MAX_DEPTH];
    u8 fifo_valid_cnt;
    u8 fifo_depth;
    u8 fifo_thd;

    u8 hla_enable; // high-level alarm
    u8 lla_enable; // low-level alarm
    u16 hla_thd;
    u16 hla_rm_thd;
    u16 lla_thd;
    u16 lla_rm_thd;

    u8 irq_count;
    u8 dma_port_id;
    struct aic_dma_transfer_info dma_rx_info;
    struct aic_gpai_irq_info irq_info;

    aicos_sem_t complete;
};

void aich_gpai_enable(int enable);
void aich_gpai_ch_enable(u32 ch, int enable);
int aich_gpai_ch_init(struct aic_gpai_ch *chan, u32 pclk);

irqreturn_t aich_gpai_isr(int irq, void *arg);

int hal_gpai_get_data(struct aic_gpai_ch *chan, u16 *val, u32 timeout);
s32 aich_gpai_data2vol(u16 data);

struct aic_gpai_ch *hal_gpai_ch_is_valid(u32 ch);
void hal_gpai_set_ch_num(u32 num);

void aich_gpai_status_show(struct aic_gpai_ch *chan);
s32 hal_gpai_clk_init(void);
void hal_gpai_clk_get(struct aic_gpai_ch *chan);
#ifdef AIC_GPAI_DRV_DMA
void hal_gpai_config_dma(struct aic_gpai_ch *chan);
void hal_gpai_start_dma(struct aic_gpai_ch *chan);
void hal_gpai_stop_dma(struct aic_gpai_ch *chan);
#endif
#if defined(AIC_GPAI_DRV_V21)
void aich_gpai_adc_sel_enable(int adc_type);
#endif
#ifdef AIC_GPAI_DRV_DO
int hal_gpai_do_set_ch(u8 do_ch, u8 ad_ch);
void hal_gpai_do_enable(u8 do_ch);
#endif
#endif // end of _ARTINCHIP_HAL_GPAI_H_
