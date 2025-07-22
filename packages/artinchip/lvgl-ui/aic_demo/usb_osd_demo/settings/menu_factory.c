/*
 * Copyright (C) 2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include "usb_osd_settings.h"
#include "../usb_osd_ui.h"

#define USB_OSD_MENU_WIDTH  800
#define USB_OSD_MENU_HEIGHT 300

#define USB_OSD_MENU_HIDE_TIME_MS   (4 * 1000)

enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;

static void lv_menu_set_style(lv_obj_t * menu, struct osd_settings_manager *mgr)
{
    struct aicfb_screeninfo info;
    u32 height = USB_OSD_MENU_HEIGHT;
    u32 width = USB_OSD_MENU_WIDTH;
    lv_style_t *normal = &mgr->menu.normal_style;
    lv_style_t *rotate = &mgr->menu.rotate_style;

    mpp_fb_ioctl(mgr->fb, AICFB_GET_SCREENINFO, &info);

    lv_style_init(normal);
    lv_style_init(rotate);

    lv_style_set_bg_opa(normal, LV_OPA_90);
    lv_style_set_bg_opa(rotate, LV_OPA_90);

    if (info.width < width)
        width = info.width * 0.8;
    if (height > info.height)
        height = info.height * 0.8;

    lv_style_set_width(normal, width);
    lv_style_set_height(normal, height);
    lv_style_set_align(normal, LV_ALIGN_BOTTOM_MID);

    if (info.height > USB_OSD_MENU_WIDTH)
        width = USB_OSD_MENU_WIDTH;
    else
        width = info.height * 0.8;

    if (info.width > USB_OSD_MENU_HEIGHT)
        height = USB_OSD_MENU_HEIGHT;
    else
        height = info.width * 0.8;

    lv_style_set_width(rotate, width);
    lv_style_set_height(rotate, height);
    lv_style_set_align(rotate, LV_ALIGN_BOTTOM_MID);

    lv_obj_add_style(menu, normal, 0);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
}

static void lv_config_disp_property(lv_settings_mgr_t *mgr)
{
    lv_settings_ctx_t *ctx = &mgr->ctx;
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

    if (mpp_fb_ioctl(mgr->fb, AICFB_SET_DISP_PROP, &disp_prop))
        printf("mpp fb ioctl set disp prop failed\n");
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
        *label = lv_label_create(obj);

        lv_snprintf(srt, sizeof(srt), "%d", val);
        lv_label_set_text(*label, srt);
        lv_obj_set_width(*label, 25);
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

static lv_obj_t * create_screen_lock_mode_dropdown(lv_obj_t * parent, lv_settings_mgr_t *mgr)
{
    lv_settings_ctx_t *ctx = &mgr->ctx;

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, "LOGO \n"
                            "Pictures\n"
#ifdef LV_USB_OSD_PLAY_VIDEO
                            "Video\n"
#endif
                            "Blank screen");

    lv_dropdown_set_selected(dd, ctx->lock_mode.value);

    lv_obj_add_event_cb(dd, screen_lock_mode_dropdown_event_handler, LV_EVENT_ALL, mgr);
    return dd;
}

static lv_obj_t * create_screen_lock_delay_dropdown(lv_obj_t * parent, lv_settings_mgr_t *mgr)
{
    unsigned int lock_delay[] = {     15 * 1000,     30 * 1000,  1 * 60 * 1000,
                                  2 * 60 * 1000, 5 * 60 * 1000, 10 * 60 * 1000,
                                  0 };
    lv_settings_ctx_t *ctx = &mgr->ctx;
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
    lv_obj_add_event_cb(dd, screen_lock_delay_dropdown_event_handler, LV_EVENT_ALL, mgr);
    return dd;
}

static lv_obj_t * create_screen_blank_delay_dropdown(lv_obj_t * parent, lv_settings_mgr_t *mgr)
{
    unsigned int blank_delay[] = { 5 * 60 * 1000, 10 * 60 * 1000, 15 * 60 * 1000,
                                  30 * 60 * 1000, 0 };
    lv_settings_ctx_t *ctx = &mgr->ctx;
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
    lv_obj_add_event_cb(dd, screen_blank_delay_dropdown_event_handler, LV_EVENT_ALL, mgr);
    return dd;
}

static lv_obj_t * create_screen_rotate_dropdown(lv_obj_t * parent)
{
    /*Create a normal drop down list*/
    unsigned int lvgl_rotation[] = {LV_DISPLAY_ROTATION_0,
                                    LV_DISPLAY_ROTATION_270,
                                    LV_DISPLAY_ROTATION_180,
                                    LV_DISPLAY_ROTATION_90};
    unsigned int index = lv_display_get_rotation(NULL);
    lv_obj_t * dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, "0 \n"
                            "90\n"
                            "180\n"
                            "270");

    lv_dropdown_set_selected(dd, lvgl_rotation[index]);
    lv_obj_add_event_cb(dd, screen_rotate_dropdown_event_handler, LV_EVENT_ALL, NULL);
    return dd;
}

