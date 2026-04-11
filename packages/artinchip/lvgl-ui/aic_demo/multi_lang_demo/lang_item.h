/*
 * Copyright (C) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LANG_ITEM_H
#define LANG_ITEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "multi_lang.h"

typedef struct _lang_item_t {
    lv_obj_t *obj;
    lv_style_t *style;
    lv_style_t *last_style;
    char *key;
    int item_type;
} lang_item_t;

void lang_item_init(lv_ll_t *list, uint32_t node_size);

lang_item_t *lang_item_add(lv_ll_t *list, lv_obj_t *obj, lv_style_t *style,
                           char *key, int item_type);

void lang_item_remove(lv_ll_t *list, lang_item_t *item);

void lang_item_clear(lv_ll_t * ll_p);

void lang_item_update(lv_ll_t *list, multi_lang_t *multi_ctx);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // LANG_ITEM_H
