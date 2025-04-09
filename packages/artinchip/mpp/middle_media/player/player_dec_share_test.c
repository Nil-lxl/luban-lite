/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Author: <che.jiang@artinchip.com>
 *  Desc: video decoder use extra buf
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
#include "player_dec_share_test.h"

#ifdef PLAYER_DEMO_USE_VE_FILL_FB
#define PLAYER_SHARE_RECEIVE_ALL_FRAME_FLAG 0x02
#define PLAYER_DEC_SHARE_THREAD_CREATE 0x0
#define PLAYER_DEC_SHARE_THREAD_RUN 0x1
#define PLAYER_DEC_SHARE_THREAD_EXIT 0x2
#define PLAYER_DEC_SHARE_THREAD_DESTROY 0x3

#define PLAYER_DEC_SHARE_FB_X 100
#define PLAYER_DEC_SHARE_FB_Y 0
#define PLAYER_DEC_SHARE_DEC_CROP_X 100
#define PLAYER_DEC_SHARE_DEC_CROP_Y 100
#define PLAYER_DEC_SHARE_DEC_CROP_WIDTH 200
#define PLAYER_DEC_SHARE_DEC_CROP_HEIGHT 200

#define PLAYER_DEC_CROP_UPDATE_TIME_1S (1000 * 1000)

struct player_fb_video_render {
    struct aicfb_layer_data layer;
    s32 fd;
    rt_device_t render_dev;
};

struct player_dec_share_data {
    pthread_t thread_id;
    s32 thread_run_flag;
    struct mpp_frame disp_frames[2];
    s32 cur_disp_frame_id;
    s32 last_disp_frame_id;
    u32 disp_frame_num;
    s32 flags;
    s32 init_flag;

    u32 receive_frame_num;
    u32 show_frame_ok_num;
    u32 show_frame_fail_num;
    u32 giveback_frame_fail_num;
    u32 giveback_frame_ok_num;

};

struct ext_frame_allocator {
	struct frame_allocator base;
};

static struct aicfb_screeninfo g_screen_info = {0};
static struct player_dec_share_data g_dec_share_data = {0};
static struct player_fb_video_render g_fb_video_render = {0};

static void *player_vdec_share_frame_thread(void *p_thread_data);

extern void player_demo_stop(void);

static int alloc_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame,
                              int width, int height, enum mpp_pixel_format format)
{
    frame->buf.format = g_screen_info.format;
    frame->buf.size.width = g_screen_info.width;
    frame->buf.size.height = g_screen_info.height;
    frame->buf.stride[0] = g_screen_info.stride;
    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.phy_addr[0] = (unsigned long)g_screen_info.framebuffer;

    return 0;
}

static int free_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    // we use the ui layer framebuffer, do not need to free

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


static int player_vdec_share_clear_screen()
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
    fill.dst_buf.phy_addr[0] = (unsigned int)g_screen_info.framebuffer;
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

s32 player_vdec_share_frame_render_init()
{
    struct player_fb_video_render *fb_render = &g_fb_video_render;
    fb_render->render_dev = rt_device_find("aicfb");

    if (fb_render->render_dev == NULL) {
        loge("rt_device_find aicfb failed!");
        return -1;
    }

    fb_render->layer.layer_id = AICFB_LAYER_TYPE_UI;
    rt_device_control(fb_render->render_dev,
                      AICFB_GET_LAYER_CONFIG, &fb_render->layer);

    if (rt_device_control(fb_render->render_dev,
                          AICFB_GET_SCREENINFO, &g_screen_info) < 0) {
        loge("fb ioctl() AICFB_GET_SCREEN_SIZE failed!");
        return -1;
    }
    player_vdec_share_clear_screen();
    return 0;
}


