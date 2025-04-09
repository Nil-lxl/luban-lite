/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "../photo_frame.h"
#include "../anmi/photo_frame_anim.h"

extern lv_obj_t *photo_frame_main;
extern lv_obj_t *photo_frame_photo;
extern lv_obj_t *photo_frame_setting;
extern lv_obj_t *photo_frame_calendar;
extern lv_obj_t *photo_frame_video;
extern lv_obj_t *photo_frame_magic;
extern lv_timer_t *auto_play_timer;

void photo_frame_screen_deinit(void);
lv_obj_t *photo_frame_main_create(lv_obj_t *parent);
lv_obj_t *photo_frame_photo_create(lv_obj_t *parent);
lv_obj_t *photo_frame_setting_create(lv_obj_t *parent);
lv_obj_t *photo_frame_calendar_create(lv_obj_t *parent);
lv_obj_t *photo_frame_video_create(lv_obj_t *parent);
lv_obj_t *photo_frame_magic_create(lv_obj_t *parent);
