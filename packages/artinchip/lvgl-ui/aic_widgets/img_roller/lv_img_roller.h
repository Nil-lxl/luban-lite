/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_IMG_ROLLER_H
#define LV_IMG_ROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#ifndef LV_USE_IMG_ROLLER
#define LV_USE_IMG_ROLLER 1
#endif

typedef enum {
    LV_ROLL_LOOP_OFF,
    LV_ROLL_LOOP_ON,
} lv_roll_loop_enable_t;

#if LV_USE_IMG_ROLLER

typedef struct {
    lv_obj_t obj;
    uint32_t img_cur;
    uint32_t img_last;
    uint32_t img_cnt;
    lv_dir_t img_direction;
    bool is_adjusting;
    lv_coord_t threshold;
    bool is_switching;
    lv_roll_loop_enable_t loop_en;
    lv_obj_t *obj_switch;
    lv_anim_enable_t anim_switch;
    uint8_t transform_ratio;
} lv_img_roller_t;

#if LVGL_VERSION_MAJOR > 8
LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_img_roller_class;
#else
extern const lv_obj_class_t lv_img_roller_class;
#endif

/**
 * Create a img_roller widget
 * @param parent    pointer to a parent widget
 * @return          the created img_roller
 */
lv_obj_t *lv_img_roller_create(lv_obj_t *parent);

/**
 * Set the direction of the img_roller
 * @param obj       pointer to a img_rolloer widget
 * @param dir       LV_DIR_HOR/LV_DIR_VER
 */
void lv_img_roller_set_direction(lv_obj_t *obj, lv_dir_t dir);

/**
 * Set the loop mode of img_rolloer
 * @param obj       pointer to a img_rolloer widget
 * @param loop_en   LV_ROLL_LOOP_ON/OFF
 */
void lv_img_roller_set_loop_mode(lv_obj_t *obj, lv_roll_loop_enable_t loop_en);

/**
 * Set the transform ratio when rolling
 * @param obj       pointer to a img_rolloer widget
 * @param ratio     suggest range [64, 256]
 */
void lv_img_roller_set_transform_ratio(lv_obj_t *obj, uint8_t ratio);

/**
 * Add a image to the img_rolloer
 * @param obj       pointer to a img_rolloer widget
 * @param src       1) pointer to an ::lv_image_dsc_t descriptor (converted by LVGL's image converter) (e.g. &my_img) or
 *                  2) path to an image file (e.g. "L:/rodata/img.png")or
 *                  3) a SYMBOL (e.g. LV_SYMBOL_OK)
 * @return          the widget where the content of the image can be created
 */

lv_obj_t *lv_img_roller_add_child(lv_obj_t *obj, const void *src);

/**
 * Get the number of children
 * @param obj       pointer to a img_rolloer widget
 * @return          the number of children
 */
uint32_t lv_img_roller_child_count(lv_obj_t *obj);


/**
 * Get the index of children
 * @param obj       pointer to a img_rolloer widget
 * @return          the index of children
 */
uint32_t lv_img_get_active_id(lv_obj_t *obj);

/**
 * Set Active Child
 * @param obj       pointer to a img_rolloer widget
 * @param id        the index of the child to Set
 * @param anim_en   LV_ANIM_ON/OFF
 */
void lv_img_roller_set_active(lv_obj_t *obj, uint32_t id, lv_anim_enable_t anim_en);

/**
 * Make a img_roller ready
 * @param obj  pointer to a img_rolloer widget
 *
 */
void lv_img_roller_ready(lv_obj_t *obj);


#endif /*LV_USE_IMG_ROLLER*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_USE_IMG_ROLLER */
