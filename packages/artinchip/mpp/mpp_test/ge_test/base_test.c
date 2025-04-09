/*
 * Copyright (C) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ardon <haidong.pan@artinchip.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <aic_core.h>
#include "mpp_ge.h"
#include "mpp_fb.h"
#include "artinchip_fb.h"
#include "aic_hal_clk.h"

#include "./public/bmp.h"
#include "./public/ge_fb.h"
#include "./public/ge_mem.h"

#include <getopt.h>

#ifndef LOG_TAG
#define LOG_TAG "ge_test"
#endif

#define LOGE(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) aic_log(AIC_LOG_WARN, "W", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) aic_log(AIC_LOG_DEBUG, "D", fmt, ##__VA_ARGS__)

#define ROT_SRC_CENTER_X    24
#define ROT_SRC_CENTER_Y    194

static char g_src_input[128] = {"/sdcard/ge_test/image/base/clock.bmp"};
static char g_bg_input[128] = {"/sdcard/ge_test/image/base/clock.bmp"};

#define ROT_DST_CENTER_X    294
#define ROT_DST_CENTER_Y    297

#define APP_FB_NUM 2

#define PARA_NUM   0
#define PARA_CIR   1

static struct aicfb_screeninfo g_info = {0};

static int degree_list[] = {
    0, 4096,
    428, 4073,
    851, 4006,
    1265, 3895,
    1665, 3741,
    2047, 3547,
    2407, 3313,
    2740, 3043,
    3043, 2740,
    3313, 2407,
    3547, 2048,
    3741, 1665,
    3895, 1265,
    4006, 851,
    4073, 428,
    4096, 0,
    4073, -428,
    4006, -851,
    3895, -1265,
    3741, -1665,
    3547, -2047,
    3313, -2407,
    3043, -2740,
    2740, -3043,
    2407, -3313,
    2048, -3547,
    1665, -3741,
    1265, -3895,
    851, -4006,
    428, -4073,
    0, -4096,
    -428, -4073,
    -851, -4006,
    -1265, -3895,
    -1665, -3741,
    -2047, -3547,
    -2407, -3313,
    -2740, -3043,
    -3043, -2740,
    -3313, -2407,
    -3547, -2048,
    -3741, -1665,
    -3895, -1265,
    -4006, -851,
    -4073, -428,
    -4096, 0,
    -4073, 428,
    -4006, 851,
    -3895, 1265,
    -3741, 1665,
    -3547, 2047,
    -3313, 2407,
    -3043, 2740,
    -2740, 3043,
    -2407, 3313,
    -2048, 3547,
    -1665, 3741,
    -1265, 3895,
    -851, 4006,
    -428, 4073,
    0, 4096,
};

static void usage(char *app)
{
    printf("Usage: %s [Options]: \n", app);
    printf("\t-c, --circle test\n");
    printf("\t-i, --input src\n");
    printf("\t-o, --input bg_src\n");
    printf("\t-n, --number of run, default run once\n");
    printf("\t-d, --delay, unit: ms");
    printf("\t-u, --usage\n\n");

    printf("\tfor example:\n");
    printf("\tge_fill -n 10 -d 0\n");
    printf("\tge_bitblt -i /data/clock.bmp -c\n");
    printf("\tge_rotate -i /data/second.bmp -o /data/clock.bmp\n");
}

static long long int str2int(char *_str)
{
    if (_str == NULL) {
        pr_err("The string is empty!\n");
        return -1;
    }

    if (strncmp(_str, "0x", 2))
        return atoi(_str);
    else
        return strtoll(_str, NULL, 16);
}

static void show_ge_freq(void)
{
    int ret = hal_clk_get_freq(CLK_GE);

    printf("GE current freq: %d MHz\n", ret / 1000000);
}

static void draw_clock(struct ge_bitblt *blt, struct ge_fb_info *info,
                       struct ge_buf *buffer, struct bmp_header *head)
{
    unsigned int fb_phy = 0;

    memset(blt, 0, sizeof(struct ge_bitblt));

    /* source buffer */
    memcpy(&blt->src_buf, &buffer->buf, sizeof(struct mpp_buf));

    blt->src_buf.crop_en = 1;
    blt->src_buf.crop.x = 0;
    blt->src_buf.crop.y = 0;
    blt->src_buf.crop.width = head->width;
    blt->src_buf.crop.height = abs(head->height);

    fb_phy = fb_get_cur_frame(info);
    blt->dst_buf.buf_type = MPP_PHY_ADDR;
    blt->dst_buf.phy_addr[0] = fb_phy;
    blt->dst_buf.stride[0] = info->fb_data.stride;
    blt->dst_buf.size.width = info->fb_data.width;
    blt->dst_buf.size.height = info->fb_data.height;
    blt->dst_buf.format = info->fb_data.format;
    blt->ctrl.flags = 0;
    blt->dst_buf.crop_en = 1;
    blt->dst_buf.crop.x = 0;
    blt->dst_buf.crop.y = 0;
    blt->dst_buf.crop.width = head->width;
    blt->dst_buf.crop.height = abs(head->height);
}

