/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include <dirent.h>

#include "lv_tpc_run.h"
#include "usb_osd_settings.h"
#include "usb_osd_video.h"
#include "usb_osd_ui.h"
#include "mpp_fb.h"

#define USB_OSD_CONFIG_FILE      "/data/lvgl_data/osd.config"

#define USB_OSD_MENU_WIDTH  800
#define USB_OSD_MENU_HEIGHT 300

#define USB_OSD_SCREEN_LOCK_TIME_MS (LV_USB_OSD_SCREEN_LOCK_TIME * 1000)
#define USB_OSD_SCREEN_LOCK_MODE LV_USB_OSD_SCREEN_LOCK_MODE

#ifdef LV_USB_OSD_SCREEN_BLANK_TIME_AFTER_LOCK
#define USB_OSD_SCREEN_BLANK_TIME_MS (LV_USB_OSD_SCREEN_BLANK_TIME_AFTER_LOCK * 1000)
#else
#define USB_OSD_SCREEN_BLANK_TIME_MS    0
#endif

#define USB_OSD_MENU_HIDE_TIME_MS   (4 * 1000)

#ifndef LVGL_STORAGE_PATH
#define LVGL_STORAGE_PATH "/rodata/lvgl_data"
#endif
#define USB_OSD_PICTURES_DIR    LVGL_STORAGE_PATH"/pictures"

enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;

static lv_obj_t * brightness_label = NULL;
static lv_obj_t * contrast_label = NULL;
static lv_obj_t * saturation_label = NULL;
static lv_obj_t * hue_label = NULL;

static lv_obj_t * brightness_slider = NULL;
static lv_obj_t * contrast_slider = NULL;
static lv_obj_t * saturation_slider = NULL;
static lv_obj_t * hue_slider = NULL;

static lv_obj_t * screen_blank_time_obj = NULL;

static lv_obj_t * menu;
static lv_obj_t * root_page;
static lv_obj_t * g_image;
static lv_obj_t * g_logo_image;

static struct mpp_fb * g_fb;

static unsigned int g_screen_lock_time_ms = 0;
static unsigned int g_screen_lock_mode = 0;
static unsigned int g_screen_blank_time_ms = 0;

static unsigned int g_sleep_count = 0;
static unsigned int g_image_index = 1;
static unsigned int g_image_count = 0;

static unsigned int g_menu_draw_count = 0;
static unsigned int g_hide_time_count = 0;

static struct osd_settings_context g_settings_ctx = { 0 };

bool lv_settings_menu_is_enabled(void)
{
    return !lv_obj_has_flag(menu, LV_OBJ_FLAG_HIDDEN);
}

