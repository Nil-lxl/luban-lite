/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "player_backend_ops.h"
#include "aic_ui.h"
#if LV_USE_AIC_SIMULATOR == 0
#include "../lv_aic_player.h"

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "mpp_ge.h"
#include "mpp_fb.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "frame_allocator.h"
#include "backend_common.h"

#define FRAME_TIMEOUT         200
#define DECODER_NOT_CREATED   4099
#define FRAME_CAPTURE_TIMEOUT 4100

#define CHECK_DATA_OR_RETURN(data, cmd) \
    do { \
        if (!(data)) { \
            LV_LOG_WARN("%s: data is NULL", #cmd); \
            return LV_RES_INV; \
        } \
    } while (0)
/**********************
 *      TYPEDEFS
 **********************/
struct aic_player_ctx {
    struct frame_allocator allocator;

    struct aic_player *player;
    struct av_media_info media_info;

    atomic_int status;
    uint16_t draw_layer;

    bool keep_last_frame;

    /* mjpeg */
    lv_ll_t frame_ll;
    uint8_t allocated_frame_count; /* it is only used for callback for functions of aic_player_control's AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_ALLOCATOR */
    unsigned long phy_frame_addr[2];

    void *image_src;
    uint8_t *image_data;
    struct aicfb_screeninfo screen_info;
    struct mpp_buf buf; /* only used by v8 for drawing images */

    /* backend decoding sync, only used in draw mode performance */
    uint8_t decoding;
    bool stop_thread;
    bool has_new_request;

    lv_area_t area;
    int16_t image_rotation;
    int16_t disp_rotation;
    pthread_cond_t frame_cond;
    pthread_mutex_t frame_mutex;
    pthread_t frame_thread;
};

/* Core interface functions */
static void * aic_player_backend_create();
static void aic_player_backend_destroy(void *ctx);
static lv_res_t aic_player_backend_set_src(void *ctx, const char *src);
static lv_res_t aic_player_backend_control(void *ctx, player_cmd_t cmd, void *data);

/* Auxiliary drawing and format conversion functions */
static int check_player_configuration(uint32_t layer);

/* Core player functionality functions */
static int player_alloc_image_buf(struct aic_player_ctx *aic_ctx);
static int player_free_image_buf(struct aic_player_ctx *aic_ctx);
static int player_alloc_frame_buffer(struct aic_player_ctx *aic_ctx);
static void player_free_frame_buffer(struct aic_player_ctx *aic_ctx);
static uint8_t *player_get_image_date(struct aic_player_ctx *aic_ctx);
static int player_select_layer(struct aic_player_ctx *aic_ctx);
static int player_check_hw_capability(uint32_t rotate, uint32_t scale_x, uint32_t scale_y);
static void player_resource_cleanup(struct aic_player_ctx *aic_ctx);

/* Decoding-related functions */
static int player_get_frame(struct aic_player_ctx * aic_ctx);
static int player_get_frame_send_signal(struct aic_player_ctx *aic_ctx);
static int player_put_frame(struct aic_player_ctx *aic_ctx);
static int player_put_frame_all(struct aic_player_ctx *aic_ctx);

/* Thread and callback functions */
static int player_decode_thread_create(struct aic_player_ctx *aic_ctx);
static void player_decode_thread_destroy(struct aic_player_ctx *aic_ctx);
static void player_thread_lock(struct aic_player_ctx *aic_ctx);
static void player_thread_unlock(struct aic_player_ctx *aic_ctx);
static int aic_player_event_callback(void *ctx, int event_type, int param1, int param2);
static void *player_decode_entry(void *ptr);

/* Player command handlers */
static lv_res_t player_handle_update_display_area(void *ctx);
static lv_res_t player_handle_get_frame(void *ctx);

const player_backend_ops_t aic_backend_ops_template  = {
    .name = "aic_player",
    .create = aic_player_backend_create,
    .destroy = aic_player_backend_destroy,
    .set_src = aic_player_backend_set_src,
    .control = aic_player_backend_control
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
const player_backend_ops_t *aic_backend_get_template(void)
{
    return &aic_backend_ops_template;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void * aic_player_backend_create(void)
{
    player_backend_ops_t *ops = lv_malloc_zeroed(sizeof(player_backend_ops_t));
    if (!ops) {
        LV_LOG_ERROR("alloc player_backend_ops_t failed");
        return NULL;
    }

    const player_backend_ops_t *template = aic_backend_get_template();
    memcpy(ops, template, sizeof(player_backend_ops_t));

    struct aic_player_ctx *aic_ctx = lv_malloc_zeroed(sizeof(struct aic_player_ctx));
    if (!aic_ctx) {
        LV_LOG_ERROR("alloc aic_player_ctx failed");
        goto create_backend_failed;
    }

    if (backend_get_screen_info(&aic_ctx->screen_info) < 0) {
        lv_mem_free(ops);
        lv_mem_free(aic_ctx);
        LV_LOG_ERROR("get screen info failed");
        goto create_backend_failed;
    }

    if (player_decode_thread_create(aic_ctx) < 0) {
        goto create_backend_failed;
    }

    ops->ctx = aic_ctx;
    aic_ctx->player = aic_player_create(NULL);
    if (!aic_ctx->player) {
        LV_LOG_ERROR("create aic player failed");
        goto create_backend_failed;
    }

    aic_player_set_event_callback(aic_ctx->player, aic_ctx, aic_player_event_callback);

    _lv_ll_init(&aic_ctx->frame_ll, sizeof(struct mpp_frame));

    atomic_store(&aic_ctx->status, PLAYER_STATUS_IDLE);

    return ops;

create_backend_failed:
    if (aic_ctx->player)
        aic_player_destroy(aic_ctx->player);
    if (aic_ctx)
        lv_mem_free(aic_ctx);
    if (ops)
        lv_mem_free(ops);
    return NULL;
}

static void aic_player_backend_destroy(void *ctx)
{
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    struct aic_player_ctx *aic_ctx = ops->ctx;

    player_decode_thread_destroy(aic_ctx);

    player_resource_cleanup(aic_ctx);

    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO && aic_ctx->keep_last_frame) {
        int enable = 0;
        aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
    }

    if (aic_ctx->player)
        aic_player_destroy(aic_ctx->player);

    if (aic_ctx)
        lv_mem_free(aic_ctx);

    if (ops)
        lv_mem_free(ops);

    aic_ctx->image_data = NULL;
}

static lv_res_t aic_player_backend_set_src(void *ctx, const char *src)
{
    int res = LV_RES_INV;
    const player_backend_ops_t *player_ctx = (player_backend_ops_t *)ctx;
    lv_aic_player_t *player = (lv_aic_player_t *)player_ctx->obj;
    struct aic_player_ctx *aic_ctx = player_ctx->ctx;

    if (!aic_ctx) {
        LV_LOG_ERROR("aic_ctx is NULL");
        return LV_RES_INV;
    }

    /* cleanup old resources */
    atomic_store(&aic_ctx->status, PLAYER_STATUS_IDLE);
    player_resource_cleanup(aic_ctx);

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
            aic_ctx->draw_layer = player_select_layer(aic_ctx);
        } else {
            aic_ctx->draw_layer = player->draw_layer;
        }

        /* check the configuration */
        if (check_player_configuration(aic_ctx->draw_layer) < 0)
            goto src_failed;
    }

    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO)
        aic_ctx->image_src = lv_malloc_zeroed(128);
    else
        aic_ctx->image_src = lv_malloc_zeroed(sizeof(lv_image_dsc_t));
    if (!aic_ctx->image_src) {
        LV_LOG_ERROR("lv_malloc_zeroed image src failed");
        goto src_failed;
    }

    if ((aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF ||
        aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF)) {
        res = player_alloc_image_buf(aic_ctx);
        if (res < 0) {
            LV_LOG_ERROR("player_alloc_image_buff failed");
            goto src_failed;
        }

        lv_image_dsc_t *image_dst = (lv_image_dsc_t *)aic_ctx->image_src;
        backend_draw_color_buffer(image_dst, 0xff000000);
    }

    /* here are the video related settings */
    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO) {
        player_handle_update_display_area(ctx);
        if (!aic_ctx->keep_last_frame) {
            int enable = 1;
            aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
            aic_ctx->keep_last_frame = true;
        }
    }

    atomic_store(&aic_ctx->status, PLAYER_STATUS_READY);

    return LV_RES_OK;

