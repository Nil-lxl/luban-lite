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
#include <time.h>

#include <posix/string.h>

#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"

#include "drv_dvp.h"
#include "mpp_vin.h"

#ifdef AIC_DISPLAY_DRV
#include "mpp_fb.h"
#include "artinchip_fb.h"
#endif

/* MUST and ONLY enable one of the follow mode: */
#define DE_SCALE_ENABLE         1
#define DE_CROP_ENABLE          0
#define VIN_CROP_ENABLE         0

#if (DE_SCALE_ENABLE + DE_CROP_ENABLE + VIN_CROP_ENABLE) != 1
#error "MUST and ONLY enable one: DE_SCALE_ENABLE, DE_CROP_ENABLE, VIN_CROP_ENABLE"
#endif

#define VIN_MAX_CHANNELS        2
#define VID_BUF_NUM             3
#define VID_BUF_MIN_NUM         3
#define VID_SCALE_OFFSET        0

struct vin_test_ctx {
    struct vin_dev_ctx ctx;
    u32 channel;
    u32 show_channel;
    u32 frame_count;
    u32 ref_cnt;
    int rotation;
    struct mpp_rect dst_pos;
    bool running[VIN_MAX_CHANNELS];
    aicos_thread_t thread[VIN_MAX_CHANNELS];

    u32 num_buffers;
    struct vin_video_buf binfo[VIN_MAX_CHANNELS];
#ifdef AIC_DISPLAY_DRV
    struct aicfb_layer_data cached_layer;
#endif
};

static struct vin_test_ctx g_vin_test_ctx = {0};

static const char *g_vin_names[VIN_DEV_MAX] = {"DVP", "CSI"};
#define VIN_NAME(ctx)       g_vin_names[(ctx)->type]

#ifdef AIC_DISPLAY_DRV
static struct aicfb_screeninfo g_fb_info = {0};
static struct mpp_fb *g_fb = NULL;
#endif

static void usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\nOptions:\n");
    printf("  -d, --device <dev>     Video in device (dvp/csi), default: dvp\n");
    printf("  -C, --channel <n>      Channel number (0-%d), default: 0\n", VIN_MAX_CHANNELS - 1);
    printf("  -f, --format <fmt>     Pixel format (nv12/nv16/yuv400), default: nv16\n");
    printf("  -c, --count <n>        Number of frames to capture, 0=infinite, default: 10\n");
    printf("  -s, --stitch <mode>    Stitch mode (none/v/h), default: none\n");
    printf("  -S, --show             Show the video of given channel, only valid for none stitch mode\n");
    printf("  -h, --help             Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s -C 0 -c 10          # Capture 10 frames from channel 0\n", program);
    printf("  %s -C 1 -f nv12 -c 0   # Continuous capture from channel 1\n", program);
    printf("  %s -s v -c 10          # Vertical stitch mode\n", program);
    printf("  %s -s h -c 10          # Horizontal stitch mode\n", program);
}

static enum mpp_stitch_mode parse_stitch_mode(const char *arg)
{
    if (strncasecmp("v", arg, 1) == 0) {
        printf("Stitch mode: Vertical (CH0 top, CH1 bottom)\n");
        return MPP_STITCH_V_MODE;
    }
    if (strncasecmp("h", arg, 1) == 0) {
        printf("Stitch mode: Horizontal (CH0 left, CH1 right)\n");
        return MPP_STITCH_H_MODE;
    }
    if (strncasecmp("none", arg, 4) == 0) {
        printf("Stitch mode: None\n");
        return MPP_STITCH_NONE;
    }
    pr_err("Invalid stitch mode: %s (should use: none/v/h)\n", arg);
    return MPP_STITCH_INVALID;
}

static int parse_format(const char *arg)
{
    if (strncasecmp("nv12", arg, 4) == 0)
        return MPP_FMT_NV12;
    if (strncasecmp("nv16", arg, 4) == 0)
        return MPP_FMT_NV16;
    if (strncasecmp("yuv400", arg, 6) == 0)
        return MPP_FMT_YUV400;

    return -1;
}

static int get_fb_info(void)
{
    int ret = 0;

#ifdef AIC_DISPLAY_DRV
    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0)
        pr_err("Failed to get screen info! errno: -%d\n", -ret);
#endif
    return ret;
}

static int set_ui_layer_alpha(int val)
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

static int camera_get_fmt(struct vin_dev_ctx *vin_ctx)
{
    int ret = 0;
    struct mpp_video_fmt f = {0};

    ret = mpp_vin2_ioctl(VIN_IN_G_FMT, &f, 0, vin_ctx);
    if (ret < 0) {
        pr_err("Failed to get camera format! err -%d\n", -ret);
        return -1;
    }

    vin_ctx->src_fmt = f;
    vin_ctx->src_size.width = f.width;
    vin_ctx->src_size.height = f.height;

    if (f.bus_type == MEDIA_BUS_RAW8_MONO) {
        pr_info("Forbid the output format to YUV400\n");
        vin_ctx->dst_fmt.pixelformat = MPP_FMT_YUV400;
    }

    return 0;
}

