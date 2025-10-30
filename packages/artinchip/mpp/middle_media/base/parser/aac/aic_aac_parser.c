/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: qi.xu@artinchip.com
 * Desc: aac parser
 */

#include <malloc.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_dec_type.h"
#include "aic_stream.h"
#include "aic_aac_parser.h"

#define AAC_ADTS_HEADER_SIZE 7
#define AAC_ADTS_HEADER_SIZE_WITH_CRC 9
#define AAC_MAX_FRAME_SIZE (1024 * 6) // 6KB should be enough for most AAC frames

struct aic_aac_parser {
    struct aic_parser base;
    struct aic_stream* stream;
    s64 file_size;
    s64 current_pos;
    s32 has_crc;
    struct aic_parser_av_media_info media_info;
};

// ADTS header structure
typedef struct {
    u16 syncword;            // 12 bits
    u8 id;                   // 1 bit
    u8 layer;                // 2 bits
    u8 protection_absent;    // 1 bit
    u8 profile;              // 2 bits
    u8 sampling_freq_index;  // 4 bits
    u8 private_bit;          // 1 bit
    u8 channel_config;       // 3 bits
    u8 originality;          // 1 bit
    u8 home;                 // 1 bit
    u8 copyrighted_id_bit;   // 1 bit
    u8 copyrighted_id_start; // 1 bit
    u16 frame_length;        // 13 bits
    u16 adts_buffer_fullness;// 11 bits
    u8 num_raw_data_blocks;  // 2 bits
} ADTSHeader;

static const int sampling_freq_table[] = {
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};

static int adts_aac_resync(struct aic_aac_parser *parser)
{
    uint16_t state;
    u8 buf;

    // skip data until an ADTS frame is found
    aic_stream_read(parser->stream, &state, 1);
    while (aic_stream_tell(parser->stream) < parser->file_size) {
        aic_stream_read(parser->stream, &buf, 1);
        state = (state << 8) | buf;
        //loge("state: 0x%x, 0x%llx", state, aic_stream_tell(parser->stream));
        if ((state >> 4) != 0xFFF)
            continue;
        aic_stream_seek(parser->stream, -2, SEEK_CUR);
        break;
    }

    return 0;
}

static s32 parse_adts_header(struct aic_aac_parser *parser, ADTSHeader *header, u8 *data)
{
    // Parse ADTS header
    header->syncword = (data[0] << 4) | (data[1] >> 4);
    if (header->syncword != 0xFFF) {
        loge("Invalid ADTS syncword: 0x%x, tell: 0x%llx", header->syncword, aic_stream_tell(parser->stream));
        return PARSER_INVALIDDATA;
    }

    header->id = (data[1] >> 3) & 0x01;
    header->layer = (data[1] >> 1) & 0x03;
    header->protection_absent = data[1] & 0x01;

    header->profile = (data[2] >> 6) & 0x03;
    header->sampling_freq_index = (data[2] >> 2) & 0x0F;
    header->private_bit = (data[2] >> 1) & 0x01;
    header->channel_config = ((data[2] & 0x01) << 2) | (data[3] >> 6);
    header->originality = (data[3] >> 5) & 0x01;
    header->home = (data[3] >> 4) & 0x01;
    header->copyrighted_id_bit = (data[3] >> 3) & 0x01;
    header->copyrighted_id_start = (data[3] >> 2) & 0x01;
    header->frame_length = ((data[3] & 0x03) << 11) | (data[4] << 3) | (data[5] >> 5);
    header->adts_buffer_fullness = ((data[5] & 0x1F) << 6) | (data[6] >> 2);
    header->num_raw_data_blocks = data[6] & 0x03;

    parser->has_crc = !header->protection_absent;

    return PARSER_OK;
}

s32 aac_peek(struct aic_parser *parser, struct aic_parser_packet *pkt)
{
    struct aic_aac_parser *aac_parser = (struct aic_aac_parser *)parser;
    u8 header_buf[AAC_ADTS_HEADER_SIZE_WITH_CRC];
    ADTSHeader header;
    s32 ret;
    s64 bytes_read;
    int cur_pos = 0;

    adts_aac_resync(aac_parser);

    cur_pos = aic_stream_tell(aac_parser->stream);
    if (cur_pos >= aac_parser->file_size) {
        return PARSER_EOS;
    }
    // Read ADTS header
    bytes_read = aic_stream_read(aac_parser->stream, header_buf, AAC_ADTS_HEADER_SIZE);
    if (bytes_read != AAC_ADTS_HEADER_SIZE) {
        if (bytes_read == 0) {
            pkt->flag |= PACKET_EOS;
            return PARSER_EOS;
        }
        return PARSER_ERROR;
    }

    // Parse header
    ret = parse_adts_header(aac_parser, &header, header_buf);
    if (ret != PARSER_OK) {
        return ret;
    }

    aic_stream_seek(aac_parser->stream, cur_pos, SEEK_SET);
    // Fill packet info
    pkt->size = header.frame_length;//- AAC_ADTS_HEADER_SIZE;
    pkt->pts = 0; // AAC doesn't have PTS in the stream, need to calculate based on frame count
    pkt->dts = 0;
    pkt->duration = (1024 * 1000000) / sampling_freq_table[header.sampling_freq_index]; // 1024 samples per frame
    pkt->type = MPP_MEDIA_TYPE_AUDIO;
    if ((cur_pos + header.frame_length) >= aac_parser->file_size)
        pkt->flag = PACKET_FLAG_EOS;
    else
        pkt->flag = 0;
    logi("peek packet, size: %d, tell: %llx", pkt->size, aic_stream_tell(aac_parser->stream));

    return PARSER_OK;
}

