/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <posix/string.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"
#include "aic_utils.h"
#include "aic_drv_gpio.h"

#include "drv_dvp.h"
#include "mpp_vin.h"
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif

#ifdef AIC_DISPLAY_DRV
#include "artinchip_fb.h"
#include "mpp_fb.h"
#endif

#if defined(AIC_USING_GE) && !defined(AIC_CHIP_D13X)
#define SUPPORT_ROTATION
#include "mpp_ge.h"
#endif

/* MUST and ONLY enable one of follow mode: */
#define DE_SCALE_ENABLE        0
#define DE_CROP_ENABLE         0
#define DVP_CROP_ENABLE        1

/* Global macro and variables */

#define VID_BUF_NUM             3
#define VID_BUF_PLANE_NUM       2
#define VID_SCALE_OFFSET        0

#define VID_DROP_FRESH_FRAME    1

#define MAX_DVP_CHANNELS        2

enum dvp_state {
    DVP_STATUS_INIT,
    DVP_STATUS_READY,
    DVP_STATUS_PLAY,
    DVP_STATUS_PAUSE
};

static enum dvp_state g_dvp_status[MAX_DVP_CHANNELS] = {
    DVP_STATUS_INIT,
    DVP_STATUS_INIT
};

static const char sopts[] = "f:c:a:t:h";
static const struct option lopts[] = {
    {"format",        required_argument, NULL, 'f'},
    {"capture",       required_argument, NULL, 'c'},
    {"angle",         required_argument, NULL, 'a'},
    {"channel",       required_argument, NULL, 't'},
    {"usage",               no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

struct aic_dvp_data {
    int w;
    int h;
    int frame_size;
    int frame_cnt;
    int fresh_frame;
    int rotation;
    u32 channel;
    struct mpp_rect dst_pos;        // position in DE video player

    struct mpp_video_fmt src_fmt;   // format of DVP input, i.e. camera output
    struct dvp_out_fmt   dst_fmt;   // format of DVP output
    uint32_t num_buffers;
    struct vin_video_buf binfo;

    aicos_thread_t thread;
    int thread_running;
};

static struct aic_dvp_data g_vdata[MAX_DVP_CHANNELS] = {0};
#ifdef AIC_DISPLAY_DRV
static struct mpp_fb *g_fb = NULL;
static struct aicfb_screeninfo g_fb_info = {0};
#endif
#ifdef SUPPORT_ROTATION
static struct mpp_ge *g_ge_dev = NULL;
#endif

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -f, --format\t\tformat of input video, NV16/NV12/YUV400 etc\n");
    printf("\t -c, --count\t\tthe number of capture frame.(0 means infinity) \n");
#ifdef SUPPORT_ROTATION
    printf("\t -a, --angle\t\tthe angle of rotation \n");
#endif
    printf("\t -t, --channel\t\tthe channel number (0:CH0 1:CH1 2:BOTH)\n");
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -f nv16 -c 10 -t 0\n", program);
    printf("Example: %s -f nv16 -c 10 -t 1\n", program);
    printf("Example: %s -f nv16 -c 10 -t 2  # Both channels\n", program);
}

int get_fb_info(void)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV
    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0)
        pr_err("Failed to get screen info! errno: -%d\n", -ret);
#endif
    pr_info("Screen width: %d, height %d\n",
            g_fb_info.width, g_fb_info.height);
    return ret;
}

int set_ui_layer_alpha(int val)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;

    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);

    if (ret < 0)
        pr_err("Failed to update alpha! errno: -%d\n", -ret);
#endif
    return ret;
}

