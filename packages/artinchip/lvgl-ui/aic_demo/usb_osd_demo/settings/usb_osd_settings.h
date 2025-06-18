/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
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

#define USB_OSD_CONFIG_FILE      "/data/lvgl_data/osd.config"

struct osd_property {
    unsigned int value;
    cJSON *cjson;
};

struct osd_settings_context {
    cJSON *root;

    struct osd_property lock_mode;
    struct osd_property lock_time;
    struct osd_property blank_time;

    struct osd_property rotation;
    struct osd_property pwm;

    struct osd_property brightness;
    struct osd_property contrast;
    struct osd_property saturation;
    struct osd_property hue;

};

struct slider_obj {
    lv_obj_t * base;
    lv_obj_t * label;
};

struct osd_settings_manager {
    struct osd_settings_context ctx;
    unsigned int long_press_end;
    struct mpp_fb *fb;

    lv_obj_t * screen;

    struct {
        lv_obj_t * background;
        lv_obj_t * logo;
        lv_obj_t * pictures;
    } images;

    struct slider_obj brightness;
    struct slider_obj contrast;
    struct slider_obj saturation;
    struct slider_obj hue;

    struct slider_obj backlight;

    struct {
        unsigned int image_index;
        unsigned int image_count;
    } pic;

    struct {
        lv_obj_t * base;
        lv_style_t normal_style;
        lv_style_t rotate_style;
        unsigned int draw_count;
        unsigned int hide_time_count;
    } menu;

    struct {
        unsigned int new_mode;
        unsigned int old_mode;
        unsigned int lock_time_ms;

        unsigned int blank_time_ms;
        lv_obj_t * blank_option;
    } lock;
};

typedef struct osd_settings_manager lv_settings_mgr_t;
typedef struct osd_settings_context lv_settings_ctx_t;

lv_obj_t * lv_settings_screen_create(void);

void lv_settings_menu_init(lv_obj_t * parent, struct osd_settings_manager *mgr);

void lv_load_settings_screen(void);

int lv_parse_config_file(char *config_file, struct osd_settings_context *settings_cxt);

/* settings menu callback */
unsigned int lv_get_screen_lock_mode(void);

unsigned int lv_get_screen_lock_time_ms(void);

unsigned int lv_get_screen_blank_time_ms(void);

void lv_settings_disp_logo(void);

bool lv_get_logo_state(void);

void lv_settings_disp_pic(void);

void lv_settings_changed_state(bool usb_suepend);

/* event handle */
void screen_lock_mode_dropdown_event_handler(lv_event_t * e);

void screen_lock_delay_dropdown_event_handler(lv_event_t * e);

void screen_blank_delay_dropdown_event_handler(lv_event_t * e);

void screen_rotate_dropdown_event_handler(lv_event_t * e);

void menu_hide_callback(lv_timer_t *tmr);

void menu_event_handler(lv_event_t * e);

void recovery_btn_event_handler(lv_event_t * e);

void settings_screen_event_cb(lv_event_t * e);

void backlight_pwm_config(unsigned int channel, unsigned int level);

void backlight_slider_event_cb(lv_event_t * e);

void slider_event_cb(lv_event_t * e);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_DEMO_USB_OSD_SETTINGS_H */
