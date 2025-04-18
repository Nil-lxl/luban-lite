/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef LV_GE2D_H
#define LV_GE2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lv_color.h"
#include "lv_hal_disp.h"
#include "lv_draw_sw.h"

typedef lv_draw_sw_ctx_t lv_draw_aic_ctx_t;

struct _lv_disp_drv_t;

void lv_draw_aic_ctx_init(struct _lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx);

void lv_draw_aic_ctx_deinit(struct _lv_disp_drv_t * drv, lv_draw_ctx_t * draw_ctx);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GE2D_H*/
