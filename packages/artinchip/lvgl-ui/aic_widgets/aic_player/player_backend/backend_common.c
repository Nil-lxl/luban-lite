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

#include "frame_allocator.h"
#if __linux__
#include <linux/dma-buf.h>
#include <linux/fb.h>
#include <video/artinchip_fb.h>
#endif

void backend_draw_color_buffer(lv_image_dsc_t *image_dst, uint32_t color)
{
    int ret = 0;
    struct mpp_ge *ge = NULL;
    struct ge_fillrect fill = {0};

    fill.dst_buf.buf_type = MPP_PHY_ADDR;
#if LVGL_VERSION_MAJOR == 8
    struct mpp_buf *buf = (struct mpp_buf *)image_dst->data;
    memcpy(&fill.dst_buf, buf, sizeof(struct mpp_buf));
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

#if LVGL_VERSION_MAJOR == 9
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
#if LVGL_VERSION_MAJOR == 9
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
#if __linux__
    struct aicfb_screeninfo *screen_info = (struct aicfb_screeninfo *)info;

    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        LV_LOG_ERROR("open %s", "/dev/fb0");
        return;
    }

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        LV_LOG_ERROR("ioctl FBIOGET_FSCREENINFO");
        close(fb_fd);
        return;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0) {
        LV_LOG_ERROR("ioctl FBIOGET_VSCREENINFO");
        close(fb_fd);
        return;
    }

    screen_info.width = var.xres;
    screen_info.height = var.yres;
    screen_info.smem_len = fix.smem_len;
    screen_info.stride = fix.line_length;

    close(fb_fd);
#else
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
#endif
}

int backend_dma_buf_alloc(backend_cma_buf_t *buf, uint32_t size)
{
    if (buf == NULL)
        return -1;

    buf->fd = -1;
    buf->phy_addr = 0;
    buf->size = size;
    buf->data = NULL;
#if __linux__
    int dma_dev_fd = dmabuf_device_open();
    if (dma_dev_fd < 0) {
        LV_LOG_ERROR("open dmabuf device failed", (int)size);
        return -1;
    }

    buf->fd = dmabuf_alloc(dma_dev_fd, size);
    if (buf->fd < 0) {
        LV_LOG_ERROR("malloc cma buffer failed, size = %d", (int)size);
        return -1;
    }

    int ret = ioctl(buf->fd, DMA_BUF_IOCTL_GET_PHY_ADDR, &buf->phy_addr);
    if (ret < 0) {
        goto alloc_error;
    }

    buf->data = dmabuf_mmap(buf->dma_fd, size);
    if (buf->data == MAP_FAILED) {
        goto alloc_error;
    }

    dmabuf_device_close(dma_dev_fd);
    return 0;

alloc_error:
    if (dma_fd > 0) {
        dmabuf_device_close(dma_fd);
    }
    if (buf->data != MAP_FAILED && buf->data) {
        dmabuf_munmap((unsigned char*)buf->phy_addr[0], size);
        buf->data = NULL;
        dmabuf_free(buf->fd[0]);
    }

    return -1;
#else
    buf->data = aicos_malloc_try_cma(size);
    if (!buf->data) {
        LV_LOG_ERROR("malloc cma buffer failed, size = %d", (int)size);
        return -1;
    }
    buf->size = size;
    buf->phy_addr = (uint32_t)(uintptr_t)(buf->data);
    return 0;
#endif
}

void backend_dma_buf_free(backend_cma_buf_t *buf)
{
    if (buf == NULL)
        return;
#if __linux__
    if (buf->data != MAP_FAILED && buf->data) {
        dmabuf_munmap((unsigned char*)buf->phy_addr, buf->size);
    }
    if (buf->fd >= 0) {
        dmabuf_free(buf->fd);
    }
#else
    if (buf->phy_addr)
        aicos_free(MEM_CMA, (void*)(unsigned long)buf->phy_addr);
#endif
}
#endif
