/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "aic_lv_ffmpeg.h"
#include "media_list.h"
#include "aic_audio_ui.h"
#include "demo_hub.h"

#if LVGL_VERSION_MAJOR == 8
#define lv_img_header_t         lv_img_header_t
#else
#define lv_img_decoder_get_info lv_image_decoder_get_info
#define lv_img_header_t         lv_image_header_t
#define lv_img_t                lv_image_t
#endif

#define ANIMA_COVER_IN       0
#define ANIMA_COVER_OUT      1

#define PLAY_MODE_CIRCLE     0
#define PLAY_MODE_RANDOM     1
#define PLAY_MODE_CIRCLE_ONE 2

#define ANIMA_DELAY_TIME_MS  100

typedef struct _easy_player {
    lv_obj_t *ffmpeg;
    media_list_t *list;
    media_info_t cur_info;
    int cur_pos;
    int mode;
    int play_status;
} easy_player_t;

typedef struct _ui_anim_user_data_t {
    lv_obj_t *target;
    easy_player_t *player;
    int32_t val;
} ui_anim_user_data_t;

static void cover_move_anima(lv_obj_t *cover, int anima_in);
static void play_point_rotate_anima(lv_obj_t *play_point, easy_player_t *player);

static void media_list_init(easy_player_t *player);
static void aic_audio_ui_impl(lv_obj_t *parent, easy_player_t *player);

static void ui_aic_audio_cb(lv_event_t *e);
static void heart_switch_cb(lv_event_t *e);
static void play_mode_cb(lv_event_t *e);
static void play_continue_pause_cb(lv_event_t *e);
static void play_next_cb(lv_event_t *e);
static void play_last_cb(lv_event_t *e);

static void cover_rotate_cb(lv_timer_t * timer);
static void play_time_update_cb(lv_timer_t * timer);

static lv_obj_t *play_point;
static lv_obj_t *cover;
static lv_obj_t *progress;
static lv_obj_t *duration;
static lv_obj_t *name;
static lv_obj_t *album;
static lv_obj_t *author;
static lv_obj_t *introduce;
static lv_obj_t *text_file;
static lv_obj_t *play_control;
static lv_obj_t *play_control_next;

static lv_timer_t *cover_rotate_timer;
static lv_timer_t *play_time_timer;

static int cover_rotate_angle = 0;

lv_obj_t *aic_audio_ui_init(void)
{
    static easy_player_t player;
    lv_obj_t *aic_audio_ui = lv_obj_create(NULL);
    lv_obj_clear_flag(aic_audio_ui, LV_OBJ_FLAG_SCROLLABLE);

#ifdef AIC_CHIP_D12X
    return aic_audio_ui;
#endif

    player.ffmpeg = lv_ffmpeg_player_create(aic_audio_ui);

    player.list = media_list_create();

    media_list_init(&player);
    media_list_get_info(player.list, &player.cur_info, MEDIA_TYPE_POS_OLDEST);

    aic_audio_ui_impl(aic_audio_ui, &player);

    lv_ffmpeg_player_set_src(player.ffmpeg, player.cur_info.source);
    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);

    play_time_timer = lv_timer_create(play_time_update_cb, 100 , &player);
    cover_rotate_timer = lv_timer_create(cover_rotate_cb, 30 , &player);

    lv_obj_add_event_cb(aic_audio_ui, ui_aic_audio_cb, LV_EVENT_ALL, &player);

    return aic_audio_ui;
}

static void get_media_info_from_src(easy_player_t *player, const char *src_path, media_info_t *info)
{
    struct av_media_info av_media;

    lv_ffmpeg_player_set_src(player->ffmpeg, src_path);
    lv_ffmpeg_player_set_cmd_ex(player->ffmpeg, LV_FFMPEG_PLAYER_CMD_GET_MEDIA_INFO_EX, &av_media);

    info->duration_ms = av_media.duration / 1000;
    info->file_size_bytes = av_media.file_size;
    if (av_media.has_video)
        info->type = MEDIA_TYPE_VIDEO;
    else
        info->type = MEDIA_TYPE_AUDIO;
}