void lv_settings_menu_enable(bool enable)
{
    if (enable)
        lv_obj_clear_flag(menu, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
}

bool lv_pictures_screen_is_enabled(void)
{
    return !lv_obj_has_flag(g_image, LV_OBJ_FLAG_HIDDEN);
}

void lv_pictures_screen_enable(bool enable)
{
    if (enable)
        lv_obj_clear_flag(g_image, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(g_image, LV_OBJ_FLAG_HIDDEN);
}

bool lv_logo_screen_is_enabled(void)
{
    return !lv_obj_has_flag(g_logo_image, LV_OBJ_FLAG_HIDDEN);
}

void lv_logo_screen_enable(bool enable)
{
    if (enable)
        lv_obj_clear_flag(g_logo_image, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(g_logo_image, LV_OBJ_FLAG_HIDDEN);
}

static void usb_osd_pic_callback(lv_timer_t *tmr)
{
    lv_obj_t * settings_screen = tmr->user_data;
    char image_str[64];

    if ((settings_screen != lv_scr_act()) ||
            !lv_pictures_screen_is_enabled())
        return;

    g_sleep_count++;

    /*
     * Automatically switches one picture every 5 seconds
     * menu_event_cb() switches picture will not reset this timer
     */
    if ((g_sleep_count % 10) == 0) {
        snprintf(image_str, sizeof(image_str), LVGL_PATH(pictures/image%d.jpg), g_image_index);
        lv_img_set_src(g_image, image_str);

        g_image_index++;

        if (g_image_index >= g_image_count)
            g_image_index = 0;

        g_sleep_count = 0;
    }
}

static unsigned int old_mode = 0;

#ifdef LV_USB_OSD_SETTINGS_MENU
static unsigned int long_press_end = 0;

static void screen_global_alpha_set(void)
{
    struct aicfb_alpha_config alpha = {0};
    int ret;

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = AICFB_GLOBAL_ALPHA_MODE;
    alpha.value = 255;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        printf("set alpha failed\n");
}

static void back_pictures_screen(void)
{
    lv_logo_screen_enable(false);
    lv_pictures_screen_enable(true);
    screen_global_alpha_set();
}

static void back_logo_screen(void)
{
    lv_logo_screen_enable(true);
    lv_pictures_screen_enable(false);
    screen_global_alpha_set();
}
#endif /* LV_USB_OSD_SETTINGS_MENU */

static void lv_save_config_file(void)
{
    struct osd_settings_context *ctx = &g_settings_ctx;
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

static void menu_event_cb(lv_event_t * e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    char image_str[64];

    /* switch picture by tp gestur event */
    if (code == LV_EVENT_GESTURE) {
        if (!lv_pictures_screen_is_enabled())
            return;

        if(dir == LV_DIR_LEFT)
            g_image_index = (g_image_index == 0) ? (g_image_count - 1) : (g_image_index - 1);

        if(dir == LV_DIR_RIGHT)
            g_image_index = (g_image_index == g_image_count - 1) ? (0) : (g_image_index + 1);

        snprintf(image_str, sizeof(image_str), LVGL_PATH(pictures/image%d.jpg), g_image_index);
        lv_img_set_src(g_image, image_str);
    }

#ifdef LV_USB_OSD_SETTINGS_MENU
    /* show settings menu by tp long pressed event */
    if (code == LV_EVENT_LONG_PRESSED) {
        if (!lv_settings_menu_is_enabled())
            lv_settings_menu_enable(true);

        old_mode = lv_get_screen_lock_mode();
        /*
         * Normally, a LV_EVENT_CLICKED event will be generated after the
         * LV_EVENT_LONG_PRESSED event ends, and we need to filter it out.
         */
        long_press_end = 1;
    }

    if (code == LV_EVENT_CLICKED) {
        if (lv_settings_menu_is_enabled()) {

            /* Filter out a LV_EVENT_CLICKED event */
            if (long_press_end == 1 || is_video_screen_switched()) {
                long_press_end = 0;
                video_screen_switched(false);
                return;
            }

            lv_settings_menu_enable(false);

            lv_save_config_file();
        }

        /* switch to video screen to display controls such as video progress bars */
        if(lv_get_screen_lock_mode() == DISPLAY_VIDEO)
            lv_load_video_screen();

        if (is_usb_disp_suspend()) {
            int new_mode = lv_get_screen_lock_mode();

            if (old_mode == DISPLAY_VIDEO && new_mode != DISPLAY_VIDEO)
                lv_video_stop();

            if (old_mode == DISPLAY_VIDEO && new_mode == DISPLAY_VIDEO)
                return;

            old_mode = new_mode;

            switch (new_mode) {
            case DISPLAY_LOGO:
                back_logo_screen();
                break;
            case DISPLAY_PICTURES:
                back_pictures_screen();
                break;
            case BLANK_SCREEN:
            {
                lv_pictures_screen_enable(false);
                lv_logo_screen_enable(false);

                usb_osd_modify_blank_status(true);
                break;
            }
            case DISPLAY_VIDEO:
                back_video_screen();
                break;
            default:
                printf("Invalid lock screen mode\n");
                break;
            }
        } else {
            lv_pictures_screen_enable(false);
            lv_logo_screen_enable(false);
            if (!is_usb_disp_suspend())
                lvgl_put_tp();
        }
    }
#endif /* LV_USB_OSD_SETTINGS_MENU */
}

/*
 * Record settings menu operation event to determine whether to hide
 */
static void menu_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * draw_menu = lv_event_get_user_data(e);

    if(code == LV_EVENT_DRAW_MAIN_END && draw_menu == menu)
        g_menu_draw_count++;
}

static void menu_hide_callback(lv_timer_t *tmr)
{
    if (g_menu_draw_count == 0)
        return;

    /*
     * There has been no menu operation for USB_OSD_MENU_HIDE_TIME_MS.
     * Automatically hide the settings menu.
     */
    if (g_menu_draw_count == g_hide_time_count) {
        lv_settings_menu_enable(false);
#if LVGL_VERSION_MAJOR == 8
        lv_event_send(menu, LV_EVENT_CLICKED, NULL);
#else
        lv_obj_send_event(menu, LV_EVENT_CLICKED, NULL);
#endif

        if (!is_usb_disp_suspend())
            lvgl_put_tp();

        g_hide_time_count = 0;
        g_menu_draw_count = 0;
        return;
    }

    g_hide_time_count = g_menu_draw_count;
}

static void slider_event_cb(lv_event_t * e)
{
    struct osd_settings_context *ctx = &g_settings_ctx;
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * slider_label = lv_event_get_user_data(e);
    struct aicfb_disp_prop disp_prop;
    char buf[8];
    u32 origin, value;

    origin = (u32)lv_slider_get_value(slider);
    lv_snprintf(buf, sizeof(buf), "%d", origin);
    lv_label_set_text(slider_label, buf);

    if (mpp_fb_ioctl(g_fb, AICFB_GET_DISP_PROP, &disp_prop)) {
        printf("mpp fb ioctl get disp prop failed\n");
        return;
    }

    /* limit value in [25, 75] */
    value = (origin >> 1) + 25;

    if (slider_label == brightness_label) {
        disp_prop.bright = value;
        cJSON_SetIntValue(ctx->brightness.cjson, origin);
    }

    if (slider_label == contrast_label) {
        disp_prop.contrast = value;
        cJSON_SetIntValue(ctx->contrast.cjson, origin);
    }

    if (slider_label == saturation_label) {
        disp_prop.saturation = value;
        cJSON_SetIntValue(ctx->saturation.cjson, origin);
    }

    if (slider_label == hue_label) {
        disp_prop.hue = value;
        cJSON_SetIntValue(ctx->hue.cjson, origin);
    }

    if (mpp_fb_ioctl(g_fb, AICFB_SET_DISP_PROP, &disp_prop)) {
        printf("mpp fb ioctl set disp prop failed\n");
        return;
    }
}
static void lv_config_disp_property(struct osd_settings_context *ctx)
{
    struct aicfb_disp_prop disp_prop;
    u32 value;

    /* limit value in [25, 75] */
    value = (ctx->brightness.value >> 1) + 25;
    disp_prop.bright = value;

    value = (ctx->contrast.value >> 1) + 25;
    disp_prop.contrast = value;

    value = (ctx->saturation.value >> 1) + 25;
    disp_prop.saturation = value;

    value = (ctx->hue.value >> 1) + 25;
    disp_prop.hue = value;

    if (mpp_fb_ioctl(g_fb, AICFB_SET_DISP_PROP, &disp_prop)) {
        printf("mpp fb ioctl set disp prop failed\n");
        return;
    }
}

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
static lv_obj_t * backlight_label = NULL;
static lv_obj_t * backlight_slider = NULL;

static void backlight_pwm_config(u32 channel, u32 level)
{
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    /* pwm frequency: 1KHz = 1000000ns */
    rt_pwm_set(pwm_dev, channel, 1000000, 10000 * level);
}

static void backlight_slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * slider_label = lv_event_get_user_data(e);
    struct osd_settings_context *ctx = &g_settings_ctx;

    char buf[8];
    u32 value;

    value = (u32)lv_slider_get_value(slider);
    lv_snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(slider_label, buf);
    cJSON_SetIntValue(ctx->pwm.cjson, value);

    backlight_pwm_config(AIC_PWM_BACKLIGHT_CHANNEL, value);
}
#endif

static void recovery_btn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    struct osd_settings_context *ctx = &g_settings_ctx;

    if(code == LV_EVENT_CLICKED) {

        lv_slider_set_value(brightness_slider, 50, LV_ANIM_OFF);
        lv_label_set_text(brightness_label, "50");
        cJSON_SetIntValue(ctx->brightness.cjson, 50);

        lv_slider_set_value(contrast_slider, 50, LV_ANIM_OFF);
        lv_label_set_text(contrast_label, "50");
        cJSON_SetIntValue(ctx->contrast.cjson, 50);

        lv_slider_set_value(saturation_slider, 50, LV_ANIM_OFF);
        lv_label_set_text(saturation_label, "50");
        cJSON_SetIntValue(ctx->saturation.cjson, 50);

        lv_slider_set_value(hue_slider, 50, LV_ANIM_OFF);
        lv_label_set_text(hue_label, "50");
        cJSON_SetIntValue(ctx->hue.cjson, 50);

        struct aicfb_disp_prop disp_prop = { 50, 50, 50, 50 };
        mpp_fb_ioctl(g_fb, AICFB_SET_DISP_PROP, &disp_prop);
    }
}

static void screen_lock_mode_dropdown_event_handler(lv_event_t * e)
{
    struct osd_settings_context *ctx = &g_settings_ctx;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        g_screen_lock_mode = lv_dropdown_get_selected(obj);
        cJSON_SetIntValue(ctx->lock_mode.cjson, g_screen_lock_mode);

        if (g_screen_lock_mode == DISPLAY_LOGO)
            lv_obj_clear_flag(screen_blank_time_obj, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(screen_blank_time_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void screen_lock_delay_dropdown_event_handler(lv_event_t * e)
{
    struct osd_settings_context *ctx = &g_settings_ctx;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        unsigned int index = lv_dropdown_get_selected(obj);

        switch (index) {
        case SECONDS_15_LOCK:
            g_screen_lock_time_ms = 15 * 1000;
            break;
        case SECONDS_30_LOCK:
            g_screen_lock_time_ms = 30 * 1000;
            break;
        case MINUTES_1_LOCK:
            g_screen_lock_time_ms = 1 * 60 * 1000;
            break;
        case MINUTES_2_LOCK:
            g_screen_lock_time_ms = 2 * 60 * 1000;
            break;
        case MINUTES_5_LOCK:
            g_screen_lock_time_ms = 5 * 60 * 1000;
            break;
        case MINUTES_10_LOCK:
            g_screen_lock_time_ms = 10 * 60 * 1000;
            break;
        case SCREEN_NEVER_LOCK:
            g_screen_lock_time_ms = 0;
            break;
        default:
            rt_kprintf("Unknown screen lock index: %#x\n", index);
            break;
        }

        cJSON_SetIntValue(ctx->lock_time.cjson, g_screen_lock_time_ms);
    }
}

static void screen_blank_delay_dropdown_event_handler(lv_event_t * e)
{
    struct osd_settings_context *ctx = &g_settings_ctx;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        unsigned int index = lv_dropdown_get_selected(obj);

        switch (index) {
        case MINUTES_5_BLANK:
            g_screen_blank_time_ms = 5 * 60 * 1000;
            break;
        case MINUTES_10_BLANK:
            g_screen_blank_time_ms = 10 * 60 * 1000;
            break;
        case MINUTES_15_BLANK:
            g_screen_blank_time_ms = 15 * 60 * 1000;
            break;
        case MINUTES_30_BLANK:
            g_screen_blank_time_ms = 30 * 60 * 1000;
            break;
        case SCREEN_NEVER_BLANK:
            g_screen_blank_time_ms = 0;
            break;
        default:
            rt_kprintf("Unknown screen blank after lock index: %#x\n", index);
            break;
        }

        cJSON_SetIntValue(ctx->blank_time.cjson, g_screen_blank_time_ms);
    }
}

int lv_get_screen_lock_time_ms(void)
{
    return g_screen_lock_time_ms;
}

int lv_get_screen_lock_mode(void)
{
    return g_screen_lock_mode;
}

int lv_get_screen_blank_tmie_ms(void)
{
    return g_screen_blank_time_ms;
}

static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

static lv_obj_t * create_slider(lv_obj_t * parent,
                                const char * icon, const char * txt,
                                int32_t min, int32_t max, int32_t val,
                                lv_obj_t ** label)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);
    char srt[8];

    lv_obj_t * slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, LV_FLEX_FLOW_COLUMN);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }

    if (label) {
        lv_obj_t * label_obj = lv_menu_cont_create(parent);
        *label = lv_label_create(label_obj);

        lv_snprintf(srt, sizeof(srt), "%d", val);
        lv_label_set_text(*label, srt);

        lv_obj_set_flex_flow(label_obj, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(label_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    }

    return slider;
}

static lv_obj_t * create_switch(lv_obj_t * parent,
                                const char * icon, const char * txt, bool chk)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

    lv_obj_t * sw = lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return sw;
}

