/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#include "aic_ui.h"
#include "photo_frame_screen.h"

LV_FONT_DECLARE(eastern_large_12);
LV_FONT_DECLARE(eastern_large_24);

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);
extern lv_obj_t *eastern_large_label_comp(lv_obj_t *parent, uint16_t weight);

lv_obj_t *com_dropdown(lv_obj_t *parent, int16_t width, int16_t height, void *desc, lv_font_t *font);

static void back_main_cb(lv_event_t * e);
static void auto_play_switch_cb(lv_event_t * e);
static void cur_anim_switch_cb(lv_event_t * e);
static void full_mode_switch_cb(lv_event_t * e);

lv_obj_t *photo_frame_setting;

static void setting_impl(lv_obj_t *parent);

static lv_font_t *font_addr;

lv_obj_t *photo_frame_setting_create(lv_obj_t *parent)
{
    photo_frame_setting = lv_img_create(parent);
    lv_img_set_src(photo_frame_setting, LVGL_PATH(photo_frame/bg.jpg));
    lv_obj_add_flag(photo_frame_setting, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(photo_frame_setting, LV_OBJ_FLAG_SCROLLABLE);

    setting_impl(photo_frame_setting);

    lv_obj_t *title = lv_img_create(photo_frame_setting);
    lv_img_set_src(title, LVGL_PATH(photo_frame/setting_page_title.png));
    lv_obj_set_pos(title, adaptive_width(0), adaptive_height(50));
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);

    lv_obj_t *home_btn = com_imgbtn_comp(photo_frame_setting, LVGL_PATH(photo_frame/home.png), NULL);
    lv_obj_set_pos(home_btn, adaptive_width(30), adaptive_height(26));
    lv_obj_set_align(home_btn, LV_ALIGN_TOP_LEFT);

    lv_obj_add_event_cb(home_btn, back_main_cb, LV_EVENT_ALL, NULL);

    return photo_frame_setting;
}

static void setting_impl(lv_obj_t *parent)
{
    lv_obj_t *container_bg = lv_img_create(parent);
    lv_img_set_src(container_bg, LVGL_PATH(photo_frame/setting_page_container_bg.png));
    lv_obj_set_pos(container_bg, adaptive_width(53), adaptive_height(142));

    int16_t font_size = 24;
    if (LV_HOR_RES <= 480)
        font_size = 12;

    font_addr = (lv_font_t *)&eastern_large_24;
    if (LV_HOR_RES <= 480)
        font_addr = (lv_font_t *)&eastern_large_12;

    lv_obj_t *option_title = eastern_large_label_comp(parent, font_size);
    lv_obj_set_style_text_color(option_title, lv_color_hex(0x242412), 0);
    lv_label_set_text(option_title, "Auto Switch");
    lv_obj_set_pos(option_title, adaptive_width(88), adaptive_height(160));
    lv_obj_t *auto_switch_dropdown = com_dropdown(parent, adaptive_width(240), adaptive_height(50), (void *)setting_auto_play_desc, font_addr);
    lv_obj_set_pos(auto_switch_dropdown, adaptive_width(88), adaptive_height(200));

    option_title = eastern_large_label_comp(parent, font_size);
    lv_obj_set_style_text_color(option_title, lv_color_hex(0x242412), 0);
    lv_label_set_text(option_title, "Photo switch anim");
    lv_obj_set_pos(option_title, adaptive_width(402), adaptive_height(160));
    lv_obj_t *cur_anim_dropdown = com_dropdown(parent, adaptive_width(240), adaptive_height(50), (void *)setting_cur_anim_desc, font_addr);
    lv_obj_set_pos(cur_anim_dropdown, adaptive_width(402), adaptive_height(198));

    option_title = eastern_large_label_comp(parent, font_size);
    lv_obj_set_style_text_color(option_title, lv_color_hex(0x242412), 0);
    lv_label_set_text(option_title, "Full mode");
    lv_obj_set_pos(option_title, adaptive_width(730), adaptive_height(160));
    lv_obj_t *full_drown_dropdown = com_dropdown(parent, adaptive_width(240), adaptive_height(50), (void *)setting_full_mode_desc, font_addr);
    lv_obj_set_pos(full_drown_dropdown, adaptive_width(730), adaptive_height(198));

    lv_dropdown_set_selected(auto_switch_dropdown, g_setting.auto_play);
    lv_dropdown_set_selected(cur_anim_dropdown, g_setting.cur_anmi - PHOTO_SWITCH_ANIM_NONE);
    lv_dropdown_set_selected(full_drown_dropdown, g_setting.cur_photo_full_mode - FULL_MODE_NONRMAL);

    lv_obj_add_event_cb(auto_switch_dropdown, auto_play_switch_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(cur_anim_dropdown, cur_anim_switch_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(full_drown_dropdown, full_mode_switch_cb, LV_EVENT_ALL, NULL);
}

static void back_main_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        lv_img_cache_invalidate_src(NULL);
        lv_obj_clear_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(photo_frame_setting, LV_OBJ_FLAG_HIDDEN);
    }
}

static void auto_play_switch_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        int16_t select = lv_dropdown_get_selected(obj);
        g_setting.auto_play = select;
    }
}

static void cur_anim_switch_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        int16_t select = lv_dropdown_get_selected(obj);
        g_setting.cur_anmi = select - PHOTO_SWITCH_ANIM_NONE;
    }
}

static void full_mode_switch_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        int16_t select = lv_dropdown_get_selected(obj);
        g_setting.cur_photo_full_mode = select - FULL_MODE_NONRMAL;
    }
}