s32 player_vdec_share_frame_init(struct aic_player *player)
{
    if (player == NULL) {
        loge("player is NULL\n");
        return -1;
    }

    s32 ret = 0;
    struct frame_allocator *allocator = NULL;
    struct mpp_dec_crop_info crop;
    struct player_dec_share_data *p_dec_share_data = &g_dec_share_data;
    pthread_attr_t attr;

    memset(p_dec_share_data, 0, sizeof(struct player_dec_share_data));
    ret = player_vdec_share_frame_render_init();
    if (ret != 0) {
        loge("player dec share frame render init failed %d", ret);
        return -1;
    }

    /*create vdecoder frame buffer share with FB*/
    allocator = open_allocator();
    ret = aic_player_control(player, AIC_PLAYER_CMD_SET_VDEC_EXT_FRAME_ALLOCATOR,
                             (void *)allocator);
    if (ret != 0) {
        loge("player set vdec ext frame alloc failed %d", ret);
        return -1;
    }

    /*config initial crop info avoid display full decoder frame*/
    crop.crop_out_x = PLAYER_DEC_SHARE_FB_X;
    crop.crop_out_y = PLAYER_DEC_SHARE_FB_Y;
    crop.crop_x = PLAYER_DEC_SHARE_DEC_CROP_X;
    crop.crop_y = PLAYER_DEC_SHARE_DEC_CROP_Y;
    crop.crop_width = PLAYER_DEC_SHARE_DEC_CROP_WIDTH;
    crop.crop_height = PLAYER_DEC_SHARE_DEC_CROP_HEIGHT;
    aic_player_control(player, AIC_PLAYER_CMD_SET_VDEC_SET_CROP_INFO, (void *)&crop);

    /*create the thread of get decoder frame and render process*/
    pthread_attr_init(&attr);
    attr.stacksize = 4 * 1024;
    attr.schedparam.sched_priority = 25;
    p_dec_share_data->thread_run_flag = PLAYER_DEC_SHARE_THREAD_CREATE;
    ret = pthread_create(&p_dec_share_data->thread_id, &attr,
                         player_vdec_share_frame_thread, (void *)player);
    if (ret != 0) {
        loge("create thread player vdec share frame failed %d", ret);
        return -1;
    }
    p_dec_share_data->init_flag = 1;

    return 0;
}

s32 player_vdec_share_frame_deinit()
{
    g_dec_share_data.thread_run_flag = PLAYER_DEC_SHARE_THREAD_EXIT;

    return 0;
}

static void player_vdec_share_print_frame_count()
{
    struct player_dec_share_data *p_dec_share_data = &g_dec_share_data;
    printf("[%s:%d]receive_frame_num:%u,"
           "show_frame_ok_num:%u,"
           "show_frame_fail_num:%u,"
           "giveback_frame_ok_num:%u,"
           "giveback_frame_fail_num:%u,"
           "disp_frame_num:%u\n",
           __FUNCTION__, __LINE__, p_dec_share_data->receive_frame_num,
           p_dec_share_data->show_frame_ok_num,
           p_dec_share_data->show_frame_fail_num,
           p_dec_share_data->giveback_frame_ok_num,
           p_dec_share_data->giveback_frame_fail_num,
           p_dec_share_data->disp_frame_num);
}

static void player_vdec_share_render_frame_swap(s32 *a, s32 *b)
{
    s32 tmp = *a;
    *a = *b;
    *b = tmp;
}

s32 player_vdec_share_get_frame(struct aic_player *player,
                                struct mpp_frame *frame_info)
{
    return aic_player_control(player,
                              AIC_PLAYER_CMD_GET_VDEC_DECODER_FRAME,
                              (void *)frame_info);
}

s32 player_vdec_share_put_frame(struct aic_player *player,
                                struct mpp_frame *frame_info)
{
    return aic_player_control(player,
                              AIC_PLAYER_CMD_PUT_VDEC_DECODER_FRAME,
                              (void *)frame_info);
}

