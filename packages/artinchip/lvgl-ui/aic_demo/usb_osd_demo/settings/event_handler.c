/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include "../video/usb_osd_video.h"
#include "usb_osd_settings.h"
#include "../usb_osd_ui.h"
#include "usbd_display.h"
#include "lv_tpc_run.h"

#include <msh.h>

static void lv_set_ui_alpha(lv_settings_mgr_t *mgr, unsigned int mode, unsigned int value);

static inline bool lv_pictures_is_enabled(lv_settings_mgr_t *mgr)
{
    return !lv_obj_has_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);
}

static inline void lv_pictures_enable(lv_settings_mgr_t *mgr, bool enable)
{
    if (enable)
        lv_obj_clear_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);
}

static inline bool lv_logo_is_enabled(lv_settings_mgr_t *mgr)
{
    return !lv_obj_has_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
}

static inline void lv_logo_enable(lv_settings_mgr_t *mgr, bool enable)
{
    if (enable)
        lv_obj_clear_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
}

static inline bool lv_settings_menu_is_enabled(lv_settings_mgr_t *mgr)
{
    return !lv_obj_has_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);
}

static inline void lv_settings_menu_enable(lv_settings_mgr_t *mgr, bool enable)
{
    if (enable)
        lv_obj_clear_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);
}

static inline void lv_display_logo(lv_settings_mgr_t *mgr)
{
    lv_logo_enable(mgr, true);
    lv_pictures_enable(mgr, false);
    lv_set_ui_alpha(mgr, AICFB_GLOBAL_ALPHA_MODE, 255);
}

static inline void lv_display_pictures(lv_settings_mgr_t *mgr)
{
    lv_logo_enable(mgr, false);
    lv_pictures_enable(mgr, true);
    lv_set_ui_alpha(mgr, AICFB_GLOBAL_ALPHA_MODE, 255);
}

static inline void lv_display_blank(lv_settings_mgr_t *mgr)
{
    lv_pictures_enable(mgr, false);
    lv_logo_enable(mgr, false);

    lv_blank_screen_enable(true);
}

static void lv_set_ui_alpha(lv_settings_mgr_t *mgr, unsigned int mode, unsigned int value)
{
    struct aicfb_alpha_config alpha = {0};
    int ret;

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = mode;
    alpha.value = value;
    ret = mpp_fb_ioctl(mgr->fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        printf("set alpha failed\n");
}

static void lv_pictures_switch(lv_settings_mgr_t *mgr, lv_dir_t dir)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);
    unsigned int index = mgr->pic.image_index;
    unsigned int count = mgr->pic.image_count;
    char image_str[64];

    if (!lv_pictures_is_enabled(mgr))
        return;

    if(dir == LV_DIR_LEFT)
        index = (index == 0) ? (count - 1) : (index - 1);

    if(dir == LV_DIR_RIGHT)
        index = (index == count - 1) ? (0) : (index + 1);

    if (index >= count)
        index = 0;

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        snprintf(image_str, sizeof(image_str), LVGL_PATH(pic_rotate/image%d.jpg), index);
    else
        snprintf(image_str, sizeof(image_str), LVGL_PATH(pic_normal/image%d.jpg), index);

    mgr->pic.image_index = index;
    lv_img_set_src(mgr->images.pictures, image_str);
}

#ifdef LV_USB_OSD_SETTINGS_MENU
static void lv_video_start(lv_settings_mgr_t *mgr)
{
    lv_video_play();
    lv_set_ui_alpha(mgr, AICFB_PIXEL_ALPHA_MODE, 0);
}

static void lv_settings_long_pressed_handle(lv_settings_mgr_t *mgr)
{
    if (!lv_settings_menu_is_enabled(mgr)) {
        lv_settings_menu_enable(mgr, true);

        mgr->lock.old_mode = mgr->lock.new_mode;
        /*
        * Normally, a LV_EVENT_CLICKED event will be generated after the
        * LV_EVENT_LONG_PRESSED event ends, and we need to filter it out.
        */
        mgr->long_press_end = 1;
    }
}

