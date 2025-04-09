/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include <stdlib.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "photo_frame_screen.h"

#define ZOOM_ERROR 2
#define PHOTO_ANIM_TIME 1000

#if LVGL_VERSION_MAJOR == 8
#define lv_img_header_t         lv_img_header_t
#else
#define lv_img_header_t         lv_image_header_t
#endif

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);

static void *get_now_img_list_src(void);
static void *get_next_img_list_src(void);
static void *get_last_img_list_src(void);

static int16_t calculate_zoom_ratio(lv_obj_t *img_obj, photo_full_mode mode);
static int check_img_bg_color_fill(lv_obj_t *img_container);

static void touch_cb(lv_event_t * e);
static void back_main_cb(lv_event_t * e);
static void auto_swith_picture_cb(lv_timer_t *timer);

lv_obj_t *photo_frame_photo;

static lv_obj_t *photo_frame_photo_2;
static photo_switch_anim_t swith_anim;
static base_anim_t home_icon_anim;
lv_timer_t *auto_play_timer;

lv_obj_t *photo_frame_photo_create(lv_obj_t *parent)
{
    photo_switch_anim_init(&swith_anim);
    base_anim_init(&home_icon_anim);

    photo_frame_photo = lv_obj_create(parent);
    lv_obj_remove_style_all(photo_frame_photo);
    lv_obj_set_size(photo_frame_photo, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(photo_frame_photo, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *photo_frame_photo_1 = lv_obj_create(photo_frame_photo);
    lv_obj_remove_style_all(photo_frame_photo_1);
    lv_obj_set_size(photo_frame_photo_1, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(photo_frame_photo_1, lv_color_hex(g_setting.bg_color), 0);
    lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_100, 0);
    lv_obj_clear_flag(photo_frame_photo_1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *img = lv_img_create(photo_frame_photo_1);
    lv_img_set_src(img, get_now_img_list_src());
    lv_obj_center(img);
    lv_img_set_zoom(img, calculate_zoom_ratio(img, g_setting.cur_photo_full_mode));
    lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);

    photo_frame_photo_2 = lv_obj_create(photo_frame_photo);
    lv_obj_remove_style_all(photo_frame_photo_2);
    lv_obj_set_size(photo_frame_photo_2, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(photo_frame_photo_2, lv_color_hex(g_setting.bg_color), 0);
    lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
    lv_obj_clear_flag(photo_frame_photo_2, LV_OBJ_FLAG_SCROLLABLE);
    img = lv_img_create(photo_frame_photo_2);
    lv_img_set_src(img, get_now_img_list_src());
    lv_obj_center(img);
    lv_img_set_zoom(img, calculate_zoom_ratio(img, g_setting.cur_photo_full_mode));

    lv_obj_t *photo_frame_touch = lv_obj_create(photo_frame_photo);
    lv_obj_remove_style_all(photo_frame_touch);
    lv_obj_set_size(photo_frame_touch, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_flag(photo_frame_touch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(photo_frame_touch, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *home_btn = com_imgbtn_comp(photo_frame_photo, LVGL_PATH(photo_frame/home.png), NULL);
    lv_obj_set_pos(home_btn, adaptive_width(30), adaptive_height(26));
    lv_obj_set_align(home_btn, LV_ALIGN_TOP_LEFT);

    auto_play_timer = lv_timer_create(auto_swith_picture_cb, PHOTO_ANIM_TIME * 3, NULL);

    lv_obj_add_event_cb(photo_frame_touch, touch_cb, LV_EVENT_ALL, home_btn);
    lv_obj_add_event_cb(home_btn, back_main_cb, LV_EVENT_ALL, NULL);

    return photo_frame_photo;
}

static void *get_now_img_list_src(void)
{
    return g_list->img_src[g_list->img_pos];
}

static void *get_last_img_list_src(void)
{
    g_list->img_pos--;
    g_list->img_pos = g_list->img_pos < 0 ? g_list->img_num - 1 : g_list->img_pos;

    return g_list->img_src[g_list->img_pos];
}

static void *get_next_img_list_src(void)
{
    g_list->img_pos++;
    g_list->img_pos = g_list->img_pos >= g_list->img_num ? 0 : g_list->img_pos;

    return g_list->img_src[g_list->img_pos];
}

static int16_t calc_zoom_based_on_dim(uint16_t dimension, uint16_t max_res, photo_full_mode mode)
{
    int16_t zoom = 256;

    switch (mode) {
        case FULL_MODE_NONRMAL:
            zoom = ((float)max_res / (float)dimension) * 256;
            break;
        case FULL_MODE_ORIGINAL:
            if (dimension > max_res)
                zoom = ((float)max_res / (float)dimension) * 256;
            else
                zoom = 256;
            break;
        case FULL_MODE_FILL_COVER:
            break;
        default:
            break;
    }

    return zoom;
}

static int16_t calculate_zoom_ratio(lv_obj_t *img_obj, photo_full_mode mode)
{
    lv_img_header_t header;
    int16_t zoom = 256;

    lv_img_decoder_get_info(lv_img_get_src(img_obj), &header);

    if (((float)header.w / (float)LV_HOR_RES) > ((float)header.h / (float)LV_VER_RES)) {
        zoom = calc_zoom_based_on_dim(header.w, LV_HOR_RES, mode);
    } else {
        zoom = calc_zoom_based_on_dim(header.h, LV_VER_RES, mode);
    }

    return zoom;
}

static void switch_img(bool next)
{
#if OPTIMIZE
    /* wait for the animation to end and release memory resources to avoid running out of memory */
    if (lv_anim_count_running() != 0)
        return;

    /* clear the cache of the previous image */
    for (int i = 0; i < g_list->img_num; i++) {
        if (i == g_list->img_pos)
            continue;
#if LVGL_VERSION_MAJOR == 8
        lv_img_cache_invalidate_src(g_list->img_src[i]);
#else
        lv_image_cache_drop(g_list->img_src[i]);
#endif
    }
#endif
    lv_obj_t *photo_frame_photo_1 = lv_obj_get_child(photo_frame_photo, 0);
    lv_obj_t *photo_frame_photo_2 = lv_obj_get_child(photo_frame_photo, 1);

    lv_obj_t *img_photo_frame_photo_1 = lv_obj_get_child(photo_frame_photo_1, 0);
    lv_obj_t *img_photo_frame_photo_2 = lv_obj_get_child(photo_frame_photo_2, 0);

    lv_img_set_src(img_photo_frame_photo_1, get_now_img_list_src());
    lv_img_set_zoom(img_photo_frame_photo_1, calculate_zoom_ratio(img_photo_frame_photo_1, g_setting.cur_photo_full_mode));
    lv_img_set_angle(img_photo_frame_photo_1, 0);
    /* pos and opa reset */
    lv_obj_set_pos(photo_frame_photo_1, 0, 0);
    lv_obj_set_style_opa(photo_frame_photo_1, LV_OPA_100, 0);
    lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_100, 0);
    lv_obj_set_pos(img_photo_frame_photo_1, 0, 0);
    lv_obj_set_style_img_opa(img_photo_frame_photo_1, LV_OPA_100, 0);
    lv_obj_set_style_opa(img_photo_frame_photo_1, LV_OPA_100, 0);

    lv_obj_update_layout(photo_frame_photo_1);

    if (next)
        lv_img_set_src(img_photo_frame_photo_2, get_next_img_list_src());
    else
        lv_img_set_src(img_photo_frame_photo_2, get_last_img_list_src());
    lv_img_set_zoom(img_photo_frame_photo_2, calculate_zoom_ratio(img_photo_frame_photo_2, g_setting.cur_photo_full_mode));
    lv_img_set_angle(img_photo_frame_photo_2, 0);
    /* pos and opa reset */
    lv_obj_set_pos(photo_frame_photo_2, 0, 0);
    lv_obj_set_style_opa(photo_frame_photo_2, LV_OPA_100, 0);
    lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
    lv_obj_set_pos(img_photo_frame_photo_2, 0, 0);
    lv_obj_set_style_img_opa(img_photo_frame_photo_2, LV_OPA_100, 0);
    lv_obj_set_style_opa(img_photo_frame_photo_2, LV_OPA_100, 0);

    lv_obj_update_layout(photo_frame_photo_2);

    int16_t anim_switch = g_setting.cur_anmi;
    if (g_setting.cur_anmi == PHOTO_SWITCH_ANIM_RANDOM) {
        anim_switch = rand() % PHOTO_SWITCH_ANIM_RANDOM;
    }

    switch (anim_switch) {
    case PHOTO_SWITCH_ANIM_NONE:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        if (check_img_bg_color_fill(img_photo_frame_photo_2))
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
        else
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        photo_switch_anim_none_set(&swith_anim, img_photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
        photo_switch_anim_none_start(&swith_anim);
        break;
    case PHOTO_SWITCH_ANIM_FAKE_IN_OUT:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        photo_swith_fade_in_out_set(&swith_anim, img_photo_frame_photo_1, img_photo_frame_photo_2, PHOTO_ANIM_TIME);
        photo_swith_fade_in_out_start(&swith_anim);
        break;
    case PHOTO_SWITCH_ANIM_PUSH:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        if (check_img_bg_color_fill(img_photo_frame_photo_2))
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
        else
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_switch_anim_push_from_bottom_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_push_from_bottom_start(&swith_anim);
        } else {
            photo_switch_anim_push_from_top_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_push_from_top_start(&swith_anim);
        }
        break;
    case PHOTO_SWITCH_ANIM_COVER:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        if (check_img_bg_color_fill(img_photo_frame_photo_2))
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
        else
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_switch_anim_cover_from_right_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_cover_from_right_start(&swith_anim);
        } else {
            photo_switch_anim_cover_from_left_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_cover_from_left_start(&swith_anim);
        }
        break;
    case PHOTO_SWITCH_ANIM_UNCOVER:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        if (check_img_bg_color_fill(img_photo_frame_photo_2))
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
        else
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_switch_anim_uncover_from_right_set(&swith_anim, photo_frame_photo_2, photo_frame_photo_1, PHOTO_ANIM_TIME);
            photo_switch_anim_uncover_from_right_start(&swith_anim);
        } else {
            photo_switch_anim_uncover_from_left_set(&swith_anim, photo_frame_photo_2, photo_frame_photo_1, PHOTO_ANIM_TIME);
            photo_switch_anim_uncover_from_left_start(&swith_anim);
        }
        break;
#if USE_SWITCH_ANIM_ZOOM
    case PHOTO_SWITCH_ANIM_ZOOM:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_swith_zoom_enlarge_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_swith_zoom_enlarge_start(&swith_anim);
        } else {
            photo_swith_zoom_shrink_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_swith_zoom_shrink_start(&swith_anim);
        }
        break;
#endif
    case PHOTO_SWITCH_ANIM_FLY_PASE:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_switch_anim_fly_pase_enlarge_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_fly_pase_enlarge_start(&swith_anim);
        } else {
            photo_switch_anim_fly_pase_shrink_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_fly_pase_shrink_start(&swith_anim);
        }
        break;
#if USE_SWITCH_ANIM_COMB
    case PHOTO_SWITCH_ANIM_COMB:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        if (check_img_bg_color_fill(img_photo_frame_photo_2))
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_100, 0);
        else
            lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        photo_switch_anim_comb_horizontal_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
        photo_switch_anim_comb_horizontal_start(&swith_anim);
        break;
#endif
    case PHOTO_SWITCH_ANIM_FLOAT:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        photo_switch_anim_float_in_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
        photo_switch_anim_float_in_start(&swith_anim);
        break;
#if USE_SWITCH_ANIM_ROTATE
    case PHOTO_SWITCH_ANIM_ROTATE:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_switch_anim_enlarge_rotate_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_enlarge_rotate_start(&swith_anim);
        } else {
            photo_switch_anim_shrink_rotate_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_shrink_rotate_start(&swith_anim);
        }
        break;
    case PHOTO_SWITCH_ANIM_ROTATE_MOVE:
        lv_obj_set_style_bg_opa(photo_frame_photo_1, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(photo_frame_photo_2, LV_OPA_0, 0);
        if (next) {
            photo_switch_anim_rotate_move_right_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_rotate_move_right_start(&swith_anim);
        } else {
            photo_switch_anim_rotate_move_left_set(&swith_anim, photo_frame_photo_1, photo_frame_photo_2, PHOTO_ANIM_TIME);
            photo_switch_anim_rotate_move_left_start(&swith_anim);
        }
        break;
#endif
    default:
        break;
    }
}

