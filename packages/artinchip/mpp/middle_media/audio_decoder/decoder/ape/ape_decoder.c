/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <xiaodong.zhao@artinchip.com>
 * Desc: ape_decoder interface
 */

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#include "mpp_dec_type.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_log.h"

#include "audio_decoder.h"

#include "decoder.h"

#define BLOCKS_PER_LOOP     1152

#define INPUT_CHUNKSIZE     (20 * 1024)
#define APE_PCM_BUF_SIZE    (BLOCKS_PER_LOOP << 2)

enum ape_decode_state_e
{
    APE_STATE_PARSE_HEADER = 0,
    APE_STATE_PARSE_SEEK_TABLE,
    APE_STATE_SEEK_TO_FIRST_FRAME,
    APE_STATE_DECODE_FRAME,
};

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

struct ape_audio_decoder {
    struct aic_audio_decoder decoder;
    struct mpp_packet *curr_packet;

    struct ape_ctx_t ape_ctx;

    unsigned char es_buf[INPUT_CHUNKSIZE];
    int es_data_size;

    int32_t decoded0[BLOCKS_PER_LOOP];
    int32_t decoded1[BLOCKS_PER_LOOP];

    unsigned short pcm_buf[BLOCKS_PER_LOOP << 1];
    unsigned int pcm_samples;

    int first_byte;

    enum ape_decode_state_e state;
    int total_skip_size;
    int seek_table_offset;

    unsigned int frame_id;
    unsigned int frame_count;
    int eos;
};

int __ape_decode_init(struct aic_audio_decoder *decoder, struct aic_audio_decode_config *config)
{
    struct ape_audio_decoder *ape_decoder = (struct ape_audio_decoder *)decoder;
    struct audio_frame_manager_cfg cfg;

    ape_decoder->decoder.pm = audio_pm_create(config);
    if (!ape_decoder->decoder.pm) {
        loge("audio_pm_create ape failed.\n");
        return -1;
    }
    ape_decoder->frame_count = config->frame_count;
    ape_decoder->frame_id = 0;
    ape_decoder->pcm_samples = 0;
    ape_decoder->es_data_size = 0;
    ape_decoder->ape_ctx.cur_frame = 0;
    ape_decoder->total_skip_size = 0;
    ape_decoder->seek_table_offset = 0;
    ape_decoder->eos = 0;
    ape_decoder->first_byte = 3;
    ape_decoder->state = APE_STATE_PARSE_HEADER;

    cfg.bits_per_sample = 16;
    cfg.samples_per_frame = BLOCKS_PER_LOOP * 4 + 512;   // use to malloc frame buf
    cfg.frame_count = ape_decoder->frame_count;
    ape_decoder->decoder.fm = audio_fm_create(&cfg);
    if (ape_decoder->decoder.fm == NULL) {
        loge("audio_fm_create ape fail!!!\n");
        goto failed;
    }

    // ape_decoder->bits_per_sample = cfg.bits_per_sample;

    return 0;

failed:
    if (ape_decoder->decoder.pm) {
        audio_pm_destroy(ape_decoder->decoder.pm);
        ape_decoder->decoder.pm = NULL;
    }

    return -1;
}

int __ape_decode_destroy(struct aic_audio_decoder *decoder)
{
    struct ape_audio_decoder *ape_decoder = (struct ape_audio_decoder *)decoder;
    if (!ape_decoder) {
        return -1;
    }

    if (ape_decoder->decoder.fm) {
        audio_fm_destroy(ape_decoder->decoder.fm);
        ape_decoder->decoder.fm = NULL;
    }

    if (ape_decoder->decoder.pm) {
        audio_pm_destroy(ape_decoder->decoder.pm);
        ape_decoder->decoder.pm = NULL;
    }

    if (ape_decoder->ape_ctx.frames) {
        mpp_free(ape_decoder->ape_ctx.frames);
    }

    mpp_free(ape_decoder);

    return 0;

}

