/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _AIC_CUSTOM_COOKBOOK_UI_H
#define _AIC_CUSTOM_COOKBOOK_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

#define MAX_PATH_LEN               256
#define TEXT_LENGTH                1024
#define MAX_MAIN_INGREDIENTS       24
#define MAX_SUPPORTING_INGREDIENTS 64
#define INGREDIENTS_TEXT_LEN       64

typedef struct _cookbook_data {
    /* head */
    char cookbook_title[MAX_PATH_LEN];
    char food_path[MAX_PATH_LEN];
    char additional_info[TEXT_LENGTH];

    /* introduce */
    char food_name[MAX_PATH_LEN]; /* compatible ori image */
    char food_introduction[TEXT_LENGTH];

    /* ingredients */
    char main_ingredients[MAX_MAIN_INGREDIENTS][INGREDIENTS_TEXT_LEN];
    char main_ingredients_weight[MAX_MAIN_INGREDIENTS][INGREDIENTS_TEXT_LEN];
    char supporting_ingredients[MAX_SUPPORTING_INGREDIENTS][INGREDIENTS_TEXT_LEN];
    char supporting_ingredients_weight[MAX_SUPPORTING_INGREDIENTS][INGREDIENTS_TEXT_LEN];

    /* cooking steps */
    char cooking_steps[TEXT_LENGTH * 2];
    char note[TEXT_LENGTH * 2];

} cookbook_data_t;

lv_obj_t *cookbook_comp_create(lv_obj_t *parent);
void cookbook_comp_set_data(lv_obj_t *obj, cookbook_data_t *data);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
