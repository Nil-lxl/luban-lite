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
    u32 start_color;
    u32 end_color;
    unsigned int type;
    unsigned int dst_x;
    unsigned int dst_y;
    unsigned int dst_width;
    unsigned int dst_height;
} fillrect_example_para;

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-t  --gradient type: 0,1,2\n");
    printf("\t-s  --start color(argb): 0xffffffff\n");
    printf("\t-e  --end color(argb): 0x0\n");
    printf("\t-x  --dst x\n");
    printf("\t-y  --dst y\n");
    printf("\t-w  --dst width\n");
    printf("\t-h  --dst height\n\n");

    printf("\tfor example:\n");
    printf("\tge_fillrect_example -s 0xffffffff -e 0x0 -t 1 -x 0 -y 0 -w 100 -h 100\n");
}

static int match_parameters(int argc, char **argv, fillrect_example_para *para)
{
    const char sopts[] = "us:e:t:x:y:w:h:";
    const struct option lopts[] = {
        {"usage",       no_argument,       NULL, 'u'},
        {"start_color", required_argument, NULL, 's'},
        {"end_color",   required_argument, NULL, 'e'},
        {"type",        required_argument, NULL, 't'},
        {"dst_x",       required_argument, NULL, 'x'},
        {"dst_y",       required_argument, NULL, 'y'},
        {"width",       required_argument, NULL, 'w'},
        {"height",      required_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    memset(para, 0, sizeof(fillrect_example_para));
    para->start_color = 0xffffffff;

    optind = 0;
    int ret = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 's':
            para->start_color = strtol(optarg, NULL, 16);
            break;
        case 'e':
            para->end_color = strtol(optarg, NULL, 16);
            break;
        case 't':
            para->type = strtol(optarg, NULL, 10);
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

static void ensure_para_valid(struct aicfb_screeninfo *screen_info, fillrect_example_para *para)
{
    if (para->dst_x >= screen_info->width)
        para->dst_x = screen_info->width - 1;

    if (para->dst_y >= screen_info->height)
        para->dst_y = screen_info->height - 1;

    if (para->dst_width == 0)
        para->dst_width = screen_info->width;

    if (para->dst_height == 0)
        para->dst_height = screen_info->height;

    if (para->dst_width >= screen_info->width)
        para->dst_width = screen_info->width;

    if (para->dst_height >= screen_info->height)
        para->dst_height = screen_info->height;

    if (para->dst_x + para->dst_width >= screen_info->width)
        para->dst_x = 0;

    if (para->dst_y + para->dst_height >= screen_info->height)
        para->dst_y = 0;
}

static int fillrect_to_framebuffer(struct mpp_ge *ge, struct aicfb_screeninfo *screen_info, fillrect_example_para *para)
{
    struct ge_fillrect fill = {0};

    fill.type = para->type;
    fill.start_color = para->start_color; /* argb format */
    fill.end_color = para->end_color;

    fill.dst_buf.buf_type = MPP_PHY_ADDR;
    fill.dst_buf.phy_addr[0] = (intptr_t)screen_info->framebuffer;
    fill.dst_buf.stride[0] = screen_info->stride;
    fill.dst_buf.size.width = screen_info->width;
    fill.dst_buf.size.height = screen_info->height;
    fill.dst_buf.format = screen_info->format;
    fill.dst_buf.crop_en = 1;
    fill.dst_buf.crop.x = para->dst_x;
    fill.dst_buf.crop.y = para->dst_y;
    fill.dst_buf.crop.width = para->dst_width;
    fill.dst_buf.crop.height = para->dst_height;

    /**
     * Prepare a memory buffer for hardware (e.g., DMA) access by ensuring
     * CPU cache coherence. Steps:
     * 1. Clean (flush) Dcache to write CPU-modified data to main memory.
     * 2. Invalidate Dcache to force reload from memory after hardware writes.
     */
    aicos_dcache_clean_invalid_range((void *)((uintptr_t)screen_info->framebuffer), screen_info->smem_len);

    int ret = mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        printf("fillrect task failed\n");
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

static int ge_fillrect_example(int argc, char **argv)
{
    int ret = -1;
    struct mpp_fb *fb = NULL;
    struct mpp_ge *ge = NULL;
    struct aicfb_screeninfo screen_info = {0};
    static fillrect_example_para fillrect_para = {0};

    if (match_parameters(argc, argv, &fillrect_para) < 0)
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

    ensure_para_valid(&screen_info, &fillrect_para);

    fillrect_to_framebuffer(ge, &screen_info, &fillrect_para);
EXIT:
    if (ge)
        mpp_ge_close(ge);

    if (fb)
        mpp_fb_close(fb);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(ge_fillrect_example, ge_fillrect_example, ge fillrect example);
