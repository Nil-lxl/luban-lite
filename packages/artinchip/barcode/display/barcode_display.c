/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#include <string.h>
#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"

#include "display/barcode_display.h"
#include "config/barcode_config.h"

#ifdef BARCODE_ENABLE_DISPLAY

#include "../include/video_font_data.h"

static void aicfb_lcd_putc(barcode_display_t *disp, unsigned int x, unsigned int y, char ch)
{
    unsigned long dcache_size, dcache_start;
    int pbytes = disp->fb_info.bits_per_pixel / 8;
    int i, row;
    void *line;

    line = (unsigned char *)(disp->fb_info.framebuffer + y * disp->fb_info.stride + x * pbytes);
    dcache_start = ALIGN_DOWN((unsigned long)line, ARCH_DMA_MINALIGN);

    for (row = 0; row < VIDEO_FONT_HEIGHT; row++) {
        unsigned int idx = (ch - 32) * VIDEO_FONT_HEIGHT + row;
        uint32_t bits = video_fontdata[idx];

        uint16_t *dst = line;

        for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
            *dst++ = (bits & 0x80000000) ? 0xFFFF : 0x0000;
            bits <<= 1;
        }

        line += disp->fb_info.stride;
    }

    dcache_size = ALIGN_UP((unsigned long)line - dcache_start, ARCH_DMA_MINALIGN);
    aicos_dcache_clean_range((unsigned long *)dcache_start, dcache_size);
}

#ifdef BARCODE_DISPLAY_ROTATION

static void video_layer_set(barcode_display_t *disp)
{
    struct aicfb_layer_data layer = {0};
    int ret;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    layer.buf.buf_type = MPP_PHY_ADDR;
    layer.buf.size.width = disp->fb_info.width;
    layer.buf.size.height = disp->fb_info.height;
    layer.buf.format = disp->fb_info.format;
    layer.buf.stride[0] = disp->fb_info.stride;
    layer.buf.phy_addr[0] = (u32)(long)disp->ge_out_buffer;

    ret = mpp_fb_ioctl(disp->fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0) {
        pr_err("Display: update_layer_config error, %d", ret);
    }

    mpp_fb_ioctl(disp->fb, AICFB_WAIT_FOR_VSYNC, &layer);
}

static int do_rotate(barcode_display_t *disp,
                     int src_w, int src_h,
                     unsigned long phy_addr_y,
                     unsigned long phy_addr_uv)
{
    struct ge_bitblt blt = {0};
    struct mpp_buf *src = &blt.src_buf;
    struct mpp_buf *dst = &blt.dst_buf;
    int ret;

    // g73 only support YUV400, we only need Y component
    src->format = MPP_FMT_YUV400;
    src->buf_type = MPP_PHY_ADDR;
    src->phy_addr[0] = phy_addr_y;
    src->phy_addr[1] = phy_addr_uv;
    src->stride[0] = src_w;
    src->stride[1] = src_w;
    src->size.width = src_w;
    src->size.height = src_h;

    dst->format = disp->fb_info.format;
    dst->buf_type = MPP_PHY_ADDR;
    dst->phy_addr[0] = (u32)(long)disp->ge_out_buffer;
    dst->stride[0] = disp->fb_info.stride;
    dst->size.width = disp->fb_info.width;
    dst->size.height = disp->fb_info.height;

    blt.ctrl.flags = disp->rotation;

    ret = mpp_ge_bitblt(disp->ge_dev, &blt);
    if (ret < 0) {
        pr_err("Display: GE bitblt failed, ret: %d\n", ret);
        return -1;
    }

    ret = mpp_ge_emit(disp->ge_dev);
    if (ret < 0) {
        pr_err("Display: GE emit failed\n");
        return -1;
    }

    ret = mpp_ge_sync(disp->ge_dev);
    if (ret < 0) {
        pr_err("Display: GE sync failed\n");
        return -1;
    }

    return 0;
}

#endif /* BARCODE_DISPLAY_ROTATION */

