/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <aic_core.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_tpc_run.h"
#include "lv_draw_ge2d.h"
#include "lv_mpp_dec.h"

#if LV_DISP_FB_DUMP
static char g_save_path[128] = { 0 };
static bool g_dump_flag = false;

static inline void lv_disp_buf_dump(lv_display_t *disp)
{
    if (g_dump_flag) {
        g_dump_flag = false;
        lv_fs_res_t res = LV_FS_RES_OK;
        lv_color_format_t cf = lv_display_get_color_format(disp);
        uint32_t h = lv_display_get_vertical_resolution(disp);
        uint32_t w = lv_display_get_horizontal_resolution(disp);
        uint32_t stride = lv_draw_buf_width_to_stride(w, cf);
        lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);
        lv_fs_file_t file_save;
        uint32_t data_write = 0;
        char src[128];

        snprintf(src, 127, "L:%s", g_save_path);
        res = lv_fs_open(&file_save, src, LV_FS_MODE_WR);
        if(res != LV_FS_RES_OK) {
            LV_LOG_ERROR("open %s failed", src);
            return;
        }
        lv_fs_write(&file_save, disp_buf->data, h * stride, &data_write);
        lv_fs_close(&file_save);
        printf("save %s ok\n", g_save_path);
    }
}

static void lv_fb_dump(int argc, char **argv)
{
    int ret;
    const char sopts[] = "u:o:";
    const struct option lopts[] = {
        {"usage",   no_argument, NULL, 'u'},
        {"output",  required_argument, NULL, 'o'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'o':
            strcpy(g_save_path, optarg);
            break;
        case 'u':
            printf("Usage: %s [Options]: \n", argv[0]);
            printf("\t-o, --output  image path\n");
            printf("\tfor example:\n");
            printf("\tlv_fb_dump -o /sdcard/out.bin\n");
            return;
        default:
            LV_LOG_ERROR("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    if (strlen(g_save_path)) {
        g_dump_flag = true;
    } else {
        printf("please check output path:%s\n", g_save_path);
    }
    return;
}

MSH_CMD_EXPORT_ALIAS(lv_fb_dump, lv_dump, lvgl dump display buffer);
#endif

static void display_cal_frame_rate(aic_disp_t *aic_disp)
{
#if DISP_SHOW_FPS == 1
    static int start_cal = 0;
    static int frame_cnt = 0;
    static u64 start_us, end_us;
    int interval = 1000 * 1000;
    int time_diff = 0;

    if (start_cal == 0) {
        start_cal = 1;
        start_us = (long)aic_get_time_us();
    }

    end_us = (long)aic_get_time_us();
    time_diff = (int)(end_us - start_us);
    if (time_diff >= interval) {
        aic_disp->fps = frame_cnt;
        frame_cnt = 0;
        start_cal = 0;
    } else {
        frame_cnt++;
    }
#else
    (void)aic_disp;
#endif
    return;
}

static inline void *get_fb_buf_by_id(aic_disp_t *aic_disp, int id)
{
    if (id == 1)
        return (void *)(aic_disp->info.framebuffer + aic_disp->fb_size);
    else
        return (void *)aic_disp->info.framebuffer;
}

#if LV_USE_OS && defined(AIC_PAN_DISPLAY)
static inline void wait_sync_ready(aic_disp_t *aic_disp)
{
    while (aic_disp->sync_ready == false) {
        if (aic_disp->exit_status)
            break;

        lv_thread_sync_wait(&aic_disp->sync_notify);
    }
}
#endif

static inline void disp_do_blit(aic_disp_t *aic_disp, lv_display_t *disp, lv_draw_buf_t *disp_buf)
{
    void *dest_buf = get_fb_buf_by_id(aic_disp, aic_disp->buf_id);
    int32_t hor_res = lv_display_get_horizontal_resolution(disp);
    int32_t ver_res = lv_display_get_vertical_resolution(disp);
    lv_color_format_t cf = lv_display_get_color_format(disp);
    int32_t src_stride = lv_draw_buf_width_to_stride(hor_res, cf);
    int32_t dst_stride = (int32_t)aic_disp->info.stride;
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

#if LV_USE_DRAW_GE2D
    lv_draw_ge2d_rotate(disp_buf->data, dest_buf, hor_res, ver_res,
                        src_stride, dst_stride, rotation, cf);
#else
    lv_draw_sw_rotate(disp_buf->data, dest_buf, hor_res, ver_res,
                      src_stride, dst_stride, rotation, cf);
#endif
}

static inline void display_power_on(aic_disp_t *aic_disp)
{
#ifndef AIC_DISP_COLOR_BLOCK
    static bool first_frame = true;
    if (first_frame) {
        mpp_fb_ioctl(aic_disp->fb, AICFB_POWERON, 0);
        first_frame = false;
    }
#else
    (void)aic_disp;
#endif
}

static inline void disp_draw_buf_flush(aic_disp_t *aic_disp, lv_display_t *disp, lv_draw_buf_t *disp_buf)
{
#if LV_USE_OS && defined(AIC_PAN_DISPLAY)
    wait_sync_ready(aic_disp);
    aic_disp->sync_ready = false;
    disp_do_blit(aic_disp, disp, disp_buf);
    mpp_fb_ioctl(aic_disp->fb, AICFB_PAN_DISPLAY , &aic_disp->buf_id);

    display_power_on(aic_disp);
    aic_disp->flush_act = true;
    lv_thread_sync_signal(&aic_disp->sync);
#else
#ifdef AIC_PAN_DISPLAY
    disp_do_blit(aic_disp, disp, disp_buf);
    mpp_fb_ioctl(aic_disp->fb, AICFB_PAN_DISPLAY , &aic_disp->buf_id);

    display_power_on(aic_disp);
    mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
#else
    display_power_on(aic_disp);
    mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
    disp_do_blit(aic_disp, disp, disp_buf);
#endif // AIC_PAN_DISPLAY
#endif
}

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    aic_disp_t *aic_disp = (aic_disp_t *)lv_display_get_user_data(disp);

    lv_draw_buf_t *disp_buf = lv_display_get_buf_active(disp);
    (void)px_map;

    if (lv_disp_flush_is_last(disp)) {
#if LV_DISP_FB_DUMP
        // dump frame buffer
        lv_disp_buf_dump(disp);
#endif
        if (disp_buf->data == aic_disp->buf_aligned) {
#ifdef AIC_PAN_DISPLAY
            aic_disp->buf_id  =  aic_disp->buf_id > 0 ? 0 : 1;
#else
            aic_disp->buf_id = 0;
#endif
            // flush cache
            aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(aic_disp->buf_size, CACHE_LINE_SIZE));
            disp_draw_buf_flush(aic_disp, disp, disp_buf);
        } else {
            // flush cache
            aicos_dcache_clean_invalid_range((ulong *)disp_buf->data, (ulong)ALIGN_UP(aic_disp->fb_size, CACHE_LINE_SIZE));
#ifdef AIC_PAN_DISPLAY
            int index;
            if(disp_buf->data == aic_disp->info.framebuffer) {
                index = 0;
            } else {
                index = 1;
            }

            // send framebuffer to display
            mpp_fb_ioctl(aic_disp->fb, AICFB_PAN_DISPLAY , &index);
            display_power_on(aic_disp);
            // wait vsync
            mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
#endif
        }

        display_cal_frame_rate(aic_disp);
    }

    lv_display_flush_ready(disp);
}

