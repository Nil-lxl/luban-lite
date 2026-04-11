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

#include "mpp_ge.h"
#include "bmp.h"
#include "ge_mem.h"

static void ge_process_fill_set(struct ge_fillrect *fill, struct ge_buf *src_buf)
{
    memset(fill, 0, sizeof(struct ge_fillrect));

    memcpy(&fill->dst_buf, &src_buf->buf, sizeof(struct mpp_buf));

    fill->type = GE_NO_GRADIENT;
    fill->start_color = 0xffffffff;
}

static int ge_fillrect_simple_test(int argc, char **argv)
{
    int ret = -1;
    int map_size = -1;
    struct mpp_ge *ge = NULL;
    struct ge_buf *src_buf = NULL;
    unsigned char *mem_map = NULL;
    struct ge_fillrect fill = {0};

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

    printf("fill before:\n");
    ge_buf_argb8888_printf(mem_map, src_buf);

    ge_process_fill_set(&fill, src_buf);

    aicos_dcache_clean_invalid_range((void *)((uintptr_t)(unsigned char *)(uintptr_t)src_buf->buf.phy_addr[0]), map_size);
    ret =  mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        printf("ge fillrect fail\n");
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

    printf("fill after:\n");
    ge_buf_argb8888_printf(mem_map, src_buf);

EXIT:
    if (src_buf)
        ge_buf_free(src_buf);

    if (ge)
        mpp_ge_close(ge);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(ge_fillrect_simple_test, ge_fillrect_simple_test, ge simple fill test);
