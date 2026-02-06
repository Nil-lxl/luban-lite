/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"

#if LVGL_VERSION_MAJOR == 8
#define LV_DISP_ROTATION_0      LV_DISP_ROT_0
#define LV_DISP_ROTATION_90     LV_DISP_ROT_90
#define LV_DISP_ROTATION_180    LV_DISP_ROT_180
#define LV_DISP_ROTATION_270    LV_DISP_ROT_270

#define LV_IMAGE_SRC_FILE       LV_IMG_SRC_FILE
#define lv_disp_rotation_t      lv_disp_rot_t

#define lv_malloc               lv_mem_alloc
#define lv_free                 lv_mem_free

#define lv_image_t              lv_img_t
#define lv_image_dsc_t          lv_img_dsc_t
#define lv_image_src_t          lv_img_src_t
#define lv_image_src_get_type   lv_img_src_get_type
#define lv_image_align_t        uint8_t

#define lv_image_set_src        lv_img_set_src
#define lv_image_get_src        lv_img_get_src
#define lv_image_src_get_type   lv_img_src_get_type
#define lv_image_set_pivot      lv_img_set_pivot
#define lv_image_get_pivot      lv_img_get_pivot
#define lv_image_set_scale       lv_img_set_zoom
#define lv_image_get_scale       lv_img_get_zoom
#define lv_image_set_rotation   lv_img_set_angle
#define lv_image_get_rotation   lv_img_get_angle
#define lv_image_set_offset_x   lv_img_set_offset_x
#define lv_image_get_offset_y   lv_img_get_offset_y
#define lv_image_set_offset_y   lv_img_set_offset_y
#define lv_image_cache_drop     lv_img_cache_invalidate_src

#define lv_obj_send_event       lv_event_send

#define lv_result_t             lv_res_t
#define LV_RESULT_OK            LV_RES_OK

#define lv_timer_delete         lv_timer_del

#define lv_realloc              lv_mem_realloc

void *lv_malloc_zeroed(size_t size);
#endif
