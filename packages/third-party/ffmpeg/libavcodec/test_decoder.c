/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc:  decoder test demo
 */

#include "avcodec.h"
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define FFMPEG_OUT_BUF_SIZE (70 * 1024)
#define FFMPEG_WMA_WAV_LEN  (28)
#define FFMPEG_WMA_EXTRA_OFF (18)
#define TEST_WMA_SAMPLE1

typedef struct ffmpeg_decoder_handle {
    AVCodec *codec;
    AVCodecContext *actx;
} ffmpeg_decoder_handle;

/*WAVEFORMATEX structure*/
#ifdef TEST_WMA_SAMPLE1
#define FFMPEG_IN_BUF_SIZE  (0x02E7)
static unsigned char g_extend_data[FFMPEG_WMA_WAV_LEN] = {
    0x61, 0x01, 0x02, 0x00, // codec_id + channels
    0x44, 0xAC, 0x00, 0x00, // sample_rate
    0x80, 0x3E, 0x00, 0x00, // bit_rate
    0xE7, 0x02, 0x10, 0x00, // block_align + bits_per_sample
    0x0A, 0x00, 0x00, 0x00, // extra_size
    0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00
};
#else
#define FFMPEG_IN_BUF_SIZE  (0x0600)
static unsigned char g_extend_data[FFMPEG_WMA_WAV_LEN] = {
    0x61, 0x01, 0x02, 0x00, // codec_id + channels
    0x00, 0x7D, 0x00, 0x00, // sample_rate
    0xA0, 0x0F, 0x00, 0x00, // bit_rate
    0x00, 0x06, 0x10, 0x00, // block_align + bits_per_sample
    0x0A, 0x00, 0x00, 0x88, // extra_size
    0x00, 0x00, 0x17, 0x00,
    //0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00
};
#endif

/*Initial wma media info by file, this info may be parase by libavformat*/
static void ffmpeg_params_init(AVCodecContext *actx, enum AVCodecID codec_id)
{
    if (!actx) {
        printf("actx is null.\n");
        return;
    }
    /*   This data read from wma file */
    actx->codec_id = codec_id;
    actx->codec_tag = 0x1061;         //2byte - 0x0161
#ifdef TEST_WMA_SAMPLE1
    actx->channels = 2;               //2byte - 0x0002
    actx->sample_rate = 44100;        //4byte - 0x0000AC44
    actx->bit_rate = 16000 * 8;       //4byte - 0x00003E80
    actx->block_align = FFMPEG_IN_BUF_SIZE; //2byte - 0x02E7
    actx->bits_per_sample = 16;       //2byte - 0x0010
    actx->extradata_size = 0xA;       //2Byte - 0x000A;
#else
    actx->channels = 2;               //2byte - 0x0002
    actx->sample_rate = 32000;        //4byte - 0x00007D00
    actx->bit_rate = 4000 * 8;        //4byte - 0x0000FA0
    actx->block_align = FFMPEG_IN_BUF_SIZE; //2byte - 0x0600
    actx->bits_per_sample = 16;       //2byte - 0x0010
    actx->extradata_size = 0xA;       //2Byte - 0x000A;
#endif
    actx->extradata = &g_extend_data[FFMPEG_WMA_EXTRA_OFF];
}

static ffmpeg_decoder_handle *ffmpeg_create_decoder(enum AVCodecID codec_id)
{
    int ret;
    ffmpeg_decoder_handle *h_decoder = av_malloc(sizeof(ffmpeg_decoder_handle));
    if (!h_decoder)
        return NULL;

    avcodec_register_all();

    h_decoder->actx = avcodec_alloc_context();
    if (!h_decoder->actx) {
        printf("avcodec_alloc_context failed.\n");
        return NULL;
    }

    ffmpeg_params_init(h_decoder->actx, codec_id);

    h_decoder->codec = avcodec_find_decoder(codec_id);
    if (!h_decoder->codec) {
        av_log(h_decoder->actx, AV_LOG_ERROR, "avcodec_find_decoder failed\n");
        return NULL;
    }

    ret = avcodec_open(h_decoder->actx, h_decoder->codec);
    if (ret) {
        av_log(h_decoder->actx, AV_LOG_ERROR, "avcodec_open failed %d.\n", ret);
        return NULL;
    }
    return h_decoder;
}


