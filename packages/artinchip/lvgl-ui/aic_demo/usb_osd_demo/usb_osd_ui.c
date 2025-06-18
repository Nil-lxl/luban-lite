/*
 * Copyright (C) 2024-2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "usb_osd_ui.h"
#include "video/usb_osd_video.h"
#include "aic_lv_ffmpeg.h"
#include "lv_tpc_run.h"

//#define USB_OSD_LOG
#ifdef USB_OSD_LOG
#define USB_OSD_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define USB_OSD_DEBUG(fmt, ...)
#endif

static lv_osd_manager_t* instance = NULL;

#ifdef LV_USB_OSD_LOGO_TYPE_VIDEO
static lv_obj_t *ffmpeg = NULL;
static lv_timer_t *play_end_timer = NULL;
#endif

static lv_osd_manager_t* usb_osd_get_ui_manager()
{
    return instance;
}

static void blank_timer_cb(lv_timer_t *tmr)
{
    lv_osd_manager_t* mgr = tmr->user_data;

    if (!lv_get_logo_state())
        /* the status of the logo screen has been changed */
        return;

    if (mgr->usb_suspend) {
        mgr->screen_blank = true;
        lvgl_put_tp();
        mpp_fb_ioctl(mgr->fb, AICFB_POWEROFF, 0);
        rt_sem_take(mgr->osd_sem, RT_WAITING_FOREVER);
    }
}

void handle_logo_state(lv_osd_manager_t* mgr)
{
    lv_obj_t * screen = mgr->screens[SETTINGS_SCREEN];

    if (!mgr->usb_suspend || !(mgr->new_state & STATE_LOGO))
        return;

    USB_OSD_DEBUG("%s\n", __func__);

    lv_scr_load(screen);
    lv_settings_disp_logo();

    unsigned int blank_time = lv_get_screen_blank_time_ms();
    if (blank_time > 0) {
        lv_timer_t* timer = lv_timer_create(blank_timer_cb, blank_time, mgr);
        lv_timer_set_repeat_count(timer, 1);
    }

    if (mgr->usb_suspend) {
        mgr->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
        mgr->adjusted_alpha_value = 255;
    } else {
        mgr->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
        mgr->adjusted_alpha_value = 0;
    }

}

void handle_usb_resume_state(lv_osd_manager_t* mgr)
{
    (void)mgr;
    USB_OSD_DEBUG("%s\n", __func__);
}

void handle_pictures_state(lv_osd_manager_t* mgr)
{
    lv_obj_t * screen = mgr->screens[SETTINGS_SCREEN];

    if (!mgr->usb_suspend || !(mgr->new_state & STATE_PICTURES))
        return;

    lv_scr_load(screen);

    lv_settings_disp_pic();

    if (mgr->usb_suspend) {
        mgr->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
        mgr->adjusted_alpha_value = 255;
    } else {
        mgr->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
        mgr->adjusted_alpha_value = 0;
    }
}

void handle_settings_state(lv_osd_manager_t* mgr)
{
    lv_obj_t * screen = mgr->screens[SETTINGS_SCREEN];
    bool usb_suspend = mgr->new_state & STATE_USB_SUSPEND;

    USB_OSD_DEBUG("%s\n", __func__);

    lv_scr_load(screen);

    lv_settings_changed_state(usb_suspend);

    if (usb_suspend) {
        if (mgr->new_state & STATE_VIDEO) {
            mgr->adjusted_alpha_mode = AICFB_PIXEL_ALPHA_MODE;
            mgr->adjusted_alpha_value = 0;
        } else {
            mgr->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
            mgr->adjusted_alpha_value = 255;
        }
    } else {
        mgr->adjusted_alpha_mode = AICFB_MIXDER_ALPHA_MODE;
        mgr->adjusted_alpha_value = 220;
    }
}

void handle_blank_state(lv_osd_manager_t* mgr)
{
    mgr->screen_blank = true;
    USB_OSD_DEBUG("%s\n", __func__);
}

