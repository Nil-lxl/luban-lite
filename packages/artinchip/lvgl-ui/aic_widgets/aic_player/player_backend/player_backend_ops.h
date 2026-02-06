/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"

typedef enum {
    PLAYER_STATUS_IDLE,
    PLAYER_STATUS_READY,
    PLAYER_STATUS_RUNNING,
    PLAYER_STATUS_STOP,
    PLAYER_STATUS_PAUSE,
    PLAYER_STATUS_END,
    _PLAYER_STATUS_LAST
} player_status_t;

typedef enum {
    PLAYER_CMD_START,
    PLAYER_CMD_STOP,
    PLAYER_CMD_PAUSE,
    PLAYER_CMD_RESUME,
    PLAYER_CMD_PLAY_END,
    PLAYER_CMD_GET_MEDIA_INFO,
    PLAYER_CMD_SET_VOLUME,
    PLAYER_CMD_GET_VOLUME,
    PLAYER_CMD_SET_PLAY_TIME,
    PLAYER_CMD_GET_PLAY_TIME,
    PLAYER_CMD_GET_FRAME,
    PLAYER_CMD_GET_IMAGE_SRC,
    PLAYER_CMD_UPDATE_DISPLAY_AREA,
    PLAYER_CMD_SET_PLAYBACK_RATE,
} player_cmd_t;

typedef struct {
    char *name;

    void *ctx;

    void *(*create)(void);

    void (*destroy)(void *ctx);

    lv_res_t (*set_src)(void *ctx, const char *src);

    lv_res_t (*control)(void *ctx, player_cmd_t cmd, void *data);

    const lv_obj_t *obj;    // private readonly
} player_backend_ops_t;

const player_backend_ops_t * player_backend_match_file_suffix(const char *src);

static inline void *player_backend_create(const player_backend_ops_t *ops) {
    if (!ops)
        return NULL;
    return ops->create();
}

static inline void player_backend_destroy(void *ctx) {
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    if (!ops)
        return;
    ops->destroy(ctx);
}

static inline lv_res_t player_backend_set_src(void *ctx, const char *src) {
    return ((player_backend_ops_t *)ctx)->set_src(ctx, src);
}

static inline lv_res_t player_backend_control(void *ctx, player_cmd_t cmd, void *data) {
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    if (!ops)
        return LV_RES_INV;
    return ops->control(ctx, cmd, data);
}

static inline void player_backend_set_obj(void *ctx, const lv_obj_t *obj) {
    if (!ctx)
        return;
    ((player_backend_ops_t *)ctx)->obj = obj;
}

static inline const char *player_backend_get_name(void *ctx)
{
    player_backend_ops_t *ops = (player_backend_ops_t *)ctx;
    if (!ops)
        return NULL;
    return ops->name;
}
