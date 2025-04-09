/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "eastern_large_label_comp.h"

LV_FONT_DECLARE(eastern_large_12);
LV_FONT_DECLARE(eastern_large_24);

lv_obj_t *eastern_large_label_comp(lv_obj_t *parent, uint16_t weight)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, LV_SIZE_CONTENT);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_label_set_recolor(label, true);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_100, 0);

    switch (weight)
    {
    case 12:
        lv_obj_set_style_text_font(label, &eastern_large_12, 0);
        break;
    case 24:
        lv_obj_set_style_text_font(label, &eastern_large_24, 0);
        break;
    default:
        printf("the weight(%d) is not included, only supports 12, 20, 24, 36.\n", weight);
        break;
    }
    return label;
}
