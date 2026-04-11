/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lv_aic_player.h"
#include "aic_ui.h"

#if LV_USE_AIC_SIMULATOR == 0
#include "./player_backend/player_backend_ops.h"

static lv_res_t player_backend_ensure(lv_obj_t *obj, const char *src);

static void lv_player_refresh_next_frame_cb(lv_timer_t * timer);

static void lv_aic_player_set_image_src(lv_obj_t *obj);

static bool lv_player_is_slave(lv_obj_t *obj);
static void lv_player_attach_slave(lv_obj_t *obj, lv_obj_t *slave_obj);
static void lv_player_propagate_src_to_slaves(lv_obj_t * obj);
static void lv_player_invalidate_slaves(lv_obj_t * obj);
static void lv_player_remove_all_slaves(lv_obj_t *obj);

static void player_reset_frame_syc(lv_obj_t *obj);
static bool player_should_wait_sync(lv_obj_t * obj);
static bool player_reset_frame_sync(lv_obj_t *obj, void *data);
static void player_set_width(lv_obj_t *obj, uint32_t width);
static void player_set_height(lv_obj_t *obj, uint32_t height);
static void player_reset_size(lv_obj_t *obj);
static void player_update_size(lv_obj_t *obj);

static void lv_aic_player_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_aic_player_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_aic_player_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void lv_aic_slave_player_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_aic_slave_player_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

const lv_obj_class_t lv_aic_player_class = {
    .constructor_cb = lv_aic_player_constructor,
    .destructor_cb = lv_aic_player_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .event_cb = lv_aic_player_event,
    .instance_size = sizeof(lv_aic_player_t),
#if LVGL_VERSION_MAJOR == 8
    .base_class = &lv_img_class,
#elif LVGL_VERSION_MAJOR == 9
    .base_class = &lv_image_class,
    .name = "lv_aic_player",
#endif
};

const lv_obj_class_t lv_aic_slave_class = {
    .constructor_cb = lv_aic_slave_player_constructor,
    .destructor_cb = lv_aic_slave_player_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_aic_slave_player_t),
#if LVGL_VERSION_MAJOR == 8
    .base_class = &lv_img_class,
#elif LVGL_VERSION_MAJOR == 9
    .base_class = &lv_image_class,
    .name = "lv_aic_slave",
#endif
};

lv_obj_t *lv_aic_player_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_aic_player_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_res_t lv_aic_player_set_src(lv_obj_t * obj, const char *src)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (!obj || !src || lv_player_is_slave(obj))
        return LV_RES_INV;

    lv_image_src_t src_type = lv_image_src_get_type(src);
    if (src_type != LV_IMAGE_SRC_FILE) {
        LV_LOG_ERROR("unknown type");
        return LV_RES_INV;
    }

    if (lv_image_get_src(obj))
        lv_image_cache_drop(lv_image_get_src(obj));

    if (!player->timer) {
        player->timer = lv_timer_create(lv_player_refresh_next_frame_cb, 5, obj);
        if (!player->timer) {
            goto src_failed;
        }
        _lv_ll_init(&player->slave_players, sizeof(lv_obj_t *));
    }

    if (player_backend_ensure(obj, src) == LV_RES_INV)
        goto src_failed;

    if (player_backend_set_src(player->ctx, src) == LV_RES_INV) {
        goto src_failed;
    }

    player_reset_frame_syc(obj);

    lv_player_propagate_src_to_slaves(obj);

    lv_aic_player_set_image_src(obj);

    player_update_size(obj);

    return LV_RES_OK;

src_failed:
    if (player->ctx) {
        player_backend_destroy(player->ctx);
        player->ctx = NULL;
    }

    if (player->timer) {
        lv_timer_delete(player->timer);
        player->timer = NULL;
    }
    return LV_RES_INV;
}

