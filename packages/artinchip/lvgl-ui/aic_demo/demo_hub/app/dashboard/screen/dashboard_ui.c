/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "dashboard_ui.h"
#include "aic_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"

typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

static void ui_dashboard_cb(lv_event_t *e);

static lv_obj_t *dashboard_ui = NULL;
static lv_obj_t *img_bg = NULL;
static lv_obj_t *arc = NULL;
static lv_obj_t *img_speed_high = NULL;
static lv_obj_t *img_speed_low = NULL;
static lv_obj_t *img_speed_km = NULL;
static lv_obj_t *img_t1 = NULL;
static lv_obj_t *img_t2 = NULL;
static lv_obj_t *img_t3 = NULL;
static lv_obj_t *img_t4 = NULL;
static lv_obj_t *img_t5 = NULL;
static lv_obj_t *img_t6 = NULL;
static lv_obj_t *img_t8 = NULL;
static lv_obj_t *img_t12 = NULL;

static lv_obj_t *img_b1 = NULL;
static lv_obj_t *img_b2 = NULL;
static lv_obj_t *img_b3 = NULL;
static lv_obj_t *img_b4 = NULL;
static lv_obj_t *img_b5 = NULL;
static lv_obj_t *img_b6 = NULL;
static lv_obj_t *img_b7 = NULL;
static lv_obj_t *img_b8 = NULL;
static lv_obj_t *img_b9 = NULL;

static lv_obj_t *img_trip = NULL;
static lv_obj_t *img_trip_0 = NULL;
static lv_obj_t *img_trip_1 = NULL;
static lv_obj_t *img_trip_2 = NULL;
static lv_obj_t *img_trip_3 = NULL;
static lv_obj_t *img_trip_km = NULL;

static lv_timer_t *point_timer = NULL;
static lv_timer_t *speed_timer = NULL;
static lv_timer_t *trip_timer = NULL;
static lv_timer_t *signal_timer = NULL;

static int last_high = -1;
static int last_low = -1;
static int end_degree = 250;
static int direct = 0;
static int cur_degree = 0;
static int max_speed = 99;

static lv_obj_t *obj_list[17] = { NULL };
static disp_size_t disp_size;

LV_FONT_DECLARE(ui_font_Title);

static int pos_width_conversion(int width)
{
    float scr_width = LV_HOR_RES;
    int out = (int)((scr_width / 1024.0) * width);
    return out;
}

static int pos_height_conversion(int height)
{
    float scr_height = LV_VER_RES;
    return (int)((scr_height / 600.0) * (float)height);
}

static void point_callback(lv_timer_t *tmr)
{
    lv_arc_set_end_angle(arc, cur_degree);
    if (direct == 0) {
        cur_degree++;
        if (cur_degree > end_degree) {
            cur_degree = end_degree - 1;
            direct = 1;
        }
    } else {
        cur_degree--;
        if (cur_degree < 0) {
            cur_degree = 1;
            direct = 0;
        }
    }

    return;
}

static void speed_callback(lv_timer_t *tmr)
{
    char data_str[128];
    int speed_num = ((cur_degree  << 8) / end_degree * max_speed) >> 8;

    int cur_low = speed_num % 10;
    int cur_high = speed_num / 10;

    if (cur_low != last_low) {
        ui_snprintf(data_str, "%sdashboard/speed_num/s_%d.png", LVGL_DIR, speed_num % 10);
        lv_img_set_src(img_speed_low, data_str);
        last_low = cur_low;
    }

    if (cur_high != last_high) {
        ui_snprintf(data_str, "%sdashboard/speed_num/s_%d.png", LVGL_DIR, speed_num / 10);
        lv_img_set_src(img_speed_high, data_str);
        last_high = cur_high;
    }
}

static void trip_callback(lv_timer_t *tmr)
{
    char data_str[128];
    int num[4];
    int cur;
    static int trip = 98;

    (void)tmr;
    trip++;
    if (trip >= 9999)
        trip = 0;

    num[0] = trip / 1000;
    cur = trip % 1000;
    num[1] = cur / 100;
    cur = cur % 100;
    num[2] = cur / 10;
    num[3] = cur % 10;

    ui_snprintf(data_str, "%sdashboard/mileage/%d.png", LVGL_DIR, num[0]);
    lv_img_set_src(img_trip_0, data_str);

    ui_snprintf(data_str, "%sdashboard/mileage/%d.png", LVGL_DIR, num[1]);
    lv_img_set_src(img_trip_1, data_str);

    ui_snprintf(data_str, "%sdashboard/mileage/%d.png", LVGL_DIR, num[2]);
    lv_img_set_src(img_trip_2, data_str);

    ui_snprintf(data_str, "%sdashboard/mileage/%d.png", LVGL_DIR, num[3]);
    lv_img_set_src(img_trip_3, data_str);
}

