/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: Encapsulate different codec interfaces
 */

#ifndef AVCODEC_AVCODEC_H
#define AVCODEC_AVCODEC_H

/**
 * @file
 * @ingroup libavc
 * Libavcodec external API header
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include "codec_id.h"
#include "libavutil/log.h"
#include "libavutil/common.h"
#include "libavutil/avutil.h"
#include "libavutil/mem.h"

#define AV_INPUT_BUFFER_PADDING_SIZE 64
#define AV_INPUT_BUFFER_MIN_SIZE 16384

typedef struct AVCodec AVCodec;

#define FF_CMP_SAD  0
#define FF_CMP_SSE  1
#define FF_CMP_SATD 2
#define FF_CMP_DCT  3
#define FF_CMP_PSNR 4
#define FF_CMP_BIT  5
#define FF_CMP_RD   6
#define FF_CMP_ZERO 7
#define FF_CMP_VSAD 8
#define FF_CMP_VSSE 9
#define FF_CMP_NSSE 10
#define FF_CMP_W53  11
#define FF_CMP_W97  12
#define FF_CMP_DCTMAX 13
#define FF_CMP_DCT264 14
#define FF_CMP_CHROMA 256

typedef struct AVCodecContext {
    char codec_name[32];
    enum AVCodecID     codec_id;
    unsigned int codec_tag;
    void *priv_data;
    int bit_rate;

    int flags;

    unsigned char *extradata;
    int extradata_size;

    int sample_rate; ///< samples per sec
    int channels;
    int bits_per_sample;

    int frame_size;
    int frame_number;   ///< audio or video frame number
    int block_align;

    AVCodec *codec;

    int flags2;

} AVCodecContext;

typedef struct AVPacket {
    int64_t pts;
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    int   flags;
    int side_data_elems;
    int64_t duration;
    int64_t pos;                            ///< byte position in stream, -1 if unknown
} AVPacket;


typedef struct AVFrame {
#define AV_NUM_DATA_POINTERS 8
    uint8_t *data[AV_NUM_DATA_POINTERS];
    int frame_size;
    uint8_t **extended_data;
    int width, height;
    int nb_samples;
    int format;
    int key_frame;
    int64_t pts;
    int64_t pkt_pts;
    int64_t pkt_dts;
    int sample_rate;
    int flags;
    int64_t pkt_pos;
    int64_t pkt_duration;
    int channels;
    int pkt_size;
} AVFrame;

struct AVCodec
{
    const char *name;
    enum AVMediaType type;
    enum AVCodecID id;
    int priv_data_size;
    int (*init)(AVCodecContext *);
    int (*encode)(AVCodecContext *, uint8_t *buf, int buf_size, void *data);
    int (*close)(AVCodecContext *);
    int (*decode)(struct AVCodecContext *, void *data, int *got_frame_ptr, struct AVPacket *avpkt);
    int capabilities;

    struct AVCodec *next;
    void (*flush)(AVCodecContext *);
};


/**
 * Find a decoder by CodecID
 */
AVCodec *avcodec_find_decoder(enum AVCodecID id);


/**
 * Allocate an AVCodecContext and set its fields to default values. The
 * resulting struct should be freed with avcodec_free_context().
 */
AVCodecContext *avcodec_alloc_context(void);


/**
 * Free the codec context and everything associated with it and write NULL to
 * the provided pointer.
 */
void avcodec_free_context(AVCodecContext **avctx);

/**
 * @return a positive value if s is open
 * with no corresponding avcodec_close()), 0 otherwise.
 */
int avcodec_open(AVCodecContext *avctx, AVCodec *codec);



/**
 * Decode the audio frame of size avpkt->size from avpkt->data into frame.
 * @param      avctx the codec context
 * @param[out] frame The AVFrame in which to store decoded audio samples.
 * @param[out] got_frame_ptr Zero if no frame could be decoded, otherwise it is non-zero.
 * @param[in]  avpkt The input AVPacket containing the input buffer.
 * @return A negative error code is returned
 */
int avcodec_decode_audio(AVCodecContext *avctx, AVFrame *frame, int *got_frame_ptr, struct AVPacket *avpkt);

/**
 * Close a given AVCodecContext and free all the data associated with it
 */
int avcodec_close(AVCodecContext *avctx);


/**
 * Register the codec codec and initialize libavcodec.
 *
 * @warning either this function or avcodec_register_all() must be called
 * before any other libavcodec functions.
 */
void avcodec_register(AVCodec *codec);

/**
 * Register all the codecs, parsers and bitstream filters which were enabled at
 * configuration time. If you do not call this function you can select exactly
 * which formats you want to support, by using the individual registration
 * functions.
 */
void avcodec_register_all(void);


/**
 * Reset the internal codec state / flush internal buffers. Should be called
 * e.g. when seeking or when switching to a different stream.
 *
 * @note for decoders, when refcounted frames are not used
 *
 * @note for encoders, this function will only do something if the encoder
 * declares support for AV_CODEC_CAP_ENCODER_FLUSH.
 */
void avcodec_flush_buffers(AVCodecContext *avctx);

#ifdef __cplusplus
}
#endif

#endif
