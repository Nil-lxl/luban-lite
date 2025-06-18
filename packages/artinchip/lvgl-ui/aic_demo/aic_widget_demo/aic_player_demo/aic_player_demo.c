/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "aic_ui.h"
#include "lvgl.h"
#include "lv_aic_player.h"

#define CHECK_CONDITION_RETURN_VOID(confition, ...) \
    if (confition)  { __VA_ARGS__; return; }

static bool del_player = false;

static void print_del(void)
{
    printf("lv_aic_player have been deleted\n");
}

static void scale_player_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player = lv_event_get_user_data(e);
    lv_obj_t * slider = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        int32_t value = lv_slider_get_value(slider);
        lv_aic_player_set_scale(player, value);
    }
}

static void rotate_player_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player = lv_event_get_user_data(e);
    lv_obj_t * slider = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        int32_t value = lv_slider_get_value(slider);
        lv_aic_player_set_rotation(player, value);
    }
}

static void start_button_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player = lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
    }
}

static void pause_button_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player= lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_PAUSE, NULL);
    }
}

static void resume_button_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player= lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_RESUME, NULL);
    }
}

static void seek_button_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player= lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        u64 seek = 0;
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_SET_PLAY_TIME, &seek);
    }
}

static void auto_restart_button_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player = lv_event_get_user_data(e);
    lv_obj_t * btn = lv_event_get_current_target(e);

    if (code == LV_EVENT_CLICKED) {
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
        lv_aic_player_set_auto_restart(player, true);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
    }
}

static void del_button_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * player= lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_del(player);
        del_player = true;
        CHECK_CONDITION_RETURN_VOID(del_player, print_del());
    }
}

static void move_player_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    /* LV_OBJ_FLAG_USER_4 flag is used to record whether there is movement and the click event will not be triggered if there is movement */
    if (LV_EVENT_PRESSED == code)
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_USER_4);

    if (code == LV_EVENT_PRESSING) {
        if (lv_obj_has_flag(obj, LV_OBJ_FLAG_USER_4) == false)  lv_obj_set_style_opa(obj, LV_OPA_100, 0);
        lv_indev_t *indev = lv_indev_get_act();
        if (indev == NULL) return;

        lv_indev_type_t indev_type = lv_indev_get_type(indev);
        if (indev_type != LV_INDEV_TYPE_POINTER) return;

        lv_point_t vect;
        lv_indev_get_vect(indev, &vect);
        lv_coord_t x = lv_obj_get_x_aligned(obj) + vect.x;
        lv_coord_t y = lv_obj_get_y_aligned(obj) + vect.y;

        /* after moving it will not trigger click */
        lv_obj_add_flag(obj, LV_OBJ_FLAG_USER_4);

        lv_obj_set_pos(obj, x, y);
    }
}

static lv_obj_t *common_btn(lv_obj_t *parent, char *desc, lv_event_cb_t event_cb, void *event_user_data)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, desc);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, event_user_data);
    return btn;
}

void aic_player_demo()
{
    lv_obj_t *player = lv_aic_player_create(lv_scr_act());
    lv_aic_player_set_src(player, LVGL_PATH(elevator_mjpg.mp4));
    lv_obj_center(player);

    lv_obj_set_style_bg_color(player, lv_color_hex(0x0), 0);
    lv_obj_add_flag(player, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(player, move_player_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *scale_slider = lv_slider_create(lv_scr_act());
    lv_slider_set_range(scale_slider, 64, 512);
    lv_slider_set_value(scale_slider, 256, LV_ANIM_OFF);
    lv_obj_set_size(scale_slider, LV_HOR_RES * 0.03, LV_VER_RES * 0.8);
    lv_obj_align(scale_slider, LV_ALIGN_RIGHT_MID, -LV_HOR_RES * 0.02, 0);
    lv_obj_add_event_cb(scale_slider, scale_player_cb, LV_EVENT_ALL, player);

    lv_obj_t *rotate_slider = lv_slider_create(lv_scr_act());
    lv_slider_set_range(rotate_slider, 0, 3600);
    lv_slider_set_value(rotate_slider, 0, LV_ANIM_OFF);
    lv_obj_set_size(rotate_slider, LV_HOR_RES * 0.03, LV_VER_RES * 0.8);
    lv_obj_align(rotate_slider, LV_ALIGN_LEFT_MID, LV_HOR_RES * 0.02, 0);
    lv_obj_add_event_cb(rotate_slider, rotate_player_cb, LV_EVENT_ALL, player);

    lv_obj_t *btn_container = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(btn_container, LV_OPA_0, 0);
    lv_obj_set_scroll_dir(btn_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(btn_container, LV_SCROLL_SNAP_START);
    lv_obj_set_size(btn_container, LV_HOR_RES * 0.8, LV_VER_RES * 0.4);
    lv_obj_set_align(btn_container, LV_ALIGN_TOP_MID);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    common_btn(btn_container, "Start", start_button_handler, player);
    common_btn(btn_container, "Pause", pause_button_handler, player);
    common_btn(btn_container, "Resume", resume_button_handler, player);
    common_btn(btn_container, "Seek", seek_button_handler, player);
    common_btn(btn_container, "Auto Restart", auto_restart_button_handler, player);
    common_btn(btn_container, "del", del_button_handler, player);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Video Test");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, -LV_VER_RES * 0.1, 0);
}
