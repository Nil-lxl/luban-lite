/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include "aic_common.h"
#include "aic_errno.h"
#include "aic_log.h"

#ifdef AIC_USING_DVP
#include "drv_dvp.h"
#endif
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif
#include "mpp_vin.h"

#define VIN_BUF_INIT_VAL            0x7C

#ifdef AIC_DVP_SUPPORT_DEMUX
#define DVP_PARAM_CH                ch
#define DVP_PARAMS_CH               ,ch
#else
#define DVP_PARAM_CH
#define DVP_PARAMS_CH
#endif

int mpp_vin2_init(struct vin_dev_ctx *ctx)
{
    rt_device_t camera_dev = NULL;

    if (!ctx || !ctx->camera) {
        pr_err("Must given the name of camera\n");
        return -1;
    }

    if (ctx->state != VIN_STATE_INIT) {
        pr_info("VIN device is already initialized\n");
        return 0;
    }

    if (!ctx->camera_dev) {
        camera_dev = rt_device_find(ctx->camera);
        if (!camera_dev) {
            pr_err("Failed to find %s\n", ctx->camera);
            return -1;
        }
        if (rt_device_open(camera_dev, RT_DEVICE_FLAG_RDONLY) < 0) {
            pr_err("Failed to open %s\n", ctx->camera);
            return -1;
        }
        ctx->camera_dev = camera_dev;
    }

#ifdef AIC_USING_DVP
    if (ctx->type == VIN_DEV_DVP) {
        if (aic_dvp_probe())
            goto error;
        if (aic_dvp_open())
            goto error;
    }
#endif

    ctx->state = VIN_STATE_READY;
    return 0;

error:
    rt_device_close(ctx->camera_dev);
    ctx->camera_dev = NULL;
    return -1;
}

int mpp_vin2_reinit(u32 ch, struct vin_dev_ctx *ctx)
{
    if (!ctx->camera || !ctx->vin_buf) {
        pr_err("Must init MPP vin first!\n");
        return -1;
    }

#ifdef AIC_USING_DVP
    if (ctx->type == VIN_DEV_DVP) {
        if (aic_dvp_vb_init(DVP_PARAM_CH))
            return -1;
    }
#endif

    return 0;
}

int mpp_vin2_vb_init(u32 ch, struct vin_dev_ctx *ctx)
{
    if (!ctx) {
        pr_err("Invalid context\n");
        return -1;
    }

    if (ctx->state == VIN_STATE_INIT) {
        pr_err("Must call mpp_vin2_init() first!\n");
        return -1;
    }

#ifdef AIC_USING_DVP
    if (ctx->type == VIN_DEV_DVP) {
        if (aic_dvp_vb_init(DVP_PARAM_CH))
            return -1;
    }
#endif

    return 0;
}

void mpp_vin2_vb_deinit(u32 ch, struct vin_dev_ctx *ctx)
{
    if (!ctx)
        return;

#ifdef AIC_USING_DVP
    if (ctx->type == VIN_DEV_DVP) {
        aic_dvp_vb_deinit(DVP_PARAM_CH);
    }
#endif
}

void mpp_vin2_deinit(struct vin_dev_ctx *ctx)
{
    rt_device_t camera_dev = NULL;

    if (!ctx)
        return;

    if (ctx->camera_dev) {
        camera_dev = (rt_device_t)ctx->camera_dev;
        rt_device_close(camera_dev);
        ctx->camera_dev = NULL;
    }

    if (ctx->vin_buf) {
        aicos_free(MEM_CMA, ctx->vin_buf);
        ctx->vin_buf = NULL;
    }
#ifdef AIC_USING_DVP
    if (ctx->type == VIN_DEV_DVP)
        aic_dvp_close();
#endif

    ctx->state = VIN_STATE_INIT;
}

static int mpp_vin2_req_buf(struct vin_dev_ctx *ctx, struct vin_video_buf *binfo, u32 ch)
{
    u32 buf_size = ctx->dst_fmt.framesize * binfo->num_buffers + CACHE_LINE_SIZE;
    u32 align_offset = 0;
    char *tmp = NULL;

    if (ctx->dst_fmt.stitch_mode != MPP_STITCH_INVALID)
        buf_size *= 2;

    if (!ctx->vin_buf) {
        ctx->vin_buf = (char *)aicos_malloc(MEM_CMA, buf_size);
        if (!ctx->vin_buf) {
            pr_err("Failed to malloc %d buffer\n", buf_size);
            return -1;
        }
#ifdef VIN_BUF_INIT_VAL
        memset(ctx->vin_buf, VIN_BUF_INIT_VAL, buf_size);
#endif
        pr_debug("MPP VIN buffer: 0x%lx, size %d\n", (ptr_t)ctx->vin_buf, buf_size);
    }

    align_offset = CACHE_LINE_SIZE - (ptr_t)ctx->vin_buf % CACHE_LINE_SIZE;
    tmp = ctx->vin_buf + align_offset;

    if (ctx->dst_fmt.stitch_mode == MPP_STITCH_NONE) {
        buf_size /= 2;
        if (ch > 0)
            tmp += buf_size;
    }

    return aic_dvp_req_buf(tmp, buf_size - align_offset, binfo DVP_PARAMS_CH);
}