static void move_second_hand(struct ge_rotation *rot, struct ge_fb_info *info,
                            struct ge_buf *buffer, int sin, int cos)
{
    unsigned int fb_phy = 0;

    memset(rot, 0, sizeof(struct ge_rotation));

    /* source buffer */
    memcpy(&rot->src_buf, &buffer->buf, sizeof(struct mpp_buf));
    rot->src_rot_center.x = ROT_SRC_CENTER_X;
    rot->src_rot_center.y = ROT_SRC_CENTER_Y;

    /* destination buffer */
    fb_phy = fb_get_cur_frame(info);
    rot->dst_buf.buf_type = MPP_PHY_ADDR;
    rot->dst_buf.phy_addr[0] = fb_phy;
    rot->dst_buf.stride[0] = info->fb_data.stride;
    rot->dst_buf.size.width = info->fb_data.width;
    rot->dst_buf.size.height = info->fb_data.height;
    rot->dst_buf.format = info->fb_data.format;
    rot->dst_rot_center.x = ROT_DST_CENTER_X;
    rot->dst_rot_center.y = ROT_DST_CENTER_Y;

    rot->ctrl.alpha_en = 1;

    rot->angle_sin = sin;
    rot->angle_cos = cos;
}

static void ge_rotate_thread(void *arg)
{
    int ret = 0;
    int i = 0, num = 0, loops = 1;
    int index = 0;
    int *para = NULL;
    int circle = 0;

    int bg_fd = -1;
    int src_fd = -1;
    enum mpp_pixel_format bg_fmt = 0;
    enum mpp_pixel_format src_fmt = 0;
    struct ge_buf *bg_buffer = NULL;
    struct ge_buf *src_buffer = NULL;
    struct bmp_header bg_head = {0};
    struct bmp_header src_head = {0};

    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_rotation rot = {0};
    struct ge_fb_info *fb_info = NULL;

    para = arg;
    circle = para[PARA_CIR];
    if (para[PARA_NUM] >= 0)
        loops = para[PARA_NUM];


    ge = mpp_ge_open();
    if (!ge) {
        LOGE("open ge device error\n");
        goto ROTATION_OUT;
    }

    fb_info = fb_open();

    bg_fd = bmp_open(g_bg_input, &bg_head);
    if (bg_fd < 0) {
        LOGE("open bmp error, path = %s\n", g_bg_input);
        goto ROTATION_OUT;
    }

    src_fd = bmp_open(g_src_input, &src_head);
    if (src_fd < 0) {
        LOGE("open bmp error, path = %s\n", g_src_input);
        goto ROTATION_OUT;
    }

    bg_fmt = bmp_get_fmt(&bg_head);
    bg_buffer = ge_buf_malloc(bg_head.width, abs(bg_head.height), bg_fmt);
    if (bg_buffer == NULL) {
        LOGE("malloc bg buffer error\n");
        goto ROTATION_OUT;
    }

    src_fmt = bmp_get_fmt(&src_head);
    src_buffer = ge_buf_malloc(src_head.width, abs(src_head.height), src_fmt);
    if (src_buffer == NULL) {
        LOGE("malloc src buffer error\n");
        goto ROTATION_OUT;
    }

    ret = bmp_read(bg_fd, (void *)((uintptr_t)bg_buffer->buf.phy_addr[0]), &bg_head);
    if (ret < 0) {
        LOGE("bg bmp_read error\n");
        goto ROTATION_OUT;
    }
    ge_buf_clean_dcache(bg_buffer);

    ret = bmp_read(src_fd, (void *)((uintptr_t)src_buffer->buf.phy_addr[0]), &src_head);
    if (ret < 0) {
        LOGE("src bmp_read error\n");
        goto ROTATION_OUT;
    }
    ge_buf_clean_dcache(src_buffer);

    while (1) {
        // LOGI("num : %d\n", num);
        draw_clock(&blt, fb_info, bg_buffer, &bg_head);

        ret = mpp_ge_bitblt(ge, &blt);
        if (ret < 0) {
            LOGE("ge blt fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }

        move_second_hand(&rot, fb_info, src_buffer,
                    degree_list[i * 2],
                    degree_list[i * 2 + 1]);

        ret = mpp_ge_rotate(ge, &rot);
        if (ret < 0) {
            LOGE("ge rotate fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }

        fb_start_and_wait(fb_info);
        fb_swap_frame(fb_info);
        //LOGI("index : %d\n", index);

        i++;
        if (i == 61) {
            i = 0;
        }
        index = !index;
        num++;

        if (circle == 0) {
            if (num == loops)
                break;
        }
        aicos_msleep(1000);
    }

    if (fb_info->swap_flag == 0) {
        fb_start_and_wait(fb_info);
    }

    printf("ge rotate test success\n");
ROTATION_OUT:
    if (bg_fd > 0)
        bmp_close(bg_fd);
    if (src_fd > 0)
        bmp_close(src_fd);

    if (ge)
        mpp_ge_close(ge);

    if (fb_info)
        fb_close(fb_info);

    if (bg_buffer)
        ge_buf_free(bg_buffer);
    if (src_buffer)
        ge_buf_free(src_buffer);
}

static void ge_rotate(int argc, char **argv)
{
    aicos_thread_t thid = NULL;

    int ret = 0;
    int para[2] = {0};
    const char sopts[] = "ucn:i:o:";
    const struct option lopts[] = {
        {"usage",      no_argument, NULL, 'u'},
        {"circle",     no_argument, NULL, 'c'},
        {"src_input",  required_argument, NULL, 'i'},
        {"out_input",  required_argument, NULL, 'o'},
        {"number",     required_argument, NULL, 'n'},
        {0, 0, 0, 0}
    };

    strncpy(g_src_input, "/sdcard/ge_test/image/base/second.bmp", sizeof(g_src_input) - 1);
    strncpy(g_bg_input, "/sdcard/ge_test/image/base/clock.bmp", sizeof(g_bg_input) - 1);
    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'c':
            para[PARA_CIR] = 1;
            break;
        case 'i':
            strncpy(g_src_input, optarg, sizeof(g_src_input) - 1);
            break;
        case 'o':
            strncpy(g_bg_input, optarg, sizeof(g_bg_input) - 1);
            break;
        case 'n':
            if (str2int(optarg) <= 0) {
                LOGE("Invalid parameter: %#x\n", ret);
                return;
            }
            para[PARA_NUM] = str2int(optarg);
            break;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    thid = aicos_thread_create("ge_rotate", 8096, 0, ge_rotate_thread, para);
    if (thid == NULL) {
        LOGE("Failed to create thread\n");
        return;
    }
}
MSH_CMD_EXPORT_ALIAS(ge_rotate, ge_rotate, ge rotate test);

static void ge_bitblt(int argc, char **argv)
{
    int ret = -1, num = 0, loops = 1, delay = 1000;
    struct mpp_ge *ge = NULL;
    struct ge_bitblt blt = {0};
    struct ge_fb_info *fb_info = NULL;
    struct timespec begin, now;
    struct bmp_header bmp_head = {0};
    enum mpp_pixel_format bmp_fmt = 0;
    struct ge_buf *bmp_buffer = NULL;
    int bmp_fd = -1;

    int circle = 0;
    const char sopts[] = "ucn:d:i:";
    const struct option lopts[] = {
        {"usage",     no_argument, NULL,       'u'},
        {"circle",    no_argument, NULL,       'c'},
        {"src_input", required_argument, NULL, 'i'},
        {"number",    required_argument, NULL, 'n'},
        {"delay",     required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };

    strncpy(g_src_input, "/sdcard/ge_test/image/base/clock.bmp", sizeof(g_src_input) - 1);
    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'c':
            circle = 1;
            break;
        case 'n':
            if (str2int(optarg) <= 0) {
                LOGE("Invalid parameter: %#x\n", ret);
                return;
            }
            loops = str2int(optarg);
            break;
        case 'd':
            delay = str2int(optarg);
            continue;
        case 'i':
            strncpy(g_src_input, optarg, sizeof(g_src_input) - 1);
            break;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    show_ge_freq();

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("ge open fail\n");
        return;
    }

    fb_info = fb_open();
    if (!fb_info) {
        LOGE("fb_open fail\n");
        goto BITBLT_OUT;
    }

    bmp_fd = bmp_open(g_src_input, &bmp_head);
    if (bmp_fd < 0) {
        LOGE("open bmp error, path = %s\n", g_src_input);
        goto BITBLT_OUT;
    }
    bmp_fmt = bmp_get_fmt(&bmp_head);
    bmp_buffer = ge_buf_malloc(bmp_head.width, abs(bmp_head.height), bmp_fmt);
    if (bmp_buffer == NULL) {
        LOGE("malloc bmp_buffer error\n");
        goto BITBLT_OUT;
    }

    ret = bmp_read(bmp_fd, (void *)((uintptr_t)bmp_buffer->buf.phy_addr[0]), &bmp_head);
    if (ret < 0) {
        LOGE("bmp_read error\n");
        ge_buf_free(bmp_buffer);
        goto BITBLT_OUT;
    }
    ge_buf_clean_dcache(bmp_buffer);

    gettimespec(&begin);
    while (1) {
        LOGD("num: %d\n", num);
        memset(&blt, 0, sizeof(struct ge_bitblt));

        /* source buffer */
        memcpy(&blt.src_buf, &bmp_buffer->buf, sizeof(struct mpp_buf));

        /* dstination buffer */
        blt.dst_buf.buf_type = MPP_PHY_ADDR;
        blt.dst_buf.phy_addr[0] = (unsigned long)fb_info->fb_data.framebuffer;
        blt.dst_buf.stride[0] = fb_info->fb_data.stride;
        blt.dst_buf.size.width = fb_info->fb_data.width;
        blt.dst_buf.size.height = fb_info->fb_data.height;
        blt.dst_buf.format = fb_info->fb_data.format;

        blt.dst_buf.crop_en = 1;
        blt.dst_buf.crop.x = 0;
        blt.dst_buf.crop.y = 0;
        blt.dst_buf.crop.width = bmp_head.width;
        blt.dst_buf.crop.height = abs(bmp_head.height);

        printf("width = %d, height = %d\n", blt.dst_buf.crop.width, blt.dst_buf.crop.height);

        ret =  mpp_ge_bitblt(ge, &blt);
        if (ret < 0) {
            LOGE("ge bitblt fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }
        num++;
        if (delay)
            aicos_msleep(delay);

        if (num && (num % 2000 == 0)) {
            gettimespec(&now);
            show_fps("GE bitblt", &begin, &now, 2000);
            gettimespec(&begin);
        }

        if (circle == 0) {
            if(num == loops)
                break;
        }
    }

    printf("ge bitblt test success\n");
BITBLT_OUT:
    if (ge)
        mpp_ge_close(ge);
    if (fb_info)
        fb_close(fb_info);
    if (bmp_buffer)
        ge_buf_free(bmp_buffer);
    if (bmp_fd > 0)
        bmp_close(bmp_fd);
    return;
}
MSH_CMD_EXPORT_ALIAS(ge_bitblt, ge_bitblt, ge bitblit test);

static void ge_fillrect(int argc, char **argv)
{
    int ret = -1, num = 0, loops = 1, delay = 200;
    int index = 0;
    struct mpp_ge *ge = NULL;
    struct mpp_fb *fb = NULL;
    struct ge_fillrect fill;
    int width;
    int height;
    struct timespec begin, now;
    unsigned int g_dst_phy = 0;

    int circle = 0;
    const char sopts[] = "ucn:d:";
    const struct option lopts[] = {
        {"usage",   no_argument, NULL, 'u'},
        {"circle",  no_argument, NULL, 'c'},
        {"number",  required_argument, NULL, 'n'},
        {"delay",   required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };

#ifndef AIC_PAN_DISPLAY
    LOGE("Must enable DE double-FrameBuffer in menuconfig first.\n");
    return;
#endif

    optind = 0;
    while ((ret = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (ret) {
        case 'c':
            circle = 1;
            continue;
        case 'n':
            if (str2int(optarg) <= 0) {
                LOGE("Invalid parameter: %#x\n", ret);
                return;
            }
            loops = str2int(optarg);
            continue;
        case 'd':
            delay = str2int(optarg);
            continue;
        case 'u':
            usage(argv[0]);
            return;
        default:
            LOGE("Invalid parameter: %#x\n", ret);
            return;
        }
    }

    show_ge_freq();

    fb = mpp_fb_open();
    if (!fb) {
        LOGE("open mpp fb failed\n");
        return;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &g_info);
    if (ret) {
        LOGE("get screen info failed\n");
        goto out;
    }

    g_dst_phy = (unsigned long)g_info.framebuffer;
    width = g_info.width;
    height = g_info.height;
    LOGI("FB: 0x%08x, width: %d, height: %d\n", g_dst_phy, width, height);

    ge = mpp_ge_open();
    if (!ge) {
        LOGE("ge open fail\n");
        goto out;
    }

    gettimespec(&begin);
    while (1) {
        LOGD("fill: %d\n", num);
        memset(&fill, 0, sizeof(struct ge_fillrect));

        fill.type = GE_NO_GRADIENT;
        if (num%2 == 0)
            fill.start_color = 0xff11ff11;
        else
            fill.start_color = 0x11ff11ff;
        fill.end_color = 0;
        fill.dst_buf.buf_type = MPP_PHY_ADDR;

        if (!index) {
            fill.dst_buf.phy_addr[0] = g_dst_phy;
        } else {
            if (APP_FB_NUM > 1) {
                fill.dst_buf.phy_addr[0] = g_dst_phy + g_info.smem_len;
            } else {
                fill.dst_buf.phy_addr[0] = g_dst_phy;
            }
        }

        fill.dst_buf.stride[0] = g_info.stride;
        fill.dst_buf.size.width = g_info.width;
        fill.dst_buf.size.height = g_info.height;
        fill.dst_buf.format = g_info.format;
        fill.ctrl.flags = 0;
        fill.ctrl.alpha_en = 0;

        fill.dst_buf.crop_en = 0;
        fill.dst_buf.crop.x = 0;
        fill.dst_buf.crop.y = 0;
        fill.dst_buf.crop.width = width;
        fill.dst_buf.crop.height = height;

        ret =  mpp_ge_fillrect(ge, &fill);
        if (ret < 0) {
            LOGE("ge fillrect fail\n");
        }

        ret = mpp_ge_emit(ge);
        if (ret < 0) {
            LOGE("ge emit fail\n");
        }

        ret = mpp_ge_sync(ge);
        if (ret < 0) {
            LOGE("ge sync fail\n");
            break;
        }
        num++;
        if (delay)
            aicos_msleep(delay);

        if (APP_FB_NUM > 1) {
            ret = mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &index);
            if (ret == 0) {
                ret = mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, &index);
                if (ret < 0)
                    LOGE("wait for sync error\n");
            } else {
                LOGE("pan display fail\n");
            }
        }

        if (num && (num % 2000 == 0)) {
            gettimespec(&now);
            show_fps("GE fillrect", &begin, &now, 2000);
            gettimespec(&begin);
        }

        if (circle == 0) {
            if(num == loops)
                break;
        }

        index = !index;
    }

    printf("ge fill test success\n");
out:
    mpp_ge_close(ge);
    mpp_fb_close(fb);

    return;
}
MSH_CMD_EXPORT_ALIAS(ge_fillrect, ge_fill, ge fill test);