static void lv_save_config_file(lv_settings_mgr_t *mgr)
{
    struct osd_settings_context *ctx = &mgr->ctx;
    char *json_string = NULL;
    FILE *file = NULL;

    file = fopen(USB_OSD_CONFIG_FILE, "wb");
    if (file == NULL) {
        pr_err("fopen %s failed, when save file\n", USB_OSD_CONFIG_FILE);
        return;
    }

    json_string = cJSON_Print(ctx->root);
    if (json_string != NULL)
        fwrite(json_string, strlen(json_string), 1, file);
    else
        pr_err("failed to print cjson when save osd config\n");

    fclose(file);
}


static void lv_settings_hide_menu(lv_settings_mgr_t *mgr)
{
    /* Filter out a LV_EVENT_CLICKED event */
    if (mgr->long_press_end == 1) {
        mgr->long_press_end = 0;
        return;
    }

    lv_settings_menu_enable(mgr, false);
    lv_update_ui_settings_state();
    lv_save_config_file(mgr);
}

static void lv_settings_switch_lock_mode(lv_settings_mgr_t *mgr)
{
    /* stop play video */
    if (mgr->lock.old_mode == DISPLAY_VIDEO && mgr->lock.new_mode != DISPLAY_VIDEO)
        lv_video_stop();

    /* play video continue */
    if (mgr->lock.old_mode == DISPLAY_VIDEO && mgr->lock.new_mode == DISPLAY_VIDEO)
        return;

    mgr->lock.old_mode = mgr->lock.new_mode;

    switch (mgr->lock.new_mode) {
    case DISPLAY_LOGO:
        lv_display_logo(mgr);
        break;
    case DISPLAY_PICTURES:
        lv_display_pictures(mgr);
        break;
    case BLANK_SCREEN:
        lv_display_blank(mgr);
        break;
    case DISPLAY_VIDEO:
        lv_video_start(mgr);
        break;
    default:
        printf("Invalid lock screen mode\n");
        break;
    }

    lv_update_ui_settings_state();
}
#endif /* LV_USB_OSD_SETTINGS_MENU */

void settings_screen_event_cb(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    /* switch picture by tp gestur event */
    if (code == LV_EVENT_GESTURE)
        lv_pictures_switch(mgr, dir);

#ifdef LV_USB_OSD_SETTINGS_MENU
    /* show settings menu by tp long pressed event */
    if (code == LV_EVENT_LONG_PRESSED) {
        lv_settings_long_pressed_handle(mgr);
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        if (lv_settings_menu_is_enabled(mgr))
            lv_settings_hide_menu(mgr);

        /* switch to video screen to display controls such as video progress bars */
        if(mgr->lock.new_mode == DISPLAY_VIDEO)
            lv_load_video_screen();

        /* usb suspend */
        if (lv_get_ui_settings_state()) {
            lv_settings_switch_lock_mode(mgr);
        } else {
            /* usb resume */
            lv_pictures_enable(mgr, false);
            lv_logo_enable(mgr, false);

            lv_update_ui_settings_state();
            lvgl_put_tp();
        }
    }
#endif /* LV_USB_OSD_SETTINGS_MENU */
}

/*
 * Record settings menu operation event to determine whether to hide
 */
void menu_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    struct osd_settings_manager *mgr = lv_event_get_user_data(e);
    lv_obj_t * draw_menu = lv_event_get_target(e);

    if(code == LV_EVENT_DRAW_MAIN_END && draw_menu == mgr->menu.base)
        mgr->menu.draw_count++;
}

void menu_hide_callback(lv_timer_t *tmr)
{
    struct osd_settings_manager *mgr = tmr->user_data;

    if (mgr->menu.draw_count == 0 || lv_obj_has_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN))
        return;

    /*
     * There has been no menu operation for USB_OSD_MENU_HIDE_TIME_MS.
     * Automatically hide the settings menu.
     */
    if (mgr->menu.draw_count == mgr->menu.hide_time_count) {

        lv_obj_add_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);

#if LVGL_VERSION_MAJOR == 8
        lv_event_send(mgr->screen, LV_EVENT_CLICKED, NULL);
#else
        lv_obj_send_event(mgr->screen, LV_EVENT_CLICKED, NULL);