int mpp_vin2_ioctl(int cmd, void *arg, u32 ch, struct vin_dev_ctx *ctx)
{
    rt_device_t camera_dev = NULL;

    if (!ctx) {
        pr_err("Invalid context\n");
        return -EINVAL;
    }

    if (ctx->type != VIN_DEV_DVP) {
        pr_err("Unsupported VIN device type: %d\n", ctx->type);
        return -EINVAL;
    }

    camera_dev = (rt_device_t)ctx->camera_dev;

    switch (cmd) {
    case DVP_IN_S_FMT:
        return aic_dvp_set_in_fmt((struct mpp_video_fmt *)arg);

    case DVP_OUT_S_FMT:
        return aic_dvp_set_out_fmt((struct dvp_out_fmt *)arg);

#ifdef AIC_USING_CAMERA
    case DVP_IN_G_FMT:
        if (camera_dev)
            return rt_device_control(camera_dev, CAMERA_CMD_GET_FMT, arg);

        pr_err("Must init camera first!\n");
        return -ENODEV;

    case DVP_STREAM_ON:
        camera_start(camera_dev);
        if (aic_dvp_stream_on(DVP_PARAM_CH))
            return -1;
        return 0;

    case DVP_STREAM_OFF:
        camera_stop(camera_dev);
        if (aic_dvp_stream_off(DVP_PARAM_CH))
            return -1;
        return 0;

    case DVP_STREAM_PAUSE:
        camera_pause(camera_dev);
        aicos_msleep(50);
        aic_dvp_stream_pause(DVP_PARAM_CH);
        return 0;

    case DVP_STREAM_RESUME:
        camera_resume(camera_dev);
        aic_dvp_stream_resume(DVP_PARAM_CH);
        return 0;
#endif
#ifdef AIC_DVP_DRV
    case DVP_REQ_BUF:
        return mpp_vin2_req_buf(ctx, (struct vin_video_buf *)arg, ch);

    case DVP_Q_BUF:
        return aic_dvp_q_buf((u32)(ptr_t)arg DVP_PARAMS_CH);

    case DVP_DQ_BUF:
        return aic_dvp_dq_buf((u32 *)arg DVP_PARAMS_CH);

    case DVP_GET_TIMESTAMP:
        return aic_dvp_get_timestamp((u32)(ptr_t)arg DVP_PARAMS_CH);

#endif
    default:
        pr_err("Unsupported ioctl command: 0x%x\n", cmd);
        return -EINVAL;
    }
    return 0;
}

static struct vin_dev_ctx g_vin1_ctx = {
    .type = VIN_DEV_DVP,
};

int mpp_vin_init(char *camera)
{
    strncpy(g_vin1_ctx.camera, camera, sizeof(g_vin1_ctx.camera));
    g_vin1_ctx.camera[sizeof(g_vin1_ctx.camera) - 1] = '\0';

    if (mpp_vin2_init(&g_vin1_ctx))
        return -1;

    return mpp_vin2_vb_init(0, &g_vin1_ctx);
}

void mpp_vin_deinit(void)
{
    mpp_vin2_vb_deinit(0, &g_vin1_ctx);
    mpp_vin2_deinit(&g_vin1_ctx);
}

int mpp_vin_reinit(void)
{
    return mpp_vin2_reinit(0, &g_vin1_ctx);
}

int mpp_dvp_ioctl(int cmd, void *arg)
{
    int ret = mpp_vin2_ioctl(cmd, arg, 0, &g_vin1_ctx);

    if (ret)
        return ret;

    switch (cmd) {
        case DVP_OUT_S_FMT:
            memcpy(&g_vin1_ctx.dst_fmt, arg, sizeof(struct dvp_out_fmt));
            g_vin1_ctx.dst_fmt.stitch_mode = MPP_STITCH_INVALID;
            break;
        case DVP_IN_S_FMT:
            memcpy(&g_vin1_ctx.src_fmt, arg, sizeof(struct mpp_video_fmt));
            break;
        default:
            break;
    }
    return ret;
}
