/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_PBUS_H__
#define _ARTINCHIP_HAL_PBUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_common.h"

#define AIC_PBUS_SIZE   (64 * 1024)

enum aic_pbus_pol {
    AIC_PBUS_POL_LOW_ACTIVE = 0,
    AIC_PBUS_POL_HIGH_ACTIVE = 1,
};

enum aic_pbus_edge {
    AIC_PBUS_POL_FALL_EDGE = 0,
    AIC_PBUS_POL_RISE_EDGE = 1,
};

enum aic_pbus_clk_div {
    AIC_PBUS_CLK_DIV_UNDEF = 0,
    AIC_PBUS_CLK_DIV_2,
    AIC_PBUS_CLK_DIV_4,
    AIC_PBUS_CLK_DIV_8
};

struct aic_pbus_timing {
    u16 holdtime;
    u16 delaytime;
};

struct aic_pbus_cfg {
    bool                    bus_clk_enable;
    enum aic_pbus_edge      bus_clk_pol;
    enum aic_pbus_clk_div   bus_clk_div;

    enum aic_pbus_pol       out_enable_pol;
    enum aic_pbus_pol       wr_enable_pol;
    enum aic_pbus_pol       addr_valid_pol;
    enum aic_pbus_pol       cs_pol;

    struct aic_pbus_timing  out_enable;
    struct aic_pbus_timing  wr_data;
    struct aic_pbus_timing  wr_rd;
    struct aic_pbus_timing  addr_valid;
    struct aic_pbus_timing  addr;
    struct aic_pbus_timing  cs;
};

int hal_pbus_init(void);
int hal_pbus_deinit(void);
int hal_pbus_get_cfg(struct aic_pbus_cfg *cfg);
int hal_pbus_set_cfg(struct aic_pbus_cfg *cfg);
int hal_pbus_read(u32 offset, u8 *buf, u32 len);
int hal_pbus_write(u32 offset, u8 *buf, u32 len);

#ifdef __cplusplus
}
#endif

#endif
