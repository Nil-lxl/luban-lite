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
#include <math.h>

#include <aic_core.h>

#include "mpp_ge.h"
#include "mpp_fb.h"
#include "bmp.h"
#include "ge_mem.h"

#define PI 3.141592653589
#define SIN(x) (sin((x)* PI / 180.0))
#define COS(x) (cos((x)* PI / 180.0))

typedef struct {
    char src_src[128];
    unsigned int center_x;
    unsigned int center_y;
    unsigned int dst_x;
    unsigned int dst_y;
    int angle_sin;
    int angle_cos;
} rotate_example_para;

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-c  --rotate center x\n");
    printf("\t-d  --rotate center y\n");
    printf("\t-x  --dst x\n");
    printf("\t-y  --dst y\n");
    printf("\t-a  --angle_degrees(0-360.0)\n\n");

    printf("\tfor example:\n");
    printf("\tge_rotate_example -i /data/singer_alpha.bmp -a 90.0 -c 50 -d 50 -x 600 - y 300 \n");
}

static int match_parameters(int argc, char **argv, rotate_example_para *para)
{
    float angle = 0;
    const char sopts[] = "ui:c:d:x:y:a:";
    const struct option lopts[] = {
        {"usage",       no_argument,       NULL, 'u'},
        {"src_src" ,    required_argument, NULL, 'i'},
        {"center_x",    required_argument, NULL, 'c'},
        {"center_y",    required_argument, NULL, 'd'},
        {"dst_x",       required_argument, NULL, 'x'},
        {"dst_y",       required_argument, NULL, 'y'},
        {"angle",       required_argument, NULL, 'a'},
        {0, 0, 0, 0}
    };

    memset(para, 0, sizeof(rotate_example_para));
    strncpy(para->src_src, "/sdcard/image/singer_alpha.bmp", 128 - 1);

    optind = 0;
    int ret = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'i':
            strncpy(para->src_src, optarg, 128 - 1);
            break;
        case 'c':
            para->center_x = strtol(optarg, NULL, 10);
            break;
        case 'd':
            para->center_y = strtol(optarg, NULL, 10);
            break;
        case 'x':
            para->dst_x = strtol(optarg, NULL, 10);
            break;
        case 'y':
            para->dst_y = strtol(optarg, NULL, 10);
            break;
        case 'a':
            angle = strtof(optarg, NULL);
            while(angle < 0) angle += 360.0;
            while(angle >= 360) angle -= 360.0;
            para->angle_sin = SIN((double)angle) * 4096;
            para->angle_cos = COS((double)angle) * 4096;
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

static int rotate_to_framebuffer(struct mpp_ge *ge, struct ge_buf *src_buffer,
                                    struct aicfb_screeninfo *screen_info, rotate_example_para *para)
{
    struct ge_rotation rot = {0};

    /* src layer set */
    memcpy(&rot.src_buf, &src_buffer->buf, sizeof(struct mpp_buf));
    rot.src_rot_center.x = para->center_x;
    rot.src_rot_center.y = para->center_y;

    /* screen is rgb format */
    rot.dst_buf.buf_type = MPP_PHY_ADDR;
    rot.dst_buf.phy_addr[0] = (intptr_t)screen_info->framebuffer;
    rot.dst_buf.stride[0] = screen_info->stride;
    rot.dst_buf.size.width = screen_info->width;
    rot.dst_buf.size.height = screen_info->height;
    rot.dst_buf.format = screen_info->format;
    rot.dst_buf.crop_en = 1;
    rot.dst_buf.crop.x = 0;
    rot.dst_buf.crop.y = 0;
    rot.dst_buf.crop.width = screen_info->width;
    rot.dst_buf.crop.height = screen_info->height;

    rot.dst_rot_center.x = para->dst_x;
    rot.dst_rot_center.y = para->dst_y;

    rot.angle_sin = para->angle_sin;
    rot.angle_cos = para->angle_cos;

    /**
     * Prepare a memory buffer for hardware (e.g., DMA) access by ensuring
     * CPU cache coherence. Steps:
     * 1. Clean (flush) Dcache to write CPU-modified data to main memory.
     * 2. Invalidate Dcache to force reload from memory after hardware writes.
     */
    aicos_dcache_clean_invalid_range((void *)((uintptr_t)screen_info->framebuffer), screen_info->smem_len);
    ge_buf_clean_dcache(src_buffer);

    int ret = mpp_ge_rotate(ge, &rot);
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

static int ge_rotate_example(int argc, char **argv)
{
    int ret = -1;
    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct ge_buf *src_buffer = NULL;
    struct bmp_header bmp_head = {0};

    struct mpp_fb *fb = NULL;
    struct mpp_ge *ge = NULL;
    struct aicfb_screeninfo screen_info = {0};
    static rotate_example_para rotate_para = {0};

    if (match_parameters(argc, argv, &rotate_para) < 0)
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

    bmp_fd = bmp_open(rotate_para.src_src, &bmp_head);
    if (bmp_fd < 0) {
        printf("open bmp error, path = %s\n", rotate_para.src_src);
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

    rotate_to_framebuffer(ge, src_buffer, &screen_info, &rotate_para);
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
MSH_CMD_EXPORT_ALIAS(ge_rotate_example, ge_rotate_example, ge rotate base example);