#endif

        if (!lv_is_usb_disp_suspend())
            lvgl_put_tp();

        mgr->menu.draw_count = 0;
        mgr->menu.hide_time_count = 0;
        return;
    }

    mgr->menu.hide_time_count = mgr->menu.draw_count;
}

void slider_event_cb(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_settings_ctx_t *ctx = &mgr->ctx;
    lv_obj_t * slider = lv_event_get_target(e);
    struct aicfb_disp_prop disp_prop;
    char buf[8];
    u32 origin, value;

    origin = (u32)lv_slider_get_value(slider);
    lv_snprintf(buf, sizeof(buf), "%d", origin);

    if (mpp_fb_ioctl(mgr->fb, AICFB_GET_DISP_PROP, &disp_prop))
        printf("mpp fb ioctl get disp prop failed\n");

    /* limit value in [25, 75] */
    value = (origin >> 1) + 25;

    if (slider == mgr->brightness.base) {
        disp_prop.bright = value;
        lv_label_set_text(mgr->brightness.label, buf);
        cJSON_SetIntValue(ctx->brightness.cjson, origin);
    }

    if (slider == mgr->contrast.base) {
        disp_prop.contrast = value;
        lv_label_set_text(mgr->contrast.label, buf);
        cJSON_SetIntValue(ctx->contrast.cjson, origin);
    }

    if (slider == mgr->saturation.base) {
        disp_prop.saturation = value;
        lv_label_set_text(mgr->saturation.label, buf);
        cJSON_SetIntValue(ctx->saturation.cjson, origin);
    }

    if (slider == mgr->hue.base) {
        disp_prop.hue = value;
        lv_label_set_text(mgr->hue.label, buf);
        cJSON_SetIntValue(ctx->hue.cjson, origin);
    }

    if (mpp_fb_ioctl(mgr->fb, AICFB_SET_DISP_PROP, &disp_prop))
        printf("mpp fb ioctl set disp prop failed\n");
}

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)

void backlight_pwm_config(u32 channel, u32 level)
{
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    /* pwm frequency: 1KHz = 1000000ns */
    rt_pwm_set(pwm_dev, channel, 1000000, 10000 * level);
}

void backlight_slider_event_cb(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_settings_ctx_t *ctx = &mgr->ctx;
    lv_obj_t * slider = lv_event_get_target(e);

    char buf[8];
    u32 value;

    value = (u32)lv_slider_get_value(slider);
    lv_snprintf(buf, sizeof(buf), "%d", value);

    lv_label_set_text(mgr->backlight.label, buf);
    cJSON_SetIntValue(ctx->pwm.cjson, value);

    backlight_pwm_config(AIC_PWM_BACKLIGHT_CHANNEL, value);
}
#endif

static void disp_prop_recovery(lv_settings_mgr_t *mgr, lv_settings_ctx_t *ctx)
{
    lv_slider_set_value(mgr->brightness.base, 50, LV_ANIM_OFF);
    lv_label_set_text(mgr->brightness.label, "50");
    cJSON_SetIntValue(ctx->brightness.cjson, 50);

    lv_slider_set_value(mgr->contrast.base, 50, LV_ANIM_OFF);
    lv_label_set_text(mgr->contrast.label, "50");
    cJSON_SetIntValue(ctx->contrast.cjson, 50);

    lv_slider_set_value(mgr->saturation.base, 50, LV_ANIM_OFF);
    lv_label_set_text(mgr->saturation.label, "50");
    cJSON_SetIntValue(ctx->saturation.cjson, 50);

    lv_slider_set_value(mgr->hue.base, 50, LV_ANIM_OFF);
    lv_label_set_text(mgr->hue.label, "50");
    cJSON_SetIntValue(ctx->hue.cjson, 50);

    struct aicfb_disp_prop disp_prop = { 50, 50, 50, 50 };
    mpp_fb_ioctl(mgr->fb, AICFB_SET_DISP_PROP, &disp_prop);
}

