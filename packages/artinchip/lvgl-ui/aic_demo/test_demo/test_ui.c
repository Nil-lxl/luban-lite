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
#include "rtthread.h"

#define LCD_HOR_RES 1920
#define LCD_VER_RES 1080

static lv_obj_t *scr;
static lv_obj_t *gray_scr;
static lv_obj_t *touch_scr;
static lv_obj_t *container;
static lv_timer_t *timer;
static uint8_t sum;
static lv_obj_t *img1;
static lv_obj_t *img2;
static lv_obj_t *img3;


static rt_thread_t test_thread;

void timer_cb(lv_timer_t *timer) {
    switch (sum) {
        case 0:
            /* code */
            lv_obj_add_flag(img1, LV_OBJ_FLAG_HIDDEN);
            lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
            lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
            sum++;
            break;
        case 1:
            lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(scr, lv_color_hex(0xff0000), 0);//red
            sum++;
            break;
        case 2:
            lv_obj_set_style_bg_color(scr, lv_color_hex(0x00ff00), 0);//green
            sum++;
            break;
        case 3:
            lv_obj_set_style_bg_color(scr, lv_color_hex(0x0000ff), 0);//blue
            sum++;
            break;
        case 4:
            lv_obj_set_style_bg_color(scr, lv_color_hex(0xff00f0), 0);//pink
            sum++;
            break;
        case 5:
            lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
            sum++;
            break;
        case 6:
            lv_scr_load_anim(gray_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
            sum++;
            break;
        case 7:
            lv_obj_clear_flag(img1, LV_OBJ_FLAG_HIDDEN);
            // lv_scr_load_anim(touch_scr,LV_SCR_LOAD_ANIM_NONE,0,0,false);
            sum = 0;
            break;
        case 8:
            // sum=0;
            break;

        default:
            break;
    }
}

static void touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_user_data(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t direction = lv_indev_get_gesture_dir(lv_indev_get_act());

        switch (direction) {
            case LV_DIR_LEFT:
                lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_ORANGE, 3), 0);
                break;
            case LV_DIR_RIGHT:
                lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_BLUE, 3), 0);
                break;
            case LV_DIR_TOP:
                lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_PINK, 3), 0);
                break;
            case LV_DIR_BOTTOM:
                lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_PURPLE, 3), 0);
                break;
            default:
                break;
        }



    }
}
void lv_hide_obj(lv_obj_t* obj){
    lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
}
void lv_show_obj(lv_obj_t* obj){
    if(lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN)){
        lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);
    }
}


void test_thread_entry(void *param) {
    while (1) {
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        lv_show_obj(container);
        rt_thread_mdelay(1000);

        lv_hide_obj(container);
        // lv_obj_set_style_bg_color(scr, lv_color_hex(0xff0000), 0);//red
        // rt_thread_mdelay(1000);

        // lv_obj_set_style_bg_color(scr, lv_color_hex(0x00ff00), 0);//green
        // rt_thread_mdelay(1000);

        // lv_obj_set_style_bg_color(scr, lv_color_hex(0x0000ff), 0);//blue
        // rt_thread_mdelay(1000);

        // lv_obj_set_style_bg_color(scr, lv_color_hex(0xff00f0), 0);//pink
        // rt_thread_mdelay(1000);

        lv_show_obj(img1);
        rt_thread_mdelay(1000);

        lv_hide_obj(img1);
        lv_show_obj(img2);
        rt_thread_mdelay(1500);

        lv_hide_obj(img2);
        lv_show_obj(img3);
        rt_thread_mdelay(1500);

        lv_hide_obj(img3);

    }
}

void test_ui_init() {
    scr = lv_scr_act();

    container = lv_obj_create(scr);
    lv_obj_center(container);
    lv_obj_set_size(container, LCD_HOR_RES - 40, LCD_VER_RES - 20);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_border_color(container, lv_color_white(), 0);

    img1 = lv_img_create(scr);
    lv_img_set_src(img1, LVGL_PATH(images/img1920x1080_4.jpg));
    img2 = lv_img_create(scr);
    lv_img_set_src(img2, LVGL_PATH(images/img1920x1080_5.jpg));
    img3 = lv_img_create(scr);
    lv_img_set_src(img3, LVGL_PATH(images/img1920x1080_6.jpg));
    lv_obj_add_flag(img1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img3, LV_OBJ_FLAG_HIDDEN);



    // gray_lvl();
    // touch_test();
    // timer = lv_timer_create(timer_cb, 1000, NULL);

    // lv_obj_add_event_cb(scr,touch_cb,LV_EVENT_GESTURE,NULL);
    test_thread = rt_thread_create("display_test", test_thread_entry, NULL, 8 * 1024, 17, 5);
    if (test_thread != NULL) {
        rt_thread_startup(test_thread);
    }
}

static lv_obj_t *gray_bar[10];
const uint32_t gray_color[10] = { 0x303030,0x404040,0x505050,0x606060,0x707070,0x808080,0x909090,0xA0A0A0,0xB0B0B0,0xC0C0C0 };
void create_gray_lvl() {
    gray_scr = lv_obj_create(NULL);
    lv_obj_set_size(gray_scr, LCD_HOR_RES, LCD_VER_RES);
    for (int i = 0;i < 10;i++) {
        gray_bar[i] = lv_obj_create(gray_scr);
        lv_obj_set_size(gray_bar[i], lv_pct(100), LCD_VER_RES / 10);
        lv_obj_set_pos(gray_bar[i], 0, (LCD_VER_RES / 10) * i);
        lv_obj_set_style_pad_all(gray_bar[i], 0, 0);
        lv_obj_set_style_border_width(gray_bar[i], 0, 0);
        lv_obj_set_style_radius(gray_bar[i], 0, 0);
        lv_obj_set_style_bg_color(gray_bar[i], lv_color_hex(gray_color[i]), 0);
    }
}

void touch_test() {
    touch_scr = lv_obj_create(NULL);
    // lv_obj_set_size(touch_scr,LCD_HOR_RES,LCD_VER_RES);
    lv_obj_set_style_bg_color(touch_scr, lv_color_white(), 0);

    lv_obj_add_event_cb(touch_scr, touch_cb, LV_EVENT_GESTURE, touch_scr);

}
void ui_init(void) {
    test_ui_init();
}
