/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Authors:  jun.ma <jun.ma@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"
#include "aic_player.h"

FAKE_IMAGE_DECLARE(bg_dark);
LV_IMG_DECLARE(pre_normal);
LV_IMG_DECLARE(pre_press);
LV_IMG_DECLARE(play_normal);
LV_IMG_DECLARE(play_press);
LV_IMG_DECLARE(pause_norma_1);
LV_IMG_DECLARE(pause_press_1);
LV_IMG_DECLARE(next_normal);
LV_IMG_DECLARE(next_press);

#define LVGL_PLAYER_STATE_PLAY  1
#define LVGL_PLAYER_STATE_PAUSE  2
#define LVGL_PLAYER_STATE_STOP   3

struct lvgl_player_context{
    int file_cnt;
    int file_index;
    int player_state;
    struct aic_player *player;
    int sync_flag;
    struct av_media_info media_info;
    int demuxer_detected_flag;
    int player_end;
    struct mpp_size screen_size;
    struct mpp_rect disp_rect;
};

static char g_filename[][256] = {
    LVGL_FILE_LIST_PATH(sample.mp4),
};

static struct lvgl_player_context g_lvgl_player_ctx;
static lv_obj_t *g_btn_pre;
static lv_obj_t *g_btn_next;
static lv_obj_t *g_btn_pause_play;
static lv_obj_t *g_btn_hide_show;
static lv_obj_t *img_bg = NULL;


static s32 event_handle(void* app_data,s32 event,s32 data1,s32 data2)
{
    int ret = 0;
    struct lvgl_player_context *ctx = (struct lvgl_player_context *)app_data;

    switch (event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            ctx->player_end = 1;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == ctx->sync_flag) {
                ctx->demuxer_detected_flag = 1;
            }
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == ctx->sync_flag) {
                ctx->player_end = 1;
            }
            break;
        default:
            break;
    }
    return ret;
}

static int set_disp_rect(struct lvgl_player_context *ctx)
{
    int ret = 0;

    if (!ctx->media_info.has_video) {
        return 0;
    }
    aic_player_get_screen_size(ctx->player, &ctx->screen_size);
    //attention:disp not exceed screen_size
    if (ctx->media_info.video_stream.width > ctx->screen_size.width) {
        ctx->disp_rect.x = 0;
        ctx->disp_rect.width = ctx->screen_size.width;
    } else {
        ctx->disp_rect.x = (ctx->screen_size.width - ctx->media_info.video_stream.width) / 2;
        ctx->disp_rect.width = ctx->media_info.video_stream.width;
    }

    if (ctx->media_info.video_stream.height > ctx->screen_size.height) {
        ctx->disp_rect.y = 0;
        ctx->disp_rect.height = ctx->screen_size.height;
    } else {
        ctx->disp_rect.y = (ctx->screen_size.height - ctx->media_info.video_stream.height) / 2;
        ctx->disp_rect.height = ctx->media_info.video_stream.height;
    }
    printf("Media size %d x %d, Display offset (%d, %d) size %d x %d\n",
           ctx->media_info.video_stream.width, ctx->media_info.video_stream.height,
           ctx->disp_rect.x, ctx->disp_rect.y, ctx->disp_rect.width, ctx->disp_rect.height);

    ret = aic_player_set_disp_rect(ctx->player, &ctx->disp_rect);
    if (ret != 0) {
        printf("aic_player_set_disp_rect error\n");
        return -1;
    }
    return ret;
}

static int lvgl_play(struct lvgl_player_context *ctx)
{
    int k = 0;
    int ret = 0;

    k  = ctx->file_index%ctx->file_cnt;
    aic_player_set_uri(ctx->player,g_filename[k]);
    ctx->sync_flag = AIC_PLAYER_PREPARE_SYNC;
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_ASYNC) {
        ret = aic_player_prepare_async(ctx->player);
    } else {
        ret = aic_player_prepare_sync(ctx->player);
    }
    if (ret) {
        return -1;
    }
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_SYNC) {
        ret =  aic_player_get_media_info(ctx->player,&ctx->media_info);
        if (ret != 0) {
            return -1;
        }

        ret = aic_player_start(ctx->player);
        if(ret != 0) {
            return -1;
        }

        ret = set_disp_rect(ctx);
        if(ret != 0) {
            return -1;
        }

        ret = aic_player_play(ctx->player);
        if (ret != 0) {
            return -1;
        }
    }
    return 0;
}

