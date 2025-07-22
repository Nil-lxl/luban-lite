/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef LV_AIC_VIDEO_WINDOW_H
#define LV_AIC_VIDEO_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
#if LVGL_VERSION_MAJOR == 8
    lv_img_t image;
#else
    lv_image_t image;
#endif
    uint32_t width;
    uint32_t height;
    lv_color_t color;

    uint32_t last_width;
    uint32_t last_height;
    lv_color_t last_color;
} lv_aic_video_window_t;

lv_obj_t * lv_aic_video_window_create(lv_obj_t * parent);

void lv_aic_video_window_set_size(lv_obj_t * obj, uint32_t width, uint32_t height);

void lv_aic_video_window_set_color(lv_obj_t * obj, lv_color_t color);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

