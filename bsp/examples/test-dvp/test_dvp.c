/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
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

/* Global macro and variables */

#define VID_BUF_NUM             3
#define VID_BUF_PLANE_NUM       2
#define VID_SCALE_OFFSET        0

#define VID_DROP_FRESH_FRAME    1

enum dvp_state {
    DVP_STATUS_INIT,
    DVP_STATUS_READY,
    DVP_STATUS_PLAY,
    DVP_STATUS_PAUSE
};

static enum dvp_state g_dvp_status = DVP_STATUS_INIT;

static const char sopts[] = "f:c:a:h";
static const struct option lopts[] = {
    {"format",        required_argument, NULL, 'f'},
    {"capture",       required_argument, NULL, 'c'},
    {"angle",         required_argument, NULL, 'a'},
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
    int dst_fmt;  // output format
    struct mpp_rect dst_pos;

    struct mpp_video_fmt src_fmt;
    uint32_t num_buffers;
    struct vin_video_buf binfo;
};

static struct aic_dvp_data g_vdata = {0};
#ifdef AIC_DISPLAY_DRV
static struct mpp_fb *g_fb = NULL;
static struct aicfb_screeninfo g_fb_info = {0};
#endif
#ifdef SUPPORT_ROTATION
static struct mpp_ge *g_ge_dev = NULL;
#endif

/* Functions */

static void usage(char *program)
{
    printf("Usage: %s [options]: \n", program);
    printf("\t -f, --format\t\tformat of input video, NV16/NV12/YUV400 etc\n");
    printf("\t -c, --count\t\tthe number of capture frame.(0 means infinity) \n");
#ifdef SUPPORT_ROTATION
    printf("\t -a, --angle\t\tthe angle of rotation \n");
#endif
    printf("\t -h, --usage \n");
    printf("\n");
    printf("Example: %s -f nv16 -c 1\n", program);
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

int sensor_get_fmt(void)
{
    int ret = 0;
    struct mpp_video_fmt f = {0};

    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &f);
    if (ret < 0) {
        pr_err("Failed to get sensor format! err -%d\n", -ret);
        // return -1;
    }

    g_vdata.src_fmt = f;
    g_vdata.w = g_vdata.src_fmt.width;
    g_vdata.h = g_vdata.src_fmt.height;
    pr_info("Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x\n",
            f.width, f.height, f.code, f.bus_type, f.colorspace);

    if (f.bus_type == MEDIA_BUS_RAW8_MONO) {
        pr_info("Forbid the output format to YUV400\n");
        g_vdata.dst_fmt = MPP_FMT_YUV400;
    }

    return 0;
}

int dvp_subdev_set_fmt(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &g_vdata.src_fmt);
    if (ret < 0) {
        pr_err("Failed to set DVP in-format! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int dvp_cfg(int width, int height, int format)
{
    int ret = 0;
    struct dvp_out_fmt f = {0};

    f.width = g_vdata.src_fmt.width;
    f.height = g_vdata.src_fmt.height;
    f.pixelformat = format;
    f.num_planes = VID_BUF_PLANE_NUM;
    f.frame_offset = 0;

    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &f);
    if (ret < 0) {
        pr_err("Failed to set DVP out-format! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

int dvp_request_buf(struct vin_video_buf *vbuf)
{
    int i, min_num = VID_BUF_NUM;

    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)vbuf) < 0) {
        pr_err("Failed to request buf!\n");
        return -1;
    }

    pr_info("Buf   Plane[0]     size   Plane[1]     size\n");
    for (i = 0; i < vbuf->num_buffers; i++) {
        pr_info("%3d 0x%x %8d 0x%x %8d\n", i,
            vbuf->planes[i * vbuf->num_planes].buf,
            vbuf->planes[i * vbuf->num_planes].len,
            vbuf->planes[i * vbuf->num_planes + 1].buf,
            vbuf->planes[i * vbuf->num_planes + 1].len);
    }

#ifdef SUPPORT_ROTATION
    if (g_vdata.rotation)
        min_num++;

    g_vdata.num_buffers = g_vdata.binfo.num_buffers - 1;
#else
    g_vdata.num_buffers = g_vdata.binfo.num_buffers;
#endif

    if (vbuf->num_buffers < min_num) {
        pr_err("The number of video buf must >= %d!\n", min_num);
        return -1;
    }

    return 0;
}

void dvp_release_buf(int num)
{
#if 0
    int i;
    struct video_buf_info *binfo = NULL;

    for (i = 0; i < num; i++) {
        binfo = &g_vdata.binfo[i];
        if (binfo->vaddr) {
            munmap(binfo->vaddr, binfo->len);
            binfo->vaddr = NULL;
        }
    }
#endif
}

int dvp_queue_buf(int index)
{
    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        pr_err("Q failed! Maybe buf state is invalid.\n");
        return -1;
    }

    return 0;
}

