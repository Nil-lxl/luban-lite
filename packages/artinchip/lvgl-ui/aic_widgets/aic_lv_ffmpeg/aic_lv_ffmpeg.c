/*
 * Copyright (C) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Zequan Liang <zequan.liang@artinchip.com>
 *          Ning Fang <ning.fang@artinchip.com>
 */

#include "aic_lv_ffmpeg.h"

#if LV_USE_FFMPEG == 0 && defined(AIC_MPP_PLAYER_INTERFACE)

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define MY_CLASS &lv_aic_player_class

struct ffmpeg_context_s {
    struct aic_player *aic_player;
    struct av_media_info media_info;
    bool has_keep_last_frame;
    int play_end;
    int play_async;
    int play_detected;
};

#define FRAME_DEF_REFR_PERIOD   33  /*[ms]*/

static void lv_ffmpeg_player_get_disp_area(lv_obj_t *obj, lv_point_t *pos,
                                            lv_coord_t *width, lv_coord_t *height, lv_coord_t *rotation);
static void lv_ffmpeg_player_set_disp_area(lv_obj_t *obj, lv_point_t pos,
                                        lv_coord_t width, lv_coord_t height, lv_coord_t rotation);
static void lv_ffmpeg_player_protect_disp_area(lv_obj_t *obj, lv_point_t *pos,
                                            lv_coord_t *width, lv_coord_t *height, lv_coord_t *rotation);

static void lv_ffmpeg_player_frame_update_cb(lv_timer_t * timer);

static void lv_ffmpeg_player_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ffmpeg_player_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static int aic_event_handle(void *app_data, int event, int data1, int data2)
{
    int ret = 0;
    struct ffmpeg_context_s *ffmpeg_ctx = (struct ffmpeg_context_s *)app_data;

    switch(event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            ffmpeg_ctx->play_end = 1;
            break;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            ffmpeg_ctx->play_detected = 1;
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            break;
        default:
            break;
    }
    return ret;
}

const lv_obj_class_t lv_aic_player_class = {
    .constructor_cb = lv_ffmpeg_player_constructor,
    .destructor_cb = lv_ffmpeg_player_destructor,
    .instance_size = sizeof(lv_ffmpeg_player_t),
#if LVGL_VERSION_MAJOR == 8
    .base_class = &lv_img_class
#else
    .base_class = &lv_image_class
#endif
};

void lv_ffmpeg_init(void)
{
    return;
}

int lv_ffmpeg_get_frame_num(const char * path)
{
    int ret = -1;

    return ret;
}

lv_obj_t * lv_ffmpeg_player_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_res_t lv_ffmpeg_player_set_src(lv_obj_t * obj, const char *path)
{
    lv_res_t res = LV_RES_INV;
    char fake_image_name[128] = {0};
    struct ffmpeg_context_s *ffmpeg_ctx;

    lv_ffmpeg_player_t *player = (lv_ffmpeg_player_t *)obj;
    lv_point_t pos;
    lv_coord_t width, height, rotation;

    if (!player->ffmpeg_ctx) {
        player->ffmpeg_ctx = calloc(1, sizeof(struct ffmpeg_context_s));
        if(player->ffmpeg_ctx == NULL) {
            LV_LOG_ERROR("ffmpeg_ctx malloc failed");
            goto failed;
        }
    }

    ffmpeg_ctx = player->ffmpeg_ctx;
    if (!ffmpeg_ctx->aic_player) {
        ffmpeg_ctx->aic_player = aic_player_create((char *)path);
        if (!ffmpeg_ctx->aic_player) {
            LV_LOG_ERROR("create aic player failed: %s", path);
            goto failed;
        }
        aic_player_set_event_callback(ffmpeg_ctx->aic_player, ffmpeg_ctx, aic_event_handle);
    } else {
        aic_player_stop(ffmpeg_ctx->aic_player);
    }

    lv_timer_pause(player->timer);
    aic_player_set_uri(ffmpeg_ctx->aic_player, (char *)path);

    ffmpeg_ctx->play_detected = 0;
    if (ffmpeg_ctx->play_async == 0) {
        res = aic_player_prepare_sync(ffmpeg_ctx->aic_player);
    } else {
        res = aic_player_prepare_async(ffmpeg_ctx->aic_player);
    }

    if (res) {
        LV_LOG_ERROR("aic_player_prepare failed");
        goto failed;
    }

    res = aic_player_get_media_info(ffmpeg_ctx->aic_player,
                                        &ffmpeg_ctx->media_info);
    if (res != 0) {
        LV_LOG_ERROR("aic_player_get_media_info failed");
        goto failed;
    }

    /* update the obj before redraw */
    lv_obj_update_layout(obj);

    player->imgdsc.header.w = ffmpeg_ctx->media_info.video_stream.width;
    player->imgdsc.header.h = ffmpeg_ctx->media_info.video_stream.height;
    ffmpeg_ctx->play_end = 0;

    if (ffmpeg_ctx->media_info.has_audio == 1 && ffmpeg_ctx->media_info.has_video == 0) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); /* audio did't not disp */
        lv_timer_resume(player->timer);
        return LV_RES_OK;
    }

    /* here are the video related settings */
    if (player->keep_last_frame && !ffmpeg_ctx->has_keep_last_frame) {
        int enable = 1;
        aic_player_control(ffmpeg_ctx->aic_player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
        ffmpeg_ctx->has_keep_last_frame = true;
    }

    lv_ffmpeg_player_get_disp_area(obj, &pos, &width, &height, &rotation);
    if (width == 0 && height == 0) {
        width = player->imgdsc.header.w;
        height = player->imgdsc.header.h;
    }

    snprintf(fake_image_name, 128, "L:/%dx%d_%d_%08x.fake",
             (int)width, (int)height, 0, 0x00000000);
    lv_img_set_src(&player->img.obj, fake_image_name);

    if (width == 0 && height == 0) {
        width = player->imgdsc.header.w;
        height = player->imgdsc.header.h;
    }

    lv_ffmpeg_player_protect_disp_area(obj, &pos, &width, &height, &rotation);

    if (ffmpeg_ctx->play_async == 0)
        lv_ffmpeg_player_set_disp_area(obj , pos, width, height, rotation);
    lv_timer_resume(player->timer);
    return LV_RES_OK;

failed:
    return LV_RES_INV;
}

