/*
 * Copyright (C) 2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui Mai <huahui.mai@artinchip.com>
 */

#include "usb_osd_video.h"
#include "usb_osd_ui.h"
#include "mpp_fb.h"

#include "media_list.h"
#include "aic_lv_ffmpeg.h"

#define ALIGN_EVEN(x)   ((x) & (~1))

static lv_obj_t *progress = NULL;
static lv_obj_t *pass_time = NULL;
static lv_obj_t *duration = NULL;

static lv_obj_t *play_control_next = NULL;
static lv_obj_t *play_control_last = NULL;
static lv_obj_t *play_control = NULL;

static lv_obj_t *control_bg = NULL;

static lv_timer_t *play_time_timer = NULL;

#define GET_MINUTES(ms) (((ms / 1000) % (60 * 60)) / 60)
#define GET_SECONDS(ms) (((ms / 1000) % 60))

#define PLAY_MODE_CIRCLE     0
#define PLAY_MODE_RANDOM     1
#define PLAY_MODE_CIRCLE_ONE 2

typedef struct _easy_player {
    lv_obj_t *ffmpeg;
    media_list_t *list;
    media_info_t cur_info;
    int cur_pos;
    int mode;
    int play_status;
} easy_player_t;

static easy_player_t player;

void lv_video_play(void)
{
    lv_ffmpeg_player_set_src(player.ffmpeg, player.cur_info.source);
    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);
    lv_ffmpeg_player_set_auto_restart(player.ffmpeg, true);
    lv_timer_resume(play_time_timer);

    lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
    lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
}

void lv_video_stop(void)
{
    printf("video stop \n");
    lv_timer_pause(play_time_timer);
    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_STOP, NULL);
    lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/play.png), NULL);
    lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/play.png), NULL);
}

static void lv_bg_dark_set(lv_obj_t * parent)
{
    struct mpp_fb * fb = mpp_fb_open();
    struct aicfb_screeninfo info;
    char bg_dark[256];

    mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &info);
    snprintf(bg_dark, 255, "L:/%dx%d_%d_%08x.fake",\
                 info.width, info.height, 0, 0x00000000);

    lv_obj_t * img_bg = lv_img_create(parent);
    lv_img_set_src(img_bg, bg_dark);
    lv_obj_set_pos(img_bg, 0, 0);
    mpp_fb_close(fb);
}

static void lv_get_hv_timing(unsigned int *hdisplay, unsigned int *vdisplay)
{
    struct mpp_fb * fb = mpp_fb_open();
    struct aicfb_screeninfo info;

    mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &info);

    *hdisplay = info.width;
    *vdisplay = info.height;

    mpp_fb_close(fb);
}

static bool long_press_end = 0;

void video_screen_switched(bool e)
{
    long_press_end = e;
}

bool is_video_screen_switched(void)
{
    return long_press_end;
}

static void show_control_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    static int show = 0;

    if (code == LV_EVENT_LONG_PRESSED) {
        back_settings_screen();
        /*
         * Normally, a LV_EVENT_CLICKED event will be generated after the
         * LV_EVENT_LONG_PRESSED event ends, and we need to filter it out.
         */
        video_screen_switched(true);
    }

    if (code == LV_EVENT_CLICKED) {
        show = show == 0 ? 1 : 0;

        if (show)
            lv_obj_clear_flag(control_bg, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(control_bg, LV_OBJ_FLAG_HIDDEN);
    }
}

static inline unsigned int control_bg_calc_height(unsigned int height)
{
    unsigned int h = ALIGN_EVEN(height / 6);

    return h > 68 ? h: 68;
}

static inline unsigned int progress_calc_width(unsigned int width)
{
    return ALIGN_EVEN((width / 10) * 8);
}

static inline unsigned int progress_calc_xpos(unsigned int width)
{
    return ALIGN_EVEN(width / 10);
}

static void get_media_info_from_src(const char *src_path, media_info_t *info)
{
    struct av_media_info av_media;

    lv_ffmpeg_player_set_src(player.ffmpeg, src_path);
    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_GET_MEDIA_INFO_EX, &av_media);

    info->duration_ms = av_media.duration / 1000;
    info->file_size_bytes = av_media.file_size;
    if (av_media.has_video)
        info->type = MEDIA_TYPE_VIDEO;
    else
        info->type = MEDIA_TYPE_AUDIO;
}

