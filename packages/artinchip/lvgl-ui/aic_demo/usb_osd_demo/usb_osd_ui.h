/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#ifndef LV_DEMO_USB_OSD_H
#define LV_DEMO_USB_OSD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

enum screen_lock_delay {
    SECONDS_15_LOCK,
    SECONDS_30_LOCK,

    MINUTES_1_LOCK,
    MINUTES_2_LOCK,
    MINUTES_5_LOCK,
    MINUTES_10_LOCK,

    SCREEN_NEVER_LOCK,
};

enum screen_blank_delay {
    MINUTES_5_BLANK,
    MINUTES_10_BLANK,
    MINUTES_15_BLANK,
    MINUTES_30_BLANK,

    SCREEN_NEVER_BLANK,
};

enum screen_lock_mode {
    DISPLAY_LOGO,
    DISPLAY_PICTURES,
#ifdef LV_USB_OSD_PLAY_VIDEO
    DISPLAY_VIDEO,
#endif
    BLANK_SCREEN,

    NEVER_LOCK_MODE,
};

#define USB_OSD_WAKEUP_KEY    LV_USB_OSD_SETTINGS_WAKEUP_KEY

#ifdef LV_USB_OSD_LOGO_IMAGE
#define USB_OSD_UI_LOGO_IMAGE LV_USB_OSD_LOGO_IMAGE
#endif

#ifdef LV_USB_OSD_LOGO_VIDEO
#define USB_OSD_UI_LOGO_VIDEO_PATH  "/data/lvgl_data/"
#define USB_OSD_UI_LOGO_VIDEO       LV_USB_OSD_LOGO_VIDEO
#if (AIC_FB_ROTATE_DEGREE == 0)
#define USB_OSD_UI_LOGO_VIDEO_ROTATE    0
#else
#define USB_OSD_UI_LOGO_VIDEO_ROTATE    AIC_FB_ROTATE_DEGREE
#endif
#endif /* LV_USB_OSD_LOGO_VIDEO */

void usb_osd_ui_init(void);
bool is_usb_disp_suspend(void);
void usb_osd_modify_blank_status(bool enable);
void back_video_screen(void);
void back_settings_screen(void);

void lv_load_video_screen(void);

#if !defined(LV_USB_OSD_PLAY_VIDEO)
#define DISPLAY_VIDEO       (-1)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_USB_OSD_H*/
