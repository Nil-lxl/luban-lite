/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#include "player_backend_map.h"

typedef struct backend_cma_buf {
    int fd;
    uint32_t phy_addr;
    uint8_t *data;
    int size;
} backend_cma_buf_t;

void backend_draw_color_buffer(lv_image_dsc_t *image_dst, uint32_t color);
uint32_t backend_fmt_mpp_to_lv(int cf);
int backend_fmt_lv_to_mpp(uint32_t cf);
int backend_align_stride(int width, int fmt);
int backend_get_screen_info(void *info); // info type is struct aicfb_screeninfo
int backend_dma_buf_alloc(backend_cma_buf_t *buf, uint32_t size);
void backend_dma_buf_free(backend_cma_buf_t *buf);