int dvp_dequeue_buf(int *index)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        pr_err("DQ failed! Maybe cannot receive data from Camera. err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int dvp_start(void)
{
    int ret = 0;

    if (g_dvp_status != DVP_STATUS_READY) {
        pr_err("Invalid status: %d\n", g_dvp_status);
        return -1;
    }

    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        pr_err("Failed to start streaming! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int dvp_stop(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        pr_err("Failed to stop streaming! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

/* Enable one of follow: SCALE or CROP */
#define DVP_SCALE       1
#define DVP_CROP        0

int video_layer_disable(void)
{
    int ret = 0;
#ifdef AIC_DISPLAY_DRV
    struct aicfb_layer_data layer = {0};
    layer.enable = 0;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0)
        pr_err("Failed to disable video layer!");

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

    src->format = vdata->dst_fmt;
    src->buf_type = MPP_PHY_ADDR;
    src->phy_addr[0] = binfo->planes[index * VID_BUF_PLANE_NUM].buf;
    src->phy_addr[1] = binfo->planes[index * VID_BUF_PLANE_NUM + 1].buf;
    src->stride[0] = vdata->w;
    src->stride[1] = vdata->w;
    src->size.width = vdata->w;
    src->size.height = vdata->h;

    dst->format = vdata->dst_fmt;
    dst->buf_type = MPP_PHY_ADDR;
    dst->phy_addr[0] = binfo->planes[g_vdata.num_buffers * VID_BUF_PLANE_NUM].buf;
    dst->phy_addr[1] = binfo->planes[g_vdata.num_buffers * VID_BUF_PLANE_NUM + 1].buf;
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
#if 0
    printf("GE: %d(%d) * %d -> %d * %d, canvas %d(%d) * %d\n",
           src->size.width, src->stride[0],
           src->size.height,
           dst->crop.width, dst->crop.height,
           dst->size.width, dst->stride[0],
           dst->size.height);
#endif
    ret =  mpp_ge_bitblt(g_ge_dev, &blt);
    if (ret < 0) {
        pr_err("GE bitblt failed\n");
        return -1;
    }

    ret = mpp_ge_emit(g_ge_dev);
    if (ret < 0) {
        pr_err("GE emit failed\n");
        return -1;
    }

    ret = mpp_ge_sync(g_ge_dev);
    if (ret < 0) {
        pr_err("GE sync failed\n");
        return -1;
    }
    return 0;
}
#endif

int dvp_set_output_pos(u32 x, u32 y, u32 width, u32 height)
{
    if (!width || !height || x > g_fb_info.width || y > g_fb_info.height) {
        pr_err("Invalid position: [%d, %d] %d x %d\n", x, y, width, height);
        return -1;
    }

    g_vdata.dst_pos.x = x;
    g_vdata.dst_pos.y = y;
    g_vdata.dst_pos.width = width;
    g_vdata.dst_pos.height = height;
    return 0;
}

int video_layer_set(struct aic_dvp_data *vdata, int index)
{
#ifdef AIC_DISPLAY_DRV
    int i;
#if DVP_SCALE
    u32 min_side = 0, max_side = 0;
#endif
    struct aicfb_layer_data layer = {0};
    struct vin_video_buf *binfo = &vdata->binfo;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    /* Dst image */
    layer.pos.x = vdata->dst_pos.x;
    layer.pos.y = vdata->dst_pos.y;
#if DVP_SCALE
    min_side = min(vdata->dst_pos.width, vdata->dst_pos.height);
    max_side = max(vdata->dst_pos.width, vdata->dst_pos.height);
    if (max_side > (min_side * 2)) {
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
    layer.scale_size.width = vdata->w;
    layer.scale_size.height = vdata->h;
    if (g_fb_info.width > vdata->w)
        layer.pos.x = (g_fb_info.width - vdata->w) / 2;
    if (g_fb_info.height > vdata->h)
        layer.pos.y = (g_fb_info.height - vdata->h) / 2;
#endif

#if DVP_CROP
    layer.buf.crop_en = 1;
    layer.buf.crop.x = 0;
    layer.buf.crop.y = 0;
    layer.buf.crop.width = min(vdata->w, (int)g_fb_info.width);
    layer.buf.crop.height = min(vdata->h, (int)g_fb_info.height);
#endif

    /* Src image */
    if (vdata->rotation == MPP_ROTATION_0
        || vdata->rotation == MPP_ROTATION_180) {
        layer.buf.size.width = vdata->w;
        if (aic_dvp_sfield_mode())
            layer.buf.size.height = vdata->h / 2;
        else
            layer.buf.size.height = vdata->h;
    } else {
        if (aic_dvp_sfield_mode())
            layer.buf.size.width = vdata->h / 2;
        else
            layer.buf.size.width = vdata->h;
        layer.buf.size.height = vdata->w;
    }

    layer.buf.format = vdata->dst_fmt;
    layer.buf.buf_type = MPP_PHY_ADDR;

    for (i = 0; i < VID_BUF_PLANE_NUM; i++) {
        layer.buf.stride[i] = layer.buf.size.width;
        layer.buf.phy_addr[i] = binfo->planes[index * VID_BUF_PLANE_NUM + i].buf;
    }

    if (mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0) {
        pr_err("Failed to update layer config!\n");
        return -1;
    }
#endif
    return 0;
}

static int dvp_pause(void)
{
    if (g_dvp_status == DVP_STATUS_PAUSE) {
        pr_info("DVP is already pausing\n");
        return 0;
    }
    if (g_dvp_status != DVP_STATUS_PLAY) {
        pr_err("Invalid status: %d\n", g_dvp_status);
        return -1;
    }

    if (mpp_dvp_ioctl(DVP_STREAM_PAUSE, NULL) < 0) {
        pr_err("Failed to pause stream!\n");
        return -1;
    }
    g_dvp_status = DVP_STATUS_PAUSE;
    return 0;
}

static int dvp_resume(void)
{
    if (g_dvp_status == DVP_STATUS_PLAY) {
        pr_info("DVP is already playing\n");
        return 0;
    }
    if (g_dvp_status != DVP_STATUS_PAUSE) {
        pr_err("Invalid status: %d\n", g_dvp_status);
        return -1;
    }

    if (mpp_dvp_ioctl(DVP_STREAM_RESUME, NULL) < 0) {
        pr_err("Failed to play stream!\n");
        return -1;
    }
    g_vdata.fresh_frame = VID_DROP_FRESH_FRAME;
    g_dvp_status = DVP_STATUS_PLAY;
    return 0;
}

void dvp_thread_stop(void)
{
    g_dvp_status = DVP_STATUS_INIT;
}

static void test_dvp_thread(void *arg)
{
    int i, index = 0;
    struct timespec begin, now;

    if (dvp_request_buf(&g_vdata.binfo) < 0)
        return;

    for (i = 0; i < g_vdata.num_buffers; i++) {
        if (dvp_queue_buf(i) < 0)
            return;
    }
    g_dvp_status = DVP_STATUS_READY;

    if (dvp_start() < 0)
        return;

#if DVP_SCALE
    pr_info("DVP scale is enable\n");
    if (dvp_set_output_pos(VID_SCALE_OFFSET, VID_SCALE_OFFSET,
                           g_fb_info.width - VID_SCALE_OFFSET * 2,
                           g_fb_info.height - VID_SCALE_OFFSET * 2))
        return;
#else
    pr_info("DVP scale is disable\n");
#endif

    gettimespec(&begin);
    i = 0;
    g_dvp_status = DVP_STATUS_PLAY;
    g_vdata.fresh_frame = VID_DROP_FRESH_FRAME;
    while (g_dvp_status == DVP_STATUS_PLAY || g_dvp_status == DVP_STATUS_PAUSE) {
        if (g_dvp_status == DVP_STATUS_PAUSE) {
            aicos_msleep(100);
            continue;
        }

        if (g_vdata.frame_cnt != 0 && i >= g_vdata.frame_cnt) {
            break;
        }
        i++;

        if (dvp_dequeue_buf(&index) < 0)
            break;

        if (g_vdata.fresh_frame) {
            dvp_queue_buf(index);
            g_vdata.fresh_frame--;
            continue;
        }

        // pr_debug("Set the buf %d to video layer\n", index);
        if (g_vdata.rotation) {
#ifdef SUPPORT_ROTATION
            if (do_rotate(&g_vdata, index) < 0)
                break;

            if (video_layer_set(&g_vdata, g_vdata.num_buffers) < 0)
                break;
#endif
        } else {
            if (video_layer_set(&g_vdata, index) < 0)
                break;
        }

        dvp_queue_buf(index);

        if (i && (i % 1000 == 0)) {
            char tmp[32] = "";

            snprintf(tmp, 32, "[DVP] %6d", i);
            gettimespec(&now);
            show_fps(tmp, &begin, &now, 1000);
            gettimespec(&begin);
        }
    }

    dvp_stop();
    dvp_release_buf(g_vdata.binfo.num_buffers);
    mpp_vin_deinit();
    if (g_fb) {
        video_layer_disable();
        mpp_fb_close(g_fb);
        g_fb = NULL;
    }

#ifdef SUPPORT_ROTATION
    if (g_ge_dev) {
        mpp_ge_close(g_ge_dev);
        g_ge_dev = NULL;
    }
#endif

    g_dvp_status = DVP_STATUS_INIT;
    pr_info("Total receive %d frames, exit\n", i);
}

static int dvp_play_ctrl(char *action)
{
    if (strncasecmp(action, "r", 1) == 0)
        return dvp_resume();

    if (strncasecmp(action, "p", 1) == 0)
        return dvp_pause();

    if (strncasecmp(action, "s", 1) == 0) {
        dvp_thread_stop();
        return 0;
    }

    printf("Invalid action: %s\n", action);
    return -1;
}

static int cmd_test_dvp(int argc, char **argv)
{
    int c;
    aicos_thread_t thid = NULL;

    if (g_dvp_status != DVP_STATUS_INIT) {
        /* DVP is running, so just do play control */
        if (argc != 2) {
            printf("Usage:\n\t%s [pause/resume/stop]: \n\n", argv[0]);
            return -1;
        }
        return dvp_play_ctrl(argv[1]);
    }

    memset(&g_vdata, 0, sizeof(struct aic_dvp_data));
    g_vdata.dst_fmt = MPP_FMT_NV16;
    g_vdata.frame_cnt = 1;
    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'f':
            if (strncasecmp("nv12", optarg, strlen(optarg)) == 0)
                g_vdata.dst_fmt = MPP_FMT_NV12;
            if (strncasecmp("yuv400", optarg, strlen(optarg)) == 0)
                g_vdata.dst_fmt = MPP_FMT_YUV400;
            break;

        case 'c':
            g_vdata.frame_cnt = str2int(optarg);
            break;

#ifdef SUPPORT_ROTATION
        case 'a':
            g_vdata.rotation = (str2int(optarg) % 360) / 90;
            break;
#endif

        case 'h':
            usage(argv[0]);
            return 0;

        default:
            break;
        }
    }

    pr_info("Capture %d frames from camera\n", g_vdata.frame_cnt);
    pr_info("DVP out format: 0x%x\n", g_vdata.dst_fmt);

    if (mpp_vin_init(CAMERA_DEV_NAME))
        return -1;

    if (sensor_get_fmt() < 0)
        goto error_out;

    if (dvp_subdev_set_fmt() < 0)
        goto error_out;

    if (g_vdata.dst_fmt == MPP_FMT_NV16)
        g_vdata.frame_size = g_vdata.w * g_vdata.h * 2;
    else if (g_vdata.dst_fmt == MPP_FMT_NV12)
        g_vdata.frame_size = (g_vdata.w * g_vdata.h * 3) >> 1;
    else if (g_vdata.dst_fmt == MPP_FMT_YUV400)
        g_vdata.frame_size = g_vdata.w * g_vdata.h;

    g_fb = mpp_fb_open();
    if (!g_fb) {
        pr_err("Failed to open FB\n");
        goto error_out;
    }

#ifdef SUPPORT_ROTATION
    if (g_vdata.rotation) {
        g_ge_dev = mpp_ge_open();
        if (!g_ge_dev)
            goto error_out;
        printf("Rotate %d by GE\n", g_vdata.rotation * 90);
    }
#endif

    if (get_fb_info() < 0)
        goto error_out;

    if (set_ui_layer_alpha(0) < 0)
        goto error_out;

    if (dvp_cfg(g_vdata.w, g_vdata.h, g_vdata.dst_fmt) < 0)
        goto error_out;

    thid = aicos_thread_create("test_dvp", 4096, 0, test_dvp_thread, NULL);
    if (thid == NULL) {
        pr_err("Failed to create DVP thread\n");
        return -1;
    }
    return 0;

error_out:
    mpp_vin_deinit();
#ifdef SUPPORT_ROTATION
    if (g_ge_dev) {
        mpp_ge_close(g_ge_dev);
        g_ge_dev = NULL;
    }
#endif
    if (g_fb) {
        mpp_fb_close(g_fb);
        g_fb = NULL;
    }
    return -1;
}
MSH_CMD_EXPORT_ALIAS(cmd_test_dvp, test_dvp, Test DVP and camera);