static void obj_set_clear_hidden_flag(lv_obj_t *obj)
{
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void signal_callback(lv_timer_t *tmr)
{
    static int index = 0;
    static int mode = 0;
    (void)tmr;

    if (obj_list[index]) {
        obj_set_clear_hidden_flag(obj_list[index]);
    }

    mode++;
    if (mode == 2) {
        mode = 0;
        index++;
        if (index == 17) {
            index = 0;
        }
    }
}

lv_obj_t *dashboard_ui_init(void)
{
    if(LV_HOR_RES <= 480) disp_size = DISP_SMALL;
    else if(LV_HOR_RES <= 800) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    dashboard_ui = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(dashboard_ui, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(dashboard_ui, LV_OPA_100, 0);

    lv_obj_t *black_bg = lv_obj_create(dashboard_ui);
    lv_obj_remove_style_all(black_bg);
    lv_obj_set_align(black_bg, LV_ALIGN_TOP_LEFT);
    lv_obj_set_size(black_bg, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(black_bg, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(black_bg, LV_OPA_100, 0);

    img_bg = lv_img_create(dashboard_ui);
    if (disp_size == DISP_SMALL) {
        lv_img_set_src(img_bg, LVGL_PATH(dashboard/bg/normal.png));
    } else if (disp_size == DISP_LARGE) {
        lv_img_set_src(img_bg, LVGL_PATH(dashboard/bg/normal.jpg));
    }
    lv_obj_set_align(img_bg, LV_ALIGN_CENTER);
    lv_obj_set_style_opa(img_bg, LV_OPA_100, 0);

    static lv_style_t style_bg;
    if (style_bg.prop_cnt > 1) {
        lv_style_reset(&style_bg);
    } else {
        lv_style_init(&style_bg);
    }
    lv_style_set_arc_rounded(&style_bg, 0);
    lv_style_set_arc_width(&style_bg, 0);
    lv_style_set_arc_opa(&style_bg, LV_OPA_0);

    static lv_style_t style_fp;
    if (style_fp.prop_cnt > 1) {
        lv_style_reset(&style_fp);
    } else {
        lv_style_init(&style_fp);
    }
    lv_style_set_arc_color(&style_fp, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_arc_rounded(&style_fp, 0);
    if (disp_size == DISP_SMALL) {
        lv_style_set_arc_width(&style_fp, 80 * 0.45);
    } else if (disp_size == DISP_LARGE) {
        lv_style_set_arc_width(&style_fp, 80);
    }
    lv_style_set_arc_img_src(&style_fp, LVGL_PATH(dashboard/point/normal.png));
    // Create an Arc
    arc = lv_arc_create(dashboard_ui);
    if (disp_size == DISP_SMALL) {
        lv_obj_set_size(arc, 243, 243); /* this widget must maintain image aspect ratio */
        lv_arc_set_rotation(arc, 135);
        lv_obj_set_pos(arc, 2, 0);
    } else if (disp_size == DISP_LARGE) {
        lv_obj_set_size(arc, 540, 540);
        lv_arc_set_rotation(arc, 135);
    }
    lv_arc_set_angles(arc, 0, end_degree);
    lv_arc_set_bg_angles(arc, 0, end_degree);
    lv_arc_set_range(arc, 0, max_speed);
    lv_arc_set_value(arc, 0);
    lv_arc_set_end_angle(arc, 0);
    lv_obj_add_style(arc, &style_fp, LV_PART_INDICATOR);
    lv_obj_add_style(arc, &style_bg, LV_PART_MAIN);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_align(arc, LV_ALIGN_CENTER);

    // speed
    img_speed_high = lv_img_create(dashboard_ui);
    lv_img_set_src(img_speed_high, LVGL_PATH(dashboard/speed_num/s_0.png));
    lv_obj_set_pos(img_speed_high, pos_width_conversion(450), pos_height_conversion(280));

    img_speed_low = lv_img_create(dashboard_ui);
    lv_img_set_src(img_speed_low, LVGL_PATH(dashboard/speed_num/s_0.png));
    lv_obj_set_pos(img_speed_low, pos_width_conversion(520), pos_height_conversion(280));

    img_speed_km = lv_img_create(dashboard_ui);
    lv_img_set_src(img_speed_km, LVGL_PATH(dashboard/speed_num/s_kmh.png));
    lv_obj_set_pos(img_speed_km, pos_width_conversion(470), pos_height_conversion(370));

    img_t1 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t1, LVGL_PATH(dashboard/head/t_1.png));
    lv_obj_set_pos(img_t1, pos_width_conversion(40), pos_height_conversion(8));

    img_t2 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t2, LVGL_PATH(dashboard/head/t_2.png));
    lv_obj_set_pos(img_t2, pos_width_conversion(100), pos_height_conversion(8));

    img_t3 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t3, LVGL_PATH(dashboard/head/t_3.png));
    lv_obj_set_pos(img_t3, pos_width_conversion(160), pos_height_conversion(8));

    img_t4 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t4, LVGL_PATH(dashboard/head/t_4.png));
    lv_obj_set_pos(img_t4, pos_width_conversion(220), pos_height_conversion(8));

    img_t12 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t12, LVGL_PATH(dashboard/head/t_12.png));
    lv_obj_set_pos(img_t12, pos_width_conversion(414), pos_height_conversion(8));

    img_t5 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t5, LVGL_PATH(dashboard/head/t_5.png));
    lv_obj_set_pos(img_t5, pos_width_conversion(770), pos_height_conversion(8));

    img_t6 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t6, LVGL_PATH(dashboard/head/t_6.png));
    lv_obj_set_pos(img_t6, pos_width_conversion(822), pos_height_conversion(8));

    img_t8 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_t8, LVGL_PATH(dashboard/head/t_8.png));
    lv_obj_set_pos(img_t8, pos_width_conversion(878), pos_height_conversion(8));

    img_b1 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b1, LVGL_PATH(dashboard/end/b_1.png));
    lv_obj_set_pos(img_b1, pos_width_conversion(82), pos_height_conversion(528));

    img_b2 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b2, LVGL_PATH(dashboard/end/b_2.png));
    lv_obj_set_pos(img_b2, pos_width_conversion(140), pos_height_conversion(528));

    img_b3 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b3, LVGL_PATH(dashboard/end/b_3.png));
    lv_obj_set_pos(img_b3, pos_width_conversion(202), pos_height_conversion(528));

    img_b4 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b4, LVGL_PATH(dashboard/end/b_4.png));
    lv_obj_set_pos(img_b4, pos_width_conversion(262), pos_height_conversion(528));

    img_b5 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b5, LVGL_PATH(dashboard/end/b_5.png));
    lv_obj_set_pos(img_b5, pos_width_conversion(322), pos_height_conversion(528));

    img_b6 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b6, LVGL_PATH(dashboard/end/b_6.png));
    lv_obj_set_pos(img_b6, pos_width_conversion(656), pos_height_conversion(528));

    img_b7 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b7, LVGL_PATH(dashboard/end/b_7.png));
    lv_obj_set_pos(img_b7, pos_width_conversion(720), pos_height_conversion(528));

    img_b8 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b8, LVGL_PATH(dashboard/end/b_8.png));
    lv_obj_set_pos(img_b8, pos_width_conversion(782), pos_height_conversion(528));

    img_b9 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_b9, LVGL_PATH(dashboard/end/b_9.png));
    lv_obj_set_pos(img_b9, pos_width_conversion(838), pos_height_conversion(528));

    int trip_offset = pos_width_conversion(429);
    img_trip = lv_img_create(dashboard_ui);
    lv_img_set_src(img_trip, LVGL_PATH(dashboard/mileage/trip.png));
    lv_obj_set_pos(img_trip, trip_offset, pos_height_conversion(534));

    img_trip_0 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_trip_0, LVGL_PATH(dashboard/mileage/0.png));
    lv_obj_set_pos(img_trip_0, pos_width_conversion(65) + trip_offset, pos_height_conversion(534));

    img_trip_1 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_trip_1, LVGL_PATH(dashboard/mileage/0.png));
    lv_obj_set_pos(img_trip_1, pos_width_conversion(80) + trip_offset, pos_height_conversion(534));

    img_trip_2 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_trip_2, LVGL_PATH(dashboard/mileage/9.png));
    lv_obj_set_pos(img_trip_2, pos_width_conversion(95) + trip_offset, pos_height_conversion(534));

    img_trip_3 = lv_img_create(dashboard_ui);
    lv_img_set_src(img_trip_3, LVGL_PATH(dashboard/mileage/8.png));
    lv_obj_set_pos(img_trip_3, pos_width_conversion(110) + trip_offset, pos_height_conversion(534));

    img_trip_km = lv_img_create(dashboard_ui);
    lv_img_set_src(img_trip_km, LVGL_PATH(dashboard/mileage/km.png));
    lv_obj_set_pos(img_trip_km, pos_width_conversion(130) + trip_offset, pos_height_conversion(534));

    obj_list[0] = img_t1;
    obj_list[1] = img_t2;
    obj_list[2] = img_t3;
    obj_list[3] = img_t4;
    obj_list[4] = img_t12;
    obj_list[5] = img_t5;
    obj_list[6] = img_t6;
    obj_list[7] = img_t8;

    obj_list[8] = img_b1;
    obj_list[9] = img_b2;
    obj_list[10] = img_b3;
    obj_list[11] = img_b4;
    obj_list[12] = img_b5;
    obj_list[13] = img_b6;
    obj_list[14] = img_b7;
    obj_list[15] = img_b8;
    obj_list[16] = img_b9;

    point_timer = lv_timer_create(point_callback, 10, 0);
    speed_timer = lv_timer_create(speed_callback, 60, 0);
    trip_timer = lv_timer_create(trip_callback, 1000 * 5, 0);
    signal_timer = lv_timer_create(signal_callback, 500, 0);

    lv_obj_add_event_cb(dashboard_ui, ui_dashboard_cb, LV_EVENT_ALL, NULL);

    return dashboard_ui;
}

static void ui_dashboard_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_obj_clean(dashboard_ui); /* pre-release memory to avoid issues of insufficient memory when loading hte next screen */
        lv_timer_del(point_timer);
        lv_timer_del(speed_timer);
        lv_timer_del(trip_timer);
        lv_timer_del(signal_timer);
    }

    if (code == LV_EVENT_DELETE) {;}

    if (code == LV_EVENT_SCREEN_LOADED) {;}
}