void lv_aic_player_set_cmd(lv_obj_t * obj, lv_aic_player_cmd_t cmd, void *data)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (!obj || !player->ctx || !player->timer || lv_player_is_slave(obj))
        return;

    switch (cmd) {
    case LV_AIC_PLAYER_CMD_START:
        player_backend_control(player->ctx, PLAYER_CMD_START, data);
        player_backend_control(player->ctx, PLAYER_CMD_GET_FRAME, NULL);
        lv_timer_resume(player->timer);
        break;
    case LV_AIC_PLAYER_CMD_STOP:
        if (player->ctx) {
            player_backend_destroy(player->ctx);
            player->ctx = NULL;
            LV_LOG_WARN("The playback resource has been release, indicating a possible exception.");
        }
        if (lv_image_get_src(obj))
            lv_image_cache_drop(lv_image_get_src(obj));
        lv_image_set_src(obj, NULL);
        lv_timer_pause(player->timer);
        break;
    case LV_AIC_PLAYER_CMD_PAUSE:
        player_backend_control(player->ctx, PLAYER_CMD_PAUSE, data);
        lv_timer_pause(player->timer);
        break;
    case LV_AIC_PLAYER_CMD_RESUME:
        player_backend_control(player->ctx, PLAYER_CMD_RESUME, data);
        lv_timer_resume(player->timer);
        break;
    case LV_AIC_PLAYER_CMD_GET_MEDIA_INFO:
        player_backend_control(player->ctx, PLAYER_CMD_GET_MEDIA_INFO, data);
        break;
    case LV_AIC_PLAYER_CMD_PLAY_END:
        player_backend_control(player->ctx, PLAYER_CMD_PLAY_END, data);
        break;
    case LV_AIC_PLAYER_CMD_SET_VOLUME:
        player_backend_control(player->ctx, PLAYER_CMD_SET_VOLUME, data);
        break;
    case LV_AIC_PLAYER_CMD_GET_VOLUME:
        player_backend_control(player->ctx, PLAYER_CMD_GET_VOLUME, data);
        break;
    case LV_AIC_PLAYER_CMD_SET_PLAY_TIME:
        player_reset_frame_sync(obj, data);
        player_backend_control(player->ctx, PLAYER_CMD_SET_PLAY_TIME, data);
        break;
    case LV_AIC_PLAYER_CMD_GET_PLAY_TIME:
        player_backend_control(player->ctx, PLAYER_CMD_GET_PLAY_TIME, data);
        break;
    case LV_AIC_PLAYER_CMD_ATTACH_SLAVE:
        lv_player_attach_slave(obj, data);
        lv_player_propagate_src_to_slaves(obj);
        break;
    case LV_AIC_PLAYER_CMD_ATTACH_GROUP:
        player->group = (lv_obj_t *)data;
        break;
    case LV_AIC_PLAYER_CMD_SET_PLAYBACK_RATE:
        if (!data) {
            LV_LOG_WARN("LV_AIC_PLAYER_CMD_SET_PLAYBACK_RATE: data is NULL");
            return;
        }
        float rate = *(float *)data;
        if (rate < 0.1f || rate > 10.0f) {
            LV_LOG_ERROR("Invalid playback rate: %.2f (valid range: 0.1 to 10.0)", rate);
            return;
        }
        if (player_backend_control(player->ctx, PLAYER_CMD_SET_PLAYBACK_RATE, &rate) == LV_RES_OK) {
            player->playback_rate = rate;
        }
        break;
    case LV_AIC_PLAYER_CMD_GET_PLAYBACK_RATE:
        if (!data) {
            LV_LOG_WARN("LV_AIC_PLAYER_CMD_GET_PLAYBACK_RATE: data is NULL");
            return;
        }
        *(float *)data = player->playback_rate;
        break;
    default:
        LV_LOG_WARN("unknown cmd %d", cmd);
        break;
    }
}

void lv_aic_player_set_auto_restart(lv_obj_t * obj, bool en)
{
    if (!obj || lv_player_is_slave(obj))
        return;

    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    player->auto_restart = en;
}

void lv_aic_player_set_draw_layer(lv_obj_t * obj, lv_aic_player_draw_layer_t layer)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    if (!obj || lv_player_is_slave(obj))
        return;
    player->draw_layer = layer;
}

