/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ZeQuan Liang <zequan.liang@artinchip.com>
 */

#ifndef _LV_SWIPE_V1_H
#define _LV_SWIPE_V1_H

#ifdef __cplusplus
extern "C" {
#endif
/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_obj_t obj;
    uint16_t switching;
    uint16_t child_count;
    uint16_t anim_time;
    lv_anim_path_cb_t path;
    lv_ll_t child_ll;
} lv_swipe_v1_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief Create a swipe component object
 * @param parent Parent object
 * @return Pointer to the created swipe component object
 */
lv_obj_t *lv_swipe_v1_create(lv_obj_t *parent);

/**
 * @brief Add state resources (active/deactive) for a child element
 * @param obj          Swipe component object
 * @param child_idx    Child element index
 * @param active_src   Active state resource pointer (e.g., image path)
 * @param deactive_src Deactive state resource pointer
 */
void lv_swipe_v1_child_add_state_src(lv_obj_t *obj, uint16_t child_idx, void *active_src, void *deactive_src);

/**
 * @brief Switch to the next child element
 * @param obj     Swipe component object
 * @param enable  Whether to enable animation
 */
void lv_swipe_v1_set_next(lv_obj_t *obj, lv_anim_enable_t enable);

/**
 * @brief Switch to the previous child element
 * @param obj     Swipe component object
 * @param enable  Whether to enable animation
 */
void lv_swipe_v1_set_prev(lv_obj_t *obj, lv_anim_enable_t enable);

/**
 * @brief Set animation parameters
 * @param obj         Swipe component object
 * @param anim_time   Animation duration in milliseconds
 * @param path        Animation path callback function
 */
void lv_swipe_v1_set_anim_params(lv_obj_t *obj, uint16_t anim_time, lv_anim_path_cb_t path);

/**
 * @brief Set active state resource for a child element
 * @param obj          Swipe component object
 * @param child_idx    Child element index
 * @param src_idx      Resource index (e.g., Nth active resource set)
 */
void lv_swipe_v1_set_child_active_src(lv_obj_t *obj, uint16_t child_idx, uint16_t src_idx);

/**
 * @brief Set deactive state resource for a child element
 * @param obj          Swipe component object
 * @param child_idx    Child element index
 * @param src_idx      Resource index
 */
void lv_swipe_v1_set_child_deactive_src(lv_obj_t *obj, uint16_t child_idx, uint16_t src_idx);

/**
 * @brief Toggle active state of the current child element
 * @param obj Swipe component object
 */
void lv_swipe_v1_toggle_child_active(lv_obj_t *obj);

/**
 * @brief Get index of the currently active child element
 * @param obj Swipe component object
 * @return Index of active child, or -1 on failure
 */
int16_t lv_swipe_v1_get_active_child(lv_obj_t *obj);

/**
 * @brief Get total number of child elements
 * @param obj Swipe component object
 * @return Number of child elements
 */
uint16_t lv_swipe_v1_get_child_count(lv_obj_t *obj);

/**
 * @brief Get active resource index of a child element
 * @param obj        Swipe component object
 * @param child_idx  Child element index
 * @return Resource index, or -1 on failure
 */
int16_t lv_swipe_v1_get_child_active_src(lv_obj_t *obj, uint16_t child_idx);

/**
 * @brief Get child element object by index
 * @param obj        Swipe component object
 * @param child_idx  Child element index
 * @return Child object pointer, or NULL on failure
 */
lv_obj_t *lv_swipe_v1_get_child(lv_obj_t *obj, uint16_t child_idx);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