static lv_obj_t * creat_screen_lock_mode_dropdown(lv_obj_t * parent)
{
    struct osd_settings_context *ctx = &g_settings_ctx;

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, "LOGO \n"
                            "Pictures\n"
#ifdef LV_USB_OSD_PLAY_VIDEO
                            "Video\n"
#endif
                            "Blank screen");

    lv_dropdown_set_selected(dd, ctx->lock_mode.value);

    lv_obj_add_event_cb(dd, screen_lock_mode_dropdown_event_handler, LV_EVENT_ALL, NULL);
    return dd;
}

static lv_obj_t * creat_screen_lock_delay_dropdown(lv_obj_t * parent)
{
    unsigned int lock_delay[] = {     15 * 1000,     30 * 1000,  1 * 60 * 1000,
                                  2 * 60 * 1000, 5 * 60 * 1000, 10 * 60 * 1000,
                                  0 };
    struct osd_settings_context *ctx = &g_settings_ctx;
    int i;

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, "15 seconds\n"
                            "30 seconds\n"
                            "1 minutes\n"
                            "2 minutes\n"
                            "5 minutes\n"
                            "10 minutes\n"
                            "Never");

    for (i = 0; i < ARRAY_SIZE(lock_delay); i++)
        if (ctx->lock_time.value == lock_delay[i])
            break;

    lv_dropdown_set_selected(dd, i);
    lv_obj_add_event_cb(dd, screen_lock_delay_dropdown_event_handler, LV_EVENT_ALL, NULL);
    return dd;
}