src_failed:
    if (aic_ctx->image_src) {
        lv_free(aic_ctx->image_src);
        aic_ctx->image_src = NULL;
    }

    player_free_image_buf(aic_ctx);

    if (aic_ctx->player) {
        aic_player_stop(aic_ctx->player);
    }
    return LV_RES_INV;
}

static lv_res_t aic_player_backend_control(void *ctx, player_cmd_t cmd, void *data)
{
    const player_backend_ops_t *player_ctx = (player_backend_ops_t *)ctx;
    struct aic_player_ctx *aic_ctx = player_ctx->ctx;
    lv_res_t res = LV_RES_OK;

    if (!aic_ctx) {
        LV_LOG_WARN("player is NULL, please set src first");
        return LV_RES_INV;
    }

    switch(cmd) {
        case PLAYER_CMD_START:
            res = aic_player_start(aic_ctx->player) ? LV_RES_INV : LV_RES_OK;
            atomic_store(&aic_ctx->status, PLAYER_STATUS_RUNNING);
            break;
        case PLAYER_CMD_PAUSE:
            res = aic_player_pause(aic_ctx->player) ? LV_RES_INV : LV_RES_OK;
            atomic_store(&aic_ctx->status, PLAYER_STATUS_PAUSE);
            break;
        case PLAYER_CMD_RESUME:
            res = aic_player_play(aic_ctx->player) ? LV_RES_INV : LV_RES_OK;
            atomic_store(&aic_ctx->status, PLAYER_STATUS_RUNNING);
            break;
        case PLAYER_CMD_PLAY_END:
            CHECK_DATA_OR_RETURN(data, PLAYER_CMD_PLAY_END);
            *(bool *)data = (aic_ctx->status == PLAYER_STATUS_END);
            res = LV_RES_OK;
            break;
        case PLAYER_CMD_GET_MEDIA_INFO:
            CHECK_DATA_OR_RETURN(data, PLAYER_CMD_GET_MEDIA_INFO);
            memcpy(data, &aic_ctx->media_info, sizeof(struct av_media_info));
            res = LV_RES_OK;
            break;
        case PLAYER_CMD_SET_VOLUME:
            CHECK_DATA_OR_RETURN(data, PLAYER_CMD_SET_VOLUME);
            res = aic_player_set_volum(aic_ctx->player, *(s32 *)data) ? LV_RES_INV : LV_RES_OK;
            break;
        case PLAYER_CMD_GET_VOLUME:
            CHECK_DATA_OR_RETURN(data, PLAYER_CMD_GET_VOLUME);
            res = aic_player_get_volum(aic_ctx->player, (s32 *)data) ? LV_RES_INV : LV_RES_OK;
            break;
        case PLAYER_CMD_SET_PLAY_TIME:
            CHECK_DATA_OR_RETURN(data, PLAYER_CMD_SET_PLAY_TIME);
            res = aic_player_seek(aic_ctx->player, *(u64 *)data) ? LV_RES_INV : LV_RES_OK;
            if (res == LV_RES_INV) {
                LV_LOG_ERROR("seek err, set player status idle");
                atomic_store(&aic_ctx->status, PLAYER_STATUS_IDLE);
                return LV_RES_INV;
            }
            atomic_store(&aic_ctx->status, PLAYER_STATUS_RUNNING);
            break;
        case PLAYER_CMD_GET_PLAY_TIME:
            CHECK_DATA_OR_RETURN(data, PLAYER_CMD_GET_PLAY_TIME);
            *(u64 *)data = aic_player_get_play_time(aic_ctx->player);
            res = (*(u64 *)data < 0) ? LV_RES_INV : LV_RES_OK;
            break;
        case PLAYER_CMD_GET_FRAME:
            return player_handle_get_frame(ctx);
        case PLAYER_CMD_UPDATE_DISPLAY_AREA:
            return player_handle_update_display_area(ctx);
        case PLAYER_CMD_GET_IMAGE_SRC:
            if (!aic_ctx->image_src || !data) {
                return LV_RES_INV;
            }
            *(void **)data = aic_ctx->image_src;
            return LV_RES_OK;
        case PLAYER_CMD_SET_PLAYBACK_RATE:
            LV_LOG_WARN("AIC backend does not support playback rate control");
            return LV_RES_INV;
        default:
            LV_LOG_ERROR("Error cmd: %d", cmd);
            res = LV_RES_INV;
            break;
    }

    return res;
}

