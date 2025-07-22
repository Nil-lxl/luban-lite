/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lv_aic_player.h"
#include "aic_ui.h"

#if defined(AIC_MPP_PLAYER_INTERFACE) && LV_USE_AIC_SIMULATOR == 0
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpp_ge.h"
#include "mpp_fb.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "frame_allocator.h"

#define MY_CLASS &lv_aic_player_class

#define BYTE_ALIGN(x, byte) (((x) + ((byte) - 1))&(~((byte) - 1)))

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    LV_AIC_PLAYER_STATUS_INIT,
    LV_AIC_PLAYER_STATUS_DETECTED,
    LV_AIC_PLAYER_STATUS_READY,
    LV_AIC_PLAYER_STATUS_START,
    LV_AIC_PLAYER_STATUS_RUNNING,
    LV_AIC_PLAYER_STATUS_STOP,
    LV_AIC_PLAYER_STATUS_PAUSE,
    LV_AIC_PLAYER_STATUS_END,
    LV_AIC_PLAYER_STATUS_DELETE,
    _LV_AIC_PLAYER_STATUS_LAST
} lv_aic_player_status;

struct aic_player_ctx_s {
    struct frame_allocator allocator;

    struct aic_player *player;
    struct av_media_info media_info;
    uint32_t status;
    uint32_t draw_layer;

    bool has_keep_last_frame;

    /* mjpeg */
    lv_ll_t frame_ll;
    uint8_t frame_alloc_buffer_times; /* it is only used for callback for functions of aic_player_control's AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_ALLOCATOR */
    unsigned long frame_addr[2];

    uint8_t * image_data;
    struct aicfb_screeninfo screen_info;

    /* backend decoding sync, only used in draw mode performance */
    uint8_t frame_decoding;
    uint8_t * frame_decoded_buf;
    volatile bool frame_thread_exit;
    bool frame_thread_create;
    lv_mutex_t frame_mutex;
    lv_thread_sync_t frame_decode_sync;
    lv_thread_t frame_thread;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void update_video_cb(lv_timer_t * timer);

static void fill_image_color(lv_image_dsc_t *image_dst, lv_color_t color);

static void obtain_fb_info(struct aicfb_screeninfo *screen_info);
static int obtain_align_stride(int width, enum mpp_pixel_format fmt);
static lv_color_format_t switch_mpp_fmt_to_lv_fmt(enum mpp_pixel_format cf);
static enum mpp_pixel_format switch_lv_fmt_to_mpp_fmt(lv_color_format_t cf);

static int player_event_handle(void *app_data, int event, int data1, int data2);
static struct aic_player_ctx_s *player_open(void);
static void player_close(struct aic_player_ctx_s *aic_ctx);
static int player_image_alloc(struct aic_player_ctx_s *aic_ctx);
static void player_image_free(struct aic_player_ctx_s *aic_ctx);
static uint8_t *player_get_image_date(struct aic_player_ctx_s *aic_ctx);
static void player_update_image_date(struct aic_player_ctx_s *aic_ctx);
static int player_auto_get_draw_layer(struct aic_player_ctx_s *aic_ctx);
static int player_hardware_limit(struct aic_player_ctx_s *aic_ctx, uint32_t rotate, uint32_t scale_x, uint32_t scale_y);
static int player_get_frame_sync(struct aic_player_ctx_s * aic_ctx, bool sleep_en);
static int player_get_frame_async(struct aic_player_ctx_s *aic_ctx);
static void player_get_frame_sync_entry(void *ptr);
static int player_put_frame_sync(struct aic_player_ctx_s *aic_ctx);
static int player_put_frame_sync_all(struct aic_player_ctx_s *aic_ctx);
static void player_thread_lock(struct aic_player_ctx_s *aic_ctx);
static void player_thread_unlock(struct aic_player_ctx_s *aic_ctx);

static int lv_player_init_image_resource(lv_obj_t * obj);
static int lv_player_init_image_area(lv_obj_t * obj);

static void lv_aic_player_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_aic_player_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_aic_player_event(const lv_obj_class_t * class_p, lv_event_t * e);

const lv_obj_class_t lv_aic_player_class = {
    .constructor_cb = lv_aic_player_constructor,
    .destructor_cb = lv_aic_player_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .event_cb = lv_aic_player_event,
    .instance_size = sizeof(lv_aic_player_t),
    .base_class = &lv_image_class,
    .name = "lv_aic_player",
};

lv_obj_t * lv_aic_player_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_aic_player_set_draw_layer(lv_obj_t * obj, lv_aic_player_draw_layer_t layer)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    player->draw_layer = layer;
}