static void move_icon_anim_cb(void * var, int32_t v)
{
    lv_obj_set_y(var, v);

    /* setting zoom currently results in an incorrect refresh area for lvgl calculations */
    lv_obj_t *img = lv_obj_get_child(photo_frame_photo_2, 0);
    if (lv_img_get_zoom(img) > 256) {
        lv_obj_invalidate(var);
    }
}
static void move_icon_anim_init(lv_anim_t *anim, lv_obj_t *obj, int16_t start_val, int16_t end_val)
{
    lv_anim_init(anim);
    lv_anim_set_var(anim, obj);
    lv_anim_set_values(anim, start_val, end_val);
    lv_anim_set_time(anim, 500);
    lv_anim_set_exec_cb(anim, move_icon_anim_cb);
    lv_anim_set_path_cb(anim, lv_anim_path_linear);
    lv_anim_start(anim);
}

static void touch_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *home_btn = lv_event_get_user_data(e);
    static lv_anim_t anim;

    if (code == LV_EVENT_GESTURE)
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if(dir == LV_DIR_LEFT) switch_img(true);
        if(dir == LV_DIR_RIGHT) switch_img(false);
        if(dir == LV_DIR_TOP) {
            if (lv_obj_get_y(home_btn) < 0)
                return;
            move_icon_anim_init(&anim, home_btn, 26, -100);
        }
        if(dir == LV_DIR_BOTTOM) {
            if (lv_obj_get_y(home_btn) > 0)
                return;
            move_icon_anim_init(&anim, home_btn, -100, 26);
        }
    }
}

static void back_main_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_clear_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(photo_frame_photo, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(auto_play_timer);
        lv_img_cache_set_size(8);
    }
}

static void auto_swith_picture_cb(lv_timer_t *timer)
{
    if (g_setting.auto_play == false)
        return;
    switch_img(true);
}

static int check_img_bg_color_fill(lv_obj_t *img_container)
{
#if OPTIMIZE
    lv_img_header_t header;
    lv_img_decoder_get_info(lv_img_get_src(img_container), &header);

    int16_t img_actual_height = header.h * lv_img_get_zoom(img_container) / 256.0;
    int16_t img_actual_width = header.w * lv_img_get_zoom(img_container) / 256.0;

    if ((img_actual_height >= (LV_VER_RES - ZOOM_ERROR)) &&
        (img_actual_width >= (LV_HOR_RES -ZOOM_ERROR)))
    return 0;
#else
    return 1;
#endif

    return 1;
}
