/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "demo_hub.h"
#include "footer_ui.h"
#include "sys_met_hub.h"

#include  "../app_entrance.h"
#include "aic_core.h"

#define NOT_DISPLAY_PERFORMANCE 1
#define NOT_DISPLAY_TIME        0

typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

typedef struct _icon_app_list {
    void *icon_path;
    void *icon_pre_path;
    void (*event_cb)(lv_event_t * e);
} icon_app_list;

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);

static disp_size_t disp_size;
static sys_met_hub *sys_info;

static int last_tabview_tab = 0;

static void widgets_example_cb(lv_event_t * e)
{
    app_entrance(APP_WIDGETS, 1);
}

static void input_test_ui_cb(lv_event_t * e)
{
    app_entrance(APP_INPUT_TEST, 1);
}

static void layout_list_ui_cb(lv_event_t * e)
{
    app_entrance(APP_LAYOUT_LIST_EXAMPLE, 1);
}

static void layout_table_ui_cb(lv_event_t * e)
{
    app_entrance(APP_LAYOUT_TABLE_EXAMPLE, 1);
}

static void dashboard_ui_cb(lv_event_t * e)
{
    app_entrance(APP_DASHBOARD, 1);
}

static void elevator_ui_cb(lv_event_t * e)
{
#if defined(AIC_CHIP_D12X)
    if (LV_HOR_RES == 480)
        return;
#endif
    app_entrance(APP_ELEVATOR, 1);
}

static void aic_video_ui_cb(lv_event_t * e)
{
#if defined(AIC_CHIP_D12X)
    if (LV_HOR_RES == 480)
        return;
#endif
    app_entrance(APP_VIDEO, 1);
}

static void aic_audio_ui_cb(lv_event_t * e)
{
#if defined(AIC_CHIP_D13X)
    if (LV_HOR_RES == 480)
        app_entrance(APP_AUDIO, 1);
#endif
}

static void camera_ui_cb(lv_event_t * e)
{
#if defined(AIC_CHIP_D12X)
    if (LV_HOR_RES == 480)
        return;
#endif
    if (LV_HOR_RES == 1024)
        return;
    app_entrance(APP_CAMERA, 1);
}

static void coffee_ui_cb(lv_event_t * e)
{
    app_entrance(APP_COFFEE, 1);
}

static void a_86box_ui_cb(lv_event_t * e)
{
    app_entrance(APP_86BOX, 1);
}

static void steam_box_ui_cb(lv_event_t * e)
{
    app_entrance(APP_STEAM_BOX, 1);
}

static void photo_frame_ui_cb(lv_event_t * e)
{
    app_entrance(APP_PHOTO_FRAME, 1);
}

static icon_app_list icon_list_480_272[] = {
    {LVGL_PATH(navigator/dashboard.png), LVGL_PATH(navigator/dashboard_selected.png), dashboard_ui_cb},
    {LVGL_PATH(navigator/elevator.png), LVGL_PATH(navigator/elevator_selected.png), elevator_ui_cb},
    {LVGL_PATH(navigator/video.png), LVGL_PATH(navigator/video_selected.png), aic_video_ui_cb},
    {LVGL_PATH(navigator/music.png), LVGL_PATH(navigator/music_selected.png), aic_audio_ui_cb},
    {LVGL_PATH(navigator/coffee.png), LVGL_PATH(navigator/coffee_selected.png), coffee_ui_cb},
    {LVGL_PATH(navigator/camera.png), LVGL_PATH(navigator/camera_selected.png), camera_ui_cb},
    {LVGL_PATH(navigator/stream.png), LVGL_PATH(navigator/stream_selected.png), steam_box_ui_cb},
    {LVGL_PATH(navigator/a86_box.png), LVGL_PATH(navigator/a86_box_selected.png), a_86box_ui_cb},

    {LVGL_PATH(navigator/photo_frame.png), LVGL_PATH(navigator/photo_frame_selected.png), photo_frame_ui_cb},
    {LVGL_PATH(navigator/widgets.png), LVGL_PATH(navigator/widgets_selected.png), widgets_example_cb},
    {LVGL_PATH(navigator/stress.png), LVGL_PATH(navigator/stress_selected.png), NULL},
    {LVGL_PATH(navigator/benchmark.png), LVGL_PATH(navigator/benchmark_selected.png), NULL},
    {LVGL_PATH(navigator/keypad_encoder.png), LVGL_PATH(navigator/keypad_encoder_selected.png), input_test_ui_cb},
    {LVGL_PATH(navigator/list.png), LVGL_PATH(navigator/list_selected.png), layout_list_ui_cb},
    {LVGL_PATH(navigator/table.png), LVGL_PATH(navigator/table_selected.png), layout_table_ui_cb},
    {LVGL_PATH(navigator/printer.png), LVGL_PATH(navigator/printer_selected.png), NULL},

    {LVGL_PATH(navigator/weather.png), LVGL_PATH(navigator/weather_selected.png), NULL},
    {LVGL_PATH(navigator/alarm_clock.png), LVGL_PATH(navigator/alarm_clock_selected.png), NULL},
    {LVGL_PATH(navigator/calculator.png), LVGL_PATH(navigator/calculator_selected.png), NULL},
    {LVGL_PATH(navigator/screen_casting.png), LVGL_PATH(navigator/screen_casting_selected.png), NULL},
    {LVGL_PATH(navigator/monitor.png), LVGL_PATH(navigator/monitor_selected.png), NULL},
    {LVGL_PATH(navigator/ebike.png), LVGL_PATH(navigator/ebike_selected.png), NULL},
    {LVGL_PATH(navigator/gateway.png), LVGL_PATH(navigator/gateway_selected.png), NULL},
    {LVGL_PATH(navigator/charging_station.png), LVGL_PATH(navigator/charging_station_selected.png), NULL},
};

static icon_app_list icon_list_720_720[] = {
    {LVGL_PATH(navigator/dashboard.png), LVGL_PATH(navigator/dashboard_selected.png), dashboard_ui_cb},
    {LVGL_PATH(navigator/elevator.png), LVGL_PATH(navigator/elevator_selected.png), elevator_ui_cb},
    {LVGL_PATH(navigator/video.png), LVGL_PATH(navigator/video_selected.png), aic_video_ui_cb},
    {LVGL_PATH(navigator/music.png), LVGL_PATH(navigator/music_selected.png), aic_audio_ui_cb},
    {LVGL_PATH(navigator/coffee.png), LVGL_PATH(navigator/coffee_selected.png), coffee_ui_cb},
    {LVGL_PATH(navigator/camera.png), LVGL_PATH(navigator/camera_selected.png), camera_ui_cb},
    {LVGL_PATH(navigator/stream.png), LVGL_PATH(navigator/stream_selected.png), steam_box_ui_cb},
    {LVGL_PATH(navigator/a86_box.png), LVGL_PATH(navigator/a86_box_selected.png), a_86box_ui_cb},

    {LVGL_PATH(navigator/photo_frame.png), LVGL_PATH(navigator/photo_frame_selected.png), photo_frame_ui_cb},
    {LVGL_PATH(navigator/widgets.png), LVGL_PATH(navigator/widgets_selected.png), widgets_example_cb},
    {LVGL_PATH(navigator/stress.png), LVGL_PATH(navigator/stress_selected.png), NULL},
    {LVGL_PATH(navigator/benchmark.png), LVGL_PATH(navigator/benchmark_selected.png), NULL},
    {LVGL_PATH(navigator/keypad_encoder.png), LVGL_PATH(navigator/keypad_encoder_selected.png), input_test_ui_cb},
    {LVGL_PATH(navigator/list.png), LVGL_PATH(navigator/list_selected.png), layout_list_ui_cb},
    {LVGL_PATH(navigator/table.png), LVGL_PATH(navigator/table_selected.png), layout_table_ui_cb},
    {LVGL_PATH(navigator/printer.png), LVGL_PATH(navigator/printer_selected.png), NULL},

    {LVGL_PATH(navigator/weather.png), LVGL_PATH(navigator/weather_selected.png), NULL},
    {LVGL_PATH(navigator/alarm_clock.png), LVGL_PATH(navigator/alarm_clock_selected.png), NULL},
    {LVGL_PATH(navigator/calculator.png), LVGL_PATH(navigator/calculator_selected.png), NULL},
    {LVGL_PATH(navigator/screen_casting.png), LVGL_PATH(navigator/screen_casting_selected.png), NULL},
    {LVGL_PATH(navigator/monitor.png), LVGL_PATH(navigator/monitor_selected.png), NULL},
    {LVGL_PATH(navigator/ebike.png), LVGL_PATH(navigator/ebike_selected.png), NULL},
    {LVGL_PATH(navigator/gateway.png), LVGL_PATH(navigator/gateway_selected.png), NULL},
    {LVGL_PATH(navigator/charging_station.png), LVGL_PATH(navigator/charging_station_selected.png), NULL},
};

