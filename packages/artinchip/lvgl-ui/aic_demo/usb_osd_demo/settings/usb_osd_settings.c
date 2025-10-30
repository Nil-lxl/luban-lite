/*
 * Copyright (C) 2024-2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include <dirent.h>

#include "lv_tpc_run.h"
#include "usb_osd_settings.h"
#include "../video/usb_osd_video.h"
#include "../usb_osd_ui.h"
#include "mpp_fb.h"

#define USB_OSD_SCREEN_LOCK_TIME_MS (LV_USB_OSD_SCREEN_LOCK_TIME * 1000)
#define USB_OSD_SCREEN_LOCK_MODE LV_USB_OSD_SCREEN_LOCK_MODE

#ifdef LV_USB_OSD_SCREEN_BLANK_TIME_AFTER_LOCK
#define USB_OSD_SCREEN_BLANK_TIME_MS (LV_USB_OSD_SCREEN_BLANK_TIME_AFTER_LOCK * 1000)
#else
#define USB_OSD_SCREEN_BLANK_TIME_MS    0
#endif

#ifndef LVGL_STORAGE_PATH
#define LVGL_STORAGE_PATH "/rodata/lvgl_data"
#endif
#define USB_OSD_PICTURES_DIR           LVGL_STORAGE_PATH"/pic_normal"
#define USB_OSD_ROTATE_PICTURES_DIR    LVGL_STORAGE_PATH"/pic_rotate"

static struct osd_settings_manager g_settings_mgr = { 0 };

unsigned int osd_rotation;

static lv_settings_mgr_t *lv_settings_get_manager()
{
    return &g_settings_mgr;
}

void usb_osd_ui_rotate(unsigned int rotation)
{
    size_t flag;
    unsigned int lvgl_rotation[] = {LV_DISPLAY_ROTATION_0,
                                    LV_DISPLAY_ROTATION_270,
                                    LV_DISPLAY_ROTATION_180,
                                    LV_DISPLAY_ROTATION_90};
    unsigned int index = (rotation % 360) / 90;

    flag = aicos_enter_critical_section();
    osd_rotation = lvgl_rotation[index];
    aicos_leave_critical_section(flag);
}

void lv_load_settings_screen(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    lv_scr_load(mgr->screen);
    lv_obj_clear_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);
}

static void usb_osd_pic_callback(lv_timer_t *tmr)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);
    struct osd_settings_manager *mgr = tmr->user_data;
    char image_str[64];

    if ((mgr->screen != lv_scr_act()) ||
        lv_obj_has_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN))
        return;

    /*
     * Automatically switches one picture every 5 seconds
     * menu_event_cb() switches picture will not reset this timer
     */
    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        snprintf(image_str, sizeof(image_str), LVGL_PATH(pic_rotate/image%d.jpg), mgr->pic.image_index);
    else
        snprintf(image_str, sizeof(image_str), LVGL_PATH(pic_normal/image%d.jpg), mgr->pic.image_index);

    lv_img_set_src(mgr->images.pictures, image_str);

    mgr->pic.image_index++;

    if (mgr->pic.image_index >= mgr->pic.image_count)
        mgr->pic.image_index = 0;
}

static void lv_bg_dark_config(lv_display_rotation_t rotation)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();
    struct aicfb_screeninfo info;
    char bg_dark[256];

    mpp_fb_ioctl(mgr->fb, AICFB_GET_SCREENINFO, &info);

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        snprintf(bg_dark, 255, "L:/%dx%d_%d_%08x.fake",\
                 info.height, info.width, 0, 0x00000000);
    else
        snprintf(bg_dark, 255, "L:/%dx%d_%d_%08x.fake",\
                 info.width, info.height, 0, 0x00000000);

    lv_img_set_src(mgr->images.background, bg_dark);
    lv_obj_set_pos(mgr->images.background, 0, 0);
}

static void lv_bg_dark_init(lv_obj_t * parent)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);

    lv_bg_dark_config(rotation);
}

static void lv_logo_image_create(lv_obj_t * parent)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);
    lv_settings_mgr_t *mgr = lv_settings_get_manager();
    char logo_image_src[64];

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_ROTATE_IMAGE);
    else
        snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_IMAGE);

    lv_img_set_src(mgr->images.logo, logo_image_src);
    lv_obj_set_pos(mgr->images.logo, 0, 0);
    lv_obj_add_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
}

static unsigned int lv_calculate_pictures_count(void)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);
    int image_count = 0;
    struct dirent *file;
    DIR *dir;

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        dir = opendir(USB_OSD_ROTATE_PICTURES_DIR);
    else
        dir = opendir(USB_OSD_PICTURES_DIR);

    if (!dir) {
        printf("failed to open pictures dir\n");
        return 0;
    }

    while ((file = readdir(dir)) != NULL)
        image_count++;

    closedir(dir);
    return image_count;
}

