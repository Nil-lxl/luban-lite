/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: player video special demo
 */

#include "aic_osal.h"
#include "aic_player.h"
#include "frame_allocator.h"
#include "mpp_decoder.h"
#include "mpp_fb.h"
#include "mpp_ge.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "player_video_test.h"

#ifdef PLAYER_DEMO_VIDEO_EXT_RENDER
#define EXT_RENDER_RECV_ALL_FRAME_FLAG  0x02
#define VIDEO_EXT_RENDER_THREAD_CREATE  0x0
#define VIDEO_EXT_RENDER_THREAD_RUN     0x1
#define VIDEO_EXT_RENDER_THREAD_EXIT    0x2
#define VIDEO_EXT_RENDER_THREAD_DESTROY 0x3

struct player_fb_video_render {
    struct aicfb_layer_data layer;
    s32 fd;
    rt_device_t render_dev;
};

struct video_ext_render_data {
    pthread_t thread_id;
    s32 thread_run_flag;
    struct mpp_frame disp_frames[2];
    s32 cur_disp_frame_id;
    s32 last_disp_frame_id;
    u32 disp_frame_num;
    s32 flags;
    s32 init_flag;
    bool debug_en;

    u32 receive_frame_num;
    u32 giveback_frame_fail_num;
    u32 giveback_frame_ok_num;
};

struct ext_frame_allocator {
	struct frame_allocator base;
};

static struct aicfb_screeninfo g_screen_info = {0};
static struct video_ext_render_data g_video_ext_render = {0};
static struct player_fb_video_render g_fb_video_render = {0};

static void *video_ext_render_thread(void *p_thread_data);

extern void player_demo_stop(void);


#ifndef AIC_CHIP_D21X
static int get_fb_info(struct mpp_buf *buf, int *comp, int *mem_size)
{
    int height = buf->size.height;

    mem_size[0] = height * buf->stride[0];
    switch (buf->format) {
    case MPP_FMT_YUV420P:
        *comp = 3;
        mem_size[1] = mem_size[2] = mem_size[0] >> 2;
        break;
    case MPP_FMT_YUV444P:
        *comp = 3;
        mem_size[1] = mem_size[2] = mem_size[0];
        break;

    case MPP_FMT_YUV422P:
        *comp = 3;
        mem_size[1] = mem_size[2] = mem_size[0] >> 1;
        break;
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
        *comp = 2;
        mem_size[1] = mem_size[0] >> 1;
        break;
    case MPP_FMT_YUV400:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_BGR_888:
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_565:
    case MPP_FMT_RGB_565:
        *comp = 1;
        break;

    default:
        loge("pixel format not support %d", buf->format);
        return -1;
    }

    return 0;
}

static int alloc_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame,
                              int width, int height, enum mpp_pixel_format format)
{
    int mem_size[3] = {0, 0, 0};
    int comp = 0;
    int i;

    frame->buf.size.height = height;
    frame->buf.stride[0] = width;
    frame->buf.format = format;

    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.phy_addr[0] = frame->buf.phy_addr[1] = frame->buf.phy_addr[2] = 0;

    get_fb_info(&frame->buf, &comp, mem_size);

    for (i = 0; i < comp; i++) {
        frame->buf.phy_addr[i] = mpp_phy_alloc(mem_size[i]);

        if (frame->buf.phy_addr[i] == 0) {
            loge("alloc(%d) failed, need %d bytes", i, mem_size[i]);
            goto failed;
        }
        logi("alloc frame buf.phy_addr[%d] = 0x%x, size = %d\n",
            i, frame->buf.phy_addr[i], mem_size[i]);
    }

    return 0;

failed:
    for (i = 0; i < comp; i++) {
        if (frame->buf.phy_addr[i]) {
            mpp_phy_free(frame->buf.phy_addr[i]);
            frame->buf.phy_addr[i] = 0;
        }
    }
    return -1;
}

static int free_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    int mem_size[3] = {0, 0, 0};
    int comp = 0;
    int i;

    get_fb_info(&frame->buf, &comp, mem_size);

    for (i = 0; i < comp; i++) {
        if (frame->buf.phy_addr[i]) {
            logi("free frame buf.phy_addr[%d] = 0x%x\n",
                i, frame->buf.phy_addr[i]);
            mpp_phy_free(frame->buf.phy_addr[i]);
            frame->buf.phy_addr[i] = 0;
        }
    }
    return 0;
}

static int close_allocator(struct frame_allocator *p)
{
    struct ext_frame_allocator *impl = (struct ext_frame_allocator *)p;
    mpp_free(impl);

    return 0;
}

static struct alloc_ops def_ops = {
    .alloc_frame_buffer = alloc_frame_buffer,
    .free_frame_buffer = free_frame_buffer,
    .close_allocator = close_allocator,
};