/*return consumed size*/
static int ape_input_data(struct ape_audio_decoder *ape_decoder, char *buf, int size)
{
    if ((NULL == ape_decoder) || (NULL == buf) || (size <= 0)) {
        loge("invalid parameter! [%p/%p/%d]\n", ape_decoder, buf, size);
        return -1;
    }

    if (size > INPUT_CHUNKSIZE - ape_decoder->es_data_size) {
        loge("may drop es data! %d/%d\n", size, INPUT_CHUNKSIZE - ape_decoder->es_data_size);
        size = INPUT_CHUNKSIZE - ape_decoder->es_data_size;
    }

    if (size > 0) {
        memcpy(ape_decoder->es_buf + ape_decoder->es_data_size, buf, size);
        ape_decoder->es_data_size += size;
    }

    return size;
}

static int ape_decode(struct ape_audio_decoder *ape_decoder, unsigned char *buf, int *size)
    {
    static int nblocks, frm_size;
    int used, i, ret;
    struct ape_ctx_t *ape_ctx = NULL;

    if ((NULL == ape_decoder) || (NULL == buf) || (NULL == size))
    {
        loge("<%s:%d> Invalid parameter!\n", __func__, __LINE__);
        return -1;
    }

    ape_ctx = &ape_decoder->ape_ctx;

    if (ape_ctx->new_frame) {
        frm_size = ape_ctx->frames[ape_ctx->cur_frame].size;

        /* Calculate how many blocks there are in this frame */
        if (ape_ctx->cur_frame == (ape_ctx->totalframes - 1)) {
            nblocks = ape_ctx->finalframeblocks;
        } else {
            nblocks = ape_ctx->blocksperframe;
        }

        ape_ctx->currentframeblocks = nblocks;

        used = 0;
        /* Initialise the frame decoder */
        init_frame_decoder(ape_ctx, buf, &ape_decoder->first_byte, &used);

        if (*size < used) {
            loge("critical error! size:%d used:%d\n", *size, used);
            return -1;
        }

        /* Update buffer */
        memmove(buf, buf + used, *size - used);
        *size -= used;
        frm_size -= used;

        ape_ctx->new_frame = 0;
    }

    if (ape_ctx->cur_frame < (ape_ctx->totalframes - 1)) {
        if (frm_size < 0) {
            loge("frm_size:%d\n", frm_size);
        }

        if (*size < MIN(frm_size, 10240)) {
            // loge("size:%d\n", *size);
            return -1;
        }
    }

    /* Decode the frame a chunk at a time */
    int sub_blocks = MIN(BLOCKS_PER_LOOP, nblocks);
    ret = decode_chunk(ape_ctx, buf, &ape_decoder->first_byte,
                    &used, ape_decoder->decoded0,
                    ape_decoder->decoded1, sub_blocks);
    if (ret < 0) {
        /* Frame decoding error, abort */
        loge("decode err, used:%d\n", used);
        return ret;
    }

    if (*size < used) {
        loge("critical error! size:%d used:%d sub_block:%d block:%d\n", *size, used, sub_blocks, nblocks);
        return -1;
    }

    /* Update the buffer */
    memmove(buf, buf + used, *size - used);
    *size -= used;
    frm_size -= used;

    /* Convert the output samples to WAV format and write to output file */
    if (ape_ctx->bps == 8) {
        for (i = 0; i < sub_blocks; i++) {
            /* 8 bit WAV uses unsigned samples */
            if (1 == ape_ctx->channels)
            {
                ape_decoder->pcm_buf[ape_decoder->pcm_samples] = (ape_decoder->decoded0[i] + 0x80) & 0xff;
            }
            else if (2 == ape_ctx->channels)
            {
                ape_decoder->pcm_buf[ape_decoder->pcm_samples * 2] = (ape_decoder->decoded0[i] + 0x80) & 0xff;
                ape_decoder->pcm_buf[ape_decoder->pcm_samples * 2 + 1] = (ape_decoder->decoded1[i] + 0x80) & 0xff;
            }
            ape_decoder->pcm_samples++;
        }
    } else {
        for (i = 0 ; i < sub_blocks ; i++) {
            if (1 == ape_ctx->channels)
            {
                ape_decoder->pcm_buf[ape_decoder->pcm_samples] = ape_decoder->decoded0[i];
            }
            else if (2 == ape_ctx->channels)
            {
                ape_decoder->pcm_buf[ape_decoder->pcm_samples * 2] = (ape_decoder->decoded0[i] & 0xffff);
                ape_decoder->pcm_buf[ape_decoder->pcm_samples * 2 + 1] = (ape_decoder->decoded1[i] & 0xffff);
            }

            ape_decoder->pcm_samples++;
        }
    }

    /* Decrement the block count */
    nblocks -= sub_blocks;
    if (nblocks <= 0) {
        ape_ctx->new_frame = 1;
        ape_ctx->cur_frame++;
    }

    return 0;
}