void lv_ffmpeg_player_set_cmd(lv_obj_t * obj, lv_ffmpeg_player_cmd_t cmd)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;
    struct ffmpeg_context_s *ffmpeg_ctx;
    int ret;

    if(!player->ffmpeg_ctx) {
        LV_LOG_ERROR("ffmpeg_ctx is NULL");
        return;
    }

    ffmpeg_ctx = player->ffmpeg_ctx;
    if (!ffmpeg_ctx->aic_player) {
        LV_LOG_ERROR("aic_player is NULL ");
        return;
    }

    switch(cmd) {
        case LV_FFMPEG_PLAYER_CMD_START:
            ret = aic_player_start(ffmpeg_ctx->aic_player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_start failed");
                break;
            }
            LV_LOG_INFO("aic player start");
            break;
        case LV_FFMPEG_PLAYER_CMD_STOP:
            ret = aic_player_stop(ffmpeg_ctx->aic_player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_stop failed");
                break;
            }
            LV_LOG_INFO("aic player stop");
            break;
        case LV_FFMPEG_PLAYER_CMD_PAUSE:
            ret = aic_player_pause(ffmpeg_ctx->aic_player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_pause failed");
                break;
            }
            LV_LOG_INFO("aic player pause");
            break;
        case LV_FFMPEG_PLAYER_CMD_RESUME:
            ret = aic_player_play(ffmpeg_ctx->aic_player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_start failed");
                break;
            }
            LV_LOG_INFO("aic player resume");
            break;
        default:
            LV_LOG_ERROR("Error cmd: %d", cmd);
            break;
    }
}

