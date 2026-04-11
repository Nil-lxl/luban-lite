/*
 * Copyright (C) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lang_item.h"
#include "lvgl.h"

void lang_item_init(lv_ll_t *list, uint32_t node_size)
{
    _lv_ll_init(list, node_size);
}

lang_item_t *lang_item_add(lv_ll_t *list, lv_obj_t *obj, lv_style_t *style,
                           char *key, int item_type)
{
    lang_item_t *item = (lang_item_t *)_lv_ll_ins_head(list);

    if (item) {
        item->obj = obj;
        item->style = style;
        item->key = key;
        item->item_type = item_type;
        item->last_style = NULL;
    }

    return item;
}

void lang_item_remove(lv_ll_t *list, lang_item_t *item)
{
    _lv_ll_remove(list, item);
    lv_mem_free(item);
}

void lang_item_clear(lv_ll_t *list)
{
    _lv_ll_clear(list);
}

void lang_item_update(lv_ll_t *list, multi_lang_t *multi_ctx)
{
    lang_item_t *item;

    if (!list) {
        return;
    }

    lv_style_t * cur_style;
    cur_style = multi_lang_get_cur_style(multi_ctx);
    _LV_LL_READ_BACK(list, item) {
        const char *text = multi_lang_get_text_by_key(multi_ctx, item->key);
        //TODO: should check item_type
        lv_label_set_text(item->obj, text);
        if (item->last_style) {
            lv_obj_remove_style(item->obj, item->last_style, 0);
            item->last_style = NULL;
        }

        if (item->style) {
            lv_obj_add_style(item->obj, item->style, 0);
            item->last_style = item->style;
        } else {
            if (cur_style) {
                lv_obj_add_style(item->obj, cur_style, 0);
                item->last_style = cur_style;
            }
        }
    }

    return;
}