static int ape_output_pcm(struct ape_audio_decoder *ape_decoder, struct aic_audio_frame *frame)
{
    int size;

    if (!ape_decoder || !frame)
    {
        loge("invalid parameter [%p/%p]\n", ape_decoder, frame);
        return DEC_ERR_NULL_PTR;
    }

    if (ape_decoder->pcm_samples < BLOCKS_PER_LOOP)
    {
        frame->channels = 0;
        frame->sample_rate = 0;
        frame->pts = 0;
        frame->bits_per_sample = 0;
        frame->id = 0;
        frame->flag = 0;
        frame->size = 0;

        return DEC_NO_RENDER_FRAME; /* need more data */
    }

    size = BLOCKS_PER_LOOP * 2 * ape_decoder->ape_ctx.channels;
    memcpy(frame->data, ape_decoder->pcm_buf, size);

    ape_decoder->pcm_samples -= BLOCKS_PER_LOOP;
    if (ape_decoder->pcm_samples > 0)
    {
        memmove(ape_decoder->pcm_buf,
                ape_decoder->pcm_buf + BLOCKS_PER_LOOP * ape_decoder->ape_ctx.channels,
                ape_decoder->pcm_samples * 2 * ape_decoder->ape_ctx.channels);
    }

    frame->channels = ape_decoder->ape_ctx.channels;
    frame->sample_rate = ape_decoder->ape_ctx.samplerate;
    frame->pts = ape_decoder->curr_packet->pts;
    frame->bits_per_sample = 16;
    frame->id = ape_decoder->frame_id++;
    frame->flag = ape_decoder->eos;
    frame->size = size;

    return 0;
}

