/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#ifndef _XPWM_H_
#define _XPWM_H_

#include "hal_xpwm.h"

int drv_xpwm_enable(u32 ch, bool enable);
int drv_xpwm_set(u32 ch, u32 period_ns, u32 duty_ns, u32 pulse_cnt);
int drv_xpwm_get(u32 ch, u32 *duty_ns, u32 *period_ns);
int drv_xpwm_set_fifo_num(u32 ch, u32 fifo_num);
int drv_xpwm_set_fifo(u32 ch, struct aic_xpwm_fifo fifo_info);
#ifdef AIC_USING_DMA
int drv_xpwm_dma_set_fifo(u32 ch, struct aic_xpwm_buf_info dma_info, void *second_cb, void *cb_para);
#endif
int drv_xpwm_get_fifo(u32 ch);
int drv_xpwm_init(void);

#endif // end of _XPWM_H_
