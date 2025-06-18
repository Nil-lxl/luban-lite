/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */
#include "lv_aic_camera.h"

#if defined(AIC_USING_CAMERA) && defined(AIC_DISPLAY_DRV) && defined(AIC_DVP_DRV)
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpp_vin.h"
#include "drv_camera.h"

#include "artinchip_fb.h"
#include "mpp_fb.h"
#include "mpp_mem.h"

#if AIC_CAMERA_USE_BARCODE
#include "include/yydecoder.h"
#define BARCODE_DEMO_SUCCESS    1
#endif

#define MY_CLASS (&lv_aic_camera_class)
#define VID_BUF_NUM             3
#define VID_BUF_PLANE_NUM       2
#define VID_SCALE_OFFSET        0

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    LV_AIC_CAMERA_STATUS_INIT,
    LV_AIC_CAMERA_STATUS_READY,
    LV_AIC_CAMERA_STATUS_START,
    LV_AIC_CAMERA_STATUS_RUNNING,
    LV_AIC_CAMERA_STATUS_STOP,
    LV_AIC_CAMERA_STATUS_DELETE,
    _LV_AIC_CAMERA_STATUS_LAST
} lv_aic_camera_status;

struct aic_camera_ctx_s {
    int w;
    int h;
    int frame_size;
    int frame_cnt;
    int rotation;
    int dst_fmt;  // output format
    struct mpp_video_fmt src_fmt;
    uint32_t num_buffers;
    struct vin_video_buf binfo;
    struct mpp_rect dst_rect;

    struct mpp_fb *fb;
    struct aicfb_screeninfo fb_info;

    uint32_t status;
    lv_mutex_t mutex;
    lv_thread_sync_t video_sync;
    lv_thread_t video_thread;

#if AIC_CAMERA_USE_BARCODE
    lv_thread_sync_t barcdoe_sync;
    lv_thread_t barcdoe_thread;
    void *barcode_buffer;
    bool barcode_create;
    bool barcode_ready;
#endif
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_aic_camera_constructor(const lv_obj_class_t * class_p,
                                         lv_obj_t * obj);
static void lv_aic_camera_destructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj);
static void lv_aic_camera_event(const lv_obj_class_t * class_p, lv_event_t * e);

static int mpp_format_to_camera_format(lv_aic_camera_format format);
static int sensor_get_format(struct aic_camera_ctx_s *aic_ctx);
static int lv_camera_video_layer_set(struct aic_camera_ctx_s *aic_ctx, int index);
static int lv_camera_video_layer_disable(struct aic_camera_ctx_s *aic_ctx);
static int lv_camera_subdev_set(struct aic_camera_ctx_s *aic_ctx);
static int lv_camera_config(struct aic_camera_ctx_s *aic_ctx);
static int lv_camera_stop(void);
static int lv_camera_start(void);
static int lv_camera_dequeue_buf(int *index);
static int lv_camera_queue_buf(int index);
static int lv_camera_request_buf(struct vin_video_buf *vbuf);
static void lv_camera_release_buf(int num);
static void lv_camera_draw_video_layer_entry(void *ptr);

#if AIC_CAMERA_USE_BARCODE
static void barcode_decode_entry(void *ptr);
static void barcode_decode_create(struct aic_camera_ctx_s *aic_ctx);
static void barcode_decode_delete(struct aic_camera_ctx_s *aic_ctx);
static void barcode_decode_buffer_sync(struct aic_camera_ctx_s *aic_ctx, int index);
static void barcode_decode(lv_obj_t *obj);
#endif

const lv_obj_class_t lv_aic_camera_class = {
    .constructor_cb = lv_aic_camera_constructor,
    .destructor_cb = lv_aic_camera_destructor,
    .event_cb = lv_aic_camera_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_aic_camera_t),
    .base_class = &lv_obj_class,
    .name = "aic_camera",
};

lv_obj_t * lv_aic_camera_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_res_t lv_aic_camera_set_format(lv_obj_t * obj, lv_aic_camera_format format)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    camera->format = format;
    return LV_RES_OK;
}

