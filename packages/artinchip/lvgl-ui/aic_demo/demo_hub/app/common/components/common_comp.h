/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#ifndef _AIC_SHOWCASE_DEMO_COMP_UI_H
#define _AIC_SHOWCASE_DEMO_COMP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

lv_obj_t *com_imgbtn_switch_comp(lv_obj_t *parent, void *img_src_0, void *img_src_1);
lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);
lv_obj_t *com_label_comp(lv_obj_t *parent);
lv_obj_t *com_empty_imgbtn_comp(lv_obj_t *parent, void *pressed_img_src);
lv_obj_t *com_empty_imgbtn_switch_comp(lv_obj_t *parent, void *img_src);
#if LV_USE_FREETYPE
lv_obj_t *ft_label_comp(lv_obj_t *parent, uint16_t weight, uint16_t style, const char *path);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
