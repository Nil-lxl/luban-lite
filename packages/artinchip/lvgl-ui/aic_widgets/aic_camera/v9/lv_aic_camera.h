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
#ifndef LV_AIC_CREAMER_H
#define LV_AIC_CREAMER_H

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
struct aic_camera_ctx_s;

extern const lv_obj_class_t lv_aic_camera_class;

/* in_data is decoded data from camera; out_data and out_length are for output barcode data, out_data == barcode_data  */
typedef void (*lv_aic_camera_barcode_cb_t)(const char *in_data, int in_length, char *out_data, int out_length);
typedef struct {
    lv_obj_t obj;
    uint32_t format;
#if AIC_CAMERA_USE_BARCODE
    bool barcode_only;
    bool barcode_en;
    char *barcode_data;
    int barcode_data_size;
    lv_aic_camera_barcode_cb_t barcode_cb;
#endif
    struct aic_camera_ctx_s * aic_ctx;
} lv_aic_camera_t;

typedef enum {
    LV_AIC_CAMERA_FORMAT_NV16,
    LV_AIC_CAMERA_FORMAT_NV12,
    LV_AIC_CAMERA_FORMAT_YUV400, /* only support 8bit raw camera */
    _LV_AIC_CAMERA_FORMAT_LAST
} lv_aic_camera_format;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Create a new aic_camera object.
 * @param parent pointer to an object, it will be the parent of the new camera object
 * @return pointer to the created aic_camera object
 */
lv_obj_t * lv_aic_camera_create(lv_obj_t * parent);

/**
 * Set the format of the output buffer for the camera.
 * @param obj pointer to an aic_camera object
 * @param format specifies the desired output buffer format for the camera
 * @return LV_RES_OK: no error; LV_RES_INV: invalid parameter or operation failed.
 */
lv_res_t lv_aic_camera_set_format(lv_obj_t * obj, lv_aic_camera_format format);

/**
 * Open the camera and allocate necessary resources for operation.
 * @param obj pointer to an aic_camera object
 * @return LV_RES_OK: no error; LV_RES_INV: unable to open the camera or allocate resources.
 */
lv_res_t lv_aic_camera_open(lv_obj_t * obj);

/**
 * Start the camera operation to begin capturing frames.
 * @param obj pointer to an aic_camera object
 * @return LV_RES_OK: no error; LV_RES_INV: unable to start the camera.
 */
lv_res_t lv_aic_camera_start(lv_obj_t * obj);

/**
 * Stop the camera operation and release allocated resources.
 * @param obj pointer to an aic_camera object
 * @return LV_RES_OK: no error; LV_RES_INV: unable to stop the camera or release resources.
 */
lv_res_t lv_aic_camera_stop(lv_obj_t * obj);

#if AIC_CAMERA_USE_BARCODE
 /* only decode barcode, no need to draw */
lv_res_t lv_aic_camera_barcode_only(lv_obj_t *obj);

lv_res_t lv_aic_camera_barcode_enable(lv_obj_t *obj, bool enable);

lv_res_t lv_aic_camera_barcode_disable(lv_obj_t *obj, bool disabled);

lv_res_t lv_aic_camera_barcode_callback(lv_obj_t *obj, lv_aic_camera_barcode_cb_t cb, char *data, int data_size);
#endif
#endif
#ifdef __cplusplus
} /*extern "C"*/
#endif

