/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "demo_hub.h"
#include "custom_list_ui.h"
#include "string.h"

static void simple_switch_note_cb(lv_event_t * e);
static void simple_switch_love_cb(lv_event_t * e);

static void simple_switch_note_cb(lv_event_t * e)
{
    static uint8_t note_status = 0;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *note = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        if (note_status)
            lv_img_set_src(note, LVGL_PATH(layout_example/note.png));
        else
            lv_img_set_src(note, LVGL_PATH(layout_example/note_default.png));
    }

    note_status = note_status == 0 ? 1: 0;
}

static void simple_switch_love_cb(lv_event_t * e)
{
    static uint8_t love_status = 0;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *love = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        if (love_status)
            lv_img_set_src(love, LVGL_PATH(layout_example/love.png));
        else
            lv_img_set_src(love, LVGL_PATH(layout_example/love_default.png));
    }

    love_status = love_status == 0 ? 1: 0;
}

static void del_container_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *trash = lv_event_get_target(e);
    lv_obj_t *container = lv_obj_get_parent(trash);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_del(container);
    }
}

lv_obj_t *simple_list_create(lv_obj_t *parent)
{
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(list, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(list, lv_theme_get_color_primary(NULL), 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title_container = lv_obj_create(list);
    lv_obj_remove_style_all(title_container);
    lv_obj_set_size(title_container, LV_HOR_RES, LV_VER_RES * 0.15);
    lv_obj_set_style_bg_color(title_container, lv_theme_get_color_primary(NULL), 0);
    lv_obj_set_align(title_container, LV_ALIGN_TOP_LEFT);

    lv_obj_t *img_label = lv_img_create(title_container);
    lv_img_set_src(img_label, LVGL_PATH(layout_example/title_label.png));
    lv_obj_center(img_label);

    return list;
}

void simple_list_add(lv_obj_t *obj, char *context)
{
    static int color = 0;
    lv_color_t bg_color;

    if (color == 0)
        bg_color = lv_theme_get_color_secondary(NULL);
    else
        bg_color = lv_theme_get_color_primary(NULL);

    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, lv_pct(100), lv_pct(15));
    lv_obj_set_style_bg_color(container, bg_color, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_100, 0);

    lv_obj_t *note = lv_img_create(container);
    lv_img_set_src(note, LVGL_PATH(layout_example/note_default.png));
    lv_obj_set_align(note, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(note, LV_HOR_RES * 0.04, 0);
    lv_obj_add_flag(note, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(note, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(container);
    if (context == NULL)
        lv_label_set_text(label, "simple list context");
    else
        lv_label_set_text(label, (const char *)context);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(label, LV_ALIGN_CENTER);

    lv_obj_t *love = lv_img_create(container);
    lv_img_set_src(love, LVGL_PATH(layout_example/love_default.png));
    lv_obj_set_align(love, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(love, -LV_HOR_RES * 0.04, 0);
    lv_obj_add_flag(love, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(love, LV_OBJ_FLAG_CLICKABLE);

    color = color == 0 ? 1 : 0;

    lv_obj_add_event_cb(note, simple_switch_note_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(love, simple_switch_love_cb, LV_EVENT_ALL, NULL);
}

lv_obj_t *simple_list_grid_create(lv_obj_t *parent)
{
    return simple_list_create(parent);
}

void simple_list_grid_add(lv_obj_t *obj, char *context)
{
    static lv_coord_t simple_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t simple_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(5), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    static int color = 0;
    lv_color_t bg_color;

    if (color == 0)
        bg_color = lv_theme_get_color_secondary(NULL);
    else
        bg_color = lv_theme_get_color_primary(NULL);

    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, lv_pct(100), lv_pct(15));
    lv_obj_set_style_bg_color(container, bg_color, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_100, 0);
    lv_obj_set_style_grid_column_dsc_array(container, simple_col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(container, simple_row_dsc, 0);
    lv_obj_set_layout(container, LV_LAYOUT_GRID);

    lv_obj_t *note = lv_img_create(container);
    lv_obj_set_size(note, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_img_set_src(note, LVGL_PATH(layout_example/note_default.png));
    lv_obj_add_flag(note, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(note, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(note, LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *label = lv_label_create(container);
    if (context == NULL)
        lv_label_set_text(label, "simple list context");
    else
        lv_label_set_text(label, (const char *)context);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_CENTER, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *love = lv_img_create(container);
    lv_obj_set_size(love, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_img_set_src(love, LVGL_PATH(layout_example/love_default.png));
    lv_obj_add_flag(love, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(love, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(love, LV_GRID_ALIGN_CENTER, 2, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    color = color == 0 ? 1 : 0;

    lv_obj_add_event_cb(note, simple_switch_note_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(love, simple_switch_love_cb, LV_EVENT_ALL, NULL);
}

lv_obj_t *complex_list_create(lv_obj_t *parent)
{
    return simple_list_create(parent);
}

void complex_list_add(lv_obj_t *obj, char *context, int btn_num)
{
    static int color = 0;
    lv_color_t bg_color;

    if (color == 0)
        bg_color = lv_theme_get_color_secondary(NULL);
    else
        bg_color = lv_theme_get_color_primary(NULL);

    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, lv_pct(100), lv_pct(20));
    lv_obj_set_style_bg_color(container, bg_color, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_100, 0);

    lv_obj_t *net = lv_img_create(container);
    lv_img_set_src(net, LVGL_PATH(layout_example/net.png));
    lv_obj_add_flag(net, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(net, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(net, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(net, LV_HOR_RES * 0.04, 0);

    lv_obj_t *label = lv_label_create(container);
    if (context == NULL)
        lv_label_set_text(label, "Title");
    else
        lv_label_set_text(label, (const char *)context);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_align(label, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(label, LV_HOR_RES * 0.13, -LV_VER_RES * 0.05);

    /* btn pos:
     * (btn_start) btn1 (gap) btn2 (gap) btn3
    */
    const int btn_start = LV_HOR_RES * 0.13;
    const int btn_gap = (LV_HOR_RES * 0.077) * 0.2;;
    int btn_x_pos = btn_start;
    for (int i = 0; i < btn_num; i++) {
        lv_obj_t *btn = lv_btn_create(container);
        lv_obj_set_size(btn, LV_HOR_RES * 0.077, LV_VER_RES * 0.077);
        lv_obj_set_align(btn, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_bg_color(btn, lv_theme_get_color_emphasize(NULL), 0);
        lv_obj_set_pos(btn, btn_x_pos, LV_VER_RES * 0.05);
        btn_x_pos +=  (LV_HOR_RES * 0.077) + btn_gap;
    }

    if (btn_num == 0) {
        lv_obj_t *red_bg = lv_obj_create(container);
        lv_obj_set_size(red_bg, lv_pct(20), lv_pct(100));
        lv_obj_set_pos(red_bg, LV_HOR_RES, 0);
        lv_obj_set_style_bg_color(red_bg,lv_color_hex(0xE02020), 0);
    }

    lv_obj_t *love = lv_img_create(container);
    lv_img_set_src(love, LVGL_PATH(layout_example/love_default.png));
    lv_obj_add_flag(love, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(love, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(love, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(love, -LV_HOR_RES * 0.1, 0);

    lv_obj_t *trash = lv_img_create(container);
    lv_img_set_src(trash, LVGL_PATH(layout_example/trash.png));
    lv_obj_add_flag(trash, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(trash, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(trash, LV_ALIGN_RIGHT_MID);
    lv_obj_set_pos(trash, -LV_HOR_RES * 0.04, 0);

    color = color == 0 ? 1 : 0;

    lv_obj_add_event_cb(trash, del_container_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(love, simple_switch_love_cb, LV_EVENT_ALL, NULL);
}

lv_obj_t *complex_grid_list_create(lv_obj_t *parent)
{
    return simple_list_create(parent);
}

void complex_grid_list_add(lv_obj_t *obj, char *context_title, char *context_title_desc)
{
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t col_dsc[] = {50, LV_GRID_FR(1), 20, 30, LV_GRID_TEMPLATE_LAST};

    static int color = 0;
    lv_color_t bg_color;

    if (color == 0)
        bg_color = lv_theme_get_color_secondary(NULL);
    else
        bg_color = lv_theme_get_color_primary(NULL);

    lv_obj_t *container = lv_obj_create(obj);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, lv_pct(100), lv_pct(20));
    lv_obj_set_style_bg_color(container, bg_color, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_100, 0);
    lv_obj_set_style_grid_row_dsc_array(container, row_dsc, 0);
    lv_obj_set_style_grid_column_dsc_array(container, col_dsc, 0);
    lv_obj_set_layout(container, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_column(container, 20, 0);

    lv_obj_t *net = lv_img_create(container);
    lv_img_set_src(net, LVGL_PATH(layout_example/net.png));
    lv_obj_add_flag(net, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(net, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(net, LV_GRID_ALIGN_END, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 2);

    lv_obj_t *title = lv_label_create(container);
    if (title == NULL)
        lv_label_set_text(title, "Title");
    else
        lv_label_set_text(title, (const char *)context_title);
    lv_obj_set_size(title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 1, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *desc_title = lv_label_create(container);
    if (desc_title == NULL)
        lv_label_set_text(desc_title, "desc_title");
    else
        lv_label_set_text(desc_title, (const char *)context_title_desc);
    lv_obj_set_size(desc_title, LV_HOR_RES -50 -20 -30 -50, LV_SIZE_CONTENT);
    lv_label_set_long_mode(desc_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_grid_cell(desc_title, LV_GRID_ALIGN_START, 1, 1,
                         LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_t *love = lv_img_create(container);
    lv_img_set_src(love, LVGL_PATH(layout_example/love_default.png));
    lv_obj_add_flag(love, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(love, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(love, LV_GRID_ALIGN_CENTER, 2, 1,
                         LV_GRID_ALIGN_CENTER, 0, 2);

    lv_obj_t *trash = lv_img_create(container);
    lv_img_set_src(trash, LVGL_PATH(layout_example/trash.png));
    lv_obj_add_flag(trash, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_flag(trash, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_grid_cell(trash, LV_GRID_ALIGN_CENTER, 3, 1,
                         LV_GRID_ALIGN_CENTER, 0, 2);

    color = color == 0 ? 1 : 0;

    lv_obj_add_event_cb(trash, del_container_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(love, simple_switch_love_cb, LV_EVENT_ALL, NULL);
}