/* Auxiliary drawing and format conversion functions */
static int check_player_configuration(uint32_t layer)
{
    /* check the configuration */
    if (layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF ||
        layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
#ifndef AIC_MPP_PLAYER_VIDEO_EXT_RENDER
        LV_LOG_ERROR("Setting src is wrong, the configuration is incorrect.\n"
                     "Please use command scons --menuconfig, and open the configuration according to the path:\n"
                     "-----------------ArtInChip Luban-Lite SDK Configuration------------ \n"
                     "Local packages options  ---> \n"
                     "  ArtInChip packages options  ---> \n"
                     "    [*] aic-mpp  ---> \n"
                     "      [*] Enable player using external video render \n"
                     "------------------------------------------------------------------ \n");
        return -1;
#endif
    }
    if (layer == LV_AIC_PLAYER_LAYER_VIDEO) {
#ifdef AIC_MPP_PLAYER_VIDEO_EXT_RENDER
        LV_LOG_ERROR("Setting src is wrong, the configuration is incorrect.\n"
                     "Please use command scons --menuconfig, and close the configuration according to the path:\n"
                     "-----------------ArtInChip Luban-Lite SDK Configuration------------ \n"
                     "Local packages options  ---> \n"
                     "  ArtInChip packages options  ---> \n"
                     "    [*] aic-mpp  ---> \n"
                     "      [ ] Enable player using external video render\n"
                     "------------------------------------------------------------------ \n");
        return -1;
#endif
    }

    return 0;
}