void lv_ffmpeg_player_set_cmd_ex(lv_obj_t * obj, lv_ffmpeg_player_cmd_t cmd, void *data)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;
    struct ffmpeg_context_s *ffmpeg_ctx;
    int ret;

    bool *end = (bool *)data;
    struct av_media_info *media_info = (struct av_media_info *)data;
    s32 *set_vol = (s32 *)data;
    s32 *get_vol = (s32 *)data;
    u64 *seek = (u64 *)data;

    if(!player->ffmpeg_ctx) {
        LV_LOG_ERROR("ffmpeg_ctx is NULL");
        return;
    }

    ffmpeg_ctx = player->ffmpeg_ctx;
    if (!ffmpeg_ctx->aic_player) {
        LV_LOG_ERROR("aic_player is NULL ");
        return;
    }

    switch(cmd) {
        case LV_FFMPEG_PLAYER_CMD_START:
        case LV_FFMPEG_PLAYER_CMD_STOP:
        case LV_FFMPEG_PLAYER_CMD_PAUSE:
        case LV_FFMPEG_PLAYER_CMD_RESUME:
            lv_ffmpeg_player_set_cmd(obj, cmd);
            break;
        case LV_FFMPEG_PLAYER_CMD_PLAY_END_EX:
            *end = ffmpeg_ctx->play_end == 1 ? true : false;
            break;
        case LV_FFMPEG_PLAYER_CMD_GET_MEDIA_INFO_EX:
            memcpy(media_info, &ffmpeg_ctx->media_info, sizeof(struct av_media_info));
            break;
        case LV_FFMPEG_PLAYER_CMD_SET_VOLUME_EX:
            ret = aic_player_set_volum(ffmpeg_ctx->aic_player, *set_vol);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_set_volum failed");
                break;
            }
            break;
        case LV_FFMPEG_PLAYER_CMD_GET_VOLUME_EX:
            ret = aic_player_get_volum(ffmpeg_ctx->aic_player, get_vol);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_get_volum failed");
                break;
            }
            break;
        case LV_FFMPEG_PLAYER_CMD_SET_PLAY_TIME_EX:
            ffmpeg_ctx->play_end = 0;
            ret = aic_player_seek(ffmpeg_ctx->aic_player, *seek);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_seek failed");
                break;
            }
            break;
        case LV_FFMPEG_PLAYER_CMD_GET_PLAY_TIME_EX:
            if (ffmpeg_ctx->play_end == 1) {
                *seek = ffmpeg_ctx->media_info.duration;
                return;
            }
            *seek = aic_player_get_play_time(ffmpeg_ctx->aic_player);
            if (*seek < 0) {
                LV_LOG_ERROR("aic_player_get_play_time failed");
                break;
            }
            break;
        case LV_FFMPEG_PLAYED_CMD_KEEP_LAST_FRAME_EX:
            player->keep_last_frame = true;
            break;
#ifdef PLAYER_ASYNC
        /* async src is not currently supported */
        case LV_FFMPEG_PLAYER_CMD_SET_SRC_ASYNC:
            ffmpeg_ctx->play_async = 1;
            break;
        case LV_FFMPEG_PLAYER_WAIT_SRC_ASYNC:
            while(ffmpeg_ctx->play_detected == 0) {
                rt_thread_mdelay(5);
            }
            lv_timer_ready(player->timer);
            break;
#endif
        default:
            LV_LOG_ERROR("Error cmd: %d", cmd);
            break;
    }
}

void lv_ffmpeg_player_set_auto_restart(lv_obj_t * obj, bool en)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;
    player->auto_restart = en;
}

static void lv_ffmpeg_player_frame_update_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = (lv_obj_t *)timer->user_data;
    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;
    struct ffmpeg_context_s *ffmpeg_ctx;

    static lv_point_t point, last_point;
    static lv_coord_t rotation, last_rotation;
    static lv_coord_t width, last_width;
    static lv_coord_t height, last_height;

    ffmpeg_ctx = player->ffmpeg_ctx;
    if(!ffmpeg_ctx) {
        return;
    }

    if (!ffmpeg_ctx->aic_player) {
        return;
    }

    if (ffmpeg_ctx->media_info.has_audio == 1 && ffmpeg_ctx->media_info.has_video == 0) {
        goto update_cb_out;
    }

    lv_ffmpeg_player_get_disp_area(obj, &point, &width, &height, &rotation);

    /* avoid repeatedly setting in callback functions */
    if (last_point.x != point.x || last_point.y != point.y ||
        last_width != width || last_height != height ||
        last_rotation != rotation) {
        lv_ffmpeg_player_protect_disp_area(obj, &point, &width, &height, &rotation);
        lv_ffmpeg_player_set_disp_area(obj, point, width, height, rotation);
        last_point.x = point.x;
        last_point.y = point.y;
        last_height = height;
        last_width = width;
        last_rotation = rotation;
    }

update_cb_out:
    if (player->auto_restart && ffmpeg_ctx->play_end) {
        ffmpeg_ctx->play_end = 0;
        aic_player_seek(ffmpeg_ctx->aic_player, 0);
        LV_LOG_INFO("auto_restart");
    }
}