int sensor_get_fmt(u32 ch)
{
    int ret = 0;
    struct mpp_video_fmt f = {0};

    ret = mpp_dvp2_ioctl(DVP_IN_G_FMT, &f, ch);
    if (ret < 0) {
        pr_err("Failed to get sensor format for channel %d! err -%d\n", ch, -ret);
        return -1;
    }

    g_vdata[ch].src_fmt = f;
    g_vdata[ch].w = g_vdata[ch].src_fmt.width;
    g_vdata[ch].h = g_vdata[ch].src_fmt.height;
    pr_info("[ch%d] Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x\n",
            ch, f.width, f.height, f.code, f.bus_type, f.colorspace);

    if (f.bus_type == MEDIA_BUS_RAW8_MONO) {
        pr_info("[ch%d] Forbid the output format to YUV400\n", ch);
        g_vdata[ch].dst_fmt.pixelformat = MPP_FMT_YUV400;
    }

    return 0;
}

int dvp_subdev_set_fmt(u32 ch)
{
    int ret = 0;

    ret = mpp_dvp2_ioctl(DVP_IN_S_FMT, &g_vdata[ch].src_fmt, ch);
    if (ret < 0) {
        pr_err("Failed to set DVP in-format for channel %d! err -%d\n", ch, -ret);
        return -1;
    }

    return 0;
}

int dvp_cfg(u32 ch)
{
    struct mpp_video_fmt *src = &g_vdata[ch].src_fmt;
    struct dvp_out_fmt *dst = &g_vdata[ch].dst_fmt;
    int ret = 0;

#if DVP_CROP_ENABLE
    /* Crop the camera image in center-aligned way */
    if (src->width > g_fb_info.width) {
        dst->width = g_fb_info.width;
        dst->crop_x = (src->width - g_fb_info.width) / 2;
    } else {
        dst->width = src->width;
    }

    if (src->height > g_fb_info.height) {
        dst->height = g_fb_info.height;
        dst->crop_y = (src->height - g_fb_info.height) / 2;
    } else {
        dst->height = src->height;
    }
    pr_info("[ch%d] DVP crop: x %d, y %d, w %d, h %d\n",
            ch, dst->crop_x, dst->crop_y, dst->width, dst->height);
#else
    dst->width = src->width;
    dst->height = src->height;
#endif

    if (dst->pixelformat == MPP_FMT_NV16)
        g_vdata[ch].frame_size = g_vdata[ch].w * g_vdata[ch].h * 2;
    else if (dst->pixelformat == MPP_FMT_NV12)
        g_vdata[ch].frame_size = (g_vdata[ch].w * g_vdata[ch].h * 3) >> 1;
    else if (dst->pixelformat == MPP_FMT_YUV400)
        g_vdata[ch].frame_size = g_vdata[ch].w * g_vdata[ch].h;

    dst->num_planes = VID_BUF_PLANE_NUM;
    dst->frame_offset = 0;

    ret = mpp_dvp2_ioctl(DVP_OUT_S_FMT, dst, ch);
    if (ret < 0) {
        pr_err("Failed to set DVP out-format for channel %d! err -%d\n", ch, -ret);
        return -1;
    }

    return 0;
}

int dvp_request_buf(struct vin_video_buf *vbuf, u32 ch)
{
    int i, min_num = VID_BUF_NUM;

    if (mpp_dvp2_ioctl(DVP_REQ_BUF, (void *)vbuf, ch) < 0) {
        pr_err("Failed to request buf for channel %d!\n", ch);
        return -1;
    }

    pr_info("[ch%d] Buf Plane[0]   size   Plane[1]   size\n", ch);
    for (i = 0; i < vbuf->num_buffers; i++) {
        pr_info("      %3d 0x%08x %-6d 0x%08x %-6d\n", i,
            vbuf->planes[i * vbuf->num_planes].buf,
            vbuf->planes[i * vbuf->num_planes].len,
            vbuf->planes[i * vbuf->num_planes + 1].buf,
            vbuf->planes[i * vbuf->num_planes + 1].len);
    }

#ifdef SUPPORT_ROTATION
    if (g_vdata[ch].rotation)
        min_num++;

    g_vdata[ch].num_buffers = g_vdata[ch].binfo.num_buffers - 1;
#else
    g_vdata[ch].num_buffers = g_vdata[ch].binfo.num_buffers;
#endif

    if (vbuf->num_buffers < min_num) {
        pr_err("[ch%d] The number of video buf must >= %d!\n", ch, min_num);
        return -1;
    }

    return 0;
}

