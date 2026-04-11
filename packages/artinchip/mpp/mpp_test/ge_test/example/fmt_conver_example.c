/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ZeQuan Liang <zequan.liang@artinchip.com>
 */

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <aic_core.h>

#include "mpp_ge.h"
#include "mpp_fb.h"
#include "bmp.h"
#include "ge_mem.h"

struct StrToFormat {
    char *str;
    int format;
};

typedef struct {
    char src_src[128];
    int format;
} fmt_conver_example_para;

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-i  --src src\n");
    printf("\t-f  --dst format\n\n");

    printf("\tfor example:\n");
    printf("\tge_fmt_conver_example_example -i /data/singer_alpha.bmp -f RGB565\n");
}

static int str_to_format(char *str)
{
    int i = 0;
    int table_size = 0;
    static struct StrToFormat table[] = {
        {"argb8888", MPP_FMT_ARGB_8888},
        {"abgr8888", MPP_FMT_ABGR_8888},
        {"rgba8888", MPP_FMT_RGBA_8888},
        {"bgra8888", MPP_FMT_BGRA_8888},
        {"xrgb8888", MPP_FMT_XRGB_8888},
        {"xbgr8888", MPP_FMT_XBGR_8888},
        {"rgbx8888", MPP_FMT_RGBX_8888},
        {"bgrx8888", MPP_FMT_BGRX_8888},
        {"argb4444", MPP_FMT_ARGB_4444},
        {"abgr4444", MPP_FMT_ABGR_4444},
        {"rgba4444", MPP_FMT_RGBA_4444},
        {"bgra4444", MPP_FMT_BGRA_4444},
        {"rgb565", MPP_FMT_RGB_565},
        {"bgr565", MPP_FMT_BGR_565},
        {"argb1555", MPP_FMT_ARGB_1555},
        {"abgr1555", MPP_FMT_ABGR_1555},
        {"rgba5551", MPP_FMT_RGBA_5551},
        {"bgra5551", MPP_FMT_BGRA_5551},
        {"rgb888", MPP_FMT_RGB_888},
        {"bgr888", MPP_FMT_BGR_888},
        {"yuv420", MPP_FMT_YUV420P},
        {"nv12", MPP_FMT_NV12},
        {"nv21", MPP_FMT_NV21},
        {"yuv422", MPP_FMT_YUV422P},
        {"nv16", MPP_FMT_NV16},
        {"nv61", MPP_FMT_NV61},
        {"yuyv", MPP_FMT_YUYV},
        {"yvyu", MPP_FMT_YVYU},
        {"uyvy", MPP_FMT_UYVY},
        {"vyuy", MPP_FMT_VYUY},
        {"yuv400", MPP_FMT_YUV400},
        {"yuv444", MPP_FMT_YUV444P},
    };
    table_size = sizeof(table) / sizeof(table[0]);
    for (i = 0; i < table_size; i++) {
        if (!strncmp(str, table[i].str, strlen(table[i].str)))
            return table[i].format;
    }
    return -1;
}

static int match_parameters(int argc, char **argv, fmt_conver_example_para *para)
{
    const char sopts[] = "ui:f:";
    const struct option lopts[] = {
        {"usage",     no_argument,       NULL, 'u'},
        {"src_src" ,  required_argument, NULL, 'i'},
        {"dst_fmt",   required_argument, NULL, 'f'},
        {0, 0, 0, 0}
    };

    memset(para, 0, sizeof(fmt_conver_example_para));
    strncpy(para->src_src, "/sdcard/image/singer_alpha.bmp", 128 - 1);

    optind = 0;
    int ret = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'i':
            strncpy(para->src_src, optarg, 128 - 1);
            break;
        case 'f':
            para->format = str_to_format(optarg);
            if (para->format < 0) {
                printf("mode set error, please set against\n\n");
                printf("\r--RGB format supports: \n");
                printf("\r--ARGB8888, ABGR8888, RGBA8888, BGRA8888\n");
                printf("\r--XRGB8888, XBGR8888, RGBX8888, BGRX8888\n");
                printf("\r--ARGB4444, ABGR4444, RGBA4444, BGRA4444\n");
                printf("\r--ARGB1555, ABGR1555, RGBA5551, BGRA5551\n");
                printf("\r--RGB888,   BGR888,   RGB565,   BGR565\n\n");
#ifndef AIC_GE_DRV_V11
                printf("\r--YUV format supports: \n");
                printf("\r--YUV420, NV12, NV21\n");
                printf("\r--YUV422, NV16, NV61 , YUYV, YVYU, UYVY, VYUY\n");
                printf("\r--YUV400, YUV444\n");
#endif
                return -1;
            }
            break;
        case 'u':
            usage(argv[0]);
            return -1;
        default:
            printf("Invalid parameter: %#x\n", ret);
            return -1;
        }
    }

    return 0;
}

static int format_converse(struct mpp_ge *ge, struct ge_buf *src_buffer,
                            struct ge_buf *dst_buffer)
{
    struct ge_bitblt blt = {0};

