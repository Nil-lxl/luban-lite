/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: aic asf parser, support wma and wmv formats
 */

#define LOG_TAG "asf_parse"

#include <malloc.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <inttypes.h>
#include "aic_asf_parser.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_dec_type.h"
#include "aic_stream.h"
#include "aic_parser.h"
#include "asf.h"

s32 asf_peek(struct aic_parser *parser, struct aic_parser_packet *pkt)
{
    struct aic_asf_parser *asf_parse_parser = (struct aic_asf_parser *)parser;

    return asf_peek_packet(asf_parse_parser, pkt);
}

s32 asf_read(struct aic_parser *parser, struct aic_parser_packet *pkt)
{
    struct aic_asf_parser *asf_parse_parser = (struct aic_asf_parser *)parser;

    return asf_read_packet(asf_parse_parser, pkt);
}

s32 asf_get_media_info(struct aic_parser *parser,
                       struct aic_parser_av_media_info *media)
{
    int i;
    int64_t duration = 0;
    struct aic_asf_parser *c = (struct aic_asf_parser *)parser;

    logi("================ media info =======================");
    for (i = 0; i < c->nb_streams; i++) {
        struct asf_stream_ctx *st = c->streams[i];
        if (st->codecpar.codec_type == MPP_MEDIA_TYPE_VIDEO) {
            media->has_video = 1;
            if (st->codecpar.codec_id == CODEC_ID_H264)
                media->video_stream.codec_type = MPP_CODEC_VIDEO_DECODER_H264;
            else if (st->codecpar.codec_id == CODEC_ID_MJPEG)
                media->video_stream.codec_type = MPP_CODEC_VIDEO_DECODER_MJPEG;
            else
                media->video_stream.codec_type = -1;

            media->video_stream.width = st->codecpar.width;
            media->video_stream.height = st->codecpar.height;
            if (st->codecpar.extradata_size > 0) {
                media->video_stream.extra_data_size =
                    st->codecpar.extradata_size;
                media->video_stream.extra_data = st->codecpar.extradata;
            }
            logi("video codec_type: %d codec_id %d",
                 media->video_stream.codec_type, st->codecpar.codec_id);
            logi("video width: %d, height: %d", st->codecpar.width,
                 st->codecpar.height);
            logi("video extra_data_size: %d", st->codecpar.extradata_size);
        } else if (st->codecpar.codec_type == MPP_MEDIA_TYPE_AUDIO) {
            media->has_audio = 1;
            if (st->codecpar.codec_id == CODEC_ID_WMAV1 || st->codecpar.codec_id == CODEC_ID_WMAV2)
                media->audio_stream.codec_type = MPP_CODEC_AUDIO_DECODER_WMA;
            else
                media->audio_stream.codec_type = MPP_CODEC_AUDIO_DECODER_UNKOWN;

            media->audio_stream.bits_per_sample =
                st->codecpar.bits_per_coded_sample;
            media->audio_stream.nb_channel = st->codecpar.channels;
            media->audio_stream.sample_rate = st->codecpar.sample_rate;
            media->audio_stream.bit_rate = st->codecpar.bit_rate;
            media->audio_stream.block_align = st->codecpar.block_align;
            if (st->codecpar.extradata_size > 0) {
                media->audio_stream.extra_data_size =
                    st->codecpar.extradata_size;
                media->audio_stream.extra_data = st->codecpar.extradata;
            }
            logi("audio codec_type: %d codec_id %d",
                 media->audio_stream.codec_type, st->codecpar.codec_id);
            logi("audio bits_per_sample: %d",
                 st->codecpar.bits_per_coded_sample);
            logi("audio channels: %d", st->codecpar.channels);
            logi("audio sample_rate: %d", st->codecpar.sample_rate);
            logi("audio bit_rate: %"PRId64"", st->codecpar.bit_rate);
            logi("audio block_align: %d", st->codecpar.block_align);
            logi("audio extra_data_size: %d", st->codecpar.extradata_size);
        } else {
            loge("unknown stream(%d) type: %d", i, st->codecpar.codec_type);
        }

        if (st->duration > duration)
            duration = st->duration;
    }

    media->file_size = aic_stream_size(c->stream);
    media->seek_able = 1;
    media->duration = duration;

    return 0;
}

s32 asf_seek(struct aic_parser *parser, s64 time)
{
    s32 ret = 0;
    struct aic_asf_parser *asf_parse_parser = (struct aic_asf_parser *)parser;
    ret = asf_seek_packet(asf_parse_parser, time);
    return ret;
}

s32 asf_parse_init(struct aic_parser *parser)
{
    struct aic_asf_parser *asf_parse_parser = (struct aic_asf_parser *)parser;

    if (asf_read_header(asf_parse_parser) < 0) {
        loge("asf_parse open failed");
        return -1;
    }

    return 0;
}

s32 asf_parse_destroy(struct aic_parser *parser)
{
    struct aic_asf_parser *asf_parser = (struct aic_asf_parser *)parser;
    if (parser == NULL) {
        return -1;
    }

    asf_read_close(asf_parser);
    aic_stream_close(asf_parser->stream);
    mpp_free(asf_parser);
    return 0;
}

s32 aic_asf_parser_create(unsigned char *uri, struct aic_parser **parser)
{
    s32 ret = 0;
    struct aic_asf_parser *asf_parser = NULL;

    asf_parser =
        (struct aic_asf_parser *)mpp_alloc(sizeof(struct aic_asf_parser));
    if (asf_parser == NULL) {
        loge("mpp_alloc aic_parser failed!!!!!\n");
        ret = -1;
        goto exit;
    }
    memset(asf_parser, 0, sizeof(struct aic_asf_parser));

    if (aic_stream_open((char *)uri, &asf_parser->stream, O_RDONLY) < 0) {
        loge("stream open fail");
        ret = -1;
        goto exit;
    }

    asf_parser->base.get_media_info = asf_get_media_info;
    asf_parser->base.peek = asf_peek;
    asf_parser->base.read = asf_read;
    asf_parser->base.destroy = asf_parse_destroy;
    asf_parser->base.seek = asf_seek;
    asf_parser->base.init = asf_parse_init;

    *parser = &asf_parser->base;

    return ret;

exit:
    if (asf_parser->stream) {
        aic_stream_close(asf_parser->stream);
    }
    if (asf_parser) {
        mpp_free(asf_parser);
    }
    return ret;
}