static void backlight_pwm_recovery(lv_settings_mgr_t *mgr, lv_settings_ctx_t *ctx)
{
#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    char buf[8];
    u32 value;

    value = 80; /* default value */
    lv_snprintf(buf, sizeof(buf), "%d", value);

    lv_label_set_text(mgr->backlight.label, buf);
    cJSON_SetIntValue(ctx->pwm.cjson, value);

    backlight_pwm_config(AIC_PWM_BACKLIGHT_CHANNEL, value);
#else
    (void)mgr;
    (void)ctx;
#endif
}

static void disp_rotate_recovery(lv_settings_mgr_t *mgr, lv_settings_ctx_t *ctx)
{
    lv_obj_t * dd = mgr->rotate.dropdown;

    lv_dropdown_set_selected(dd, 0);
    usb_display_set_rotate(0);
}

void recovery_btn_event_handler(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_settings_ctx_t *ctx = &mgr->ctx;

    disp_prop_recovery(mgr, ctx);
    backlight_pwm_recovery(mgr, ctx);
    disp_rotate_recovery(mgr, ctx);
}

void screen_lock_mode_dropdown_event_handler(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_settings_ctx_t *ctx = &mgr->ctx;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        mgr->lock.new_mode = lv_dropdown_get_selected(obj);
        cJSON_SetIntValue(ctx->lock_mode.cjson, mgr->lock.new_mode);

        if (mgr->lock.new_mode == DISPLAY_LOGO)
            lv_obj_clear_flag(mgr->lock.blank_option, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(mgr->lock.blank_option, LV_OBJ_FLAG_HIDDEN);
    }
}

void screen_lock_delay_dropdown_event_handler(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_settings_ctx_t *ctx = &mgr->ctx;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        unsigned int index = lv_dropdown_get_selected(obj);

        switch (index) {
        case SECONDS_15_LOCK:
            mgr->lock.lock_time_ms = 15 * 1000;
            break;
        case SECONDS_30_LOCK:
            mgr->lock.lock_time_ms = 30 * 1000;
            break;
        case MINUTES_1_LOCK:
            mgr->lock.lock_time_ms = 1 * 60 * 1000;
            break;
        case MINUTES_2_LOCK:
            mgr->lock.lock_time_ms = 2 * 60 * 1000;
            break;
        case MINUTES_5_LOCK:
            mgr->lock.lock_time_ms = 5 * 60 * 1000;
            break;
        case MINUTES_10_LOCK:
            mgr->lock.lock_time_ms = 10 * 60 * 1000;
            break;
        case SCREEN_NEVER_LOCK:
            mgr->lock.lock_time_ms = 0;
            break;
        default:
            rt_kprintf("Unknown screen lock index: %#x\n", index);
            break;
        }

        cJSON_SetIntValue(ctx->lock_time.cjson, mgr->lock.lock_time_ms);
    }
}

void screen_blank_delay_dropdown_event_handler(lv_event_t * e)
{
    lv_settings_mgr_t *mgr = lv_event_get_user_data(e);
    lv_settings_ctx_t *ctx = &mgr->ctx;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        unsigned int index = lv_dropdown_get_selected(obj);

        switch (index) {
        case MINUTES_5_BLANK:
            mgr->lock.blank_time_ms = 5 * 60 * 1000;
            break;
        case MINUTES_10_BLANK:
            mgr->lock.blank_time_ms = 10 * 60 * 1000;
            break;
        case MINUTES_15_BLANK:
            mgr->lock.blank_time_ms = 15 * 60 * 1000;
            break;
        case MINUTES_30_BLANK:
            mgr->lock.blank_time_ms = 30 * 60 * 1000;
            break;
        case SCREEN_NEVER_BLANK:
            mgr->lock.blank_time_ms = 0;
            break;
        default:
            rt_kprintf("Unknown screen blank after lock index: %#x\n", index);
            break;
        }

        cJSON_SetIntValue(ctx->blank_time.cjson, mgr->lock.blank_time_ms);
    }
}

void screen_rotate_dropdown_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        unsigned int index = lv_dropdown_get_selected(obj);
        usb_display_set_rotate(index * 90);
    }
}

void device_upgrade_event_cb(lv_event_t * e)
{
    msh_exec("aicupg \n", 8);
}