/* Core player functionality functions */
static int alloc_player_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame,
                              int width, int height, enum mpp_pixel_format format)
{
    struct aic_player_ctx * aic_ctx = (struct aic_player_ctx *)p;

    int screen_format;
    int stride;
    int alloc_buffer_times = aic_ctx->allocated_frame_count;

    int media_width = aic_ctx->media_info.video_stream.width;
    int media_height = aic_ctx->media_info.video_stream.height;
    screen_format = aic_ctx->screen_info.format;
    stride = backend_align_stride(media_width, screen_format);

    frame->buf.format = screen_format;
    frame->buf.size.width = media_width;
    frame->buf.size.height = media_height;
    frame->buf.stride[0] = stride;
    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.phy_addr[0] = (unsigned long)aic_ctx->phy_frame_addr[alloc_buffer_times];

    aic_ctx->allocated_frame_count++;
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

static int player_alloc_image_buf(struct aic_player_ctx *aic_ctx)
{
    lv_image_dsc_t *image_dst;

    player_free_frame_buffer(aic_ctx);
    if (player_alloc_frame_buffer(aic_ctx) < 0) {
        LV_LOG_ERROR("player_alloc_frame_buffer failed");
        return -1;
    }

    aic_ctx->image_data = (uint8_t *)aic_ctx->phy_frame_addr[0];

    int width = aic_ctx->media_info.video_stream.width;
    int height = aic_ctx->media_info.video_stream.height;
    int screen_format = aic_ctx->screen_info.format;
    int stride = backend_align_stride(width, screen_format);
    uint8_t * img_data = player_get_image_date(aic_ctx);

    /* decode into screen format */
    image_dst = (lv_image_dsc_t *)aic_ctx->image_src;
#if LVGL_VERSION_MAJOR == 8
    aic_ctx->buf.buf_type = MPP_PHY_ADDR;
    aic_ctx->buf.size.width = width;
    aic_ctx->buf.size.height = height;
    aic_ctx->buf.format = screen_format;
    aic_ctx->buf.stride[0] = stride;
    aic_ctx->buf.phy_addr[0] = (unsigned int)(uintptr_t)img_data;

    image_dst->header.always_zero = 0;
    image_dst->header.w = width;
    image_dst->header.h = height;
    image_dst->header.cf = LV_IMG_CF_RESERVED_16;
    image_dst->data = (uint8_t *)&aic_ctx->buf;
#elif LVGL_VERSION_MAJOR == 9
    image_dst->header.w = width;
    image_dst->header.h = height;
    image_dst->header.cf = backend_fmt_mpp_to_lv(screen_format);
    image_dst->header.stride = stride;
    image_dst->header.flags = 0;
    image_dst->header.magic = LV_IMAGE_HEADER_MAGIC;
    image_dst->data = img_data;
    image_dst->data_size = stride * height;
#endif
    return 0;
}

static int player_free_image_buf(struct aic_player_ctx *aic_ctx)
{
    if (!aic_ctx) {
        return -1;
    }

    player_free_frame_buffer(aic_ctx);

    return 0;
}

static int player_alloc_frame_buffer(struct aic_player_ctx *aic_ctx)
{
    int i = 0;
    s32 ret = 0;

    int width = ALIGN_UP(aic_ctx->media_info.video_stream.width, 16);
    int height = ALIGN_UP(aic_ctx->media_info.video_stream.height, 16);

    int screen_format = aic_ctx->screen_info.format;
    int stride = backend_align_stride(width, screen_format);

    s32 alloc_times = 1;
    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
        alloc_times = 2;
    }

    for (i = 0; i < alloc_times; i++) {
        aic_ctx->phy_frame_addr[i] = (unsigned long)mpp_phy_alloc(stride * height);
        if (!aic_ctx->phy_frame_addr[i]) {
            LV_LOG_ERROR("mpp_phy_alloc failed, i = %d, alloc_times = %d", i, alloc_times);
            player_free_frame_buffer(aic_ctx);
            return -1;
        }
    }

    s32 ext_frame_buffer_num = alloc_times - 1;
    ret = aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_NUM,
                             (void *)&ext_frame_buffer_num);
    if (ret != 0) {
        LV_LOG_ERROR("player set vdec ext frame num failed %d", ret);
        player_free_frame_buffer(aic_ctx);
        return -1;
    }

    aic_ctx->allocated_frame_count = 0;
    aic_ctx->allocator.ops = &frame_buffer_alloc_ops;
    ret = aic_player_control(aic_ctx->player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_ALLOCATOR,
                             (void *)aic_ctx);
    if (ret != 0) {
        LV_LOG_ERROR("player set vdec ext frame alloc failed %d", ret);
        player_free_frame_buffer(aic_ctx);
        return -1;
    }

    return 0;
}

static void player_free_frame_buffer(struct aic_player_ctx *aic_ctx)
{
    int i = 0;
    for (i = 0; i < 2; i++) {
        if (aic_ctx->phy_frame_addr[i] != 0)
            mpp_phy_free(aic_ctx->phy_frame_addr[i]);
        aic_ctx->phy_frame_addr[i] = 0;
    }
}

static uint8_t *player_get_image_date(struct aic_player_ctx *aic_ctx)
{
    return aic_ctx->image_data;
}

static int player_select_layer(struct aic_player_ctx *aic_ctx)
{
    int draw_layer = LV_AIC_PLAYER_LAYER_VIDEO;

#ifdef PRJ_CHIP
    if (aic_ctx->media_info.has_video == 1) {
        if (strcmp(PRJ_CHIP, "d12x") == 0) {
            draw_layer = LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF;
        } else if ((strcmp(PRJ_CHIP, "d13x") == 0)) {
#if LV_COLOR_DEPTH == 16
            draw_layer = LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF;
#else
            draw_layer = LV_AIC_PLAYER_LAYER_VIDEO;
#endif
        } else if ((strcmp(PRJ_CHIP, "d21x") == 0)) {
            draw_layer = LV_AIC_PLAYER_LAYER_VIDEO;
        }
    }
#endif

    return draw_layer;
}

static int player_check_hw_capability(uint32_t rotate, uint32_t scale_x, uint32_t scale_y)
{
#ifdef PRJ_CHIP
    if (strcmp(PRJ_CHIP, "d12x") == 0) {
        return true;
    } else if (strcmp(PRJ_CHIP, "d13x") == 0) {
#if LVGL_VERSION_MAJOR == 8
        bool scale_condition = (scale_x == 256);
#elif LVGL_VERSION_MAJOR == 9
        bool scale_condition = (scale_x == LV_SCALE_NONE && scale_y == LV_SCALE_NONE);
#endif
        if (scale_condition && (rotate == 0 || rotate == 900 || rotate == 1800 || rotate == 2700)) {
            return false;
        } else {
            return true;
        }
    } else if (strcmp(PRJ_CHIP, "d21x") == 0) {
        if (rotate == 0 || rotate == 900 || rotate == 1800 || rotate == 2700) {
            return false;
        } else {
            return true;
        }
    }
#endif
    return false;
}

static void player_resource_cleanup(struct aic_player_ctx * aic_ctx)
{
    while (1) {
        player_thread_lock(aic_ctx);
        bool is_decoding = aic_ctx->decoding;
        player_thread_unlock(aic_ctx);

        if (!is_decoding)
            break;
        aicos_msleep(3);
    }

    player_put_frame_all(aic_ctx);
    aic_player_stop(aic_ctx->player);
    player_free_image_buf(aic_ctx);

    if (aic_ctx->image_src) {
        lv_free(aic_ctx->image_src);
        aic_ctx->image_src = NULL;
    }

    memset(&aic_ctx->area, 0, sizeof(lv_area_t));
}

static bool player_decoder_data_ready(int ret)
{
    switch (ret)
    {
    case DEC_NO_EMPTY_PACKET:
    case DEC_NO_READY_PACKET:
    case DEC_NO_RENDER_FRAME:
    case DECODER_NOT_CREATED:
    case FRAME_CAPTURE_TIMEOUT:
        return false;
    default:
        break;
    }

    return true;
}

/* Decoding-related functions */
static int player_get_frame(struct aic_player_ctx *aic_ctx)
{
    int ret = 0;
    bool ready = true;
    int result = -1;
    uint32_t start_time = 0;
    struct mpp_frame frame = {0};
    struct mpp_frame *p_frame = NULL;

    if (atomic_load(&aic_ctx->status) != PLAYER_STATUS_RUNNING) {
        return -1;
    }

    player_thread_lock(aic_ctx);
    aic_ctx->decoding = true;
    player_thread_unlock(aic_ctx);

    player_put_frame(aic_ctx);

    start_time = lv_tick_get();
    do {
        if (lv_tick_elaps(start_time) > FRAME_TIMEOUT) {
            LV_LOG_WARN("get frame timeout: %d", (int)lv_tick_elaps(start_time));
            break;
        }

        ret = aic_player_get_frame(aic_ctx->player, (void *)&frame); /* will block the thread */
        if (!ret) {
            player_thread_lock(aic_ctx);
            p_frame = (struct mpp_frame *)_lv_ll_ins_tail(&aic_ctx->frame_ll);
            player_thread_unlock(aic_ctx);
            if (!p_frame) {
                LV_LOG_ERROR("Failed to allocate frame node");
                break;
            }

            lv_memcpy(p_frame, &frame, sizeof(struct mpp_frame));
            LV_LOG_INFO("get frame %p, id = %d",
                       (uint8_t *)(uintptr_t)p_frame->buf.phy_addr[0],
                       p_frame->id);

            if (frame.flags & FRAME_FLAG_EOS) {
                atomic_store(&aic_ctx->status, PLAYER_STATUS_END);
            }
            result = 0;
            break;
        }
        if (ret < 0) {
            LV_LOG_ERROR("get frame error, ret = %d", ret);
            break;
        }
        ready = player_decoder_data_ready(ret);
        if (!ready) {
            aicos_msleep(3);
            continue;
        }
    } while (1);

    player_thread_lock(aic_ctx);
    if (result == 0 && p_frame) {
        aic_ctx->image_data = (uint8_t *)(uintptr_t)p_frame->buf.phy_addr[0];
    }
    aic_ctx->decoding = false;
    player_thread_unlock(aic_ctx);

    return result;
}