    /* src layer set */
    memcpy(&blt.src_buf, &src_buffer->buf, sizeof(struct mpp_buf));

    /* dst_buffer->buf.format == fmt_conver_example_para->format */
    memcpy(&blt.dst_buf, &dst_buffer->buf, sizeof(struct mpp_buf));
    // blt.dst_buf.format = dst_buffer->buf.format;

    /**
     * Prepare a memory buffer for hardware (e.g., DMA) access by ensuring
     * CPU cache coherence. Steps:
     * 1. Clean (flush) Dcache to write CPU-modified data to main memory.
     * 2. Invalidate Dcache to force reload from memory after hardware writes.
     */
    ge_buf_clean_dcache(src_buffer);
    ge_buf_clean_dcache(dst_buffer);

    int ret = mpp_ge_bitblt(ge, &blt);
    if (ret < 0) {
        printf("bitblt task failed\n");
        return ret;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        printf("emit task failed\n");
        return ret;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        printf("ge sync fail\n");
        return ret;
    }

    return 0;
}

static int copy_to_framebuffer(struct mpp_ge *ge, struct ge_buf *src_buffer,
                                struct aicfb_screeninfo *screen_info)
{
    struct ge_bitblt blt = {0};

    /* src layer set */
    memcpy(&blt.src_buf, &src_buffer->buf, sizeof(struct mpp_buf));

    /* screen is rgb format */
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (intptr_t)screen_info->framebuffer;
    blt.dst_buf.stride[0] = screen_info->stride;
    blt.dst_buf.size.width = screen_info->width;
    blt.dst_buf.size.height = screen_info->height;
    blt.dst_buf.format = screen_info->format;
    blt.dst_buf.crop_en = 1;
    blt.dst_buf.crop.x = 0;
    blt.dst_buf.crop.y = 0;
    blt.dst_buf.crop.width = src_buffer->buf.size.width;
    blt.dst_buf.crop.height = src_buffer->buf.size.height;

    /**
     * Prepare a memory buffer for hardware (e.g., DMA) access by ensuring
     * CPU cache coherence. Steps:
     * 1. Clean (flush) Dcache to write CPU-modified data to main memory.
     * 2. Invalidate Dcache to force reload from memory after hardware writes.
     */
    aicos_dcache_clean_invalid_range((void *)((uintptr_t)screen_info->framebuffer), screen_info->smem_len);
    ge_buf_clean_dcache(src_buffer);

    int ret = mpp_ge_bitblt(ge, &blt);
    if (ret < 0) {
        printf("bitblt task failed\n");
        return ret;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        printf("emit task failed\n");
        return ret;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        printf("ge sync fail\n");
        return ret;
    }

    return 0;
}

static int ge_fmt_conver_example(int argc, char **argv)
{
    int ret = -1;
    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct ge_buf *src_buffer = NULL;
    struct ge_buf *dst_buffer = NULL;
    struct bmp_header bmp_head = {0};

    struct mpp_fb *fb = NULL;
    struct mpp_ge *ge = NULL;

    static fmt_conver_example_para fmt_conver_para = {0};
    static struct aicfb_screeninfo screen_info = {0};

    if (match_parameters(argc, argv, &fmt_conver_para) < 0)
        return -1;

    ge = mpp_ge_open();
    if (!ge) {
        printf("open ge device error\n");
        goto EXIT;
    }

    fb = mpp_fb_open();
    if (!fb) {
        printf("mpp fb open failed\n");
        goto EXIT;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &screen_info);
    if (ret) {
        printf("mpp_fb_ioctl ops failed\n");
        goto EXIT;
    }

    bmp_fd = bmp_open(fmt_conver_para.src_src, &bmp_head);
    if (bmp_fd < 0) {
        printf("open bmp error, path = %s\n", fmt_conver_para.src_src);
        goto EXIT;
    }

    bmp_fmt = bmp_get_fmt(&bmp_head);

    src_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), bmp_fmt);
    if (src_buffer == NULL) {
        printf("malloc src buffer error\n");
        goto EXIT;
    }

    /* default format is rgb */
    ret = bmp_read(bmp_fd, (void *)((uintptr_t)src_buffer->buf.phy_addr[0]), &bmp_head);
    if (ret < 0) {
        printf("src bmp_read error\n");
        goto EXIT;
    }

    /* dst_buffer->buf.format == fmt_conver_example_para->format */
    dst_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), fmt_conver_para.format);
    if (dst_buffer == NULL) {
        printf("malloc dst buffer error\n");
        goto EXIT;
    }

    format_converse(ge, src_buffer, dst_buffer);

    copy_to_framebuffer(ge, dst_buffer, &screen_info);
EXIT:
    if (bmp_fd > 0)
        bmp_close(bmp_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb)
        mpp_fb_close(fb);

    if (src_buffer)
        ge_buf_free(src_buffer);

    if (dst_buffer)
        ge_buf_free(dst_buffer);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ge_fmt_conver_example, ge_fmt_conver_example, ge format conver example);
