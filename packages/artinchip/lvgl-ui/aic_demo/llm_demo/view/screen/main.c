/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "../view.h"
#include "../../llm_demo.h"

extern lv_pinyin_dict_t s3700_pinyin_def_dict[];

static void create_header(lv_obj_t *parent);
static void create_input_area(lv_obj_t* parent);
static lv_obj_t *create_message(lv_obj_t *parent, const void *logo, const char *text, lv_color_t color, ui_chat_side side);

static void label_auto_layout(lv_obj_t *label, ui_chat_side side);
static void textarea_event_cb(lv_event_t * e);
static void fold_keyboard_event_cb(lv_event_t * e);

static void wifi_status_cb(lv_timer_t *t);
static lv_obj_t *wifi_img;

lv_obj_t *chat_ui_create(lv_obj_t *parent)
{
    lv_obj_t *chat_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(chat_obj);
    lv_obj_clear_flag(chat_obj, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_pos(chat_obj, 0, 0);
    lv_obj_set_size(chat_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(chat_obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chat_obj, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);

    create_header(chat_obj);

    lv_obj_t *chat_container = lv_obj_create(chat_obj);
    lv_obj_remove_style_all(chat_container);
    lv_obj_clear_flag(chat_container, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_pos(chat_container, 0, LV_VER_RES * 0.1);
    if (LV_HOR_RES == 480)
        lv_obj_set_size(chat_container, LV_HOR_RES, LV_VER_RES * 0.7);
    else
        lv_obj_set_size(chat_container, LV_HOR_RES, LV_VER_RES * 0.8);
    lv_obj_set_style_bg_color(chat_container, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chat_container, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(chat_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(chat_container, LV_DIR_VER);
    lv_obj_add_event_cb(chat_container, fold_keyboard_event_cb, LV_EVENT_ALL, chat_obj);

    create_message(chat_container, lv_llm_get_message_logo_src(), "你好！", llm_get_message_style_color(), UI_CHAT_SIDE_LEFT);

    create_input_area(chat_obj);

    lv_obj_scroll_to_view_recursive(chat_container, true);

    return chat_obj;
}

void chat_ui_message_set_text(lv_obj_t *label, const char *text)
{
    lv_label_set_text(label, text);
    label_auto_layout(label, UI_CHAT_SIDE_LEFT);
}

void handle_ui_message(char *msg)
{
    static lv_timer_t *wifi_status_timer = NULL;
    static char wifi_status[64] = {0};

    lv_strcpy(wifi_status, msg);
    if (wifi_status_timer == NULL) {
        wifi_status_timer = lv_timer_create(wifi_status_cb, 300, &wifi_status);
    }
    if (lv_strcmp(msg, WIFI_START_MSG) == 0) {
        lv_timer_resume(wifi_status_timer);
    }
}

static void textarea_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_t * chat_obj = lv_event_get_user_data(e);
        lv_obj_t * input_container = lv_obj_get_child(chat_obj, -1);
        lv_obj_scroll_to_view(input_container, true);
    } else if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_t * chat_obj = lv_event_get_user_data(e);
        lv_obj_t * input_container = lv_obj_get_child(chat_obj, -1);
        lv_obj_t * chat_container = lv_obj_get_child(chat_obj, -2);
        lv_obj_t * input_area = lv_obj_get_child(input_container, 0);

        const char *usr_input = lv_textarea_get_text(input_area);
        if (lv_strlen(usr_input) == 0)
            return;
        if (lv_strcmp(usr_input, "clear") == 0) {
            lv_obj_clean(chat_container);
            create_message(chat_container, lv_llm_get_message_logo_src(), "你好", llm_get_message_style_color(), UI_CHAT_SIDE_LEFT);
            lv_textarea_set_text(input_area, "");
            return;
        }
        create_message(chat_container, lv_llm_get_message_user_src(), usr_input, llm_get_message_user_color(), UI_CHAT_SIDE_RIGHT);

        lv_obj_t * sys_respond = create_message(chat_container, lv_llm_get_message_logo_src(), "", llm_get_message_style_color(), UI_CHAT_SIDE_LEFT);
        llm_request_submit(usr_input, sys_respond);
        lv_obj_scroll_to_view_recursive(sys_respond, true);
        lv_obj_scroll_to_view(input_container, true);
        lv_textarea_set_text(input_area, "");
    }
}

static void fold_keyboard_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * chat_obj = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (lv_obj_get_scroll_y(chat_obj) != 0)
            lv_obj_scroll_to_y(chat_obj, 0, true);
    }
}

static void create_header(lv_obj_t *parent)
{
    lv_obj_t *list_icon = lv_img_create(parent);
    lv_img_set_src(list_icon, LVGL_PATH(list.png));
    lv_obj_set_align(list_icon, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(list_icon, 10, 10);
    lv_obj_add_flag(list_icon, LV_OBJ_FLAG_CLICKABLE);

    // Init header_label
    lv_obj_t *header_label = lv_label_create(parent);
    lv_label_set_text(header_label, "LLM-TEST");
    lv_obj_set_align(header_label, LV_ALIGN_TOP_MID);
    lv_label_set_long_mode(header_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(header_label, 0, 10);
    if (LV_HOR_RES == 480)
        lv_obj_set_style_text_font(header_label, montserratmedium_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    else
        lv_obj_set_style_text_font(header_label, montserratmedium_24, LV_PART_MAIN | LV_STATE_DEFAULT);
     wifi_img = lv_img_create(parent);
    lv_img_set_src(wifi_img, LVGL_PATH(wifi.png));
    lv_obj_set_align(wifi_img, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_pos(wifi_img, -10, 10);
}

static void create_input_area(lv_obj_t* parent)
{
    lv_obj_t *input_container = lv_obj_create(parent);
    lv_obj_remove_style_all(input_container);
    if (LV_HOR_RES == 480)
        lv_obj_set_pos(input_container, 0, LV_VER_RES * 0.85);
    else
        lv_obj_set_pos(input_container, 0, LV_VER_RES * 0.92);
    lv_obj_set_size(input_container, LV_HOR_RES, LV_VER_RES * 0.5);
    // lv_obj_set_flex_flow(input_container, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_flex_align(input_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(input_container, LV_DIR_VER);

    lv_obj_t * pinyin_ime = lv_ime_pinyin_create(input_container);
    lv_obj_set_style_text_font(pinyin_ime, noto_sanss_chinese_16, 0);
    lv_ime_pinyin_set_dict(pinyin_ime, s3700_pinyin_def_dict);

    lv_obj_t *textarea = lv_textarea_create(input_container);
    lv_obj_set_size(textarea, LV_HOR_RES, LV_PCT(10));
    lv_textarea_set_one_line(textarea, true);
    lv_obj_set_style_text_font(textarea, noto_sanss_chinese_16, 0);

    lv_obj_t * keyboard  = lv_keyboard_create(input_container);
    lv_obj_set_size(keyboard , LV_HOR_RES, LV_PCT(75));
    lv_ime_pinyin_set_keyboard(pinyin_ime, keyboard);
    lv_ime_pinyin_set_mode(pinyin_ime, LV_IME_PINYIN_MODE_K26);
    lv_obj_align_to(keyboard, textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_VER_RES * 0.053);

    lv_obj_t * cand_panel = lv_ime_pinyin_get_cand_panel(pinyin_ime);
    lv_obj_set_size(cand_panel, LV_PCT(100), LV_PCT(10));
    lv_obj_set_style_bg_color(cand_panel, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(cand_panel, LV_OPA_100, 0);
    lv_obj_set_style_radius(cand_panel, 5, 0);
    lv_obj_align_to(cand_panel, keyboard, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard , textarea);

    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_ALL, parent);
}

static lv_obj_t *create_message(lv_obj_t *parent, const void *logo, const char *text, lv_color_t color, ui_chat_side side)
{
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_HOR_RES, LV_SIZE_CONTENT);
    lv_obj_t * chat_obj = lv_obj_get_parent(parent);
    lv_obj_add_event_cb(container, fold_keyboard_event_cb, LV_EVENT_ALL, chat_obj);

    lv_obj_t *logo_img = lv_img_create(container);
    lv_img_set_src(logo_img, logo);
    if (side == UI_CHAT_SIDE_LEFT) {
        lv_obj_set_align(logo_img, LV_ALIGN_TOP_LEFT);
        lv_obj_set_pos(logo_img, 10, 10);
    } else {
        lv_obj_set_align(logo_img, LV_ALIGN_TOP_RIGHT);
        lv_obj_set_pos(logo_img, -10, 10);
    }
    lv_obj_t *text_label = lv_label_create(container);
    lv_label_set_text(text_label, text);
    if (LV_HOR_RES == 1024)
        lv_obj_set_style_text_font(text_label, noto_sanss_chinese_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    else
        lv_obj_set_style_text_font(text_label, noto_sanss_chinese_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_radius(text_label, 10, 0);
    lv_obj_set_style_bg_opa(text_label, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(text_label, color, 0);
    lv_obj_set_style_pad_all(text_label, 5, 0);
    label_auto_layout(text_label, side);

    return text_label;
}

static void label_auto_layout(lv_obj_t *label, ui_chat_side side)
{
    lv_obj_t *container = lv_obj_get_parent(label);
    lv_obj_t *logo = lv_obj_get_child(container, 0);

    lv_obj_update_layout(label);

    const char *txt = lv_label_get_text(label);
    const lv_font_t *font = lv_obj_get_style_text_font(label, 0);
    int len = strlen(txt);
    int32_t letter_space = lv_obj_get_style_text_letter_space(label, LV_PART_MAIN);

    int32_t width = lv_text_get_width(txt, len, font, letter_space);
    lv_coord_t image_with = lv_obj_get_content_width(logo);
    int gap = 10;
    int max_width = LV_HOR_RES - image_with - 10 - 10 - gap;
    if (width > LV_HOR_RES - image_with - 10 - 10)
        lv_obj_set_width(label, max_width);
    else
        lv_obj_set_width(label, width + 10);
    if (side == UI_CHAT_SIDE_LEFT) {
        lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);
        lv_obj_set_pos(label, 20 + image_with, 10);
    } else {
        lv_obj_set_align(label, LV_ALIGN_TOP_RIGHT);
        lv_obj_set_pos(label, - 20 - image_with, 10);
    }
}

static void wifi_status_cb(lv_timer_t *t)
{
    char *msg = t->user_data;
    static int wifi_connect;

    if (lv_strcmp(msg, WIFI_START_MSG) == 0) {
        lv_obj_add_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
        wifi_connect = 0;
    } else if (lv_strcmp(msg, WIFI_CONNECTING_MSG) == 0) {
        wifi_connect = 1;
    } else if (lv_strcmp(msg, WIFI_ERROR_MSG) == 0) {
        lv_obj_add_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(t);
        wifi_connect = 0;
    } else if (lv_strcmp(msg, WIFI_CONNECTED_MSG) == 0) {
        lv_obj_remove_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(t);
        wifi_connect = 0;
    }

    if (wifi_connect) {
        if (lv_obj_has_flag(wifi_img, LV_OBJ_FLAG_HIDDEN) == true)
            lv_obj_remove_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
    }
}