lv_res_t lv_aic_player_set_src(lv_obj_t * obj, const char *src)
{
    int res = LV_RES_INV;
    lv_aic_player_t *player = (lv_aic_player_t *)obj;
    lv_image_src_t src_type = lv_image_src_get_type(src);

    if (src_type != LV_IMAGE_SRC_FILE) {
        LV_LOG_ERROR("unknown type");
        goto src_failed;
    }

    if (!player->aic_ctx) {
        player->aic_ctx = player_open();
        if (!player->aic_ctx) {
            goto src_failed;
        }
        player->timer = lv_timer_create(update_video_cb, 5, obj);
        if (!player->timer) {
            goto src_failed;
        }

        _lv_ll_init(&player->aic_ctx->frame_ll, sizeof(struct mpp_frame));
        obtain_fb_info(&player->aic_ctx->screen_info);
    } else {
        player_put_frame_sync_all(player->aic_ctx);
        aic_player_stop(player->aic_ctx->player);
        lv_timer_pause(player->timer);
    }

    struct aic_player_ctx_s * aic_ctx = player->aic_ctx;

    /* skip the driver letter and the possible : after the letter */
    char *real_path = (char *)src;
    if (real_path[1] == ':' && ((real_path[0] >= 'A' && real_path[0] <= 'Z') || (real_path[0] >= 'a' && real_path[0] <= 'z'))) {
        real_path += 2;
    }

    res = aic_player_set_uri(aic_ctx->player, real_path);
    if (res) {
        LV_LOG_ERROR("aic_player_set_uri failed, path = %s", real_path);
        goto src_failed;
    }

    res = aic_player_prepare_sync(aic_ctx->player);
    if (res) {
        LV_LOG_ERROR("aic_player_prepare sync failed");
        goto src_failed;
    }

    res = aic_player_get_media_info(aic_ctx->player, &aic_ctx->media_info);
    if (res) {
        LV_LOG_ERROR("aic_player_get_media_info failed");
        goto src_failed;
    }

    if (aic_ctx->media_info.has_audio == 1 && aic_ctx->media_info.has_video == 0) {
        aic_ctx->draw_layer = LV_AIC_PLAYER_LAYER_NONE;
    } else {
        if (player->draw_layer == LV_AIC_PLAYER_LAYER_DEFAULT) {
            aic_ctx->draw_layer = player_auto_get_draw_layer(aic_ctx);
        } else {
            aic_ctx->draw_layer = player->draw_layer;
        }
#ifndef AIC_MPP_PLAYER_VE_USE_FILL_FB
        if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF ||
            aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
            LV_LOG_ERROR("draw layer ui must open AIC_MPP_PLAYER_VE_USE_FILL_FB");
            goto src_failed;
        }
#else
        if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO) {
            LV_LOG_ERROR("draw layer video must close AIC_MPP_PLAYER_VE_USE_FILL_FB");
            goto src_failed;
        }
#endif
    }

    if ((aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF ||
        aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF)) {
        res = lv_player_init_image_resource(obj);
        if (res < 0) {
            LV_LOG_ERROR("lv_player_init_image_resource failed");
            goto src_failed;
        }
    }

    /* here are the video related settings */
    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO &&
        player->keep_last_frame && !aic_ctx->has_keep_last_frame) {
        int enable = 1;
        aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
        aic_ctx->has_keep_last_frame = true;
    }

    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO) {
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x0), 0);
        lv_player_init_image_area(obj);
    } else if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF ||
               aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
        lv_color_t bg_color = lv_color_hex(0x0);
        fill_image_color(&player->image_dst, bg_color);

        lv_image_set_src(obj, &player->image_dst);
    }

    lv_timer_resume(player->timer);

    aic_ctx->status = LV_AIC_PLAYER_STATUS_READY;

    return LV_RES_OK;

src_failed:
    if (player->timer) {
        lv_timer_pause(player->timer);
    }

    if (player->aic_ctx) {
        aic_player_stop(player->aic_ctx->player);
    }
    return LV_RES_INV;
}

