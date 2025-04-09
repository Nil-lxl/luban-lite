/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "aic_core.h"
#include "mpp_ge.h"

#define ALIGN_8B(x) (((x) + (7)) & ~7)
#define ALIGN_1024B(x) ((x+1023) & (~1023))

struct lv_mpp_buf
{
    struct mpp_buf buf;
    unsigned char *data;
    int size;
};

struct lv_mpp_buf *lv_mpp_image_alloc(int width, int height, enum mpp_pixel_format fmt)
{
    struct lv_mpp_buf *image;

    int size = 0;
    image = (struct lv_mpp_buf *)aicos_malloc(MEM_DEFAULT, sizeof(struct lv_mpp_buf));
    if (!image) {
        LV_LOG_ERROR("aicos_malloc failed");
        return NULL;
    }

    memset(image, 0, sizeof(struct lv_mpp_buf));
    if (fmt == MPP_FMT_ARGB_8888) {
        image->buf.stride[0] = ALIGN_8B(width * 4);
    } else if (fmt == MPP_FMT_RGB_565) {
        image->buf.stride[0] = ALIGN_8B(width * 2);
    } else {
        LV_LOG_ERROR("unsupport fmt:%d", fmt);
        aicos_free(MEM_DEFAULT, image);
        return NULL;
    }

    size = image->buf.stride[0] * height;
    image->size = size;
    image->buf.buf_type = MPP_PHY_ADDR;
    image->buf.size.width = width;
    image->buf.size.height = height;
    image->buf.format = fmt;
    image->data = (unsigned char *)aicos_malloc(MEM_CMA, size + 1023);
    if (!image->data) {
        aicos_free(MEM_DEFAULT, image);
        return NULL;
    } else {
        image->buf.phy_addr[0] = (unsigned int)ALIGN_1024B(((unsigned long)image->data));
        aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)image->buf.phy_addr[0]), size);
    }

    return image;
}

void lv_mpp_image_flush_cache(struct lv_mpp_buf *image)
{
    aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)image->buf.phy_addr[0]), image->size);
    return;
}

void lv_mpp_image_free(struct lv_mpp_buf *image)
{
    if (image) {
        if (image->data)
            aicos_free(MEM_CMA, image->data);

        aicos_free(MEM_DEFAULT, image);
    }

    return;
}

static struct mpp_ge *g_ge = NULL;

int lv_ge_fill(struct mpp_buf *buf, enum ge_fillrect_type type,
        unsigned int start_color,  unsigned int end_color, int blend)
{
    int ret;
    struct ge_fillrect fill = { 0 };

    if (!g_ge)
        g_ge = mpp_ge_open();

    /* fill info */
    fill.type = type;
    fill.start_color = start_color;
    fill.end_color = end_color;

    /* dst buf */
    memcpy(&fill.dst_buf, buf, sizeof(struct mpp_buf));

    /* ctrl */
    fill.ctrl.flags = 0;
    fill.ctrl.alpha_en = blend;

    ret =  mpp_ge_fillrect(g_ge, &fill);
    if (ret < 0) {
        LV_LOG_WARN("fillrect1 fail\n");
        return LV_RES_INV;
    }
    ret = mpp_ge_emit(g_ge);
    if (ret < 0) {
        LV_LOG_WARN("emit fail\n");
        return LV_RES_INV;
    }
    ret = mpp_ge_sync(g_ge);
    if (ret < 0) {
        LV_LOG_WARN("sync fail\n");
        return LV_RES_INV;
    }

    return LV_RES_OK;
}
