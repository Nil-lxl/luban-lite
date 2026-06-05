/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#ifndef BARCODE_DECODER_H
#define BARCODE_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <rtthread.h>
#include "aic_osal.h"

#include "config/barcode_config.h"

typedef struct {
    unsigned char result[BARCODE_MAX_BUF_LEN];
    int result_len;
    bool ready;
    aicos_mutex_t lock;
    aicos_event_t data_ready_event;
} barcode_decoder_t;

/**
 * Initialize barcode decoder
 * @param decoder Decoder handle
 * @return 0 on success, negative value on failure
 */
int barcode_decoder_init(barcode_decoder_t *decoder);

/**
 * Deinitialize barcode decoder
 * @param decoder Decoder handle
 */
void barcode_decoder_deinit(barcode_decoder_t *decoder);

/**
 * Process image and decode barcode
 * @param decoder Decoder handle
 * @param img Image data
 * @param width Image width
 * @param height Image height
 * @return 0 on success, negative value on failure
 */
int barcode_decoder_process(barcode_decoder_t *decoder,
                            unsigned char *img, int width, int height);

/**
 * Get decoded result
 * @param decoder Decoder handle
 * @param out Output buffer
 * @param max_len Maximum length
 * @return true if result available, false otherwise
 */
bool barcode_decoder_get_result(barcode_decoder_t *decoder,
                                unsigned char *out, int max_len);

/**
 * Signal that data is ready
 * @param decoder Decoder handle
 */
void barcode_decoder_signal_ready(barcode_decoder_t *decoder);

/**
 * Wait for data to be ready
 * @param decoder Decoder handle
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, negative value on failure
 */
int barcode_decoder_wait_data(barcode_decoder_t *decoder, u32 timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BARCODE_DECODER_H */