static void media_list_init()
{
    media_info_t info;

    strcpy(info.name, "Known 1");
    strcpy(info.source, LVGL_PATH_ORI(video/cartoon.mp4));
    get_media_info_from_src((const char *)info.source, &info);
    media_list_add_info(player.list, &info);

    strcpy(info.name, "Known 2");
    strcpy(info.source,  LVGL_PATH_ORI(video/cartoon.mp4));
    get_media_info_from_src((const char *)info.source, &info);
    media_list_add_info(player.list, &info);
}

static void play_control_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        if (player.play_status == 0) {
            lv_timer_resume(play_time_timer);
            lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_RESUME, NULL);
            lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
            lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
        } else {
            lv_timer_pause(play_time_timer);
            lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_PAUSE, NULL);
            lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/play.png), NULL);
            lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/play.png), NULL);
        }
        player.play_status = player.play_status == 1 ? 0 : 1;
    }
}

static void updata_ui_info(char *last_src)
{
    u64 seek_time = 0;
    media_list_get_info(player.list, &player.cur_info, player.cur_pos);
    lv_bar_set_range(progress, 0, player.cur_info.duration_ms);
    lv_bar_set_value(progress, 0, LV_ANIM_OFF);
    lv_label_set_text_fmt(pass_time, "%02d:%02d", 0 ,0);
    lv_label_set_text_fmt(duration, "%02ld:%02ld", GET_MINUTES(player.cur_info.duration_ms), GET_SECONDS(player.cur_info.duration_ms));

    if (strcmp(last_src, player.cur_info.source) == 0)
        lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_SET_PLAY_TIME_EX, &seek_time);
    else
        lv_ffmpeg_player_set_src(player.ffmpeg, player.cur_info.source);
}

static void play_next_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int info_num = media_list_get_info_num(player.list);
    char last_src[256] = {0};

    if (code == LV_EVENT_CLICKED) {
        player.cur_pos++;
        strcpy(last_src, player.cur_info.source);
        if (player.cur_pos >= info_num)
            player.cur_pos = 0;
        updata_ui_info(last_src);

        lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
        lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
        player.play_status = 1;
    }
}

static void play_last_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int info_num = media_list_get_info_num(player.list);
    char last_src[256] = {0};

    if (code == LV_EVENT_CLICKED) {
        player.cur_pos--;
        strcpy(last_src, player.cur_info.source);

        if (player.cur_pos < 0)
            player.cur_pos = info_num - 1;
        updata_ui_info(last_src);

        lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
        lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
        player.play_status = 1;
    }
}

static void play_time_update_cb(lv_timer_t * timer)
{
    long escap_time = 0;
    bool play_end = false;

    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_PLAY_END_EX, &play_end);
    if (play_end == true && (player.mode != PLAY_MODE_CIRCLE_ONE)) {
#if LVGL_VERSION_MAJOR == 8
        lv_event_send(play_control_next, LV_EVENT_CLICKED, NULL); /* auto play next */
#else
        lv_obj_send_event(play_control_next, LV_EVENT_CLICKED, NULL);
#endif
        return;
    }

    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_GET_PLAY_TIME_EX, &escap_time);

    escap_time = escap_time / 1000;
    lv_bar_set_value(progress, escap_time, LV_ANIM_ON);
    lv_label_set_text_fmt(pass_time, "%02ld:%02ld", GET_MINUTES(escap_time), GET_SECONDS(escap_time));
}

