/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <xiaodong.zhao@artinchip.com>
 * Desc: aic ape parser
 */

#include <malloc.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include "aic_mov_parser.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_dec_type.h"
#include "aic_stream.h"
#include "aic_ape_parser.h"
// #include "ape.h"
#include "parser.h"

struct aic_ape_parser {
    struct aic_parser base;
    struct aic_stream *stream;

    struct ape_ctx_t ape_ctx;
    s64 file_size;
    s64 current_pos;
    unsigned first_packet_pos;
    s64 duration;//us
    uint32_t frame_id;
};

s32 ape_peek(struct aic_parser *parser, struct aic_parser_packet *pkt)
{
    s32 ret = 0;
    s64 cur_pos = 0;
    struct aic_ape_parser *ape_parser = (struct aic_ape_parser *)parser;

    cur_pos = aic_stream_tell(ape_parser->stream);
    if (cur_pos >= ape_parser->file_size) {
        ret = PARSER_EOS;
    }

    pkt->size = 1024;
    pkt->pts = 0;
    pkt->dts = 0;
    pkt->duration = (1152 * 1000000) / 44100; // 1152 samples per frame
    pkt->type = MPP_MEDIA_TYPE_AUDIO;
    if (cur_pos >= ape_parser->file_size)
        pkt->flag = PACKET_FLAG_EOS;
    else
        pkt->flag = 0;

    return ret;
}

s32 ape_read(struct aic_parser * parser, struct aic_parser_packet *pkt)
{
    struct aic_ape_parser *ape_parser = (struct aic_ape_parser *)parser;
    s64 bytes_read;

    // Read the full frame
    bytes_read = aic_stream_read(ape_parser->stream, pkt->data, pkt->size);
    if (bytes_read < 0) {
        loge("parser error\n");
        return PARSER_ERROR;
    } else if (bytes_read < pkt->size) {
        logi("file read over!");
        pkt->size = bytes_read;
    }

    if (aic_stream_tell(ape_parser->stream) >= ape_parser->file_size) {
        pkt->flag = PACKET_FLAG_EOS;
    }

    ape_parser->current_pos += bytes_read;

    return PARSER_OK;
}

s32 ape_get_media_info(struct aic_parser *parser, struct aic_parser_av_media_info *media)
{
    struct aic_ape_parser *ape_parser = (struct aic_ape_parser *)parser;
    media->has_video = 0;
    media->has_audio = 1;
    media->file_size = ape_parser->file_size;
    media->duration = ape_parser->duration;    // us
    media->audio_stream.codec_type = MPP_CODEC_AUDIO_DECODER_APE;
    media->audio_stream.bits_per_sample = 16;
    media->audio_stream.nb_channel = ape_parser->ape_ctx.channels;
    media->audio_stream.sample_rate = ape_parser->ape_ctx.samplerate;

    return 0;
}

s32 ape_seek(struct aic_parser *parser, s64 time)
{
    return 0;
}

s32 ape_init(struct aic_parser *parser)
{
    int ret;
    unsigned char *buf = NULL;
    struct aic_ape_parser *ape_parser = (struct aic_ape_parser *)parser;

    buf = (unsigned char *)mpp_alloc(1024);
    if (NULL == buf) {
        loge("mpp_alloc ape_init failed!!!!!\n");
        return -1;
    }
    ape_parser->file_size = aic_stream_size(ape_parser->stream);

    ret = aic_stream_read(ape_parser->stream, buf, 1024);
    if (ret <= 0) {
        loge("aic_stream_read failed!!!!!\n");
        mpp_free(buf);
        return -1;
    }

    ret = ape_parseheaderbuf(&ape_parser->ape_ctx, buf, ret);
    if (ret < 0) {
        loge("ape parser header error!\n");
    }

    if (ape_parser->ape_ctx.samplerate) {
        ape_parser->duration = ape_parser->ape_ctx.totalsamples;
        ape_parser->duration *= 1000000;
        ape_parser->duration /= ape_parser->ape_ctx.samplerate;
    }

    aic_stream_seek(ape_parser->stream, 0, SEEK_SET);
    mpp_free(buf);

    return 0;
}

s32 ape_destroy(struct aic_parser *parser)
{
    struct aic_ape_parser *ape_parser = (struct aic_ape_parser *)parser;

    if (ape_parser == NULL) {
        return -1;
    }

    aic_stream_close(ape_parser->stream);
    mpp_free(ape_parser);
    return 0;
}

s32 aic_ape_parser_create(unsigned char *uri, struct aic_parser **parser)
{
    s32 ret = 0;
    struct aic_ape_parser *ape_parser = NULL;

    ape_parser = (struct aic_ape_parser *)mpp_alloc(sizeof(struct aic_ape_parser));
    if (ape_parser == NULL) {
        loge("mpp_alloc aic_parser failed!!!!!\n");
        ret = -1;
        goto exit;
    }
    memset(ape_parser, 0, sizeof(struct aic_ape_parser));

    if (aic_stream_open((char *)uri, &ape_parser->stream, O_RDONLY) < 0) {
        loge("stream open fail");
        ret = -1;
        goto exit;
    }

    ape_parser->base.get_media_info = ape_get_media_info;
    ape_parser->base.peek = ape_peek;
    ape_parser->base.read = ape_read;
    ape_parser->base.destroy = ape_destroy;
    ape_parser->base.seek = ape_seek;
    ape_parser->base.init = ape_init;

    *parser = &ape_parser->base;

    return ret;

exit:
    if (ape_parser->stream) {
        aic_stream_close(ape_parser->stream);
    }
    if (ape_parser) {
        mpp_free(ape_parser);
    }
    return ret;
}
