/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "mpp_fb.h"
#include "usb_osd_ui.h"
#include "usb_osd_video.h"
#include "usb_osd_settings.h"
#include "aic_lv_ffmpeg.h"
#include "lv_tpc_run.h"

//#define USB_OSD_LOG
#ifdef USB_OSD_LOG
#define USB_OSD_DEBUG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define USB_OSD_DEBUG(fmt, ...)
#endif

#define DISPLAY_LOGO_SCREEN        (0x1 << 0)
#define DISPLAY_PICTURES_SCREEN    (0x1 << 1)
#define DISPLAY_VIDEO_SCREEN       (0x1 << 2)
#define DISPLAY_SETTINGS_SCREEN    (0x1 << 3)
#define DISPLAY_GLOBAL_ALPHA_LATER (0x1 << 4)
#define DISPLAY_MIXDER_ALPHA_LATER (0x1 << 5)
#define DISPLAY_PIXEL_ALPHA_LATER  (0x1 << 6)
#define DISPLAY_VIDEO_SUSPEND      (0x1 << 7)

static lv_obj_t * video_screen = NULL;
static lv_obj_t * settings_screen = NULL;
static lv_obj_t * logo_screen = NULL;

static struct mpp_fb *g_fb = NULL;
static unsigned int disp_osd_flags = 0;
static bool screen_blank = false;
static rt_sem_t osd_sem = NULL;

#ifdef LV_USB_OSD_LOGO_TYPE_VIDEO
static lv_obj_t *ffmpeg = NULL;
#endif

#define ClrBit(reg, bit)         ((reg) &= ~(bit))
#define SetBit(reg, bit)         ((reg) |= (bit))

#define usb_osd_add_flag(bit, flags) SetBit(flags, bit)
#define usb_osd_clear_flag(bit, flags)  ClrBit(flags, bit)

static inline bool is_usb_osd_has_flag(unsigned int nr, unsigned int flags)
{
    return nr & flags ? true : false;
}

static void usb_osd_open_mpp_fb()
{
    g_fb = mpp_fb_open();
}

bool is_usb_disp_suspend(void)
{
    return is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags);
}

void usb_osd_modify_blank_status(bool enable)
{
    if (enable)
        screen_blank = true;
    else
        screen_blank = false;
}

bool is_usb_osd_screen_blank(void)
{
    return screen_blank;
}

void back_video_screen(void)
{
    usb_osd_add_flag(DISPLAY_VIDEO_SCREEN, disp_osd_flags);
}

void back_settings_screen(void)
{
    usb_osd_add_flag(DISPLAY_SETTINGS_SCREEN, disp_osd_flags);
}

void lv_load_video_screen(void)
{
    lv_scr_load(video_screen);
}

static void usb_osd_blank_screen_callback(lv_timer_t *tmr)
{
    if (!lv_logo_screen_is_enabled())
        /* the status of the logo screen has been changed */
        return;

    USB_OSD_DEBUG("%s, %d\n", __func__, __LINE__);

    if (is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags)) {
        usb_osd_modify_blank_status(true);
        mpp_fb_ioctl(g_fb, AICFB_POWEROFF, 0);
        lvgl_put_tp();
        rt_sem_take(osd_sem, RT_WAITING_FOREVER);
    }
}

/*
 * usb_osd_ui_callback()     - usb osd ui timer callback
 *
 * This timer monitor the status of disp_osd_flags and switchs screens. The disp_osd_flags
 * will be modified by usb_disp_suspend_enter() and usb_disp_suspend_exit() first.
 *
 * To ensure that no tears occur when the UI switch screen, this timer callback will be
 * called twice when switching screens.
 *
 * The steps to switch logo screen are as follows:
 *
 *                     usb set logo screen flag
 *
 *                       |
 *                       |
 *                       |
 *                       v
 * ----------------------------------------------------------------------------------------->
 *     ^                           ^                              ^                             lvgl timer
 *     |                           |                              |                             Cycle 200 milliseconds
 *     |                           |                              |
 *     |                           |                              |
 *
 *   this cycle do nothing      lvgl switch to logo screen      enable ui global alpha mode
 *                              set global alpha flag
 */
static void usb_osd_ui_callback(lv_timer_t *tmr)
{
    struct aicfb_alpha_config alpha = {0};
    lv_obj_t * screen = lv_scr_act();

    if (is_usb_osd_screen_blank()) {
        mpp_fb_ioctl(g_fb, AICFB_POWEROFF, 0);
        rt_sem_take(osd_sem, RT_WAITING_FOREVER);
    }

    /*
     * Make sure that only the settings menu can displayed when the
     * usb display thread is working
     */
    if (!is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags)) {

        lv_logo_screen_enable(false);
        lv_pictures_screen_enable(false);
    }

    /* There are no events to respond to, just return */
    if ((disp_osd_flags & 0x7F) == 0x0)
        return;

    if ((screen != settings_screen) &&
                is_usb_osd_has_flag(DISPLAY_SETTINGS_SCREEN, disp_osd_flags)) {
        lv_scr_load(settings_screen);
        lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);

        lv_settings_menu_enable(true);
        lv_pictures_screen_enable(false);
        lv_logo_screen_enable(false);

        USB_OSD_DEBUG("Switch to settings screen\n");
        /* alpha blend later */
        if (is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags)) {
            if (lv_get_screen_lock_mode() == DISPLAY_VIDEO)
                usb_osd_add_flag(DISPLAY_MIXDER_ALPHA_LATER, disp_osd_flags);
            else
                usb_osd_add_flag(DISPLAY_GLOBAL_ALPHA_LATER, disp_osd_flags);
        } else {
            usb_osd_add_flag(DISPLAY_MIXDER_ALPHA_LATER, disp_osd_flags);
        }

        usb_osd_clear_flag(DISPLAY_SETTINGS_SCREEN, disp_osd_flags);
        return;
    }

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;

    if (is_usb_osd_has_flag(DISPLAY_LOGO_SCREEN, disp_osd_flags)) {
        usb_osd_clear_flag(DISPLAY_LOGO_SCREEN, disp_osd_flags);
        int blank_time = lv_get_screen_blank_tmie_ms();

        if (screen != settings_screen)
            lv_scr_load(settings_screen);

        lv_logo_screen_enable(true);
        lv_settings_menu_enable(false);
        lv_pictures_screen_enable(false);

        USB_OSD_DEBUG("Switch to LOGO screen\n");

        if (blank_time != 0) {
            lv_timer_t * blank_timer = lv_timer_create(usb_osd_blank_screen_callback, blank_time, 0);
            lv_timer_set_repeat_count(blank_timer, 1);
        }

        usb_osd_add_flag(DISPLAY_GLOBAL_ALPHA_LATER, disp_osd_flags);
        return;
    }

    if (is_usb_osd_has_flag(DISPLAY_PICTURES_SCREEN, disp_osd_flags)) {
        usb_osd_clear_flag(DISPLAY_PICTURES_SCREEN, disp_osd_flags);

        if (screen != settings_screen)
            lv_scr_load(settings_screen);

        lv_logo_screen_enable(false);
        lv_settings_menu_enable(false);
        lv_pictures_screen_enable(true);

        USB_OSD_DEBUG("Switch to pictures screen\n");

        usb_osd_add_flag(DISPLAY_GLOBAL_ALPHA_LATER, disp_osd_flags);
        return;
    }

    if (is_usb_osd_has_flag(DISPLAY_VIDEO_SCREEN, disp_osd_flags)) {
        usb_osd_clear_flag(DISPLAY_VIDEO_SCREEN, disp_osd_flags);

        lv_scr_load(video_screen);

        USB_OSD_DEBUG("Switch to video screen\n");

        usb_osd_add_flag(DISPLAY_PIXEL_ALPHA_LATER, disp_osd_flags);
        return;
    }

    /*
     * Make sure the settings screen has been drawn before configure alpha
     */
    if (is_usb_osd_has_flag(DISPLAY_MIXDER_ALPHA_LATER, disp_osd_flags)) {
        usb_osd_clear_flag(DISPLAY_MIXDER_ALPHA_LATER, disp_osd_flags);

        USB_OSD_DEBUG("enable mixder alpha mode\n");

        alpha.mode = AICFB_MIXDER_ALPHA_MODE;
        alpha.value = 220;
        mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
        lvgl_get_tp();
        return;
    }

    if (is_usb_osd_has_flag(DISPLAY_GLOBAL_ALPHA_LATER, disp_osd_flags)) {
        usb_osd_clear_flag(DISPLAY_GLOBAL_ALPHA_LATER, disp_osd_flags);

        USB_OSD_DEBUG("enable global alpha mode\n");

        alpha.mode = AICFB_GLOBAL_ALPHA_MODE;
        alpha.value = 255;
        mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
        return;
    }

    if (is_usb_osd_has_flag(DISPLAY_PIXEL_ALPHA_LATER, disp_osd_flags)) {
        usb_osd_clear_flag(DISPLAY_PIXEL_ALPHA_LATER, disp_osd_flags);

        USB_OSD_DEBUG("enable pixel alpha mode and play video\n");

        lv_video_play();

        alpha.mode = AICFB_PIXEL_ALPHA_MODE;
        alpha.value = 220;
        mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);

        return;
    }

    /* show or hide settings menu */
    if (is_usb_osd_has_flag(DISPLAY_SETTINGS_SCREEN, disp_osd_flags)) {
        if (!lv_settings_menu_is_enabled()) {
            if (!is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags)) {
                alpha.mode = AICFB_MIXDER_ALPHA_MODE;
                alpha.value = 220;
                mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
            }

            lv_settings_menu_enable(true);

            lvgl_get_tp();
            USB_OSD_DEBUG("Show settings menu\n");
        } else {
            alpha.mode = AICFB_GLOBAL_ALPHA_MODE;
            alpha.value = is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags) ? 255 : 0;
            mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);

            lv_settings_menu_enable(false);
            if (!is_usb_disp_suspend())
                lvgl_put_tp();
            USB_OSD_DEBUG("Hide settings menu\n");
        }

        usb_osd_clear_flag(DISPLAY_SETTINGS_SCREEN, disp_osd_flags);
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