static struct frame_allocator *open_allocator()
{
    struct ext_frame_allocator *impl =
        (struct ext_frame_allocator *)mpp_alloc(sizeof(struct ext_frame_allocator));
    if (impl == NULL) {
        return NULL;
    }
    memset(impl, 0, sizeof(struct ext_frame_allocator));

    impl->base.ops = &def_ops;

    return &impl->base;
}
#endif

static int clear_frame_buffer()
{
    int ret = 0;
    struct ge_fillrect fill = {0};
    struct mpp_ge *ge = NULL;

    ge = mpp_ge_open();
    if (!ge) {
        loge("open ge device error\n");
        return -1;
    }

    fill.ctrl.flags = 0;
    fill.type = GE_NO_GRADIENT;
    fill.start_color = 0;
    //fill.end_color = 0;

    //fb_phy = fb_info->framebuffer;
    fill.dst_buf.buf_type = MPP_PHY_ADDR;
    fill.dst_buf.phy_addr[0] =
        (unsigned long)g_screen_info.framebuffer;
    fill.dst_buf.stride[0] = g_screen_info.stride;
    fill.dst_buf.size.width = g_screen_info.width;
    fill.dst_buf.size.height = g_screen_info.height;
    fill.dst_buf.format = g_screen_info.format;
    fill.dst_buf.crop_en = 0;

    ret =  mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        loge("ge fillrect fail\n");
        ret = -1;
        goto _EXIT;
    }
    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        loge("ge emit fail\n");
        ret = -1;
        goto _EXIT;
    }
    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        loge("ge sync fail\n");
        ret = -1;
        goto _EXIT;
    }

_EXIT:
    if (ge)
        mpp_ge_close(ge);
    return ret;
}

s32 video_ext_render_init()
{
    struct player_fb_video_render *fb_render = &g_fb_video_render;
    fb_render->render_dev = rt_device_find("aicfb");

    if (fb_render->render_dev == NULL) {
        loge("rt_device_find aicfb failed!");
        return -1;
    }
#ifdef AIC_CHIP_D21X
    fb_render->layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
#else
    fb_render->layer.layer_id = AICFB_LAYER_TYPE_UI;
#endif
    rt_device_control(fb_render->render_dev,
                      AICFB_GET_LAYER_CONFIG, &fb_render->layer);

    if (rt_device_control(fb_render->render_dev,
                          AICFB_GET_SCREENINFO, &g_screen_info) < 0) {
        loge("fb ioctl() AICFB_GET_SCREEN_SIZE failed!");
        return -1;
    }
    printf("g_screen_info:width=%d, height=%d!\n", g_screen_info.width, g_screen_info.height);
    clear_frame_buffer();
    return 0;
}


s32 player_video_ext_render_init(struct aic_player *player)
{
    if (player == NULL) {
        loge("player is NULL\n");
        return -1;
    }

    s32 ret = 0;

    struct video_ext_render_data *p_ext_render = &g_video_ext_render;
    pthread_attr_t attr;

    memset(p_ext_render, 0, sizeof(struct video_ext_render_data));
    ret = video_ext_render_init();
    if (ret != 0) {
        loge("player dec share frame render init failed %d", ret);
        return -1;
    }

#ifndef AIC_CHIP_D21X
    struct frame_allocator *allocator = NULL;
    s32 ext_frame_num = 1;
    /*create vdecoder frame buffer share with FB*/
    ret = aic_player_control(player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_NUM,
                             (void *)&ext_frame_num);

    allocator = open_allocator();
    ret = aic_player_control(player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_ALLOCATOR,
                             (void *)allocator);
    if (ret != 0) {
        loge("player set vdec ext frame alloc failed %d", ret);
        return -1;
    }
#endif
    /*create the thread of get decoder frame and render process*/
    pthread_attr_init(&attr);
    attr.stacksize = 4 * 1024;
    attr.schedparam.sched_priority = 25;
    p_ext_render->thread_run_flag = VIDEO_EXT_RENDER_THREAD_CREATE;
    ret = pthread_create(&p_ext_render->thread_id, &attr,
                         video_ext_render_thread, (void *)player);
    if (ret != 0) {
        loge("create thread player ext render failed %d", ret);
        return -1;
    }
    p_ext_render->init_flag = 1;

    return 0;
}

s32 player_video_ext_render_deinit()
{
    g_video_ext_render.thread_run_flag = VIDEO_EXT_RENDER_THREAD_EXIT;
    if (g_video_ext_render.thread_id != 0)
        pthread_join(g_video_ext_render.thread_id, NULL);

    return 0;
}