s32 aac_read(struct aic_parser *parser, struct aic_parser_packet *pkt)
{
    struct aic_aac_parser *aac_parser = (struct aic_aac_parser *)parser;
    s64 bytes_read;

    // Read the full frame
    bytes_read = aic_stream_read(aac_parser->stream, pkt->data, pkt->size);
    if (bytes_read != pkt->size) {
        return PARSER_ERROR;
    }

    if (aic_stream_tell(aac_parser->stream) >= aac_parser->file_size)
        pkt->flag = PACKET_FLAG_EOS;

    aac_parser->current_pos += pkt->size;

    return PARSER_OK;
}

s32 aac_get_media_info(struct aic_parser *parser, struct aic_parser_av_media_info *media)
{
    struct aic_aac_parser *aac_parser = (struct aic_aac_parser *)parser;

    memcpy(media, &aac_parser->media_info, sizeof(*media));
    media->has_video = 0;
    media->has_audio = 1;
    media->audio_stream.bits_per_sample = aac_parser->media_info.audio_stream.bits_per_sample;

    return PARSER_OK;
}

s32 aac_seek(struct aic_parser *parser, s64 time)
{
    logi("do not support seek");
    return 0;
}

s32 aac_init(struct aic_parser *parser)
{
    struct aic_aac_parser *aac_parser = (struct aic_aac_parser *)parser;
    u8 header_buf[AAC_ADTS_HEADER_SIZE];
    ADTSHeader header;
    s64 current_pos;
    s32 ret;

    // Get file size
    aac_parser->file_size = aic_stream_size(aac_parser->stream);
    aac_parser->current_pos = 0;

    // Save current position
    current_pos = aic_stream_tell(aac_parser->stream);

    // Read first frame header
    aic_stream_seek(aac_parser->stream, 0, SEEK_SET);
    if (aic_stream_read(aac_parser->stream, header_buf, AAC_ADTS_HEADER_SIZE) != AAC_ADTS_HEADER_SIZE) {
        aic_stream_seek(aac_parser->stream, current_pos, SEEK_SET);
        return PARSER_ERROR;
    }

    // Parse header
    ret = parse_adts_header(aac_parser, &header, header_buf);
    if (ret != PARSER_OK) {
        aic_stream_seek(aac_parser->stream, current_pos, SEEK_SET);
        return ret;
    }

    aac_parser->media_info.has_audio = 1;
    aac_parser->media_info.has_video = 0;
    aac_parser->media_info.duration = 0; // Need to parse whole file to get duration
    aac_parser->media_info.file_size = aac_parser->file_size;

    // Audio stream info
    aac_parser->media_info.audio_stream.codec_type = MPP_CODEC_AUDIO_DECODER_AAC;
    aac_parser->media_info.audio_stream.sample_rate = sampling_freq_table[header.sampling_freq_index];
    aac_parser->media_info.audio_stream.nb_channel = 2; //faad dec channel is 2, header.channel_config;
    aac_parser->media_info.audio_stream.bits_per_sample = 16; // AAC is typically 16-bit
    aac_parser->media_info.audio_stream.bit_rate = 0; // Can be calculated if duration is known
    aac_parser->media_info.audio_stream.extra_data_size = 0;
    // Restore position
    aic_stream_seek(aac_parser->stream, current_pos, SEEK_SET);

    return PARSER_OK;
}

s32 aac_destroy(struct aic_parser *parser)
{
    struct aic_aac_parser *aac_parser = (struct aic_aac_parser *)parser;

    if (aac_parser == NULL) {
        return -1;
    }

    if (aac_parser->stream)
        aic_stream_close(aac_parser->stream);
    if (aac_parser)
        mpp_free(aac_parser);
    return 0;
}

s32 aic_aac_parser_create(unsigned char *uri, struct aic_parser **parser)
{
    int ret = 0;
    struct aic_aac_parser *aac_parser = NULL;

    aac_parser = (struct aic_aac_parser *)mpp_alloc(sizeof(struct aic_aac_parser));
    if (aac_parser == NULL) {
        loge("mpp_alloc aic_parser failed!!!!!\n");
        ret = -1;
        goto exit;
    }
    memset(aac_parser, 0, sizeof(struct aic_aac_parser));

    if (aic_stream_open((char *)uri, &aac_parser->stream, O_RDONLY) < 0) {
        loge("stream open fail");
        ret = -1;
        goto exit;
    }

    aac_parser->base.get_media_info = aac_get_media_info;
    aac_parser->base.peek = aac_peek;
    aac_parser->base.read = aac_read;
    aac_parser->base.destroy = aac_destroy;
    aac_parser->base.seek = aac_seek;
    aac_parser->base.init = aac_init;

    *parser = &aac_parser->base;

    return ret;

exit:
    if (aac_parser) {
        if (aac_parser->stream) {
            aic_stream_close(aac_parser->stream);
        }
        mpp_free(aac_parser);
    }
    return ret;
}
