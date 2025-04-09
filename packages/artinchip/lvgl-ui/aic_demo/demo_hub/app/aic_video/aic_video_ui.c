#include "aic_ui.h"
#include "lvgl.h"
#include "media_list.h"
#include "aic_lv_ffmpeg.h"
#include "../app_entrance.h"

#define BYTE_ALIGN(x, byte) (((x) + ((byte) - 1))&(~((byte) - 1)))

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

extern int check_lvgl_de_config(void);
extern void fb_dev_layer_set(void);
extern void fb_dev_layer_reset(void);
extern void fb_dev_layer_only_video(void);

extern lv_obj_t *nonto_sans_label_comp(lv_obj_t *parent, uint16_t weight);
extern lv_obj_t *com_imgbtn_switch_comp(lv_obj_t *parent, void *img_src_0, void *img_src_1);
extern lv_obj_t *com_imgbtn_comp(lv_obj_t *parent, void *img_src, void *pressed_img_src);

static lv_obj_t *video_title;
static lv_obj_t *pass_time;
static lv_obj_t *duration;
static lv_obj_t *progress;

static lv_obj_t *play_control_next;
static lv_timer_t *play_time_timer;

static easy_player_t player;

static void media_list_init();
static void aic_video_ui_impl(lv_obj_t *parent);
static void get_media_info_from_src(const char *src_path, media_info_t *info);

static void ui_aic_video_cb(lv_event_t *e);
static void show_control_cb(lv_event_t *e);
static void play_mode_cb(lv_event_t *e);
static void play_continue_pause_cb(lv_event_t *e);
static void play_next_cb(lv_event_t *e);
static void play_last_cb(lv_event_t *e);
static void back_home_cb(lv_event_t *e);

static void play_time_update_cb(lv_timer_t * timer);

static int adaptive_width(int width);
static int adaptive_height(int height);

lv_obj_t *aic_video_ui_init(void)
{
    lv_obj_t *aic_video_ui = lv_obj_create(NULL);
    lv_obj_clear_flag(aic_video_ui, LV_OBJ_FLAG_SCROLLABLE);

#ifdef AIC_CHIP_D12X
    extern void layer_sys_ui_visible(int flag);
    layer_sys_ui_visible(0);
    return aic_video_ui;
#endif

    if (check_lvgl_de_config() == -1) {
        LV_LOG_INFO("Please check defined AICFB_ARGB8888 and LV_COLOR_DEPTH is 32 or\n");
        LV_LOG_INFO("check defined AICFB_RGB565 and LV_COLOR_DEPTH is 16\n");
        return aic_video_ui;
    }
    fb_dev_layer_set();

    player.ffmpeg = lv_ffmpeg_player_create(aic_video_ui);
    lv_obj_set_size(player.ffmpeg, LV_HOR_RES, LV_VER_RES);
    player.list = media_list_create();
    media_list_init();
    media_list_get_info(player.list, &player.cur_info, MEDIA_TYPE_POS_OLDEST);
    player.mode = PLAY_MODE_CIRCLE;
    player.play_status = 1;

    aic_video_ui_impl(aic_video_ui);

    lv_obj_add_event_cb(aic_video_ui, ui_aic_video_cb, LV_EVENT_ALL, &player);

    /* Enable LV_FFMPEG_PLAYED_CMD_KEEP_LAST_FRAME_EX when playing multiple video sources
       to avoid black screens during source switching, though it will increase memory usage.
       Alternatively, add a loading icon to indicate to users that a switch is in progress,
       which can mitigate the impact of black screens on viewing experience.
       Need to set before setting src. */
    //lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYED_CMD_KEEP_LAST_FRAME_EX, NULL);

    lv_ffmpeg_player_set_src(player.ffmpeg, player.cur_info.source);
    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);

    return aic_video_ui;
}

static void ui_aic_video_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        lv_timer_del(play_time_timer);
        media_list_destroy(player.list);
        fb_dev_layer_reset();
    }
    if (code == LV_EVENT_DELETE) {;}
    if (code == LV_EVENT_SCREEN_LOADED) {;}
}

static void media_list_init()
{
    media_info_t info;

    strcpy(info.name, "Known 1");
    strcpy(info.source, LVGL_PATH_ORI(common/elevator_mjpg.mp4));
    get_media_info_from_src((const char *)info.source, &info);
    media_list_add_info(player.list, &info);

    strcpy(info.name, "Known 2");
    strcpy(info.source,  LVGL_PATH_ORI(common/elevator_mjpg.mp4));
    get_media_info_from_src((const char *)info.source, &info);
    media_list_add_info(player.list, &info);
}

