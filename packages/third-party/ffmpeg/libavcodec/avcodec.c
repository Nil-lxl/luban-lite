/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: Encapsulate different codec interfaces
 */
#include "avcodec.h"
#include <float.h>
#include <limits.h>

#ifdef LPKG_FFMPEG_MEM_USING_CMA
#include "aic_osal.h"
#endif

AVCodec *default_avcodec = NULL;

void avcodec_register(AVCodec *format)
{
    AVCodec **p;
    p = &default_avcodec;
    while (*p != NULL)
        p = &(*p)->next;
    *p = format;
    format->next = NULL;
}

AVCodecContext *avcodec_alloc_context(void)
{
    AVCodecContext *avctx = av_malloc(sizeof(AVCodecContext));

    if (avctx == NULL)
        return NULL;

    memset(avctx, 0, sizeof(AVCodecContext));

    return avctx;
}

void avcodec_free_context(AVCodecContext **avctx)
{
    if (!avctx)
        return;

    avcodec_close(*avctx);

    av_freep(avctx);
}

int avcodec_open(AVCodecContext *avctx, AVCodec *codec)
{
    int ret = -1;

    if (avctx->codec)
        goto end;

    if (codec->priv_data_size > 0) {
#ifdef LPKG_FFMPEG_MEM_USING_CMA
        avctx->priv_data = aicos_malloc(MEM_CMA, codec->priv_data_size);
#else
        avctx->priv_data = av_mallocz(codec->priv_data_size);
#endif
        if (!avctx->priv_data)
            goto end;
    } else {
        avctx->priv_data = NULL;
    }

    avctx->codec = codec;
    avctx->codec_id = codec->id;
    avctx->frame_number = 0;
    ret = avctx->codec->init(avctx);
    if (ret < 0) {
#ifdef LPKG_FFMPEG_MEM_USING_CMA
        aicos_free(MEM_CMA, avctx->priv_data);
#else
        av_freep(&avctx->priv_data);
#endif
        avctx->codec = NULL;
        goto end;
    }
    ret = 0;
end:
    return ret;
}


int avcodec_decode_audio(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, struct AVPacket *avpkt)
{
    int ret = 0;

    if (got_frame_ptr) {
        ret = avctx->codec->decode(avctx, (void*)frame, got_frame_ptr, avpkt);
        avctx->frame_number++;
    }

    return ret;
}

int avcodec_encode_audio(AVCodecContext *avctx, int16_t *samples, int *frame_size_ptr, uint8_t *buf, int buf_size)
{
    return 0;
}

int avcodec_close(AVCodecContext *avctx)
{
    if (!avctx->codec)
        return -1;

    if (avctx->codec->close)
        avctx->codec->close(avctx);
#ifdef LPKG_FFMPEG_MEM_USING_CMA
    aicos_free(MEM_CMA, avctx->priv_data);
#else
    av_freep(&avctx->priv_data);
#endif
    avctx->codec = NULL;
    return 0;
}

AVCodec *avcodec_find_decoder(enum AVCodecID id)
{
    AVCodec *p;
    p = default_avcodec;
    while (p) {
        if (p->decode != NULL && p->id == id)
            return p;
        p = p->next;
    }
    return NULL;
}


void avcodec_flush_buffers(AVCodecContext *avctx)
{
    if (avctx->codec->flush)
        avctx->codec->flush(avctx);
}
