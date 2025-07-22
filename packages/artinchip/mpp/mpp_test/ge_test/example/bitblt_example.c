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

typedef struct {
    char src_src[128];
    unsigned int dst_x;
    unsigned int dst_y;
    unsigned int dst_width;
    unsigned int dst_height;
    unsigned int dither_en;
} bitblt_example_para;

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-i  --src src\n");
    printf("\t-x  --dst x\n");
    printf("\t-y  --dst y\n");
    printf("\t-w  --dst width\n");
    printf("\t-h  --dst height\n\n");
    printf("\t-d  --dither enable\n\n");

    printf("\tfor example:\n");
    printf("\tge_bitblt_example -i /data/singer_alpha.bmp -x 0 -y 0 -w 100 -h 100 -d 1\n");
}

static int match_parameters(int argc, char **argv, bitblt_example_para *para)
{
    const char sopts[] = "ui:x:y:w:h:d:";
    const struct option lopts[] = {
        {"usage",     no_argument,       NULL, 'u'},
        {"src_src" ,  required_argument, NULL, 'i'},
        {"dst_x",     required_argument, NULL, 'x'},
        {"dst_y",     required_argument, NULL, 'y'},
        {"width",     required_argument, NULL, 'w'},
        {"height",    required_argument, NULL, 'h'},
        {"dither_en", required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };

    memset(para, 0, sizeof(bitblt_example_para));
    strncpy(para->src_src, "/sdcard/image/singer_alpha.bmp", 128 - 1);

    optind = 0;
    int ret = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'i':
            strncpy(para->src_src, optarg, 128 - 1);
            break;
        case 'x':
            para->dst_x = strtol(optarg, NULL, 10);
            break;
        case 'y':
            para->dst_y = strtol(optarg, NULL, 10);
            break;
        case 'w':
            para->dst_width = strtol(optarg, NULL, 10);
            break;
        case 'h':
            para->dst_height = strtol(optarg, NULL, 10);
        case 'd':
            para->dither_en = strtol(optarg, NULL, 10);
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

static void ensure_para_valid(struct ge_buf *src_buffer,
                              struct aicfb_screeninfo *screen_info, bitblt_example_para *para)
{
    if (para->dst_x >= screen_info->width)
        para->dst_x = screen_info->width - 1;

    if (para->dst_y >= screen_info->height)
        para->dst_y = screen_info->height - 1;

    if (para->dst_width == 0)
        para->dst_width = src_buffer->buf.size.width;

    if (para->dst_height == 0)
        para->dst_height = src_buffer->buf.size.height;

    if (para->dst_width >= screen_info->width)
        para->dst_width = screen_info->width;

    if (para->dst_height >= screen_info->height)
        para->dst_height = screen_info->height;

    if (para->dst_x + para->dst_width >= screen_info->width)
        para->dst_x = 0;

    if (para->dst_y + para->dst_height >= screen_info->height)
        para->dst_y = 0;
}

static int copy_to_framebuffer(struct mpp_ge *ge, struct ge_buf *src_buffer,
                                struct aicfb_screeninfo *screen_info, bitblt_example_para *para)
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
    blt.dst_buf.crop.x = para->dst_x;
    blt.dst_buf.crop.y = para->dst_y;
    blt.dst_buf.crop.width = para->dst_width;
    blt.dst_buf.crop.height = para->dst_height;

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

static int ge_bitblt_example(int argc, char **argv)
{
    int ret = -1;
    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct ge_buf *src_buffer = NULL;
    struct bmp_header bmp_head = {0};

    struct mpp_fb *fb = NULL;
    struct mpp_ge *ge = NULL;

    static bitblt_example_para bitble_para = {0};
    static struct aicfb_screeninfo screen_info = {0};

    if (match_parameters(argc, argv, &bitble_para) < 0)
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

    bmp_fd = bmp_open(bitble_para.src_src, &bmp_head);
    if (bmp_fd < 0) {
        printf("open bmp error, path = %s\n", bitble_para.src_src);
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

    ensure_para_valid(src_buffer, &screen_info, &bitble_para);

    copy_to_framebuffer(ge, src_buffer, &screen_info, &bitble_para);
EXIT:
    if (bmp_fd > 0)
        bmp_close(bmp_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb)
        mpp_fb_close(fb);

    if (src_buffer)
        ge_buf_free(src_buffer);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ge_bitblt_example, ge_bitblt_example, ge bitble base example);
