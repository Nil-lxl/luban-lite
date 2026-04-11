/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef CANVAS_IMAGE_H
#define CANVAS_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"
#include "aic_core.h"
#include "mpp_ge.h"

struct lv_mpp_buf
{
    struct mpp_buf buf;
    unsigned char *data;
    int size;
};

struct lv_mpp_buf *lv_mpp_image_alloc(int width, int height, enum mpp_pixel_format fmt);

void lv_mpp_image_flush_cache(struct lv_mpp_buf *image);

void lv_mpp_image_free(struct lv_mpp_buf *image);

int lv_ge_fill(struct mpp_buf *buf, enum ge_fillrect_type type,
               unsigned int start_color,  unsigned int end_color, int blend);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //CANVAS_IMAGE_H