void lv_aic_player_set_cmd(lv_obj_t * obj, lv_aic_player_cmd_t cmd, void *data)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;
    int ret;

    bool *end = (bool *)data;
    struct av_media_info *media_info = (struct av_media_info *)data;
    s32 *set_vol = (s32 *)data;
    s32 *get_vol = (s32 *)data;
    u64 *seek = (u64 *)data;

    aic_ctx = player->aic_ctx;
    if (!player->aic_ctx) {
        LV_LOG_WARN("player is NULL, please set src first");
        return;
    }

    switch(cmd) {
        case LV_AIC_PLAYER_CMD_START:
            ret = aic_player_start(aic_ctx->player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_start failed");
                break;
            }
            lv_timer_ready(player->timer);
            aic_ctx->status = LV_AIC_PLAYER_STATUS_START;
            break;
        case LV_AIC_PLAYER_CMD_STOP:
            ret = aic_player_stop(aic_ctx->player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_stop failed");
            }
            aic_ctx->status = LV_AIC_PLAYER_STATUS_STOP;
            break;
        case LV_AIC_PLAYER_CMD_PAUSE:
            ret = aic_player_pause(aic_ctx->player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_pause failed");
            }
            aic_ctx->status = LV_AIC_PLAYER_STATUS_PAUSE;
            break;
        case LV_AIC_PLAYER_CMD_RESUME:
            ret = aic_player_play(aic_ctx->player);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_start failed");
                break;
            }
            lv_timer_ready(player->timer);
            aic_ctx->status = LV_AIC_PLAYER_STATUS_RUNNING;
            break;
        case LV_AIC_PLAYER_CMD_PLAY_END:
            *end = aic_ctx->status == LV_AIC_PLAYER_STATUS_END ? true : false;
            break;
        case LV_AIC_PLAYER_CMD_GET_MEDIA_INFO:
            memcpy(media_info, &aic_ctx->media_info, sizeof(struct av_media_info));
            break;
        case LV_AIC_PLAYER_CMD_SET_VOLUME:
            ret = aic_player_set_volum(aic_ctx->player, *set_vol);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_set_volum failed");
            }
            break;
        case LV_AIC_PLAYER_CMD_GET_VOLUME:
            ret = aic_player_get_volum(aic_ctx->player, get_vol);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_get_volum failed");
            }
            break;
        case LV_AIC_PLAYER_CMD_SET_PLAY_TIME:
            ret = aic_player_seek(aic_ctx->player, *seek);
            if (ret != 0) {
                LV_LOG_ERROR("aic_player_seek failed");
                break;
            }
            lv_timer_ready(player->timer);
            aic_ctx->status = LV_AIC_PLAYER_STATUS_RUNNING;
            break;
        case LV_AIC_PLAYER_CMD_GET_PLAY_TIME:
            if (aic_ctx->status == LV_AIC_PLAYER_STATUS_END) {
                *seek = aic_ctx->media_info.duration;
                return;
            }
            *seek = aic_player_get_play_time(aic_ctx->player);
            if (*seek < 0) {
                LV_LOG_ERROR("aic_player_get_play_time failed");
            }
            break;
        case LV_AIC_PLAYER_CMD_KEEP_LAST_FRAME_EX:
            player->keep_last_frame = true;
            break;
        default:
            LV_LOG_ERROR("Error cmd: %d", cmd);
            break;
    }
}

void lv_aic_player_set_auto_restart(lv_obj_t * obj, bool en)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    player->auto_restart = en;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int player_get_frame_sync(struct aic_player_ctx_s * aic_ctx, bool sleep_en)
{
    s32 ret = 0;
    struct mpp_frame frame = {0};
    struct mpp_frame *p_frame = NULL;

    player_thread_lock(aic_ctx);
    aic_ctx->frame_decoding = true;
    player_thread_unlock(aic_ctx);

    player_put_frame_sync(aic_ctx);

    do {
        ret = aic_player_get_frame(aic_ctx->player, (void *)&frame); /* automatically sync audio and video */
        if (!ret) {
            p_frame = (struct mpp_frame *)_lv_ll_ins_tail(&aic_ctx->frame_ll);
            lv_memcpy(p_frame, &frame, sizeof(struct mpp_frame));
            LV_LOG_INFO("get frame %p, id = %d", (uint8_t *)(uintptr_t)p_frame->buf.phy_addr[0], p_frame->id);
            if (frame.flags & FRAME_FLAG_EOS)
                aic_ctx->status = LV_AIC_PLAYER_STATUS_END;
        } else if (ret == DEC_NO_EMPTY_PACKET || ret == DEC_NO_READY_PACKET) {
            if (sleep_en == false) {
                return -1;
            }
            aicos_msleep(1);
        } else {
            LV_LOG_ERROR("get frame failed, ret = %d", ret);
            return -1;
        }
    } while(ret == DEC_NO_EMPTY_PACKET || ret == DEC_NO_READY_PACKET);

    player_thread_lock(aic_ctx);
    aic_ctx->frame_decoded_buf = (uint8_t *)(uintptr_t)p_frame->buf.phy_addr[0];
    aic_ctx->frame_decoding = false;
    player_thread_unlock(aic_ctx);
    return 0;
}

static int player_get_frame_async(struct aic_player_ctx_s * aic_ctx)
{
    lv_thread_sync_signal(&aic_ctx->frame_decode_sync);
    return 0;
}

static int player_put_frame_sync(struct aic_player_ctx_s *aic_ctx)
{
    s32 ret = 0;
    struct mpp_frame *p_frame = NULL;

    int push_frame_num = 1;
    /* Ensure that all frames are fully captured, then start from the beginning again,
       making sure that the frames used are only those that are decoded. */
    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF)
        push_frame_num = 2;

    if (_lv_ll_get_len(&aic_ctx->frame_ll) >= push_frame_num) {
        p_frame = (struct mpp_frame *)_lv_ll_get_head(&aic_ctx->frame_ll);
        ret = aic_player_put_frame(aic_ctx->player, (void *)p_frame);
        if (ret)
            LV_LOG_ERROR("put frame failed, ret = %d", ret);
        else
            LV_LOG_INFO("put frame %p, id = %d", (uint8_t *)(uintptr_t)p_frame->buf.phy_addr[0], p_frame->id);
        _lv_ll_remove(&aic_ctx->frame_ll, p_frame);
        lv_free(p_frame);
    }
    return 0;
}

