/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
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

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#if defined(AIC_MPP_PLAYER_INTERFACE)
#include "aic_player.h"
#endif
/*********************
 *      DEFINES
 *********************/
#define lv_image_align_t uint8_t
/**********************
 *      TYPEDEFS
 **********************/
struct aic_player_ctx_s;

extern const lv_obj_class_t lv_aic_player_class;

typedef struct {
    lv_img_t img;
    lv_img_dsc_t img_dst;
    uint32_t draw_layer;              /**< draw mode */
    bool auto_restart;
    bool keep_last_frame;
    lv_timer_t * timer;
    struct aic_player_ctx_s * aic_ctx;
} lv_aic_player_t;

typedef enum {
    LV_AIC_PLAYER_CMD_START,          /* data type is NULL * */
    LV_AIC_PLAYER_CMD_STOP,           /* data type is NULL * */
    LV_AIC_PLAYER_CMD_PAUSE,          /* data type is NULL * */
    LV_AIC_PLAYER_CMD_RESUME,         /* data type is NULL * */
    LV_AIC_PLAYER_CMD_PLAY_END,       /* data type is NULL * */
    LV_AIC_PLAYER_CMD_GET_MEDIA_INFO, /* data type is struct av_media_info * */
    LV_AIC_PLAYER_CMD_SET_VOLUME,     /* data type is s32 *, rang: 0 ~ 100 */
    LV_AIC_PLAYER_CMD_GET_VOLUME,     /* data type is s32 *, rang: 0 ~ 100 */
    LV_AIC_PLAYER_CMD_SET_PLAY_TIME,  /* data type is u64 *, unite: microsecond */
    LV_AIC_PLAYER_CMD_GET_PLAY_TIME,  /* data type is u64 *, unite: microsecond */
    LV_AIC_PLAYER_CMD_KEEP_LAST_FRAME_EX, /* data is NULL, caching last frame on multiple src switches avoids black screen */
} lv_aic_player_cmd_t;

typedef enum {
    LV_AIC_PLAYER_LAYER_DEFAULT,
    LV_AIC_PLAYER_LAYER_UI_SINGLE_BUF, /**< need ex memory, but more flexible */
    LV_AIC_PLAYER_LAYER_UI_DOUBLE_BUF, /**< need ex memory, but more flexible */
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

/* the following function uses a function similar to lv_img_xxx_xxx */
void lv_aic_player_set_pivot(lv_obj_t * obj, int32_t x, int32_t y);

void lv_aic_player_set_rotation(lv_obj_t * obj, int32_t angle);

void lv_aic_player_set_scale(lv_obj_t * obj, uint32_t scale_y);

void lv_aic_player_set_scale_x(lv_obj_t * obj, uint32_t scale_x);

void lv_aic_player_set_scale_y(lv_obj_t * obj, uint32_t scale_y);

void lv_aic_player_set_offset_x(lv_obj_t * obj, int32_t x);

void lv_aic_player_set_offset_y(lv_obj_t * obj, int32_t y);

void lv_aic_player_set_inner_align(lv_obj_t * obj, lv_image_align_t align);

void lv_aic_player_get_pivot(lv_obj_t * obj, lv_point_t * pivot);

int32_t lv_aic_player_get_rotation(lv_obj_t * obj);

int32_t lv_aic_player_get_scale(lv_obj_t * obj);

int32_t lv_aic_player_get_scale_x(lv_obj_t * obj);

int32_t lv_aic_player_get_scale_y(lv_obj_t * obj);

int32_t lv_aic_player_get_offset_x(lv_obj_t * obj);

int32_t lv_aic_player_get_offset_y(lv_obj_t * obj);

lv_image_align_t lv_aic_player_get_inner_align(lv_obj_t * obj);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif
