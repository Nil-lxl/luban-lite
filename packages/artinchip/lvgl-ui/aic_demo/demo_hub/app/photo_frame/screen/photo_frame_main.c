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
#include "../../app_entrance.h"

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);

static void main_icon_impl(lv_obj_t *parent);

static void icon_slide_cb(lv_event_t * e);
static void back_navigator_cb(lv_event_t * e);
static void enter_video_page_cb(lv_event_t * e);
static void enter_calendar_page_cb(lv_event_t * e);
static void enter_photo_page_cb(lv_event_t * e);
static void enter_setting_page_cb(lv_event_t * e);
static void enter_magic_page_cb(lv_event_t * e);

lv_obj_t *photo_frame_main;

static lv_coord_t coord_x_list[5];
static lv_obj_t *icon_container;

lv_obj_t *photo_frame_main_create(lv_obj_t *parent)
{
    lv_img_cache_set_size(9);

    photo_frame_main = lv_img_create(parent);
    lv_img_set_src(photo_frame_main, LVGL_PATH(photo_frame/bg.jpg));
    lv_obj_add_flag(photo_frame_main, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(photo_frame_main, LV_OBJ_FLAG_SCROLLABLE);

    main_icon_impl(photo_frame_main);

    lv_obj_t *title = lv_img_create(photo_frame_main);
    lv_img_set_src(title, LVGL_PATH(photo_frame/main_page_title.png));
    lv_obj_set_pos(title, adaptive_width(0), adaptive_height(76));
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);

    lv_obj_t *sign = lv_img_create(photo_frame_main);
    lv_img_set_src(sign, LVGL_PATH(photo_frame/main_page_sign.png));
    lv_obj_set_pos(sign, adaptive_width(0), adaptive_height(-10));
    lv_obj_set_align(sign, LV_ALIGN_BOTTOM_RIGHT);

    lv_obj_t *home_btn = com_imgbtn_comp(photo_frame_main, LVGL_PATH(photo_frame/home.png), NULL);
    lv_obj_set_pos(home_btn, adaptive_width(30), adaptive_height(26));
    lv_obj_set_align(home_btn, LV_ALIGN_TOP_LEFT);

    lv_obj_add_event_cb(home_btn, back_navigator_cb, LV_EVENT_ALL, NULL);

    return photo_frame_main;
}

static void coord_x_list_init(void)
{
    coord_x_list[0] = -adaptive_width(700);
    coord_x_list[1] = -adaptive_width(350);
    coord_x_list[2] = adaptive_width(0);
    coord_x_list[3] = adaptive_width(350);
    coord_x_list[4] = adaptive_width(700);
}

static int16_t coordx_to_zoom(lv_coord_t coord_x)
{
    return 256 - LV_ABS(coord_x) / 5;
}

static lv_obj_t *main_icon_create(lv_obj_t *parent, char *src, int16_t x)
{
    lv_obj_t *icon = com_imgbtn_comp(parent, src, NULL);
    lv_obj_set_align(icon, LV_ALIGN_CENTER);
    lv_obj_set_pos(icon, x, adaptive_height(50));
    lv_img_set_zoom(icon, coordx_to_zoom(x));
    return icon;
}

static void anim_move_x_and_zoom_cb(void * var, int32_t v)
{
    lv_obj_set_pos(var, v, adaptive_height(50));
    lv_img_set_zoom(var, coordx_to_zoom(v));
}

static void move_icon_to_track_anim(lv_anim_t *anim, lv_obj_t *img_obj, lv_anim_path_cb_t path_cb,
                       int32_t start_value, int32_t end_value, uint32_t duration)
{
    lv_anim_init(anim);
    lv_anim_set_var(anim, img_obj);
    lv_anim_set_values(anim, start_value, end_value);
    lv_anim_set_time(anim, duration);
    lv_anim_set_exec_cb(anim, anim_move_x_and_zoom_cb);
    lv_anim_set_path_cb(anim, path_cb);
}

static void move_icon_to_track_anim_start(lv_anim_t *anim)
{
    lv_anim_start(anim);
}

static void move_icon_to_track(lv_obj_t *icon_container)
{
    static lv_anim_t move_icon_anim[6];
    int16_t child_cnt = lv_obj_get_child_cnt(icon_container);
    int16_t min_diff = 10000, diff = 0, obj_offset = 0;
    for (int16_t i = 0; i < child_cnt; i++) {
        lv_obj_t *icon_child = lv_obj_get_child(icon_container, i);
        lv_coord_t x = lv_obj_get_x_aligned(icon_child);

        /* Find the offset of the object with the smallest distance from the center */
        diff = LV_ABS(coord_x_list[2] - x);
        if (diff < min_diff) {
            min_diff = diff;
            obj_offset = coord_x_list[2] - x;
        }
    }

    for (int16_t i = 0; i < child_cnt; i++) {
        lv_obj_t *icon_child = lv_obj_get_child(icon_container, i);
        lv_coord_t x = lv_obj_get_x_aligned(icon_child);
        move_icon_to_track_anim(&move_icon_anim[i], icon_child, lv_anim_path_linear, x, x + obj_offset, 200);
        move_icon_to_track_anim_start(&move_icon_anim[i]);
    }
}

static void main_icon_impl(lv_obj_t *parent)
{
    coord_x_list_init();

    icon_container = lv_obj_create(parent);
    lv_obj_remove_style_all(icon_container);
    lv_obj_set_size(icon_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_flag(icon_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(icon_container, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *video_icon = main_icon_create(icon_container, LVGL_PATH(photo_frame/video_icon.png), coord_x_list[0]);
    lv_obj_t *calendar_icon = main_icon_create(icon_container, LVGL_PATH(photo_frame/calendar_icon.png), coord_x_list[1]);
    lv_obj_t *photo_icon = main_icon_create(icon_container, LVGL_PATH(photo_frame/photo_icon.png), coord_x_list[2]);
    lv_obj_t *setting_icon = main_icon_create(icon_container, LVGL_PATH(photo_frame/setting_icon.png), coord_x_list[3]);
    lv_obj_t *magic_icon = main_icon_create(icon_container, LVGL_PATH(photo_frame/magic_icon.png), coord_x_list[4]);

    lv_obj_add_event_cb(icon_container, icon_slide_cb, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(video_icon, enter_video_page_cb, LV_EVENT_ALL, parent);
    lv_obj_add_event_cb(calendar_icon, enter_calendar_page_cb, LV_EVENT_ALL, parent);
    lv_obj_add_event_cb(photo_icon, enter_photo_page_cb, LV_EVENT_ALL, parent);
    lv_obj_add_event_cb(setting_icon, enter_setting_page_cb, LV_EVENT_ALL, parent);
    lv_obj_add_event_cb(magic_icon, enter_magic_page_cb, LV_EVENT_ALL, parent);
}

static void icon_slide_cb(lv_event_t * e)
{
    static  lv_point_t click_point_1;
    static  lv_point_t click_point_2;

    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    int16_t child_cnt = lv_obj_get_child_cnt(icon_container);

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(lv_indev_get_act(), &click_point_1);
        return;
    } else if (code == LV_EVENT_PRESSING) {
        lv_indev_get_point(lv_indev_get_act(), &click_point_2);

        int16_t offset = click_point_2.x - click_point_1.x;

        for (int16_t i = 0; i < child_cnt; i++) {
            lv_obj_t *icon_child = lv_obj_get_child(icon_container, i);
            lv_coord_t x = lv_obj_get_x_aligned(icon_child);

            x += offset;
            lv_obj_set_pos(icon_child, x, adaptive_height(50));
            lv_img_set_zoom(icon_child, coordx_to_zoom(x));
        }

        click_point_1.x = click_point_2.x;
    } else if (code == LV_EVENT_RELEASED) {
        move_icon_to_track(icon_container);
    };
}

static void back_navigator_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        app_entrance(APP_NAVIGATION, 1);
        if (auto_play_timer)
            lv_timer_del(auto_play_timer);
    }
}

static void enter_video_page_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *parent = lv_event_get_user_data(e);

    icon_slide_cb(e);

    if (code == LV_EVENT_SHORT_CLICKED) {
        lv_obj_add_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        if (!photo_frame_video)
            photo_frame_video_create(lv_obj_get_parent(parent));
        else
            lv_obj_clear_flag(photo_frame_video, LV_OBJ_FLAG_HIDDEN);
    }
}

static void enter_calendar_page_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *parent = lv_event_get_user_data(e);

    icon_slide_cb(e);

    if (code == LV_EVENT_SHORT_CLICKED) {
        lv_obj_add_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        if (!photo_frame_calendar)
            photo_frame_calendar_create(lv_obj_get_parent(parent));
        else
            lv_obj_clear_flag(photo_frame_calendar, LV_OBJ_FLAG_HIDDEN);
    }

}

static void enter_photo_page_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *parent = lv_event_get_user_data(e);

    icon_slide_cb(e);

    if (code == LV_EVENT_SHORT_CLICKED) {
#if OPTIMIZE
        lv_img_cache_set_size(4);
#else
        lv_img_cache_set_size(3);
#endif
        lv_obj_add_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        if (!photo_frame_photo) {
            photo_frame_photo_create(lv_obj_get_parent(parent));
        } else {
            lv_timer_resume(auto_play_timer);
            lv_obj_clear_flag(photo_frame_photo, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void enter_setting_page_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *parent = lv_event_get_user_data(e);

    icon_slide_cb(e);

    if (code == LV_EVENT_SHORT_CLICKED) {
        lv_img_cache_invalidate_src(NULL);
        lv_obj_add_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        if (!photo_frame_setting)
            photo_frame_setting_create(lv_obj_get_parent(parent));
        else
            lv_obj_clear_flag(photo_frame_setting, LV_OBJ_FLAG_HIDDEN);
    }
}

static void enter_magic_page_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *parent = lv_event_get_user_data(e);

    icon_slide_cb(e);

    if (code == LV_EVENT_SHORT_CLICKED) {
        lv_obj_add_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        if (!photo_frame_magic)
            photo_frame_magic_create(lv_obj_get_parent(parent));
        else
            lv_obj_clear_flag(photo_frame_magic, LV_OBJ_FLAG_HIDDEN);
    }
}
