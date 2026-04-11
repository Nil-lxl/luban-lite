/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: wma decoder interface
 */

#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "mpp_dec_type.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"

#include "audio_decoder.h"
#include "avcodec.h"

#define WMA_BLOCK_MAX_SIZE (1 << 11)

struct wma_audio_decoder {
    struct aic_audio_decoder decoder;
    uint32_t frame_id;
    int frame_count;
    AVCodec *codec;
    AVCodecContext *actx;
};

static int __wma_decode_init_params(struct aic_audio_decoder *decoder,
                                    struct aic_audio_decode_params *params)
{
    int ret = 0;
    struct audio_frame_manager_cfg cfg;
    struct wma_audio_decoder *wma_decoder = (struct wma_audio_decoder *)decoder;

    if (!wma_decoder)
        return DEC_ERR_NOT_CREATE;

    if (!wma_decoder->actx || !wma_decoder->codec || !params || !params->extradata)
        return DEC_ERR_NULL_PTR;

    if (params->channels <= 0 || params->sample_rate <= 0 ||
        params->bit_rate <= 0 || params->block_align <= 0 ||
        params->bits_per_sample <= 0 || params->extradata_size <= 0) {
        loge("wrong params: channels=%d, sample_rate=%d, bit_rate=%d,"
             "block_align=%d, bits_per_sample=%d, extradata_size=%d.\n",
             params->channels, params->sample_rate,
             params->bit_rate, params->block_align,
             params->bits_per_sample, params->extradata_size);
        return DEC_ERR_NOT_SUPPORT;
    }

    wma_decoder->actx->channels = params->channels;
    wma_decoder->actx->sample_rate = params->sample_rate;
    wma_decoder->actx->bit_rate = params->bit_rate;
    wma_decoder->actx->block_align = params->block_align;
    wma_decoder->actx->bits_per_sample = params->bits_per_sample;
    wma_decoder->actx->extradata_size = params->extradata_size;
    wma_decoder->actx->extradata = params->extradata;

    ret = avcodec_open(wma_decoder->actx, wma_decoder->codec);
    if (ret) {
        loge("avcodec_open failed %d.\n", ret);
        return ret;
    }

    cfg.bits_per_sample = wma_decoder->actx->bits_per_sample;
    cfg.samples_per_frame = WMA_BLOCK_MAX_SIZE * params->channels;
    cfg.frame_count = wma_decoder->frame_count;
    wma_decoder->decoder.fm = audio_fm_create(&cfg);
    if (wma_decoder->decoder.fm == NULL) {
        loge("audio_fm_create fail!!!\n");
        return DEC_ERR_NULL_PTR;
    }
    return DEC_OK;
}

int __wma_decode_init(struct aic_audio_decoder *decoder, struct aic_audio_decode_config *config)
{
    struct wma_audio_decoder *wma_decoder = (struct wma_audio_decoder *)decoder;

    if (!wma_decoder)
        return DEC_ERR_NOT_CREATE;

    if (!config)
        return DEC_ERR_NULL_PTR;

    if (config->frame_count <= 0)
        return DEC_ERR_NOT_SUPPORT;

    wma_decoder->decoder.pm = audio_pm_create(config);
    wma_decoder->frame_count = config->frame_count;
    wma_decoder->frame_id = 0;

    avcodec_register_all();

    wma_decoder->actx = avcodec_alloc_context();
    if (!wma_decoder->actx) {
        loge("avcodec_alloc_context failed.\n");
        return DEC_ERR_NULL_PTR;
    }

    wma_decoder->codec = avcodec_find_decoder(AV_CODEC_ID_WMAV2);
    if (!wma_decoder->codec) {
        loge("avcodec_find_decoder %d failed\n", AV_CODEC_ID_WMAV2);
        return DEC_ERR_NULL_PTR;
    }

    return DEC_OK;
}

int __wma_decode_destroy(struct aic_audio_decoder *decoder)
{
    struct wma_audio_decoder *wma_decoder = (struct wma_audio_decoder *)decoder;

    if (!wma_decoder)
        return DEC_ERR_NOT_CREATE;

    if (wma_decoder->actx) {
        avcodec_free_context(&wma_decoder->actx);
        wma_decoder->codec = NULL;
    }

    if (wma_decoder->decoder.pm) {
        audio_pm_destroy(wma_decoder->decoder.pm);
        wma_decoder->decoder.pm = NULL;
    }

    if (wma_decoder->decoder.fm) {
        audio_fm_destroy(wma_decoder->decoder.fm);
        wma_decoder->decoder.fm = NULL;
    }

    mpp_free(wma_decoder);
    return DEC_OK;
}

