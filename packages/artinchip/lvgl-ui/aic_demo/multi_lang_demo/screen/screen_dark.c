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

lv_obj_t *screen_dark = NULL;

static lv_ll_t widget_list;

static lv_obj_t *menu_label = NULL;
static lv_obj_t *button_label = NULL;

static void key_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        lv_scr_load(screen_white);
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

lv_obj_t *init_screen_dark(void)
{
    lang_item_init(&widget_list, sizeof(lang_item_t));

    screen_dark = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_dark, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(screen_dark, screen_event_handler, LV_EVENT_ALL, NULL);

    lv_obj_t * menu_btn = lv_btn_create(screen_dark);
    lv_obj_align(menu_btn, LV_ALIGN_CENTER, 0, -80);
    lv_obj_add_event_cb(menu_btn, key_event_handler, LV_EVENT_ALL, NULL);

    menu_label = lv_label_create(menu_btn);
    lv_obj_center(menu_label);
    lang_item_add(&widget_list, menu_label, NULL, "light_key", 0);

    lv_obj_t * button_btn = lv_btn_create(screen_dark);
    lv_obj_align(button_btn, LV_ALIGN_CENTER, 0, 0);

    button_label = lv_label_create(button_btn);
    lv_obj_center(button_label);
    lang_item_add(&widget_list, button_label, NULL, "peple_key", 0);

    return screen_dark;
}