static lv_color_format_t lv_display_fmt(enum mpp_pixel_format cf)
{
    lv_color_format_t fmt = LV_COLOR_FORMAT_ARGB8888;
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
    return fmt;
}

#if LV_USE_OS && defined(AIC_PAN_DISPLAY) && defined(LV_DISPLAY_ROTATE_EN)
static void aic_display_thread(void *ptr)
{
    aic_disp_t *aic_disp = (aic_disp_t *)ptr;

    while(1) {
        while (aic_disp->flush_act == false) {
            if(aic_disp->exit_status)
                break;

            lv_thread_sync_wait(&aic_disp->sync);
        }

        aic_disp->flush_act = false;
        mpp_fb_ioctl(aic_disp->fb, AICFB_WAIT_FOR_VSYNC, 0);
        aic_disp->sync_ready = true;
        lv_thread_sync_signal(&aic_disp->sync_notify);

        if (aic_disp->exit_status) {
            LV_LOG_INFO("Ready to exit aic display thread.");
            break;
        }
    }

    lv_thread_sync_delete(&aic_disp->sync);
    lv_thread_sync_delete(&aic_disp->sync_notify);
    LV_LOG_INFO("Exit aic display thread.");
}
#endif

#if defined(LV_DISPLAY_ROTATE_EN)
static uint8_t *create_draw_buf(aic_disp_t *aic_disp, int w, int h, lv_color_format_t cf)
{
    int bpp = lv_color_format_get_bpp(cf) / 8;
    int buf_size = ALIGN_UP(w, 8) * ALIGN_UP(h, 8) * bpp;

    aic_disp->buf = (uint8_t *)aicos_malloc_try_cma(buf_size + CACHE_LINE_SIZE - 1);
    if (!aic_disp->buf) {
        return NULL;
    } else {
        unsigned int align_addr = (unsigned int)ALIGN_UP((ulong) aic_disp->buf, CACHE_LINE_SIZE);
        aic_disp->buf_aligned = (uint8_t *)(ulong)align_addr;
        aicos_dcache_clean_invalid_range((ulong *)(ulong)align_addr, ALIGN_UP(buf_size, CACHE_LINE_SIZE));
    }

    aic_disp->buf_size = ALIGN_UP(buf_size, CACHE_LINE_SIZE);
    return aic_disp->buf_aligned;
}
#endif

