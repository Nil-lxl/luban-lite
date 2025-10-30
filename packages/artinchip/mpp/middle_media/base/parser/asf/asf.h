/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: advance streaming format parser, support wma and wmv formats
 */

#ifndef __ASF_H__
#define __ASF_H__

#include <unistd.h>
#include "aic_parser.h"
#include "aic_tag.h"


struct asf_stream_ctx {
    void *priv_data;
    int index;
    int id;
    int64_t nb_frames;
    int64_t duration;
    uint64_t start_time;
    uint64_t create_time;
    struct aic_codec_param codecpar;
};


#define ASF_MAX_TRACK_NUM 8
struct aic_asf_parser {
    struct aic_parser base;
    struct aic_stream *stream;
    void *priv_data;
    int nb_streams;
    struct asf_stream_ctx *streams[ASF_MAX_TRACK_NUM];
    struct asf_index_entry *cur_sample;
};

int asf_read_header(struct aic_asf_parser *s);
int asf_read_close(struct aic_asf_parser *s);
int asf_peek_packet(struct aic_asf_parser *s, struct aic_parser_packet *pkt);
int asf_seek_packet(struct aic_asf_parser *s, s64 pts);
int asf_read_packet(struct aic_asf_parser *s, struct aic_parser_packet *pkt);
#endif /* _ASF_H */
