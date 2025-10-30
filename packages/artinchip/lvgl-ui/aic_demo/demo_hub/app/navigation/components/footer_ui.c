/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "footer_ui.h"
#include "string.h"

typedef enum {
    DISP_SMALL = 1,
    DISP_MEDIUM = 2,
    DISP_LARGE = 3,
} disp_size_t;

#define CAL_PROPORTIONS(X) ((X / 480) * 10)

lv_obj_t * footer_ui_create(lv_obj_t *parent)
{
    static disp_size_t disp_size = DISP_SMALL;

    if(LV_HOR_RES <= 480) disp_size = DISP_SMALL;
    else if(LV_HOR_RES <= 800) disp_size = DISP_SMALL;
    else disp_size = DISP_SMALL;

    lv_obj_t *footer = lv_obj_create(parent);
    lv_obj_set_size(footer, disp_size * CAL_PROPORTIONS(480), disp_size * CAL_PROPORTIONS(480));
    lv_obj_set_style_bg_color(footer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_10, 0);
    lv_obj_set_style_clip_corner(footer, 255, 0);
    lv_obj_set_style_radius(footer, 5, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(footer, disp_size * CAL_PROPORTIONS(480) / 2, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_INTERNAL, 0);

    return footer;
}

void footer_ui_switch(lv_obj_t *obj, int act)
{
    if (obj == NULL) {
        return;
    }

    if (act == FOOTER_TURN_OFF) {
        lv_obj_set_style_bg_opa(obj, LV_OPA_10, 0);
    } else {
        lv_obj_set_style_bg_opa(obj, LV_OPA_100, 0);
    }
}