void dvp_release_buf(int num, u32 ch)
{
#if 0
    int i;
    struct video_buf_info *binfo = NULL;

    for (i = 0; i < num; i++) {
        binfo = &g_vdata[ch].binfo[i];
        if (binfo->vaddr) {
            munmap(binfo->vaddr, binfo->len);
            binfo->vaddr = NULL;
        }
    }
#endif
}

int dvp_queue_buf(int index, u32 ch)
{
    if (mpp_dvp2_ioctl(DVP_Q_BUF, (void *)(ptr_t)index, ch) < 0) {
        pr_err("[ch%d] Q failed! Maybe buf state is invalid.\n", ch);
        return -1;
    }

    return 0;
}

int dvp_dequeue_buf(int *index, u32 ch)
{
    int ret = 0;

    ret = mpp_dvp2_ioctl(DVP_DQ_BUF, (void *)index, ch);
    if (ret < 0) {
        pr_err("[ch%d] DQ failed! Maybe cannot receive data from Camera. err -%d\n", ch, -ret);
        return -1;
    }

    return 0;
}

static int dvp_start(u32 ch)
{
    int ret = 0;

    if (g_dvp_status[ch] != DVP_STATUS_READY) {
        pr_err("[ch%d] Invalid status: %d\n", ch, g_dvp_status[ch]);
        return -1;
    }

    ret = mpp_dvp2_ioctl(DVP_STREAM_ON, NULL, ch);
    if (ret < 0) {
        pr_err("[ch%d] Failed to start streaming! err -%d\n", ch, -ret);
        return -1;
    }

    return 0;
}

static int dvp_stop(u32 ch)
{
    int ret = 0;

    ret = mpp_dvp2_ioctl(DVP_STREAM_OFF, NULL, ch);
    if (ret < 0) {
        pr_err("[ch%d] Failed to stop streaming! err -%d\n", ch, -ret);
        return -1;
    }

    return 0;
}

int video_layer_disable(void)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV
    struct aicfb_layer_data layer = {0};
    layer.enable = 0;
    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;

    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer);

    if (ret < 0)
        pr_err("Failed to disable video layer!\n");

#endif
    return ret;
}

#ifdef SUPPORT_ROTATION
int do_rotate(struct aic_dvp_data *vdata, int index)
{
    struct ge_bitblt blt = {0};
    struct mpp_buf  *src = &blt.src_buf;
    struct mpp_buf  *dst = &blt.dst_buf;
    struct vin_video_buf *binfo = &vdata->binfo;
    int ret = 0;

    src->format = vdata->dst_fmt.pixelformat;
    src->buf_type = MPP_PHY_ADDR;
    src->phy_addr[0] = binfo->planes[index * VID_BUF_PLANE_NUM].buf;
    src->phy_addr[1] = binfo->planes[index * VID_BUF_PLANE_NUM + 1].buf;
    src->stride[0] = vdata->w;
    src->stride[1] = vdata->w;
    src->size.width = vdata->w;
    src->size.height = vdata->h;

    dst->format = vdata->dst_fmt.pixelformat;
    dst->buf_type = MPP_PHY_ADDR;
    dst->phy_addr[0] = binfo->planes[vdata->num_buffers * VID_BUF_PLANE_NUM].buf;
    dst->phy_addr[1] = binfo->planes[vdata->num_buffers * VID_BUF_PLANE_NUM + 1].buf;
    if (vdata->rotation == MPP_ROTATION_0
        || vdata->rotation == MPP_ROTATION_180) {
        dst->stride[0] = vdata->w;
        dst->stride[1] = vdata->w;
        dst->size.width = vdata->w;
        dst->size.height = vdata->h;
    } else {
        dst->stride[0] = vdata->h;
        dst->stride[1] = vdata->h;
        dst->size.width = vdata->h;
        dst->size.height = vdata->w;
    }
    blt.ctrl.flags = vdata->rotation;

    ret =  mpp_ge_bitblt(g_ge_dev, &blt);
    if (ret < 0) {
        pr_err("[ch%d] GE bitblt failed\n", vdata->channel);
        return -1;
    }

    ret = mpp_ge_emit(g_ge_dev);
    if (ret < 0) {
        pr_err("[ch%d] GE emit failed\n", vdata->channel);
        return -1;
    }

    ret = mpp_ge_sync(g_ge_dev);
    if (ret < 0) {
        pr_err("[ch%d] GE sync failed\n", vdata->channel);
        return -1;
    }
    return 0;
}
#endif