/*
*update crop info
*fb_dec_x\fb_dec_y: set dec position offset on fb
*dec_crop_x\fb_dec_y\dec_crop_width\dec_crop_height: set dec crop info
*frame->buf.size width\height: should be equal to fb size
*frame->buf.crop width\height: decoder picture size
*
*                  fb width
*|---------------------------------------------|
*|                                             |
*|     (fb_dec_x, fb_dec_y)                    |
*|         |-----------------------------|     |
*|         |   (dec_crop_x, dec_crop_y)  |     |
*|         |     |--------------|        |     |
*|         |     |              |        |     |  fb height
*|         |     |  dec_crop    |        |     |
*|         |     |              |        |     |
*|         |     |--------------|        |     |
*|         |                             |     |
*|         |-----------------------------|     |
*|                                             |
*|                                             |
*|---------------------------------------------|
*
*/
s32 player_vdec_share_update_crop(struct aic_player *player, struct mpp_frame *frame)
{
    static struct timespec pev = { 0 }, cur = { 0 };

    if (pev.tv_sec == 0) {
        clock_gettime(CLOCK_REALTIME, &pev);
        return 0;
    } else {
        long diff = 0;
        clock_gettime(CLOCK_REALTIME, &cur);
        diff = (cur.tv_sec - pev.tv_sec) * 1000 * 1000 +
               (cur.tv_nsec - pev.tv_nsec) / 1000;

        /*update config every  1s */
        if (diff > PLAYER_DEC_CROP_UPDATE_TIME_1S) {
            pev = cur;
        } else {
            return 0;
        }
    }

    static unsigned int fb_dec_x = PLAYER_DEC_SHARE_FB_X;
    static unsigned int fb_dec_y = PLAYER_DEC_SHARE_FB_Y;
    static unsigned int dec_crop_x = PLAYER_DEC_SHARE_DEC_CROP_X;
    static unsigned int dec_crop_y = PLAYER_DEC_SHARE_DEC_CROP_Y;
    static unsigned int dec_crop_width = PLAYER_DEC_SHARE_DEC_CROP_WIDTH;
    static unsigned int dec_crop_height = PLAYER_DEC_SHARE_DEC_CROP_HEIGHT;

    struct mpp_dec_crop_info crop;

    /*clear screen, avoid the old residual data display on screen*/
    player_vdec_share_clear_screen();

    crop.crop_out_x = fb_dec_x;
    crop.crop_out_y = fb_dec_y;
    crop.crop_x = dec_crop_x;
    crop.crop_y = dec_crop_y;
    crop.crop_width = dec_crop_width;
    crop.crop_height = dec_crop_height;

    printf("screen: width %d, height %d, frame type%d:  %d(%d,%d,%d) x %d, frame crop[%d,%d,%d,%d]\n",
        g_screen_info.width, g_screen_info.height, frame->buf.buf_type, frame->buf.size.width,
        frame->buf.stride[0], frame->buf.stride[1],frame->buf.stride[2],frame->buf.size.height,
        frame->buf.crop.x, frame->buf.crop.y, frame->buf.crop.width, frame->buf.crop.height);
    printf("crop_info: x %d, y %d, width %d height %d out_x %d out_y %d\n",
        crop.crop_x, crop.crop_y, crop.crop_width,
        crop.crop_height, crop.crop_out_x, crop.crop_out_y);

    /*update new decoder offset */
    aic_player_control(player, AIC_PLAYER_CMD_SET_VDEC_SET_CROP_INFO, (void *)&crop);

    /*dynamic adjust fb dec position and dec crop info*/
    fb_dec_x += 8;
    fb_dec_y += 2;
    dec_crop_x += 2;
    dec_crop_y += 2;
    dec_crop_width += 8;
    dec_crop_height += 4;

    /*checking bodunary */
    if (dec_crop_x + dec_crop_width >= frame->buf.crop.width) {
        dec_crop_x = PLAYER_DEC_SHARE_DEC_CROP_X;
        dec_crop_width = PLAYER_DEC_SHARE_DEC_CROP_WIDTH;
    }
    if (dec_crop_y + dec_crop_height >= frame->buf.crop.height) {
        dec_crop_y = PLAYER_DEC_SHARE_DEC_CROP_X;
        dec_crop_height = PLAYER_DEC_SHARE_DEC_CROP_HEIGHT;
    }

    if (fb_dec_x + frame->buf.crop.width > g_screen_info.width) {
        fb_dec_x = PLAYER_DEC_SHARE_FB_X;
        dec_crop_width = PLAYER_DEC_SHARE_DEC_CROP_WIDTH;
    }
    if (fb_dec_y + frame->buf.crop.height > g_screen_info.height) {
        fb_dec_y = PLAYER_DEC_SHARE_FB_Y;
        dec_crop_height = PLAYER_DEC_SHARE_DEC_CROP_HEIGHT;
    }

    if (dec_crop_x > PLAYER_DEC_SHARE_DEC_CROP_X + 100)
        dec_crop_x = PLAYER_DEC_SHARE_DEC_CROP_X;

    if (dec_crop_y > PLAYER_DEC_SHARE_DEC_CROP_Y + 100)
        dec_crop_y = PLAYER_DEC_SHARE_DEC_CROP_Y;

    return 0;
}

