/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _AIC_CUSTOM_LIST_UI_H
#define _AIC_CUSTOM_LIST_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

lv_obj_t *simple_list_create(lv_obj_t *parent);
void simple_list_add(lv_obj_t *obj, char *context);

lv_obj_t *simple_list_grid_create(lv_obj_t *parent);
void simple_list_grid_add(lv_obj_t *obj, char *context);

lv_obj_t *complex_list_create(lv_obj_t *parent);
void complex_list_add(lv_obj_t *obj, char *context, int btn_num);

lv_obj_t *complex_grid_list_create(lv_obj_t *parent);
void complex_grid_list_add(lv_obj_t *obj, char *context_title, char *context_title_desc);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