static icon_app_list icon_list_1024_600[] = {
    {LVGL_PATH(navigator/dashboard.png), LVGL_PATH(navigator/dashboard_selected.png), dashboard_ui_cb},
    {LVGL_PATH(navigator/elevator.png), LVGL_PATH(navigator/elevator_selected.png), elevator_ui_cb},
    {LVGL_PATH(navigator/video.png), LVGL_PATH(navigator/video_selected.png), aic_video_ui_cb},
    {LVGL_PATH(navigator/music.png), LVGL_PATH(navigator/music_selected.png), NULL},
    {LVGL_PATH(navigator/coffee.png), LVGL_PATH(navigator/coffee_selected.png), coffee_ui_cb},
    {LVGL_PATH(navigator/camera.png), LVGL_PATH(navigator/camera_selected.png), camera_ui_cb},
    {LVGL_PATH(navigator/stream.png), LVGL_PATH(navigator/stream_selected.png), steam_box_ui_cb},
    {LVGL_PATH(navigator/a86_box.png), LVGL_PATH(navigator/a86_box_selected.png), a_86box_ui_cb},

    {LVGL_PATH(navigator/photo_frame.png), LVGL_PATH(navigator/photo_frame_selected.png), photo_frame_ui_cb},
    {LVGL_PATH(navigator/widgets.png), LVGL_PATH(navigator/widgets_selected.png), widgets_example_cb},
    {LVGL_PATH(navigator/stress.png), LVGL_PATH(navigator/stress_selected.png), NULL},
    {LVGL_PATH(navigator/benchmark.png), LVGL_PATH(navigator/benchmark_selected.png), NULL},
    {LVGL_PATH(navigator/keypad_encoder.png), LVGL_PATH(navigator/keypad_encoder_selected.png), input_test_ui_cb},
    {LVGL_PATH(navigator/list.png), LVGL_PATH(navigator/list_selected.png), NULL},
    {LVGL_PATH(navigator/table.png), LVGL_PATH(navigator/table_selected.png), NULL},
    {LVGL_PATH(navigator/printer.png), LVGL_PATH(navigator/printer_selected.png), NULL},

    {LVGL_PATH(navigator/weather.png), LVGL_PATH(navigator/weather_selected.png), NULL},
    {LVGL_PATH(navigator/alarm_clock.png), LVGL_PATH(navigator/alarm_clock_selected.png), NULL},
    {LVGL_PATH(navigator/calculator.png), LVGL_PATH(navigator/calculator_selected.png), NULL},
    {LVGL_PATH(navigator/screen_casting.png), LVGL_PATH(navigator/screen_casting_selected.png), NULL},
    {LVGL_PATH(navigator/monitor.png), LVGL_PATH(navigator/monitor_selected.png), NULL},
    {LVGL_PATH(navigator/ebike.png), LVGL_PATH(navigator/ebike_selected.png), NULL},
    {LVGL_PATH(navigator/gateway.png), LVGL_PATH(navigator/gateway_selected.png), NULL},
    {LVGL_PATH(navigator/charging_station.png), LVGL_PATH(navigator/charging_station_selected.png), NULL},
};

static lv_obj_t *footer_container;

static lv_timer_t *timer_time;
static lv_timer_t *timer_sys;

static int hours = 0;
static int minutes = 0;
static int seconds = 0;

void time_info_update_cb(lv_timer_t * timer)
{
    lv_obj_t *time_info_container = (lv_obj_t *)timer->user_data;
    lv_obj_t *hours_info = lv_obj_get_child(time_info_container, 0);
    lv_obj_t *second_info = lv_obj_get_child(time_info_container, 1);
    lv_obj_t *min_info = lv_obj_get_child(time_info_container, 2);

    seconds++;
    if (seconds % 2)
        lv_label_set_text_fmt(second_info, ":");
    else
        lv_label_set_text_fmt(second_info, " ");

    lv_label_set_text_fmt(min_info, "%02d", minutes);
    lv_label_set_text_fmt(hours_info, "%02d", hours);
    if (seconds >= 60)  {
        minutes++;
        seconds = 0;
    }
    if (minutes >= 60)  {
        hours++;
        minutes = 0;
    }
    if (hours >= 24) hours = 0;
}