static lv_obj_t * creat_screen_blank_delay_dropdown(lv_obj_t * parent)
{
    unsigned int blank_delay[] = {  5 * 60 * 1000, 10 * 60 * 1000, 15 * 60 * 1000,
                                  30 * 60 * 1000, 0 };
    struct osd_settings_context *ctx = &g_settings_ctx;
    int i;

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, "5 minutes \n"
                            "10 minutes\n"
                            "15 minutes\n"
                            "30 minutes\n"
                            "Never");

    for (i = 0; i < ARRAY_SIZE(blank_delay); i++)
        if (ctx->blank_time.value == blank_delay[i])
            break;

    lv_dropdown_set_selected(dd, i);
    lv_obj_add_event_cb(dd, screen_blank_delay_dropdown_event_handler, LV_EVENT_ALL, NULL);
    return dd;
}

static void lv_mpp_fb_open(void)
{
    g_fb = mpp_fb_open();
}

static void lv_bg_dark_set(lv_obj_t * parent)
{
    struct aicfb_screeninfo info;
    char bg_dark[256];

    mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &info);
    snprintf(bg_dark, 255, "L:/%dx%d_%d_%08x.fake",\
                 info.width, info.height, 0, 0x00000000);

    lv_obj_t * img_bg = lv_img_create(parent);
    lv_img_set_src(img_bg, bg_dark);
    lv_obj_set_pos(img_bg, 0, 0);
}