static int player_put_frame_sync_all(struct aic_player_ctx_s * aic_ctx)
{
    s32 ret;
    while (_lv_ll_is_empty(&aic_ctx->frame_ll) == false) {
        struct mpp_frame *p_frame = _lv_ll_get_head(&aic_ctx->frame_ll);
        ret = aic_player_put_frame(aic_ctx->player, (void *)p_frame);
        if (ret)
            LV_LOG_ERROR("put frame failed, ret = %d", ret);
        else
            LV_LOG_INFO("put frame %p, id = %d", (uint8_t *)(uintptr_t)p_frame->buf.phy_addr[0], p_frame->id);
        _lv_ll_remove(&aic_ctx->frame_ll, p_frame);
        lv_free(p_frame);
    }

    return 0;
}

static void player_thread_lock(struct aic_player_ctx_s *aic_ctx)
{
    if (aic_ctx->frame_thread_create)
        lv_mutex_lock(&aic_ctx->frame_mutex);
}

static void player_thread_unlock(struct aic_player_ctx_s *aic_ctx)
{
    if (aic_ctx->frame_thread_create)
        lv_mutex_unlock(&aic_ctx->frame_mutex);
}

static void player_get_frame_sync_entry(void *ptr)
{
    lv_aic_player_t *player = (lv_aic_player_t *)ptr;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;

    while (!aic_ctx->frame_thread_exit) {
        lv_thread_sync_wait(&aic_ctx->frame_decode_sync);
        if (aic_ctx->frame_thread_exit) break;
        player_get_frame_sync(aic_ctx, -1);
    }
}

static void update_video_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = (lv_obj_t *)timer->user_data;
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;

    if (!aic_ctx)
        return;

    if (player->auto_restart && aic_ctx->status == LV_AIC_PLAYER_STATUS_END) {
        aic_player_seek(aic_ctx->player, 0);
        aic_ctx->status = LV_AIC_PLAYER_STATUS_RUNNING;
        return;
    }

    if (!(aic_ctx->status == LV_AIC_PLAYER_STATUS_START || aic_ctx->status == LV_AIC_PLAYER_STATUS_RUNNING))
        return;

    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF) {
        player_get_frame_sync(aic_ctx, false);
    } else if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
        /* first frame */
        if (_lv_ll_get_len(&aic_ctx->frame_ll) == 0) {
            player_get_frame_sync(aic_ctx, false);
        }

        if (aic_ctx->frame_decoding == true)
            return;

        player_get_frame_async(aic_ctx);
    } else {
        return;
    }

    lv_obj_invalidate(obj);

    player_update_image_date(aic_ctx);
    uint8_t * img_data = player_get_image_date(aic_ctx);
    player->image_dst.data = img_data;

    lv_image_cache_drop(lv_image_get_src(obj)); /* clear the cache to make img_dst update automatically */
}

static int player_event_handle(void *app_data, int event, int data1, int data2)
{
    int ret = 0;
    struct aic_player_ctx_s *aic_ctx = (struct aic_player_ctx_s *)app_data;

    switch(event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            aic_ctx->status = LV_AIC_PLAYER_STATUS_END;
            break;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            aic_ctx->status = LV_AIC_PLAYER_STATUS_DETECTED;
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            break;
        default:
            break;
    }
    return ret;
}

static struct aic_player_ctx_s *player_open(void)
{
    struct aic_player_ctx_s *aic_ctx = calloc(1, sizeof(struct aic_player_ctx_s));
    if(aic_ctx == NULL) {
        LV_LOG_ERROR("aic_ctx malloc failed");
        return NULL;
    }

    aic_ctx->player = aic_player_create(NULL);
    if (!aic_ctx->player) {
        LV_LOG_ERROR("create aic player failed");
        lv_mem_free(aic_ctx);
        return NULL;
    }

    aic_player_set_event_callback(aic_ctx->player, aic_ctx, player_event_handle);

    return aic_ctx;
}

static void player_close(struct aic_player_ctx_s *aic_ctx)
{
    if (aic_ctx->player) {
        if (aic_ctx->frame_thread_create) {
            aic_ctx->frame_thread_exit = true;
            lv_thread_sync_signal(&aic_ctx->frame_decode_sync);
            lv_thread_delete(&aic_ctx->frame_thread);
            lv_thread_sync_delete(&aic_ctx->frame_decode_sync);
            lv_mutex_delete(&aic_ctx->frame_mutex);
        }

        if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO && aic_ctx->has_keep_last_frame) {
            int enable = 0;
            aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
        }
        aic_player_stop(aic_ctx->player);
        aic_player_destroy(aic_ctx->player);
    }
    free(aic_ctx);
    aic_ctx = NULL;
}

static void obtain_fb_info(struct aicfb_screeninfo *screen_info)
{
    struct mpp_fb *fb = mpp_fb_open();
    int ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, screen_info);
    if (ret) {
        LV_LOG_ERROR("get screen info failed");
    }
    mpp_fb_close(fb);
}

static lv_color_format_t switch_mpp_fmt_to_lv_fmt(enum mpp_pixel_format cf)
{
    lv_color_format_t fmt = LV_COLOR_FORMAT_ARGB8888;

    switch(cf) {
        case MPP_FMT_RGB_565:
            fmt = LV_COLOR_FORMAT_RGB565;
            break;
        case MPP_FMT_RGB_888:
            fmt = LV_COLOR_FORMAT_RGB888;
            break;
        case MPP_FMT_ARGB_8888:
            fmt = LV_COLOR_FORMAT_ARGB8888;
            break;
        case MPP_FMT_XRGB_8888:
            fmt = LV_COLOR_FORMAT_XRGB8888;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }

    return fmt;
}