void lv_port_disp_init(void)
{
    void *buf1;
    void *buf2;
    aic_disp_t *aic_disp;
    int fb_size;
    int width;
    int height;
    int stride;
    lv_color_format_t cf;

#if LV_USE_MPP_DEC
    lv_mpp_dec_init();
#endif

#if LV_USE_DRAW_GE2D
    lv_draw_ge2d_init();
#endif

    aic_disp = (aic_disp_t *)lv_malloc_zeroed(sizeof(aic_disp_t));
    if (!aic_disp) {
        LV_LOG_ERROR("malloc aic display failed");
        return;
    }

    aic_disp->fb = mpp_fb_open();
    if (aic_disp->fb) {
        mpp_fb_ioctl(aic_disp->fb, AICFB_GET_SCREENINFO, &aic_disp->info);
        width = aic_disp->info.width;
        height = aic_disp->info.height;
        stride = aic_disp->info.stride;
        fb_size = height * stride;
        aic_disp->fb_size = fb_size;

#ifdef AIC_PAN_DISPLAY
        buf1 = (void *)(aic_disp->info.framebuffer + fb_size);
        buf2 = (void *)aic_disp->info.framebuffer;
#else
        buf1 = (void *)aic_disp->info.framebuffer;
        buf2 = NULL;
#endif
    } else {
        LV_LOG_ERROR("get device fb info failed");
        lv_free(aic_disp);
        return;
    }

    cf = lv_display_fmt(aic_disp->info.format);
#if defined(LV_DISPLAY_ROTATE_EN)
    if (!create_draw_buf(aic_disp, width, height, cf)) {
        LV_LOG_ERROR("create draw buffer failed");
        mpp_fb_close(aic_disp->fb);
        lv_free(aic_disp);
        return;
    } else {
        buf1 = (void *)aic_disp->buf_aligned;
        buf2 = NULL;
        aic_disp->buf_id = 0;
    }
#endif

    lv_display_t *disp = lv_display_create(width, height);
    lv_display_set_color_format(disp, cf);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, aic_disp);
    lv_display_set_buffers(disp, buf1, buf2, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);

#if defined(LV_DISPLAY_ROTATE_EN) && defined(LV_ROTATE_DEGREE)
    lv_display_set_rotation(disp, LV_ROTATE_DEGREE / 90);
#endif

#if LV_USE_OS && defined(AIC_PAN_DISPLAY) && defined(LV_DISPLAY_ROTATE_EN)
    aic_disp->sync_ready = true;
    lv_thread_sync_init(&aic_disp->sync);
    lv_thread_sync_init(&aic_disp->sync_notify);
    lv_thread_init(&aic_disp->thread, LV_THREAD_PRIO_HIGH, aic_display_thread, 4 * 1024, aic_disp);
#endif

#ifndef AIC_MONKEY_TEST
#if defined(KERNEL_RTTHREAD) && defined(AIC_USING_TOUCH)
    int result = tpc_run(AIC_TOUCH_PANEL_NAME, width, height);
    if (result) {
        LV_LOG_INFO("can't find touch panel\n");
    }
#endif
#endif
}