int barcode_display_init(barcode_display_t *disp)
{
    int ret;

    if (!disp) {
        return -1;
    }

    memset(disp, 0, sizeof(barcode_display_t));

    disp->fb = mpp_fb_open();
    if (disp->fb < 0) {
        pr_err("Display: mpp_fb_open() failed\n");
        return -1;
    }

    ret = mpp_fb_ioctl(disp->fb, AICFB_GET_SCREENINFO, &disp->fb_info);
    if (ret < 0) {
        pr_err("Display: ioctl() failed! errno: -%d\n", -ret);
        goto error_out;
    }

    memset(disp->fb_info.framebuffer, 0, disp->fb_info.smem_len);
    aicos_dcache_clean_range(disp->fb_info.framebuffer, disp->fb_info.smem_len);

    ret = mpp_fb_ioctl(disp->fb, AICFB_POWERON, &disp->fb_info);
    if (ret < 0) {
        pr_err("Display: ioctl() failed! errno: -%d\n", -ret);
        goto error_out;
    }

#ifdef BARCODE_DISPLAY_ROTATION
    disp->rotation = BARCODE_ROTATION_ANGLE;
    if (disp->rotation) {
        disp->ge_dev = mpp_ge_open();
        if (!disp->ge_dev) {
            goto error_out;
        }
    }
    disp->ge_out_buffer = aicos_malloc_try_cma(disp->fb_info.smem_len);
    if (!disp->ge_out_buffer) {
        pr_err("Display: allocate GE buffer failed\n");
        goto error_out;
    }
    pr_info("Display: Rotate %d by GE, buffer: %p\n", disp->rotation * 90, disp->ge_out_buffer);
#endif

    disp->initialized = true;  /* Mark as fully initialized */
    pr_info("Display: Screen width: %d, height %d\n", disp->fb_info.width, disp->fb_info.height);

    return 0;

error_out:
    /* Partial cleanup: only release resources that were successfully allocated */
    if (disp->fb) {
        mpp_fb_close(disp->fb);
        disp->fb = NULL;
    }
#ifdef BARCODE_DISPLAY_ROTATION
    if (disp->ge_dev) {
        mpp_ge_close(disp->ge_dev);
        disp->ge_dev = NULL;
    }
    if (disp->ge_out_buffer) {
        aicos_free(MEM_CMA, disp->ge_out_buffer);
        disp->ge_out_buffer = NULL;
    }
#endif
    return -1;
}

void barcode_display_deinit(barcode_display_t *disp)
{
    if (!disp || !disp->initialized) {
        return;
    }

    if (disp->fb) {
        mpp_fb_close(disp->fb);
        disp->fb = NULL;
    }

#ifdef BARCODE_DISPLAY_ROTATION
    if (disp->ge_dev) {
        mpp_ge_close(disp->ge_dev);
        disp->ge_dev = NULL;
    }
    if (disp->ge_out_buffer) {
        aicos_free(MEM_CMA, disp->ge_out_buffer);
        disp->ge_out_buffer = NULL;
    }
#endif

    disp->initialized = false;
}

int barcode_display_update_video(barcode_display_t *disp,
                                  int width, int height,
                                  unsigned long phy_addr_y,
                                  unsigned long phy_addr_uv)
{
    if (!disp || !disp->initialized) {
        return -1;
    }

#ifdef BARCODE_DISPLAY_ROTATION
    if (do_rotate(disp, width, height, phy_addr_y, phy_addr_uv) < 0) {
        return -1;
    }
    video_layer_set(disp);
#else
    struct aicfb_layer_data layer = {0};
    int i;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    layer.scale_size.width = disp->fb_info.width - BARCODE_VID_SCALE_OFFSET * 2;
    layer.scale_size.height = disp->fb_info.height - BARCODE_VID_SCALE_OFFSET * 2;
    layer.pos.x = BARCODE_VID_SCALE_OFFSET;
    layer.pos.y = BARCODE_VID_SCALE_OFFSET;

    layer.buf.size.width = width;
    layer.buf.size.height = height;
    layer.buf.format = MPP_FMT_NV12;
    layer.buf.buf_type = MPP_PHY_ADDR;

    for (i = 0; i < BARCODE_VIDEO_BUF_PLANE_NUM; i++) {
        layer.buf.stride[i] = layer.buf.size.width;
        layer.buf.phy_addr[i] = (i == 0) ? phy_addr_y : phy_addr_uv;
    }

    if (mpp_fb_ioctl(disp->fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0) {
        pr_err("Display: ioctl() failed!\n");
        return -1;
    }
#endif

    return 0;
}

int barcode_display_show_text(barcode_display_t *disp,
                              const char *text, int x, int y)
{
    int len;

    if (!disp || !disp->initialized || !text) {
        return -1;
    }

    len = strlen(text);
    for (int i = 0; i < len; i++) {
        aicfb_lcd_putc(disp, x + (i * BARCODE_FONT_WIDTH), y, text[i]);
    }

    return 0;
}

int barcode_display_set_ui_alpha(barcode_display_t *disp, int alpha)
{
    int ret;
    struct aicfb_alpha_config alpha_cfg = {0};

    if (!disp || !disp->initialized) {
        return -1;
    }

    alpha_cfg.layer_id = AICFB_LAYER_TYPE_UI;
    alpha_cfg.enable = 1;
    alpha_cfg.mode = 1;
    alpha_cfg.value = alpha;

    ret = mpp_fb_ioctl(disp->fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha_cfg);
    if (ret < 0) {
        pr_err("Display: ioctl() failed! errno: -%d\n", -ret);
        return -1;
    }

    return 0;
}

#endif /* BARCODE_ENABLE_DISPLAY */
