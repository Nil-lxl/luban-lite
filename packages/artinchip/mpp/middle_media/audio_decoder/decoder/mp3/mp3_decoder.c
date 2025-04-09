/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: aic_audio_decoder interface
 */

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#include "mpp_dec_type.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_log.h"

#include "audio_decoder.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#include "minimp3.h"

#define PCM_PADDING_SIZE (512)
#define PCM_BUFFER_SIZE (MINIMP3_MAX_SAMPLES_PER_FRAME * 2 + PCM_PADDING_SIZE)


struct mp3_audio_decoder {
    struct aic_audio_decoder decoder;
    struct mpp_packet* curr_packet;
    mp3dec_t mp3d;

    int channels;
    int sample_rate;
    int bits_per_sample;
    uint32_t frame_id;
    int frame_count;
};

int __mp3_decode_init(struct aic_audio_decoder *decoder, struct aic_audio_decode_config *config)
{
    struct mp3_audio_decoder *mp3_decoder = (struct mp3_audio_decoder *)decoder;
    struct audio_frame_manager_cfg cfg;

    mp3_decoder->decoder.pm = audio_pm_create(config);
    if (!mp3_decoder->decoder.pm) {
        loge("audio_pm_create mp3 failed.\n");
        return -1;
    }
    mp3_decoder->frame_count = config->frame_count;
    mp3_decoder->frame_id = 0;

    cfg.bits_per_sample = 16;
    cfg.samples_per_frame = PCM_BUFFER_SIZE;
    cfg.frame_count = mp3_decoder->frame_count;
    mp3_decoder->decoder.fm = audio_fm_create(&cfg);
    if (mp3_decoder->decoder.fm == NULL) {
        loge("audio_fm_create mp3 fail!!!\n");
        goto failed;
    }
    mp3_decoder->bits_per_sample = cfg.bits_per_sample;

    mp3dec_init(&mp3_decoder->mp3d);

    return 0;

failed:
    if (mp3_decoder->decoder.pm) {
        audio_pm_destroy(mp3_decoder->decoder.pm);
        mp3_decoder->decoder.pm = NULL;
    }

    return -1;
}

int __mp3_decode_destroy(struct aic_audio_decoder *decoder)
{
    struct mp3_audio_decoder *mp3_decoder = (struct mp3_audio_decoder *)decoder;
    if (!mp3_decoder) {
        return -1;
    }

    if (mp3_decoder->decoder.pm) {
        audio_pm_destroy(mp3_decoder->decoder.pm);
        mp3_decoder->decoder.pm = NULL;
    }
    if (mp3_decoder->decoder.fm) {
        audio_fm_destroy(mp3_decoder->decoder.fm);
        mp3_decoder->decoder.fm = NULL;
    }

    mpp_free(mp3_decoder);
    return 0;

}

static int set_pcm_frame_info(struct aic_audio_decoder *decoder,
                              mp3dec_frame_info_t *mp3_frame_info,
                              struct aic_audio_frame *frame)
{
    struct mp3_audio_decoder *mp3_decoder = (struct mp3_audio_decoder *)decoder;

    frame->channels =  mp3_frame_info->channels;
    frame->sample_rate = mp3_frame_info->hz;
    frame->pts = mp3_decoder->curr_packet->pts;
    frame->bits_per_sample = mp3_decoder->bits_per_sample;
    frame->id = mp3_decoder->frame_id++;
    frame->flag = mp3_decoder->curr_packet->flag;

    return 0;
}

int __mp3_decode_frame(struct aic_audio_decoder *decoder)
{
    int samples = 0;
    mp3dec_frame_info_t frame_info = {0};
    struct aic_audio_frame *frame = NULL;
    struct mp3_audio_decoder *mp3_decoder = (struct mp3_audio_decoder *)decoder;

    if (audio_pm_get_ready_packet_num(mp3_decoder->decoder.pm) == 0) {
        return DEC_NO_READY_PACKET;
    }

    if ((mp3_decoder->decoder.fm) &&
        (audio_fm_get_empty_frame_num(mp3_decoder->decoder.fm)) == 0) {
        return DEC_NO_EMPTY_FRAME;
    }
    mp3_decoder->curr_packet = audio_pm_dequeue_ready_packet(mp3_decoder->decoder.pm);
    if (!mp3_decoder->curr_packet) {
        return DEC_NO_READY_PACKET;
    }

    frame = audio_fm_decoder_get_frame(mp3_decoder->decoder.fm);
    if (!frame) {
        audio_pm_enqueue_empty_packet(mp3_decoder->decoder.pm, mp3_decoder->curr_packet);
        return DEC_NO_EMPTY_FRAME;
    }
    samples = mp3dec_decode_frame(&mp3_decoder->mp3d,
                        mp3_decoder->curr_packet->data,
                        mp3_decoder->curr_packet->size,
                        (mp3d_sample_t *)(frame->data),
                        &frame_info);

    set_pcm_frame_info(decoder, &frame_info, frame);
    frame->size = samples * sizeof(mp3d_sample_t) * frame_info.channels;

    audio_fm_decoder_put_frame(mp3_decoder->decoder.fm, frame);
    audio_pm_enqueue_empty_packet(mp3_decoder->decoder.pm, mp3_decoder->curr_packet);

    return DEC_OK;
}

int __mp3_decode_control(struct aic_audio_decoder *decoder, int cmd, void *param)
{
    return 0;
}

int __mp3_decode_reset(struct aic_audio_decoder *decoder)
{
    struct mp3_audio_decoder *mp3_decoder = (struct mp3_audio_decoder *)decoder;

    audio_pm_reset(mp3_decoder->decoder.pm);
    audio_fm_reset(mp3_decoder->decoder.fm);
    return 0;
}


struct aic_audio_decoder_ops mp3_decoder = {
    .name           = "mp3",
    .init           = __mp3_decode_init,
    .destroy        = __mp3_decode_destroy,
    .decode         = __mp3_decode_frame,
    .control        = __mp3_decode_control,
    .reset          = __mp3_decode_reset,
};

struct aic_audio_decoder* create_mp3_decoder()
{
    struct mp3_audio_decoder *s = (struct mp3_audio_decoder*)mpp_alloc(sizeof(struct mp3_audio_decoder));
    if(s == NULL)
        return NULL;
    memset(s, 0, sizeof(struct mp3_audio_decoder));
    s->decoder.ops = &mp3_decoder;
    return &s->decoder;
}
