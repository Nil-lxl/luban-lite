/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#include <string.h>
#include "aic_core.h"
#include "aic_log.h"

#include "barcode_decoder.h"
#include "yydecoder.h"

#define YY_DECODER_SUCCESS 1

int barcode_decoder_init(barcode_decoder_t *decoder)
{
    if (!decoder) {
        return -1;
    }

    memset(decoder, 0, sizeof(barcode_decoder_t));

#ifdef BARCODE_ENABLE_DECODER
    int ret = Initial_Decoder();
    if (ret != YY_DECODER_SUCCESS) {
        pr_err("Decoder: Initial_Decoder failed with %d\n", ret);
        return -1;
    }

    // Configure decoder types (0-13)
    for (int i = 0; i < BARCODE_DECODER_TYPE_COUNT;i++) {
        Set_Donfig_Decoder(i, 1);
    }

#endif

    decoder->lock = aicos_mutex_create();
    if (!decoder->lock) {
        pr_err("Decoder: create mutex failed\n");
        return -1;
    }

    decoder->data_ready_event = aicos_event_create();
    if (!decoder->data_ready_event) {
        pr_err("Decoder: create event failed\n");
        aicos_mutex_delete(decoder->lock);
        return -1;
    }

    decoder->ready = false;
    return 0;
}

void barcode_decoder_deinit(barcode_decoder_t *decoder)
{
    if (!decoder) {
        return;
    }

    if (decoder->data_ready_event) {
        aicos_event_delete(decoder->data_ready_event);
        decoder->data_ready_event = NULL;
    }

    if (decoder->lock) {
        aicos_mutex_delete(decoder->lock);
        decoder->lock = NULL;
    }

    memset(decoder, 0, sizeof(barcode_decoder_t));
}

int barcode_decoder_process(barcode_decoder_t *decoder,
                           unsigned char *img, int width, int height)
{
#ifdef BARCODE_ENABLE_DECODER
    int len = 0;

    if (!decoder || !img || width <= 0 || height <= 0) {
        pr_err("Decoder: invalid params - decoder=%p, img=%p, w=%d, h=%d\n",
               decoder, img, width, height);
        return -1;
    }

    // Check if decoder is properly initialized
    if (!decoder->lock) {
        pr_err("Decoder: not initialized (lock is NULL)\n");
        return -1;
    }

    // Reset ready flag before processing new frame
    aicos_mutex_take(decoder->lock, AICOS_WAIT_FOREVER);
    decoder->ready = false;
    aicos_mutex_give(decoder->lock);

    Decoding_Image(img, width, height);

    len = GetResultLength();
    if (len > 0 && len < BARCODE_MAX_BUF_LEN) {
        aicos_mutex_take(decoder->lock, AICOS_WAIT_FOREVER);
        memset(decoder->result, 0, sizeof(decoder->result));
        GetDecoderResult(decoder->result);
        pr_info("Decoder: GetDecoderResult returned result=%s\n", decoder->result);

        // Don't check ret value, just use the result if length > 0
        decoder->result_len = len;

        aicos_mutex_give(decoder->lock);

        return 0;  // Always return success if we got a result with len > 0
    }
#endif
    return -1;
}

bool barcode_decoder_get_result(barcode_decoder_t *decoder,
                                unsigned char *out, int max_len)
{
    if (!decoder || !out || max_len <= 0) {
        return false;
    }

    // Check if decoder is properly initialized
    if (!decoder->lock) {
        return false;
    }

    bool has_result = false;
    aicos_mutex_take(decoder->lock, AICOS_WAIT_FOREVER);
    if (decoder->result_len > 0 && decoder->result_len < max_len) {
        memcpy(out, decoder->result, decoder->result_len);
        pr_info("Decoder: result = %s\n", out);
        has_result = true;
    }
    aicos_mutex_give(decoder->lock);

    return has_result;
}

void barcode_decoder_signal_ready(barcode_decoder_t *decoder)
{
    if (!decoder) {
        return;
    }

    // Check if decoder is properly initialized
    if (!decoder->lock || !decoder->data_ready_event) {
        pr_warn("Decoder: signal_ready called but not initialized\n");
        return;
    }

    aicos_mutex_take(decoder->lock, AICOS_WAIT_FOREVER);
    decoder->ready = true;
    aicos_mutex_give(decoder->lock);

    aicos_event_send(decoder->data_ready_event, BARCODE_DATA_READY_EVENT);
}

int barcode_decoder_wait_data(barcode_decoder_t *decoder, u32 timeout_ms)
{
    uint32_t recved;

    if (!decoder) {
        return -1;
    }

    // Check if decoder is properly initialized
    if (!decoder->data_ready_event) {
        pr_err("Decoder: wait_data called but not initialized\n");
        return -1;
    }

    return aicos_event_recv(decoder->data_ready_event,
                           BARCODE_DATA_READY_EVENT,
                           &recved,
                           timeout_ms);
}