void lv_settings_menu_init(lv_obj_t * parent, struct osd_settings_manager *mgr)
{
    lv_settings_ctx_t *ctx = &mgr->ctx;
    lv_obj_t *menu = lv_menu_create(parent);
    mgr->menu.base = menu;

    lv_obj_add_event_cb(menu, menu_event_handler, LV_EVENT_ALL, mgr);
    lv_timer_create(menu_hide_callback, USB_OSD_MENU_HIDE_TIME_MS, mgr);

    lv_menu_set_style(menu, mgr);

    lv_color_t bg_color = lv_obj_get_style_bg_color(menu, 0);
    if(lv_color_brightness(bg_color) > 127)
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 10), 0);
    else
        lv_obj_set_style_bg_color(menu, lv_color_darken(lv_obj_get_style_bg_color(menu, 0), 50), 0);

    /* Create sub pages */
    lv_obj_t * cont;
    lv_obj_t * section;

    /* device sub update page */
    lv_obj_t * sub_device_upgrade_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_device_upgrade_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(sub_device_upgrade_page);
    cont = create_switch(section, NULL, "device upgrade enable", false);
    lv_obj_add_event_cb(cont, device_upgrade_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

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
    cont = create_text(section, LV_SYMBOL_RIGHT, "device upgrade", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_device_upgrade_page);

    /* sound sub page */
    lv_obj_t * sub_sound_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_sound_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_sound_page);
    section = lv_menu_section_create(sub_sound_page);
    create_switch(section, LV_SYMBOL_AUDIO, "Sound", false);
    create_slider(section, LV_SYMBOL_SETTINGS, "Media", 0, 100, 50, NULL);

    /* display sub page */
    lv_obj_t * sub_display_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_display_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_display_page);
    section = lv_menu_section_create(sub_display_page);

    unsigned int default_value;
    struct slider_obj *slider;

    cont = create_text(section, NULL, "Rotate degree", LV_MENU_ITEM_BUILDER_VARIANT_1);
    mgr->rotate.dropdown = create_screen_rotate_dropdown(cont);

    /* display sub page slider */
#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    /* backlight sub page slider */
    slider = &mgr->backlight;
    default_value = ctx->pwm.value;
    slider->base = create_slider(section, LV_SYMBOL_SETTINGS, "PWM-Backlight", 10, 100, default_value, &slider->label);
    lv_obj_add_event_cb(slider->base, backlight_slider_event_cb, LV_EVENT_VALUE_CHANGED, mgr);
    backlight_pwm_config(AIC_PWM_BACKLIGHT_CHANNEL, default_value);
#endif

    slider = &mgr->brightness;
    default_value = ctx->brightness.value;
    slider->base = create_slider(section, LV_SYMBOL_SETTINGS, "Brightness", 0, 100, default_value, &slider->label);
    lv_obj_add_event_cb(slider->base, slider_event_cb, LV_EVENT_VALUE_CHANGED, mgr);

    slider = &mgr->contrast;
    default_value = ctx->contrast.value;
    slider->base =create_slider(section, LV_SYMBOL_SETTINGS, "Contrast", 0, 100, default_value, &slider->label);
    lv_obj_add_event_cb(slider->base, slider_event_cb, LV_EVENT_VALUE_CHANGED, mgr);

    slider = &mgr->saturation;
    default_value = ctx->saturation.value;
    slider->base =create_slider(section, LV_SYMBOL_SETTINGS, "Saturation", 0, 100, default_value, &slider->label);
    lv_obj_add_event_cb(slider->base, slider_event_cb, LV_EVENT_VALUE_CHANGED, mgr);

    slider = &mgr->hue;
    default_value = ctx->hue.value;
    slider->base =create_slider(section, LV_SYMBOL_SETTINGS, "Hue", 0, 100, default_value, &slider->label);
    lv_obj_add_event_cb(slider->base, slider_event_cb, LV_EVENT_VALUE_CHANGED, mgr);

    lv_config_disp_property(mgr);

    /* display sub page recovery button */
    lv_obj_t * recovery_btn = lv_btn_create(section);
    lv_obj_add_event_cb(recovery_btn, recovery_btn_event_handler, LV_EVENT_CLICKED, mgr);
    lv_obj_center(recovery_btn);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_opa(&style, LV_OPA_50);
    lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_obj_add_style(recovery_btn, &style, 0);

    lv_obj_t * label = lv_label_create(recovery_btn);
    lv_label_set_text(label, "recovery");
    lv_obj_center(label);

    /* lock sub page */
    lv_obj_t * sub_lock_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(sub_lock_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    lv_menu_separator_create(sub_lock_page);
    section = lv_menu_section_create(sub_lock_page);

    cont = create_text(section, NULL, "Screen Lock Delay", LV_MENU_ITEM_BUILDER_VARIANT_1);
    create_screen_lock_delay_dropdown(cont, mgr);
    mgr->lock.lock_time_ms = ctx->lock_time.value;

    cont = create_text(section, NULL, "Screen Lock Mode", LV_MENU_ITEM_BUILDER_VARIANT_1);
    create_screen_lock_mode_dropdown(cont, mgr);
    mgr->lock.new_mode = ctx->lock_mode.value;
    mgr->lock.old_mode = ctx->lock_mode.value;

    cont = create_text(section, NULL, "Screen Blank After Lock", LV_MENU_ITEM_BUILDER_VARIANT_1);
    create_screen_blank_delay_dropdown(cont, mgr);
    mgr->lock.blank_option = cont;
    mgr->lock.blank_time_ms = ctx->blank_time.value;

    /* Create a root page */
    lv_obj_t * root_page = lv_menu_page_create(menu, "OSD");
    lv_obj_set_style_pad_hor(root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
    section = lv_menu_section_create(root_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Display", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_display_page);
    cont = create_text(section, LV_SYMBOL_POWER, "Lockscreen", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_lock_page);
    cont = create_text(section, LV_SYMBOL_AUDIO, "Sound", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_sound_page);
    cont = create_text(section, LV_SYMBOL_SETTINGS, "Device", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(menu, cont, sub_device_page);

    lv_menu_set_sidebar_page(menu, root_page);

#if LVGL_VERSION_MAJOR == 8
    lv_event_send(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED, NULL);
#else
    lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(menu), 0), 0), LV_EVENT_CLICKED,
                      NULL);
#endif
}