lv_res_t lv_aic_camera_open(lv_obj_t * obj)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = NULL;
    bool vin_init_status = false;

    if (camera->aic_ctx == NULL) {
        aic_ctx = lv_mem_alloc(sizeof(struct aic_camera_ctx_s));
        if(aic_ctx == NULL) {
            LV_LOG_ERROR("lv_aic_camera_open failed");
            return LV_RES_INV;
        }
        lv_memset(aic_ctx, 0x0, sizeof(struct aic_camera_ctx_s));
        camera->aic_ctx = aic_ctx;
        aic_ctx->status = LV_AIC_CAMERA_STATUS_INIT;
    }

    if (aic_ctx->status != LV_AIC_CAMERA_STATUS_INIT) {
        LV_LOG_WARN("lv_aic_camera_open have opened, status = %ld, LV_AIC_CAMERA_STATUS_INIT = %d", aic_ctx->status, LV_AIC_CAMERA_STATUS_INIT);
        return LV_RES_OK;
    }

    if (mpp_vin_init(CAMERA_DEV_NAME)) {
        LV_LOG_ERROR("mpp_vin_init camera dev name %s", CAMERA_DEV_NAME);
        goto CAMERA_ERROR;
    }
    vin_init_status = true;

    if (sensor_get_format(aic_ctx) < 0) {
        LV_LOG_ERROR("sensor_get_format failed");
        goto CAMERA_ERROR;
    }

    aic_ctx->dst_fmt = mpp_format_to_camera_format(camera->format);
    if (aic_ctx->dst_fmt == MPP_FMT_NV16)
        aic_ctx->frame_size = aic_ctx->w * aic_ctx->h * 2;
    else if (aic_ctx->dst_fmt == MPP_FMT_NV12)
        aic_ctx->frame_size = (aic_ctx->w * aic_ctx->h * 3) >> 1;
    else if (aic_ctx->dst_fmt == MPP_FMT_YUV400)
        aic_ctx->frame_size = aic_ctx->w * aic_ctx->h;
    aic_ctx->num_buffers = VID_BUF_NUM;
    aic_ctx->rotation = MPP_ROTATION_0;

    if (lv_camera_subdev_set(aic_ctx) < 0) {
        LV_LOG_ERROR("lv_camera_subdev_set failed");
        goto CAMERA_ERROR;
    }

    aic_ctx->fb = mpp_fb_open();
    if (!aic_ctx->fb) {
        LV_LOG_ERROR("Failed to open FB");
        goto CAMERA_ERROR;
    }

    if (mpp_fb_ioctl(aic_ctx->fb, AICFB_GET_SCREENINFO, &aic_ctx->fb_info) < 0) {
        LV_LOG_ERROR("Failed to get screen info!");
        goto CAMERA_ERROR;
    }

    if (lv_camera_config(aic_ctx) < 0) {
        LV_LOG_ERROR("Flv_camera_config error");
        goto CAMERA_ERROR;
    }

    lv_obj_update_layout(obj);
    lv_area_t fill_area = {0};
    lv_obj_get_coords(obj, &fill_area);

    /* update the video layer */
    aic_ctx->dst_rect.x = fill_area.x1;
    aic_ctx->dst_rect.y = fill_area.y1;
    aic_ctx->dst_rect.width = fill_area.x2 - fill_area.x1 + 1;
    aic_ctx->dst_rect.height = fill_area.y2 - fill_area.y1 + 1;

    lv_mutex_init(&aic_ctx->mutex);
    lv_thread_sync_init(&aic_ctx->video_sync);
    lv_thread_init(&aic_ctx->video_thread, 20, lv_camera_draw_video_layer_entry, 4 * 1024, camera);

#if AIC_CAMERA_USE_BARCODE
    if (camera->barcode_en) {
        barcode_decode_create(aic_ctx);
        lv_thread_sync_init(&aic_ctx->barcdoe_sync);
        lv_thread_init(&aic_ctx->barcdoe_thread, 20, barcode_decode_entry, 32 * 1024, camera);
        aic_ctx->barcode_create = true;
        aic_ctx->barcode_ready = true;
    }
#endif
    aic_ctx->status = LV_AIC_CAMERA_STATUS_READY;

    lv_obj_refresh_self_size(obj);

    return LV_RES_OK;
CAMERA_ERROR:
    if (vin_init_status)
        mpp_vin_deinit();

    if (aic_ctx && aic_ctx->fb) {
        mpp_fb_close(aic_ctx->fb);
    }

    if (aic_ctx)
        lv_mem_free(aic_ctx);
    camera->aic_ctx = NULL;

    return LV_RES_INV;
}