static void aic_video_ui_impl(lv_obj_t *parent)
{
    lv_obj_t *control_container = lv_obj_create(parent);
    lv_obj_remove_style_all(control_container);
    lv_obj_set_size(control_container, lv_pct(100),lv_pct(100));
    lv_obj_add_flag(control_container, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *head_bg = lv_obj_create(control_container);
    lv_obj_remove_style_all(head_bg);
    lv_obj_set_style_bg_color(head_bg, lv_color_hex(0x0), 0);
    lv_obj_set_style_bg_opa(head_bg, LV_OPA_100, 0);
    lv_obj_set_size(head_bg, adaptive_width(1024), adaptive_height(60));

    if (LV_HOR_RES <= 480)
        video_title = nonto_sans_label_comp(head_bg, 24);
    else
        video_title = nonto_sans_label_comp(head_bg, 36);
    lv_label_set_text(video_title, player.cur_info.name);
    lv_obj_center(video_title);

    lv_obj_t *control_bg = lv_obj_create(control_container);
    lv_obj_remove_style_all(control_bg);
    lv_obj_set_style_bg_color(control_bg, lv_color_hex(0x0), 0);
    lv_obj_set_style_bg_opa(control_bg, LV_OPA_100, 0);
    lv_obj_set_size(control_bg, adaptive_width(1024), adaptive_height(120));
    lv_obj_set_pos(control_bg, adaptive_width(0), adaptive_height(480));

    progress = lv_bar_create(control_bg);
    lv_obj_set_pos(progress, adaptive_width(26), adaptive_height(28));
    lv_bar_set_range(progress, 0, player.cur_info.duration_ms);
    lv_bar_set_value(progress, 0, LV_ANIM_OFF);
    lv_obj_set_size(progress, adaptive_width(969), adaptive_height(8));
    lv_obj_set_style_width(progress, adaptive_width(969), LV_PART_INDICATOR);
    lv_obj_set_style_height(progress, adaptive_height(8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(progress, lv_color_hex(0x8f8f8f), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress, lv_color_hex(0xffffff), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress, LV_OPA_100, LV_PART_INDICATOR);

    if (LV_HOR_RES <= 480)
        pass_time = nonto_sans_label_comp(control_bg, 12);
    else
        pass_time = nonto_sans_label_comp(control_bg, 24);
    lv_label_set_text_fmt(pass_time, "%02d:%02d", 0, 0);
    lv_obj_set_pos(pass_time, adaptive_width(26), adaptive_height(40));
    lv_obj_set_style_text_color(pass_time, lv_color_hex(0xffffff), LV_PART_MAIN);
    if (LV_HOR_RES <= 480)
        duration = nonto_sans_label_comp(control_bg, 12);
    else
        duration = nonto_sans_label_comp(control_bg, 24);
    lv_label_set_text_fmt(duration, "%02ld:%02ld", GET_MINUTES(player.cur_info.duration_ms), GET_SECONDS(player.cur_info.duration_ms));
    lv_obj_set_pos(duration, adaptive_width(930), adaptive_height(40));
    lv_obj_set_style_text_color(duration, lv_color_hex(0xffffff), LV_PART_MAIN);

    lv_obj_t *heart = com_imgbtn_switch_comp(control_bg, LVGL_PATH(aic_video/love.png), LVGL_PATH(aic_video/love_press.png));
    lv_obj_set_pos(heart, adaptive_width(798), adaptive_height(40));
    lv_obj_t *play_mode = lv_img_create(control_bg);
    lv_img_set_src(play_mode, LVGL_PATH(aic_video/random.png));
    lv_obj_set_pos(play_mode, adaptive_width(169), adaptive_height(40));
    lv_obj_add_flag(play_mode, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *play_control = com_imgbtn_switch_comp(control_bg, LVGL_PATH(aic_video/pause.png), LVGL_PATH(aic_video/play.png));
    lv_obj_set_pos(play_control, adaptive_width(488), adaptive_height(52));
    play_control_next = com_imgbtn_comp(control_bg, LVGL_PATH(aic_video/next.png), NULL);
    lv_obj_set_pos(play_control_next, adaptive_width(600), adaptive_height(60));
    lv_obj_t *play_control_last = com_imgbtn_comp(control_bg, LVGL_PATH(aic_video/last.png), NULL);
    lv_img_set_src(play_control_last, LVGL_PATH(aic_video/last.png));
    lv_obj_set_pos(play_control_last, adaptive_width(380), adaptive_height(60));

    lv_obj_t *home = com_imgbtn_comp(control_container, LVGL_PATH(aic_video/home.png), NULL);
    lv_obj_set_pos(home, adaptive_width(20), adaptive_height(14));

    lv_obj_add_event_cb(control_container, show_control_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(play_mode, play_mode_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(play_control, play_continue_pause_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(play_control_next, play_next_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(play_control_last, play_last_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(home, back_home_cb, LV_EVENT_ALL, NULL);

    play_time_timer = lv_timer_create(play_time_update_cb, 400 , NULL);
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

static void show_control_cb(lv_event_t *e)
{
    static int show = 0;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *container = lv_event_get_target(e);
    lv_obj_t *child = NULL;
    int child_cnt = lv_obj_get_child_cnt(container);

    if (code == LV_EVENT_CLICKED) {
        show = show == 0 ? 1 : 0;
        for (int i = 0; i < child_cnt; i++) {
            child = lv_obj_get_child(container, i);
            if (show) {
                fb_dev_layer_set();
                lv_obj_clear_flag(child, LV_OBJ_FLAG_HIDDEN);
            } else {
                fb_dev_layer_only_video();
                lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void play_mode_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *play_mode = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        player.mode++;
        player.mode = player.mode > PLAY_MODE_CIRCLE_ONE ? PLAY_MODE_CIRCLE : player.mode;
        if (player.mode == PLAY_MODE_CIRCLE) {
            lv_img_set_src(play_mode, LVGL_PATH(aic_video/loop.png));
            lv_ffmpeg_player_set_auto_restart(player.ffmpeg, false);
        } else if (player.mode == PLAY_MODE_RANDOM) {
            lv_img_set_src(play_mode, LVGL_PATH(aic_video/random.png));
            media_list_set_randomly(player.list);
            lv_ffmpeg_player_set_auto_restart(player.ffmpeg, false);
        } else {
            lv_img_set_src(play_mode, LVGL_PATH(aic_video/single_loop.png));
            lv_ffmpeg_player_set_auto_restart(player.ffmpeg, true);
        }
    }
}

static void play_continue_pause_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        if (player.play_status == 0) {
            lv_timer_resume(play_time_timer);
            lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_RESUME, NULL);
        } else {
            lv_timer_pause(play_time_timer);
            lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_PAUSE, NULL);
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
    lv_label_set_text(video_title, player.cur_info.name);
    lv_label_set_text_fmt(pass_time, "%02d:%02d", 0 ,0);
    lv_label_set_text_fmt(duration, "%02ld:%02ld", GET_MINUTES(player.cur_info.duration_ms), GET_SECONDS(player.cur_info.duration_ms));
    if (strcmp(last_src, player.cur_info.source) == 0) {
        lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_SET_PLAY_TIME_EX, &seek_time);
    } else {
        lv_ffmpeg_player_set_src(player.ffmpeg, player.cur_info.source);
        lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_START, NULL);
    }
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
    }
}

static void back_home_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        app_entrance(APP_NAVIGATION, 1);
    }
}

static void play_time_update_cb(lv_timer_t * timer)
{
    long escap_time = 0;
    bool play_end = false;

    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_PLAY_END_EX, &play_end);
    if (play_end == true && (player.mode != PLAY_MODE_CIRCLE_ONE)) {
#if LVGL_VERSION_MAJOR == 8
        lv_event_send(play_control_next, LV_EVENT_CLICKED, NULL); /* play next */
#else
        lv_obj_send_event(play_control_next, LV_EVENT_CLICKED, NULL); /* play next */
#endif
        return;
    }

    lv_ffmpeg_player_set_cmd_ex(player.ffmpeg, LV_FFMPEG_PLAYER_CMD_GET_PLAY_TIME_EX, &escap_time);

    escap_time = escap_time / 1000;
    lv_bar_set_value(progress, escap_time, LV_ANIM_ON);
    lv_label_set_text_fmt(pass_time, "%02ld:%02ld", GET_MINUTES(escap_time), GET_SECONDS(escap_time));
}

static int adaptive_width(int width)
{
    float src_width = LV_HOR_RES;
    int align_width = (int)((src_width / 1024.0) * width);
    return BYTE_ALIGN(align_width, 2);
}

static int adaptive_height(int height)
{
    float scr_height = LV_VER_RES;
    int align_height = (int)((scr_height / 600.0) * height);
    return BYTE_ALIGN(align_height, 2);
}