static int player_get_frame_send_signal(struct aic_player_ctx * aic_ctx)
{
    pthread_mutex_lock(&aic_ctx->frame_mutex);
    aic_ctx->has_new_request = true;
    pthread_cond_signal(&aic_ctx->frame_cond);
    pthread_mutex_unlock(&aic_ctx->frame_mutex);
    return 0;
}

static int player_put_frame(struct aic_player_ctx *aic_ctx)
{
    s32 ret = 0;
    struct mpp_frame *p_frame = NULL;

    int push_frame_num = 1;
    /* Ensure that all frames are fully captured, then start from the beginning again,
       making sure that the frames used are only those that are decoded. */
    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF)
        push_frame_num = 2;

    player_thread_lock(aic_ctx);
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
    player_thread_unlock(aic_ctx);
    return 0;
}

static int player_put_frame_all(struct aic_player_ctx * aic_ctx)
{
    s32 ret;

    player_thread_lock(aic_ctx);
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
    player_thread_unlock(aic_ctx);

    return 0;
}

static int player_decode_thread_create(struct aic_player_ctx *aic_ctx)
{
    int ret = 0;

    ret = pthread_mutex_init(&aic_ctx->frame_mutex, NULL);
    if (ret != 0) {
        LV_LOG_ERROR("Failed to initialize mutex: %d", ret);
        return -1;
    }

    ret = pthread_cond_init(&aic_ctx->frame_cond, NULL);
    if (ret != 0) {
        LV_LOG_ERROR("Failed to initialize condition variable: %d", ret);
        goto mutex_cleanup;
    }

    ret = pthread_create(&aic_ctx->frame_thread, NULL, player_decode_entry, aic_ctx);
    if (ret != 0) {
        LV_LOG_ERROR("Failed to create thread: %d", ret);
        goto cond_cleanup;
    }

    aic_ctx->stop_thread = false;
    return 0;

cond_cleanup:
    pthread_cond_destroy(&aic_ctx->frame_cond);
mutex_cleanup:
    pthread_mutex_destroy(&aic_ctx->frame_mutex);
    return -1;
}

static void player_decode_thread_destroy(struct aic_player_ctx *aic_ctx)
{
    pthread_mutex_lock(&aic_ctx->frame_mutex);
    aic_ctx->stop_thread = true;
    pthread_mutex_unlock(&aic_ctx->frame_mutex);

    pthread_cond_signal(&aic_ctx->frame_cond);
    pthread_join(aic_ctx->frame_thread, NULL);
    pthread_mutex_destroy(&aic_ctx->frame_mutex);
    pthread_cond_destroy(&aic_ctx->frame_cond);
}

static void player_thread_lock(struct aic_player_ctx *aic_ctx)
{
    pthread_mutex_lock(&aic_ctx->frame_mutex);
}

static void player_thread_unlock(struct aic_player_ctx *aic_ctx)
{
    pthread_mutex_unlock(&aic_ctx->frame_mutex);
}

static void *player_decode_entry(void *ptr)
{
    struct aic_player_ctx *aic_ctx = (struct aic_player_ctx *)ptr;
    bool stop_thread = false;

    while (1) {
        pthread_mutex_lock(&aic_ctx->frame_mutex);

        while(!aic_ctx->has_new_request && !aic_ctx->stop_thread) {
            pthread_cond_wait(&aic_ctx->frame_cond, &aic_ctx->frame_mutex);
        }

        if (aic_ctx->stop_thread) {
            pthread_mutex_unlock(&aic_ctx->frame_mutex);
            break;
        }

        aic_ctx->has_new_request = false;
        pthread_mutex_unlock(&aic_ctx->frame_mutex);

        if (stop_thread)
            break;

        player_get_frame(aic_ctx);
    }

    return NULL;
}

static int aic_player_event_callback(void *ctx, int event_type, int param1, int param2)
{
    int ret = 0;
    struct aic_player_ctx *aic_ctx = (struct aic_player_ctx *)ctx;

    switch(event_type) {
        case AIC_PLAYER_EVENT_PLAY_END:
            if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_VIDEO)
                atomic_store(&aic_ctx->status, PLAYER_STATUS_END);
            break;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            break;
        default:
            break;
    }
    return ret;
}

static int calc_video_mpp_rotation_with_check(lv_obj_t *obj, u32 *video_rotate)
{
    int disp_rot_deg = 0;
    u32 mpp_rotation = MPP_ROTATION_0;
    lv_disp_t * disp = lv_disp_get_default();
    lv_disp_rotation_t disp_rot = lv_disp_get_rotation(disp);
    lv_image_t *image = (lv_image_t *)obj;

    switch(disp_rot) {
        case LV_DISP_ROTATION_90:  disp_rot_deg = 90;  break;
        case LV_DISP_ROTATION_180: disp_rot_deg = 180; break;
        case LV_DISP_ROTATION_270: disp_rot_deg = 270; break;
        default: break;
    }
    int image_rot_deg = lv_image_get_rotation(obj) / 10;
    int phys_rot_deg = (image_rot_deg - disp_rot_deg + 360) % 360;

    if (phys_rot_deg == 90)  mpp_rotation = MPP_ROTATION_90;
    else if (phys_rot_deg == 180) mpp_rotation = MPP_ROTATION_180;
    else if (phys_rot_deg == 270) mpp_rotation = MPP_ROTATION_270;

#if LVGL_VERSION_MAJOR == 8
    bool check = player_check_hw_capability(phys_rot_deg * 10, image->zoom, image->zoom);
#elif LVGL_VERSION_MAJOR == 9
    bool check = player_check_hw_capability(phys_rot_deg * 10, image->scale_x, image->scale_y);
#endif
    if (check) {
        LV_LOG_ERROR("Hardware functionality is limited, rotation cannot be used");
        return -1;
    }

    *video_rotate = mpp_rotation;

    return 0;
}