static int lvgl_stop(struct lvgl_player_context *ctx)
{
    return aic_player_stop(ctx->player);
}

static int lvgl_pause(struct lvgl_player_context *ctx)
{
    return aic_player_pause(ctx->player);
}

static int lvgl_play_pre(struct lvgl_player_context *ctx)
{
    ctx->file_index--;
    ctx->file_index = (ctx->file_index < 0) ? (ctx->file_cnt - 1) : ctx->file_index;
    lvgl_stop(ctx);
    if (lvgl_play(ctx) != 0) {
        return -1;
    }
    return 0;
}

static int lvgl_play_next(struct lvgl_player_context *ctx)
{
    ctx->file_index++;
    ctx->file_index = (ctx->file_index > ctx->file_cnt - 1) ? 0 : ctx->file_index;
    lvgl_stop(ctx);
    if (lvgl_play(ctx) != 0) {
        return -1;
    }
    return 0;
}

static void btn_pause_play_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;
    if (code == LV_EVENT_CLICKED) {
        if (ctx->player_state == LVGL_PLAYER_STATE_STOP) {
            if (lvgl_play(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
                ctx->player_state = LVGL_PLAYER_STATE_STOP;
                ctx->player_end = 1;
                return;
            }
            ctx->player_end = 0;
            ctx->player_state = LVGL_PLAYER_STATE_PLAY;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        } else if (ctx->player_state == LVGL_PLAYER_STATE_PLAY) {
            lvgl_pause(ctx);
            ctx->player_state = LVGL_PLAYER_STATE_PAUSE;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &play_normal, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &play_press, NULL);
        } else if (ctx->player_state == LVGL_PLAYER_STATE_PAUSE) {
            lvgl_pause(ctx);
            ctx->player_state = LVGL_PLAYER_STATE_PLAY;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        }
    }
}

static void btn_next_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;

    if (code == LV_EVENT_CLICKED) {
        if (lvgl_play_next(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
            ctx->player_state =LVGL_PLAYER_STATE_STOP;
            ctx->player_end = 1;
        } else {
            ctx->player_state =LVGL_PLAYER_STATE_PLAY;
            ctx->player_end = 0;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);

        }
    }
}