int __ape_decode_frame(struct aic_audio_decoder *decoder)
{
    static int pkt_size = 1024;
    int ret = 0, pakcet_num = 0;
    struct aic_audio_frame *frame = NULL;
    struct ape_audio_decoder *ape_decoder = (struct ape_audio_decoder *)decoder;

    if (INPUT_CHUNKSIZE - ape_decoder->es_data_size > pkt_size) {
        pakcet_num = audio_pm_get_ready_packet_num(ape_decoder->decoder.pm);
        if (pakcet_num == 0) {
            // loge("no packet!\n");
            return DEC_NO_READY_PACKET;
        }

        ape_decoder->curr_packet = audio_pm_dequeue_ready_packet(ape_decoder->decoder.pm);
        if (!ape_decoder->curr_packet) {
            // loge("get packet error!\n");
            return DEC_NO_READY_PACKET;
        }

        ape_decoder->eos = ape_decoder->curr_packet->flag;

        /* input data */
        ape_input_data(ape_decoder, ape_decoder->curr_packet->data, ape_decoder->curr_packet->size);
        audio_pm_enqueue_empty_packet(ape_decoder->decoder.pm, ape_decoder->curr_packet);

        if (pkt_size != ape_decoder->curr_packet->size) {
            pkt_size = ape_decoder->curr_packet->size;
        }
    }

    /* parse header */
    if (APE_STATE_PARSE_HEADER == ape_decoder->state) {
        ret = ape_parseheaderbuf(&ape_decoder->ape_ctx, ape_decoder->es_buf, ape_decoder->es_data_size);
        if (ret >= 0) {
            ape_decoder->seek_table_offset = ret;
            ape_decoder->state = APE_STATE_PARSE_SEEK_TABLE;
        } else {
            loge("parser header error!\n");
            return DEC_OK;
        }
    }

    /* parse seek table */
    if (APE_STATE_PARSE_SEEK_TABLE == ape_decoder->state) {
        ret = ape_parse_seek_table(&ape_decoder->ape_ctx,
                                   ape_decoder->es_buf,
                                   ape_decoder->es_data_size,
                                   ape_decoder->seek_table_offset);
        if (0 == ret) {
            ape_decoder->state = APE_STATE_SEEK_TO_FIRST_FRAME;
            ape_decoder->ape_ctx.new_frame = 1;
        } else {
            loge("ape_parse_seek_table error!\n");
            return DEC_OK;
        }
    }

    /* skip junk data, seek to first frame */
    if (APE_STATE_SEEK_TO_FIRST_FRAME == ape_decoder->state) {
        if (ape_decoder->ape_ctx.firstframe > ape_decoder->total_skip_size) {
            int skip = ape_decoder->ape_ctx.firstframe - ape_decoder->total_skip_size;
            if (skip > ape_decoder->es_data_size) {
                ape_decoder->total_skip_size += ape_decoder->es_data_size;
                ape_decoder->es_data_size = 0;
            } else {
                ape_decoder->total_skip_size += skip;
                memmove(ape_decoder->es_buf, ape_decoder->es_buf + skip, ape_decoder->es_data_size - skip);
                ape_decoder->es_data_size = ape_decoder->es_data_size - skip;
            }
        } else {
            ape_decoder->state = APE_STATE_DECODE_FRAME;
        }
    }

    /* decode */
    if ((APE_STATE_DECODE_FRAME == ape_decoder->state) && (ape_decoder->pcm_samples < BLOCKS_PER_LOOP)) {
        ape_decode(ape_decoder, ape_decoder->es_buf, &ape_decoder->es_data_size);
    }

    if (ape_decoder->ape_ctx.cur_frame == ape_decoder->ape_ctx.totalframes) {
        ape_decoder->eos = 1;
    }

    /* output pcm data */
    if (ape_decoder->pcm_samples >= BLOCKS_PER_LOOP) {

        if ((ape_decoder->decoder.fm)) {
            frame = audio_fm_decoder_get_frame(ape_decoder->decoder.fm);
            if (!frame) {
                // loge("no empty frame!\n");
                return DEC_NO_EMPTY_FRAME;
            }
        }

        ape_output_pcm(ape_decoder, frame);
        audio_fm_decoder_put_frame(ape_decoder->decoder.fm, frame);
    }

    return DEC_OK;
}

int __ape_decode_control(struct aic_audio_decoder *decoder, int cmd, void *param)
{
    return 0;
}

int __ape_decode_reset(struct aic_audio_decoder *decoder)
{
    struct ape_audio_decoder *ape_decoder = (struct ape_audio_decoder *)decoder;

    audio_pm_reset(ape_decoder->decoder.pm);
    audio_fm_reset(ape_decoder->decoder.fm);
    return 0;
}


struct aic_audio_decoder_ops ape_decoder = {
    .name           = "ape",
    .init           = __ape_decode_init,
    .destroy        = __ape_decode_destroy,
    .decode         = __ape_decode_frame,
    .control        = __ape_decode_control,
    .reset          = __ape_decode_reset,
};

struct aic_audio_decoder* create_ape_decoder(void)
{
    struct ape_audio_decoder *s = (struct ape_audio_decoder*)mpp_alloc(sizeof(struct ape_audio_decoder));
    if(s == NULL)
        return NULL;
    memset(s, 0, sizeof(struct ape_audio_decoder));
    s->decoder.ops = &ape_decoder;
    return &s->decoder;
}
