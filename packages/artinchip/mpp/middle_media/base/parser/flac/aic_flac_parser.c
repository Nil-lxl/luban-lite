/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: aic flac parser
 */

#include <malloc.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <fcntl.h>
#include "aic_mov_parser.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_dec_type.h"
#include "aic_stream.h"
#include "flac.h"
#include "aic_flac_parser.h"

s32 flac_peek(struct aic_parser * parser, struct aic_parser_packet *pkt)
{
    struct aic_flac_parser *flac_parser = (struct aic_flac_parser *)parser;
    return flac_peek_packet(flac_parser,pkt);
}

s32 flac_read(struct aic_parser * parser, struct aic_parser_packet *pkt)
{
    struct aic_flac_parser *flac_parser = (struct aic_flac_parser *)parser;
    return flac_read_packet(flac_parser,pkt);
}

s32 flac_get_media_info(struct aic_parser *parser, struct aic_parser_av_media_info *media)
{
    int i;
    struct aic_flac_parser *c = (struct aic_flac_parser *)parser;

    media->has_video = 0;
    for (i = 0; i < c->nb_streams; i++) {
        struct flac_stream_ctx *st = c->streams[i];

        if (st->codecpar.codec_type == MPP_MEDIA_TYPE_AUDIO) {
            media->has_audio = 1;
            media->audio_stream.codec_type = MPP_CODEC_AUDIO_DECODER_FLAC;
            media->audio_stream.bits_per_sample =
                st->codecpar.bits_per_coded_sample;
            media->audio_stream.nb_channel = st->codecpar.channels;
            media->audio_stream.sample_rate = st->codecpar.sample_rate;
            if (st->codecpar.extradata_size > 0) {
                media->audio_stream.extra_data_size =
                    st->codecpar.extradata_size;
                media->audio_stream.extra_data = st->codecpar.extradata;
            }
            logi("audio bits_per_sample: %d",
                 st->codecpar.bits_per_coded_sample);
            logi("audio channels: %d", st->codecpar.channels);
            logi("audio sample_rate: %d", st->codecpar.sample_rate);
            logi("audio total_samples: %" PRId64 "", st->total_samples);
        } else {
            loge("unknown stream(%d) type: %d", i, st->codecpar.codec_type);
        }
    }
    media->file_size = c->file_size;
    return 0;
}

s32 flac_seek(struct aic_parser *parser, s64 time)
{
    struct aic_flac_parser *flac_parser = (struct aic_flac_parser *)parser;
    return flac_seek_packet(flac_parser,time);
}

s32 flac_init(struct aic_parser *parser)
{
    struct aic_flac_parser *flac_parser = (struct aic_flac_parser *)parser;

    if (flac_read_header(flac_parser)) {
        loge("flac read header failed");
        return -1;
    }
    return 0;
}

s32 flac_destroy(struct aic_parser *parser)
{
    struct aic_flac_parser *flac_parser = (struct aic_flac_parser *)parser;

    if (flac_parser == NULL) {
        return -1;
    }
    flac_close(flac_parser);
    aic_stream_close(flac_parser->stream);
    mpp_free(flac_parser);
    return 0;
}

s32 aic_flac_parser_create(unsigned char *uri, struct aic_parser **parser)
{
    s32 ret = 0;
    struct aic_flac_parser *flac_parser = NULL;

    flac_parser = (struct aic_flac_parser *)mpp_alloc(sizeof(struct aic_flac_parser));
    if (flac_parser == NULL) {
        loge("mpp_alloc aic_parser for flac failed!!!!!\n");
        ret = -1;
        goto exit;
    }
    memset(flac_parser, 0, sizeof(struct aic_flac_parser));

    if (aic_stream_open((char *)uri, &flac_parser->stream, O_RDONLY) < 0) {
        loge("stream open %s fail", uri);
        ret = -1;
        goto exit;
    }

    flac_parser->base.get_media_info = flac_get_media_info;
    flac_parser->base.peek = flac_peek;
    flac_parser->base.read = flac_read;
    flac_parser->base.destroy = flac_destroy;
    flac_parser->base.seek = flac_seek;
    flac_parser->base.init = flac_init;

    *parser = &flac_parser->base;

    return ret;

exit:
    if (flac_parser->stream) {
        aic_stream_close(flac_parser->stream);
    }
    if (flac_parser) {
        mpp_free(flac_parser);
    }
    return ret;
}