void handle_video_state(lv_osd_manager_t* mgr)
{
    if (!mgr->usb_suspend || (mgr->current_state & STATE_VIDEO))
        return;

    lv_obj_t * screen = mgr->screens[VIDEO_SCREEN];
    lv_scr_load(screen);
    lv_video_play();

    USB_OSD_DEBUG("%s\n", __func__);

    mgr->adjusted_alpha_mode = AICFB_PIXEL_ALPHA_MODE;
    mgr->adjusted_alpha_value = 0;
}

static const struct state_info station[] = {
    {
        .osd_state = STATE_USB_RESUME,
        .handler = handle_usb_resume_state,
    },
    {
        .osd_state = STATE_LOGO,
        .handler = handle_logo_state,
    },
    {
        .osd_state = STATE_PICTURES,
        .handler = handle_pictures_state,
    },
    {
        .osd_state = STATE_VIDEO,
        .handler = handle_video_state,
    },
    {
        .osd_state = STATE_SETTINGS,
        .handler = handle_settings_state,
    },
    {
        .osd_state = STATE_BLANK,
        .handler = handle_blank_state,
    },
};

static bool is_usb_osd_state_modify(lv_osd_manager_t* mgr)
{
    if (mgr->current_state != mgr->new_state)
        return true;

    if (mgr->current_alpha_mode != mgr->adjusted_alpha_mode ||
        mgr->current_alpha_value != mgr->adjusted_alpha_value)
        return true;

    if (mgr->screen_blank)
        return true;

    return false;
}

static void usb_osd_set_alpha_mode(struct mpp_fb* fb, unsigned int mode, unsigned int value)
{
    struct aicfb_alpha_config alpha = {0};
    int ret;

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = mode;
    alpha.value = value;
    ret = mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        printf("set alpha failed\n");
}


static void usb_osd_ui_callback(lv_timer_t* tmr)
{
    lv_osd_manager_t* mgr = (lv_osd_manager_t*)tmr->user_data;
    unsigned int mode, value;

    if (!is_usb_osd_state_modify(mgr))
        return;

    if (mgr->screen_blank) {
        mpp_fb_ioctl(mgr->fb, AICFB_POWEROFF, 0);
        rt_sem_take(mgr->osd_sem, RT_WAITING_FOREVER);
    }

    USB_OSD_DEBUG("current_state(%d) switch new state(%d)\n",
                  mgr->current_state, mgr->new_state);
    USB_OSD_DEBUG("current alpha mode(%d), value(%d)\n",
                  mgr->current_alpha_mode, mgr->current_alpha_value);

    if (mgr->current_state != mgr->new_state) {
        unsigned int state = mgr->current_state ^ mgr->new_state;

        for (int i = 0; i < ARRAY_SIZE(station); i++) {
            if (state & station[i].osd_state) {
                station[i].handler(mgr);
            }
        }

        mgr->current_state = mgr->new_state;
    } else {
        mode = mgr->adjusted_alpha_mode;
        value = mgr->adjusted_alpha_value;

        usb_osd_set_alpha_mode(mgr->fb, mode, value);
        mgr->current_alpha_mode = mode;
        mgr->current_alpha_value = value;
    }
}

static void usb_osd_ui_theme_set(void)
{
    lv_disp_t * disp = lv_disp_get_default();

    lv_theme_t * th = lv_theme_default_init(disp,
                                            lv_palette_main(LV_PALETTE_BLUE),
                                            lv_palette_main(LV_PALETTE_RED),
                                            true,   /* dark theme */
                                            LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);
}

#ifdef LV_USB_OSD_SETTINGS_MENU
static void gpio_input_irq_handler(void *args)
{
    struct osd_manager *mgr = usb_osd_get_ui_manager();

    if (mgr->current_state & STATE_SETTINGS)
        mgr->new_state &= ~(STATE_SETTINGS);
    else
        mgr->new_state |= STATE_SETTINGS;

    USB_OSD_DEBUG("%s\n", __func__);
}
#endif

