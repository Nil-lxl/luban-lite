/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: flac decoder interface
 */

#include <inttypes.h>
#include <pthread.h>
#include <rtthread.h>
#include <string.h>
#include <unistd.h>

#include "FLAC/stream_decoder.h"
#include "audio_decoder.h"
#include "mpp_dec_type.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_thread.h"
#include "mpp_ringbuf.h"
#include "share/compat.h"

#define FLAC_BUFFER_LEN (16 * 1024)
#define FLAC_MIN_FRAME_SIZE 11
#define FLAC_THD_START 0
#define FLAC_THD_STOP 1
#define FLAC_THD_EXIT 2
#define FLAC_MIN_FRAME_CNT 5

//#define FLAC_WRITE_FILE

struct flac_stream_decoder_client {
    struct aic_audio_decoder *decoder;
    uint32_t current_metadata_number;
    FLAC__bool ignore_errors;
    FLAC__bool error_occurred;
};

struct flac_audio_decoder {
    struct aic_audio_decoder decoder;
    struct mpp_packet *curr_packet;
    FLAC__StreamDecoder *stream_decoder;
    unsigned char *flac_buffer;
    uint32_t flac_buffer_len;
    uint32_t channels;
    uint32_t sample_rate;
    uint32_t bits_per_sample;
    FLAC__uint64 total_samples;
    FLAC__uint64 have_samples;
    uint32_t frame_id;
    uint32_t frame_count;
    struct flac_stream_decoder_client decoder_client_data;
    mpp_ringbuf_t ringbuf;
    pthread_t thread_id;
    mpp_sem_t ringbuf_sem;
    int packet_end_flag;
    int read_end_flag;
    int write_end_flag;
    int thread_state;
};


static void *flac_decoder_thread(void *p_decoder_data);

#ifdef FLAC_WRITE_FILE
#define FLAC_WRITE_FILE_NAME "/sdcard/test.wav"
static FILE *fout_wav_file = NULL;
extern FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 x);
extern FLAC__bool write_little_endian_int16(FILE *f, FLAC__int16 x);
extern FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 x);

static void flac_decoder_write_file(struct aic_audio_decoder *decoder,
                                    const FLAC__Frame *flac_frame,
                                    const FLAC__int32 *const buffer[])
{

    /* write WAVE header before we write the first frame */
    int i = 0;
    struct flac_audio_decoder *flac_decoder = (struct flac_audio_decoder *)decoder;
    FLAC__uint64 total_samples = flac_decoder->total_samples;
    unsigned sample_rate = flac_decoder->sample_rate;
    unsigned channels = flac_decoder->channels;
    unsigned bps = flac_decoder->bits_per_sample;
    unsigned total_size = (FLAC__uint32)(total_samples * channels * (bps / 8));

    if (flac_frame->header.number.sample_number == 0) {
        if ((fout_wav_file = fopen(FLAC_WRITE_FILE_NAME, "wb")) == NULL) {
            loge("ERROR: opening %s for output\n", FLAC_WRITE_FILE_NAME);
            return;
        }
        if (
            fwrite("RIFF", 1, 4, fout_wav_file) < 4 ||
            !write_little_endian_uint32(fout_wav_file, total_size + 36) ||
            fwrite("WAVEfmt ", 1, 8, fout_wav_file) < 8 ||
            !write_little_endian_uint32(fout_wav_file, 16) ||
            !write_little_endian_uint16(fout_wav_file, 1) ||
            !write_little_endian_uint16(fout_wav_file, (FLAC__uint16)channels) ||
            !write_little_endian_uint32(fout_wav_file, sample_rate) ||
            !write_little_endian_uint32(fout_wav_file, sample_rate * channels * (bps / 8)) ||
            !write_little_endian_uint16(fout_wav_file, (FLAC__uint16)(channels * (bps / 8))) ||
            !write_little_endian_uint16(fout_wav_file, (FLAC__uint16)bps) ||
            fwrite("data", 1, 4, fout_wav_file) < 4 ||
            !write_little_endian_uint32(fout_wav_file, total_size)) {
            loge("ERROR: write error\n");
            return;
        }
    }

    /* write decoded PCM samples */
    if (fout_wav_file) {
        for (i = 0; i < flac_frame->header.blocksize; i++) {
            if (
                !write_little_endian_int16(fout_wav_file, (FLAC__int16)buffer[0][i]) ||
                !write_little_endian_int16(fout_wav_file, (FLAC__int16)buffer[1][i])
            ) {
                loge("ERROR: write error\n");
                return;
            }
        }
        printf("write %u to wav file size %d\n.", flac_decoder->frame_id,
            flac_frame->header.blocksize * channels);
    }

    if (fout_wav_file && flac_decoder->frame_id >= 200) {
        fclose(fout_wav_file);
        fout_wav_file = NULL;
    }
}
#endif