static int ffmpeg_decoder_decode(ffmpeg_decoder_handle *h_decoder, unsigned char *in_buf,
                                 int in_size, unsigned char *out_buf)
{
    if (!h_decoder || !h_decoder->actx)
        return -1;

    int ret, got_frame = 0;
    AVFrame av_frame = {0};
    struct AVPacket avpkt = {0};

    av_frame.data[0] = out_buf;
    av_frame.frame_size = 0;
    avpkt.data = in_buf;
    avpkt.size = in_size;

    ret = avcodec_decode_audio(h_decoder->actx, &av_frame, &got_frame, &avpkt);
    if (ret < 0) {
        av_log(h_decoder->actx, AV_LOG_ERROR, "avcodec_decode_audio failed %d.\n", ret);
        return -1;
    }
    if (h_decoder->actx->frame_number % 50 == 0)
    {
        av_log(h_decoder->actx, AV_LOG_INFO, "decode ret(%d), nb_samples %d, frame_size %d, frame_number %d, got_frame %d.\n",
            ret, av_frame.nb_samples, av_frame.frame_size, h_decoder->actx->frame_number, got_frame);
        av_log(h_decoder->actx, AV_LOG_INFO, "pcm head data: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x.\n",
            out_buf[0], out_buf[1], out_buf[2], out_buf[3], out_buf[4], out_buf[5], out_buf[6], out_buf[7],
            out_buf[8], out_buf[9], out_buf[10], out_buf[11], out_buf[12], out_buf[12], out_buf[14], out_buf[15]);
        av_log(h_decoder->actx, AV_LOG_INFO, "pcm tail data: %x %x %x %x %x %x %x %x.\n",
            out_buf[av_frame.frame_size - 8], out_buf[av_frame.frame_size - 7], out_buf[av_frame.frame_size - 6],
            out_buf[av_frame.frame_size - 5], out_buf[av_frame.frame_size - 4], out_buf[av_frame.frame_size - 3],
            out_buf[av_frame.frame_size - 2], out_buf[av_frame.frame_size - 1]);
    }


    if (!got_frame)
        return 0;

    return (av_frame.frame_size);
}

static void ffmpeg_destroy_decoder(ffmpeg_decoder_handle *h_decoder)
{
    if (!h_decoder)
        return;

    if (h_decoder->actx) {
        avcodec_free_context(&h_decoder->actx);
        h_decoder->codec = NULL;
    }

    av_free(h_decoder);
}

static int ffmpeg_decoder_demo(int argc, char *argv[])
{
    int i = 0, loop_cnt;
    int in_size, out_size, file_size;
    unsigned char *in_buf, *out_buf;
    unsigned int frame_size_total, frame_size_1s;
    FILE *fin_file, *fout_file;
    ffmpeg_decoder_handle *h_decoder;
    struct stat file_stat;

    if (argc != 3) {
        printf("Usage:  %s in_file.dat out_file.pcm\n", argv[0]);
        return -1;
    }

	if(stat(argv[1], &file_stat) != 0)
		return -1;
	else
		file_size = file_stat.st_size;

    if ((fin_file = fopen(argv[1], "rb")) == NULL) {
        printf("ERROR: opening %s for infile\n", argv[1]);
        return -1;
    }
    if ((fout_file = fopen(argv[2], "wb")) == NULL) {
        printf("ERROR: opening %s for output\n", argv[2]);
        fclose(fin_file);
        return -1;
    }

    in_size = FFMPEG_IN_BUF_SIZE;
    in_buf = malloc(in_size);
    out_buf = malloc(FFMPEG_OUT_BUF_SIZE);

    h_decoder = ffmpeg_create_decoder(AV_CODEC_ID_WMAV2);
    if (!h_decoder) {
        printf("create decoder failed\n");
        goto exit;
    }
    frame_size_total = 0;
    frame_size_1s =
        h_decoder->actx->sample_rate * h_decoder->actx->bits_per_sample * h_decoder->actx->channels / 8;

    loop_cnt = file_size / in_size;
    for (i = 0; i < loop_cnt; i++) {
        fread(in_buf, 1, in_size, fin_file);
        out_size = ffmpeg_decoder_decode(h_decoder, in_buf, in_size, out_buf);
        if (out_size > 0) {
            fwrite(out_buf, 1, out_size, fout_file);
            frame_size_total += out_size;
        }
    }
    av_log(h_decoder->actx, AV_LOG_INFO, "decode total_frame_count %d, total_time %d s.\n",
        h_decoder->actx->frame_number, frame_size_total / frame_size_1s);
    ffmpeg_destroy_decoder(h_decoder);

exit:
    if (fin_file)
        fclose(fin_file);
    if (fout_file)
        fclose(fout_file);
    if (in_buf)
        free(in_buf);
    if (out_buf)
        free(out_buf);

    return 0;
}

MSH_CMD_EXPORT(ffmpeg_decoder_demo, ffmpeg decoder test);