static void usb_osd_wakeup_key_register(void)
{
#ifdef LV_USB_OSD_SETTINGS_MENU
    u32 pin;

    /* 1.get pin number */
    pin = rt_pin_get(USB_OSD_WAKEUP_KEY);

    /* 2.set pin mode to Input-PullUp */
    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    /* 3.attach irq handler */
    rt_pin_attach_irq(pin, PIN_IRQ_MODE_FALLING,
                      gpio_input_irq_handler, &pin);

    /* 4.enable pin irq */
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);
#endif
}

static int usb_osd_mode_to_state(unsigned int lock_mode)
{
    unsigned int state = 0;

    if (lock_mode == DISPLAY_LOGO)
        state |= STATE_LOGO;
    else if (lock_mode == DISPLAY_PICTURES)
        state |= STATE_PICTURES;
    else if (lock_mode == DISPLAY_VIDEO)
        state |= STATE_VIDEO;
    else if (lock_mode == BLANK_SCREEN)
        state |= STATE_BLANK;

    USB_OSD_DEBUG("osd lock mode: %d\n", lock_mode);

    return state;
}

void usb_disp_suspend_enter(void)
{
    struct osd_manager *mgr = usb_osd_get_ui_manager();
    unsigned int mode = lv_get_screen_lock_mode();

    if (lv_get_screen_lock_time_ms() == 0)
        /* Never Lock Screen */
        return;

    mgr->new_state = STATE_USB_SUSPEND | usb_osd_mode_to_state(mode);
    mgr->usb_suspend = true;

    if (mgr->new_state & STATE_BLANK)
        mgr->screen_blank = true;

    USB_OSD_DEBUG("usb suspend, osd new_state: %d\n", mgr->new_state);

    lvgl_get_tp();
}

void usb_disp_suspend_exit(void)
{
    struct osd_manager *mgr = usb_osd_get_ui_manager();

    lvgl_put_tp();

    usb_osd_set_alpha_mode(mgr->fb, AICFB_GLOBAL_ALPHA_MODE, 0);
    mpp_fb_ioctl(mgr->fb, AICFB_POWERON, 0);

    if (mgr->screen_blank) {
        rt_sem_release(mgr->osd_sem);
        mgr->screen_blank = false;
    }

    if (mgr->current_state & STATE_VIDEO)
        lv_video_stop();

    mgr->new_state = STATE_USB_RESUME;
    mgr->current_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
    mgr->current_alpha_value = 0;
    mgr->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
    mgr->adjusted_alpha_value = 0;
    mgr->usb_suspend = false;
    mgr->screen_blank = false;
    USB_OSD_DEBUG("%s\n", __func__);
}

int usb_disp_get_suspend_ms(void)
{
    int suspend_ms = lv_get_screen_lock_time_ms();

    USB_OSD_DEBUG("usb disp suspend ms: %d\n", suspend_ms);
    return suspend_ms;
}

#ifdef LV_USB_OSD_LOGO_VIDEO
static void ffmpeg_player_update_cb(lv_timer_t * timer)
{
    bool play_end = false;
    if (!ffmpeg)
        return;

    lv_ffmpeg_player_set_cmd_ex(ffmpeg, LV_FFMPEG_PLAYER_CMD_PLAY_END_EX, &play_end);
    if (play_end == true) {
        USB_OSD_DEBUG("usb disp delete ffmpeg obj\n");
        lv_obj_del(ffmpeg);
        ffmpeg = NULL;
        lv_timer_pause(play_end_timer);
        lv_timer_del(play_end_timer);
        play_end_timer = NULL;
    }
}
#endif

static lv_obj_t* create_screen(const char* type)
{
    if (rt_strcmp(type, "video") == 0)
        return lv_video_screen_create();
    else if (rt_strcmp(type, "settings") == 0)
        return lv_settings_screen_create();

    return NULL;
}

bool lv_get_ui_settings_state(void)
{
    lv_osd_manager_t* mgr = usb_osd_get_ui_manager();

    return mgr->usb_suspend;
}

