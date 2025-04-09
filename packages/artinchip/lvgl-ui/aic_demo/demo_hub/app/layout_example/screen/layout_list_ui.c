/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "layout_list_ui.h"
#include "demo_hub.h"
#include "custom_list_ui.h"

#include "aic_core.h"
#include "../app_entrance.h"

#define PAGE_WIDGET         0
#define PAGE_WIDGET_CHILD   1

typedef struct _btn_list {
    const char *img_path;
    void (*event_cb)(lv_event_t * e);
} btn_list;

static lv_obj_t * layout_btn_comp_create(lv_obj_t *parent);

static lv_obj_t *list_contain_init(lv_obj_t *parent);
static lv_obj_t *list_simple_init(lv_obj_t *parent);
static lv_obj_t *list_simple_grid_init(lv_obj_t *parent);
static lv_obj_t *list_complex_init(lv_obj_t *parent);
static lv_obj_t *list_complex_grid_init(lv_obj_t *parent);

static void ui_layout_list_cb(lv_event_t *e);
static void ui_layout_simple_cb(lv_event_t *e);
static void ui_layout_simple_grid_cb(lv_event_t *e);
static void ui_layout_complex_cb(lv_event_t *e);
static void ui_layout_complex_grid_cb(lv_event_t *e);

static void back_cb(lv_event_t *e);

static void select_disp_obj(lv_obj_t *obj);

static lv_obj_t *list_contain_ui;
static lv_obj_t *list_simple_ui;
static lv_obj_t *list_simple_grid_ui;
static lv_obj_t *list_complex_ui;
static lv_obj_t *list_complex_grid_ui;

static int now_page = 0;