static enum mpp_pixel_format switch_lv_fmt_to_mpp_fmt(lv_color_format_t cf)
{
    enum mpp_pixel_format fmt = MPP_FMT_ARGB_8888;

    switch(cf) {
        case LV_COLOR_FORMAT_RGB565:
            fmt = MPP_FMT_RGB_565;
            break;
        case LV_COLOR_FORMAT_RGB888:
            fmt = MPP_FMT_RGB_888;
            break;
        case LV_COLOR_FORMAT_ARGB8888:
            fmt = MPP_FMT_ARGB_8888;
            break;
        case LV_COLOR_FORMAT_XRGB8888:
            fmt = MPP_FMT_XRGB_8888;
            break;
        default:
            LV_LOG_ERROR("unsupported format:%d", cf);
            break;
    }

    return fmt;
}

static int obtain_align_stride(int width, enum mpp_pixel_format fmt)
{
    int stride;

    switch(fmt) {
        case MPP_FMT_RGB_565:
            stride = BYTE_ALIGN((width) * 2, 32);
            break;
        case MPP_FMT_RGB_888:
            stride = BYTE_ALIGN((width) * 3, 48);
            break;
        case MPP_FMT_ARGB_8888:
            stride = BYTE_ALIGN((width) * 4, 64);
            break;
        case MPP_FMT_XRGB_8888:
            stride = BYTE_ALIGN((width) * 4, 64);
            break;
        default:
            stride = BYTE_ALIGN((width) * 4, 16);
            LV_LOG_ERROR("unsupported format:%d", fmt);
            break;
    }
    return stride;
}

static int alloc_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame,
                              int width, int height, enum mpp_pixel_format format)
{
    struct aic_player_ctx_s * aic_ctx = (struct aic_player_ctx_s *)p;

    int screen_format;
    int stride;
    int alloc_buffer_times = aic_ctx->frame_alloc_buffer_times;

    int media_width = aic_ctx->media_info.video_stream.width;
    int media_height = aic_ctx->media_info.video_stream.height;
    screen_format = aic_ctx->screen_info.format;
    stride = obtain_align_stride(media_width, screen_format);

    frame->buf.format = screen_format;
    frame->buf.size.width = media_width;
    frame->buf.size.height = media_height;
    frame->buf.stride[0] = stride;
    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.phy_addr[0] = (unsigned long)aic_ctx->frame_addr[alloc_buffer_times];

    aic_ctx->frame_alloc_buffer_times++;
    return 0;
}

static int free_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    return 0;
}

static int close_allocator(struct frame_allocator *p)
{
    return 0;
}

static struct alloc_ops frame_buffer_alloc_ops = {
    .alloc_frame_buffer = alloc_player_frame_buffer,
    .free_frame_buffer = free_player_frame_buffer,
    .close_allocator = close_allocator,
};

static int player_image_alloc(struct aic_player_ctx_s *aic_ctx)
{
    int i = 0;
    s32 ret = 0;

    int width = BYTE_ALIGN(aic_ctx->media_info.video_stream.width, 16);
    int height = BYTE_ALIGN(aic_ctx->media_info.video_stream.height, 16);

    int screen_format = aic_ctx->screen_info.format;
    int stride = obtain_align_stride(width, screen_format);

    s32 alloc_times = 1;
    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
        alloc_times = 2;
    }

    for (i = 0; i < alloc_times; i++) {
        aic_ctx->frame_addr[i] = (unsigned long)mpp_phy_alloc(stride * height);
        if (!aic_ctx->frame_addr[i]) {
            LV_LOG_ERROR("mpp_phy_alloc failed, i = %d, alloc_times = %d", i, alloc_times);
            player_image_free(aic_ctx);
            return -1;
        }
    }

    s32 ext_frame_buffer_num = alloc_times - 1;
    ret = aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_NUM,
                             (void *)&ext_frame_buffer_num);
    if (ret != 0) {
        LV_LOG_ERROR("player set vdec ext frame num failed %d", ret);
        return -1;
    }

    aic_ctx->frame_alloc_buffer_times = 0;
    aic_ctx->allocator.ops = &frame_buffer_alloc_ops;
    ret = aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_ALLOCATOR,
                             (void *)aic_ctx);
    if (ret != 0) {
        LV_LOG_ERROR("player set vdec ext frame alloc failed %d", ret);
        return -1;
    }

    return 0;
}

static void player_image_free(struct aic_player_ctx_s *aic_ctx)
{
    int i = 0;
    for (i = 0; i < 2; i++) {
        if (aic_ctx->frame_addr[i] != 0)
            mpp_phy_free(aic_ctx->frame_addr[i]);
        aic_ctx->frame_addr[i] = 0;
    }
}

