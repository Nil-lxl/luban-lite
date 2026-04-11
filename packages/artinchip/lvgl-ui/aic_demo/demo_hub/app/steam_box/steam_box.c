/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_ui.h"
#include "lvgl.h"
#include "./components/common_comp.h"

#define BYTE_ALIGN(x, byte) (((x) + ((byte) - 1))&(~((byte) - 1)))

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);
extern lv_obj_t *com_label_comp(lv_obj_t *parent);
extern lv_obj_t *cookbook_ui_create(lv_obj_t *parent);

static void steam_box_ui_impl(lv_obj_t *parent);

static void ui_steam_box_cb(lv_event_t *e);
static void menu_ui_create_cb(lv_event_t *e);
static void switch_to_main_ui_cb(lv_event_t *e);
static void message_update_cb(lv_timer_t * timer);

static int adaptive_width(int width);
static int adaptive_height(int height);

static lv_obj_t *steam_box_impl;
static lv_obj_t *cookbook_impl;
static lv_timer_t *ui_overturn_status_timer;

lv_obj_t *steam_box_ui_init()
{
    lv_obj_t *steam_box = lv_obj_create(NULL);
    lv_obj_set_size(steam_box, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(steam_box, LV_OBJ_FLAG_SCROLLABLE);

    steam_box_ui_impl(steam_box);

    lv_obj_add_event_cb(steam_box, ui_steam_box_cb, LV_EVENT_ALL, NULL);

    return steam_box;
}

static void steam_box_ui_impl(lv_obj_t *parent)
{
    steam_box_impl = lv_obj_create(parent);
    lv_obj_remove_style_all(steam_box_impl);
    lv_obj_set_size(steam_box_impl, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(steam_box_impl, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bg = lv_img_create(steam_box_impl);
    lv_img_set_src(bg, LVGL_PATH(steam_box/main.png));

    lv_obj_t *icon = com_empty_imgbtn_comp(steam_box_impl, LVGL_PATH(steam_box/steam_icon_selected.png));
    lv_obj_set_pos(icon, adaptive_width(7), adaptive_height(130));

    icon = com_empty_imgbtn_comp(steam_box_impl, LVGL_PATH(steam_box/fry_icon_selected.png));
    lv_obj_set_pos(icon, adaptive_width(251), adaptive_height(130));

    icon = com_empty_imgbtn_comp(steam_box_impl, LVGL_PATH(steam_box/roast_icon_selected.png));
    lv_obj_set_pos(icon, adaptive_width(492), adaptive_height(130));

    lv_obj_t *menu_icon = com_empty_imgbtn_comp(steam_box_impl, LVGL_PATH(steam_box/menu_icon_selected.png));
    lv_obj_set_pos(menu_icon, adaptive_width(744), adaptive_height(130));

    lv_obj_t *message = lv_img_create(steam_box_impl);
    lv_img_set_src(message, LVGL_PATH(steam_box/message.png));
    lv_obj_set_pos(message, adaptive_width(866), adaptive_height(18));

    ui_overturn_status_timer = lv_timer_create(message_update_cb, 1000, message);

    lv_obj_add_event_cb(menu_icon, menu_ui_create_cb, LV_EVENT_ALL, parent);
}

static void menu_ui_create_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_HOR_RES != 480 || LV_VER_RES != 272 || code != LV_EVENT_CLICKED)
        return;

    if (!cookbook_impl) {
        cookbook_impl = cookbook_ui_create(scr);

        lv_obj_t *back = com_imgbtn_comp(cookbook_impl, LVGL_PATH(steam_box/back.png), NULL);
        lv_obj_set_pos(back, adaptive_width(40), adaptive_height(40));

        lv_obj_add_event_cb(back, switch_to_main_ui_cb, LV_EVENT_ALL, NULL);
    }

    lv_timer_pause(ui_overturn_status_timer);
    lv_obj_clear_flag(cookbook_impl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(steam_box_impl, LV_OBJ_FLAG_HIDDEN);
}

static void ui_steam_box_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *steam_box = (lv_obj_t *)lv_event_get_current_target(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_obj_clean(steam_box); /* pre-release memory to avoid issues of insufficient memory when loading hte next screen */
        lv_timer_del(ui_overturn_status_timer);
        cookbook_impl = NULL;
    }

    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {
        lv_img_cache_set_size(3);
    }
}

static void message_update_cb(lv_timer_t * timer)
{
    static int i = 1;
    lv_obj_t *message = (lv_obj_t *)timer->user_data;

    if (++i % 2) {
        lv_obj_add_flag(message, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(message, LV_OBJ_FLAG_HIDDEN);
    }
}

static int adaptive_width(int width)
{
    float src_width = LV_HOR_RES;
    int align_width = (int)((src_width / 1024.0) * width);
    return BYTE_ALIGN(align_width, 2);
}

static int adaptive_height(int height)
{
    float scr_height = LV_VER_RES;
    int align_height = (int)((scr_height / 600.0) * height);
    return BYTE_ALIGN(align_height, 2);
}

static void switch_to_main_ui_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_timer_resume(ui_overturn_status_timer);
        lv_obj_clear_flag(steam_box_impl, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cookbook_impl, LV_OBJ_FLAG_HIDDEN);
    }
}