int dvp_set_output_pos(u32 ch, u32 x, u32 y, u32 width, u32 height)
{
    if (!width || !height || x > g_fb_info.width || y > g_fb_info.height) {
        pr_err("[ch%d] Invalid position: [%d, %d] %d x %d\n", ch, x, y, width, height);
        return -1;
    }

    g_vdata[ch].dst_pos.x = x;
    g_vdata[ch].dst_pos.y = y;
    g_vdata[ch].dst_pos.width = width;
    g_vdata[ch].dst_pos.height = height;
    return 0;
}

int video_layer_set(struct aic_dvp_data *vdata, int index)
{
#ifdef AIC_DISPLAY_DRV
    int i;
#if DE_SCALE_ENABLE
    u32 min_side = 0, max_side = 0;
#endif
    struct aicfb_layer_data layer = {0};
    struct vin_video_buf *binfo = &vdata->binfo;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    /* Dst image */
    layer.pos.x = vdata->dst_pos.x;
    layer.pos.y = vdata->dst_pos.y;
#if DE_SCALE_ENABLE
    min_side = min(vdata->dst_pos.width, vdata->dst_pos.height);
    max_side = max(vdata->dst_pos.width, vdata->dst_pos.height);
    if (max_side > (min_side * 2)) {
        /* When the aspect ratio is too high, stretch it to a square */
        layer.scale_size.width = min_side;
        layer.scale_size.height = min_side;
        if (vdata->dst_pos.width > vdata->dst_pos.height)
            layer.pos.x = min_side;
        else
            layer.pos.y = min_side;
    } else {
        layer.scale_size.width = vdata->dst_pos.width;
        layer.scale_size.height = vdata->dst_pos.height;
    }
#else
    layer.scale_size.width = vdata->dst_pos.width;
    layer.scale_size.height = vdata->dst_pos.height;
    /* Be center-aligned if screen size is bigger than DVP output */
    if (g_fb_info.width > vdata->dst_pos.width)
        layer.pos.x = (g_fb_info.width - vdata->dst_pos.width) / 2;
    if (g_fb_info.height > vdata->dst_pos.height)
        layer.pos.y = (g_fb_info.height - vdata->dst_pos.height) / 2;
#endif

#if DE_CROP_ENABLE
    layer.buf.crop_en = 1;
    layer.buf.crop.x = 0;
    layer.buf.crop.y = 0;
    layer.buf.crop.width = min(vdata->dst_fmt.width, g_fb_info.width);
    layer.buf.crop.height = min(vdata->dst_fmt.height, g_fb_info.height);
#endif

    /* Src image */
    if (vdata->rotation == MPP_ROTATION_0
        || vdata->rotation == MPP_ROTATION_180) {
        layer.buf.size.width = vdata->dst_fmt.width;
        if (aic_dvp_sfield_mode())
            layer.buf.size.height = vdata->dst_fmt.height / 2;
        else
            layer.buf.size.height = vdata->dst_fmt.height;
    } else {
        if (aic_dvp_sfield_mode())
            layer.buf.size.width = vdata->dst_fmt.height / 2;
        else
            layer.buf.size.width = vdata->dst_fmt.height;
        layer.buf.size.height = vdata->dst_fmt.width;
    }

    layer.buf.format = vdata->dst_fmt.pixelformat;
    layer.buf.buf_type = MPP_PHY_ADDR;

    for (i = 0; i < VID_BUF_PLANE_NUM; i++) {
        layer.buf.stride[i] = layer.buf.size.width;
        layer.buf.phy_addr[i] = binfo->planes[index * VID_BUF_PLANE_NUM + i].buf;
    }

    if (mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0) {
        pr_err("[ch%d] Failed to update layer config!\n", vdata->channel);
        return -1;
    }
#endif
    return 0;
}

