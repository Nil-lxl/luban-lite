/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdio.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "lv_aic_img_dsc.h"

#if LVGL_VERSION_MAJOR > 8
static lv_image_dsc_t * img_dsc = NULL;
#endif

void aic_img_usage(void)
{
    lv_obj_t *img_normal = lv_img_create(lv_scr_act());
    lv_img_set_src(img_normal, LVGL_IMAGE_PATH(00.jpg));
    lv_obj_set_pos(img_normal, 0, 0);

    /* decode the image directly*/
#if LVGL_VERSION_MAJOR > 8
    img_dsc = lv_aic_img_dsc_create(LVGL_IMAGE_PATH(00.jpg));
    if (img_dsc) {
        lv_obj_t *img_direct = lv_img_create(lv_scr_act());
        lv_img_set_src(img_direct, img_dsc);
        lv_obj_align(img_direct, LV_ALIGN_CENTER, 0, 0);
    }
#endif
}
