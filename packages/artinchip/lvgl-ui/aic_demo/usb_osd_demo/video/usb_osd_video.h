/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#ifndef LV_DEMO_USB_VIDEO_H
#define LV_DEMO_USB_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

#if defined(LV_USB_OSD_PLAY_VIDEO)

lv_obj_t * lv_video_screen_create(void);

void lv_load_video_screen(void);

void lv_video_play(void);

void lv_video_stop(void);

void lv_video_screen_switched(bool e);

bool lv_is_video_screen_switched(void);

#else
static inline lv_obj_t * lv_video_screen_create(void)
{
    return lv_obj_create(NULL);
}

static inline void lv_video_play(void)
{

}

static inline void lv_video_stop(void)
{

}

static inline void lv_load_video_screen(void)
{

}

static inline void lv_video_screen_switched(bool e)
{

}

static inline bool lv_is_video_screen_switched(void)
{
    return false;
}

#endif /* LV_USB_OSD_PLAY_VIDEO */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_USB_VIDEO_H*/
