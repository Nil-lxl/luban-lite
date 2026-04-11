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
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

static lv_obj_t *img_bg = NULL;
static lv_obj_t *arc = NULL;
static lv_obj_t *bg_fps = NULL;
#ifdef KERNEL_RTTHREAD
static lv_obj_t *bg_cpu = NULL;
#endif
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

static int last_high = -1;
static int last_low = -1;
static int end_degree = 250;
static int direct = 0;
static int cur_degree = 0;
static int max_speed = 99;

static lv_obj_t *obj_list[17] = { NULL };

LV_FONT_DECLARE(ui_font_regular);

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

static void fps_callback(lv_timer_t *tmr)
{
    char data_str[128];
#ifdef KERNEL_RTTHREAD
    float value;
#endif

    (void)tmr;

    /* frame rate */
    ui_snprintf(data_str, "%2d FPS", fbdev_draw_fps());
    lv_label_set_text(bg_fps, data_str);

#ifdef KERNEL_RTTHREAD
    /* cpu usage */
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
    value = cpu_load_average();
#else
    value = 0;
#endif

    ui_snprintf(data_str, "%d%% CPU", (int)value);
    lv_label_set_text(bg_cpu, data_str);
#endif /* KERNEL_RTTHREAD */
    return;
}

static void speed_callback(lv_timer_t *tmr)
{
    char data_str[128];
    int speed_num = ((cur_degree  << 8) / end_degree * max_speed) >> 8;

    int cur_low = speed_num % 10;
    int cur_high = speed_num / 10;

    if (cur_low != last_low) {
        ui_snprintf(data_str, "%sspeed_num/s_%d.png", LVGL_DIR, speed_num % 10);
        lv_img_set_src(img_speed_low, data_str);
        last_low = cur_low;
    }

    if (cur_high != last_high) {
        ui_snprintf(data_str, "%sspeed_num/s_%d.png", LVGL_DIR, speed_num / 10);
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

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[0]);
    lv_img_set_src(img_trip_0, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[1]);
    lv_img_set_src(img_trip_1, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[2]);
    lv_img_set_src(img_trip_2, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DIR, num[3]);
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

void dashboard_ui_init()
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    img_bg = lv_img_create(lv_scr_act());
    lv_img_set_src(img_bg, LVGL_PATH(bg/normal.jpg));
    lv_obj_center(img_bg);

    bg_fps = lv_label_create(lv_scr_act());
    lv_obj_set_width(bg_fps, LV_SIZE_CONTENT);
    lv_obj_set_height(bg_fps, LV_SIZE_CONTENT);
    lv_obj_align(bg_fps, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(bg_fps, 20, 30);
    lv_label_set_text(bg_fps, "");
    lv_obj_set_style_text_color(bg_fps, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bg_fps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bg_fps, &ui_font_regular, LV_PART_MAIN | LV_STATE_DEFAULT);

#ifdef KERNEL_RTTHREAD
    bg_cpu = lv_label_create(lv_scr_act());
    lv_obj_set_width(bg_cpu, LV_SIZE_CONTENT);
    lv_obj_set_height(bg_cpu, LV_SIZE_CONTENT);
    lv_obj_align(bg_cpu, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(bg_cpu, 20, 10);
    lv_label_set_text(bg_cpu, "");
    lv_obj_set_style_text_color(bg_cpu, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bg_cpu, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bg_cpu, &ui_font_regular, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif

    static lv_style_t style_bg;
    lv_style_init(&style_bg);
    lv_style_set_arc_rounded(&style_bg, 0);
    lv_style_set_arc_width(&style_bg, 0);
    lv_style_set_arc_opa(&style_bg, LV_OPA_0);

    static lv_style_t style_fp;
    lv_style_init(&style_fp);

    lv_style_set_arc_color(&style_fp, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_arc_rounded(&style_fp, 0);
    lv_style_set_arc_width(&style_fp, 80);
    lv_style_set_arc_img_src(&style_fp, LVGL_PATH(point/normal.png));

    // Create an Arc
    arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc, 540, 540);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_angles(arc, 0, end_degree);
    lv_arc_set_bg_angles(arc, 0, end_degree);
    lv_arc_set_range(arc, 0, max_speed);
    lv_arc_set_value(arc, 0);
    lv_arc_set_end_angle(arc, 0);
    lv_obj_center(arc);
    lv_obj_add_style(arc, &style_fp, LV_PART_INDICATOR);
    lv_obj_add_style(arc, &style_bg, LV_PART_MAIN);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    // speed
    img_speed_high = lv_img_create(lv_scr_act());
    lv_img_set_src(img_speed_high, LVGL_PATH(speed_num/s_0.png));
    lv_obj_set_pos(img_speed_high, 450, 280);

    img_speed_low = lv_img_create(lv_scr_act());
    lv_img_set_src(img_speed_low, LVGL_PATH(speed_num/s_0.png));
    lv_obj_set_pos(img_speed_low, 520, 280);

    img_speed_km = lv_img_create(lv_scr_act());
    lv_img_set_src(img_speed_km, LVGL_PATH(speed_num/s_kmh.png));
    lv_obj_set_pos(img_speed_km, 470, 370);

    img_t1 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t1, LVGL_PATH(head/t_1.png));
    lv_obj_set_pos(img_t1, 16, 50);

    img_t2 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t2, LVGL_PATH(head/t_2.png));
    lv_obj_set_pos(img_t2, 66, 50);

    img_t3 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t3, LVGL_PATH(head/t_3.png));
    lv_obj_set_pos(img_t3, 160, 8);

    img_t4 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t4, LVGL_PATH(head/t_4.png));
    lv_obj_set_pos(img_t4, 220, 8);

    img_t12 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t12, LVGL_PATH(head/t_12.png));
    lv_obj_set_pos(img_t12, 414, 8);

    img_t5 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t5, LVGL_PATH(head/t_5.png));
    lv_obj_set_pos(img_t5, 770, 8);

    img_t6 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t6, LVGL_PATH(head/t_6.png));
    lv_obj_set_pos(img_t6, 822, 8);

    img_t8 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_t8, LVGL_PATH(head/t_8.png));
    lv_obj_set_pos(img_t8, 878, 8);

    img_b1 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b1, LVGL_PATH(end/b_1.png));
    lv_obj_set_pos(img_b1, 82, 528);

    img_b2 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b2, LVGL_PATH(end/b_2.png));
    lv_obj_set_pos(img_b2, 140, 528);

    img_b3 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b3, LVGL_PATH(end/b_3.png));
    lv_obj_set_pos(img_b3, 202, 528);

    img_b4 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b4, LVGL_PATH(end/b_4.png));
    lv_obj_set_pos(img_b4, 262, 528);

    img_b5 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b5, LVGL_PATH(end/b_5.png));
    lv_obj_set_pos(img_b5, 322, 528);

    img_b6 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b6, LVGL_PATH(end/b_6.png));
    lv_obj_set_pos(img_b6, 656, 528);

    img_b7 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b7, LVGL_PATH(end/b_7.png));
    lv_obj_set_pos(img_b7, 720, 528);

    img_b8 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b8, LVGL_PATH(end/b_8.png));
    lv_obj_set_pos(img_b8, 782, 528);

    img_b9 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_b9, LVGL_PATH(end/b_9.png));
    lv_obj_set_pos(img_b9, 838, 528);

    int trip_offset = 429;
    img_trip = lv_img_create(lv_scr_act());
    lv_img_set_src(img_trip, LVGL_PATH(mileage/trip.png));
    lv_obj_set_pos(img_trip, trip_offset, 534);

    img_trip_0 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_trip_0, LVGL_PATH(mileage/0.png));
    lv_obj_set_pos(img_trip_0, 65 + trip_offset, 534);

    img_trip_1 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_trip_1, LVGL_PATH(mileage/0.png));
    lv_obj_set_pos(img_trip_1, 80 + trip_offset, 534);

    img_trip_2 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_trip_2, LVGL_PATH(mileage/9.png));
    lv_obj_set_pos(img_trip_2, 95 + trip_offset, 534);

    img_trip_3 = lv_img_create(lv_scr_act());
    lv_img_set_src(img_trip_3, LVGL_PATH(mileage/8.png));
    lv_obj_set_pos(img_trip_3, 110 + trip_offset, 534);

    img_trip_km = lv_img_create(lv_scr_act());
    lv_img_set_src(img_trip_km, LVGL_PATH(mileage/km.png));
    lv_obj_set_pos(img_trip_km, 130 + trip_offset, 534);

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

    lv_timer_create(point_callback, 10, 0);
    lv_timer_create(fps_callback, 1000, 0);
    lv_timer_create(speed_callback, 60, 0);
    lv_timer_create(trip_callback, 1000 * 5, 0);
    lv_timer_create(signal_callback, 500, 0);
}

void ui_init(void)
{
    dashboard_ui_init();
}
