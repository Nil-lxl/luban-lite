/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"

#if LVGL_VERSION_MAJOR == 8
#define lv_image_dsc_t lv_img_dsc_t
#endif

void backend_draw_color_buffer(lv_image_dsc_t *image_dst, uint32_t color);
uint32_t backend_fmt_mpp_to_lv(int cf);
int backend_fmt_lv_to_mpp(uint32_t cf);
int backend_align_stride(int width, int fmt);
int backend_get_screen_info(void *info); // info type is struct aicfb_screeninfo
