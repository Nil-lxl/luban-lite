/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Author: <che.jiang@artinchip.com>
 *  Desc: audio render share demo
 */

#include "aic_osal.h"
#include "frame_allocator.h"
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
#include "aic_audio_render_manager.h"
#include "player_audio_render_share_test.h"

#ifdef AIC_MPP_PLAYER_AUDIO_RENDER_SHARE_TEST

#define USER_AUDIO_RENDER_FILE_CNT 9
#define USER_AUDIO_RENDER_PRIORITY AUDIO_RENDER_SCENE_HIGHEST_PRIORITY
#define USER_AUDIO_RENDER_SAMPLERATE 44100
#define USER_AUDIO_RENDER_CHANNELS 1

struct user_audio_render {
    struct aic_audio_render *render;
    struct aic_audio_render_attr ao_attr;
    int user_audio_play_enable;

};

struct user_audio_render g_user_audio_render = {0};
char g_user_file_path[USER_AUDIO_RENDER_FILE_CNT][64] = {
    "/sdcard/pcm16/welcome.pcm",
    "/sdcard/pcm16/yue.pcm",
    "/sdcard/pcm16/b.pcm",
    "/sdcard/pcm16/1.pcm",
    "/sdcard/pcm16/2.pcm",
    "/sdcard/pcm16/3.pcm",
    "/sdcard/pcm16/4.pcm",
    "/sdcard/pcm16/5.pcm",
    "/sdcard/pcm16/6.pcm",
};


static int pcm_file_read(int idx, char **data, int *size)
{
    if (idx >= USER_AUDIO_RENDER_FILE_CNT) {
        return -1;
    }
    int buf_size;
    char *buf_data;

    int fd = open(g_user_file_path[idx], O_RDONLY);
    if (fd < 0) {
        loge("open file %s failed", g_user_file_path[idx]);
        return -1;
    }

    buf_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    printf("[%s]read %d file %s buf_size %d.\n", __func__,
        idx, g_user_file_path[idx], buf_size);

    buf_data = mpp_alloc(buf_size);
    if (!buf_data) {
        loge("malloc pcm size %d failed", buf_size);
        return -1;
    }

    read(fd, buf_data, buf_size);

    close(fd);

    *data = buf_data;
    *size = buf_size;

    return 0;
}

static int64_t pcm_render_wait_time(int size)
{
    if (size <= 0)
        return 0;
    struct aic_audio_render_attr *p_ao_attr = &g_user_audio_render.ao_attr;
    int pcm_size_1s = p_ao_attr->sample_rate * 16 / 8;
    int64_t data_size = (int64_t)size;

    if (pcm_size_1s <= 0)
        return 0;

    return (data_size * 1000000 / pcm_size_1s);//us
}

static void player_audio_render_play_warning_tone(struct user_audio_render *p_user_audio_render)
{
    int pcm_idx = 0;
    int pcm_size = 0;
    char *pcm_buf = NULL;
    int64_t wait_time = 0;


    while (1) {
        /*Read pcm File*/
        if (pcm_file_read(pcm_idx, &pcm_buf, &pcm_size)) {
            if (pcm_buf)
                mpp_free(pcm_buf);
            break;
        }

        /*Do Audio Render*/
        aic_audio_render_rend(p_user_audio_render->render, pcm_buf, pcm_size);
        pcm_idx++;
        wait_time = pcm_render_wait_time(pcm_size);
        printf("pcm_size %d, idx %d, wait_time %ld.\n", pcm_size, pcm_idx, wait_time);
        usleep(wait_time);
        if (pcm_buf) {
            mpp_free(pcm_buf);
            pcm_buf = NULL;
        }
    }
}

s32 player_audio_render_share_play()
{
    s32 ret = 0;
    s32 value = 0;
    struct aic_audio_render_attr ao_attr;
    struct audio_render_create_params create_params;
    struct user_audio_render *p_user_audio_render = &g_user_audio_render;

    p_user_audio_render->ao_attr.channels = USER_AUDIO_RENDER_CHANNELS;
    p_user_audio_render->ao_attr.sample_rate = USER_AUDIO_RENDER_SAMPLERATE;

    create_params.dev_id = 0;
    create_params.scene_type = AUDIO_RENDER_SCENE_WARNING_TONE;
    if (aic_audio_render_create(&p_user_audio_render->render, &create_params)) {
        loge("aic_audio_render_create failed\n");
        return -1;
    }

    if (aic_audio_render_init(p_user_audio_render->render)) {
        loge("aic_audio_render_init failed\n");
        ret = -1;
        goto exit;
    }

    value = USER_AUDIO_RENDER_PRIORITY;
    if (aic_audio_render_control(p_user_audio_render->render,
                                 AUDIO_RENDER_CMD_SET_SCENE_PRIORITY,
                                 &value)) {
        loge("aic_audio_render_control set priority failed\n");
        ret = -1;
        goto exit;
    }

    if (aic_audio_render_control(p_user_audio_render->render,
                                 AUDIO_RENDER_CMD_CLEAR_CACHE,
                                 NULL)) {
        loge("aic_audio_render_control clear cache failed\n");
        return -1;
    }

    if (aic_audio_render_control(p_user_audio_render->render,
                                 AUDIO_RENDER_CMD_GET_ATTR,
                                 &ao_attr)) {
        loge("aic_audio_render_control get priority failed\n");
        ret = -1;
        goto exit;
    }

    /*Set warnning tone audio params*/
    if (aic_audio_render_control(p_user_audio_render->render,
                                AUDIO_RENDER_CMD_SET_ATTR,
                                &p_user_audio_render->ao_attr)) {
        loge("aic_audio_render_control set new attribute failed\n");
        ret = -1;
        goto exit;
    }


    value = 100;
    if (aic_audio_render_control(p_user_audio_render->render,
                                 AUDIO_RENDER_CMD_SET_VOL,
                                 &value)) {
        loge("aic_audio_render_control set volume failed\n");
        ret = -1;
        goto exit;
    }


    player_audio_render_play_warning_tone(p_user_audio_render);

exit:

    /*Play warnning tone over, change to the last audio attribute*/
    p_user_audio_render->ao_attr.channels = ao_attr.channels;
    p_user_audio_render->ao_attr.sample_rate = ao_attr.sample_rate;
    if (aic_audio_render_control(p_user_audio_render->render,
                                AUDIO_RENDER_CMD_SET_ATTR,
                                &p_user_audio_render->ao_attr)) {
        loge("aic_audio_render_control set the last attribute failed\n");
    }

    aic_audio_render_destroy(p_user_audio_render->render);
    p_user_audio_render->render = NULL;
    return ret;
}

#endif