void sys_info_update_cb(lv_timer_t * timer)
{
    lv_obj_t *disp_sys_info = (lv_obj_t *)timer->user_data;

    sys_met_hub_get_info(sys_info);
    lv_label_set_text_fmt(disp_sys_info, "fps %d cpu %d", sys_info->draw_fps, (int)sys_info->cpu_usage);
}

static void act_sub_tabview(lv_event_t * e)
{
    lv_obj_t * tapview = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    int footer_num = lv_obj_get_child_cnt(footer_container);
    int footer_now = lv_tabview_get_tab_act(tapview);

    if (code == LV_EVENT_VALUE_CHANGED) {
        for (int i = 0; i < footer_num; i++) {
            footer_ui_switch(lv_obj_get_child(footer_container, i), FOOTER_TURN_OFF);
        }
        footer_ui_switch(lv_obj_get_child(footer_container, footer_now), FOOTER_TURN_ON);
        last_tabview_tab = footer_now;
    }
}

void navigator_ui_init_impl(lv_obj_t *parent)
{
    sys_info = sys_met_hub_create();

#if LVGL_VERSION_MAJOR == 8
    lv_obj_t *tv_top = lv_tabview_create(parent, LV_DIR_TOP, 0);
#else
    lv_obj_t *tv_top = lv_tabview_create(parent);
    lv_tabview_set_tab_bar_position(tv_top, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tv_top, 0);
#endif
    lv_obj_set_size(tv_top, lv_pct(100), lv_pct(100));
    lv_obj_set_align(tv_top, LV_ALIGN_TOP_LEFT);

    icon_app_list *icon_list = NULL;
    int icon_num = 0;
    if (disp_size == DISP_SMALL) {
        icon_list = icon_list_480_272;
        icon_num = (sizeof(icon_list_480_272) / sizeof(icon_list_480_272[0]));
    } else if (disp_size == DISP_MEDIUM) {
        icon_list = icon_list_720_720;
        icon_num = (sizeof(icon_list_720_720) / sizeof(icon_list_720_720[0]));
    } else {
        icon_list = icon_list_1024_600;
        icon_num = (sizeof(icon_list_1024_600) / sizeof(icon_list_1024_600[0]));
    }

    char tv_tab_name[64] = {0};
    int now_icon_num = 0;
    int tv_tab_num = icon_num / 8;
    if (icon_num % 8 != 0) tv_tab_num++;

    lv_obj_set_style_bg_opa(tv_top, LV_OPA_100, 0);
    lv_obj_set_style_bg_img_src(tv_top, LVGL_PATH(navigator/main_bg.jpg), 0);
    lv_obj_set_style_bg_img_opa(tv_top, LV_OPA_100, 0);

    footer_container = lv_obj_create(parent);
    lv_obj_set_style_border_width(footer_container, 0, 0);
    lv_obj_set_style_bg_color(footer_container, lv_theme_get_color_primary(NULL), 0);
    lv_obj_set_style_bg_opa(footer_container, LV_OPA_0, 0);
    lv_obj_set_size(footer_container, LV_HOR_RES * 0.2, LV_VER_RES * 0.2);
    lv_obj_set_align(footer_container, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_pos(footer_container, 0, LV_VER_RES * 0.05);
    lv_obj_clear_flag(footer_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_align(footer_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN);

    lv_obj_t *disp_sys_info = lv_label_create(parent);
    lv_obj_set_align(disp_sys_info, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_pos(disp_sys_info, 0, 0);
    lv_obj_set_size(disp_sys_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(disp_sys_info, lv_theme_get_color_emphasize(NULL), 0);
    lv_label_set_text_fmt(disp_sys_info, "fps %d cpu %d", 0, 0);

    lv_obj_t *time_info_container = lv_obj_create(parent);
    lv_obj_remove_style_all(time_info_container);
    lv_obj_clear_flag(time_info_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_align(time_info_container, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_size(time_info_container, 60, 20);
    lv_obj_t *hours_info = lv_label_create(time_info_container);
    lv_obj_set_align(hours_info, LV_ALIGN_CENTER);
    lv_obj_set_pos(hours_info, -15, 0);
    lv_obj_set_size(hours_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text_fmt(hours_info, "%02d", hours);
    lv_obj_set_style_text_color(hours_info,  lv_theme_get_color_emphasize(NULL), 0);

    lv_obj_t *second_info = lv_label_create(time_info_container);
    lv_obj_set_align(second_info, LV_ALIGN_CENTER);
    lv_obj_set_pos(second_info, 0, 0);
    lv_obj_set_size(second_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    if (seconds % 2)
        lv_label_set_text_fmt(second_info, ":");
    else
        lv_label_set_text_fmt(second_info, " ");
    lv_obj_set_style_text_color(second_info,  lv_theme_get_color_emphasize(NULL), 0);

    lv_obj_t *min_info = lv_label_create(time_info_container);
    lv_obj_set_align(min_info, LV_ALIGN_CENTER);
    lv_obj_set_pos(min_info, 15, 0);
    lv_obj_set_size(min_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text_fmt(min_info, "%02d", minutes);
    lv_obj_set_style_text_color(min_info,  lv_theme_get_color_emphasize(NULL), 0);

    for (int i = 0; i < tv_tab_num; i++) {
        snprintf(tv_tab_name, sizeof(tv_tab_name), "tv_tab_num_%d", i);
        lv_obj_t *tv_tab = lv_tabview_add_tab(tv_top, tv_tab_name);

        lv_obj_t *img_btn_contain = lv_obj_create(tv_tab);
        lv_obj_clear_flag(img_btn_contain, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_align(img_btn_contain, LV_ALIGN_CENTER);
        lv_obj_set_pos(img_btn_contain, 0, 0);
        lv_obj_set_size(img_btn_contain, LV_HOR_RES * 0.92, LV_VER_RES * 0.82);
        lv_obj_set_style_border_width(img_btn_contain, 0, 0);
        lv_obj_set_style_bg_opa(img_btn_contain, LV_OPA_0, 0);
        lv_obj_set_style_bg_color(img_btn_contain, lv_theme_get_color_primary(NULL), 0);
        lv_obj_set_flex_flow(img_btn_contain, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(img_btn_contain, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN);

        lv_obj_t *footer = footer_ui_create(footer_container);
        if (i == 0) {
            footer_ui_switch(footer, FOOTER_TURN_ON);
        }

        for (int j = 0; j < 8; j++) {
            if (now_icon_num == icon_num) break;
            lv_obj_t *btn = com_imgbtn_comp(img_btn_contain ,icon_list[now_icon_num].icon_path, icon_list[now_icon_num].icon_pre_path);
            if (icon_list[now_icon_num].event_cb != NULL) {
                lv_obj_add_event_cb(btn, icon_list[now_icon_num].event_cb, LV_EVENT_CLICKED, NULL);
            }
            now_icon_num++;
        }
    }

    timer_time = lv_timer_create(time_info_update_cb, 1000, time_info_container);
    timer_sys = lv_timer_create(sys_info_update_cb, 100, disp_sys_info);

#if NOT_DISPLAY_PERFORMANCE
    lv_obj_add_flag(disp_sys_info, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(timer_sys);
#endif

#if NOT_DISPLAY_TIME
    lv_obj_add_flag(time_info_container, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(timer_time);
#endif

    lv_tabview_set_act(tv_top, last_tabview_tab, 0);
    lv_obj_add_event_cb(tv_top, act_sub_tabview, LV_EVENT_ALL, NULL);
}

static void ui_navigator_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *navigator_scr = (lv_obj_t *)lv_event_get_current_target(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_obj_clean(navigator_scr); /* pre-release memory to avoid issues of insufficient memory when loading hte next screen */
        sys_met_hub_delete(sys_info);
        lv_img_cache_set_size(4);
        lv_timer_del(timer_time);
        lv_timer_del(timer_sys);
    }

    if (code == LV_EVENT_DELETE) {;}

    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

lv_obj_t *navigation_ui_init(void)
{
    if(LV_HOR_RES <= 480) disp_size = DISP_SMALL;
    else if(LV_HOR_RES <= 800) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    lv_obj_t *ui_navigator = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_navigator, LV_OBJ_FLAG_SCROLLABLE);
#if defined(AIC_CHIP_D21X)
    lv_img_cache_set_size(20);
#else
    lv_img_cache_set_size(14);
#endif
    navigator_ui_init_impl(ui_navigator); /* capable of adapting to screen size */

    lv_obj_add_event_cb(ui_navigator, ui_navigator_cb, LV_EVENT_ALL, NULL);

    return ui_navigator;
}
