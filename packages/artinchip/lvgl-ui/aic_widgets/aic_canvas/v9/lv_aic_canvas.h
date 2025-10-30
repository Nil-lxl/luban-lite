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

#include "lv_conf_internal.h"
#include "lv_image.h"
#include "lv_draw_image.h"
#include "canvas_image.h"

LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_aic_canvas_class;

typedef struct {
    lv_image_t img;
    lv_draw_buf_t *draw_buf;
    lv_draw_buf_t static_buf;
    struct lv_mpp_buf *img_buf;
} lv_aic_canvas_t;


lv_obj_t * lv_aic_canvas_create(lv_obj_t *parent);


lv_res_t lv_aic_canvas_alloc_buffer(lv_obj_t *obj, lv_coord_t w, lv_coord_t h);


void lv_aic_canvas_draw_text(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                            lv_draw_label_dsc_t *draw_dsc, const char *txt);

void lv_aic_canvas_draw_text_to_center(lv_obj_t *obj, lv_draw_label_dsc_t *label_dsc, const char *txt);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_AIC_CANVAS_H*/