lv_res_t lv_aic_camera_start(lv_obj_t * obj)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = camera->aic_ctx;
    if (aic_ctx && (aic_ctx->status == LV_AIC_CAMERA_STATUS_READY)) {
        aic_ctx->status = LV_AIC_CAMERA_STATUS_START;
        lv_thread_sync_signal(&camera->aic_ctx->video_sync);
        return LV_RES_OK;
    } else {
        LV_LOG_WARN("the camera status error, status = %ld", aic_ctx->status);
    }
    return LV_RES_INV;
}

lv_res_t lv_aic_camera_stop(lv_obj_t * obj)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = camera->aic_ctx;
    if (aic_ctx && (aic_ctx->status == LV_AIC_CAMERA_STATUS_RUNNING)) {
        lv_mutex_lock(&aic_ctx->mutex);
        aic_ctx->status = LV_AIC_CAMERA_STATUS_STOP;
        lv_mutex_unlock(&aic_ctx->mutex);
        return LV_RES_OK;
    } else {
        LV_LOG_WARN("the camera status error, status = %ld", aic_ctx->status);
    }
    return LV_RES_OK;
}

#if AIC_CAMERA_USE_BARCODE
lv_res_t lv_aic_camera_barcode_enable(lv_obj_t *obj, bool enable)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = camera->aic_ctx;
    if (aic_ctx) {
        LV_LOG_WARN("Before enable the barcode, please make sure the camera is not opened.");
        return LV_RES_INV;
    }
    camera->barcode_en = enable;
    return LV_RES_OK;
}

lv_res_t lv_aic_camera_barcode_disable(lv_obj_t *obj, bool disabled)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = camera->aic_ctx;
    if (aic_ctx) {
        LV_LOG_WARN("Before enable the barcode, please make sure the camera is not opened.");
        return LV_RES_INV;
    }
    camera->barcode_en = disabled;
    return LV_RES_OK;
}

lv_res_t lv_aic_camera_barcode_callback(lv_obj_t *obj, lv_aic_camera_barcode_cb_t cb, char *data, int data_size)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = camera->aic_ctx;

    if (aic_ctx) {
        LV_LOG_WARN("Before enable the barcode, please make sure the camera is not opened.");
        return LV_RES_INV;
    }

    camera->barcode_cb = cb;
    camera->barcode_data = data;
    camera->barcode_data_size = data_size;
    return LV_RES_OK;
}

lv_res_t lv_aic_camera_barcode_only(lv_obj_t *obj)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    camera->barcode_only = true;
    return LV_RES_OK;
}

static void barcode_decode_create(struct aic_camera_ctx_s *aic_ctx)
{
    int ret;
    static int barcode_init = 0;

    if (barcode_init == 0) {
        ret = Initial_Decoder();
        if (ret != BARCODE_DEMO_SUCCESS) {
            LV_LOG_WARN("Initial_Decoder failed with %d \n", ret);
        }
    }

    for (int i = 0; i <= 13; i++) {
        if (barcode_init == 0)
            Set_Donfig_Decoder(i, 1);
    }
    barcode_init = 1;

    /* only malloc the Y plane to barcode buffer */
    aic_ctx->barcode_buffer = aicos_malloc_try_cma(aic_ctx->w * aic_ctx->h);
    if (aic_ctx->barcode_buffer == NULL)
        LV_LOG_WARN("malloc barcode buffer failed, size: %d\n", aic_ctx->w * aic_ctx->h);
}

static void barcode_decode_delete(struct aic_camera_ctx_s *aic_ctx)
{
    if (aic_ctx->barcode_buffer)
        aicos_free(MEM_CMA, aic_ctx->barcode_buffer);
    aic_ctx->barcode_buffer = NULL;
}

static void barcode_decode_buffer_sync(struct aic_camera_ctx_s *aic_ctx, int index)
{
    struct vin_video_buf *binfo = &aic_ctx->binfo;
    unsigned long buf = binfo->planes[index * VID_BUF_PLANE_NUM].buf;
    int len = binfo->planes[index * VID_BUF_PLANE_NUM].len;

    /* only copy the Y plane to barcode buffer */
    aicos_dcache_invalid_range((void*)buf, len);
    aicos_memcpy(aic_ctx->barcode_buffer, (void *)buf, len);
}

