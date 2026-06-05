/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_DRV_MTOP_H_
#define _AIC_DRV_MTOP_H_

#include "aic_hal_clk.h"
#include "aic_hal_mtop.h"

enum {
    MTOP_SET_PERIOD_MODE,
    MTOP_ENABLE,
};

struct mtop_dev {
    struct rt_device dev;
    struct aic_mtop_dev mtop_handle;
    char *name;
#ifdef RT_USING_PM
    u8 mtop_clk_pm_flag;
#endif
};

#endif
