/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "photo_frame_screen.h"

extern lv_obj_t *photo_frame_main;
extern lv_obj_t *photo_frame_photo;
extern lv_obj_t *photo_frame_setting;
extern lv_obj_t *photo_frame_calendar;
extern lv_obj_t *photo_frame_video;
extern lv_obj_t *photo_frame_magic;
extern lv_timer_t *auto_play_timer;

void photo_frame_screen_deinit(void)
{
    photo_frame_main = NULL;
    photo_frame_photo = NULL;
    photo_frame_setting = NULL;
    photo_frame_calendar = NULL;
    photo_frame_video = NULL;
    photo_frame_magic = NULL;
    auto_play_timer = NULL;
}