static int flac_decoder_put_frame(struct aic_audio_decoder *decoder,
                                  const FLAC__Frame *flac_frame,
                                  const FLAC__int32 *const buffer[])
{
    char *data;
    int i = 0, pos = 0, try_cnt = 100;
    struct flac_audio_decoder *flac_decoder = (struct flac_audio_decoder *)decoder;
    struct aic_audio_frame *frame = NULL;

    if (flac_decoder->sample_rate <= 0 || flac_decoder->channels < 2 ||
        flac_decoder->frame_count < 1) {
        loge("wrong flac decoder params: sample_rate %"PRIu32", "
             "channels %"PRIu32", frame_count %"PRIu32".",
             flac_decoder->sample_rate, flac_decoder->channels, flac_decoder->frame_count);
        return -1;
    }
    if (flac_decoder->decoder.fm == NULL) {
        struct audio_frame_manager_cfg cfg;
        cfg.bits_per_sample = flac_decoder->bits_per_sample;
        cfg.samples_per_frame = flac_decoder->channels * flac_frame->header.blocksize;
        cfg.frame_count = flac_decoder->frame_count > FLAC_MIN_FRAME_CNT ?
            flac_decoder->frame_count : FLAC_MIN_FRAME_CNT;
        flac_decoder->decoder.fm = audio_fm_create(&cfg);
        if (flac_decoder->decoder.fm == NULL) {
            loge("audio_fm_create fail!!!\n");
        }
        return -1;
    }

    if (flac_decoder->thread_state || flac_decoder->write_end_flag)
        return 0;

    /*If all sample data has been received, exit directly*/
    if (flac_decoder->have_samples > flac_decoder->total_samples)
        return 0;

    /*Try to get empty frame and write frame data*/
    while (i++ < try_cnt) {
        frame = audio_fm_decoder_get_frame(flac_decoder->decoder.fm);
        if (frame)
            break;
        usleep(10000);
    }
    if (!frame) {
        flac_decoder->have_samples += flac_frame->header.blocksize;
        loge("get empty cur_sample %"PRIu32", total_samples %"PRIu64", "
            "have_samples %"PRIu64", timeout %d ms.",
            flac_frame->header.blocksize, flac_decoder->total_samples,
            flac_decoder->have_samples, try_cnt * 10);
        return -1;
    }

    frame->pts = (flac_decoder->have_samples) * 1000000 / frame->sample_rate;
    flac_decoder->have_samples += flac_frame->header.blocksize;

    frame->channels = flac_frame->header.channels;
    frame->sample_rate = flac_decoder->sample_rate;
    frame->bits_per_sample = flac_decoder->bits_per_sample;
    frame->id = flac_decoder->frame_id++;
    frame->flag = flac_decoder->have_samples >= flac_decoder->total_samples ? FRAME_FLAG_EOS : 0;

    data = frame->data;
    if (frame->flag == FRAME_FLAG_EOS)
        flac_decoder->write_end_flag = 1;

    logd("write_pcm frame_id %"PRIu32", blocksize %"PRIu32", pts %lld.", flac_decoder->frame_id,
         flac_frame->header.blocksize, frame->pts);
    logd("write_pcm total_samples %" PRIu64 ", have_samples %" PRIu64 ".",
         flac_decoder->total_samples, flac_decoder->have_samples);


    for (i = 0; i < flac_frame->header.blocksize; i++) {
        /* output sample(s) in 16-bit signed little-endian PCM */
        data[pos++] = ((FLAC__int16)buffer[0][i] >> 0) & 0xff;
        data[pos++] = ((FLAC__int16)buffer[0][i] >> 8) & 0xff;
        if (frame->channels == 2) {
            data[pos++] = ((FLAC__int16)buffer[1][i] >> 0) & 0xff;
            data[pos++] = ((FLAC__int16)buffer[1][i] >> 8) & 0xff;
        }
    }
    audio_fm_decoder_put_frame(flac_decoder->decoder.fm, frame);

    return 0;
}