static void usb_osd_ui_alpha_set(void)
{
    struct aicfb_alpha_config alpha = {0};
    int ret;

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = AICFB_GLOBAL_ALPHA_MODE;
    alpha.value = 0;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        printf("set alpha failed\n");
}

#ifdef LV_USB_OSD_SETTINGS_MENU
static void gpio_input_irq_handler(void *args)
{
    if (is_usb_osd_has_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags) &&
            is_usb_osd_screen_blank())
        return;

    USB_OSD_DEBUG("%s, %d\n", __func__, __LINE__);

    usb_osd_add_flag(DISPLAY_SETTINGS_SCREEN, disp_osd_flags);
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

void usb_disp_suspend_enter(void)
{
    int mode = lv_get_screen_lock_mode();
    int time_ms = lv_get_screen_lock_time_ms();

    usb_osd_add_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags);

    if (time_ms == 0)
        /* Never Lock Screen */
        return;

    switch (mode) {
    case DISPLAY_LOGO:
        usb_osd_add_flag(DISPLAY_LOGO_SCREEN, disp_osd_flags);
        break;
    case DISPLAY_PICTURES:
        usb_osd_add_flag(DISPLAY_PICTURES_SCREEN, disp_osd_flags);
        break;
    case DISPLAY_VIDEO:
        usb_osd_add_flag(DISPLAY_VIDEO_SCREEN, disp_osd_flags);
        break;
    case BLANK_SCREEN:
        usb_osd_modify_blank_status(true);
        lvgl_put_tp();
        break;
    default:
        printf("Unknown mode: %#x\n", mode);
        break;
    }

    USB_OSD_DEBUG("Add video suspend flag\n");

    if (mode != BLANK_SCREEN)
        lvgl_get_tp();
}

void usb_disp_suspend_exit(void)
{
    struct aicfb_alpha_config alpha = {0};

    usb_osd_clear_flag(DISPLAY_VIDEO_SUSPEND, disp_osd_flags);
    USB_OSD_DEBUG("Clear video suspend flag\n");

    lvgl_put_tp();

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = AICFB_GLOBAL_ALPHA_MODE;
    alpha.value = 0;
    mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    mpp_fb_ioctl(g_fb, AICFB_POWERON, 0);

    if (is_usb_osd_screen_blank()) {
        usb_osd_modify_blank_status(false);
        rt_sem_release(osd_sem);
    }

    if (lv_get_screen_lock_mode() == DISPLAY_VIDEO)
        lv_video_stop();
}

int usb_disp_get_suspend_ms(void)
{
    int suspend_ms = lv_get_screen_lock_time_ms();

    USB_OSD_DEBUG("usb disp suspend ms: %d\n", suspend_ms);
    return suspend_ms;
}

static void usb_osd_ui_display_logo(void)
{
#if defined(LV_USB_OSD_LOGO_IMAGE)
    char logo_image_src[64];
    snprintf(logo_image_src, sizeof(logo_image_src), LVGL_PATH(%s), USB_OSD_UI_LOGO_IMAGE);

    logo_screen = lv_scr_act();
    lv_obj_t * logo_img = lv_img_create(logo_screen);
    lv_img_set_src(logo_img, logo_image_src);
    lv_obj_set_pos(logo_img, 0, 0);
#elif defined(LV_USB_OSD_LOGO_VIDEO)
    char logo_video_src[64];
    snprintf(logo_video_src, sizeof(logo_video_src), "%s%s",
            USB_OSD_UI_LOGO_VIDEO_PATH, USB_OSD_UI_LOGO_VIDEO);

    logo_screen = lv_scr_act();
    ffmpeg = lv_ffmpeg_player_create(logo_screen);
    lv_obj_set_style_transform_angle(ffmpeg, USB_OSD_UI_LOGO_VIDEO_ROTATE, LV_PART_MAIN);
    lv_obj_add_flag(ffmpeg, LV_OBJ_FLAG_HIDDEN);

    lv_ffmpeg_player_set_src(ffmpeg, logo_video_src);
    lv_ffmpeg_player_set_cmd_ex(ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);
    lv_ffmpeg_player_set_auto_restart(ffmpeg, false);
#endif
}

void usb_osd_ui_init(void)
{
    usb_osd_open_mpp_fb();

    usb_osd_ui_theme_set();
    usb_osd_ui_alpha_set();
    usb_osd_wakeup_key_register();

    usb_osd_ui_display_logo();

    osd_sem = rt_sem_create("osdsem", 0, RT_IPC_FLAG_PRIO);
    if (osd_sem == RT_NULL) {
        pr_err("create osd semaphore failed\n");
        return;
    }

    settings_screen = lv_settings_screen_creat();
    video_screen = lv_video_screen_creat();

    lv_timer_create(usb_osd_ui_callback, 200, 0);
}

void ui_init(void)
{
    usb_osd_ui_init();
}