static int dvp_pause(u32 ch)
{
    if (g_dvp_status[ch] == DVP_STATUS_PAUSE) {
        pr_info("[ch%d] DVP is already pausing\n", ch);
        return 0;
    }
    if (g_dvp_status[ch] != DVP_STATUS_PLAY) {
        pr_err("[ch%d] Invalid status: %d\n", ch, g_dvp_status[ch]);
        return -1;
    }

    if (mpp_dvp2_ioctl(DVP_STREAM_PAUSE, NULL, ch) < 0) {
        pr_err("[ch%d] Failed to pause stream!\n", ch);
        return -1;
    }
    g_dvp_status[ch] = DVP_STATUS_PAUSE;
    return 0;
}

static int dvp_resume(u32 ch)
{
    if (g_dvp_status[ch] == DVP_STATUS_PLAY) {
        pr_info("[ch%d] DVP is already playing\n", ch);
        return 0;
    }
    if (g_dvp_status[ch] != DVP_STATUS_PAUSE) {
        pr_err("[ch%d] Invalid status: %d\n", ch, g_dvp_status[ch]);
        return -1;
    }

    if (mpp_dvp2_ioctl(DVP_STREAM_RESUME, NULL, ch) < 0) {
        pr_err("[ch%d] Failed to play stream!\n", ch);
        return -1;
    }
    g_vdata[ch].fresh_frame = VID_DROP_FRESH_FRAME;
    g_dvp_status[ch] = DVP_STATUS_PLAY;
    return 0;
}

static int media_dev_init(void)
{
    if (!g_fb) {
        g_fb = mpp_fb_open();
        if (!g_fb) {
            pr_err("Failed to open FB\n");
            return -1;
        }
    }

#ifdef SUPPORT_ROTATION
    if (!g_ge_dev) {
        g_ge_dev = mpp_ge_open();
        if (!g_ge_dev) {
            pr_err("Failed to open GE\n");
        }
    }
#endif

    return 0;
}

static void media_dev_free(void)
{
    int i;
    int active_channels = 0;

    for (i = 0; i < MAX_DVP_CHANNELS; i++) {
        if (g_vdata[i].thread_running) {
            active_channels++;
        }
    }

    if (active_channels == 0) {
#ifdef SUPPORT_ROTATION
        if (g_ge_dev) {
            mpp_ge_close(g_ge_dev);
            g_ge_dev = NULL;
        }
#endif
        if (g_fb) {
            video_layer_disable();
            mpp_fb_close(g_fb);
            g_fb = NULL;
        }
    }
}

void dvp_thread_stop(u32 ch)
{
    g_dvp_status[ch] = DVP_STATUS_INIT;
    g_vdata[ch].thread_running = 0;
}