static void lv_logo_image_create(lv_obj_t * parent)
{
#ifdef LV_USB_OSD_LOGO_IMAGE
    char logo_image_src[64];
    snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_IMAGE);
#else
    struct aicfb_screeninfo info;
    char logo_image_src[256];

    mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &info);
    snprintf(logo_image_src, 255, "L:/%dx%d_%d_%08x.fake",\
                 info.width, info.height, 0, 0x00000000);
#endif

    g_logo_image = lv_img_create(parent);
    lv_img_set_src(g_logo_image, logo_image_src);
    lv_obj_set_pos(g_logo_image, 0, 0);
    lv_obj_add_flag(g_logo_image, LV_OBJ_FLAG_HIDDEN);
}

static unsigned int lv_calculate_pictures_count(void)
{
    DIR *dir;
    struct dirent *file;
    int image_count = 0;

    dir = opendir(USB_OSD_PICTURES_DIR);
    if (!dir) {
        printf("failed to open %s dir\n", USB_OSD_PICTURES_DIR);
        return 0;
    }

    while ((file = readdir(dir)) != NULL)
        image_count++;

    printf("find image num: %d in %s dir\n",image_count, USB_OSD_PICTURES_DIR);

    closedir(dir);
    return image_count;
}

static void lv_menu_set_style(lv_obj_t * menu)
{
    struct aicfb_screeninfo info;
    u32 height = USB_OSD_MENU_HEIGHT;
    u32 width = USB_OSD_MENU_WIDTH;

    mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &info);

    static lv_style_t menu_style;
    lv_style_init(&menu_style);
    lv_style_set_bg_opa(&menu_style, LV_OPA_90);

    if (info.width < width)
        width = info.width * 0.8;
    if (height > info.height)
        height = info.height * 0.8;

    lv_style_set_width(&menu_style, width);
    lv_style_set_height(&menu_style, height);
    lv_style_set_align(&menu_style, LV_ALIGN_BOTTOM_MID);

    lv_obj_add_style(menu, &menu_style, 0);
}