static int vin_subdev_set_fmt(struct vin_dev_ctx *vin_ctx)
{
    int ret = 0;

    ret = mpp_vin2_ioctl(VIN_IN_S_FMT, &vin_ctx->src_fmt, 0, vin_ctx);
    if (ret < 0) {
        pr_err("Failed to set VIN in-format! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int vin_dev_cfg(struct vin_dev_ctx *vin_ctx)
{
    struct mpp_video_fmt *src = &vin_ctx->src_fmt;
    struct vin_video_fmt *dst = &vin_ctx->dst_fmt;
    int ret = 0;

#if VIN_CROP_ENABLE
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
#else
    dst->width = src->width;
    dst->height = src->height;
#endif

    if (dst->pixelformat == MPP_FMT_NV16)
        dst->framesize = dst->width * dst->height * 2;
    else if (dst->pixelformat == MPP_FMT_NV12)
        dst->framesize = (dst->width * dst->height * 3) >> 1;
    else if (dst->pixelformat == MPP_FMT_YUV400)
        dst->framesize = dst->width * dst->height;

    dst->frame_offset = 0;

    ret = mpp_vin2_ioctl(VIN_OUT_S_FMT, dst, 0, vin_ctx);
    if (ret < 0) {
        pr_err("Failed to set VIN out-format! err -%d\n", -ret);
        return -1;
    }
    return 0;
}

static int vin_request_buf(struct vin_video_buf *binfo, u32 ch, struct vin_dev_ctx *ctx)
{
    struct vin_video_buf *ch0_binfo = &g_vin_test_ctx.binfo[0];
    int i, min_num = VID_BUF_MIN_NUM;

    if (MPP_IS_STITCH(ctx->dst_fmt.stitch_mode) && (ch > 0)) {
        /* CH1 get the video buf from CH0 binfo */
        binfo->num_buffers = ch0_binfo->num_buffers;
        binfo->num_planes  = ch0_binfo->num_planes;
        memcpy(binfo->planes, &ch0_binfo->planes[binfo->num_planes * binfo->num_buffers],
               sizeof(struct vin_video_plane) * binfo->num_buffers * binfo->num_planes);
    }

    if (mpp_vin2_ioctl(VIN_REQ_BUF, binfo, ch, ctx) < 0) {
        pr_err("Failed to request buf for channel %d!\n", ch);
        return -1;
    }

    pr_info("[%s%d] Buf Plane[0]   size   Plane[1]   size\n", VIN_NAME(ctx), ch);
    for (i = 0; i < binfo->num_buffers; i++) {
        pr_info("       %3d 0x%08x %-6d 0x%08x %-6d\n", i,
            binfo->planes[i * binfo->num_planes].buf,
            binfo->planes[i * binfo->num_planes].len,
            binfo->planes[i * binfo->num_planes + 1].buf,
            binfo->planes[i * binfo->num_planes + 1].len);
    }

    if (binfo->num_buffers < min_num) {
        pr_err("[ch%d] The number of video buf must >= %d!\n", ch, min_num);
        return -1;
    }

    return 0;
}

static int vin_queue_buf(int index, u32 ch, struct vin_dev_ctx *ctx)
{
    if (mpp_vin2_ioctl(VIN_Q_BUF, (void *)(ptr_t)index, ch, ctx) < 0) {
        pr_err("[%s%d] Q failed! Maybe buf state is invalid.\n", VIN_NAME(ctx), ch);
        return -1;
    }

    return 0;
}

static int vin_dequeue_buf(int *index, u32 ch, struct vin_dev_ctx *ctx)
{
    int ret = 0;

    ret = mpp_vin2_ioctl(VIN_DQ_BUF, (void *)index, ch, ctx);
    if (ret < 0) {
        pr_err("[%s%d] DQ failed! Maybe cannot receive data from Camera. err -%d\n",
               VIN_NAME(ctx), ch, -ret);
        return -1;
    }

    return 0;
}

static int vin_start(u32 ch, struct vin_dev_ctx *ctx)
{
    int ret = 0;

    ret = mpp_vin2_ioctl(VIN_STREAM_ON, NULL, ch, ctx);
    if (ret < 0) {
        pr_err("[%s%d] Failed to start streaming! err -%d\n",
               VIN_NAME(ctx), ch, -ret);
        return -1;
    }

    return 0;
}

static int vin_stop(u32 ch, struct vin_dev_ctx *ctx)
{
    int ret = 0;

    ret = mpp_vin2_ioctl(VIN_STREAM_OFF, NULL, ch, ctx);
    if (ret < 0) {
        pr_err("[%s%d] Failed to stop streaming! err -%d\n",
               VIN_NAME(ctx), ch, -ret);
        return -1;
    }

    return 0;
}

static void vin_show_fmt(struct vin_dev_ctx *vin_ctx)
{
    struct mpp_video_fmt *src_fmt = &vin_ctx->src_fmt;
    struct vin_video_fmt *dst_fmt = &vin_ctx->dst_fmt;

    printf("\nThe stream format:\n");
    printf("\t[Camera] %s (0x%x - 0x%x)\n"
           "\t\t└─> [VIN] %s (0x%x)\n"
           "\t\t\t└─>[Panel] (0x%x)\n",
           vin_ctx->camera, src_fmt->code, src_fmt->bus_type,
           VIN_NAME(vin_ctx), dst_fmt->pixelformat, dst_fmt->pixelformat);
}

static void vin_show_size(struct vin_test_ctx *ctx)
{
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    struct mpp_video_fmt *src_fmt = &vin_ctx->src_fmt;
    struct vin_video_fmt *dst_fmt = &vin_ctx->dst_fmt;
#ifdef AIC_DISPLAY_DRV
    struct aicfb_layer_data *layer = &ctx->cached_layer;
#endif

    printf("The stream size:\n");
    printf("\t[Camera] %s %d x %d\n"
           "\t\t└─> [VIN] %s %d x %d (Crop: [%d, %d] %d x %d)\n",
           vin_ctx->camera, src_fmt->width, src_fmt->height,
           VIN_NAME(vin_ctx), dst_fmt->width, dst_fmt->height,
           dst_fmt->crop_x, dst_fmt->crop_y, dst_fmt->width, dst_fmt->height);

#ifdef AIC_DISPLAY_DRV
#if DE_SCALE_ENABLE
    printf("\t\t\t└─>[Panel] %d x %d (Pos: [%d, %d] Scale: %d x %d)\n\n",
           g_fb_info.width, g_fb_info.height,
           layer->pos.x, layer->pos.y,
           layer->scale_size.width, layer->scale_size.height);
#elif DE_CROP_ENABLE
    printf("\t\t\t└─>[Panel] %d x %d (Pos: [%d, %d] Crop: %d x %d)\n\n",
           g_fb_info.width, g_fb_info.height,
           layer->buf.crop.x, layer->buf.crop.y,
           layer->buf.crop.width, layer->buf.crop.height);
#else
    printf("\t\t\t└─>[Panel] %d x %d (Pos: [%d, %d] %d x %d)\n\n",
           g_fb_info.width, g_fb_info.height,
           layer->pos.x, layer->pos.y,
           layer->buf.size.width, layer->buf.size.height);
#endif
#endif


}

#ifdef AIC_DISPLAY_DRV

static int video_layer_disable(void)
{
    int ret = 0;
    struct aicfb_layer_data layer = {0};
    layer.enable = 0;
    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;

    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer);

    if (ret < 0)
        pr_err("Failed to disable video layer!\n");

    return ret;
}

static int vin_set_output_pos(struct vin_test_ctx *ctx,
                              u32 x, u32 y, u32 width, u32 height)
{
    if (!width || !height || x > g_fb_info.width || y > g_fb_info.height) {
        pr_err("[ch%d] Invalid position: [%d, %d] %d x %d\n", ctx->show_channel,
               x, y, width, height);
        return -1;
    }

    ctx->dst_pos.x = x;
    ctx->dst_pos.y = y;
    ctx->dst_pos.width = width;
    ctx->dst_pos.height = height;
    return 0;
}

static int vin_output_region_cfg(struct vin_test_ctx *ctx)
{
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;

#if DE_SCALE_ENABLE
    pr_info("[%s] DE scale is enable\n", VIN_NAME(vin_ctx));
    if (vin_set_output_pos(ctx, VID_SCALE_OFFSET, VID_SCALE_OFFSET,
                           g_fb_info.width - VID_SCALE_OFFSET * 2,
                           g_fb_info.height - VID_SCALE_OFFSET * 2))
        return -1;
#elif VIN_CROP_ENABLE
    pr_info("[%s] VIN crop is enable\n", VIN_NAME(vin_ctx));
    if (vin_set_output_pos(ctx, 0, 0,
                           min(g_fb_info.width, vin_ctx->dst_fmt.width),
                           min(g_fb_info.height, vin_ctx->dst_fmt.height)))
        return -1;
#else
    if (vin_set_output_pos(ctx, 0, 0, vin_ctx->dst_fmt.width, vin_ctx->dst_fmt.height))
        return -1;
#endif

#if DE_CROP_ENABLE
    pr_info("[%s] DE crop is enable\n", VIN_NAME(vin_ctx));
#endif
    return 0;
}

/*
 * Pre-compute static layer configuration (position, scale, crop, stride, etc.).
 * Call once before the streaming loop; the result is cached in ctx->cached_layer.
 */
static int video_layer_init(struct vin_test_ctx *ctx)
{
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    struct vin_video_fmt *dst_fmt = &vin_ctx->dst_fmt;
    struct aicfb_layer_data *layer = &ctx->cached_layer;

    memset(layer, 0, sizeof(*layer));
    layer->layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer->enable = 1;

    /* Dst image */
    layer->pos.x = ctx->dst_pos.x;
    layer->pos.y = ctx->dst_pos.y;

#if DE_SCALE_ENABLE
    /* Proportional scaling: fit source into dst_pos while preserving aspect ratio */
    float ratio_w = (float)ctx->dst_pos.width / dst_fmt->width;
    float ratio_h = (float)ctx->dst_pos.height / dst_fmt->height;
    float scale = (ratio_w < ratio_h) ? ratio_w : ratio_h;

    layer->scale_size.width = ALIGN_DOWN((u32)(dst_fmt->width * scale), 8);
    layer->scale_size.height = ALIGN_DOWN((u32)(dst_fmt->height * scale), 8);
    /* Center within dst_pos */
    layer->pos.x = ctx->dst_pos.x + (ctx->dst_pos.width - layer->scale_size.width) / 2;
    layer->pos.y = ctx->dst_pos.y + (ctx->dst_pos.height - layer->scale_size.height) / 2;
#else
    layer->scale_size.width = ctx->dst_pos.width;
    layer->scale_size.height = ctx->dst_pos.height;
    /* Be center-aligned if screen size is bigger than VIN output */
    if (g_fb_info.width > ctx->dst_pos.width)
        layer->pos.x = (g_fb_info.width - ctx->dst_pos.width) / 2;
    if (g_fb_info.height > ctx->dst_pos.height)
        layer->pos.y = (g_fb_info.height - ctx->dst_pos.height) / 2;
#endif

#if DE_CROP_ENABLE
    layer->buf.crop_en = 1;
    layer->buf.crop.x = 0;
    layer->buf.crop.y = 0;
    layer->buf.crop.width = min(dst_fmt->width, g_fb_info.width);
    layer->buf.crop.height = min(dst_fmt->height, g_fb_info.height);
#endif

    /* Src image */
    if (ctx->rotation == MPP_ROTATION_0 || ctx->rotation == MPP_ROTATION_180) {
        layer->buf.size.width = dst_fmt->width;
        if (aic_dvp_sfield_mode())
            layer->buf.size.height = dst_fmt->height / 2;
        else
            layer->buf.size.height = dst_fmt->height;

        if (dst_fmt->stitch_mode == MPP_STITCH_V_MODE) {
            layer->scale_size.width /= 2;
            layer->buf.size.height *= 2;
        } else if (dst_fmt->stitch_mode == MPP_STITCH_H_MODE) {
            layer->scale_size.height /= 2;
            layer->buf.size.width *= 2;
        }
    } else {
        if (aic_dvp_sfield_mode())
            layer->buf.size.width = dst_fmt->height / 2;
        else
            layer->buf.size.width = dst_fmt->height;
        layer->buf.size.height = dst_fmt->width;
    }

    layer->buf.format = dst_fmt->pixelformat;
    layer->buf.buf_type = MPP_PHY_ADDR;

    for (int i = 0; i < VIN_MAX_PLANE_NUM; i++) {
        if (dst_fmt->stitch_mode == MPP_STITCH_H_MODE)
            layer->buf.stride[i] = layer->buf.size.width * 2;
        else
            layer->buf.stride[i] = layer->buf.size.width;
    }

    return 0;
}

/*
 * Per-frame update: only refresh buffer physical addresses and push to FB.
 * Must be called after video_layer_init().
 */
static int video_layer_update_buf(struct vin_test_ctx *ctx, u32 ch, int index)
{
    struct vin_video_buf *binfo = &ctx->binfo[ch];
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    struct vin_video_fmt *dst_fmt = &vin_ctx->dst_fmt;
    struct aicfb_layer_data *layer = &ctx->cached_layer;

    for (int i = 0; i < VIN_MAX_PLANE_NUM; i++) {
        if (MPP_IS_STITCH(dst_fmt->stitch_mode) && aic_dvp_sfield_mode())
            layer->buf.phy_addr[i] = binfo->planes[index * binfo->num_planes + i].buf
                                    + binfo->planes[index * binfo->num_planes + i].len / 2;
        else
            layer->buf.phy_addr[i] = binfo->planes[index * binfo->num_planes + i].buf;
    }

    if (mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, layer) < 0) {
        pr_err("[ch%d] Failed to update layer config!\n", ch);
        return -1;
    }

    return 0;
}
#endif

static int media_dev_init(void)
{
#ifdef AIC_DISPLAY_DRV
    if (!g_fb) {
        g_fb = mpp_fb_open();
        if (!g_fb) {
            pr_err("Failed to open FB\n");
            return -1;
        }
    }
#endif

#ifdef SUPPORT_ROTATION
    if (!g_ge_dev) {
        g_ge_dev = mpp_ge_open();
        if (!g_ge_dev) {
            pr_err("Failed to open GE\n");
            return -1;
        }
    }
#endif

    return 0;
}

static void media_dev_deinit(void)
{
    int i;
    int active_channels = 0;

    for (i = 0; i < VIN_MAX_CHANNELS; i++) {
        if (g_vin_test_ctx.running[i]) {
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
#ifdef AIC_DISPLAY_DRV
        if (g_fb) {
            video_layer_disable();
            mpp_fb_close(g_fb);
            g_fb = NULL;
        }
#endif
    }
}

static int vin_dev_ch_init(struct vin_test_ctx *ctx, u32 ch)
{
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    int ret;

    ret = mpp_vin2_vb_init(ch, vin_ctx);
    if (ret < 0) {
        pr_err("[%s%d] Failed to initialize VB for channel %d\n",
               VIN_NAME(vin_ctx), ch);
        return -1;
    }

    ctx->binfo[ch].num_buffers = ctx->num_buffers;
    ret = vin_request_buf(&ctx->binfo[ch], ch, vin_ctx);
    if (ret < 0) {
        pr_err("[%s%d] Failed to request buffers\n", VIN_NAME(vin_ctx), ch);
        return -1;
    }

    ctx->ref_cnt++;
    return 0;
}

static int vin_dev_init(struct vin_test_ctx *ctx)
{
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    u32 i, ch = ctx->channel;
    int ret = 0;

    if (vin_ctx->state == VIN_STATE_INIT) {
        ret = mpp_vin2_init(vin_ctx);
        if (ret < 0) {
            pr_err("[%s%d] Failed to initialize VIN device\n",
                   VIN_NAME(vin_ctx), ch);
            return -1;
        }

        if (camera_get_fmt(vin_ctx) < 0)
            goto error;

        if (vin_subdev_set_fmt(vin_ctx) < 0)
            goto error;

        if (vin_dev_cfg(vin_ctx) < 0)
            goto error;
    }

    ctx->num_buffers = VID_BUF_NUM;
    if (vin_dev_ch_init(ctx, ch) < 0)
        goto error;

    if (vin_ctx->dst_fmt.stitch_mode != MPP_STITCH_INVALID) {
        for (i = 0; i < VIN_MAX_CHANNELS; i++) {
            if (i == ch)
                continue;
            if (vin_dev_ch_init(ctx, i) < 0)
                goto error;
        }
    }
    vin_ctx->state = VIN_STATE_READY;
    return 0;

error:
    mpp_vin2_deinit(vin_ctx);
    return -1;
}

static void vin_dev_deinit(u32 ch, struct vin_test_ctx *ctx)
{
    mpp_vin2_vb_deinit(ch, &ctx->ctx);

    if (ctx->ref_cnt > 0)
        ctx->ref_cnt--;
    if (ctx->ref_cnt == 0)
        mpp_vin2_deinit(&ctx->ctx);
}

static void vin_test_thread(void *arg)
{
    struct vin_test_ctx *ctx = &g_vin_test_ctx;
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    struct timespec begin, now;
    u32 buf_index, frame = 0;
    u32 ch = (u32)arg;
    int ret;

#ifdef RT_USING_PM
    rt_pm_module_request(PM_VIN_ID, PM_SLEEP_MODE_NONE);
#endif

    for (u32 i = 0; i < ctx->num_buffers; i++) {
        ret = vin_queue_buf(i, ch, vin_ctx);
        if (ret < 0) {
            pr_err("[%s%d] Failed to queue buffer %d\n",
                   VIN_NAME(vin_ctx), ch, i);
            goto exit;
        }
    }

    ret = vin_start(ch, vin_ctx);
    if (ret < 0) {
        pr_err("[%s%d] Failed to start streaming\n", VIN_NAME(vin_ctx), ch);
        goto exit;
    }

    if (ch == ctx->show_channel) {
        if (vin_output_region_cfg(ctx) < 0)
            goto exit;
    }

#ifdef AIC_DISPLAY_DRV
    if (ch == ctx->show_channel) {
        if (video_layer_init(ctx) < 0)
            goto exit;
    }
#endif

    vin_show_fmt(&ctx->ctx);
    vin_show_size(ctx);

    vin_ctx->state = VIN_STATE_STREAMING;
    ctx->running[ch] = true;

#ifdef AIC_DVP_NO_SIGNAL_PATTERN
    video_layer_update_buf(ctx, ch, ctx->num_buffers - 1);
#endif

    pr_info("[%s%d] Start streaming\n", VIN_NAME(vin_ctx), ch);
    gettimespec(&begin);
    while (ctx->running[ch] && (ctx->frame_count == 0 || frame < ctx->frame_count)) {
        if (vin_ctx->state == VIN_STATE_PAUSED) {
            aicos_msleep(100);
            continue;
        }

        ret = vin_dequeue_buf((int *)&buf_index, ch, vin_ctx);
        if (ret < 0) {
            pr_err("[%s%d] Failed to dequeue buffer\n", VIN_NAME(vin_ctx), ch);
            break;
        }

        frame++;
        pr_debug("[%s%d] Frame %d/%d, buffer %d\n", VIN_NAME(vin_ctx), ch,
                 frame, ctx->frame_count, buf_index);

#ifdef AIC_DISPLAY_DRV
        if ((ch == ctx->show_channel) && (video_layer_update_buf(ctx, ch, buf_index) < 0)) {
            pr_err("[%s%d] Failed to set video layer for buf %d\n",
                   VIN_NAME(vin_ctx), ch, buf_index);
            break;
        }
#endif

        ret = vin_queue_buf(buf_index, ch, vin_ctx);
        if (ret < 0) {
            pr_err("[%s%d] Failed to queue buf %d\n",
                   VIN_NAME(vin_ctx), ch, buf_index);
            break;
        }

        if (frame && (frame % 1000 == 0)) {
            char tmp[32] = "";

            snprintf(tmp, 32, "[%s%d] %6d", VIN_NAME(vin_ctx), ch, frame);
            gettimespec(&now);
            show_fps(tmp, &begin, &now, 1000);
            gettimespec(&begin);
        }
    }

exit:
    vin_ctx->state = VIN_STATE_INIT;
    ctx->running[ch] = false;

    vin_stop(ch, vin_ctx);
    vin_dev_deinit(ch, ctx);
    media_dev_deinit();

#ifdef RT_USING_PM
    rt_pm_module_release(PM_VIN_ID, PM_SLEEP_MODE_NONE);
#endif

    pr_info("[%s%d] Stopped streaming, captured %d frames\n",
            VIN_NAME(vin_ctx), ch, frame);
}

static void vin_test_stitch_thread(void *arg)
{
    struct vin_test_ctx *ctx = &g_vin_test_ctx;
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    struct timespec begin, now;
    u32 buf_index, tmp_index;
    u32 frame = 0;
    int ret, i;

#ifdef RT_USING_PM
    rt_pm_module_request(PM_VIN_ID, PM_SLEEP_MODE_NONE);
#endif

    for (i = 0; i < VIN_MAX_CHANNELS; i++) {
        for (u32 j = 0; j < ctx->num_buffers; j++) {
            ret = vin_queue_buf(j, i, vin_ctx);
            if (ret < 0) {
                pr_err("Failed to queue buffer %d for channel %d\n", j, i);
                goto exit;
            }
        }
    }

    for (i = VIN_MAX_CHANNELS - 1; i >= 0; i--) {
        ret = vin_start(i, vin_ctx);
        if (ret < 0) {
            pr_err("[%s%d] Failed to start streaming\n", VIN_NAME(vin_ctx), i);
            goto exit;
        }
    }

    if (vin_output_region_cfg(ctx) < 0)
        goto exit;

#ifdef AIC_DISPLAY_DRV
    if (video_layer_init(ctx) < 0)
        goto exit;
#endif

    vin_show_fmt(&ctx->ctx);
    vin_show_size(ctx);

#ifdef AIC_DVP_NO_SIGNAL_PATTERN
    video_layer_update_buf(ctx, 0, ctx->num_buffers - 1);
#endif

    vin_ctx->state = VIN_STATE_STREAMING;

    for (i = 0; i < VIN_MAX_CHANNELS; i++)
        ctx->running[i] = true;

    pr_info("[%s] Started streaming\n", VIN_NAME(vin_ctx));
    gettimespec(&begin);
    while (ctx->running[0] &&
           (ctx->frame_count == 0 || frame < ctx->frame_count)) {
        ret = vin_dequeue_buf((int *)&buf_index, 0, vin_ctx);
        if (ret < 0) {
            pr_err("[%s] Failed to dequeue buffer\n", VIN_NAME(vin_ctx));
            break;
        }

        /* Release the buf of the other channel */
        for (i = 1; i < VIN_MAX_CHANNELS; i++) {
            ret = vin_dequeue_buf((int *)&tmp_index, i, vin_ctx);
            if (ret < 0) {
                pr_err("[%s%d] Failed to dequeue buffer\n", VIN_NAME(vin_ctx), i);
                break;
            }

            ret = vin_queue_buf(tmp_index, i, vin_ctx);
            if (ret < 0) {
                pr_err("[%s%d] Failed to queue buf %d\n", VIN_NAME(vin_ctx), i, tmp_index);
                break;
            }
        }

        frame++;
        pr_debug("[%s] Frame %d/%d, buffer %d\n", VIN_NAME(vin_ctx),
                 frame, ctx->frame_count, buf_index);

#ifdef AIC_DISPLAY_DRV
        if (video_layer_update_buf(ctx, 0, buf_index) < 0) {
            pr_err("[%s] Failed to set video layer for buf %d\n",
                   VIN_NAME(vin_ctx), buf_index);
            break;
        }
#endif

        ret = vin_queue_buf(buf_index, 0, vin_ctx);
        if (ret < 0) {
            pr_err("[%s] Failed to queue buf %d\n", VIN_NAME(vin_ctx), buf_index);
            break;
        }

        if (frame && (frame % 1000 == 0)) {
            char tmp[32] = "";

            snprintf(tmp, 32, "[%s] %6d", VIN_NAME(vin_ctx), frame);
            gettimespec(&now);
            show_fps(tmp, &begin, &now, 1000);
            gettimespec(&begin);
        }
    }

exit:
    vin_ctx->state = VIN_STATE_INIT;
    for (i = VIN_MAX_CHANNELS - 1; i >= 0; i--) {
        ctx->running[i] = false;
        vin_stop(i, vin_ctx);
        vin_dev_deinit(i, ctx);
    }
    media_dev_deinit();

#ifdef RT_USING_PM
    rt_pm_module_release(PM_VIN_ID, PM_SLEEP_MODE_NONE);
#endif

    pr_info("[%s] Stopped streaming, captured %d frames\n",
            VIN_NAME(vin_ctx), frame);
}

static void vin_ctx_init(struct vin_test_ctx *ctx,
                         enum vin_dev_type type,
                         u32 ch, u32 frame_count,
                         enum mpp_stitch_mode stitch_mode, u32 dst_fmt)
{
    ctx->ctx.type = type;
    strncpy(ctx->ctx.camera, "camera", sizeof(ctx->ctx.camera) - 1);
    ctx->channel = ch;
    ctx->frame_count = frame_count;
    ctx->num_buffers = VID_BUF_NUM;
    ctx->ctx.dst_fmt.stitch_mode = stitch_mode;
    ctx->ctx.dst_fmt.pixelformat = dst_fmt;
}

static int vin_test_single_channel(struct vin_test_ctx *ctx, enum vin_dev_type type,
                                   u32 ch, u32 frame_count, u32 dst_fmt)
{
    vin_ctx_init(ctx, type, ch, frame_count, MPP_STITCH_INVALID, dst_fmt);
    if (vin_dev_init(ctx) < 0)
        return -1;

    ctx->thread[ch] = aicos_thread_create("t_vin", 4096, 0, vin_test_thread, (void *)ch);
    if (!ctx->thread[ch]) {
        pr_err("[%s%d] Failed to create thread\n", VIN_NAME(&ctx->ctx), ch);
        vin_dev_deinit(ch, ctx);
        media_dev_deinit();
        return -1;
    }

    return 0;
}

static int vin_test_multi_channel(struct vin_test_ctx *ctx, enum vin_dev_type type,
                                  u32 frame_count, u32 dst_fmt)
{
    char thread_name[16] = "";
    int i;

    /* Clear the channel NO. in stitch mode */
    vin_ctx_init(ctx, type, 0, frame_count, MPP_STITCH_NONE, dst_fmt);
    if (vin_dev_init(ctx) < 0)
        return -1;

    for (i = 0; i < VIN_MAX_CHANNELS; i++) {
        snprintf(thread_name, sizeof(thread_name), "t_vin%d", i);
        ctx->thread[i] = aicos_thread_create(thread_name, 4096, 0, vin_test_thread, (void *)i);
        if (!ctx->thread[i]) {
            pr_err("Failed to create thread for channel %d\n", i);
            vin_dev_deinit(i, ctx);
            media_dev_deinit();
            return -1;
        }
    }

    return 0;
}

static int vin_test_stitch_mode(struct vin_test_ctx *ctx, enum vin_dev_type type,
                                u32 frame_count, u32 dst_fmt,
                                enum mpp_stitch_mode stitch_mode)
{
    /* Clear the channel NO. in stitch mode */
    vin_ctx_init(ctx, type, 0, frame_count, stitch_mode, dst_fmt);
    if (vin_dev_init(ctx) < 0)
        return -1;

    ctx->thread[0] = aicos_thread_create("t_vin", 4096, 0, vin_test_stitch_thread, NULL);
    if (!ctx->thread[0]) {
        pr_err("Failed to create stitch thread\n");
        vin_dev_deinit(0, ctx);
        media_dev_deinit();
        return -1;
    }

    return 0;
}

static int vin_play_ctrl(char *action)
{
    struct vin_test_ctx *ctx = &g_vin_test_ctx;
    struct vin_dev_ctx *vin_ctx = &ctx->ctx;
    int i;

    if (strncasecmp(action, "r", 1) == 0) {
        if (vin_ctx->state == VIN_STATE_STREAMING) {
            pr_info("VIN is already playing\n");
            return 0;
        }
        if (vin_ctx->state != VIN_STATE_PAUSED) {
            pr_err("Invalid state: %d\n", vin_ctx->state);
            return -1;
        }
        for (i = 0; i < VIN_MAX_CHANNELS; i++) {
            if (ctx->running[i])
                mpp_vin2_ioctl(VIN_STREAM_RESUME, NULL, i, vin_ctx);
        }
        vin_ctx->state = VIN_STATE_STREAMING;
        pr_info("VIN resumed\n");
        return 0;
    }

    if (strncasecmp(action, "p", 1) == 0) {
        if (vin_ctx->state == VIN_STATE_PAUSED) {
            pr_info("VIN is already paused\n");
            return 0;
        }
        if (vin_ctx->state != VIN_STATE_STREAMING) {
            pr_err("Invalid state: %d\n", vin_ctx->state);
            return -1;
        }
        for (i = 0; i < VIN_MAX_CHANNELS; i++) {
            if (ctx->running[i])
                mpp_vin2_ioctl(VIN_STREAM_PAUSE, NULL, i, vin_ctx);
        }
        vin_ctx->state = VIN_STATE_PAUSED;
        pr_info("VIN paused\n");
        return 0;
    }

    printf("Invalid action: %s\n", action);
    return -1;
}

static int cmd_test_vin(int argc, char **argv)
{
    static const char sopts[] = "d:C:f:c:s:S:h";
    static const struct option lopts[] = {
        {"device",  required_argument, NULL, 'd'},
        {"channel", required_argument, NULL, 'C'},
        {"format",  required_argument, NULL, 'f'},
        {"count",   required_argument, NULL, 'c'},
        {"stitch",  required_argument, NULL, 's'},
        {"show",    required_argument, NULL, 'S'},
        {"help",    no_argument,       NULL, 'h'},
        {0, 0, 0, 0}
    };

    enum mpp_stitch_mode stitch_mode = MPP_STITCH_INVALID;
    enum vin_dev_type dev_type = VIN_DEV_DVP;
    u32 dst_fmt = MPP_FMT_NV16;
    u32 frame_cnt = 10, ch = 0;
    int ret, c;

    if (g_vin_test_ctx.ctx.state == VIN_STATE_STREAMING ||
        g_vin_test_ctx.ctx.state == VIN_STATE_PAUSED) {
        /* VIN is running, so just do play control */
        if (argc != 2) {
            printf("Usage:\n\t%s [pause/resume/stop]: \n\n", argv[0]);
            return -1;
        }
        return vin_play_ctrl(argv[1]);
    }

    optind = 0;
    memset(&g_vin_test_ctx, 0, sizeof(g_vin_test_ctx));

    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'd':
            if (strncasecmp("dvp", optarg, 3) == 0) {
                dev_type = VIN_DEV_DVP;
            } else if (strncasecmp("csi", optarg, 3) == 0) {
                dev_type = VIN_DEV_CSI;
            } else {
                pr_err("Invalid device: %s\n", optarg);
                return -1;
            }
            break;

        case 'c':
            frame_cnt = atoi(optarg);
            break;

        case 'C':
            ch = atoi(optarg);
            if (ch >= VIN_MAX_CHANNELS) {
                pr_err("Invalid channel: %d\n", ch);
                return -1;
            }
            g_vin_test_ctx.show_channel = ch;
            break;

        case 'f':
            ret = parse_format(optarg);
            if (ret < 0) {
                pr_err("Invalid format: %s\n", optarg);
                return -1;
            }
            dst_fmt = ret;
            break;

        case 's':
            stitch_mode = parse_stitch_mode(optarg);
            if (stitch_mode < 0) {
                pr_err("Invalid stitch mode: %s\n", optarg);
                return -1;
            }
#if !DE_SCALE_ENABLE
            if (stitch_mode != MPP_STITCH_NONE && stitch_mode != MPP_STITCH_INVALID) {
                pr_err("Stitch mode requires DE_SCALE_ENABLE\n");
                return -1;
            }
#endif
            break;

        case 'S':
            g_vin_test_ctx.show_channel = atoi(optarg);
            if (g_vin_test_ctx.show_channel >= VIN_MAX_CHANNELS) {
                pr_err("Invalid show channel: %d\n", g_vin_test_ctx.show_channel);
                return -1;
            }
            break;

        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }

    ret = media_dev_init();
    if (ret < 0)
        return -1;

    if (get_fb_info() < 0)
        goto error_out;

    if (set_ui_layer_alpha(0) < 0)
        goto error_out;

    if (stitch_mode == MPP_STITCH_INVALID)
        return vin_test_single_channel(&g_vin_test_ctx, dev_type, ch, frame_cnt, dst_fmt);
    else if (stitch_mode == MPP_STITCH_NONE)
        return vin_test_multi_channel(&g_vin_test_ctx, dev_type, frame_cnt, dst_fmt);
    else
        return vin_test_stitch_mode(&g_vin_test_ctx, dev_type, frame_cnt, dst_fmt, stitch_mode);

error_out:
    media_dev_deinit();
    return ret;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_vin, test_vin, Test Video In);
