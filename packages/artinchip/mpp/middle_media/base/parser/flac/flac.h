/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: flac parser
 */

#ifndef __FLAC_H__
#define __FLAC_H__

#include "aic_parser.h"
#include <unistd.h>
#include "aic_tag.h"

struct flac_stream_ctx {
    int index;
    uint64_t total_samples;
    struct aic_codec_param codecpar;
};

#define FLAC_MAX_TRACK_NUM 1

struct aic_flac_parser {
    struct aic_parser base;
    struct aic_stream *stream;
    uint64_t file_size;
    int nb_streams;
    struct flac_stream_ctx *streams[FLAC_MAX_TRACK_NUM];
};

int flac_read_header(struct aic_flac_parser *s);
int flac_close(struct aic_flac_parser *s);
int flac_peek_packet(struct aic_flac_parser *s, struct aic_parser_packet *pkt);
int flac_seek_packet(struct aic_flac_parser *s, s64 seek_time);
int flac_read_packet(struct aic_flac_parser *s, struct aic_parser_packet *pkt);
#endif
