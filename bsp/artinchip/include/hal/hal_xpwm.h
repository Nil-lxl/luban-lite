/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_XPWM_H_
#define _ARTINCHIP_HAL_XPWM_H_

#include "aic_osal.h"
#include "aic_common.h"
#include "hal_dma.h"
#include "aic_dma_id.h"

/* XPWM_STS_FLAG */
#define CNT_ZRO_FLG_SHIFT              0
#define CNT_PRD_FLG_SHIFT              1
#define REG_UPDT_FLG_SHIFT             2
#define FIFO_AVAL_FLG_SHIFT            3
#define FIFO_UDFL_FLG_SHIFT            4
#define FIFO_OVFL_FLG_SHIFT            5
#define XPWM_FRC_PUL_FLG_SHIFT         6
#define XPWM_PUL_LIMIT_FLG_SHIFT       7
#define CNT_CMP_FLG_SHIFT              8

#define XPWM_FIFO_MIN                  1

#ifdef FPGA_BOARD_ARTINCHIP
#define XPWM_CLK_RATE                  48000000 /* 48 MHz */
#define XPWM_TB_CLK_RATE               48000000 /* 48 MHz */
#else
#define XPWM_CLK_RATE                  50000000 /* 50 MHz */
#define XPWM_TB_CLK_RATE               50000000 /* 50 MHz */
#endif

enum xpwm_work_mode {
    XPWM_PWM_MODE,
    XPWM_NORMAL_MODE,
    XPWM_REG_UPDT_MODE,
    XPWM_FIFO_NORMAL_MODE,
    XPWM_FIFO_DMA_MODE,
};

enum xpwm_polarity {
    XPWM_POLARITY_NORMAL,
    XPWM_POLARITY_INVERSED,
};

struct xpwm_int_s {
    u32 cnt_zro_int_en:1;
    u32 cnt_prd_int_en:1;
    u32 reg_updt_int_en:1;
    u32 fifo_aval_int_en:1;
    u32 fifo_udfl_int_en:1;
    u32 fifo_ovfl_int_en:1;
    u32 frc_pul_int_en:1;
    u32 pul_limit_int_en:1;
    u32 cnt_cmp_int_en:1;
};

struct aic_xpwm_fifo {
    u32 ch;
    u32 fifo_num;
    u32 fifo_index;
    u32 pul_num[XPWM_FIFO_MAX];
    u32 pul_prd[XPWM_FIFO_MAX];
    u32 pul_cmp[XPWM_FIFO_MAX];
};

struct aic_xpwm_updt {
    u32 ch;
    u32 pul_num;
    u32 pul_prd;
    u32 pul_cmp;
};

struct aic_xpwm_buf_info
{
    u32 *buf;
    u32 buf_len;
};

struct aic_xpwm_transfer_info
{
    struct aic_dma_chan *dma_chan;
    struct aic_xpwm_buf_info buf_info;
};

struct aic_xpwm_arg {
    u8 available;
    u8 id;
    u8 xpwm_mode;
    u32 dma_id;
    u32 base;
    int irq;
    int clk;
    int set_clk_rate;
    int act_clk_rate;
    u32 tb_clk_rate;
    float freq;
    u32 period;
    u32 duty;
    u32 pulse_cnt;
    s32 def_level;
    u32 dma_en;
    u32 fifo_en;
    u32 fifo_th;
    u32 pul_limit_en;
    enum xpwm_polarity polarity;
    struct xpwm_int_s xpwm_int;
    struct aic_xpwm_transfer_info t_info;
};

extern struct aic_xpwm_arg g_xpwm_args[AIC_XPWM_CH_NUM];

void hal_xpwm_ch_init(struct aic_xpwm_arg *data);
int hal_xpwm_pul_reset(u32 ch);
int hal_xpwm_set(u32 ch, u32 duty_ns, u32 period_ns, u32 pulse_cnt);
int hal_xpwm_set_direct(u32 ch, u32 cmp, u32 prd, u32 pulse_cnt);
void hal_xpwm_imd_update(u32 ch, u32 enable);
int hal_xpwm_get(u32 ch, u32 *duty_ns, u32 *period_ns);
int hal_xpwm_enable(u32 ch);
int hal_xpwm_disable(u32 ch);
u32 hal_xpwm_int_stat(u32 ch);
void hal_xpwm_int_clr(u32 ch, u32 mask);
int hal_xpwm_set_fifo(u32 ch, u32 pul_num, u32 pul_prd, u32 pul_cmp);
void hal_xpwm_cnt_en(u32 ch, u32 val);
int hal_xpwm_is_enable(u32 ch);
void hal_xpwm_resume(u32 ch);
void hal_xpwm_fifo_flush(u32 ch);
void hal_xpwm_irq_en_set(u32 ch, struct xpwm_int_s *xpwm_int);
bool hal_ch_is_xpwm(u32 ch);
int xpwm_cal_prd_duty(u32 ch, u32 duty_ns, u32 period_ns);
int hal_xpwm_voltage_set(u32 ch, u32 vol);
#ifdef AIC_USING_DMA
int hal_xpwm_dma_config(u32 ch, dma_async_callback callback, void *callback_param);
#endif
#endif // end of _ARTINCHIP_HAL_XPWM_H_
