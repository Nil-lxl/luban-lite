/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: player audio special demo
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
#include "aic_parser.h"
#include "player_audio_test.h"
#include "aic_audio_render_manager.h"

#ifdef AIC_MPP_PLAYER_AUDIO_RENDER_SHARE_TEST

#define USER_AUDIO_RENDER_PRIORITY AUDIO_RENDER_SCENE_HIGHEST_PRIORITY
#define USER_AUDIO_PLAYER_WAVE_BUFF_SIZE (8 * 1024)

struct user_audio_render {
    struct aic_audio_render *render;
    struct aic_audio_render_attr ao_attr;
    int user_audio_play_enable;
    struct aic_parser *parser;
};

struct user_audio_render g_user_audio_render = {0};


static int player_audio_render_play_warning_tone(struct user_audio_render *p_user_audio_render)
{
    int ret = 0;
    struct aic_parser_packet packet;
    unsigned char *wav_buff = NULL;

    int wav_buff_size = USER_AUDIO_PLAYER_WAVE_BUFF_SIZE;

    wav_buff = aicos_malloc(MEM_DEFAULT, wav_buff_size);
    if (wav_buff == NULL) {
        loge("aicos_malloc error\n");
        return -1;
    }


    while (1) {
        /*Read wav data*/
        memset(&packet, 0, sizeof(struct aic_parser_packet));
        ret = aic_parser_peek(p_user_audio_render->parser, &packet);
        if (ret == PARSER_EOS) {
            break;
        }
        if (packet.size > wav_buff_size) {
            loge("pkt size[%d] larger than wav_buf_size[%d]\n",
                packet.size, wav_buff_size);
            break;
        }

        packet.data = wav_buff;
        aic_parser_read(p_user_audio_render->parser, &packet);

        /*Do Audio Render*/
        aic_audio_render_rend(p_user_audio_render->render, packet.data, packet.size);
        usleep(1000);
    }

    if (wav_buff)
        free(wav_buff);

    return 0;
}




s32 player_audio_render_share_play(char *file_path)
{
    s32 ret = 0;
    s32 value = 0;

    struct aic_parser_av_media_info media_info;
    struct aic_audio_render_attr ao_attr;
    struct audio_render_create_params create_params;
    struct user_audio_render *p_user_audio_render = &g_user_audio_render;


    if (!file_path) {
        loge("file_path is null\n");
        return -1;
    }

    aic_parser_create((unsigned char *)file_path, &p_user_audio_render->parser);
    if (p_user_audio_render->parser == NULL) {
        loge("parser create %s failed", file_path);
        goto exit;
    }
    if (aic_parser_init(p_user_audio_render->parser)) {
        loge("aic_parser_init fail\n");
        goto exit;
    }
    if (aic_parser_get_media_info(p_user_audio_render->parser, &media_info)) {
        loge("aic_parser_get_media_info fail\n");
        return -1;
    }

    p_user_audio_render->ao_attr.channels = media_info.audio_stream.nb_channel;
    p_user_audio_render->ao_attr.sample_rate = media_info.audio_stream.sample_rate;
    p_user_audio_render->ao_attr.bits_per_sample = media_info.audio_stream.bits_per_sample;

    logi("play warningtone channels:%d, sample_rate:%d, bit_per_sample:%d",
        p_user_audio_render->ao_attr.channels, p_user_audio_render->ao_attr.sample_rate,
        p_user_audio_render->ao_attr.bits_per_sample);

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
    if (p_user_audio_render->render) {
        if (aic_audio_render_control(p_user_audio_render->render,
                                    AUDIO_RENDER_CMD_SET_ATTR,
                                    &ao_attr)) {
            loge("aic_audio_render_control set the last attribute failed\n");
        }

        aic_audio_render_destroy(p_user_audio_render->render);
        p_user_audio_render->render = NULL;
    }

    if (p_user_audio_render->parser) {
        aic_parser_destroy(p_user_audio_render->parser);
        p_user_audio_render->parser = NULL;
    }

    return ret;
}

#endif