static void lv_menu_rotate(lv_display_rotation_t rotation)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        lv_obj_add_style(mgr->menu.base, &mgr->menu.rotate_style, 0);
    else
        lv_obj_add_style(mgr->menu.base, &mgr->menu.normal_style, 0);

    lv_bg_dark_config(rotation);
}

static void lv_logo_image_rotate(lv_display_rotation_t rotation)
{
#ifdef LV_USB_OSD_LOGO_IMAGE
    lv_settings_mgr_t *mgr = lv_settings_get_manager();
    char logo_image_src[256];
    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_ROTATE_IMAGE);
    else
        snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_IMAGE);

    lv_img_set_src(mgr->images.logo, logo_image_src);
#else
    (void)rotation;
#endif
}

static void usb_osd_ui_rotate_callback(lv_timer_t *tmr)
{
    lv_display_rotation_t origin_rotation = lv_display_get_rotation(NULL);
    lv_settings_mgr_t *mgr = lv_settings_get_manager();
    struct osd_settings_context *ctx = &mgr->ctx;

    if (origin_rotation != osd_rotation) {
        lv_display_set_rotation(NULL, osd_rotation);

        lv_menu_rotate(osd_rotation);
        lv_logo_image_rotate(osd_rotation);

        mgr->pic.image_count = lv_calculate_pictures_count();

        if (osd_rotation == LV_DISPLAY_ROTATION_90 || osd_rotation == LV_DISPLAY_ROTATION_270)
            lv_img_set_src(mgr->images.pictures, LVGL_PATH(pic_rotate/image0.jpg));
        else
            lv_img_set_src(mgr->images.pictures, LVGL_PATH(pic_normal/image0.jpg));

        cJSON_SetIntValue(ctx->rotation.cjson, osd_rotation * 90);
    }

}

static void lv_settings_rotate(struct osd_settings_context *ctx)
{
    osd_rotation = ctx->rotation.value / 90;

    lv_display_set_rotation(NULL, osd_rotation);
}

void lv_settings_disp_logo(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    lv_obj_clear_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);
}

bool lv_get_logo_state(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    return !lv_obj_has_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
}

void lv_settings_disp_pic(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    lv_obj_clear_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);
}

void lv_settings_changed_state(bool usb_suepend)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    if (lv_obj_has_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);

        lvgl_get_tp();
    } else {
        lv_obj_add_flag(mgr->menu.base, LV_OBJ_FLAG_HIDDEN);

        if (!usb_suepend)
            lvgl_put_tp();
    }

    if (!usb_suepend) {
        lv_obj_add_flag(mgr->images.pictures, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mgr->images.logo, LV_OBJ_FLAG_HIDDEN);
    }
}

unsigned int lv_get_screen_lock_mode(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    return mgr->lock.new_mode;
}
unsigned int lv_get_screen_lock_time_ms(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    return mgr->lock.lock_time_ms;
}
unsigned int lv_get_screen_blank_time_ms(void)
{
    lv_settings_mgr_t *mgr = lv_settings_get_manager();

    return mgr->lock.blank_time_ms;
}

static void lv_pictures_init(struct osd_settings_manager *mgr, lv_obj_t * obj)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        lv_img_set_src(obj, LVGL_PATH(pic_rotate/image0.jpg));
    else
        lv_img_set_src(obj, LVGL_PATH(pic_normal/image0.jpg));

    lv_obj_set_pos(obj, 0, 0);

    mgr->pic.image_index = 0;

    mgr->pic.image_count = lv_calculate_pictures_count();
    if (mgr->pic.image_count > 0)
        lv_timer_create(usb_osd_pic_callback, 5000, mgr);
}

static struct osd_settings_manager * lv_settings_manager_init(lv_obj_t * parent)
{
    struct osd_settings_manager *mgr = &g_settings_mgr;

    mgr->fb = mpp_fb_open();
    mgr->screen = parent;

    mgr->images.background = lv_img_create(parent);
    mgr->images.logo = lv_img_create(parent);
    mgr->images.pictures = lv_img_create(parent);

    lv_pictures_init(mgr, mgr->images.pictures);
    parent->user_data = mgr;

    return mgr;
}

lv_obj_t * lv_settings_screen_create(void)
{
    lv_obj_t * settings_screen = lv_obj_create(NULL);

    struct osd_settings_manager *mgr = lv_settings_manager_init(settings_screen);
    struct osd_settings_context *ctx = &mgr->ctx;

    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(settings_screen, settings_screen_event_cb, LV_EVENT_ALL, mgr);

    lv_parse_config_file(USB_OSD_CONFIG_FILE, ctx);
    lv_settings_rotate(ctx);
    lv_bg_dark_init(settings_screen);
    lv_logo_image_create(settings_screen);

    lv_settings_menu_init(settings_screen, mgr);

    lv_menu_rotate(osd_rotation);
    lv_timer_create(usb_osd_ui_rotate_callback, 20, mgr);

    return settings_screen;
}
