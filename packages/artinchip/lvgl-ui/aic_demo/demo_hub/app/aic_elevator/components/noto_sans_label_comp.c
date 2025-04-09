/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "noto_sans_label_comp.h"

LV_FONT_DECLARE(noto_sans_medium_14);
LV_FONT_DECLARE(noto_sans_medium_36);

static lv_obj_t *normal_label_comp(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, LV_SIZE_CONTENT);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_label_set_recolor(label, true);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_100, 0);
    return label;
}

lv_obj_t *nonto_sans_media_label_comp(lv_obj_t *parent, uint16_t weight)
{
    lv_obj_t *label = normal_label_comp(parent);

    switch (weight)
    {
    case 14:
        lv_obj_set_style_text_font(label, &noto_sans_medium_14, 0);
        break;
    case 36:
        lv_obj_set_style_text_font(label, &noto_sans_medium_36, 0);
        break;
    default:
        printf("nonto_sans_media the weight(%d) is not included, only supports 14, 34, 36.\n", weight);
        break;
    }
    return label;
}
