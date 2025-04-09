/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _AIC_CUSTOM_TABLE_UI_H
#define _AIC_CUSTOM_TABLE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

lv_obj_t *simple_table_create(lv_obj_t *parent);
/* add cell order: from left to right, upside down */
void simple_table_add(lv_obj_t *obj, char *context);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