static void media_list_init(easy_player_t *player)
{
    media_info_t info;

    strcpy(info.name, LVGL_PATH(aic_audio/song_name_leisure_time.png));
    lv_snprintf(info.source, sizeof(info.source), "%s/aic_audio/leisure_time.mp3", LVGL_STORAGE_PATH);
    strcpy(info.author, LVGL_PATH(aic_audio/singer_name_zhaodage.png));
    strcpy(info.album, LVGL_PATH(aic_audio/album_name_none.png));
    strcpy(info.cover, LVGL_PATH(aic_audio/song_img_leisure_time.png));
    strcpy(info.text_file, LVGL_PATH(aic_audio/lyric_enjoy.png));
    strcpy(info.introduce, LVGL_PATH(aic_audio/lyric_welcome.png));

    get_media_info_from_src(player, (const char *)info.source, &info);

    media_list_add_info(player->list, &info);

    strcpy(info.name, LVGL_PATH(aic_audio/song_name_deep_and_serene.png));
    lv_snprintf(info.source, sizeof(info.source), "%s/aic_audio/deep_serene.mp3", LVGL_STORAGE_PATH);
    strcpy(info.author, LVGL_PATH(aic_audio/singer_name_zhaodage.png));
    strcpy(info.album, LVGL_PATH(aic_audio/album_name_none.png));
    strcpy(info.cover, LVGL_PATH(aic_audio/song_img_deep_serene.png));
    strcpy(info.text_file, LVGL_PATH(aic_audio/lyric_enjoy.png));
    strcpy(info.introduce, LVGL_PATH(aic_audio/lyric_welcome.png));

    get_media_info_from_src(player, (const char *)info.source, &info);
    media_list_add_info(player->list, &info);
}