static void player_video_ext_render_print()
{
    struct video_ext_render_data *p_ext_render = &g_video_ext_render;

    if (!p_ext_render->debug_en)
        return;

    printf("************************Ext Video_render info***********************\n");

    printf("recv_ok    display    give_ok    give_fail\n");
    printf("%7u    %7u    %7u    %9u\n",
        p_ext_render->receive_frame_num,
        p_ext_render->disp_frame_num,
        p_ext_render->giveback_frame_ok_num,
        p_ext_render->giveback_frame_fail_num);
}

void player_video_ext_render_debug(bool debug_en)
{
    g_video_ext_render.debug_en = debug_en;
    player_video_ext_render_print();
}

static void video_ext_render_frame_swap(s32 *a, s32 *b)
{
    s32 tmp = *a;
    *a = *b;
    *b = tmp;
}


static s32 video_ext_render_rend(struct mpp_frame *frame)
{
    if (frame == NULL) {
        loge("frame_info is NULL\n");
        return -1;
    }

    struct aicfb_layer_data layer = {0};
    struct player_fb_video_render *fb_render =
        (struct player_fb_video_render *)&g_fb_video_render;
#ifdef AIC_CHIP_D21X
    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
#else
    layer.layer_id = AICFB_LAYER_TYPE_UI;
#endif
    layer.rect_id = 0;
    layer.enable = 1;
    layer.buf.phy_addr[0] = frame->buf.phy_addr[0];
    layer.buf.phy_addr[1] = frame->buf.phy_addr[1];
    layer.buf.phy_addr[2] = frame->buf.phy_addr[2];
    layer.buf.stride[0] = frame->buf.stride[0];
    layer.buf.stride[1] = frame->buf.stride[1];
    layer.buf.stride[2] = frame->buf.stride[2];
    layer.buf.size.width = frame->buf.size.width;
    layer.buf.size.height = frame->buf.size.height;
    layer.buf.crop_en = 0;
    layer.buf.format = frame->buf.format;
    layer.buf.buf_type = MPP_PHY_ADDR;


    if (rt_device_control(fb_render->render_dev,
                          AICFB_UPDATE_LAYER_CONFIG,
                          &layer) < 0) {
        loge("fb ioctl() AICFB_UPDATE_LAYER_CONFIG failed!");
    }

    rt_device_control(fb_render->render_dev,
                      AICFB_WAIT_FOR_VSYNC, &layer);

    return 0;
}

static void *video_ext_render_thread(void *p_thread_data)
{
    struct video_ext_render_data *p_ext_render = &g_video_ext_render;
    s32 cur_frame_id, last_frame_id;
    s32 ret = 0;

    struct aic_player *player = (struct aic_player *)p_thread_data;
    p_ext_render->thread_run_flag = VIDEO_EXT_RENDER_THREAD_RUN;

    while (p_ext_render->thread_run_flag == VIDEO_EXT_RENDER_THREAD_RUN) {
        /*get new frame flag eos flag then exit*/
        if (p_ext_render->flags & EXT_RENDER_RECV_ALL_FRAME_FLAG) {
            goto exit;
        }

        /* get frame from player vdec*/
        cur_frame_id = p_ext_render->cur_disp_frame_id;
        last_frame_id = p_ext_render->last_disp_frame_id;
        ret = aic_player_get_frame(player, &p_ext_render->disp_frames[cur_frame_id]);
        if (ret != 0) {
            usleep(5000);
            continue;
        }
        p_ext_render->receive_frame_num++;

        /* do render one frame*/
        ret = video_ext_render_rend(&p_ext_render->disp_frames[cur_frame_id]);
        if (ret == 0) {
            p_ext_render->disp_frame_num++;
            if (p_ext_render->disp_frames[cur_frame_id].flags & FRAME_FLAG_EOS) {
                p_ext_render->flags |= EXT_RENDER_RECV_ALL_FRAME_FLAG;
                printf("[%s:%d]receive frame_end_flag\n", __FUNCTION__, __LINE__);
            }
        } else {
            loge("render error");
        }

        /*put back frame to player vdec component*/
        if (p_ext_render->disp_frame_num) {
            ret = aic_player_put_frame(player,
                &p_ext_render->disp_frames[last_frame_id]);
            if (0 == ret) {
                p_ext_render->giveback_frame_ok_num++;
            } else {
                p_ext_render->giveback_frame_fail_num++;
            }
        }

        video_ext_render_frame_swap(&p_ext_render->cur_disp_frame_id,
                               &p_ext_render->last_disp_frame_id);

        usleep(5000);
    }

exit:
    p_ext_render->init_flag = 0;
    p_ext_render->thread_run_flag = VIDEO_EXT_RENDER_THREAD_DESTROY;
    printf("[%s:%d]player ext render exit\n", __FUNCTION__, __LINE__);
    player_video_ext_render_print();
    return NULL;
}
#endif
