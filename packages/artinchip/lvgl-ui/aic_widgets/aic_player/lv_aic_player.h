/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

/**
 * @file aic_lv_aic.h
 *
 */
#ifndef LV_AIC_PLAYER_H
#define LV_AIC_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *     Explanation
 **********************/
/*
 *  +---------------------+       +---------------------+
 *  |   Main Player       |       |    Slave Player     |
 *  |  (Decode & Render)  |<----->|  (Display Only)     |
 *  |                     |       |                     |
 *  |[Frame Data] → Screen|       |[Same Frame] → Screen|
 *  +---------------------+       +---------------------+
 *
*/

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "./player_backend/player_backend_map.h"
#if defined(AIC_MPP_PLAYER_INTERFACE)
#include "aic_player.h"
#endif
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
extern const lv_obj_class_t lv_aic_player_class;
extern const lv_obj_class_t lv_aic_slave_class;

typedef struct {
    lv_image_t image;
    uint16_t draw_layer;              /* draw mode */
    bool auto_restart;
    uint32_t width;
    uint32_t height;
    uint32_t cur_frame_index;
    lv_obj_t *group;                  /* player group object */
    lv_ll_t slave_players;
    lv_timer_t * timer;
    void * ctx;
    float playback_rate;
} lv_aic_player_t;

typedef struct {
    lv_obj_t obj;
    lv_ll_t player_ll;
    uint32_t target_frame_pos;
} lv_aic_player_group_t;

typedef struct {
    lv_image_t image;
    lv_obj_t *master;
} lv_aic_slave_player_t;

typedef enum {
    LV_AIC_PLAYER_CMD_START,          /* data type is NULL * */
    LV_AIC_PLAYER_CMD_STOP,           /* data type is NULL * */
    LV_AIC_PLAYER_CMD_PAUSE,          /* data type is NULL * */
    LV_AIC_PLAYER_CMD_RESUME,         /* data type is NULL * */
    LV_AIC_PLAYER_CMD_PLAY_END,       /* data type is bool * */
    LV_AIC_PLAYER_CMD_GET_MEDIA_INFO, /* data type is struct av_media_info * */
    LV_AIC_PLAYER_CMD_SET_VOLUME,     /* data type is s32 *, rang: 0 ~ 100 */
    LV_AIC_PLAYER_CMD_GET_VOLUME,     /* data type is s32 *, rang: 0 ~ 100 */
    LV_AIC_PLAYER_CMD_SET_PLAY_TIME,  /* data type is u64 *, unite: microsecond */
    LV_AIC_PLAYER_CMD_GET_PLAY_TIME,  /* data type is u64 *, unite: microsecond */
    LV_AIC_PLAYER_CMD_ATTACH_SLAVE, /* data type is slave player */
    LV_AIC_PLAYER_CMD_ATTACH_GROUP,   /* data is lv_obj_t *, group object, auto frame sync */
    LV_AIC_PLAYER_CMD_SET_PLAYBACK_RATE, /* data type is float *, range: 0.1 ~ 10.0 */
    LV_AIC_PLAYER_CMD_GET_PLAYBACK_RATE, /* data type is float * */
} lv_aic_player_cmd_t;

typedef enum {
    LV_AIC_PLAYER_LAYER_DEFAULT,
    LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF, /* audio sync block the ui */
    LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF, /* audio sync not block the ui, but it requires more memory */
    LV_AIC_PLAYER_LAYER_VIDEO,
    LV_AIC_PLAYER_LAYER_NONE,
} lv_aic_player_draw_layer_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Create aic_player object
 * @param parent pointer to an object, it will be the parent of the new player
 * @return pointer to the created aic_player
 */
lv_obj_t * lv_aic_player_create(lv_obj_t * parent);

/**
 * Set draw mode of the player
 * @param obj pointer to a aic_player object
 * @param mode play mode
 * Note: Until the next src setting takes effect.
 */
void lv_aic_player_set_draw_layer(lv_obj_t * obj, lv_aic_player_draw_layer_t layer);

/**
 * Set the path of the file to be played
 * @param obj pointer to a aic_player object
 * @param path video file path
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info.
 */
lv_res_t lv_aic_player_set_src(lv_obj_t * obj, const char * src);

/**
 * Set command control video player
 * @param obj pointer to a aic_player object
 * @param cmd control commands
 */
void lv_aic_player_set_cmd(lv_obj_t * obj, lv_aic_player_cmd_t cmd, void *data);

/**
 * Set the video to automatically replay
 * @param obj pointer to a aic_player object
 * @param en true: enable the auto restart
 */
void lv_aic_player_set_auto_restart(lv_obj_t * obj, bool en);

/* the following function uses a function similar to lv_image_xxx_xxx */
void lv_aic_player_set_pivot(lv_obj_t * obj, int32_t x, int32_t y);

void lv_aic_player_set_rotation(lv_obj_t * obj, int32_t angle);

/* actual = set scale x and set obj width */
void lv_aic_player_set_width(lv_obj_t *obj, uint32_t width);

void lv_aic_player_set_height(lv_obj_t *obj, uint32_t height);

void lv_aic_player_set_scale(lv_obj_t * obj, uint32_t scale_y);

void lv_aic_player_set_scale_x(lv_obj_t * obj, uint32_t scale_x);

void lv_aic_player_set_scale_y(lv_obj_t * obj, uint32_t scale_y);

void lv_aic_player_set_inner_align(lv_obj_t * obj, lv_image_align_t align);

void lv_aic_player_set_offset_x(lv_obj_t * obj, int32_t x);

void lv_aic_player_set_offset_y(lv_obj_t * obj, int32_t y);

void lv_aic_player_get_pivot(lv_obj_t * obj, lv_point_t * pivot);

int32_t lv_aic_player_get_rotation(lv_obj_t * obj);

int32_t lv_aic_player_get_scale(lv_obj_t * obj);

int32_t lv_aic_player_get_scale_x(lv_obj_t * obj);

int32_t lv_aic_player_get_scale_y(lv_obj_t * obj);

int32_t lv_aic_player_get_offset_x(lv_obj_t * obj);

int32_t lv_aic_player_get_offset_y(lv_obj_t * obj);

lv_image_align_t lv_aic_player_get_inner_align(lv_obj_t * obj);

/* Slave Player: API call failed (set_src/auto_restart/set_cmd/set_draw_layer) */
lv_obj_t *lv_aic_slave_player_create(lv_obj_t * parent);

/* group control */
lv_obj_t *lv_aic_player_group_create(lv_obj_t * parent);

void lv_aic_player_group_add(lv_obj_t * group, lv_obj_t * player);

void lv_aic_player_group_remove(lv_obj_t * group, lv_obj_t * player);

void lv_aic_player_group_set_cmd(lv_obj_t * group, lv_aic_player_cmd_t cmd, void *data);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif
