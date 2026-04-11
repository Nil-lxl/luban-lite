/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
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

#include "mpp_ge.h"
#include "bmp.h"
#include "ge_mem.h"

#define PI 3.14159265
#define SIN(x) (sin((x) * PI / 180.0))
#define COS(x) (cos((x) * PI / 180.0))

static void ge_process_rotate_set(struct ge_rotation *rot, struct ge_buf *src_buf, struct ge_buf *dst_buf)
{
    double degree = 0.0;

    memset(rot, 0, sizeof(struct ge_rotation));

    memcpy(&rot->src_buf, &src_buf->buf, sizeof(struct mpp_buf));
    rot->src_rot_center.x = 0;
    rot->src_rot_center.y = 0;

    memcpy(&rot->dst_buf, &dst_buf->buf, sizeof(struct mpp_buf));
    rot->dst_rot_center.x = 0;
    rot->dst_rot_center.y = 0;

    degree = 180.0;
    rot->angle_sin = (int)(SIN(degree) * 4096);
    rot->angle_cos = (int)(COS(degree) * 4096);
}

int ge_rotate_simple_test(int argc, char **argv)
{
    int ret = -1;

    int map_size = -1;
    unsigned char *mem_map = NULL;
    struct mpp_ge *ge = NULL;
    struct ge_rotation rot = {0};
    struct ge_buf *src_buf = NULL;
    struct ge_buf *dst_buf = NULL;

    ge = mpp_ge_open();
    if (!ge) {
        printf("open ge device error\n");
        goto EXIT;
    }


    src_buf = ge_buf_malloc(10, 10, MPP_FMT_ARGB_8888);
    if (src_buf == NULL) {
        printf("malloc src_buf error\n");
        goto EXIT;
    }
    map_size = src_buf->buf.stride[0] * src_buf->buf.size.height;
    mem_map = (unsigned char *)(uintptr_t)src_buf->buf.phy_addr[0];
    memset(mem_map, 0x0, map_size);
    for (int col = 0; col < src_buf->buf.stride[0]; col += 4) {
        unsigned int *pixel = (unsigned int *)(mem_map + col + 0 * src_buf->buf.stride[0]);
        *pixel = 0xffffffff;
    }

    dst_buf = ge_buf_malloc(10, 10, MPP_FMT_ARGB_8888);
    if (dst_buf == NULL) {
        printf("malloc dst_buf error\n");
        goto EXIT;
    }

    map_size = dst_buf->buf.stride[0] * dst_buf->buf.size.height;
    mem_map = (unsigned char *)(uintptr_t)dst_buf->buf.phy_addr[0];
    memset(mem_map, 0x0, map_size);
    for (int col = 0; col < dst_buf->buf.stride[0]; col += 4) {
        unsigned int *pixel = (unsigned int *)(mem_map + col + 1 * dst_buf->buf.stride[0]);
        *pixel = 0xffffffff;
    }

    printf("rotate before\n");
    ge_buf_argb8888_printf(mem_map, dst_buf);

    ge_process_rotate_set(&rot, src_buf, dst_buf);

    aicos_dcache_clean_invalid_range((void *)((uintptr_t)(unsigned char *)(uintptr_t)src_buf->buf.phy_addr[0]), map_size);
    aicos_dcache_clean_invalid_range((void *)((uintptr_t)(unsigned char *)(uintptr_t)dst_buf->buf.phy_addr[0]), map_size);

    ret =  mpp_ge_rotate(ge, &rot);
    if (ret < 0) {
        printf("ge rotate fail\n");
        goto EXIT;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        printf("ge emit fail\n");
        goto EXIT;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        printf("ge sync fail\n");
    }

    printf("rotate after\n");
    ge_buf_argb8888_printf(mem_map, dst_buf);
EXIT:
    if (src_buf)
        ge_buf_free(src_buf);

    if (dst_buf)
        ge_buf_free(dst_buf);

    if (ge)
        mpp_ge_close(ge);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(ge_rotate_simple_test, ge_rotate_simple_test, ge simple rot test);
