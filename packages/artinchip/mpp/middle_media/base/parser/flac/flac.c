/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: flac parser
 */

#include "flac.h"
#include "aic_stream.h"
#include "aic_tag.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <inttypes.h>
#include <rtthread.h>
#include <stdlib.h>

#define SEEKPOINT_SIZE         18
#define FLAC_STREAMINFO_SIZE   34
#define FLAC_MAX_CHANNELS       8
#define FLAC_MIN_BLOCKSIZE     16
#define FLAC_MAX_BLOCKSIZE  65535
#define FLAC_MIN_FRAME_SIZE    11

#define RAW_PACKET_SIZE     1024

enum {
    FLAC_CHMODE_INDEPENDENT = 0,
    FLAC_CHMODE_LEFT_SIDE   = 1,
    FLAC_CHMODE_RIGHT_SIDE  = 2,
    FLAC_CHMODE_MID_SIDE    = 3,
};

enum {
    FLAC_METADATA_TYPE_STREAMINFO = 0,
    FLAC_METADATA_TYPE_PADDING,
    FLAC_METADATA_TYPE_APPLICATION,
    FLAC_METADATA_TYPE_SEEKTABLE,
    FLAC_METADATA_TYPE_VORBIS_COMMENT,
    FLAC_METADATA_TYPE_CUESHEET,
    FLAC_METADATA_TYPE_PICTURE,
    FLAC_METADATA_TYPE_INVALID = 127
};

static struct flac_stream_ctx *flac_new_stream(struct aic_flac_parser *s)
{
    struct flac_stream_ctx *sc;

    sc = (struct flac_stream_ctx *)mpp_alloc(sizeof(struct flac_stream_ctx));
    if (sc == NULL) {
        return NULL;
    }
    memset(sc, 0, sizeof(struct flac_stream_ctx));

    sc->index = s->nb_streams;
    s->streams[s->nb_streams++] = sc;

    return sc;
}

static inline void flac_parse_block_header(const uint8_t *block_header,
                                          int *last, int *type, int *size)
{
    int tmp = block_header[0];

    if (last)
        *last = tmp & 0x80;
    if (type)
        *type = tmp & 0x7F;
    if (size)
        *size = AIC_RB24(block_header + 1);
}


int flac_read_header(struct aic_flac_parser *s)
{
    int ret, metadata_last = 0, metadata_type, metadata_size, found_streaminfo = 0;
    uint8_t header[4];
    uint8_t *buffer = NULL;
    struct flac_stream_ctx *st = flac_new_stream(s);
    if (!st)
        return PARSER_NOMEM;

    st->codecpar.codec_type = MPP_MEDIA_TYPE_AUDIO;
    st->codecpar.codec_id = CODEC_ID_FLAC;
    s->file_size = aic_stream_size(s->stream);

    /* the parameters will be extracted from the compressed bitstream */

    /* if fLaC marker is not found, assume there is no header */
    if (aic_stream_rl32(s->stream) != MKTAG('f', 'L', 'a', 'C')) {
        aic_stream_seek(s->stream, -4, SEEK_CUR);
        return PARSER_OK;
    }

    /* process metadata blocks */
    while ((aic_stream_tell(s->stream) < s->file_size) && !metadata_last) {
        if (aic_stream_read(s->stream, header, 4) != 4)
            return PARSER_INVALIDDATA;

        flac_parse_block_header(header, &metadata_last, &metadata_type,
                                &metadata_size);
        switch (metadata_type) {
        /* allocate and read metadata block for supported types */
        case FLAC_METADATA_TYPE_STREAMINFO:
            buffer = mpp_alloc(metadata_size + 64);
            if (!buffer) {
                return PARSER_NOMEM;
            }
            if (aic_stream_read(s->stream, buffer, metadata_size) != metadata_size) {
                return PARSER_NODATA;
            }
            break;
        /* skip metadata block for unsupported types */
        default:
            ret = aic_stream_skip(s->stream, metadata_size);
            if (ret < 0)
                return ret;
        }

        if (metadata_type == FLAC_METADATA_TYPE_STREAMINFO) {
            /* STREAMINFO can only occur once */
            if (found_streaminfo) {
                return PARSER_INVALIDDATA;
            }
            if (metadata_size != FLAC_STREAMINFO_SIZE) {
                return PARSER_INVALIDDATA;
            }
            found_streaminfo = 1;
            st->codecpar.sample_rate = AIC_RB24(buffer + 10) >> 4;
            st->codecpar.channels = ((*(buffer + 12) >> 1) & 0x7) + 1;
            st->codecpar.bits_per_coded_sample = ((AIC_RB16(buffer + 12) >> 4) & 0x1f) + 1;
            st->total_samples = (AIC_RB64(buffer + 13) >> 24) & ((1ULL << 36) - 1);
            mpp_free(buffer);
        }
    }

    aic_stream_seek(s->stream, 0, SEEK_SET);

    return PARSER_OK;
}

int flac_close(struct aic_flac_parser *s)
{
    int i;

    for (i = 0; i < s->nb_streams; i++) {
        struct flac_stream_ctx *st = s->streams[i];
        if (!st) {
            continue;
        }

        mpp_free(st);
    }

    return PARSER_OK;
}

int flac_seek_packet(struct aic_flac_parser *s, s64 seek_time)
{
    return PARSER_ERROR;
}

int flac_peek_packet(struct aic_flac_parser *s, struct aic_parser_packet *pkt)
{
    int64_t pos;
    static int64_t count = 0;

    pos = aic_stream_tell(s->stream);
    if (pos >= s->file_size) {
        logd("Peek PARSER_EOS,%" PRId64 ",%" PRId64 "\n", pos, s->file_size);
        return PARSER_EOS;
    }
    if (pos + RAW_PACKET_SIZE >= s->file_size)
        pkt->size = s->file_size - pos;
    else
        pkt->size = RAW_PACKET_SIZE;
    pkt->type = MPP_MEDIA_TYPE_AUDIO;
    pkt->pts = count++;
    return PARSER_OK;
}

int flac_read_packet(struct aic_flac_parser *s, struct aic_parser_packet *pkt)
{
    int64_t pos;
    int ret;

    pos = aic_stream_tell(s->stream);
    if (pos >= s->file_size) {
        loge("PARSER_EOS,%" PRId64 ",%" PRId64 "\n",  pos, s->file_size);
        return PARSER_EOS;
    }

    ret = aic_stream_read(s->stream, pkt->data, pkt->size);
    pos = aic_stream_tell(s->stream);
    if (pos >= s->file_size) {
        printf("Read PARSER_EOS,%" PRId64 ",%" PRId64 "\n", pos, s->file_size);
        pkt->flag |= PACKET_EOS;
    } else {
        if (ret != pkt->size) {
            loge("hope_len:%d,ret:%d\n", pkt->size, ret);
        }
    }
    return PARSER_OK;
}
