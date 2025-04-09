/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */


#include "photo_frame_components.h"
#include "../photo_frame.h"

lv_obj_t *com_dropdown(lv_obj_t *parent, int16_t width, int16_t height, void *desc, lv_font_t *font)
{
    lv_obj_t * dropdown = lv_dropdown_create(parent);
    lv_dropdown_set_options_static(dropdown, desc);
    lv_obj_set_width(dropdown, width);
    lv_obj_set_height(dropdown, height);
    lv_obj_set_style_shadow_color(dropdown, lv_color_hex(0x0), 0);
    lv_obj_set_style_shadow_opa(dropdown, LV_OPA_50, 0);
    lv_obj_set_style_shadow_ofs_x(dropdown, 2, 0);
    lv_obj_set_style_shadow_ofs_y(dropdown, 2, 0);
    lv_obj_set_style_shadow_width(dropdown, 10, 0);

    lv_dropdown_set_symbol(dropdown, LVGL_PATH(photo_frame/dropdown_drown.png));

    /* main */
	lv_obj_set_style_text_color(dropdown, lv_color_hex(0x242412), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(dropdown, font, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(dropdown, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(dropdown, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_opa(dropdown, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(dropdown, lv_color_hex(0xA6A6A6), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(dropdown, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(dropdown, adaptive_width(12), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(dropdown, adaptive_height(10), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(dropdown, adaptive_height(8), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(dropdown, 8, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_set_style_shadow_width(dropdown, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_color(dropdown, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_opa(dropdown, 76, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_spread(dropdown, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_x(dropdown, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_ofs_y(dropdown, 4, LV_PART_MAIN|LV_STATE_DEFAULT);

    /* selected part */
	lv_obj_set_style_bg_opa(lv_dropdown_get_list(dropdown), LV_OPA_100, LV_PART_SELECTED|LV_STATE_CHECKED);
	lv_obj_set_style_bg_color(lv_dropdown_get_list(dropdown), lv_color_hex(0xA6A6A6), LV_PART_SELECTED|LV_STATE_CHECKED);

    /* list part */
	lv_obj_set_style_text_color(lv_dropdown_get_list(dropdown), lv_color_hex(0x242412), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(lv_dropdown_get_list(dropdown), font, LV_PART_MAIN|LV_STATE_DEFAULT);

    return dropdown;
}