static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *stream_decoder,
                                                   FLAC__byte buffer[],
                                                   size_t *bytes,
                                                   void *client_data)
{
    struct flac_stream_decoder_client *decoder_client =
        (struct flac_stream_decoder_client *)client_data;
    struct flac_audio_decoder *flac_decoder =
        (struct flac_audio_decoder *)decoder_client->decoder;

    const size_t requested_bytes = *bytes;
    int data_len = 0;

wait_next_data:
    if (flac_decoder->thread_state != FLAC_THD_START) {
        flac_decoder->read_end_flag = 1;
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }

    data_len = mpp_ringbuffer_data_len(flac_decoder->ringbuf);
    /*If the data len less than the request len,
     *should be wait enough data coming and then run decoder*/
    if (data_len < requested_bytes && flac_decoder->packet_end_flag != FRAME_FLAG_EOS) {
        mpp_sem_wait(flac_decoder->ringbuf_sem, MPP_WAIT_FOREVER);
        goto wait_next_data;
    } else if (data_len == 0 && flac_decoder->packet_end_flag == FRAME_FLAG_EOS) {
        logd("flac decoder read the end of stream !!!\n");
        flac_decoder->read_end_flag = 1;
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }

    logd("read_callback request byte %d, data_len %d.", (int)requested_bytes, data_len);
    if (requested_bytes > 0) {
        if (data_len < requested_bytes)
            *bytes = mpp_ringbuffer_get(flac_decoder->ringbuf, buffer, data_len);
        else
            *bytes = mpp_ringbuffer_get(flac_decoder->ringbuf, buffer, requested_bytes);
        if (*bytes == 0) {
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        } else {
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        }
    } else {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
    }
}

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *stream_decoder,
                                                     const FLAC__Frame *frame,
                                                     const FLAC__int32 *const buffer[],
                                                     void *client_data)
{
    struct aic_audio_decoder *decoder =
        ((struct flac_stream_decoder_client *)client_data)->decoder;

    if (frame->header.channels != 2) {
        loge("ERROR: This frame contains %"PRIu32" channels (should be 2)\n",
            frame->header.channels);
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (buffer[0] == NULL) {
        loge("ERROR: buffer [0] is NULL\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    if (buffer[1] == NULL) {
        loge("ERROR: buffer [1] is NULL\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    /* write decoded PCM samples */
#ifdef FLAC_WRITE_FILE
    flac_decoder_write_file(decoder, frame, buffer);
#endif
    flac_decoder_put_frame(decoder, frame, buffer);

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback(const FLAC__StreamDecoder *stream_decoder,
                              const FLAC__StreamMetadata *metadata, void *client_data)
{
    struct flac_audio_decoder *flac_decoder =
        (struct flac_audio_decoder *)((struct flac_stream_decoder_client *)client_data)->decoder;

    /* print some stats */
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        /* save for later */
        flac_decoder->total_samples = metadata->data.stream_info.total_samples;
        flac_decoder->sample_rate = metadata->data.stream_info.sample_rate;
        flac_decoder->channels = metadata->data.stream_info.channels;
        flac_decoder->bits_per_sample = metadata->data.stream_info.bits_per_sample;

        printf("sample rate    : %"PRIu32" Hz\n", flac_decoder->sample_rate);
        printf("channels       : %"PRIu32"\n", flac_decoder->channels);
        printf("bits per sample: %"PRIu32"\n", flac_decoder->bits_per_sample);
        printf("total samples  : %" PRIu64 "\n", flac_decoder->total_samples);
    }
}

static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
    struct flac_audio_decoder *flac_decoder =
        (struct flac_audio_decoder *)((struct flac_stream_decoder_client *)client_data)->decoder;

    if (flac_decoder->thread_state != FLAC_THD_START)
        return true;

    if (flac_decoder->read_end_flag &&
        flac_decoder->have_samples >= flac_decoder->total_samples) {
        printf("eof_callback read datalen 0\n");
        return true;
    } else {
        return false;
    }
}

static void error_callback(const FLAC__StreamDecoder *decoder,
                           FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    struct flac_audio_decoder *flac_decoder =
        (struct flac_audio_decoder *)((struct flac_stream_decoder_client *)client_data)->decoder;

    if (!flac_decoder->read_end_flag && (flac_decoder->thread_state == FLAC_THD_START))
        loge("Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

int __flac_decode_init(struct aic_audio_decoder *decoder, struct aic_audio_decode_config *config)
{
    int ret = DEC_OK;
    pthread_attr_t attr;
    FLAC__StreamDecoderInitStatus init_status;
    struct flac_audio_decoder *flac_decoder = (struct flac_audio_decoder *)decoder;

    flac_decoder->ringbuf = mpp_ringbuffer_create(FLAC_BUFFER_LEN);
    if (!flac_decoder->ringbuf) {
        loge("ERROR: allocating flac ringbuffer failed\n");
        return DEC_ERR_NULL_PTR;
    }
    flac_decoder->flac_buffer_len = FLAC_BUFFER_LEN;
    flac_decoder->decoder.pm = audio_pm_create(config);
    flac_decoder->frame_count = config->frame_count;
    flac_decoder->frame_id = 0;

    /*create flac stream decoder*/
    flac_decoder->stream_decoder = FLAC__stream_decoder_new();
    if (!flac_decoder->stream_decoder) {
        loge("ERROR: allocating decoder\n");
        ret = DEC_ERR_NULL_PTR;
        goto exit;
    }
    (void)FLAC__stream_decoder_set_md5_checking(flac_decoder->stream_decoder, true);

    flac_decoder->decoder_client_data.decoder = decoder;
    init_status = FLAC__stream_decoder_init_stream(flac_decoder->stream_decoder,
                                                   read_callback,
                                                   NULL, NULL, NULL,
                                                   eof_callback,
                                                   write_callback,
                                                   metadata_callback,
                                                   error_callback,
                                                   &flac_decoder->decoder_client_data);
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        loge("ERROR: initializing decoder: %s\n",
            FLAC__StreamDecoderInitStatusString[init_status]);
        ret = DEC_ERR_NOT_CREATE;
        goto exit;
    }

    flac_decoder->ringbuf_sem = mpp_sem_create();
    if (!flac_decoder->ringbuf_sem) {
        loge("mpp_sem_create ringbuf sem fail!\n");
        ret = DEC_ERR_NOT_CREATE;
        goto exit;
    }

    /*create flac stream decoder*/
    pthread_attr_init(&attr);
    attr.stacksize = 16 * 1024;
    attr.schedparam.sched_priority = 22;
    flac_decoder->thread_state = FLAC_THD_START;
    ret = pthread_create(&flac_decoder->thread_id, &attr,
                         flac_decoder_thread, flac_decoder);

    if (ret) {
        loge("thread create flac decoder failed %d!\n", ret);
        ret = DEC_ERR_NOT_CREATE;
        goto exit;
    }

    logi("__flac_decode_init success.");
    return DEC_OK;

exit:
    if (flac_decoder->ringbuf_sem) {
        mpp_sem_delete(flac_decoder->ringbuf_sem);
        flac_decoder->ringbuf_sem = NULL;
    }
    if (flac_decoder->ringbuf) {
        mpp_ringbuffer_destroy(flac_decoder->ringbuf);
        flac_decoder->ringbuf = NULL;
    }
    if (flac_decoder->stream_decoder) {
        FLAC__stream_decoder_delete(flac_decoder->stream_decoder);
        flac_decoder->stream_decoder = NULL;
    }
    if (flac_decoder->decoder.pm) {
        audio_pm_destroy(flac_decoder->decoder.pm);
        flac_decoder->decoder.pm = NULL;
    }
    return ret;
}

int __flac_decode_destroy(struct aic_audio_decoder *decoder)
{
    struct flac_audio_decoder *flac_decoder = (struct flac_audio_decoder *)decoder;
    if (!flac_decoder) {
        return DEC_ERR_NOT_CREATE;
    }

    mpp_sem_signal(flac_decoder->ringbuf_sem);
    if (flac_decoder->thread_state == FLAC_THD_START)
        flac_decoder->thread_state = FLAC_THD_STOP;
    /*wait thread exit and release decoder safety*/
    while(flac_decoder->thread_state != FLAC_THD_EXIT)
        usleep(5000);

    if (flac_decoder->ringbuf_sem) {
        mpp_sem_delete(flac_decoder->ringbuf_sem);
        flac_decoder->ringbuf_sem = NULL;
    }

    pthread_join(flac_decoder->thread_id, (void *)NULL);

    if (flac_decoder->ringbuf) {
        mpp_ringbuffer_destroy(flac_decoder->ringbuf);
        flac_decoder->ringbuf = NULL;
    }
    if (flac_decoder->stream_decoder) {
        FLAC__stream_decoder_delete(flac_decoder->stream_decoder);
        flac_decoder->stream_decoder = NULL;
    }
    if (flac_decoder->decoder.pm) {
        audio_pm_destroy(flac_decoder->decoder.pm);
        flac_decoder->decoder.pm = NULL;
    }
    if (flac_decoder->decoder.fm) {
        audio_fm_destroy(flac_decoder->decoder.fm);
        flac_decoder->decoder.fm = NULL;
    }
    mpp_free(flac_decoder);
    return DEC_OK;
}

int __flac_decode_frame(struct aic_audio_decoder *decoder)
{
    int ret = DEC_OK;
    struct flac_audio_decoder *flac_decoder = (struct flac_audio_decoder *)decoder;
    if (!flac_decoder) {
        return DEC_ERR_NOT_CREATE;
    }
    if (flac_decoder->thread_state != FLAC_THD_START) {
        return DEC_OK;
    }
    if ((flac_decoder->decoder.fm) &&
        (audio_fm_get_empty_frame_num(flac_decoder->decoder.fm)) == 0) {
        return DEC_NO_EMPTY_FRAME;
    }
    if (flac_decoder->decoder.pm &&
        audio_pm_get_ready_packet_num(flac_decoder->decoder.pm) == 0) {
        return DEC_NO_READY_PACKET;
    }
    if (mpp_ringbuffer_space_len(flac_decoder->ringbuf) == 0) {
        mpp_sem_signal(flac_decoder->ringbuf_sem);
        return DEC_NO_EMPTY_PACKET;
    }
    flac_decoder->curr_packet = audio_pm_dequeue_ready_packet(flac_decoder->decoder.pm);
    if (!flac_decoder->curr_packet) {
        return DEC_NO_READY_PACKET;
    }

    /*Avoid the buffer not enough lead to flac decoder failed*/
    while (mpp_ringbuffer_space_len(flac_decoder->ringbuf) < flac_decoder->curr_packet->size) {
        mpp_sem_signal(flac_decoder->ringbuf_sem);
        usleep(5000);
    }

    ret = mpp_ringbuffer_put(flac_decoder->ringbuf, flac_decoder->curr_packet->data,
                             flac_decoder->curr_packet->size);
    mpp_sem_signal(flac_decoder->ringbuf_sem);

    if (ret != flac_decoder->curr_packet->size) {
        loge("ringbuffer put size %d < %d", ret, flac_decoder->curr_packet->size);
    }

    flac_decoder->packet_end_flag = flac_decoder->curr_packet->flag;
    if (flac_decoder->curr_packet->flag == FRAME_FLAG_EOS) {
        /*Active the flac decoder read_callback, avoid read_callback in wait status*/
        mpp_sem_signal(flac_decoder->ringbuf_sem);
        usleep(10000);
    }

    audio_pm_enqueue_empty_packet(flac_decoder->decoder.pm, flac_decoder->curr_packet);

    return DEC_OK;
}

int __flac_decode_control(struct aic_audio_decoder *decoder, int cmd, void *param)
{
    return DEC_OK;
}

int __flac_decode_reset(struct aic_audio_decoder *decoder)
{
    return DEC_OK;
}

static void *flac_decoder_thread(void *p_decoder_data)
{
    struct flac_audio_decoder *flac_decoder = (struct flac_audio_decoder *)p_decoder_data;
    if (!flac_decoder->stream_decoder) {
        loge("stream_decoder is not create");
        flac_decoder->thread_state = FLAC_THD_EXIT;
        return NULL;
    }
    FLAC__stream_decoder_process_until_end_of_stream(flac_decoder->stream_decoder);

    flac_decoder->thread_state = FLAC_THD_EXIT;
    printf("flac_decoder_thread exit.\n");

    return NULL;
}

struct aic_audio_decoder_ops flac_decoder = {
    .name = "flac",
    .init = __flac_decode_init,
    .destroy = __flac_decode_destroy,
    .decode = __flac_decode_frame,
    .control = __flac_decode_control,
    .reset = __flac_decode_reset,
};

struct aic_audio_decoder *create_flac_decoder()
{
    struct flac_audio_decoder *s =
        (struct flac_audio_decoder *)mpp_alloc(sizeof(struct flac_audio_decoder));
    if (s == NULL)
        return NULL;
    memset(s, 0, sizeof(struct flac_audio_decoder));
    s->decoder.ops = &flac_decoder;
    return &s->decoder;
}
