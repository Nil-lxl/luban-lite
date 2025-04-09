/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: middle media riff common interface
 */

#include "aic_riff.h"
#include "aic_utils.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include <string.h>


int alloc_extradata(struct aic_codec_param *par, int size)
{
    mpp_free(par->extradata);
    par->extradata_size = 0;

    if (size < 0 || size >= INT32_MAX - PARSER_INPUT_BUFFER_PADDING_SIZE)
        return -1;

    par->extradata = mpp_alloc(size + PARSER_INPUT_BUFFER_PADDING_SIZE);
    if (!par->extradata)
        return -1;

    memset(par->extradata + size, 0, PARSER_INPUT_BUFFER_PADDING_SIZE);
    par->extradata_size = size;

    return 0;
}

int aic_get_extradata(struct aic_codec_param *par, struct aic_stream *pb, int size)
{
    int ret = alloc_extradata(par, size);
    if (ret < 0)
        return ret;
    ret = aic_stream_read(pb, par->extradata, size);
    if (ret != size) {
        mpp_free(par->extradata);
        par->extradata_size = 0;
        logi("Failed to read extradata of size %d\n", size);
        return ret;
    }

    return ret;
}
/* "big_endian" values are needed for RIFX file format */
int aic_get_wav_header(struct aic_stream *pb, struct aic_codec_param *par, int size,
                       int big_endian, int get_extradata)
{
    int id;
    uint64_t bitrate = 0;

    if (size < 14) {
        loge("wav header size < 14");
        return -1;
    }

    par->codec_type = MPP_MEDIA_TYPE_AUDIO;
    if (!big_endian) {
        id = aic_stream_rl16(pb);
        if (id != 0x0165) {
            par->channels = aic_stream_rl16(pb);
            par->sample_rate = aic_stream_rl32(pb);
            bitrate = aic_stream_rl32(pb) * 8LL;
            par->block_align = aic_stream_rl16(pb);
        }
    } else {
        id = aic_stream_rb16(pb);
        par->channels = aic_stream_rb16(pb);
        par->sample_rate = aic_stream_rb32(pb);
        bitrate = aic_stream_rb32(pb) * 8LL;
        par->block_align = aic_stream_rb16(pb);
    }
    if (size == 14) { /* We're dealing with plain vanilla WAVEFORMAT */
        par->bits_per_coded_sample = 8;
    } else {
        if (!big_endian) {
            par->bits_per_coded_sample = aic_stream_rl16(pb);
        } else {
            par->bits_per_coded_sample = aic_stream_rb16(pb);
        }
    }
    if (id == 0xFFFE) {
        par->codec_tag = 0;
    } else {
        par->codec_tag = id;
        par->codec_id = aic_codec_get_id(aic_codec_wav_tags, id);
    }
    if (size >= 18 &&
        id != 0x0165) {                   /* We're obviously dealing with WAVEFORMATEX */
        int cbSize = aic_stream_rl16(pb); /* cbSize */
        if (big_endian) {
            loge("WAVEFORMATEX support for RIFX files");
            return PARSER_ERROR;
        }
        size -= 18;
        cbSize = MPP_MIN(size, cbSize);
        if (cbSize >= 22 && id == 0xfffe) { /* WAVEFORMATEXTENSIBLE */
            cbSize -= 22;
            size -= 22;
        }
        if (get_extradata) {
            if (cbSize > 0) {
                if (aic_get_extradata(par, pb, cbSize) < 0)
                    return -1;
                size -= cbSize;
            }
        }
        /* It is possible for the chunk to contain garbage at the end */
        if (size > 0)
            aic_stream_skip(pb, size);
    } else if (id == 0x0165 && size >= 32) {
        int nb_streams, i;

        size -= 4;
        if (aic_get_extradata(par, pb, size) < 0)
            return -1;
        nb_streams = AIC_RL16(par->extradata + 4);
        par->sample_rate = AIC_RL32(par->extradata + 12);
        par->channels = 0;
        bitrate = 0;
        if (size < 8 + nb_streams * 20)
            return -1;
        for (i = 0; i < nb_streams; i++)
            par->channels += par->extradata[8 + i * 20 + 17];
    }

    par->bit_rate = bitrate;

    if (par->sample_rate <= 0) {
        loge("Invalid sample rate: %d\n", par->sample_rate);
        return -1;
    }

    return 0;
}

int aic_get_bmp_header(struct aic_stream *pb, struct aic_codec_param *par,
                       unsigned int *size)
{
    int tag1;
    uint32_t size_ = aic_stream_rl32(pb);
    if (size)
        *size = size_;
    par->width = aic_stream_rl32(pb);
    par->height = (int32_t)aic_stream_rl32(pb);
    aic_stream_rl16(pb); /* planes */
    aic_stream_rl16(pb); /* depth */
    tag1 = aic_stream_rl32(pb);
    aic_stream_rl32(pb); /* ImageSize */
    aic_stream_rl32(pb); /* XPelsPerMeter */
    aic_stream_rl32(pb); /* YPelsPerMeter */
    aic_stream_rl32(pb); /* ClrUsed */
    aic_stream_rl32(pb); /* ClrImportant */
    return tag1;
}
