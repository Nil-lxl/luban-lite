/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_PWMCS_H_
#define _ARTINCHIP_HAL_PWMCS_H_

#include "aic_common.h"

#ifdef CONFIG_FPGA_BOARD_ARTINCHIP
#define QEP_CLK_RATE      24000000 /* 24 MHz */
#else
#define QEP_CLK_RATE      200000000 /* 200 MHz */
#endif

#define QEP_POS_CMP_INT_FLG      BIT(8)


enum aic_qep_cnt_mode {
    QUADRATURE_COUNT = 0,
    DIR_COUNT,
    INCREMENTAL_COUNT,
    DECREMENT_COUNT,
    CW_CCW_HIGH_COUNT,
    CW_CCW_LOW_COUNT
};

enum aic_qep_icap_mode {
    RESERVED = 0,
    QEP_IDX_RISING,
    QEP_IDS_FALLING,
    QEP_IDX_MKR
};

struct aic_qep_data {
    u8 id;
};

u32 hal_qep_get_cnt(u32 ch);
u32 hal_qep_int_stat(u32 ch);
void hal_qep_clr_int(u32 ch, u32 stat);
void hal_qep_set_cnt_ep(u32 i, u32 cnt);
void hal_qep_set_cnt_cmp(u32 i, u32 cnt);
void hal_qep_config(u32 ch);
void hal_qep_int_enable(u32 ch, u32 enable);
void hal_qep_enable(u32 ch, u32 enable);
int hal_qep_init(void);
int hal_qep_deinit(void);

#endif // end of _ARTINCHIP_HAL_PWMCS_H_
