/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef AIC_DEC_H
#define AIC_DEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rtconfig.h>
#include <stdint.h>
#include <mpp_list.h>
#include "lvgl.h"

#ifndef MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE
#define MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE
#endif

#ifndef MPP_JPEG_DEC_MAX_OUT_WIDTH
#define MPP_JPEG_DEC_MAX_OUT_WIDTH  2048
#endif

#ifndef MPP_JPEG_DEC_MAX_OUT_HEIGHT
#define MPP_JPEG_DEC_MAX_OUT_HEIGHT 2048
#endif

static inline int jpeg_width_limit(int width)
{
    int r = 0;

    while(width > MPP_JPEG_DEC_MAX_OUT_WIDTH) {
        width = width >> 1;
        r++;

        if (r == 3) {
            break;
        }
    }
    return r;
}

static inline int jpeg_height_limit(int height)
{
    int r = 0;

    while(height > MPP_JPEG_DEC_MAX_OUT_HEIGHT) {
        height = height >> 1;
        r++;

        if (r == 3) {
            break;
        }
    }
    return r;
}

static inline int jpeg_size_limit(int w, int h)
{
    return LV_MAX(jpeg_width_limit(w), jpeg_height_limit(h));
}

struct framebuf_info {
    unsigned int addr;
    unsigned int align_addr;
    int size;
};

struct framebuf_head {
	struct mpp_list list;
	struct framebuf_info buf_info;
};

void aic_dec_create();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //AIC_DEC_H
