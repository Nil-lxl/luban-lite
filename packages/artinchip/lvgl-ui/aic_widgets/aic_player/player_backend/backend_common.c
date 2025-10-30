/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "backend_common.h"
#include "aic_ui.h"

#if LV_USE_AIC_SIMULATOR == 0
#include "mpp_fb.h"
#include "mpp_ge.h"
#include "mpp_mem.h"

void backend_draw_color_buffer(lv_image_dsc_t *image_dst, uint32_t color)
{
    int ret = 0;
    struct mpp_ge *ge = NULL;
    struct ge_fillrect fill = {0};

    fill.dst_buf.buf_type = MPP_PHY_ADDR;
#if LVGL_VERSION_MAJOR == 8
    int width = image_dst->header.w;
    int screen_format = backend_fmt_lv_to_mpp(image_dst->header.cf);
    int stride = backend_align_stride(width, screen_format);

    fill.dst_buf.phy_addr[0] = (unsigned int)(uintptr_t)image_dst->data;
    fill.dst_buf.stride[0] = stride;
    fill.dst_buf.size.width = image_dst->header.w;
    fill.dst_buf.size.height = image_dst->header.h;
    fill.dst_buf.format = backend_fmt_lv_to_mpp(image_dst->header.cf);
#elif LVGL_VERSION_MAJOR == 9
    fill.dst_buf.phy_addr[0] = (unsigned int)(uintptr_t)image_dst->data;
    fill.dst_buf.stride[0] = image_dst->header.stride;
    fill.dst_buf.size.width = image_dst->header.w;
    fill.dst_buf.size.height = image_dst->header.h;
    fill.dst_buf.format = backend_fmt_lv_to_mpp(image_dst->header.cf);
#endif

    fill.start_color = color;
    fill.end_color = 0;

    ge = mpp_ge_open();
    if (!ge) {
        LV_LOG_ERROR("ge open fail\n");
        return;
    }

    ret = mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        LV_LOG_ERROR("ge fillrect fail\n");
        goto draw_color_err;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        LV_LOG_ERROR("ge emit fail\n");
        goto draw_color_err;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        LV_LOG_ERROR("ge sync fail\n");
        goto draw_color_err;
    }

draw_color_err:
    mpp_ge_close(ge);
}

uint32_t backend_fmt_mpp_to_lv(int cf)
{
    uint32_t fmt = 0;

#if LVGL_VERSION_MAJOR == 8
    switch(cf) {
        case MPP_FMT_RGB_565:
            fmt = LV_IMG_CF_TRUE_COLOR;
            break;
        case MPP_FMT_ARGB_8888:
            fmt = LV_IMG_CF_TRUE_COLOR_ALPHA;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", (int)cf);
            break;
    }

#elif LVGL_VERSION_MAJOR == 9
    switch(cf) {
        case MPP_FMT_RGB_565:
            fmt = LV_COLOR_FORMAT_RGB565;
            break;
        case MPP_FMT_RGB_888:
            fmt = LV_COLOR_FORMAT_RGB888;
            break;
        case MPP_FMT_ARGB_8888:
            fmt = LV_COLOR_FORMAT_ARGB8888;
            break;
        case MPP_FMT_XRGB_8888:
            fmt = LV_COLOR_FORMAT_XRGB8888;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }
#endif
    return fmt;
}

int backend_fmt_lv_to_mpp(uint32_t cf)
{
    enum mpp_pixel_format fmt = MPP_FMT_ARGB_8888;
#if LVGL_VERSION_MAJOR == 8
    switch(cf) {
        case LV_IMG_CF_TRUE_COLOR:
            fmt = MPP_FMT_RGB_565;
            break;
        case LV_IMG_CF_TRUE_COLOR_ALPHA:
            fmt = MPP_FMT_ARGB_8888;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", (int)cf);
            break;
    }

#elif LVGL_VERSION_MAJOR == 9
    switch(cf) {
    case LV_COLOR_FORMAT_RGB565:
        fmt = MPP_FMT_RGB_565;
        break;
    case LV_COLOR_FORMAT_RGB888:
        fmt = MPP_FMT_RGB_888;
        break;
    case LV_COLOR_FORMAT_ARGB8888:
        fmt = MPP_FMT_ARGB_8888;
        break;
    case LV_COLOR_FORMAT_XRGB8888:
        fmt = MPP_FMT_XRGB_8888;
        break;
    default:
        LV_LOG_ERROR("unsupported format:%d", (int)cf);
        break;
    }
#endif
    return fmt;
}

int backend_align_stride(int width, int fmt)
{
    int stride;

    switch(fmt) {
        case MPP_FMT_RGB_565:
            stride = ALIGN_UP((width) * 2, 32);
            break;
        case MPP_FMT_RGB_888:
            stride = ALIGN_UP((width) * 3, 48);
            break;
        case MPP_FMT_ARGB_8888:
            stride = ALIGN_UP((width) * 4, 64);
            break;
        case MPP_FMT_XRGB_8888:
            stride = ALIGN_UP((width) * 4, 64);
            break;
        default:
            stride = ALIGN_UP((width) * 4, 16);
            LV_LOG_ERROR("unsupported format:%d", fmt);
            break;
    }
    return stride;
}

int backend_get_screen_info(void *info)
{
    struct aicfb_screeninfo *screen_info = (struct aicfb_screeninfo *)info;
    struct mpp_fb *fb = mpp_fb_open();
    if (!fb) {
        LV_LOG_ERROR("open fb failed");
        return -1;
    }

    int ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, screen_info);
    if (ret) {
        LV_LOG_ERROR("get screen info failed");
        return -1;
    }

    mpp_fb_close(fb);
    return 0;
}
#endif
