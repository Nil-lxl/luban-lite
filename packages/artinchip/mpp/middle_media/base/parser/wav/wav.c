/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: wav parser
 */

#include <stdlib.h>
#include <inttypes.h>
#include "aic_stream.h"
#include "wav.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include <rtthread.h>

#ifdef RT_AUDIO_REPLAY_MP_BLOCK_SIZE
    #define WAVE_FRAME_SIZE RT_AUDIO_REPLAY_MP_BLOCK_SIZE
#else
    #define WAVE_FRAME_SIZE (2*1024)
#endif

int wav_read_header(struct aic_wav_parser *s)
{
    int ret = 0;
    int r_len = 0;
    uint64_t data_size,data_size_per_sec, total_size;
    uint32_t chunk_size;
    char chunk_id[4] = {0};

    r_len = aic_stream_read(s->stream, &(s->wav_info.header), sizeof(struct wave_header));
    if (r_len != sizeof(struct wave_header)) {
        loge("read wave_header fail");
        ret = -1;
        goto _exit;
    }

    // search for fmt block
    total_size = 0;
    do {
        aic_stream_read(s->stream, chunk_id, 4);
        total_size += 4;
        if (0 == strncmp(chunk_id, "fmt", 3)) {
            chunk_size = aic_stream_rl32(s->stream);
            s->wav_info.fmt_block.fmt_size = chunk_size;
            r_len = aic_stream_read(s->stream, &(s->wav_info.fmt_block.wav_format),
                                    sizeof(struct wave_format));
            if (r_len != sizeof(struct wave_format)) {
                loge("read wave_format fail");
                ret = -1;
                goto _exit;
            }
            break;
        } else {
            chunk_size = aic_stream_rl32(s->stream);
            aic_stream_skip(s->stream, chunk_size);
            total_size += (4 + chunk_size);
            if (total_size >= aic_stream_size(s->stream)) {
                loge("read invalid wav file\n");
                ret = -1;
                goto _exit;
            }
        }
    } while(1);

    // search for data block
    do {
        int len = 0;
        len = aic_stream_read(s->stream, &(s->wav_info.data_block), sizeof(struct wave_data));
        if (len != sizeof(struct wave_data)) {
            ret = -1;
            goto _exit;
        } else if (strncmp(s->wav_info.data_block.data_id, "data", 4)) {
            aic_stream_seek(s->stream, s->wav_info.data_block.data_size, SEEK_CUR);
        } else {
            break;
        }
    } while(1);

    s->bits_per_sample = s->wav_info.fmt_block.wav_format.bits_per_sample;
    s->channels = s->wav_info.fmt_block.wav_format.channels;
    s->sample_rate = s->wav_info.fmt_block.wav_format.sample_rate;

    s->first_packet_pos = aic_stream_tell(s->stream);

    s->file_size = aic_stream_size(s->stream);
    s->stream_size = s->wav_info.data_block.data_size;

    data_size = s->stream_size;

    data_size_per_sec = s->sample_rate * (s->bits_per_sample / 8) * s->channels;

    s->data_size_per_sec = data_size_per_sec;

    s->duration = (data_size*1000*1000)/data_size_per_sec;

    s->frame_size = WAVE_FRAME_SIZE;

    s->frame_duration = (s->frame_size*1000*1000)/data_size_per_sec;

_exit:
    return ret;
}

int wav_close(struct aic_wav_parser *c)
{
    return 0;
}

int wav_seek_packet(struct aic_wav_parser *s, s64 seek_time)
{
    int ret = 0;
    uint64_t tm = seek_time/1000/1000;
    uint64_t pos = 0;

    pos = s->data_size_per_sec * tm;
    pos += s->first_packet_pos;
    if(pos > s->first_packet_pos + s->stream_size) {
        loge("seek too long seek_pos%"PRIu64"file_size%"PRIu64"\n",pos,s->file_size);
        return -1;
    }
    ret = (int)aic_stream_seek(s->stream, pos, SEEK_SET);
    return ret;
}

int wav_peek_packet(struct aic_wav_parser *s, struct aic_parser_packet *pkt)
{
    int64_t pos;

    pos = aic_stream_tell(s->stream);
    if (pos >= s->first_packet_pos + s->stream_size) {
        printf("[%s:%d]PARSER_EOS,%"PRId64",%"PRId64"\n",__FUNCTION__,__LINE__,pos,s->file_size);
        return PARSER_EOS;
    }
    pkt->size =  s->frame_size;
    if (s->first_packet_pos + s->stream_size - pos < s->frame_size) {
        pkt->size = s->first_packet_pos + s->stream_size - pos;
    }

    pkt->pts = s->frame_id * s->frame_duration;
    pkt->type = MPP_MEDIA_TYPE_AUDIO;
    return 0;
}

int wav_read_packet(struct aic_wav_parser *s, struct aic_parser_packet *pkt)
{
    int64_t pos;
    int ret;

    pos = aic_stream_tell(s->stream);
    if (pos >= s->first_packet_pos + s->stream_size) {
        printf("[%s:%d]PARSER_EOS,%"PRId64",%"PRId64"\n",__FUNCTION__,__LINE__,pos,s->file_size);
        return PARSER_EOS;
    }

    ret = aic_stream_read(s->stream,pkt->data,pkt->size);

    pos = aic_stream_tell(s->stream);
    if (pos >= s->first_packet_pos + s->stream_size) {
        pkt->flag |= PACKET_EOS;
    } else {
        if(ret != pkt->size) {
            loge("hope_len:%d,ret:%d\n",pkt->size,ret);
        }
    }
    s->frame_id++;
    return 0;
}