lv_obj_t * lv_settings_screen_creat(void)
{
    struct osd_settings_context *ctx = &g_settings_ctx;
    lv_obj_t * settings_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(settings_screen, menu_event_cb, LV_EVENT_ALL, NULL);

    lv_mpp_fb_open();
    lv_bg_dark_set(settings_screen);
    lv_logo_image_create(settings_screen);
    lv_parse_config_file(USB_OSD_CONFIG_FILE, ctx);

    g_image = lv_img_create(settings_screen);
    lv_img_set_src(g_image, LVGL_PATH(pictures/image0.jpg));
    lv_obj_set_pos(g_image, 0, 0);

    g_image_count = lv_calculate_pictures_count();
    if (g_image_count > 0)
        lv_timer_create(usb_osd_pic_callback, 500, settings_screen);

    menu = lv_menu_create(settings_screen);
    lv_obj_add_event_cb(menu, menu_event_handler, LV_EVENT_ALL, menu);
    lv_timer_create(menu_hide_callback, USB_OSD_MENU_HIDE_TIME_MS, 0);
    lv_menu_set_style(menu);

    lv_color_t bg_color = lv_obj_get_style_bg_color(menu, 0);
    if(lv_color_brightness(bg_color) > 127)
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 10), 0);
    else
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 50), 0);

    /* Create sub pages */
    lv_obj_t * cont;
    lv_obj_t * section;

    /* device sub page */
    lv_obj_t * sub_device_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_device_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_device_page);
    section = lv_menu_section_create(sub_device_page);
    cont = create_text(section, NULL, "Chip ID: D215BBV", LV_MENU_ITEM_BUILDER_VARIANT_1);
    cont = create_text(section, NULL, "Connection Type: USB", LV_MENU_ITEM_BUILDER_VARIANT_1);
    cont = create_text(section, NULL, "Resolution: 1024 * 600", LV_MENU_ITEM_BUILDER_VARIANT_1);
    cont = create_text(section, NULL,
                       "Copyright (C) 2024 ArtinChip Technology Co., Ltd.", LV_MENU_ITEM_BUILDER_VARIANT_1);

    /* sound sub page */
    lv_obj_t * sub_sound_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_sound_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_sound_page);
    section = lv_menu_section_create(sub_sound_page);
    create_switch(section, LV_SYMBOL_AUDIO, "Sound", false);
    create_slider(section, LV_SYMBOL_SETTINGS, "Media", 0, 100, 50, NULL);

    /* image sub page */
    lv_obj_t * sub_image_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_image_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_image_page);
    section = lv_menu_section_create(sub_image_page);

    /* display sub page recovery button */
    lv_obj_t * recovery_btn = lv_btn_create(section);
    lv_obj_add_event_cb(recovery_btn, recovery_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_center(recovery_btn);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_opa(&style, LV_OPA_50);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_obj_add_style(recovery_btn, &style, 0);

    lv_obj_t * label = lv_label_create(recovery_btn);
    lv_label_set_text(label, "recovery");
    lv_obj_center(label);

    unsigned int default_value;

    /* display sub page slider */
    default_value = ctx->brightness.value;
    brightness_slider = create_slider(section, LV_SYMBOL_SETTINGS, "Brightness", 0, 100, default_value, &brightness_label);
    lv_obj_add_event_cb(brightness_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, brightness_label);

    default_value = ctx->contrast.value;
    contrast_slider =create_slider(section, LV_SYMBOL_SETTINGS, "Contrast", 0, 100, default_value, &contrast_label);
    lv_obj_add_event_cb(contrast_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, contrast_label);

    default_value = ctx->saturation.value;
    saturation_slider =create_slider(section, LV_SYMBOL_SETTINGS, "Saturation", 0, 100, default_value, &saturation_label);
    lv_obj_add_event_cb(saturation_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, saturation_label);

    default_value = ctx->hue.value;
    hue_slider =create_slider(section, LV_SYMBOL_SETTINGS, "Hue", 0, 100, default_value, &hue_label);
    lv_obj_add_event_cb(hue_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, hue_label);

    lv_config_disp_property(ctx);

    /* lock sub page */
    lv_obj_t * sub_lock_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_lock_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_lock_page);
    section = lv_menu_section_create(sub_lock_page);

    cont = create_text(section, NULL, "Screen Lock Delay", LV_MENU_ITEM_BUILDER_VARIANT_1);
    creat_screen_lock_delay_dropdown(cont);
    g_screen_lock_time_ms = ctx->lock_time.value;

    cont = create_text(section, NULL, "Screen Lock Mode", LV_MENU_ITEM_BUILDER_VARIANT_1);
    creat_screen_lock_mode_dropdown(cont);
    g_screen_lock_mode = ctx->lock_mode.value;
    old_mode = ctx->lock_mode.value;

    cont = create_text(section, NULL, "Screen Blank After Lock", LV_MENU_ITEM_BUILDER_VARIANT_1);
    creat_screen_blank_delay_dropdown(cont);
    screen_blank_time_obj = cont;
    g_screen_blank_time_ms = ctx->blank_time.value;

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    /* backlight sub page */
    lv_obj_t * sub_backlight_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_backlight_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_backlight_page);
    section = lv_menu_section_create(sub_backlight_page);

    /* backlight sub page slider */
    default_value = ctx->pwm.value;
    backlight_slider = create_slider(section, LV_SYMBOL_SETTINGS, "PWM-Backlight", 10, 100, default_value, &backlight_label);
    lv_obj_add_event_cb(backlight_slider, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, backlight_label);
    backlight_pwm_config(AIC_PWM_BACKLIGHT_CHANNEL, default_value);
#endif

    /* Create a root page */
    root_page = lv_menu_page_create(menu, "OSD");
    lv_obj_set_style_pad_hor(root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(root_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Device", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_device_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Image", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_image_page);
    cont = create_text(section, LV_SYMBOL_POWER, "Lockscreen", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_lock_page);
    cont = create_text(section, LV_SYMBOL_AUDIO, "Sound", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_sound_page);
#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Backlight", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_backlight_page);
#endif

    lv_menu_set_sidebar_page(menu, root_page);

#if LVGL_VERSION_MAJOR == 8
    lv_event_send(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED, NULL);
#else
    lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED,
                      NULL);
#endif

    return settings_screen;
}