void lv_update_ui_settings_state(void)
{
    lv_osd_manager_t* mgr = usb_osd_get_ui_manager();

    if (mgr->current_state & STATE_SETTINGS)
        mgr->current_state &= ~(STATE_SETTINGS);

    if (mgr->new_state & STATE_SETTINGS)
        mgr->new_state &= ~(STATE_SETTINGS);
}

void lv_blank_screen_enable(bool enable)
{
    lv_osd_manager_t* mgr = usb_osd_get_ui_manager();

    if (enable)
        mgr->screen_blank = true;
    else
        mgr->screen_blank = false;
}

bool lv_is_usb_disp_suspend(void)
{
    lv_osd_manager_t* mgr = usb_osd_get_ui_manager();

    return mgr->usb_suspend;
}

static lv_osd_manager_t* ui_manager_get_instance(void)
{
    instance = rt_malloc(sizeof(lv_osd_manager_t));
    rt_memset(instance, 0, sizeof(lv_osd_manager_t));

    instance->fb = mpp_fb_open();
    instance->osd_sem = rt_sem_create("osdsem", 0, RT_IPC_FLAG_PRIO);

    instance->screens[VIDEO_SCREEN] = create_screen("video");
    instance->screens[SETTINGS_SCREEN] = create_screen("settings");

    instance->current_state = STATE_USB_RESUME;
    instance->current_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
    instance->current_alpha_value = 0;
    instance->adjusted_alpha_mode = AICFB_GLOBAL_ALPHA_MODE;
    instance->adjusted_alpha_value = 0;

    instance->screen_blank = false;

    return instance;
}

static void usb_osd_ui_display_logo(void)
{
    lv_display_rotation_t rotation = lv_display_get_rotation(NULL);
    lv_obj_t * logo_screen = lv_scr_act();
#if defined(LV_USB_OSD_LOGO_IMAGE)
    char logo_image_src[64];

    if (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270)
        snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_ROTATE_IMAGE);
    else
        snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_IMAGE);

    lv_obj_t * logo_img = lv_img_create(logo_screen);
    lv_img_set_src(logo_img, logo_image_src);
    lv_obj_set_pos(logo_img, 0, 0);
#elif defined(LV_USB_OSD_LOGO_VIDEO)
    char logo_video_src[64];
    snprintf(logo_video_src, sizeof(logo_video_src), "%s%s",
            USB_OSD_UI_LOGO_VIDEO_PATH, USB_OSD_UI_LOGO_VIDEO);

#if defined(LV_DISPLAY_ROTATE_EN)
    unsigned int lvgl_rotation[] = {LV_DISPLAY_ROTATION_0,
                                    LV_DISPLAY_ROTATION_270,
                                    LV_DISPLAY_ROTATION_180,
                                    LV_DISPLAY_ROTATION_90};
    rotation = lvgl_rotation[rotation] * 90;
#elif defined(AIC_FB_ROTATE_EN)
    rotation = AIC_FB_ROTATE_DEGREE;
#endif
    ffmpeg = lv_ffmpeg_player_create(logo_screen);
    lv_obj_set_style_transform_angle(ffmpeg, rotation, LV_PART_MAIN);
    lv_obj_add_flag(ffmpeg, LV_OBJ_FLAG_HIDDEN);

    lv_ffmpeg_player_set_src(ffmpeg, logo_video_src);
    lv_ffmpeg_player_set_cmd_ex(ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);
    lv_ffmpeg_player_set_auto_restart(ffmpeg, false);

    play_end_timer = lv_timer_create(ffmpeg_player_update_cb, 50 , NULL);
    lv_timer_resume(play_end_timer);
#endif
}

void usb_osd_ui_init(void)
{
    usb_osd_ui_theme_set();

    lv_osd_manager_t* mgr = ui_manager_get_instance();
    usb_osd_set_alpha_mode(mgr->fb, AICFB_GLOBAL_ALPHA_MODE, 0);
    usb_osd_wakeup_key_register();

    usb_osd_ui_display_logo();
    lv_timer_create(usb_osd_ui_callback, 200, mgr);
}

void ui_init(void)
{
    usb_osd_ui_init();
}
