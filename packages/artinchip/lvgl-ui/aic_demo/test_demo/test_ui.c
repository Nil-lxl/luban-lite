/*
 * Copyright (C) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "test_ui.h"

static lv_obj_t* scr;

static void touch_cb(lv_event_t* e){
    lv_event_code_t code=lv_event_get_code(e);
    if(code==LV_EVENT_GESTURE){
        lv_dir_t direction=lv_indev_get_gesture_dir(lv_indev_get_act());
        if(direction=LV_DIR_TOP){
            lv_obj_set_style_bg_color(scr,lv_palette_lighten(LV_PALETTE_ORANGE,3),0);
        }
    }
}

void test_ui_init()
{
    scr=lv_scr_act();
    lv_obj_set_style_bg_color(scr,lv_palette_lighten(LV_PALETTE_BLUE,3),0);

    lv_obj_t* sli=lv_slider_create(scr);
    lv_obj_center(sli);
    lv_obj_set_width(sli,1000);

    lv_obj_add_event_cb(scr,touch_cb,LV_EVENT_ALL,NULL);
}

void ui_init(void)
{
    test_ui_init();
}
