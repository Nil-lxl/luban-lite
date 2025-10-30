/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "hor_slide_screen.h"

static lv_obj_t * cont_col = NULL;

static lv_coord_t cal_cur_x(lv_coord_t x, lv_coord_t parent_w, lv_coord_t child_w)
{
    return (lv_coord_t)((float)x * child_w / parent_w);
}

static void h_on_scroll(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t *cont = lv_event_get_target(e);
    lv_obj_t *user = lv_event_get_user_data(e);
    lv_coord_t parent_w = lv_obj_get_width(cont_col);
    lv_coord_t child_w = lv_obj_get_width(user);

    if (event_code == LV_EVENT_SCROLL_BEGIN) {
        //lv_coord_t x = lv_obj_get_scroll_x(cont);
        //printf("LV_EVENT_SCROLL_BEGIN, scroll_x:%d\n", x);
    } else if (event_code == LV_EVENT_SCROLL_END) {
        //lv_coord_t x = lv_obj_get_scroll_x(cont);
        //printf("LV_EVENT_SCROLL_END, scroll_x:%d\n", x);
    } else if (event_code == LV_EVENT_SCROLL) {
        lv_coord_t x = lv_obj_get_scroll_x(cont);
        lv_obj_align_to(user, cont_col, LV_ALIGN_TOP_LEFT, cal_cur_x(x, parent_w, child_w), 0);
    }
}

lv_obj_t *hor_screen_init(void)
{
    // Create a container with COLUMN flex direction
    lv_obj_t *scr = lv_obj_create(NULL);
    cont_col = lv_obj_create(scr);
    lv_obj_remove_style_all(cont_col);
    lv_obj_set_size(cont_col, 240, 450);
    lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);

    // Create a container with ROW flex direction
    lv_obj_t * cont_row = lv_obj_create(cont_col);
    lv_obj_remove_style_all(cont_row);
    lv_obj_set_size(cont_row, 240, 50);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Add items to the row
    lv_obj_t * label_con1 = lv_obj_create(cont_row);
    lv_obj_remove_style_all(label_con1);
    lv_obj_set_size(label_con1, 80, LV_PCT(100));
    lv_obj_t *label1 = lv_label_create(label_con1);
    lv_label_set_text_fmt(label1, "Following");
    lv_obj_set_size(label1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label1);

    lv_obj_t * label_con2 = lv_obj_create(cont_row);
    lv_obj_remove_style_all(label_con2);
    lv_obj_set_size(label_con2, 80, LV_PCT(100));
    lv_obj_t *label2 = lv_label_create(label_con2);
    lv_label_set_text_fmt(label2, "For you");
    lv_obj_set_size(label2, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label2);

    lv_obj_t * label_con3 = lv_obj_create(cont_row);
    lv_obj_remove_style_all(label_con3);
    lv_obj_set_size(label_con3, 80, LV_PCT(100));
    lv_obj_t *label3 = lv_label_create(label_con3);
    lv_label_set_text_fmt(label3, "Near you");
    lv_obj_set_size(label3, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(label3, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label3);

#if LVGL_VERSION_MAJOR == 8
    lv_obj_t *main_tabview = lv_tabview_create(cont_col, LV_DIR_TOP, 0);
#else
    lv_obj_t *main_tabview = lv_tabview_create(cont_col);

    lv_tabview_set_tab_bar_position(main_tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(main_tabview, 0);
#endif
    lv_obj_set_size(main_tabview, LV_PCT(100), 400);
    lv_obj_set_pos(main_tabview, 0, 0);
    lv_obj_set_style_bg_opa(main_tabview, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(main_tabview, 0, 0);
    lv_obj_t *main_tab0 = lv_tabview_add_tab(main_tabview, "Following");
    lv_obj_t *main_tab1 = lv_tabview_add_tab(main_tabview, "For you");
    lv_obj_t *main_tab2 = lv_tabview_add_tab(main_tabview, "Near you");

    lv_obj_set_style_bg_opa(main_tab0, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(main_tab1, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(main_tab2, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(main_tab0, 0, 0);
    lv_obj_set_style_pad_all(main_tab1, 0, 0);
    lv_obj_set_style_pad_all(main_tab2, 0, 0);
    lv_obj_set_pos(main_tab0, 0, 0);
    lv_obj_set_pos(main_tab1, 0, 0);
    lv_obj_set_pos(main_tab2, 0, 0);

    lv_obj_t * sub_con0 = lv_obj_create(main_tab0);
    lv_obj_remove_style_all(sub_con0);
    lv_obj_set_flex_flow(sub_con0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(sub_con0, LV_PCT(100), LV_PCT(100));
    lv_obj_t *sub_image0 = lv_img_create(sub_con0);
    lv_img_set_src(sub_image0, LVGL_PATH(follow.png));
    lv_obj_set_pos(sub_image0, 0, 0);

    lv_obj_t * sub_label0 = lv_label_create(sub_con0);
    lv_label_set_text(sub_label0, "     Please follow me\n");

    lv_obj_t * sub_con1 = lv_obj_create(main_tab1);
    lv_obj_remove_style_all(sub_con1);
    lv_obj_set_flex_flow(sub_con1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(sub_con1, LV_PCT(100), LV_PCT(100));
    lv_obj_t *sub_image1 = lv_img_create(sub_con1);
    lv_img_set_src(sub_image1, LVGL_PATH(you.png));
    lv_obj_set_pos(sub_image1, 0, 0);
    lv_obj_t * sub_label1 = lv_label_create(sub_con1);
    lv_label_set_text(sub_label1, "     This is for you\n");

    lv_obj_t * sub_con2 = lv_obj_create(main_tab2);
    lv_obj_remove_style_all(sub_con2);
    lv_obj_set_flex_flow(sub_con2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(sub_con2, LV_PCT(100), LV_PCT(100));
    lv_obj_t *sub_image2 = lv_img_create(sub_con2);
    lv_img_set_src(sub_image2, LVGL_PATH(near.png));
    lv_obj_set_pos(sub_image2, 0, 0);

    lv_obj_t * sub_label2 = lv_label_create(sub_con2);
    lv_label_set_text(sub_label2, "     I'm near You\n");

    lv_obj_t * corner = lv_obj_create(scr);
    lv_obj_remove_style_all(corner);
    lv_obj_set_size(corner, 80, 50);
    lv_obj_set_style_bg_color(corner, lv_color_hex(0xCFD0CF), 0);
    lv_obj_set_style_bg_opa(corner, LV_OPA_50, 0);
    lv_obj_set_style_radius(corner, 30, 0);

    lv_obj_center(cont_col);
    lv_obj_align_to(corner, cont_col, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(lv_tabview_get_content(main_tabview) , h_on_scroll, LV_EVENT_ALL, corner);

    return scr;
}
