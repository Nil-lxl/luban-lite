/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: aic video render interface
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "aic_core.h"
#include "aic_render.h"
#include "artinchip_fb.h"
#include "drv_dma.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_types.h"

#define DEV_FB "aicfb"

struct aic_fb_video_render {
    struct aic_video_render base;
    struct aicfb_layer_data layer;
    s32 fd;
    rt_device_t render_dev;
};

struct aic_fb_video_render_last_frame {
    struct aicfb_layer_data layer;
    struct mpp_frame frame;
    s32 enable;
};

static struct aic_fb_video_render_last_frame g_last_frame = {0};

static s32 fb_video_render_init(struct aic_video_render *render, s32 layer, s32 dev_id)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;
    fb_render->render_dev = rt_device_find(DEV_FB);

    if (fb_render->render_dev == NULL) {
        loge("rt_device_find aicfb failed!");
        return -1;
    }

    if (strcmp(PRJ_CHIP, "d12x") == 0) {
        fb_render->layer.layer_id = AICFB_LAYER_TYPE_UI;
        rt_device_control(fb_render->render_dev, AICFB_GET_LAYER_CONFIG, &fb_render->layer);
    } else {
        fb_render->layer.layer_id = layer;
        rt_device_control(fb_render->render_dev,AICFB_GET_LAYER_CONFIG,&fb_render->layer);
        if (!g_last_frame.enable) {
            fb_render->layer.enable = 0;
            rt_device_control(fb_render->render_dev, AICFB_UPDATE_LAYER_CONFIG, &fb_render->layer);
        }
    }

    return 0;
}

static s32 fb_video_render_destroy(struct aic_video_render *render)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;

    if (strcmp(PRJ_CHIP, "d12x") == 0) {
        mpp_free(fb_render);
        return 0;
    }
    if (!g_last_frame.enable) {
        fb_render->layer.enable = 0;
        if (rt_device_control(fb_render->render_dev, AICFB_UPDATE_LAYER_CONFIG,
                              &fb_render->layer) < 0) {
            loge("fb ioctl() AICFB_UPDATE_LAYER_CONFIG failed!");
        }
    }
    mpp_free(fb_render);
    return 0;
}

static s32 fb_video_render_rend(struct aic_video_render *render, struct mpp_frame *frame_info)
{

    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;

    if (frame_info == NULL) {
        loge("frame_info=NULL\n");
        return -1;
    }
    fb_render->layer.enable = 1;
    memcpy(&fb_render->layer.buf, &frame_info->buf, sizeof(struct mpp_buf));

    if (rt_device_control(fb_render->render_dev, AICFB_UPDATE_LAYER_CONFIG, &fb_render->layer) < 0)
        loge("fb ioctl() AICFB_UPDATE_LAYER_CONFIG failed!");

    // wait vsync (wait layer config)
    rt_device_control(fb_render->render_dev, AICFB_WAIT_FOR_VSYNC, &fb_render->layer);

    return 0;
}

static s32 fb_video_render_get_screen_size(struct aic_video_render *render, struct mpp_size *size)
{
    s32 ret = 0;
    struct aicfb_screeninfo screen_info;

    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;
    if (size == NULL) {
        loge("bad params!!!!\n");
        return -1;
    }
    if (rt_device_control(fb_render->render_dev, AICFB_GET_SCREENINFO, &screen_info) < 0) {
        loge("fb ioctl() AICFB_GET_SCREEN_SIZE failed!");
        return -1;
    }
    size->width = screen_info.width;
    size->height = screen_info.height;
    return ret;
}

static s32 fb_video_render_set_dis_rect(struct aic_video_render *render, struct mpp_rect *rect)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;
    if (rect == NULL) {
        loge("param error rect==NULL\n");
        return -1;
    }
    fb_render->layer.pos.x = rect->x;
    fb_render->layer.pos.y = rect->y;
    fb_render->layer.scale_size.width = rect->width;
    fb_render->layer.scale_size.height = rect->height;
    return 0;
}

static s32 fb_video_render_get_dis_rect(struct aic_video_render *render, struct mpp_rect *rect)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;
    if (rect == NULL) {
        loge("param error rect==NULL\n");
        return -1;
    }
    rect->x = fb_render->layer.pos.x;
    rect->y = fb_render->layer.pos.y;
    rect->width = fb_render->layer.scale_size.width;
    rect->height = fb_render->layer.scale_size.height;
    return 0;
}

static s32 fb_video_render_set_on_off(struct aic_video_render *render, s32 enable)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;
    fb_render->layer.enable = enable;
    return 0;
}

static s32 fb_video_render_get_on_off(struct aic_video_render *render, s32 *enable)
{
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;
    if (enable == NULL) {
        loge("param error rect==NULL\n");
        return -1;
    }
    *enable = fb_render->layer.enable;
    return 0;
}

static s32 fb_video_render_last_frame_alloc(struct mpp_frame *frame)
{
    s32 i = 0;
    s32 buf_size = 0;

    if (!frame) {
        loge("video render frame info is null.\n");
        return -1;
    }

    for (i = 0; i < 3; i++) {
        buf_size = frame->buf.stride[i] * frame->buf.size.height;
        frame->buf.phy_addr[i] = 0;
        if (buf_size) {
            frame->buf.phy_addr[i] = mpp_phy_alloc(buf_size);
            if (frame->buf.phy_addr[i] == 0) {
                loge("mpp_phy_alloc format %d, buf_size %d fail.\n",
                     frame->buf.format, buf_size);
                goto exit;
            }
        }
    }

    return 0;

exit:
    for (i = 0; i < 3; i++) {
        if (frame->buf.phy_addr[i] != 0) {
            mpp_phy_free(frame->buf.phy_addr[i]);
            frame->buf.phy_addr[i] = 0;
        }
    }

    return -1;
}