static void player_update_image_date(struct aic_player_ctx_s *aic_ctx)
{
    player_thread_lock(aic_ctx);
    aic_ctx->image_data = aic_ctx->frame_decoded_buf;
    player_thread_unlock(aic_ctx);
}

static uint8_t *player_get_image_date(struct aic_player_ctx_s *aic_ctx)
{
    return aic_ctx->image_data;
}

static int player_auto_get_draw_layer(struct aic_player_ctx_s *aic_ctx)
{
    int draw_layer = LV_AIC_PLAYER_LAYER_VIDEO;

#ifdef PRJ_CHIP
    if (aic_ctx->media_info.has_video == 1) {
        if (lv_strcmp(PRJ_CHIP, "d12x") == 0) {
            draw_layer = LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF;
        } else if ((lv_strcmp(PRJ_CHIP, "d13x") == 0)) {
            draw_layer = LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF;
        } else if ((lv_strcmp(PRJ_CHIP, "d21x") == 0)) {
            draw_layer = LV_AIC_PLAYER_LAYER_VIDEO;
        }
    }
#endif

    return draw_layer;
}

static int player_hardware_limit(struct aic_player_ctx_s *aic_ctx, uint32_t rotate, uint32_t scale_x, uint32_t scale_y)
{
#ifdef PRJ_CHIP
    if (lv_strcmp(PRJ_CHIP, "d12x") == 0) {
        return true;
    } else if ((lv_strcmp(PRJ_CHIP, "d13x") == 0)) {
        if ((scale_x == LV_SCALE_NONE && scale_y == LV_SCALE_NONE) &&
            (rotate == 900 || rotate == 1800 || rotate == 2700))
            return false;
        else
            return true;
    } else if ((lv_strcmp(PRJ_CHIP, "d21x") == 0) && !(rotate == 900 || rotate == 1800 || rotate == 2700)) {
        return true;
    }
#endif
    return false;
}

static int lv_player_init_image_resource(lv_obj_t * obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;

    player_image_free(aic_ctx);
    if (player_image_alloc(aic_ctx) < 0) {
        LV_LOG_ERROR("player_image_alloc failed");
        return -1;
    }

    aic_ctx->image_data = (uint8_t *)aic_ctx->frame_addr[0];

    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF &&
        aic_ctx->frame_thread_create == false) {
        lv_thread_init(&aic_ctx->frame_thread, 20, player_get_frame_sync_entry, 4 * 1024, player);
        lv_thread_sync_init(&aic_ctx->frame_decode_sync);
        lv_mutex_init(&aic_ctx->frame_mutex);
        aic_ctx->frame_thread_create = true;
    }

    int width = aic_ctx->media_info.video_stream.width;
    int height = aic_ctx->media_info.video_stream.height;
    int screen_format = aic_ctx->screen_info.format;
    int stride = obtain_align_stride(width, screen_format);
    uint8_t * img_data = player_get_image_date(aic_ctx);

    /* decode into screen format */
    player->image_dst.header.w = width;
    player->image_dst.header.h = height;
    player->image_dst.header.cf = switch_mpp_fmt_to_lv_fmt(screen_format);
    player->image_dst.header.stride = stride;
    player->image_dst.header.flags = 0;
    player->image_dst.header.magic = LV_IMAGE_HEADER_MAGIC;
    player->image_dst.data = img_data;
    player->image_dst.data_size = stride * height;

    player->image.antialias = 0;
    player->image.blend_mode = LV_BLEND_MODE_NORMAL;
    player->image.w = width;
    player->image.h = height;

    return 0;
}

/* decoding and display are managed by aic_player itself;
   here, we only to ensure that the video layer is not overlapped by the ui layer. */
