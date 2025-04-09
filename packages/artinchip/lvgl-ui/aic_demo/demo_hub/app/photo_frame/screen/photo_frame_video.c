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

extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);

static void back_main_cb(lv_event_t * e);

lv_obj_t *photo_frame_video;

lv_obj_t *photo_frame_video_create(lv_obj_t *parent)
{
    photo_frame_video = lv_img_create(parent);
    lv_img_set_src(photo_frame_video, LVGL_PATH(photo_frame/bg.jpg));
    lv_obj_add_flag(photo_frame_video, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(photo_frame_video, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_img_create(photo_frame_video);
    lv_img_set_src(title, LVGL_PATH(photo_frame/video_page_title.png));
    lv_obj_set_pos(title, adaptive_width(0), adaptive_height(50));
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);

    lv_obj_t *tip = lv_img_create(photo_frame_video);
    lv_img_set_src(tip, LVGL_PATH(photo_frame/video_page_tip.png));
    lv_obj_set_align(tip, LV_ALIGN_CENTER);

    lv_obj_t *home_btn = com_imgbtn_comp(photo_frame_video, LVGL_PATH(photo_frame/home.png), NULL);
    lv_obj_set_pos(home_btn, adaptive_width(30), adaptive_height(26));
    lv_obj_set_align(home_btn, LV_ALIGN_TOP_LEFT);

    lv_obj_add_event_cb(home_btn, back_main_cb, LV_EVENT_ALL, NULL);

    return photo_frame_video;
}

static void back_main_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        lv_obj_clear_flag(photo_frame_main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(photo_frame_video, LV_OBJ_FLAG_HIDDEN);
    }
}