static void fb_video_render_dma_cb(void *param)
{
    logd("DMA complete, callback....\n");
}

static s32 fb_video_render_last_frame_copy(struct mpp_buf *src_buf, struct mpp_frame *dest_frame)
{
    s32 ret = 0, i = 0;
    s32 buf_size = 0;
    u32 size = 0;
    struct dma_chan *chan = NULL;

    for (i = 0; i < 3; i++) {
        if (src_buf->phy_addr[i] != 0) {
            buf_size = dest_frame->buf.stride[i] * dest_frame->buf.size.height;

            chan = dma_request_channel();
            if (chan == NULL) {
                loge("Alloc dma chan fail!\n");
                return -1;
            }
            ret = dmaengine_prep_dma_memcpy(chan, (unsigned long)dest_frame->buf.phy_addr[i],
                                            (unsigned long)src_buf->phy_addr[i], buf_size);
            if (ret) {
                loge("dmaengine_prep_dma_memcpy %d fail! ret = %d\n", i, ret);
                goto free_chan;
            }

            ret = dmaengine_submit(chan, fb_video_render_dma_cb, chan);
            if (ret) {
                loge("dmaengine_submit fail! ret = %d\n", ret);
                goto free_chan;
            }

            dma_async_issue_pending(chan);

            while (dmaengine_tx_status(chan, &size) != DMA_COMPLETE) {

            };
            aicos_dcache_invalid_range((void *)(unsigned long)dest_frame->buf.phy_addr[i],
                                       ALIGN_UP(buf_size, CACHE_LINE_SIZE));

        free_chan:
            if (chan)
                dma_release_channel(chan);
        }
    }

    return ret;
}

static s32 fb_video_render_last_frame_free(struct mpp_frame *frame)
{
    s32 i = 0;
    if (!frame) {
        loge("video render frame info is null\n");
        return -1;
    }
    for (i = 0; i < 3; i++) {
        if (frame->buf.phy_addr[i] != 0) {
            mpp_phy_free(frame->buf.phy_addr[i]);
            frame->buf.phy_addr[i] = 0;
        }
    }
    return 0;
}

static s32 fb_video_render_rend_last_frame(struct aic_video_render *render, s32 enable)
{
    s32 ret = 0;
    struct mpp_frame cur_frame;
    struct aic_fb_video_render *fb_render = (struct aic_fb_video_render *)render;

    if (fb_render == NULL) {
        loge("fb_render is not initialization!");
        return -1;
    }

    if (enable) {
        /*step1: malloc empty for last frame*/
        memcpy(&cur_frame.buf, &fb_render->layer.buf, sizeof(struct mpp_buf));

        /*the first frame need malloc buf for last frame*/
        if (!g_last_frame.enable) {
            printf("%s:alloc last frame firsttime, enable:%d.\n", __func__, enable);
            memset(&g_last_frame.frame, 0, sizeof(struct mpp_frame));
            ret = fb_video_render_last_frame_alloc(&cur_frame);
            if (ret) {
                loge("fb_video_render_last_frame_alloc failed.\n");
                return ret;
            }
            memcpy(&g_last_frame.frame, &cur_frame, sizeof(struct mpp_frame));
        } else {
            /*the frame resolution changed need realloc buf for last frame*/
            if ((g_last_frame.frame.buf.size.height != cur_frame.buf.size.height) ||
                (g_last_frame.frame.buf.size.width != cur_frame.buf.size.width)) {

                fb_video_render_last_frame_free(&g_last_frame.frame);

                ret = fb_video_render_last_frame_alloc(&cur_frame);
                if (ret) {
                    loge("fb_video_render_last_frame_alloc failed.\n");
                    return ret;
                }
                memcpy(&g_last_frame.frame, &cur_frame, sizeof(struct mpp_frame));
            }
        }

        /*step2: copy frame data to last frame*/
        fb_video_render_last_frame_copy(&fb_render->layer.buf, &g_last_frame.frame);

        /*step3: display last frame*/
        fb_video_render_rend(render, &g_last_frame.frame);
    } else {
        if (g_last_frame.enable) {
            printf("%s:reclaim the final last frame, enable:%d.\n", __func__, enable);
            /*reclaim the end last frame*/
            ret = fb_video_render_last_frame_free(&g_last_frame.frame);
            if (ret) {
                loge("fb_video_render_last_frame_free error %d.", ret);
                return ret;
            }
            memset(&g_last_frame.frame, 0, sizeof(struct mpp_frame));
        }
    }

    g_last_frame.enable = enable;
    return ret;
}

s32 aic_video_render_create(struct aic_video_render **render)
{
    struct aic_fb_video_render *fb_render;
    fb_render = mpp_alloc(sizeof(struct aic_fb_video_render));
    if (fb_render == NULL) {
        loge("mpp_alloc fb_render fail!!!\n");
        *render = NULL;
        return -1;
    }

    memset(fb_render, 0x00, sizeof(struct aic_fb_video_render));

    fb_render->base.init = fb_video_render_init;
    fb_render->base.destroy = fb_video_render_destroy;
    fb_render->base.rend = fb_video_render_rend;
    fb_render->base.set_dis_rect = fb_video_render_set_dis_rect;
    fb_render->base.get_dis_rect = fb_video_render_get_dis_rect;
    fb_render->base.set_on_off = fb_video_render_set_on_off;
    fb_render->base.get_on_off = fb_video_render_get_on_off;
    fb_render->base.get_screen_size = fb_video_render_get_screen_size;
    fb_render->base.rend_last_frame = fb_video_render_rend_last_frame;

    *render = &fb_render->base;
    return 0;
}