static void barcode_decode(lv_obj_t *obj)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s *aic_ctx = camera->aic_ctx;
    unsigned char result[256] = {0};
    int len = 0;

    if (aic_ctx == NULL || aic_ctx->barcode_buffer == NULL)
        return;

    Decoding_Image(aic_ctx->barcode_buffer, aic_ctx->w, aic_ctx->h);
    len =  GetResultLength();
    if (len > 0) {
        if (len >= 255)
            LV_LOG_ERROR("barcode len too long, max len is 255");
        GetDecoderResult(result);
        if (camera->barcode_cb)
            camera->barcode_cb((const char *)result, len, camera->barcode_data, camera->barcode_data_size);
    }
}
#endif

static int mpp_format_to_camera_format(lv_aic_camera_format format)
{
    switch (format)
    {
    case LV_AIC_CAMERA_FORMAT_NV16:
        return MPP_FMT_NV16;
    case LV_AIC_CAMERA_FORMAT_NV12:
        return MPP_FMT_NV12;
    case LV_AIC_CAMERA_FORMAT_YUV400:
        return MPP_FMT_YUV400;
    default:
        return LV_AIC_CAMERA_FORMAT_NV16;
    }
    return LV_AIC_CAMERA_FORMAT_NV16;
}

static int lv_camera_video_layer_set(struct aic_camera_ctx_s *aic_ctx, int index)
{
    int i;
    struct aicfb_layer_data layer = {0};
    struct vin_video_buf *binfo = &aic_ctx->binfo;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    layer.scale_size.width = aic_ctx->dst_rect.width;
    layer.scale_size.height = aic_ctx->dst_rect.height;
    layer.pos.x = aic_ctx->dst_rect.x;
    layer.pos.y = aic_ctx->dst_rect.y;

    if (aic_ctx->rotation == MPP_ROTATION_0
        || aic_ctx->rotation == MPP_ROTATION_180) {
        layer.buf.size.width = aic_ctx->w;
        layer.buf.size.height = aic_ctx->h;
    } else {
        layer.buf.size.width = aic_ctx->h;
        layer.buf.size.height = aic_ctx->w;
    }

    layer.buf.format = aic_ctx->dst_fmt;
    layer.buf.buf_type = MPP_PHY_ADDR;

    for (i = 0; i < VID_BUF_PLANE_NUM; i++) {
        layer.buf.stride[i] = layer.buf.size.width;
        layer.buf.phy_addr[i] = binfo->planes[index * VID_BUF_PLANE_NUM + i].buf;
    }

    if (mpp_fb_ioctl(aic_ctx->fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0) {
        LV_LOG_ERROR("Failed to update layer config!, x %d y %d w %d h %d",
        aic_ctx->dst_rect.x, aic_ctx->dst_rect.y, aic_ctx->dst_rect.width, aic_ctx->dst_rect.height);
        return -1;
    }

    return 0;
}

static int lv_camera_video_layer_disable(struct aic_camera_ctx_s *aic_ctx)
{
    struct aicfb_layer_data layer = {0};
    layer.enable = 0;
    int ret = mpp_fb_ioctl(aic_ctx->fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0) {
        LV_LOG_ERROR("Failed to disable video layer!");
        return -1;
    }
    return 0;
}

static void lv_camera_draw_video_layer_entry(void *ptr)
{
    int index = 0, i;
    lv_aic_camera_t *camera = (lv_aic_camera_t *)ptr;
    struct aic_camera_ctx_s *aic_ctx = camera->aic_ctx;

    lv_thread_sync_wait(&aic_ctx->video_sync);

    if (lv_camera_request_buf(&aic_ctx->binfo) < 0) {
        LV_LOG_ERROR("lv_camera_request_buf error");
        return;
    }

    for (i = 0; i < VID_BUF_NUM; i++) {
        if (lv_camera_queue_buf(i) < 0) {
            LV_LOG_ERROR("lv_camera_queue_buf error");
            return;
        }
    }

    lv_mutex_lock(&aic_ctx->mutex);
    aic_ctx->status = LV_AIC_CAMERA_STATUS_RUNNING;
    lv_mutex_unlock(&aic_ctx->mutex);

    if (lv_camera_start() < 0) {
        LV_LOG_WARN("camera start error");
        return;
    }

    while (aic_ctx->status == LV_AIC_CAMERA_STATUS_RUNNING) {
        if (lv_camera_dequeue_buf(&index) < 0)
            break;

#if AIC_CAMERA_USE_BARCODE
        if (camera->barcode_en) {
            if (aic_ctx->barcode_ready == true && aic_ctx->barcode_buffer) {
                lv_mutex_lock(&aic_ctx->mutex);
                barcode_decode_buffer_sync(aic_ctx, index);
                lv_mutex_unlock(&aic_ctx->mutex);
                lv_thread_sync_signal(&aic_ctx->barcdoe_sync);
            } else {
                lv_mutex_lock(&aic_ctx->mutex);
                aic_ctx->barcode_ready = false;
                lv_mutex_unlock(&aic_ctx->mutex);
            }
        }
        if (camera->barcode_only == false && lv_camera_video_layer_set(aic_ctx, index) < 0) {
            LV_LOG_WARN("video_layer_set error");
            break;
        }
#else
        if (lv_camera_video_layer_set(aic_ctx, index) < 0) {
            LV_LOG_WARN("video_layer_set error");
            break;
        }
#endif

        lv_camera_queue_buf(index);
    }

    aic_ctx->status = LV_AIC_CAMERA_STATUS_DELETE;
}

#if AIC_CAMERA_USE_BARCODE
static void barcode_decode_entry(void *ptr)
{
    lv_aic_camera_t *camera = (lv_aic_camera_t *)ptr;
    struct aic_camera_ctx_s *aic_ctx = camera->aic_ctx;

    while (1) {
        lv_thread_sync_wait(&aic_ctx->barcdoe_sync);

        barcode_decode((lv_obj_t *)camera);
        lv_mutex_lock(&aic_ctx->mutex);
        aic_ctx->barcode_ready = true;
        lv_mutex_unlock(&aic_ctx->mutex);
    }
}
#endif

static int sensor_get_format(struct aic_camera_ctx_s *aic_ctx)
{
    int ret = 0;
    struct mpp_video_fmt fmt = {0};

    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &fmt);
    if (ret < 0) {
        LV_LOG_ERROR("Failed to get sensor format!");
        return -1;
    }
    aic_ctx->src_fmt = fmt;
    aic_ctx->w = aic_ctx->src_fmt.width;
    aic_ctx->h = aic_ctx->src_fmt.height;
    LV_LOG_INFO("Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x", fmt.width, fmt.height, fmt.code, fmt.bus_type, fmt.colorspace);

    if (fmt.bus_type == MEDIA_BUS_RAW8_MONO) {
        LV_LOG_INFO("Forbid the output format to YUV400");
        aic_ctx->dst_fmt = MPP_FMT_YUV400;
    }

    return 0;
}