lv_obj_t *layout_list_ui_init(void)
{
    lv_obj_t *layout_ui = lv_obj_create(NULL);
    lv_obj_set_size(layout_ui, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(layout_ui, LV_OBJ_FLAG_SCROLLABLE);

    list_contain_ui = list_contain_init(layout_ui);
    list_simple_ui = list_simple_init(layout_ui);
    list_simple_grid_ui = list_simple_grid_init(layout_ui);
    list_complex_ui = list_complex_init(layout_ui);
    list_complex_grid_ui = list_complex_grid_init(layout_ui);

    lv_obj_t *back = lv_img_create(layout_ui);
    lv_img_set_src(back, LVGL_PATH(layout_example/back.png));
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(back, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(back, LV_HOR_RES * 0.05, LV_VER_RES * 0.03);

    lv_obj_add_event_cb(layout_ui, ui_layout_list_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_ALL, NULL);

    select_disp_obj(list_contain_ui);

    return layout_ui;
}

static lv_obj_t *list_contain_init(lv_obj_t *parent)
{
    btn_list list[] = {
        {LVGL_PATH(layout_example/simple_btn.png), ui_layout_simple_cb},
        {LVGL_PATH(layout_example/simple_grid_btn.png), ui_layout_simple_grid_cb},
        {LVGL_PATH(layout_example/complex_btn.png), ui_layout_complex_cb},
        {LVGL_PATH(layout_example/complex_grid_btn.png), ui_layout_complex_grid_cb},
        {NULL, NULL}
    };

    lv_obj_t *contain = lv_obj_create(parent);
    lv_obj_set_size(contain, lv_pct(100), lv_pct(100));
    lv_obj_remove_style_all(contain);
    lv_obj_set_style_bg_opa(contain, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(contain, lv_theme_get_color_primary(NULL), 0);
    lv_obj_set_size(contain, lv_pct(100), lv_pct(100));
    lv_obj_set_align(contain, LV_ALIGN_TOP_LEFT);

    lv_obj_t *head_container = lv_obj_create(contain);
    lv_obj_remove_style_all(head_container);
    lv_obj_set_pos(head_container, 0, 0);
    lv_obj_set_align(head_container, LV_ALIGN_TOP_LEFT);
    lv_obj_set_size(head_container, LV_HOR_RES, LV_VER_RES * 0.15);
    lv_obj_set_style_bg_opa(head_container, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(head_container, lv_theme_get_color_secondary(NULL), 0);

    lv_obj_t *label = lv_img_create(head_container);
    lv_img_set_src(label, LVGL_PATH(layout_example/custom_list_chose.png));
    lv_obj_center(label);

    lv_obj_t *btn_contain = lv_obj_create(contain);
    lv_obj_remove_style_all(btn_contain);
    lv_obj_set_align(btn_contain, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_contain, LV_OPA_0, 0);
    lv_obj_set_align(btn_contain, LV_ALIGN_TOP_MID);
    lv_obj_set_pos(btn_contain, 0, LV_VER_RES * 0.20);
    lv_obj_set_size(btn_contain, lv_pct(60), lv_pct(80));
    lv_obj_clear_flag(btn_contain, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(btn_contain, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(btn_contain, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN);

    for (int i = 0; i < sizeof(list) / sizeof(list[0]) - 1; i++) {
        lv_obj_t *btn = layout_btn_comp_create(btn_contain);
        lv_obj_t *btn_label = lv_img_create(btn);
        lv_img_set_src(btn_label, list[i].img_path);
        lv_obj_set_align(btn_label, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(btn, list[i].event_cb, LV_EVENT_ALL, NULL);
    }

    return contain;
}

static lv_obj_t *list_simple_init(lv_obj_t *parent)
{
    lv_obj_t *simple_list = simple_list_create(parent);
    simple_list_add(simple_list, "I leave no trace of wings in the air,");
    simple_list_add(simple_list, "but i am glad have my flight.");
    simple_list_add(simple_list, "");
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);
    simple_list_add(simple_list, NULL);

    return simple_list;
}

static lv_obj_t *list_simple_grid_init(lv_obj_t *parent)
{
    lv_obj_t *simple_grid_list = simple_list_grid_create(parent);
    simple_list_grid_add(simple_grid_list, "If winter comes,");
    simple_list_grid_add(simple_grid_list, "can spring be far behind?");
    simple_list_grid_add(simple_grid_list, "");
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);
    simple_list_grid_add(simple_grid_list, NULL);

    return simple_grid_list;
}

static lv_obj_t *list_complex_init(lv_obj_t *parent)
{
    lv_obj_t *complex_list = complex_list_create(parent);
    complex_list_add(complex_list, "I' don't eat beef", 0);
    complex_list_add(complex_list, "I' don't eat beef", 1);
    complex_list_add(complex_list, "I' don't eat beef", 2);
    complex_list_add(complex_list, NULL, 3);
    complex_list_add(complex_list, NULL, 4);
    complex_list_add(complex_list, NULL, 3);
    complex_list_add(complex_list, NULL, 2);
    complex_list_add(complex_list, NULL, 1);
    complex_list_add(complex_list, NULL, 0);

    return complex_list;
}

static lv_obj_t *list_complex_grid_init(lv_obj_t *parent)
{
    lv_obj_t *complex_grid_list = complex_grid_list_create(parent);
    complex_grid_list_add(complex_grid_list, "I' don't eat beef", "1");
    complex_grid_list_add(complex_grid_list, "I' don't eat bee1222222222222222222123123f", "112222342323423234234234234234234234234234234234234222222222222212121212121212121212");
    complex_grid_list_add(complex_grid_list, "I' don't eat b12eef", "2");
    complex_grid_list_add(complex_grid_list, NULL, "2");
    complex_grid_list_add(complex_grid_list, NULL, "3");
    complex_grid_list_add(complex_grid_list, NULL, "4");
    complex_grid_list_add(complex_grid_list, NULL, "2");
    complex_grid_list_add(complex_grid_list, NULL, "1");
    complex_grid_list_add(complex_grid_list, NULL, "2");

    return complex_grid_list;
}

static lv_obj_t *layout_btn_comp_create(lv_obj_t *parent)
{
#if LVGL_VERSION_MAJOR == 8
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_HOR_RES * 0.4, LV_VER_RES * 0.12);
    lv_obj_set_style_outline_opa(btn, LV_OPA_100, 0);
#else
    lv_obj_t *btn_container = lv_obj_create(parent);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, LV_HOR_RES * 0.42, LV_VER_RES * 0.15);

    lv_obj_t *btn = lv_btn_create(btn_container);
    lv_obj_set_size(btn, LV_HOR_RES * 0.4, LV_VER_RES * 0.12);
    lv_obj_set_style_outline_opa(btn, LV_OPA_100, 0);
    lv_obj_center(btn);
#endif
    int out_line_width = lv_obj_get_height(btn) * 0.1;
    out_line_width = out_line_width == 0 ? 1 : out_line_width;

    lv_obj_set_style_outline_width(btn, out_line_width, 0);
    lv_obj_set_style_outline_color(btn, lv_theme_get_color_emphasize(NULL), 0);
    lv_obj_set_style_bg_color(btn, lv_theme_get_color_secondary(NULL), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
    lv_obj_set_style_radius(btn, 8, 0);

    return btn;
}

static void ui_layout_list_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {;}

    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

static void back_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        if (now_page == PAGE_WIDGET_CHILD) {
            select_disp_obj(list_contain_ui);
            now_page = PAGE_WIDGET;
        } else {
            app_entrance(APP_NAVIGATION, 1);
        }
    }
}

static void select_disp_obj(lv_obj_t *obj)
{
    lv_obj_add_flag(list_contain_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(list_simple_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(list_simple_grid_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(list_complex_ui, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(list_complex_grid_ui, LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void ui_layout_simple_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(list_simple_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_layout_simple_grid_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(list_simple_grid_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_layout_complex_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(list_complex_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}

static void ui_layout_complex_grid_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        select_disp_obj(list_complex_grid_ui);
        now_page = PAGE_WIDGET_CHILD;
    }
}
