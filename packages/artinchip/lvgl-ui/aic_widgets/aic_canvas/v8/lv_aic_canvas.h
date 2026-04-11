/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_AIC_CANVAS_H
#define LV_AIC_CANVAS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lv_obj.h"
#include "lv_img.h"
#include "lv_draw_img.h"
#include "canvas_image.h"

extern const lv_obj_class_t lv_aic_canvas_class;

typedef struct {
    lv_img_t img;
    lv_img_dsc_t dsc;
    struct lv_mpp_buf *img_buf;
} lv_aic_canvas_t;

lv_obj_t *lv_aic_canvas_create(lv_obj_t *parent);

lv_res_t lv_aic_canvas_alloc_buffer(lv_obj_t *obj, lv_coord_t w, lv_coord_t h);

void lv_aic_canvas_draw_text(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                             lv_draw_label_dsc_t * draw_dsc, const char *txt);

void lv_aic_canvas_draw_text_to_center(lv_obj_t *obj, lv_draw_label_dsc_t *label_dsc, const char *txt);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_AIC_CANVAS_H*/
