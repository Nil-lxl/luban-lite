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

static void back_main_cb(lv_event_t * e);

lv_obj_t *photo_frame_calendar;

static void drawdown_style_init(lv_obj_t *dropdown);
static void calendar_impl(lv_obj_t *parent);

static lv_font_t *font_addr;
lv_obj_t *photo_frame_calendar_create(lv_obj_t *parent)
{
    font_addr = (lv_font_t *)&eastern_large_24;
    if (LV_HOR_RES <= 480)
        font_addr = (lv_font_t *)&eastern_large_12;

    photo_frame_calendar = lv_img_create(parent);
    lv_img_set_src(photo_frame_calendar, LVGL_PATH(photo_frame/bg.jpg));
    lv_obj_add_flag(photo_frame_calendar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(photo_frame_calendar, LV_OBJ_FLAG_SCROLLABLE);

    calendar_impl(photo_frame_calendar);

    lv_obj_t *title = lv_img_create(photo_frame_calendar);
    lv_img_set_src(title, LVGL_PATH(photo_frame/calendar_page_title.png));
    lv_obj_set_pos(title, adaptive_width(0), adaptive_height(50));
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);

    lv_obj_t *home_btn = com_imgbtn_comp(photo_frame_calendar, LVGL_PATH(photo_frame/home.png), NULL);
    lv_obj_set_pos(home_btn, adaptive_width(30), adaptive_height(26));
    lv_obj_set_align(home_btn, LV_ALIGN_TOP_LEFT);

    lv_obj_add_event_cb(home_btn, back_main_cb, LV_EVENT_ALL, NULL);

    return photo_frame_calendar;
}

static void drawdown_style_init(lv_obj_t *dropdown)
{
    lv_obj_set_width(dropdown, adaptive_width(240));
    lv_obj_set_height(dropdown, adaptive_height(50));
    lv_obj_set_style_shadow_color(dropdown, lv_color_hex(0x0), 0);
    lv_obj_set_style_shadow_opa(dropdown, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_x(dropdown, 2, 0);
    lv_obj_set_style_shadow_ofs_y(dropdown, 2, 0);
    lv_obj_set_style_shadow_width(dropdown, adaptive_width(5), 0);

    lv_dropdown_set_symbol(dropdown, LVGL_PATH(photo_frame/dropdown_drown.png));

    /* main */
    lv_obj_set_style_text_color(dropdown, lv_color_hex(0x242412), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(dropdown, (const lv_font_t *)font_addr, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(dropdown, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dropdown, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(dropdown, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(dropdown, lv_color_hex(0xA6A6A6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(dropdown, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(dropdown, adaptive_height(12), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(dropdown, adaptive_width(10), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(dropdown, adaptive_width(8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(dropdown, 8, LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_obj_set_style_shadow_width(dropdown, adaptive_width(8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(dropdown, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(dropdown, 30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(dropdown, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(dropdown, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(dropdown, adaptive_height(4), LV_PART_MAIN|LV_STATE_DEFAULT);

    /* selected part */
    lv_obj_set_style_bg_opa(lv_dropdown_get_list(dropdown), LV_OPA_100, LV_PART_SELECTED|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(lv_dropdown_get_list(dropdown), lv_color_hex(0xA6A6A6), LV_PART_SELECTED|LV_STATE_CHECKED);

    /* list part */
    lv_obj_set_style_text_color(lv_dropdown_get_list(dropdown), lv_color_hex(0x242412), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_dropdown_get_list(dropdown), (const lv_font_t *)font_addr, LV_PART_MAIN|LV_STATE_DEFAULT);
}
#if LVGL_VERSION_MAJOR == 8
static void calendar_draw_part_begin_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
    if(dsc->part == LV_PART_ITEMS) {
        if(dsc->id < 7) {
            dsc->label_dsc->color = lv_color_hex(0x0D3055);
            dsc->label_dsc->font = (const lv_font_t *)font_addr;
        } else if (lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_DISABLED)) {
            dsc->label_dsc->color = lv_color_hex(0xA9A2A2);
            dsc->label_dsc->font = (const lv_font_t *)font_addr;
            dsc->rect_dsc->bg_opa = 0;
        } else if(lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_CUSTOM_1)) {
            dsc->label_dsc->color = lv_color_hex(0x0D3055);
            dsc->label_dsc->font = (const lv_font_t *)font_addr;
            dsc->rect_dsc->bg_opa = 255;
            dsc->rect_dsc->bg_color = lv_color_hex(0xA6A6A6);
            dsc->rect_dsc->border_opa = 255;
            dsc->rect_dsc->border_width = 1;
            dsc->rect_dsc->border_color = lv_color_hex(0xc0c0c0);
        } else if(lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_CUSTOM_2)) {
            dsc->label_dsc->color = lv_color_hex(0x0D3055);
            dsc->label_dsc->font = (const lv_font_t *)font_addr;
            dsc->rect_dsc->bg_opa = 0;
        } else {
        }
    }
}
#endif
static void calendar_event_handler(lv_event_t * e)
{
    lv_calendar_date_t date;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * obj = lv_event_get_current_target(e);
        lv_calendar_get_pressed_date(obj, &date);
        lv_calendar_set_highlighted_dates(obj, &date, 1);
    }
}

static void calendar_impl(lv_obj_t *parent)
{
    lv_calendar_date_t calendar_today;
    lv_calendar_date_t calendar_highlihted_days[1];

    calendar_today.year = 2022;
    calendar_today.month = 9;
    calendar_today.day = 5;

    calendar_highlihted_days[0].year = 2022;
    calendar_highlihted_days[0].month = 9;
    calendar_highlihted_days[0].day = 6;

    lv_obj_t  *calendar = lv_calendar_create(photo_frame_calendar);
    lv_obj_set_size(calendar, adaptive_width(185), adaptive_height(185));
    lv_obj_align(calendar, LV_ALIGN_CENTER, adaptive_width(0), adaptive_height(27));

    lv_obj_center(calendar);
    lv_obj_set_pos(calendar, adaptive_width(0), adaptive_height(50));
    lv_obj_set_size(calendar, adaptive_width(937), adaptive_height(442));
    lv_obj_set_style_text_color(calendar, lv_color_hex(0x242412), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(calendar, font_addr, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(calendar, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(calendar, 20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(calendar, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(calendar, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(calendar, 76, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(calendar, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(calendar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(calendar, adaptive_height(4), LV_PART_MAIN|LV_STATE_DEFAULT);

    lv_calendar_set_today_date(calendar, calendar_today.year, calendar_today.month, calendar_today.day);
    lv_calendar_set_showed_date(calendar, calendar_today.year, calendar_today.month);
    lv_calendar_set_highlighted_dates(calendar, calendar_highlihted_days, 1);

    lv_obj_t *drawdown_list = lv_calendar_header_dropdown_create(calendar);
    uint32_t header_child_count = lv_obj_get_child_cnt(drawdown_list);
    /* Search for the year dropdown */
    for(int16_t idx = 0; idx < header_child_count; idx++) {
        lv_obj_t *child = lv_obj_get_child(drawdown_list, idx);
        drawdown_style_init(child);
    }

#if LVGL_VERSION_MAJOR == 8
    lv_calendar_t *calendar_obj = (lv_calendar_t *)calendar;
    lv_obj_add_event_cb(calendar_obj->btnm, calendar_draw_part_begin_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
#endif
    lv_obj_add_event_cb(calendar, calendar_event_handler, LV_EVENT_ALL, NULL);
}

static void back_main_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_clear_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(photo_frame_calendar, LV_OBJ_FLAG_HIDDEN);
    }
}

