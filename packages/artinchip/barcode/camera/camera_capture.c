/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#include <string.h>
#include "aic_core.h"
#include "aic_log.h"
#include "drv_camera.h"
#include "mpp_vin.h"

#include "camera/camera_capture.h"
#include "config/barcode_config.h"

#ifndef CAMERA_DEV_NAME
#define CAMERA_DEV_NAME "/dev/video0"
#endif

static int dvp_cfg(camera_capture_t *cam)
{
    int ret;
    struct dvp_out_fmt f = {0};

    f.width = cam->src_fmt.width;
    f.height = cam->src_fmt.height;
    f.pixelformat = cam->dst_fmt;
    f.framesize = cam->frame_size;
    f.num_planes = BARCODE_VIDEO_BUF_PLANE_NUM;

    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &f);
    if (ret < 0) {
        pr_err("DVP: ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int dvp_subdev_set_fmt(camera_capture_t *cam)
{
    int ret;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &cam->src_fmt);
    if (ret < 0) {
        pr_err("DVP: ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int sensor_get_fmt(camera_capture_t *cam)
{
    int ret;
    struct mpp_video_fmt f = {0};

    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &f);
    if (ret < 0) {
        pr_err("DVP: ioctl() failed! err -%d\n", -ret);
        return -1;  /* Return error instead of continuing with invalid format */
    }

    cam->src_fmt = f;
    cam->width = cam->src_fmt.width;
    cam->height = cam->src_fmt.height;
    pr_info("Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x\n",
            f.width, f.height, f.code, f.bus_type, f.colorspace);

    if (f.bus_type == MEDIA_BUS_RAW8_MONO) {
        pr_info("Camera: RAW8_MONO sensor detected, switching to YUV400\n");
        cam->dst_fmt = MPP_FMT_YUV400;
    } else {
        pr_info("Camera: using default NV12 format\n");
    }

    return 0;
}

int camera_capture_init(camera_capture_t *cam, const char *dev_name)
{
    memset(cam, 0, sizeof(camera_capture_t));
    cam->dst_fmt = MPP_FMT_NV12;
    cam->initialized = false;

    if (mpp_vin_init((char *)(dev_name ? dev_name : CAMERA_DEV_NAME))) {
        pr_err("Camera: mpp_vin_init failed\n");
        return -1;
    }

    cam->initialized = true;  /* Mark as initialized */

    if (sensor_get_fmt(cam) < 0) {
        goto error_out;
    }

    if (dvp_subdev_set_fmt(cam) < 0) {
        goto error_out;
    }

    if (cam->dst_fmt == MPP_FMT_NV16) {
        cam->frame_size = cam->width * cam->height * 2;
    } else if (cam->dst_fmt == MPP_FMT_NV12) {
        cam->frame_size = (cam->width * cam->height * 3) >> 1;
    } else if (cam->dst_fmt == MPP_FMT_YUV400) {
        cam->frame_size = cam->width * cam->height;
    } else {
        pr_err("Camera: unsupported format %d\n", cam->dst_fmt);
        goto error_out;
    }

    if (dvp_cfg(cam) < 0) {
        goto error_out;
    }

    cam->streaming = false;
    return 0;

error_out:
    mpp_vin_deinit();
    cam->initialized = false;  /* Reset flag on error */
    return -1;
}

void camera_capture_deinit(camera_capture_t *cam)
{
    if (!cam) {
        return;
    }

    if (cam->streaming) {
        camera_capture_stop(cam);
    }

    if (cam->initialized) {
        mpp_vin_deinit();
        cam->initialized = false;
    }

    memset(cam, 0, sizeof(camera_capture_t));
}

int camera_capture_start(camera_capture_t *cam)
{
    int ret;

    if (!cam) {
        return -1;
    }

    // Request buffers
    cam->buf_info.num_buffers = BARCODE_VIDEO_BUF_NUM;
    cam->buf_info.num_planes = BARCODE_VIDEO_BUF_PLANE_NUM;
    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)&cam->buf_info) < 0) {
        pr_err("DVP: request buffer failed\n");
        return -1;
    }

    pr_info("Buf   Plane[0]     size   Plane[1]     size\n");
    for (int i = 0; i < cam->buf_info.num_buffers; i++) {
        pr_info("%3d 0x%x %8d 0x%x %8d\n", i,
            cam->buf_info.planes[i * BARCODE_VIDEO_BUF_PLANE_NUM].buf,
            cam->buf_info.planes[i * BARCODE_VIDEO_BUF_PLANE_NUM].len,
            cam->buf_info.planes[i * BARCODE_VIDEO_BUF_PLANE_NUM + 1].buf,
            cam->buf_info.planes[i * BARCODE_VIDEO_BUF_PLANE_NUM + 1].len);
    }

    // Validate buffer count
    if (cam->buf_info.num_buffers != BARCODE_VIDEO_BUF_NUM) {
        pr_warn("Camera: buffer count mismatch (requested %d, got %d)\n",
                BARCODE_VIDEO_BUF_NUM, cam->buf_info.num_buffers);
    }

    // Queue all buffers
    for (int i = 0; i < BARCODE_VIDEO_BUF_NUM; i++) {
        if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)i) < 0) {
            pr_err("DVP: queue buffer %d failed\n", i);
            goto error_cleanup;
        }
    }

    // Start streaming
    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        pr_err("DVP: stream on failed! err -%d\n", -ret);
        goto error_cleanup;
    }

    cam->streaming = true;
    return 0;

error_cleanup:
    /* Release buffers on error to prevent memory leak */
    mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    return -1;
}

void camera_capture_stop(camera_capture_t *cam)
{
    int ret;

    if (!cam || !cam->streaming) {
        return;
    }

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        pr_err("DVP: stream off failed! err -%d\n", -ret);
        // Do not set streaming to false if stop failed
        return;
    }

    cam->streaming = false;
}

int camera_capture_dequeue(camera_capture_t *cam, int *index)
{
    int ret;

    if (!cam || !index) {
        return -1;
    }

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        pr_err("DVP: dequeue buffer failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int camera_capture_queue(camera_capture_t *cam, int index)
{
    if (!cam) {
        return -1;
    }

    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        pr_err("DVP: queue buffer failed\n");
        return -1;
    }

    return 0;
}

u32 camera_capture_get_timestamp(camera_capture_t *cam, int index)
{
    if (!cam) {
        return 0;
    }

    return mpp_dvp_ioctl(DVP_GET_TIMESTAMP, (void *)(ptr_t)index);
}

int camera_capture_copy_frame(camera_capture_t *cam, int index,
                               unsigned char *dst, int dst_size)
{
    unsigned long buf;
    int len;

    if (!cam || !dst || dst_size <= 0) {
        return -1;
    }

    buf = cam->buf_info.planes[index * BARCODE_VIDEO_BUF_PLANE_NUM].buf;
    len = cam->buf_info.planes[index * BARCODE_VIDEO_BUF_PLANE_NUM].len;

    // Validate buffer address and length
    if (buf == 0 || len == 0) {
        pr_err("Camera: invalid buffer address or length (buf=0x%lx, len=%d)\n", buf, len);
        return -1;
    }

    if (len > dst_size) {
        pr_err("Camera: frame size %d exceeds dst size %d\n", len, dst_size);
        return -1;
    }

    aicos_dcache_invalid_range((void*)buf, len);
    aicos_memcpy(dst, (void *)buf, len);

    return 0;
}