lv_obj_t *lv_aic_slave_player_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_aic_slave_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

static lv_res_t player_backend_ensure(lv_obj_t *obj, const char *src)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    const player_backend_ops_t *backend = player_backend_match_file_suffix(src);

    if (!backend)
        return LV_RES_INV;

    bool need_setup = !player->ctx ||
                     (strncmp(player_backend_get_name((void *)backend),
                              player_backend_get_name(player->ctx),
                              32) != 0);

    if (need_setup) {
        if (player->ctx)
            player_backend_destroy(player->ctx);

        player->ctx = player_backend_create(backend);
        if (!player->ctx)
            return LV_RES_INV;

        player_backend_set_obj(player->ctx, obj);
    }

    return LV_RES_OK;
}

static void lv_player_refresh_next_frame_cb(lv_timer_t * timer)
{
    lv_obj_t * obj = (lv_obj_t *)timer->user_data;
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    bool play_end = false;
    bool wait_en = false;
    u64 seek_time = 0;

    if (!player->ctx)
        return;

    if (player_should_wait_sync(obj)) {
        lv_timer_ready(timer);
        return;
    }

    player_backend_control(player->ctx, PLAYER_CMD_PLAY_END, &play_end);

    if (player->auto_restart && play_end) {
        player_backend_control(player->ctx, PLAYER_CMD_SET_PLAY_TIME, &seek_time);
    }

    if (player_backend_control(player->ctx, PLAYER_CMD_GET_FRAME, &wait_en) == LV_RES_INV) {
        if (player_reset_frame_sync(obj, &seek_time))
            return;
        return;
    }

    player->cur_frame_index++;

    lv_player_invalidate_slaves(obj);

    lv_obj_invalidate(obj);

    lv_image_cache_drop(lv_image_get_src(obj)); /* clear the cache to make image_dst update automatically */
}

static void lv_aic_player_set_image_src(lv_obj_t *obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_image_t *image = (lv_image_t *)obj;
    struct av_media_info media_info = {0};
    void *image_src = NULL;

    if (player_backend_control(player->ctx, PLAYER_CMD_GET_IMAGE_SRC, &image_src) == LV_RES_INV) {
        return;
    }

    lv_image_set_src(obj, image_src);

    if (player_backend_control(player->ctx, PLAYER_CMD_GET_MEDIA_INFO, &media_info) == LV_RES_INV)
        return;

    image->w = media_info.video_stream.width;
    image->h = media_info.video_stream.height;

    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

static bool lv_player_is_slave(lv_obj_t *obj)
{
    return lv_obj_check_type(obj, &lv_aic_slave_class);
}

static void lv_player_attach_slave(lv_obj_t * obj, lv_obj_t * slave_obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_aic_slave_player_t * slave_player = (lv_aic_slave_player_t *)slave_obj;
    lv_obj_t *node = NULL;

    if (!slave_obj || slave_player->master)
        return;

    _LV_LL_READ(&player->slave_players, node) {
        lv_obj_t *bound_slave = *(lv_obj_t **)node;
        if (bound_slave == slave_obj)
            return;
    }

    lv_obj_t **new_node = (lv_obj_t **)_lv_ll_ins_tail(&player->slave_players);
    *new_node = slave_obj;
    slave_player->master = obj;
}

static void lv_player_propagate_src_to_slaves(lv_obj_t * obj)
{
    void *image_src = NULL;
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_obj_t *node = NULL;

    if (player_backend_control(player->ctx, PLAYER_CMD_GET_IMAGE_SRC, &image_src) == LV_RES_INV) {
        return;
    }

    _LV_LL_READ(&player->slave_players, node) {
        lv_obj_t *bound_slave = *(lv_obj_t **)node;
        lv_image_set_src(bound_slave, image_src);
    }
}

static void lv_player_invalidate_slaves(lv_obj_t *obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_obj_t *node = NULL;

    _LV_LL_READ(&player->slave_players, node) {
        lv_obj_t *bound_slave = *(lv_obj_t **)node;
        lv_obj_invalidate(bound_slave);
    }
}

static void lv_player_remove_all_slaves(lv_obj_t *obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_obj_t *node = NULL;

    _LV_LL_READ(&player->slave_players, node) {
        lv_obj_t *bound_slave = *(lv_obj_t **)node;
        lv_image_set_src(bound_slave, NULL);
    }

    _lv_ll_clear(&player->slave_players);
}

static bool player_should_wait_sync(lv_obj_t * obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_aic_player_group_t * group = (lv_aic_player_group_t *)player->group;

    if (!player->group) {
        return false;
    }

    /* verify all player in group have reached target frame */
    lv_obj_t *ll_obj;
    bool all_players_in_sync = true;
    _LV_LL_READ(&group->player_ll, ll_obj) {
        lv_aic_player_t *ll_player = (lv_aic_player_t *)*(lv_obj_t **)ll_obj;
        if (ll_player->cur_frame_index != group->target_frame_pos)
            all_players_in_sync = false;
    }

    if (all_players_in_sync)
        group->target_frame_pos++;

    if (player->cur_frame_index == group->target_frame_pos)
        return true;

    return false;
}

static bool player_reset_frame_sync(lv_obj_t *obj, void *data)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_aic_player_group_t * group = (lv_aic_player_group_t *)player->group;
    u64 play_time = *(u64 *)data;

    if (data == NULL)
        return false;

    if (group && play_time != 0) {
        LV_LOG_WARN("Enable frame sync, only supports time setting to 0");
        return false;
    }

    if (group && play_time == 0) {
        player->cur_frame_index = 0;
        group->target_frame_pos = 0;
        return true;
    }

    return false;
}