static void aic_audio_ui_impl(lv_obj_t *parent, easy_player_t *player)
{
    lv_img_t *lv_img_head;
    cover = lv_img_create(parent);
    lv_img_set_src(cover, player->cur_info.cover);
    lv_obj_set_pos(cover, 36, 42);
    lv_img_head = (lv_img_t *)cover;
    lv_img_set_pivot(cover, lv_img_head->w / 2, lv_img_head->h / 2); /* set center */
    lv_img_set_angle(cover, 0);
    lv_img_set_antialias(cover, true);

    play_point = lv_img_create(parent);
    lv_obj_set_pos(play_point, 159, 30);
    lv_img_set_src(play_point, LVGL_PATH(aic_audio/play_point.png));
    lv_img_head = (lv_img_t *)play_point;
    lv_img_set_pivot(play_point, lv_img_head->w * 0.6, lv_img_head->h * 0.13);
    lv_img_set_angle(play_point, 0);
    lv_img_set_antialias(play_point, true);

    progress = lv_bar_create(parent);
    lv_obj_set_pos(progress, 0, -50);
    lv_bar_set_range(progress, 0, player->cur_info.duration_ms);
    lv_bar_set_value(progress, 0, LV_ANIM_OFF);
    lv_obj_set_size(progress, 418, 6);
    lv_obj_set_align(progress, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_style_width(progress, 418, LV_PART_INDICATOR);
    lv_obj_set_style_height(progress, 6, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress, lv_theme_get_color_emphasize(NULL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress, lv_theme_get_color_emphasize(NULL), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress, LV_OPA_100, LV_PART_INDICATOR);

    duration = lv_label_create(parent);
    lv_label_set_text_fmt(duration, "%02d:%02d", 0, 0);
    lv_obj_set_pos(duration, 410, 224);
    lv_obj_set_style_text_color(duration, lv_theme_get_color_emphasize(NULL), LV_PART_MAIN);

    lv_obj_t *heart = lv_img_create(parent);
    lv_img_set_src(heart, LVGL_PATH(aic_audio/love.png));
    lv_obj_set_pos(heart, 343, 231);
    lv_obj_t *play_mode = lv_img_create(parent);
    lv_img_set_src(play_mode, LVGL_PATH(aic_audio/circle.png));
    lv_obj_set_pos(play_mode, 130, 230);
    play_control = lv_img_create(parent);
    lv_img_set_src(play_control, LVGL_PATH(aic_audio/pause.png));
    lv_obj_set_pos(play_control, 231, 232);
    play_control_next = lv_img_create(parent);
    lv_img_set_src(play_control_next, LVGL_PATH(aic_audio/next.png));
    lv_obj_set_pos(play_control_next, 283, 239);
    lv_obj_t *play_control_last = lv_img_create(parent);
    lv_img_set_src(play_control_last, LVGL_PATH(aic_audio/last.png));
    lv_obj_set_pos(play_control_last, 182, 239);

    lv_obj_t *song_container = lv_obj_create(parent);
    lv_obj_remove_style_all(song_container);
    lv_obj_set_size(song_container, 270, 150);
    lv_obj_set_pos(song_container, 203, 20);
    name = lv_img_create(song_container);
    lv_img_set_src(name, player->cur_info.name);
    lv_obj_set_pos(name, 0, 10);
    lv_obj_set_align(name, LV_ALIGN_TOP_MID);
    album = lv_img_create(song_container);
    lv_img_set_src(album, player->cur_info.album);
    lv_obj_set_pos(album, 20, 48);
    lv_obj_set_align(album, LV_ALIGN_TOP_LEFT);
    author = lv_img_create(song_container);
    lv_img_set_src(author, player->cur_info.author);
    lv_obj_set_pos(author, 159, 48);
    lv_obj_set_align(author, LV_ALIGN_TOP_LEFT);
    introduce = lv_img_create(song_container);
    lv_img_set_src(introduce, player->cur_info.introduce);
    lv_obj_set_pos(introduce, 0, 90);
    lv_obj_set_align(introduce, LV_ALIGN_TOP_MID);
    text_file = lv_img_create(song_container);
    lv_img_set_src(text_file, player->cur_info.text_file);
    lv_obj_set_pos(text_file, 0, 113);
    lv_obj_set_align(text_file, LV_ALIGN_TOP_MID);

    /* set img tgo be clickable */
    lv_obj_add_flag(heart, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(play_mode, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(play_control, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(play_control_next, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(play_control_last, LV_OBJ_FLAG_CLICKABLE);

    /* expand clickable areas */
    lv_obj_set_ext_click_area(heart, 10);
    lv_obj_set_ext_click_area(play_mode, 10);
    lv_obj_set_ext_click_area(play_control, 10);
    lv_obj_set_ext_click_area(play_control_next, 10);
    lv_obj_set_ext_click_area(play_control_last, 10);

    lv_obj_add_event_cb(heart, heart_switch_cb, LV_EVENT_ALL, player);
    lv_obj_add_event_cb(play_mode, play_mode_cb, LV_EVENT_ALL, player);
    lv_obj_add_event_cb(play_control, play_continue_pause_cb, LV_EVENT_ALL, player);
    lv_obj_add_event_cb(play_control_next, play_next_cb, LV_EVENT_ALL, player);
    lv_obj_add_event_cb(play_control_last, play_last_cb, LV_EVENT_ALL, player);
}

static void ui_aic_audio_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    easy_player_t *play_cb = (easy_player_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_anim_del_all();
        lv_timer_del(play_time_timer);
        lv_timer_del(cover_rotate_timer);
        media_list_destroy(play_cb->list);
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

static void heart_switch_cb(lv_event_t *e)
{
    static int heart_status = 0;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *heart = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        if (heart_status == 0)
            lv_img_set_src(heart, LVGL_PATH(aic_audio/love.png));
        else
            lv_img_set_src(heart, LVGL_PATH(aic_audio/love_red.png));
    }

    heart_status = heart_status == 0 ? 1: 0;
}

static void play_mode_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *play_mode = lv_event_get_target(e);
    easy_player_t *play_cb = (easy_player_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        play_cb->mode++;
        play_cb->mode = play_cb->mode > PLAY_MODE_CIRCLE_ONE ? PLAY_MODE_CIRCLE : play_cb->mode;
        if (play_cb->mode == PLAY_MODE_CIRCLE) {
            lv_img_set_src(play_mode, LVGL_PATH(aic_audio/circle.png));
            lv_ffmpeg_player_set_auto_restart(play_cb->ffmpeg, false);
        } else if (play_cb->mode == PLAY_MODE_RANDOM) {
            lv_img_set_src(play_mode, LVGL_PATH(aic_audio/random.png));
            media_list_set_randomly(play_cb->list);
            lv_ffmpeg_player_set_auto_restart(play_cb->ffmpeg, false);
        } else {
            lv_img_set_src(play_mode, LVGL_PATH(aic_audio/circle_one.png));
            lv_ffmpeg_player_set_auto_restart(play_cb->ffmpeg, true);
        }
    }
}

static void play_continue_pause_cb(lv_event_t *e)
{
    const int status_resume = 0;
    const int status_pause = 1;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *play_control = lv_event_get_target(e);
    easy_player_t *play_cb = (easy_player_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (play_cb->play_status == 0) {
            lv_timer_resume(play_time_timer);
            lv_timer_resume(cover_rotate_timer);
            lv_img_set_src(play_control, LVGL_PATH(aic_audio/pause.png));
            lv_ffmpeg_player_set_cmd_ex(play_cb->ffmpeg, LV_FFMPEG_PLAYER_CMD_RESUME, NULL);
        } else {
            lv_timer_pause(play_time_timer);
            lv_timer_pause(cover_rotate_timer);
            lv_img_set_src(play_control, LVGL_PATH(aic_audio/play.png));
            lv_ffmpeg_player_set_cmd_ex(play_cb->ffmpeg, LV_FFMPEG_PLAYER_CMD_PAUSE, NULL);
        }
    }

    play_cb->play_status = play_cb->play_status == status_resume ? status_pause : status_resume;
}

static void play_status_update(easy_player_t *play_cb, int anima_in)
{
    media_list_get_info(play_cb->list, &play_cb->cur_info, play_cb->cur_pos);

    lv_img_set_src(cover, play_cb->cur_info.cover);
    lv_bar_set_range(progress, 0, play_cb->cur_info.duration_ms);
    lv_bar_set_value(progress, 0, LV_ANIM_OFF);
    lv_label_set_text_fmt(duration, "%02d:%02d", 0, 0);
    lv_img_set_src(name, play_cb->cur_info.name);
    lv_img_set_src(album, play_cb->cur_info.album);
    lv_img_set_src(author, play_cb->cur_info.author);
    lv_img_set_src(introduce, play_cb->cur_info.introduce);
    lv_img_set_src(text_file, play_cb->cur_info.text_file);

    cover_rotate_angle = 0;
    lv_img_set_angle(cover, 0);

    lv_timer_pause(cover_rotate_timer);
    lv_timer_pause(play_time_timer);

    lv_anim_del_all();

    cover_move_anima(cover, anima_in);
    /* the timer resume when play point anim has been deleted */
    play_point_rotate_anima(play_point, play_cb);
}

static void play_next_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    easy_player_t *play_cb = (easy_player_t *)lv_event_get_user_data(e);
    int info_num = media_list_get_info_num(play_cb->list);

    if (code == LV_EVENT_CLICKED) {
        play_cb->cur_pos++;

        if (play_cb->cur_pos >= info_num)
            play_cb->cur_pos = 0;

        play_status_update(play_cb, ANIMA_COVER_IN);
        lv_ffmpeg_player_set_src(play_cb->ffmpeg, play_cb->cur_info.source);
    }
}

static void play_last_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    easy_player_t *play_cb = (easy_player_t *)lv_event_get_user_data(e);
    int info_num = media_list_get_info_num(play_cb->list);

    if (code == LV_EVENT_CLICKED) {
        play_cb->cur_pos--;

        if (play_cb->cur_pos < 0)
            play_cb->cur_pos = info_num - 1;

        play_status_update(play_cb, ANIMA_COVER_IN);
    }
}

static void play_time_update_cb(lv_timer_t * timer)
{
    easy_player_t *play_cb = (easy_player_t *)timer->user_data;
    u64 escap_time = 0;
    int minute = 0;
    int seconds = 0;
    bool play_end = false;

    lv_ffmpeg_player_set_cmd_ex(play_cb->ffmpeg, LV_FFMPEG_PLAYER_CMD_PLAY_END_EX, &play_end);
    if (play_end && (play_cb->mode != PLAY_MODE_CIRCLE_ONE)) {
#if LVGL_VERSION_MAJOR == 8
        lv_event_send(play_control_next, LV_EVENT_CLICKED, play_cb); /* play next */
#else
        lv_obj_send_event(play_control_next, LV_EVENT_CLICKED, play_cb);
#endif
        return;
    }

    lv_ffmpeg_player_set_cmd_ex(play_cb->ffmpeg, LV_FFMPEG_PLAYER_CMD_GET_PLAY_TIME_EX, &escap_time);
    minute = (escap_time / (1000 * 1000)) / 60;
    seconds = (escap_time / (1000 * 1000)) % 60;

    lv_bar_set_value(progress, escap_time / 1000, LV_ANIM_ON);
    lv_label_set_text_fmt(duration, "%02d:%02d", minute, seconds);
}

static void cover_rotate_cb(lv_timer_t * timer)
{
    cover_rotate_angle += 5;

    if (cover_rotate_angle >= 3600)
        cover_rotate_angle = 0;
    lv_img_set_angle(cover, cover_rotate_angle);
}

static void lv_anim_set_x(lv_anim_t* a, int32_t v)
{
   lv_obj_t *obj = (lv_obj_t *)a->user_data;
   lv_obj_set_style_x(obj, v, 0);
}

static void cover_move_anima(lv_obj_t *cover, int anima_in)
{
    const int cover_x1 = 36;
    lv_anim_t ui_cover_anim;
    lv_img_t *img = (lv_img_t *)cover;
    lv_coord_t width = img->w;

    int32_t start_value, end_value;

    if (anima_in == ANIMA_COVER_IN) {
        start_value = cover_x1 - width;
        end_value = cover_x1;
    } else {
        start_value = cover_x1;
        end_value = cover_x1 + width;
    }

    lv_anim_init(&ui_cover_anim);
    lv_anim_set_time(&ui_cover_anim, 300);
    lv_anim_set_user_data(&ui_cover_anim, cover);
    lv_anim_set_custom_exec_cb(&ui_cover_anim, lv_anim_set_x);
    lv_anim_set_values(&ui_cover_anim, start_value, end_value);
    lv_anim_set_path_cb(&ui_cover_anim, lv_anim_path_linear);
    lv_anim_set_delay(&ui_cover_anim, ANIMA_DELAY_TIME_MS);
    lv_anim_set_deleted_cb(&ui_cover_anim, NULL);
    lv_anim_set_playback_time(&ui_cover_anim, 0);
    lv_anim_set_playback_delay(&ui_cover_anim, 0);
    lv_anim_set_repeat_count(&ui_cover_anim, 0);
    lv_anim_set_repeat_delay(&ui_cover_anim, 0);
    lv_anim_set_early_apply(&ui_cover_anim, false);

    lv_anim_start(&ui_cover_anim);
}

static void ui_anim_start_play_and_free(lv_anim_t *a)
{
    ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
    easy_player_t *play_cb = (easy_player_t *)usr->player;

    play_cb->play_status = 0;
    lv_img_set_src(play_control, LVGL_PATH(aic_audio/pause.png));
    lv_ffmpeg_player_set_cmd_ex(play_cb->ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);
    lv_timer_resume(play_time_timer);
    lv_timer_resume(cover_rotate_timer);

    lv_mem_free(a->user_data);
}

static void lv_anim_set_rotate(lv_anim_t* a, int32_t v)
{
    ui_anim_user_data_t *usr = (ui_anim_user_data_t *)a->user_data;
    lv_img_set_angle(usr->target, v);
}

static void play_point_rotate_anima(lv_obj_t *play_point, easy_player_t *player)
{
    lv_anim_t ui_point_up_anim;
    lv_anim_t ui_point_down_anim;

    const int up_anim_time = 300;
    const int wait_time = 200;

    ui_anim_user_data_t *anim_user_data = lv_mem_alloc(sizeof(ui_anim_user_data_t));
    anim_user_data->target = play_point;
    anim_user_data->player = player;

    lv_anim_init(&ui_point_up_anim);
    lv_anim_set_time(&ui_point_up_anim, up_anim_time);
    lv_anim_set_user_data(&ui_point_up_anim, anim_user_data);
    lv_anim_set_custom_exec_cb(&ui_point_up_anim, lv_anim_set_rotate);
    lv_anim_set_values(&ui_point_up_anim, 0, -300);
    lv_anim_set_path_cb(&ui_point_up_anim, lv_anim_path_linear);
    lv_anim_set_delay(&ui_point_up_anim, ANIMA_DELAY_TIME_MS);
    lv_anim_set_deleted_cb(&ui_point_up_anim, NULL);
    lv_anim_set_playback_time(&ui_point_up_anim, 0);
    lv_anim_set_playback_delay(&ui_point_up_anim, 0);
    lv_anim_set_repeat_count(&ui_point_up_anim, 0);
    lv_anim_set_repeat_delay(&ui_point_up_anim, 0);
    lv_anim_set_early_apply(&ui_point_up_anim, false);

    lv_anim_init(&ui_point_down_anim);
    lv_anim_set_time(&ui_point_down_anim, up_anim_time);
    lv_anim_set_user_data(&ui_point_down_anim, anim_user_data);
    lv_anim_set_custom_exec_cb(&ui_point_down_anim, lv_anim_set_rotate);
    lv_anim_set_values(&ui_point_down_anim, -300, 0);
    lv_anim_set_path_cb(&ui_point_down_anim, lv_anim_path_linear);
    lv_anim_set_delay(&ui_point_down_anim, ANIMA_DELAY_TIME_MS + up_anim_time + wait_time);
    lv_anim_set_deleted_cb(&ui_point_down_anim, ui_anim_start_play_and_free);
    lv_anim_set_playback_time(&ui_point_down_anim, 0);
    lv_anim_set_playback_delay(&ui_point_down_anim, 0);
    lv_anim_set_repeat_count(&ui_point_down_anim, 0);
    lv_anim_set_repeat_delay(&ui_point_down_anim, 0);
    lv_anim_set_early_apply(&ui_point_down_anim, false);

    lv_anim_start(&ui_point_up_anim);
    lv_anim_start(&ui_point_down_anim);
}