static int calc_video_physical_display_rect(struct mpp_rect *disp_rect, lv_area_t *real_area)
{
    lv_disp_t * disp = lv_disp_get_default();
    lv_disp_rotation_t disp_rot = lv_disp_get_rotation(disp);
    lv_coord_t orig_hor = lv_disp_get_physical_hor_res(disp);
    lv_coord_t orig_ver = lv_disp_get_physical_ver_res(disp);

    int log_x = real_area->x1;
    int log_y = real_area->y1;
    int log_width = lv_area_get_width(real_area);
    int log_height = lv_area_get_height(real_area);

    if (log_x < 0 || log_y < 0)
        return -1;

    switch(disp_rot) {
        case LV_DISP_ROTATION_90:
            disp_rect->x = log_y;
            disp_rect->y = orig_hor - log_x - log_width;
            disp_rect->width = log_height;
            disp_rect->height = log_width;
            break;
        case LV_DISP_ROTATION_180:
            disp_rect->x = orig_hor - log_x - log_width;
            disp_rect->y = orig_ver - log_y - log_height;
            disp_rect->width = log_width;
            disp_rect->height = log_height;
            break;
        case LV_DISP_ROTATION_270:
            disp_rect->x = orig_ver - log_y - log_height;
            disp_rect->y = log_x;
            disp_rect->width = log_height;
            disp_rect->height = log_width;
            break;
        default:
            disp_rect->x = log_x;
            disp_rect->y = log_y;
            disp_rect->width = log_width;
            disp_rect->height = log_height;
            break;
    }

    bool out_of_range = false;
    if (disp_rot == LV_DISP_ROTATION_90 || disp_rot == LV_DISP_ROTATION_270) {
        out_of_range = (disp_rect->x < 0 || disp_rect->x + disp_rect->width > orig_ver ||
                        disp_rect->y < 0 || disp_rect->y + disp_rect->height > orig_hor);
    } else {
        out_of_range = (disp_rect->x < 0 || disp_rect->x + disp_rect->width > orig_hor ||
                        disp_rect->y < 0 || disp_rect->y + disp_rect->height > orig_ver);
    }

    if (out_of_range) {
        LV_LOG_WARN("disp_rect out of physical display range, logic_x = %d, logic_y = %d, logic_width = %d, logic_height = %d",
                        log_x, log_y, log_width, log_height);
        return -1;
    }

    return 0;
}