static void lv_ffmpeg_player_get_disp_area(lv_obj_t *obj, lv_point_t *pos,
                                            lv_coord_t *width, lv_coord_t *height, lv_coord_t *rotation)
{
    lv_area_t coords;

    lv_obj_get_coords(obj, &coords);

    *width = lv_obj_get_width(obj);
    *height = lv_obj_get_height(obj);

    pos->x = coords.x1;
    pos->y = coords.y1;

    *rotation = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
}

static void lv_ffmpeg_player_set_disp_area(lv_obj_t *obj, lv_point_t pos,
                                        lv_coord_t width, lv_coord_t height, lv_coord_t rotation)
{
    lv_ffmpeg_player_t *player = (lv_ffmpeg_player_t *)obj;
    struct ffmpeg_context_s *ffmpeg_ctx = player->ffmpeg_ctx;
    lv_obj_t *mask_img = &player->img.obj;
    struct mpp_rect disp_rect = {0};
    u32 mpp_rotation = MPP_ROTATION_0;

    int ret = -1;
    char fake_image_name[128] = {0};

    snprintf(fake_image_name, 128, "L:/%dx%d_%d_%08x.fake",
             (int)width, (int)height, 0, 0x00000000);

    /* use fake image to change the transparency of the relevant ui layer to 0,
     * so that the video player can not be affected.
     */
    lv_img_set_src(mask_img, fake_image_name);

    disp_rect.x = pos.x;
    disp_rect.y = pos.y;
    disp_rect.width = width;
    disp_rect.height = height;
    ret = aic_player_set_disp_rect(ffmpeg_ctx->aic_player, &disp_rect);
    if (ret != 0) {
        LV_LOG_ERROR("aic_player_set_disp_rect failed");
    }

	if (rotation == 90)
		mpp_rotation = MPP_ROTATION_90;
	if (rotation == 180)
		mpp_rotation = MPP_ROTATION_180;
	if (rotation == 270)
		mpp_rotation = MPP_ROTATION_270;

    ret = aic_player_set_rotation(ffmpeg_ctx->aic_player, mpp_rotation);
    if (ret != 0) {
        LV_LOG_ERROR("aic_player_set_rotation failed");
    }
}

static void lv_ffmpeg_player_protect_disp_area(lv_obj_t *obj, lv_point_t *pos,
                                                    lv_coord_t *width, lv_coord_t *height, lv_coord_t *rotation)
{
    if (pos->x > LV_HOR_RES) {
        pos->x = LV_HOR_RES;
        LV_LOG_ERROR("lv obj pos x too large!");
    }

    if (pos->y > LV_VER_RES) {
        pos->y = LV_VER_RES;
        LV_LOG_ERROR("lv obj pos y too large!");
    }

    if (pos->x + *width > LV_HOR_RES) {
        *width = (LV_HOR_RES - pos->x);
        LV_LOG_ERROR("lv obj width too large!\n");
    }

    if (pos->y + *height > LV_VER_RES) {
        *height = (LV_VER_RES - pos->y);
        LV_LOG_ERROR("lv obj height too large!");
    }

    if (*rotation != 0 && *rotation != 90 && *rotation != 180 && *rotation != 270) {
        LV_LOG_ERROR("aic_player only supported rotation angles, 0, 90, 180, 270, rotation set to 0\n");
        *rotation = 0;
    }
}

static void lv_ffmpeg_player_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

    player->auto_restart = false;
    player->ffmpeg_ctx = NULL;
    player->timer = lv_timer_create(lv_ffmpeg_player_frame_update_cb,
                                    FRAME_DEF_REFR_PERIOD, obj);
    lv_timer_pause(player->timer);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_ffmpeg_player_destructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_ffmpeg_player_t * player = (lv_ffmpeg_player_t *)obj;

    if(player->timer) {
        lv_timer_del(player->timer);
        player->timer = NULL;
    }

    if(player->ffmpeg_ctx) {
        struct ffmpeg_context_s *ffmpeg_ctx;
        ffmpeg_ctx = player->ffmpeg_ctx;
        if (ffmpeg_ctx->aic_player) {
            if (ffmpeg_ctx->has_keep_last_frame) {
                int enable = 0;
                aic_player_control(ffmpeg_ctx->aic_player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
            }
            aic_player_stop(ffmpeg_ctx->aic_player);
            aic_player_destroy(ffmpeg_ctx->aic_player);
            ffmpeg_ctx->aic_player = NULL;
        }
        free(player->ffmpeg_ctx);
        player->ffmpeg_ctx = NULL;
    }

    LV_TRACE_OBJ_CREATE("finished");
}

#endif /*NO LV_USE_FFMPEG*/
