/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
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
#include "mpp_fb.h"
#include "settings/usb_osd_settings.h"

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
#define USB_OSD_UI_LOGO_IMAGE "logo/"LV_USB_OSD_LOGO_IMAGE
#define USB_OSD_UI_LOGO_ROTATE_IMAGE "logo_rotate/"LV_USB_OSD_LOGO_IMAGE
#endif

#ifdef LV_USB_OSD_LOGO_VIDEO
#define USB_OSD_UI_LOGO_VIDEO_PATH  "/data/lvgl_data/"
#define USB_OSD_UI_LOGO_VIDEO       LV_USB_OSD_LOGO_VIDEO

#define USB_OSD_UI_LOGO_IMAGE        "logo/logo.png"
#define USB_OSD_UI_LOGO_ROTATE_IMAGE "logo_rotate/logo.png"
#endif

bool lv_is_usb_disp_suspend(void);

#if !defined(LV_USB_OSD_PLAY_VIDEO)
#define DISPLAY_VIDEO       (-1)
#endif

typedef enum {
    STATE_USB_SUSPEND = (0x1 << 0),
    STATE_USB_RESUME  = (0x1 << 1),
    STATE_LOGO        = (0x1 << 2),
    STATE_PICTURES    = (0x1 << 3),
    STATE_VIDEO       = (0x1 << 4),
    STATE_SETTINGS    = (0x1 << 5),
    STATE_BLANK       = (0x1 << 6),

    STATE_NUM,

} osd_state_t;


typedef struct osd_manager lv_osd_manager_t;
typedef void (*state_handler)(lv_osd_manager_t*);

struct state_info {
    osd_state_t osd_state;
    void (*handler)(lv_osd_manager_t* mgr);
};

#define VIDEO_SCREEN 0
#define SETTINGS_SCREEN 1

#define SCREEN_NUM  2

struct osd_manager {
    osd_state_t current_state;
    osd_state_t new_state;

    state_handler handlers[STATE_NUM];
    lv_obj_t * screens[SCREEN_NUM];

    struct mpp_fb *fb;
    rt_sem_t osd_sem;

    unsigned int current_alpha_mode;
    unsigned int current_alpha_value;

    unsigned int adjusted_alpha_mode;
    unsigned int adjusted_alpha_value;

    bool usb_suspend;
    bool screen_blank;

    unsigned int flags;
};

bool lv_get_ui_settings_state(void);

void lv_update_ui_settings_state(void);

void lv_blank_screen_enable(bool enable);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_USB_OSD_H*/