lv_obj_t * lv_video_screen_creat(void)
{
    lv_obj_t * video_src = lv_obj_create(NULL);
    lv_obj_clear_flag(video_src, LV_OBJ_FLAG_SCROLLABLE);

    player.ffmpeg = lv_ffmpeg_player_create(video_src);
    player.list = media_list_create();
    media_list_init();
    media_list_get_info(player.list, &player.cur_info, MEDIA_TYPE_POS_OLDEST);
    player.mode = PLAY_MODE_CIRCLE;
    player.play_status = 1;
    lv_obj_add_flag(player.ffmpeg, LV_OBJ_FLAG_HIDDEN);

    lv_bg_dark_set(video_src);
    lv_obj_add_event_cb(video_src, show_control_cb, LV_EVENT_ALL, control_bg);

    unsigned int hdisplay = 0;
    unsigned int vdisplay = 0;
    lv_get_hv_timing(&hdisplay, &vdisplay);

    unsigned int control_bg_h = control_bg_calc_height(vdisplay);
    unsigned int progress_x = progress_calc_xpos(hdisplay);
    unsigned int progress_w = progress_calc_width(hdisplay);
    unsigned int duration_x = progress_w + progress_x;

    control_bg = lv_obj_create(video_src);
    lv_obj_remove_style_all(control_bg);
    lv_obj_set_style_bg_color(control_bg, lv_color_hex(0x0), 0);
    lv_obj_set_style_bg_opa(control_bg, LV_OPA_100, 0);
    lv_obj_set_size(control_bg, hdisplay, control_bg_h);
    lv_obj_set_pos(control_bg, 0, vdisplay - control_bg_h);
    lv_obj_add_flag(control_bg, LV_OBJ_FLAG_HIDDEN);

    progress = lv_bar_create(control_bg);
    lv_obj_set_pos(progress, progress_x, 10);
    lv_bar_set_range(progress, 0, player.cur_info.duration_ms);
    lv_bar_set_value(progress, 0, LV_ANIM_OFF);
    lv_obj_set_size(progress, progress_w, 8);
    lv_obj_set_style_width(progress, progress_w, LV_PART_INDICATOR);
    lv_obj_set_style_height(progress, 8, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress, lv_color_hex(0x8f8f8f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress, lv_color_hex(0xffffff), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress, LV_OPA_100, LV_PART_INDICATOR);

    pass_time = lv_label_create(control_bg);
    lv_label_set_text_fmt(pass_time, "%02d:%02d", 0, 0);
    lv_obj_set_pos(pass_time, progress_x, 30);
    lv_obj_set_style_text_color(pass_time, lv_color_hex(0xffffff), LV_PART_MAIN);

    duration = lv_label_create(control_bg);
    lv_label_set_text_fmt(duration, "%02ld:%02ld", GET_MINUTES(player.cur_info.duration_ms), GET_SECONDS(player.cur_info.duration_ms));
    lv_obj_set_pos(duration, duration_x, 30);
    lv_obj_set_style_text_color(duration, lv_color_hex(0xffffff), LV_PART_MAIN);

    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_translate_x(&style_pr, 2);
    lv_style_set_translate_y(&style_pr, 2);

    play_control_next = lv_imgbtn_create(control_bg);
    lv_imgbtn_set_src(play_control_next, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/next.png), NULL);
    lv_imgbtn_set_src(play_control_next, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/next.png), NULL);
    lv_obj_set_pos(play_control_next, (hdisplay >> 1) + (hdisplay / 10), 30);
    lv_obj_set_size(play_control_next, 48, 48);
    lv_obj_add_style(play_control_next, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_event_cb(play_control_next, play_next_cb, LV_EVENT_ALL, NULL);

    play_control_last = lv_imgbtn_create(control_bg);
    lv_imgbtn_set_src(play_control_last, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/last.png), NULL);
    lv_imgbtn_set_src(play_control_last, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/last.png), NULL);
    lv_obj_set_pos(play_control_last, (hdisplay >> 1) - (hdisplay / 10 + 48), 30);
    lv_obj_set_size(play_control_last, 48, 48);
    lv_obj_add_style(play_control_last, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_event_cb(play_control_last, play_last_cb, LV_EVENT_ALL, NULL);

    play_control = lv_imgbtn_create(control_bg);
    lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_RELEASED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
    lv_imgbtn_set_src(play_control, LV_IMGBTN_STATE_PRESSED , NULL, LVGL_PATH(video_icon/pause.png), NULL);
    lv_obj_set_pos(play_control, (hdisplay >> 1) - 24, 30);
    lv_obj_set_size(play_control, 48, 48);
    lv_obj_add_style(play_control, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_event_cb(play_control, play_control_cb, LV_EVENT_ALL, NULL);

    play_time_timer = lv_timer_create(play_time_update_cb, 400 , NULL);
    lv_timer_pause(play_time_timer);

    return video_src;
}
