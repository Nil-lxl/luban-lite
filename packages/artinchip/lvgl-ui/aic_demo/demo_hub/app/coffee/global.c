/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "global.h"
#include <stdlib.h>
#include <string.h>
#include "coffee.h"

struct category* categoryData;
enum coffeeType currentCoffeeType;
lv_obj_t* menuScr[10];
lv_obj_t* detailsScr;
lv_obj_t* productionScr;
lv_obj_t* loadScr;

void createTopTitle(lv_obj_t* scr)
{
#if LVGL_VERSION_MAJOR == 8
    static lv_point_t line_points[4] = { {56, 42}, {744, 42}, {351, 44}, {449, 44}};
#else
    static lv_point_precise_t line_points[4] = { {56, 42}, {744, 42}, {351, 44}, {449, 44}};
#endif
    static lv_style_t style_line;
    if (style_line.prop_cnt > 1) {
        lv_style_reset(&style_line);
    } else {
        lv_style_init(&style_line);
    }
    lv_style_set_line_width(&style_line, 2);
    lv_style_set_line_color(&style_line, lv_color_hex(0xefbe77));
    lv_style_set_line_rounded(&style_line, true);

    line_points[0].x = pos_width_conversion(56);
    line_points[0].y = pos_height_conversion(42);
    line_points[1].x = pos_width_conversion(744);
    line_points[1].y = pos_height_conversion(42);
    line_points[2].x = pos_width_conversion(351);
    line_points[2].y = pos_height_conversion(44);
    line_points[3].x = pos_width_conversion(449);
    line_points[3].y = pos_height_conversion(44);
    lv_obj_t* line = lv_line_create(scr);
    lv_obj_add_style(line, &style_line, 0);
    lv_line_set_points(line, line_points, 2);

    static lv_style_t style_title;
    if (style_title.prop_cnt > 1) {
        lv_style_reset(&style_title);
    } else {
        lv_style_init(&style_title);
    }
    lv_style_set_text_color(&style_title, lv_color_hex(0xefbe77));
    lv_obj_t * label = lv_label_create(scr);
    lv_label_set_text(label, "Coffee time");
    if (disp_size == DISP_SMALL) {
        lv_obj_set_style_text_font(label,&NotoSanCJK_Medium_16, 0);
    } else if (disp_size == DISP_MEDIUM) {
        lv_obj_set_style_text_font(label,&NotoSanCJK_Medium_18, 0);
    } else {
        lv_obj_set_style_text_font(label,&NotoSanCJK_Medium_20, 0);
    }
    lv_obj_set_x(label, pos_width_conversion(350));
    lv_obj_set_y(label, pos_height_conversion(15));
    lv_obj_add_style(label, &style_title, 0);
}

void globalSrcDel(void)
{
    if (loadScr != detailsScr && detailsScr)
        lv_obj_del(detailsScr);
    if (loadScr != productionScr && productionScr)
        lv_obj_del(productionScr);

    for (int i = 0; i < 10; i++)
    {
        if (loadScr != menuScr[i] && menuScr[i])
            lv_obj_del(menuScr[i]);
    }
}
