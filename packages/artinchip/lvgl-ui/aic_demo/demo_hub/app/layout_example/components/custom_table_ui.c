/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "demo_hub.h"
#include "custom_table_ui.h"
#include "string.h"
#include "layout_table_ui.h"

#define MAX_PAGE_TABLE_CELL 8
#define FOOTER_TURN_ON  1
#define FOOTER_TURN_OFF 0

#define CAL_PROPORTIONS(X) ((X / 480) * 10)

static int now_cell_num = 0, tv_sub_create = -1, now_tv_sub = 0;

static lv_coord_t table_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
static lv_coord_t table_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

static lv_obj_t *table_tv_sub_create(lv_obj_t *obj);
static lv_obj_t *footer_create(lv_obj_t *obj);
static lv_obj_t *table_cell_create(lv_obj_t *obj, char *context);

static void act_sub_tabview(lv_event_t * e);
static void del_table_cell_cb(lv_event_t * e);
static void update_table_cell(lv_event_t * e);

static lv_obj_t *footer_switch(lv_obj_t *obj, int act);

lv_obj_t *simple_table_create(lv_obj_t *parent)
{
    lv_obj_t *table = lv_obj_create(parent);
    lv_obj_remove_style_all(table);
    lv_obj_set_size(table, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(table, LV_OPA_0, 0);

    lv_obj_t *title_container = lv_obj_create(table);
    lv_obj_remove_style_all(title_container);
    lv_obj_set_size(title_container, LV_HOR_RES, LV_VER_RES * 0.15);
    lv_obj_set_style_bg_opa(title_container, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(title_container, lv_theme_get_color_primary(NULL), 0);
    lv_obj_set_align(title_container, LV_ALIGN_TOP_LEFT);

    lv_obj_t *img_label = lv_img_create(title_container);
    lv_img_set_src(img_label, LVGL_PATH(layout_example/custom_table.png));
    lv_obj_set_align(img_label, LV_ALIGN_CENTER);

    lv_obj_t *trash = lv_img_create(title_container);
    lv_img_set_src(trash, LVGL_PATH(layout_example/trash.png));
    lv_obj_set_pos(trash, -LV_HOR_RES * 0.05, 0);
    lv_obj_set_align(trash, LV_ALIGN_RIGHT_MID);
    lv_obj_add_flag(trash, LV_OBJ_FLAG_CLICKABLE);

#if LVGL_VERSION_MAJOR == 8
    lv_obj_t *table_tv = lv_tabview_create(table, LV_DIR_TOP, 0);
#else
    lv_obj_t *table_tv = lv_tabview_create(table);
    lv_tabview_set_tab_bar_position(table_tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(table_tv, 0);
#endif
    lv_obj_set_size(table_tv, LV_HOR_RES, LV_VER_RES * 0.85);
    lv_obj_set_pos(table_tv, 0, LV_VER_RES * 0.15);
    lv_obj_set_align(table_tv, LV_ALIGN_TOP_LEFT);

    lv_obj_t *footer_container = lv_obj_create(table_tv);
    lv_obj_remove_style_all(footer_container);
    lv_obj_set_style_bg_opa(footer_container, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(footer_container, lv_theme_get_color_emphasize(NULL), 0);
    lv_obj_set_size(footer_container, LV_HOR_RES, LV_VER_RES * 0.10);
    lv_obj_set_align(footer_container, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(footer_container, 0, LV_VER_RES * 0.10);
    lv_obj_set_flex_flow(footer_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer_container, LV_VER_RES * 0.07, 0);

    lv_obj_add_event_cb(table_tv, act_sub_tabview, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(trash, update_table_cell, LV_EVENT_ALL, table);

    now_cell_num = 0; /* Count the number of added tables */
    tv_sub_create = -1; /* Used to determine whether deletion should be done */
    now_tv_sub = 0;  /* Now the subtab view location */

    return table;
}

void simple_table_add(lv_obj_t *obj, char *context)
{
    static lv_obj_t *table_tv_sub = NULL;
    int row = -1, col = -1;
    lv_color_t bg_color;

    lv_obj_t *table_tv = lv_obj_get_child(obj, -1);
    lv_obj_t *footer_container = lv_obj_get_child(table_tv, -1);

    row = (now_cell_num % 8) / 2;
    col = (now_cell_num % 8) % 2;
    now_tv_sub = now_cell_num / MAX_PAGE_TABLE_CELL;

    if (tv_sub_create != now_tv_sub) {
        table_tv_sub = table_tv_sub_create(table_tv);
        footer_create(footer_container);
#if LVGL_VERSION_MAJOR == 8
        lv_event_send(table_tv, LV_EVENT_VALUE_CHANGED, NULL);
#else
        lv_obj_send_event(table_tv, LV_EVENT_VALUE_CHANGED, NULL);
#endif
        tv_sub_create = now_tv_sub;
    }

    if (row % 2)
        bg_color = lv_theme_get_color_primary(NULL);
    else
        bg_color = lv_theme_get_color_secondary(NULL);

    lv_obj_t *table_cell_container = lv_obj_create(table_tv_sub);
    lv_obj_remove_style_all(table_cell_container);
    lv_obj_set_style_bg_opa(table_cell_container, LV_OPA_100, 0);
    lv_obj_set_style_opa(table_cell_container, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(table_cell_container, bg_color, 0);
    lv_obj_set_grid_cell(table_cell_container, LV_GRID_ALIGN_STRETCH, col, 1,
                         LV_GRID_ALIGN_STRETCH, row, 1);

    table_cell_create(table_cell_container, context);

    now_cell_num++;
}

static lv_obj_t *table_tv_sub_create(lv_obj_t *obj)
{
    lv_obj_t *tv_tab = lv_tabview_add_tab(obj, "tv_tab_name");
    lv_obj_remove_style_all(tv_tab);
    lv_obj_set_style_bg_opa(tv_tab, LV_OPA_0, 0);
    lv_obj_set_size(tv_tab, lv_pct(100), lv_pct(100));
    lv_obj_set_style_grid_row_dsc_array(tv_tab, table_row_dsc, 0);
    lv_obj_set_style_grid_column_dsc_array(tv_tab, table_col_dsc, 0);
    lv_obj_set_layout(tv_tab, LV_LAYOUT_GRID);

    return tv_tab;
}

static lv_obj_t *table_cell_create(lv_obj_t *obj, char *text)
{
    lv_obj_t *label_img = lv_img_create(obj);
    lv_img_set_src(label_img, LVGL_PATH(layout_example/label.png));
    lv_obj_set_align(label_img, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(label_img, LV_HOR_RES * 0.03, 0);

    lv_obj_t *label = lv_label_create(obj);
    if (text == NULL)
        lv_label_set_text(label, "i am the text");
    else
        lv_label_set_text(label, text);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_pos(label, 0, 0);

    lv_obj_t *rect = lv_img_create(obj);
    lv_img_set_src(rect, LVGL_PATH(layout_example/rect.png));
    lv_obj_add_flag(rect, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(rect, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(rect, -LV_HOR_RES * 0.03, 0);

    lv_obj_t *hor_line = lv_obj_create(obj);
    lv_obj_remove_style_all(hor_line);
    lv_obj_set_style_bg_color(hor_line, lv_theme_get_color_emphasize(NULL), 0);
    lv_obj_set_style_bg_opa(hor_line, LV_OPA_100, 0);
    lv_obj_set_size(hor_line, lv_pct(60), 1);
    lv_obj_set_align(hor_line, LV_ALIGN_BOTTOM_MID);

    lv_obj_t *ver_line = lv_obj_create(obj);
    lv_obj_remove_style_all(ver_line);
    lv_obj_set_style_bg_color(ver_line, lv_theme_get_color_emphasize(NULL), 0);
    lv_obj_set_style_bg_opa(ver_line, LV_OPA_100, 0);
    lv_obj_set_size(ver_line, 1, lv_pct(60));
    lv_obj_set_align(ver_line, LV_ALIGN_RIGHT_MID);

    lv_obj_add_event_cb(rect, del_table_cell_cb, LV_EVENT_ALL, label);

    return NULL;
}

static lv_obj_t *footer_create(lv_obj_t *obj)
{
    lv_obj_t *footer = lv_obj_create(obj);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(footer, 1.5 * CAL_PROPORTIONS(480), 1.5 * CAL_PROPORTIONS(480));
    lv_obj_set_style_bg_color(footer, lv_theme_get_color_emphasize(NULL), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_50, 0);
    lv_obj_set_style_clip_corner(footer, 255, 0);
    lv_obj_set_style_radius(footer, 255, 0);
    lv_obj_set_style_border_color(footer, lv_theme_get_color_emphasize(NULL), 0);
    lv_obj_set_style_border_width(footer, 1.5 * CAL_PROPORTIONS(480) / 2, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_INTERNAL, 0);

    return NULL;
}

static lv_obj_t *footer_switch(lv_obj_t *obj, int act)
{
    if (act == FOOTER_TURN_OFF) {
        lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    } else {
        lv_obj_set_style_bg_opa(obj, LV_OPA_100, 0);
    }

    return NULL;
}

static void act_sub_tabview(lv_event_t * e)
{
    lv_obj_t * tv = lv_event_get_current_target(e);
    lv_obj_t * footer_container = lv_obj_get_child(tv, -1);
    lv_event_code_t code = lv_event_get_code(e);
    int footer_num = lv_obj_get_child_cnt(footer_container);
    int footer_now = lv_tabview_get_tab_act(tv);

    if (code == LV_EVENT_VALUE_CHANGED) {
        for (int i = 0; i < footer_num; i++) {
            footer_switch(lv_obj_get_child(footer_container, i), FOOTER_TURN_OFF);
        }
        footer_switch(lv_obj_get_child(footer_container, footer_now), FOOTER_TURN_ON);
    }
}

static void update_table_cell(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *table = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        cook_book_exec_ops(&books, COOK_BOOK_OPS_DELETE);
        lv_obj_del(table);
        table = simple_table_create(lv_scr_act());
        for (int i = 0; i < 256; i++) {
            if (strlen(books.name[i]) != 0) {
                simple_table_add(table, books.name[i]);
            }
        }
    }
}

static void del_table_cell_cb(lv_event_t * e)
{
    static uint8_t rect_status = 0;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *rect = lv_event_get_target(e);
    lv_obj_t *lable = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (rect_status) {
            cook_book_clear_ops(&books, lv_label_get_text(lable), COOK_BOOK_OPS_DELETE);
            lv_img_set_src(rect, LVGL_PATH(layout_example/rect.png));
        } else {
            cook_book_set_ops(&books, lv_label_get_text(lable), COOK_BOOK_OPS_DELETE);
            lv_img_set_src(rect, LVGL_PATH(layout_example/rect_y.png));
        }
    }

    rect_status = rect_status == 0 ? 1: 0;
}