static int lv_player_init_image_area(lv_obj_t * obj)
{
    static lv_area_t last_area = {0};

    char fake_image_src[128] = {0};

    lv_aic_player_t *player = (lv_aic_player_t *)obj;
    lv_image_t *image = (lv_image_t *)player;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;

    if (!aic_ctx || aic_ctx->draw_layer != LV_AIC_PLAYER_LAYER_VIDEO)
        return -1;

    lv_obj_update_layout(obj);

    lv_point_t pivot_px = {0};
    lv_image_get_pivot(obj, &pivot_px);

    /* obtain the actual display area  */
    int video_width = player->aic_ctx->media_info.video_stream.width;
    int video_height = player->aic_ctx->media_info.video_stream.height;
    lv_area_t area = {obj->coords.x1, obj->coords.y1,
                      obj->coords.x1 + image->w - 1, obj->coords.y1 + image->h - 1};
    if(image->align < _LV_IMAGE_ALIGN_AUTO_TRANSFORM) {
        lv_area_align(&obj->coords, &area, image->align, image->offset.x, image->offset.y);
    } else if(image->align == LV_IMAGE_ALIGN_TILE) {
        image->align = LV_IMAGE_ALIGN_TOP_LEFT;
        LV_LOG_WARN("draw layer video does not supports LV_IMAGE_ALIGN_TILE alignment, automatic set LV_IMAGE_ALIGN_TOP_LEFT");
    }

    lv_area_t real_area = {0};
    lv_memcpy(&real_area, &area, sizeof(lv_area_t));
    _lv_image_buf_get_transformed_area(&real_area, lv_area_get_width(&area), lv_area_get_height(&area),
                                       image->rotation, image->scale_x,
                                       image->scale_y, &pivot_px);
    lv_area_move(&real_area, area.x1, area.y1);

    if (last_area.x1 == real_area.x1 && last_area.x2 == real_area.x2 &&
        last_area.y1 == real_area.y1 && last_area.y2 == real_area.y2)
        return -1;
    else
        lv_memcpy(&last_area, &real_area, sizeof(lv_area_t));

    int alpha_en = 0;
    int image_width = ((int)real_area.x2 - (int)real_area.x1) + 1;
    int image_height = ((int)real_area.y2 - (int)real_area.y1) + 1;
    uint8_t bg_opa = lv_obj_get_style_bg_opa(obj, LV_PART_MAIN);
    lv_color_t bg_color = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
    uint32_t color = (bg_opa << 24) | (bg_color.red << 16) |
                    (bg_color.green << 8) | (bg_color.blue);
    memset(fake_image_src, 0, sizeof(fake_image_src));
    snprintf(fake_image_src, sizeof(fake_image_src), "L:/%dx%d_%d_%08x.fake",
            image_width, image_height, alpha_en, (unsigned int)color);
    lv_image_set_src(obj, fake_image_src);
    image->w        = video_width;
    image->h        = video_height;

    /* update the video layer */
    struct mpp_rect disp_rect = {0};
    disp_rect.x = real_area.x1;
    disp_rect.y = real_area.y1;
    disp_rect.width = real_area.x2 - real_area.x1 + 1;
    disp_rect.height = real_area.y2 - real_area.y1 + 1;

    int ret = -1;
    if (disp_rect.x > LV_HOR_RES || disp_rect.x < 0) {
        LV_LOG_ERROR("obj pos x is error, must be within [0,%d], x = %d", (int)LV_HOR_RES, (int)disp_rect.x);
        return -1;
    }

    if (disp_rect.y > LV_VER_RES || disp_rect.y < 0) {
        LV_LOG_ERROR("obj pos y is error, must be within [0,%d], y = %d", (int)LV_VER_RES, (int)disp_rect.y);
        return -1;
    }

    if (disp_rect.x + disp_rect.width > LV_HOR_RES) {
        LV_LOG_ERROR("display actual area is error, must be within [0,%d], x = %d, width = %d", (int)LV_VER_RES, (int)disp_rect.x, (int)disp_rect.width);
        return -1;
    }

    if (disp_rect.y + disp_rect.height > LV_VER_RES) {
        LV_LOG_ERROR("display actual area is error, must be within [0,%d], y = %d, height = %d", (int)LV_VER_RES, (int)disp_rect.y, (int)disp_rect.height);
        return -1;
    }

    ret = aic_player_set_disp_rect(aic_ctx->player, &disp_rect);
    if (ret != 0) {
        LV_LOG_ERROR("aic_player_set_disp_rect failed");
        return -1;
    }

    if (image->rotation == 0)
        return -1;

    if (player_hardware_limit(aic_ctx, image->rotation, image->scale_x, image->scale_y) == true) {
        LV_LOG_ERROR("Hardware functionality is limited, rotation cannot be used");
        return -1;
    }

    u32 mpp_rotation = MPP_ROTATION_0;
    if (image->rotation == 900)
        mpp_rotation = MPP_ROTATION_90;
    if (image->rotation == 1800)
        mpp_rotation = MPP_ROTATION_180;
    if (image->rotation == 2700)
        mpp_rotation = MPP_ROTATION_270;
    ret = aic_player_set_rotation(aic_ctx->player, mpp_rotation);
    if (ret != 0) {
        LV_LOG_ERROR("aic_player_set_rotation failed, rotate = %d", (int)image->rotation);
        return -1;
    }

    return 0;
}

static void fill_image_color(lv_image_dsc_t *image_dst, lv_color_t color)
{
    int ret = 0;
    struct mpp_ge *ge = NULL;
    struct ge_fillrect fill = {0};

    ge = mpp_ge_open();
    if (!ge) {
        LV_LOG_ERROR("ge open fail\n");
        return;
    }

    fill.start_color = lv_color_to_u32(color);
    fill.end_color = 0;

    /* dst buf */
    fill.dst_buf.buf_type = MPP_PHY_ADDR;
    fill.dst_buf.phy_addr[0] = (int32_t)(long)image_dst->data;
    fill.dst_buf.stride[0] = image_dst->header.stride;
    fill.dst_buf.size.width = image_dst->header.w;
    fill.dst_buf.size.height = image_dst->header.h;
    fill.dst_buf.format = switch_lv_fmt_to_mpp_fmt(image_dst->header.cf);
    fill.dst_buf.crop_en = 0;

    ret =  mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        LV_LOG_ERROR("ge fillrect fail\n");
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        LV_LOG_ERROR("ge emit fail\n");
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        LV_LOG_ERROR("ge sync fail\n");
    }
}

