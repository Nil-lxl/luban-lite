/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
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
lv_obj_t * lv_video_screen_creat(void);

void lv_video_play(void);

void lv_video_stop(void);

void video_screen_switched(bool e);

bool is_video_screen_switched(void);
#else
static inline lv_obj_t * lv_video_screen_creat(void)
{
    return lv_obj_create(NULL);
}

static inline void lv_video_play(void)
{

}

static inline void lv_video_stop(void)
{

}

static inline void video_screen_switched(bool e)
{

}

static inline bool is_video_screen_switched(void)
{
    return false;
}

#endif /* LV_USB_OSD_PLAY_VIDEO */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_USB_VIDEO_H*/
