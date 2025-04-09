/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#ifndef LV_DEMO_USB_OSD_SETTINGS_H
#define LV_DEMO_USB_OSD_SETTINGS_H

#include "lvgl.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

struct osd_property {
    unsigned int value;
    cJSON *cjson;
};

struct osd_settings_context {
    cJSON *root;

    struct osd_property lock_mode;
    struct osd_property lock_time;
    struct osd_property blank_time;

    struct osd_property pwm;
    struct osd_property brightness;
    struct osd_property contrast;
    struct osd_property saturation;
    struct osd_property hue;
};

int lv_parse_config_file(char *config_file, struct osd_settings_context *settings_cxt);

lv_obj_t * lv_settings_screen_creat(void);

bool lv_settings_menu_is_enabled(void);
void lv_settings_menu_enable(bool enable);

bool lv_pictures_screen_is_enabled(void);
void lv_pictures_screen_enable(bool enable);

bool lv_logo_screen_is_enabled(void);
void lv_logo_screen_enable(bool enable);

int lv_get_screen_lock_time_ms(void);
int lv_get_screen_lock_mode(void);
int lv_get_screen_blank_tmie_ms(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_DEMO_USB_OSD_SETTINGS_H */