static void player_reset_frame_syc(lv_obj_t *obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;
    lv_aic_player_group_t * group = (lv_aic_player_group_t *)player->group;

    if (group) {
        player->cur_frame_index = 0;
        group->target_frame_pos = 0;
    }
}

static void player_set_width(lv_obj_t *obj, uint32_t width)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (lv_player_is_slave(obj))
        return;

    player->width = width;

    if (!player->ctx) {
        return;
    }

    struct av_media_info media_info = {0};
    if (player_backend_control(player->ctx, PLAYER_CMD_GET_MEDIA_INFO, &media_info) == LV_RES_INV) {
        LV_LOG_WARN("get media info failed");
        return;
    }

    int video_width = media_info.video_stream.width;
    uint32_t scale_x = ((double)(width * 1.0) / (double)(video_width * 1.0)) * 256;
    int16_t errors = ((scale_x * 1.0) * video_width/ 256) - width;

    if (errors)
        LV_LOG_WARN("player scaling width has precision errors = %d", errors);

    lv_obj_set_width(obj, width);
#if LVGL_VERSION_MAJOR == 9
    lv_image_set_scale_x(obj, scale_x);
#endif
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

static void player_set_height(lv_obj_t *obj, uint32_t height)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (lv_player_is_slave(obj))
        return;

    player->height = height;

    if (!player->ctx) {
        return;
    }

    struct av_media_info media_info = {0};
    if (player_backend_control(player->ctx, PLAYER_CMD_GET_MEDIA_INFO, &media_info) == LV_RES_INV) {
        LV_LOG_WARN("get media info failed");
        return;
    }

    int video_height = media_info.video_stream.height;
    uint32_t scale_y = ((double)(height * 1.0) / (double)(video_height * 1.0)) * 256;
    int16_t errors = ((scale_y * 1.0) * video_height) / 256 - height;

    if (errors)
        LV_LOG_WARN("player scaling height has precision errors = %d", errors);

    lv_obj_set_height(obj, height);
#if LVGL_VERSION_MAJOR == 9
    lv_image_set_scale_y(obj, scale_y);
#endif
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

static void player_reset_size(lv_obj_t *obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (lv_player_is_slave(obj))
        return;

    player->width = 0;
    player->height = 0;
}