static void btn_pre_event_cb(lv_event_t *e)
{
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    struct lvgl_player_context * ctx = (struct lvgl_player_context *)btn->user_data;

    if (code == LV_EVENT_CLICKED) {
        if (lvgl_play_pre(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
            ctx->player_state =LVGL_PLAYER_STATE_STOP;
            ctx->player_end = 1;
        } else {
            ctx->player_state =LVGL_PLAYER_STATE_PLAY;
            ctx->player_end = 0;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        }
    }
}

static void btn_show_hide_event_cb(lv_event_t *e)
{
    static int show = 1;
    lv_event_code_t code = (lv_event_code_t)lv_event_get_code(e);

    if (code == LV_EVENT_RELEASED) {
        if (show) {
            lv_obj_add_flag(g_btn_pre, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_btn_pause_play, LV_OBJ_FLAG_HIDDEN);
            show = 0;
        } else {
            lv_obj_clear_flag(g_btn_pre, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_btn_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_btn_pause_play, LV_OBJ_FLAG_HIDDEN);
            show = 1;
        }
    }
}

static void create_player(lv_obj_t * parent)
{
    static lv_style_t btn_style;
    lv_style_init(&btn_style);
    lv_style_set_radius(&btn_style, 0);
    lv_style_set_border_width(&btn_style, 2);
    //lv_style_set_border_color(&btn_style, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_bg_opa(&btn_style, LV_OPA_0);

    lv_memset(&g_lvgl_player_ctx, 0, sizeof(struct lvgl_player_context));
    g_lvgl_player_ctx.player = aic_player_create(NULL);
    if (g_lvgl_player_ctx.player == NULL) {
        printf("aic_player_create fail!!!!\n");
        return;
    }

    g_lvgl_player_ctx.file_cnt =  sizeof(g_filename) / sizeof(g_filename[0]);
    g_lvgl_player_ctx.file_index = 0;
    g_lvgl_player_ctx.player_state = LVGL_PLAYER_STATE_STOP;
    aic_player_set_event_callback(g_lvgl_player_ctx.player, &g_lvgl_player_ctx, event_handle);

    g_btn_hide_show = lv_btn_create(parent);
    g_btn_hide_show->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_hide_show, 800, 40);
    lv_obj_set_size(g_btn_hide_show, 180, 520);
    lv_obj_add_event_cb(g_btn_hide_show,btn_show_hide_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_hide_show,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_hide_show,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_hide_show, LV_OPA_TRANSP, 0);

    g_btn_pre = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(g_btn_pre, LV_IMGBTN_STATE_RELEASED , NULL, &pre_normal, NULL);
    lv_imgbtn_set_src(g_btn_pre, LV_IMGBTN_STATE_PRESSED , NULL, &pre_press, NULL);
    g_btn_pre->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_pre, 200, 450);
    lv_obj_set_size(g_btn_pre, 64, 64);
    lv_obj_add_event_cb(g_btn_pre,btn_pre_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_pre,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_pre,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_pre, LV_OPA_TRANSP, 0);

    g_btn_pause_play = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &play_normal, NULL);
    lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &play_press, NULL);
    g_btn_pause_play->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_pause_play, 450, 450);
    lv_obj_set_size(g_btn_pause_play, 64, 64);
    lv_obj_add_event_cb(g_btn_pause_play,btn_pause_play_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_pause_play,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_pause_play,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_pause_play, LV_OPA_TRANSP, 0);

    g_btn_next = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(g_btn_next, LV_IMGBTN_STATE_RELEASED , NULL, &next_normal, NULL);
    lv_imgbtn_set_src(g_btn_next, LV_IMGBTN_STATE_PRESSED , NULL, &next_press, NULL);
    g_btn_next->user_data = &g_lvgl_player_ctx;
    lv_obj_set_pos(g_btn_next, 700, 450);
    lv_obj_set_size(g_btn_next, 64, 64);
    lv_obj_add_event_cb(g_btn_next,btn_next_event_cb,LV_EVENT_ALL, NULL);
    lv_obj_add_style(g_btn_next,&btn_style, 0);
    lv_obj_set_style_border_opa(g_btn_next,LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(g_btn_next, LV_OPA_TRANSP, 0);
}

static void player_callback(lv_timer_t *tmr)
{
    struct lvgl_player_context *ctx;

    (void)tmr;

    ctx = &g_lvgl_player_ctx;
    if (ctx->player_end && ctx->player_state != LVGL_PLAYER_STATE_STOP) {
        if (lvgl_play_next(ctx) != 0) {//if play fail ,it is considered play finsh.play the next one
            ctx->player_state =LVGL_PLAYER_STATE_STOP;
            ctx->player_end = 1;
        } else {
            ctx->player_state =LVGL_PLAYER_STATE_PLAY;
            ctx->player_end = 0;
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_RELEASED , NULL, &pause_norma_1, NULL);
            lv_imgbtn_set_src(g_btn_pause_play, LV_IMGBTN_STATE_PRESSED , NULL, &pause_press_1, NULL);
        }
    }
    return;
}

void simple_player_ui_init()
{
    if (AICFB_FORMAT == MPP_FMT_RGB_565
        || AICFB_FORMAT == MPP_FMT_ARGB_1555) {
        static struct mpp_fb *fd_dev;
        struct aicfb_alpha_config alpha = {0};
        alpha.layer_id = 1;
        alpha.enable = 1;
        alpha.mode = 1;
        alpha.value = 64; //set ui transparent,255 is max.
        fd_dev = mpp_fb_open();
        mpp_fb_ioctl(fd_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
        mpp_fb_close(fd_dev);
    }

    FAKE_IMAGE_INIT(bg_dark, 1024, 600, 0, 0x00000000);
    img_bg = lv_img_create(lv_scr_act());
    lv_img_set_src(img_bg, FAKE_IMAGE_NAME(bg_dark));
    create_player(img_bg);
    lv_timer_create(player_callback, 20, 0);
}

void ui_init(void)
{
    simple_player_ui_init();
}