static void test_dvp_thread(void *arg)
{
    int i = 0, index = 0;
    struct timespec begin, now;
    u32 ch = (u32)(ptr_t)arg;
    struct aic_dvp_data *vdata = &g_vdata[ch];

    pr_info("Starting DVP thread for channel %d\n", ch);

    if (dvp_request_buf(&vdata->binfo, ch) < 0)
        goto exit;

    for (i = 0; i < vdata->num_buffers; i++) {
        if (dvp_queue_buf(i, ch) < 0)
            goto exit;
    }
    g_dvp_status[ch] = DVP_STATUS_READY;

    if (dvp_start(ch) < 0)
        goto exit;

#if DE_SCALE_ENABLE
    pr_info("[ch%d] DE scale is enable\n", ch);
    if (dvp_set_output_pos(ch, VID_SCALE_OFFSET, VID_SCALE_OFFSET,
                           g_fb_info.width - VID_SCALE_OFFSET * 2,
                           g_fb_info.height - VID_SCALE_OFFSET * 2))
        goto exit;

#elif DVP_CROP_ENABLE
    pr_info("[ch%d] DVP crop is enable\n", ch);
    if (dvp_set_output_pos(ch, 0, 0,
                           min(g_fb_info.width, vdata->src_fmt.width),
                           min(g_fb_info.height, vdata->src_fmt.height)))
        goto exit;

#else
    if (dvp_set_output_pos(ch, 0, 0, vdata->dst_fmt.width, vdata->dst_fmt.height))
        goto exit;

#endif

#if DE_CROP_ENABLE
    pr_info("[ch%d] DE crop is enable\n", ch);
#endif

    gettimespec(&begin);
    i = 0;
    g_dvp_status[ch] = DVP_STATUS_PLAY;
    vdata->fresh_frame = VID_DROP_FRESH_FRAME;
    vdata->thread_running = 1;

    while (vdata->thread_running &&
           (g_dvp_status[ch] == DVP_STATUS_PLAY || g_dvp_status[ch] == DVP_STATUS_PAUSE)) {
        if (g_dvp_status[ch] == DVP_STATUS_PAUSE) {
            aicos_msleep(100);
            continue;
        }

        if (vdata->frame_cnt != 0 && i >= vdata->frame_cnt) {
            break;
        }
        i++;

        if (dvp_dequeue_buf(&index, ch) < 0)
            break;

        if (vdata->fresh_frame) {
            dvp_queue_buf(index, ch);
            vdata->fresh_frame--;
            continue;
        }

        if (vdata->rotation) {
#ifdef SUPPORT_ROTATION
            if (do_rotate(vdata, index) < 0)
                break;

            if (video_layer_set(vdata, vdata->num_buffers) < 0)
                break;
#endif
        } else {
            if (video_layer_set(vdata, index) < 0)
                break;
        }
        dvp_queue_buf(index, ch);
        if (i && (i % 1000 == 0)) {
            char tmp[32] = "";

            snprintf(tmp, 32, "[DVP%d] %6d", ch, i);
            gettimespec(&now);
            show_fps(tmp, &begin, &now, 1000);
            gettimespec(&begin);
        }
    }

exit:
    dvp_stop(ch);
    dvp_release_buf(vdata->binfo.num_buffers, ch);

    mpp_vin_sel_ch(ch);
    mpp_vin_deinit();

    pr_info("[ch%d] Total receive %d frames, exit\n", ch, i);
    vdata->thread_running = 0;
    g_dvp_status[ch] = DVP_STATUS_INIT;

    media_dev_free();
}

static int dvp_play_ctrl(char *action, u32 ch)
{
    if (ch >= MAX_DVP_CHANNELS) {
        printf("Invalid channel: %d\n", ch);
        return -1;
    }

    if (strncasecmp(action, "r", 1) == 0)
        return dvp_resume(ch);

    if (strncasecmp(action, "p", 1) == 0)
        return dvp_pause(ch);

    if (strncasecmp(action, "s", 1) == 0) {
        dvp_thread_stop(ch);
        return 0;
    }

    printf("Invalid action: %s\n", action);
    return -1;
}

