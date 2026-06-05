/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpp_vin.h"
#include "drv_camera.h"
#include "artinchip_fb.h"
#include "mpp_fb.h"
#include "mpp_ge.h"
#include "mpp_vin.h"
#include <stdbool.h>

typedef struct {
    int width;
    int height;
    int frame_size;
    int dst_fmt;
    struct mpp_video_fmt src_fmt;
    struct vin_video_buf buf_info;
    bool streaming;
    bool initialized;  /* Track if mpp_vin_init was successful */
} camera_capture_t;

/**
 * Initialize camera capture
 * @param cam Camera handle
 * @param dev_name Device name
 * @return 0 on success, negative value on failure
 */
int camera_capture_init(camera_capture_t *cam, const char *dev_name);

/**
 * Deinitialize camera capture
 * @param cam Camera handle
 */
void camera_capture_deinit(camera_capture_t *cam);

/**
 * Start video stream
 * @param cam Camera handle
 * @return 0 on success, negative value on failure
 */
int camera_capture_start(camera_capture_t *cam);

/**
 * Stop video stream
 * @param cam Camera handle
 */
void camera_capture_stop(camera_capture_t *cam);

/**
 * Dequeue to get a frame
 * @param cam Camera handle
 * @param index Output buffer index
 * @return 0 on success, negative value on failure
 */
int camera_capture_dequeue(camera_capture_t *cam, int *index);

/**
 * Queue buffer
 * @param cam Camera handle
 * @param index Buffer index
 * @return 0 on success, negative value on failure
 */
int camera_capture_queue(camera_capture_t *cam, int index);

/**
 * Get frame timestamp
 * @param cam Camera andle
 * @param index Buffer index
 * @return Timestamp
 */
u32 camera_capture_get_timestamp(camera_capture_t *cam, int index);

/**
 * Copy frame data to destination buffer
 * @param cam Camera handle
 * @param index Buffer index
 * @param dst Destination buffer
 * @param dst_size Destination buffer size
 * @return 0 on success, negative value on failure
 */
int camera_capture_copy_frame(camera_capture_t *cam, int index,
                               unsigned char *dst, int dst_size);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_CAPTURE_H */
