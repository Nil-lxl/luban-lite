/*
 * Copyright (C) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "multi_lang.h"
#include "lang_item.h"
#include "screen_white.h"
#include "screen_dark.h"
#include "lvgl.h"
#include "aic_ui.h"

extern multi_lang_t *multi_ctx;

#define LANG_INI_PATH LVGL_STORAGE_PATH"/lang"

lv_obj_t *screen_white = NULL;

static lv_ll_t widget_list;
static lv_obj_t *menu_label = NULL;
static lv_obj_t *button_label = NULL;
static lv_obj_t *time_label = NULL;


static void drop_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        LV_LOG_USER("Option: %s", buf);

        if (strcmp(buf, "中文") == 0) {
            multi_lang_set_language(multi_ctx, "chinese");
            lang_item_update(&widget_list, multi_ctx);
        } else {
            multi_lang_set_language(multi_ctx, "english");
            lang_item_update(&widget_list, multi_ctx);
        }
    }
}

static void screen_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    // update font when screen loaded
    if(code == LV_EVENT_SCREEN_LOADED) {
        lang_item_update(&widget_list, multi_ctx);
    }
}

static void key_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        printf("key_event_handler\n");
        lv_scr_load(screen_dark);
    }
}

lv_obj_t *init_screen_white(void)
{
    lang_item_init(&widget_list, sizeof(lang_item_t));

    screen_white = lv_obj_create(NULL);
    lv_obj_add_event_cb(screen_white, screen_event_handler, LV_EVENT_ALL, NULL);

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(screen_white);
    lv_dropdown_set_options(dd, "中文\n"
                            "English");

    static lv_style_t style;
    lv_style_init(&style);
#if LV_FONT_SIMSUN_16_CJK == 1
    lv_style_set_text_font(&style, &lv_font_simsun_16_cjk);
#else
    lv_style_set_text_font(&style, &lv_font_montserrat_14);
#endif

    lv_obj_t * dd_list = lv_dropdown_get_list(dd);
    lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_style(dd, &style, LV_PART_MAIN);
    lv_obj_add_style(dd_list, &style, LV_PART_MAIN);
    lv_obj_add_event_cb(dd, drop_event_handler, LV_EVENT_ALL, NULL);

    lv_obj_t * menu_btn = lv_btn_create(screen_white);
    lv_obj_align(menu_btn, LV_ALIGN_CENTER, 0, -80);
    lv_obj_add_event_cb(menu_btn, key_event_handler, LV_EVENT_ALL, NULL);

    menu_label = lv_label_create(menu_btn);
    lv_obj_center(menu_label);
    lang_item_add(&widget_list, menu_label, NULL, "dark_key", 0);

    lv_obj_t * button_btn = lv_btn_create(screen_white);
    lv_obj_align(button_btn, LV_ALIGN_CENTER, 0, 0);

    button_label = lv_label_create(button_btn);
    lv_obj_center(button_label);
    lang_item_add(&widget_list, button_label, NULL, "peple_key", 0);

    lv_obj_t * time_btn = lv_btn_create(screen_white);
    lv_obj_align(time_btn, LV_ALIGN_CENTER, 0, 80);

    time_label = lv_label_create(time_btn);
    lv_obj_center(time_label);
    lang_item_add(&widget_list, time_label, NULL, "locate_key", 0);

    return screen_white;
}