static void player_update_size(lv_obj_t *obj)
{
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (player->width != 0)
        player_set_width(obj, player->width);
    if (player->height != 0)
        player_set_height(obj, player->height);
}

static void lv_aic_player_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    player->auto_restart = false;
    player->ctx          = NULL;
    player->draw_layer   = LV_AIC_PLAYER_LAYER_DEFAULT;
    player->timer        = NULL;
    player->playback_rate = 1.0f;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_aic_player_destructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if (lv_image_get_src(obj))
        lv_image_cache_drop(lv_image_get_src(obj));

    lv_aic_player_group_remove(player->group, obj);

    lv_player_remove_all_slaves(obj);

    player_backend_destroy(player->ctx);

    if (player->timer)
        lv_timer_del(player->timer);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_aic_player_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);
    /*Call the ancestor's event handler*/
    lv_result_t res = lv_obj_event_base(&lv_aic_player_class, e);
    if(res != LV_RESULT_OK) return;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_aic_player_t * player = (lv_aic_player_t *)obj;

    if(code == LV_EVENT_SIZE_CHANGED || code == LV_EVENT_STYLE_CHANGED || code == LV_EVENT_GET_SELF_SIZE) {
        if (player->ctx == NULL || strcmp(player_backend_get_name(player->ctx), "aic_player") != 0)
            return;

        if (code != LV_EVENT_GET_SELF_SIZE)
            lv_obj_update_layout(obj);

        if (player_backend_control(player->ctx, PLAYER_CMD_UPDATE_DISPLAY_AREA, NULL) == LV_RES_INV)
            return;

        if (code != LV_EVENT_GET_SELF_SIZE)
            lv_aic_player_set_image_src(obj);
    }
}

static void lv_aic_slave_player_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_aic_slave_player_t * player = (lv_aic_slave_player_t *)obj;
    player->master = NULL;
}

static void lv_aic_slave_player_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_aic_slave_player_t * slave = (lv_aic_slave_player_t *)obj;

    if (!slave->master)
        return;

    lv_aic_player_t * master = (lv_aic_player_t *)slave->master;
    lv_obj_t ** node = NULL;

    _LV_LL_READ(&master->slave_players, node) {
        if (*node == obj) {
            _lv_ll_remove(&master->slave_players, node);
            lv_free(node);
            slave->master = NULL;
            break;
        }
    }
}
#elif LV_USE_AIC_SIMULATOR == 1
lv_obj_t * lv_aic_player_create(lv_obj_t * parent)
{
    return lv_ffmpeg_player_create(parent);
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

lv_obj_t *lv_aic_slave_player_create(lv_obj_t * parent)
{
    LV_LOG_ERROR("Simulator did't support this function");
    return NULL;
}

void lv_aic_player_set_draw_layer(lv_obj_t * obj, lv_aic_player_draw_layer_t layer)
{
    return;
}
#endif

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
#if LV_USE_AIC_SIMULATOR == 0
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
#elif LVGL_VERSION_MAJOR == 9
    player_set_width(obj, width);
#endif
#endif
}

void lv_aic_player_set_height(lv_obj_t *obj, uint32_t height)
{
#if LV_USE_AIC_SIMULATOR == 0
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
#elif LVGL_VERSION_MAJOR == 9
    player_set_height(obj, height);
#endif
#endif
}

void lv_aic_player_set_scale(lv_obj_t * obj, uint32_t scale)
{
    player_reset_size(obj);
    lv_image_set_scale(obj, scale);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
}

void lv_aic_player_set_scale_x(lv_obj_t * obj, uint32_t scale_x)
{
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
#elif LVGL_VERSION_MAJOR == 9
    player_reset_size(obj);
    lv_image_set_scale_x(obj, scale_x);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
#endif
}

void lv_aic_player_set_scale_y(lv_obj_t * obj, uint32_t scale_y)
{
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
#elif LVGL_VERSION_MAJOR == 9
    player_reset_size(obj);
    lv_image_set_scale_y(obj, scale_y);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
#endif
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
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
#elif LVGL_VERSION_MAJOR == 9
    lv_image_set_inner_align(obj, align);
    lv_obj_send_event(obj, LV_EVENT_SIZE_CHANGED, NULL);
#endif
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
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
    return 0;
#elif LVGL_VERSION_MAJOR == 9
    return lv_image_get_scale_x(obj);
#endif
}

int32_t lv_aic_player_get_scale_y(lv_obj_t * obj)
{
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
    return 0;
#elif LVGL_VERSION_MAJOR == 9
    return lv_image_get_scale_y(obj);
#endif
}

int32_t lv_aic_player_get_offset_x(lv_obj_t * obj)
{
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
    return 0;
#else
    return lv_image_get_offset_x(obj);
#endif
}

int32_t lv_aic_player_get_offset_y(lv_obj_t * obj)
{
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
    return 0;
#else
    return lv_image_get_offset_y(obj);
#endif
}

lv_image_align_t lv_aic_player_get_inner_align(lv_obj_t * obj)
{
#if LVGL_VERSION_MAJOR == 8
    LV_LOG_WARN("This function is not implemented yet");
    return 0;
#elif LVGL_VERSION_MAJOR == 9
    return lv_image_get_inner_align(obj);
#endif
}

static void lv_aic_player_group_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_aic_player_group_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

const lv_obj_class_t lv_aic_player_group_class = {
    .constructor_cb = lv_aic_player_group_constructor,
    .destructor_cb = lv_aic_player_group_destructor,
    .instance_size = sizeof(lv_aic_player_group_t),
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .base_class = &lv_obj_class,
#if LVGL_VERSION_MAJOR == 9
    .name = "lv_aic_player_group",
#endif
};

static void lv_aic_player_group_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_aic_player_group_t * player_group = (lv_aic_player_group_t *)obj;
    _lv_ll_init(&player_group->player_ll, sizeof(lv_obj_t *));
}

static void lv_aic_player_group_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_aic_player_group_t * player_group = (lv_aic_player_group_t *)obj;

    while (!_lv_ll_is_empty(&player_group->player_ll)) {
        lv_obj_t ** ll_player = (lv_obj_t **)_lv_ll_get_head(&player_group->player_ll);
        lv_obj_t * player = *ll_player;

        lv_aic_player_t * player_obj = (lv_aic_player_t *)player;
        player_obj->group = NULL;

        _lv_ll_remove(&player_group->player_ll, ll_player);
        lv_free(ll_player);
    }
}

lv_obj_t *lv_aic_player_group_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_aic_player_group_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_aic_player_group_add(lv_obj_t * group, lv_obj_t * player)
{
    lv_aic_player_group_t * player_group = (lv_aic_player_group_t *)group;
    lv_obj_t ** ll_player = (lv_obj_t **)_lv_ll_ins_tail(&player_group->player_ll);
    if (group == NULL)
        return;
    *ll_player = player;
}

void lv_aic_player_group_remove(lv_obj_t * group, lv_obj_t * player)
{
    if (!group || !player)
        return;

    lv_aic_player_group_t * player_group = (lv_aic_player_group_t *)group;
    lv_aic_player_t * player_obj = (lv_aic_player_t *)player;

    lv_obj_t ** ll_player;
    _LV_LL_READ(&player_group->player_ll, ll_player) {
        if (*ll_player == player) {
            _lv_ll_remove(&player_group->player_ll, ll_player);
            lv_free(ll_player);
            break;
        }
    }

    if (player_obj->group == group) {
        player_obj->group = NULL;
    }
}

void lv_aic_player_group_set_cmd(lv_obj_t * group, lv_aic_player_cmd_t cmd, void *data)
{
    lv_aic_player_group_t * player_group = (lv_aic_player_group_t *)group;
    lv_obj_t *ll_obj;
    _LV_LL_READ(&player_group->player_ll, ll_obj) {
        lv_obj_t *player = *(lv_obj_t **)ll_obj;
        lv_aic_player_set_cmd(player, cmd, data);
    }
}