int __wma_decode_frame(struct aic_audio_decoder *decoder)
{
    int ret, got_frame = 0;
    struct aic_audio_frame *frame = NULL;
    struct mpp_packet *packet = NULL;
    AVFrame av_frame = {0};
    struct AVPacket avpkt = {0};
    struct wma_audio_decoder *wma_decoder = (struct wma_audio_decoder *)decoder;

    if (!wma_decoder)
        return DEC_ERR_NOT_CREATE;

    if (!wma_decoder->actx)
        return DEC_ERR_NULL_PTR;

    if (audio_pm_get_ready_packet_num(wma_decoder->decoder.pm) == 0)
        return DEC_NO_READY_PACKET;

    if ((wma_decoder->decoder.fm) &&
        (audio_fm_get_empty_frame_num(wma_decoder->decoder.fm)) == 0) {
        return DEC_NO_EMPTY_FRAME;
    }

    packet = audio_pm_dequeue_ready_packet(wma_decoder->decoder.pm);
    if (!packet)
        return DEC_NO_READY_PACKET;

    frame = audio_fm_decoder_get_frame(wma_decoder->decoder.fm);
    if (!frame)
        return DEC_NO_EMPTY_FRAME;

    av_frame.data[0] = frame->data;
    av_frame.frame_size = 0;
    avpkt.data = packet->data;
    avpkt.size = packet->size;
    avpkt.flags = packet->flag;

    ret = avcodec_decode_audio(wma_decoder->actx, &av_frame, &got_frame, &avpkt);
    if (ret < 0) {
        logd("avcodec_decode_audio failed %d.\n", ret);
        audio_pm_enqueue_empty_packet(wma_decoder->decoder.pm, packet);
        return ret;
    }

    audio_pm_enqueue_empty_packet(wma_decoder->decoder.pm, packet);
    if (!got_frame)
        return DEC_NO_RENDER_FRAME;

    frame->size = av_frame.frame_size;
    frame->flag = packet->flag;
    frame->channels = wma_decoder->actx->channels;
    frame->sample_rate = wma_decoder->actx->sample_rate;
    frame->bits_per_sample = wma_decoder->actx->bits_per_sample;
    frame->channels = wma_decoder->actx->channels;
    frame->pts = packet->pts;
    frame->id = wma_decoder->frame_id++;
    audio_fm_decoder_put_frame(wma_decoder->decoder.fm, frame);

    return DEC_OK;
}

int __wma_decode_control(struct aic_audio_decoder *decoder, int cmd, void *param)
{
    int ret = DEC_OK;

    if (!decoder)
        return DEC_ERR_NOT_CREATE;

    switch (cmd) {
        case MPP_DEC_INIT_CMD_SET_PARAMS:
            ret = __wma_decode_init_params(decoder, (struct aic_audio_decode_params *)param);
            break;
        default:
            loge("Unsupport cmd %d\n", cmd);
            return DEC_ERR_NOT_SUPPORT;
    }

    return ret;
}

int __wma_decode_reset(struct aic_audio_decoder *decoder)
{
    struct wma_audio_decoder *wma_decoder = (struct wma_audio_decoder *)decoder;

    if (!decoder)
        return DEC_ERR_NOT_CREATE;

    audio_pm_reset(wma_decoder->decoder.pm);
    audio_fm_reset(wma_decoder->decoder.fm);
    return DEC_OK;
}

struct aic_audio_decoder_ops wma_decoder = {
    .name = "wma",
    .init = __wma_decode_init,
    .destroy = __wma_decode_destroy,
    .decode = __wma_decode_frame,
    .control = __wma_decode_control,
    .reset = __wma_decode_reset,
};

struct aic_audio_decoder *create_wma_decoder()
{
    struct wma_audio_decoder *s =
        (struct wma_audio_decoder *)mpp_alloc(sizeof(struct wma_audio_decoder));

    if (s == NULL)
        return NULL;

    memset(s, 0, sizeof(struct wma_audio_decoder));
    s->decoder.ops = &wma_decoder;
    return &s->decoder;
}
