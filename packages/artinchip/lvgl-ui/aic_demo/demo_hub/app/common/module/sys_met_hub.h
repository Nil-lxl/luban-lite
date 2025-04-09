/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _LV_AIC_EXTRA_SYS_MET_HUB_H
#define _LV_AIC_EXTRA_SYS_MET_HUB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"


typedef struct _sys_met_hub {
    int draw_fps;
    int cpu_usage; /* unit:  % */
    int mem_usage; /* unit: MB */
} sys_met_hub;


sys_met_hub* sys_met_hub_create(void);

int sys_met_hub_get_info(sys_met_hub* info);

void sys_met_hub_delete(sys_met_hub* info);

#endif
#ifdef __cplusplus
} /*extern "C"*/
#endif