static int lv_camera_subdev_set(struct aic_camera_ctx_s *aic_ctx)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &aic_ctx->src_fmt);
    if (ret < 0) {
        LV_LOG_ERROR("Failed to set DVP in-format!\n");
        return -1;
    }

    return 0;
}

static int lv_camera_config(struct aic_camera_ctx_s *aic_ctx)
{
    int ret = 0;
    struct dvp_out_fmt fmt = {0};

    fmt.width = aic_ctx->src_fmt.width;
    fmt.height = aic_ctx->src_fmt.height;
    fmt.pixelformat = aic_ctx->dst_fmt;
    fmt.num_planes = VID_BUF_PLANE_NUM;

    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &fmt);
    if (ret < 0) {
        LV_LOG_ERROR("Failed to set DVP out-format! err -%d", -ret);
        return -1;
    }

    return 0;
}

static int lv_camera_stop(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        LV_LOG_ERROR("Failed to stop streaming! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int lv_camera_start(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        LV_LOG_ERROR("Failed to start streaming! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int lv_camera_dequeue_buf(int *index)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        LV_LOG_ERROR("DQ failed! Maybe cannot receive data from Camera. err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int lv_camera_queue_buf(int index)
{
    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        LV_LOG_ERROR("Q failed! Maybe buf state is invalid.\n");
        return -1;
    }

    return 0;
}

int lv_camera_request_buf(struct vin_video_buf *vbuf)
{
    int i, min_num = 3;

    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)vbuf) < 0) {
        LV_LOG_ERROR("Failed to request buf!\n");
        return -1;
    }

    LV_LOG_INFO("Buf   Plane[0]     size   Plane[1]     size\n");
    for (i = 0; i < vbuf->num_buffers; i++) {
        LV_LOG_INFO("%3d 0x%x %8d 0x%x %8d\n", i,
            vbuf->planes[i * vbuf->num_planes].buf,
            vbuf->planes[i * vbuf->num_planes].len,
            vbuf->planes[i * vbuf->num_planes + 1].buf,
            vbuf->planes[i * vbuf->num_planes + 1].len);
    }


    if (vbuf->num_buffers < min_num) {
        LV_LOG_ERROR("The number of video buf must >= %d!\n", min_num);
        return -1;
    }

    return 0;
}

static void lv_camera_release_buf(int num) {;}

static void lv_aic_camera_constructor(const lv_obj_class_t * class_p,
                                        lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_aic_camera_t * camera = (lv_aic_camera_t *)obj;
    camera->format  = LV_AIC_CAMERA_FORMAT_NV16;
    camera->aic_ctx = NULL;
}

static void lv_aic_camera_destructor(const lv_obj_class_t * class_p,
                                       lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    lv_aic_camera_t * camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s * aic_ctx = camera->aic_ctx;

    if (aic_ctx) {
        lv_thread_delete(&aic_ctx->video_thread);

        lv_camera_release_buf(0);
        lv_camera_stop();
        lv_camera_video_layer_disable(aic_ctx);
        mpp_vin_deinit();
        mpp_fb_close(aic_ctx->fb);
#if AIC_CAMERA_USE_BARCODE
        if (aic_ctx->barcode_create) {
            lv_thread_delete(&aic_ctx->barcdoe_thread);
            lv_thread_sync_delete(&aic_ctx->barcdoe_sync);
            barcode_decode_delete(aic_ctx);
        }
#endif
        lv_mutex_delete(&aic_ctx->mutex);
        lv_thread_sync_delete(&aic_ctx->video_sync);
        free(aic_ctx);
    }
    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_aic_camera_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);
    static char fill_src[128] = {0};

    lv_event_code_t code = lv_event_get_code(e);
    /*Call the ancestor's event handler*/
    lv_result_t res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RESULT_OK) return;
    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_aic_camera_t *camera = (lv_aic_camera_t *)obj;
    struct aic_camera_ctx_s *aic_ctx = camera->aic_ctx;

    if (!aic_ctx)
        return;
    if(code == LV_EVENT_GET_SELF_SIZE) {
        lv_point_t * p = lv_event_get_param(e);
        p->x = aic_ctx->w;
        p->y = aic_ctx->h;
    } else if(code == LV_EVENT_DRAW_MAIN) {
#if AIC_CAMERA_USE_BARCODE
        if (camera->barcode_only)
            return;
#endif
        lv_area_t fill_area = {0};
        lv_obj_get_coords(obj, &fill_area);

        /* update the video layer */
        aic_ctx->dst_rect.x = fill_area.x1;
        aic_ctx->dst_rect.y = fill_area.y1;
        aic_ctx->dst_rect.width = fill_area.x2 - fill_area.x1 + 1;
        aic_ctx->dst_rect.height = fill_area.y2 - fill_area.y1 + 1;

        int alpha_en = 0;
        int width = fill_area.x2 - fill_area.x1 + 1;
        int height = fill_area.y2 - fill_area.y1 + 1;
        uint8_t bg_opa = lv_obj_get_style_bg_opa(obj, LV_PART_MAIN);
        lv_color_t bg_color = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
        uint32_t color = (bg_opa << 24) | (bg_color.red << 16) |
                        (bg_color.green << 8) | (bg_color.blue);
        snprintf(fill_src, sizeof(fill_src), "L:/%dx%d_%d_%08lx.fake",
                (int)width, (int)height, alpha_en, color);

        lv_draw_image_dsc_t draw_dsc;
        lv_draw_image_dsc_init(&draw_dsc);
        lv_obj_init_draw_image_dsc(obj, LV_PART_MAIN, &draw_dsc);
        draw_dsc.src = fill_src;
        lv_layer_t * layer = lv_event_get_layer(e);
        lv_draw_image(layer, &draw_dsc, &fill_area);
    }
}
#endif