static int start_dvp_channel(u32 ch)
{
    pr_info("\n[ch%d] Starting DVP channel\n", ch);

    pr_info("[ch%d] Capture %d frames from camera\n", ch, g_vdata[ch].frame_cnt);
    pr_info("[ch%d] DVP out format: 0x%x\n", ch, g_vdata[ch].dst_fmt.pixelformat);

    mpp_vin_sel_ch(ch);
    if (mpp_vin_init(CAMERA_DEV_NAME)) {
        pr_err("Failed to init VIN for channel %d\n", ch);
        return -1;
    }

    if (sensor_get_fmt(ch) < 0)
        goto error_out;

    if (dvp_subdev_set_fmt(ch) < 0)
        goto error_out;

    if (dvp_cfg(ch) < 0)
        goto error_out;

    g_vdata[ch].thread = aicos_thread_create("test_dvp", 4096, 0, test_dvp_thread, (void *)(ptr_t)ch);
    if (g_vdata[ch].thread == NULL) {
        pr_err("Failed to create DVP thread for channel %d\n", ch);
        goto error_out;
    }

    return 0;

error_out:
    mpp_vin_deinit();
    return -1;
}

static int cmd_test_dvp(int argc, char **argv)
{
    int c;
    u32 channel = 0;
    bool use_all_ch = false;
    int i;

    int running_channels = 0;
    for (i = 0; i < MAX_DVP_CHANNELS; i++) {
        if (g_dvp_status[i] != DVP_STATUS_INIT) {
            running_channels++;
        }
    }

    if (running_channels > 0 && argc == 2) {
        if (running_channels == 1) {
            for (i = 0; i < MAX_DVP_CHANNELS; i++) {
                if (g_dvp_status[i] != DVP_STATUS_INIT) {
                    return dvp_play_ctrl(argv[1], i);
                }
            }
        } else {
            printf("Multiple channels running. Please specify channel: %s [pause/resume/stop] [channel]\n", argv[0]);
            return -1;
        }
    }

    for (i = 0; i < MAX_DVP_CHANNELS; i++) {
        memset(&g_vdata[i], 0, sizeof(struct aic_dvp_data));
        g_vdata[i].dst_fmt.pixelformat = MPP_FMT_NV16;
        g_vdata[i].frame_cnt = 1;
        g_vdata[i].channel = i;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'f':
            for (i = 0; i < MAX_DVP_CHANNELS; i++) {
                if (strncasecmp("nv12", optarg, strlen(optarg)) == 0)
                    g_vdata[i].dst_fmt.pixelformat = MPP_FMT_NV12;
                if (strncasecmp("yuv400", optarg, strlen(optarg)) == 0)
                    g_vdata[i].dst_fmt.pixelformat = MPP_FMT_YUV400;
            }
            break;

        case 'c':
            for (i = 0; i < MAX_DVP_CHANNELS; i++) {
                g_vdata[i].frame_cnt = str2int(optarg);
            }
            break;

#ifdef SUPPORT_ROTATION
        case 'a':
            for (i = 0; i < MAX_DVP_CHANNELS; i++) {
                g_vdata[i].rotation = (str2int(optarg) % 360) / 90;
            }
            break;
#endif

        case 't':
            channel = str2int(optarg);
            if (channel == 2) {
                use_all_ch = true;
            } else if (channel < MAX_DVP_CHANNELS) {
                g_vdata[channel].frame_cnt = g_vdata[0].frame_cnt;
            } else {
                printf("Invalid channel: %d\n", channel);
                return -1;
            }
            break;

        case 'h':
            usage(argv[0]);
            return 0;

        default:
            break;
        }
    }

    if (media_dev_init() < 0) {
        return -1;
    }

    if (get_fb_info() < 0)
        goto error_out;

    if (set_ui_layer_alpha(0) < 0)
        goto error_out;

    if (use_all_ch) {
        for (i = 0; i < MAX_DVP_CHANNELS; i++) {
            if (start_dvp_channel(i) < 0) {
                pr_err("Failed to start channel %d\n", i);
            }
        }
    } else {
        if (start_dvp_channel(channel) < 0) {
            goto error_out;
        }
    }

    return 0;

error_out:
    media_dev_free();
    return -1;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_dvp, test_dvp, Test DVP and camera);