static lv_res_t player_handle_update_display_area(void *ctx)
{
    player_backend_ops_t *player_ctx = (player_backend_ops_t *)ctx;
    lv_obj_t *obj = (lv_obj_t *)player_ctx->obj;
    struct aic_player_ctx *aic_ctx = player_ctx->ctx;

    int ret = -1;
    u32 video_rotate = MPP_ROTATION_0;
    lv_image_t *image = (lv_image_t *)obj;
    struct mpp_rect disp_rect = {0};

    if (!aic_ctx || aic_ctx->draw_layer != LV_AIC_PLAYER_LAYER_VIDEO)
        return LV_RES_INV;

    lv_point_t pivot_px = {0};
    lv_image_get_pivot(obj, &pivot_px);

    /* obtain the actual display area  */
    lv_area_t area = {obj->coords.x1, obj->coords.y1,
                      obj->coords.x1 + image->w - 1, obj->coords.y1 + image->h - 1};
#if LVGL_VERSION_MAJOR == 9
    if(image->align < _LV_IMAGE_ALIGN_AUTO_TRANSFORM) {
        lv_area_align(&obj->coords, &area, image->align, image->offset.x, image->offset.y);
    } else if(image->align == LV_IMAGE_ALIGN_TILE) {
        image->align = LV_IMAGE_ALIGN_TOP_LEFT;
        LV_LOG_WARN("draw layer video does not supports LV_IMAGE_ALIGN_TILE alignment, automatic set LV_IMAGE_ALIGN_TOP_LEFT");
    }
#endif

    lv_area_t real_area = {0};
    lv_memcpy(&real_area, &area, sizeof(lv_area_t));
#if LVGL_VERSION_MAJOR == 8
    _lv_img_buf_get_transformed_area(&real_area, lv_area_get_width(&area), lv_area_get_height(&area),
                                       image->angle, image->zoom, &pivot_px);
#elif LVGL_VERSION_MAJOR == 9
    _lv_image_buf_get_transformed_area(&real_area, lv_area_get_width(&area), lv_area_get_height(&area),
                                       image->rotation, image->scale_x,
                                       image->scale_y, &pivot_px);
#endif
    lv_area_move(&real_area, area.x1, area.y1);
    if (aic_ctx->image_rotation == lv_image_get_rotation(obj) &&
        aic_ctx->disp_rotation == lv_disp_get_rotation(lv_disp_get_default()) &&
        aic_ctx->area.x1 == real_area.x1 && aic_ctx->area.x2 == real_area.x2 &&
        aic_ctx->area.y1 == real_area.y1 && aic_ctx->area.y2 == real_area.y2) {
        return LV_RES_INV;
    }
    aic_ctx->image_rotation = lv_image_get_rotation(obj);
    aic_ctx->disp_rotation = lv_disp_get_rotation(lv_disp_get_default());
    lv_memcpy(&aic_ctx->area, &real_area, sizeof(lv_area_t));

    int alpha_en = 0;
    int image_width = ((int)aic_ctx->area.x2 - (int)aic_ctx->area.x1) + 1;
    int image_height = ((int)aic_ctx->area.y2 - (int)aic_ctx->area.y1) + 1;
    uint32_t color = 0;

    memset(aic_ctx->image_src, 0, 128);
    snprintf(aic_ctx->image_src, 128, "L:/%dx%d_%d_%08x.fake",
            image_width, image_height, alpha_en, (unsigned int)color);

    /* must setting after start */
    if (calc_video_physical_display_rect(&disp_rect, &aic_ctx->area) < 0)
        return LV_RES_INV;

    ret = aic_player_set_disp_rect(aic_ctx->player, &disp_rect);
    if (ret != 0) {
        LV_LOG_ERROR("aic_player_set_disp_rect failed");
        return LV_RES_INV;
    }

    if (calc_video_mpp_rotation_with_check(obj, &video_rotate) < 0)
        return LV_RES_INV;

    ret = aic_player_set_rotation(aic_ctx->player, video_rotate);
    if (ret != 0) {
        LV_LOG_ERROR("aic_player_set_rotation failed, rotation = %d", video_rotate);
        return LV_RES_INV;
    }
    return LV_RES_OK;
}

static void player_handle_first_frame_double_buffer(void *ctx)
{
    const player_backend_ops_t *player_ctx = (player_backend_ops_t *)ctx;
    struct aic_player_ctx *aic_ctx = player_ctx->ctx;
    bool decoding_status = 0;
    uint32_t start_time = 0;

    player_thread_lock(aic_ctx);
    aic_ctx->decoding = true;
    player_thread_unlock(aic_ctx);

    player_get_frame_send_signal(aic_ctx);

    start_time = lv_tick_get();

    while (1) {
        if (lv_tick_elaps(start_time) > FRAME_TIMEOUT) {
            LV_LOG_WARN("get first frame timeout: %d", (int)lv_tick_elaps(start_time));
            break;
        }

        player_thread_lock(aic_ctx);
        decoding_status = aic_ctx->decoding;
        player_thread_unlock(aic_ctx);
        if (decoding_status == false)
            break;

        aicos_msleep(3);
    }
}

static lv_res_t player_handle_get_frame(void *ctx)
{
    const player_backend_ops_t *player_ctx = (player_backend_ops_t *)ctx;
    struct aic_player_ctx *aic_ctx = player_ctx->ctx;
    bool is_first_frame = false;
    bool decoding_status = 0;

    if (atomic_load(&aic_ctx->status) != PLAYER_STATUS_RUNNING)
        return LV_RES_INV;

    if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF) {
        if (player_get_frame(aic_ctx) < 0)
            return LV_RES_INV;
    } else if (aic_ctx->draw_layer == LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF) {
        /* first frame */
        player_thread_lock(aic_ctx);
        if (_lv_ll_get_len(&aic_ctx->frame_ll) == 0) {
            is_first_frame = true;
        }
        decoding_status = aic_ctx->decoding;
        player_thread_unlock(aic_ctx);

        if (is_first_frame) {
            player_handle_first_frame_double_buffer(ctx);
        }

        if (decoding_status == true) {
            return LV_RES_OK;
        }

        player_get_frame_send_signal(aic_ctx);
    } else {
        return LV_RES_INV;
    }

    player_thread_lock(aic_ctx);
    uint8_t * img_data = player_get_image_date(aic_ctx);

#if LVGL_VERSION_MAJOR == 8
    aic_ctx->buf.phy_addr[0] = (unsigned int)(uintptr_t)img_data;
#elif LVGL_VERSION_MAJOR == 9
    lv_image_dsc_t *image_dst = (lv_image_dsc_t *)aic_ctx->image_src;
    image_dst->data = img_data;
#endif
    player_thread_unlock(aic_ctx);

    return LV_RES_OK;
}
#endif