static void lv_aic_player_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    player->auto_restart = false;
    player->aic_ctx      = NULL;
    player->draw_layer   = LV_AIC_PLAYER_LAYER_DEFAULT;
    player->timer        = NULL;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_aic_player_destructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    if (!player->aic_ctx)
        return;

    _lv_ll_clear(&player->aic_ctx->frame_ll);

    player_put_frame_sync_all(player->aic_ctx);

    if (player->timer)
        lv_timer_del(player->timer);

    if (player->aic_ctx)
        player_image_free(player->aic_ctx);

    if (player->aic_ctx)
        player_close(player->aic_ctx);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_aic_player_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);
    /*Call the ancestor's event handler*/
    lv_result_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if(code == LV_EVENT_SIZE_CHANGED || code == LV_EVENT_STYLE_CHANGED) {
        lv_player_init_image_area(obj);
    }
}
#elif LV_USE_AIC_SIMULATOR == 1
lv_obj_t * lv_aic_player_create(lv_obj_t * parent)
{
    return lv_ffmpeg_player_create(parent);
}

void lv_aic_player_set_draw_layer(lv_obj_t * obj, lv_aic_player_draw_layer_t layer)
{
    return;
}

lv_res_t lv_aic_player_set_src(lv_obj_t * obj, const char * src)
{
    return lv_ffmpeg_player_set_src(obj, src);
}

void lv_aic_player_set_cmd(lv_obj_t * obj, lv_aic_player_cmd_t cmd, void *data)
{
    return lv_ffmpeg_player_set_cmd(obj, cmd);
}

void lv_aic_player_set_auto_restart(lv_obj_t * obj, bool en)
{
    return lv_ffmpeg_player_set_auto_restart(obj, en);
}
#endif

#if defined(AIC_MPP_PLAYER_INTERFACE) || LV_USE_AIC_SIMULATOR == 1
void lv_aic_player_set_pivot(lv_obj_t * obj, int32_t x, int32_t y)
{
    lv_image_set_pivot(obj, x, y);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_rotation(lv_obj_t * obj, int32_t angle)
{
    lv_image_set_rotation(obj, angle);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_width(lv_obj_t *obj, uint32_t width)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;
    if (!aic_ctx) {
        LV_LOG_WARN("player is NULL, please set src first");
        return;
    }

    int video_width = aic_ctx->media_info.video_stream.width;
    uint32_t scale_x = ((double)(width * 1.0) / (double)(video_width * 1.0)) * 256;
    int16_t errors = ((scale_x * 1.0) * video_width/ 256) - width;
    if (errors)
        LV_LOG_WARN("player scaling width has precision errors = %d", errors);
    lv_image_set_scale_x(obj, scale_x);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_height(lv_obj_t *obj, uint32_t height)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    struct aic_player_ctx_s *aic_ctx = player->aic_ctx;
    if (!aic_ctx) {
        LV_LOG_WARN("player is NULL, please set src first");
        return;
    }

    int video_height = aic_ctx->media_info.video_stream.height;
    uint32_t scale_y = ((double)(height * 1.0) / (double)(video_height * 1.0)) * 256;
    int16_t errors = ((scale_y * 1.0) * video_height) / 256 - height;
    if (errors)
        LV_LOG_WARN("player scaling height has precision errors = %d", errors);
    lv_image_set_scale_y(obj, scale_y);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_scale(lv_obj_t * obj, uint32_t scale)
{
    lv_image_set_scale(obj, scale);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_scale_x(lv_obj_t * obj, uint32_t scale_x)
{
    lv_image_set_scale_x(obj, scale_x);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_scale_y(lv_obj_t * obj, uint32_t scale_y)
{
    lv_image_set_scale_y(obj, scale_y);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_offset_x(lv_obj_t * obj, int32_t x)
{
    lv_image_set_offset_x(obj, x);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_offset_y(lv_obj_t * obj, int32_t y)
{
    lv_image_set_offset_y(obj, y);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_inner_align(lv_obj_t * obj, lv_image_align_t align)
{
    lv_image_set_inner_align(obj, align);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_get_pivot(lv_obj_t * obj, lv_point_t * pivot)
{
    return lv_image_get_pivot(obj, pivot);
}

int32_t lv_aic_player_get_rotation(lv_obj_t * obj)
{
    return lv_image_get_rotation(obj);
}

int32_t lv_aic_player_get_scale(lv_obj_t * obj)
{
    return lv_image_get_scale(obj);
}

int32_t lv_aic_player_get_scale_x(lv_obj_t * obj)
{
    return lv_image_get_scale_x(obj);
}

int32_t lv_aic_player_get_scale_y(lv_obj_t * obj)
{
    return lv_image_get_scale_y(obj);
}

int32_t lv_aic_player_get_offset_x(lv_obj_t * obj)
{
    return lv_image_get_offset_x(obj);
}

int32_t lv_aic_player_get_offset_y(lv_obj_t * obj)
{
    return lv_image_get_offset_y(obj);
}

lv_image_align_t lv_aic_player_get_inner_align(lv_obj_t * obj)
{
    return lv_image_get_inner_align(obj);
}
#endif
