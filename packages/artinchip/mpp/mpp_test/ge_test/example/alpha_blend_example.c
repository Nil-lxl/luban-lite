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
    unsigned int alpha_rule;
    unsigned int src_alpha_mode;
    unsigned int dst_alpha_mode;
    unsigned int src_global_alpha;
    unsigned int dst_global_alpha;
} alpha_blend_example_para;

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-i  --src src\n");
    printf("\t-r  --alpha rule\n");
    printf("\t-s  --src alpha mode\n");
    printf("\t-d  --dst alpha mode\n");
    printf("\t-g  --src global alpha\n");
    printf("\t-p  --dst global alpha\n\n");

    printf("\tfor example:\n");
    printf("\talpha_blend_example -i /data/singer_alpha.bmp -r 0 -s 0 -d 0 -g 100 -p 100\n");
}

static int match_parameters(int argc, char **argv, alpha_blend_example_para *para)
{
    const char sopts[] = "ui:r:s:d:g:p:";
    const struct option lopts[] = {
        {"usage",            no_argument,       NULL, 'u'},
        {"src_src" ,         required_argument, NULL, 'i'},
        {"alpha_rule",       required_argument, NULL, 'r'},
        {"src_alpha_mode",   required_argument, NULL, 's'},
        {"dst_alpha_mode",   required_argument, NULL, 'd'},
        {"src_global_alpha", required_argument, NULL, 'g'},
        {"dst_global_alpha", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
    };

    memset(para, 0, sizeof(alpha_blend_example_para));
    strncpy(para->src_src, "/sdcard/image/singer_alpha.bmp", 128 - 1);

    optind = 0;
    int ret = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'i':
            strncpy(para->src_src, optarg, 128 - 1);
            break;
        case 'r':
            para->alpha_rule = strtol(optarg, NULL, 10);
            if ((para->alpha_rule > GE_PD_DST) || (para->src_alpha_mode < GE_PD_NONE)) {
                printf("alpha rule invalid, please input against\n");
                return -1;
            }
            break;
        case 's':
            para->src_alpha_mode = strtol(optarg, NULL, 10);
            if ((para->src_alpha_mode > 3) || (para->src_alpha_mode < 0)) {
                printf("src alpha mode invalid, please input against\n");
                return -1;
            }
            break;
        case 'd':
            para->dst_alpha_mode = strtol(optarg, NULL, 10);
            if ((para->dst_alpha_mode > 3) || (para->dst_alpha_mode < 0)) {
                printf("dst alpha mode invalid, please input against\n");
                return 0;
            }
            break;
        case 'g':
            para->src_global_alpha = strtol(optarg, NULL, 10);
            if ((para->src_global_alpha > 255) || (para->src_global_alpha < 0)) {
                printf("src global alpha invalid, please input against\n");
                return 0;
            }
            break;
        case 'p':
            para->dst_global_alpha = strtol(optarg, NULL, 10);
            if ((para->dst_global_alpha > 255) || (para->dst_global_alpha < 0)) {
                printf("dst global alpha invalid, please input against\n");
                return 0;
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

    printf("r = %d, s = %d, g = %d, d = %d, p = %d\n",
    para->alpha_rule, para->src_alpha_mode, para->src_global_alpha, para->dst_alpha_mode, para->dst_global_alpha);
    return 0;
}

static int alpha_blend_with_framebuffer(struct mpp_ge *ge, struct ge_buf *src_buffer,
                                struct aicfb_screeninfo *screen_info, alpha_blend_example_para *para)
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

    blt.ctrl.alpha_en = 1;
    blt.ctrl.alpha_rules = para->alpha_rule;
    blt.ctrl.src_alpha_mode = para->src_alpha_mode;
    blt.ctrl.src_global_alpha = para->src_global_alpha;
    blt.ctrl.dst_alpha_mode = para->dst_alpha_mode;
    blt.ctrl.dst_global_alpha = para->dst_global_alpha;

    /* fill rect and rotate also perform alpha blending simultaneously, with similar settings. */
    /*
    struct ge_fillrect fill = {0};
    fill.ctrl.alpha_en = 1;
    fill.ctrl.alpha_rules = para->alpha_rule;
    fill.ctrl.src_alpha_mode = para->src_alpha_mode;
    fill.ctrl.src_global_alpha = para->src_global_alpha;
    fill.ctrl.dst_alpha_mode = para->dst_alpha_mode;
    fill.ctrl.dst_global_alpha = para->dst_global_alpha;

    struct ge_rotation rot = {0};
    rot.ctrl.alpha_en = 1;
    rot.ctrl.alpha_rules = para->alpha_rule;
    rot.ctrl.src_alpha_mode = para->src_alpha_mode;
    rot.ctrl.src_global_alpha = para->src_global_alpha;
    rot.ctrl.dst_alpha_mode = para->dst_alpha_mode;
    rot.ctrl.dst_global_alpha = para->dst_global_alpha;
    */

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

static int ge_alpha_blend_example(int argc, char **argv)
{
    int ret = -1;
    int bmp_fd = -1;
    enum mpp_pixel_format bmp_fmt = 0;
    struct ge_buf *src_buffer = NULL;
    struct bmp_header bmp_head = {0};

    struct mpp_fb *fb = NULL;
    struct mpp_ge *ge = NULL;

    static alpha_blend_example_para alpha_blend_para = {0};
    static struct aicfb_screeninfo screen_info = {0};

    if (match_parameters(argc, argv, &alpha_blend_para) < 0)
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

    bmp_fd = bmp_open(alpha_blend_para.src_src, &bmp_head);
    if (bmp_fd < 0) {
        printf("open bmp error, path = %s\n", alpha_blend_para.src_src);
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

    alpha_blend_with_framebuffer(ge, src_buffer, &screen_info, &alpha_blend_para);
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
MSH_CMD_EXPORT_ALIAS(ge_alpha_blend_example, ge_alpha_blend_example, ge alpha blend example);