static s32 player_vdec_share_video_render(struct mpp_frame *frame)
{
    if (frame == NULL) {
        loge("frame_info is NULL\n");
        return -1;
    }

    struct aicfb_layer_data layer = {0};
    struct player_fb_video_render *fb_render =
        (struct player_fb_video_render *)&g_fb_video_render;

    layer.layer_id = AICFB_LAYER_TYPE_UI;
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

static void *player_vdec_share_frame_thread(void *p_thread_data)
{
    s32 ret = 0;
    s32 cur_frame_id, last_frame_id;
    struct player_dec_share_data *p_dec_share_data = &g_dec_share_data;

    struct aic_player *player = (struct aic_player *)p_thread_data;
    p_dec_share_data->thread_run_flag = PLAYER_DEC_SHARE_THREAD_RUN;

    while (p_dec_share_data->thread_run_flag == PLAYER_DEC_SHARE_THREAD_RUN) {
        /*get new frame flag eos flag then exit*/
        if (p_dec_share_data->flags & PLAYER_SHARE_RECEIVE_ALL_FRAME_FLAG) {
            player_demo_stop();
            goto exit;
        }


        /* get frame from player vdec*/
        cur_frame_id = p_dec_share_data->cur_disp_frame_id;
        last_frame_id = p_dec_share_data->last_disp_frame_id;
        ret = player_vdec_share_get_frame(player,
                                          &p_dec_share_data->disp_frames[cur_frame_id]);
        if (ret != DEC_OK) {
            usleep(16000);
            continue;
        }
        p_dec_share_data->receive_frame_num++;

        /*update vdec crop info*/
        player_vdec_share_update_crop(player, &p_dec_share_data->disp_frames[cur_frame_id]);


        /* do render one frame*/
        ret = player_vdec_share_video_render(
            &p_dec_share_data->disp_frames[cur_frame_id]);
        if (ret == 0) {
            p_dec_share_data->disp_frame_num++;
            if (p_dec_share_data->disp_frames[cur_frame_id].flags & FRAME_FLAG_EOS) {
                p_dec_share_data->flags |= PLAYER_SHARE_RECEIVE_ALL_FRAME_FLAG;
                printf("[%s:%d]receive frame_end_flag\n", __FUNCTION__, __LINE__);
            }
        } else {
            loge("render error");
        }

        /*put back frame to player vdec component*/
        if (p_dec_share_data->disp_frame_num) {
            ret = player_vdec_share_put_frame(player,
                                              &p_dec_share_data->disp_frames[last_frame_id]);
            if (0 == ret) {
                p_dec_share_data->giveback_frame_ok_num++;
            } else {
                p_dec_share_data->giveback_frame_fail_num++;
            }
        }
        player_vdec_share_render_frame_swap(&p_dec_share_data->cur_disp_frame_id,
                                            &p_dec_share_data->last_disp_frame_id);

        usleep(10000);
    }

exit:
    pthread_join(p_dec_share_data->thread_id, NULL);
    p_dec_share_data->init_flag = 0;
    p_dec_share_data->thread_run_flag = PLAYER_DEC_SHARE_THREAD_DESTROY;
    printf("[%s:%d]player dec share thread exit\n", __FUNCTION__, __LINE__);
    player_vdec_share_print_frame_count();
    return NULL;
}
#endif
