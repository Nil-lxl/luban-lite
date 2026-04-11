/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef MBEDTLS_CRYPTO_H
#define MBEDTLS_CRYPTO_H

#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <mbedtls/base64.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AES_128_BLOCK_SIZE 16
typedef void sha_ctx_t;
typedef void aes_ctx_t;

typedef enum {
    AES_ENCRYPT = 0,
    AES_DECRYPT,
} aes_direction_t;

sha_ctx_t *sha_init(void);
int sha_update(sha_ctx_t *ctx, const uint8_t *in, int len);
int sha_final(sha_ctx_t *ctx, const uint8_t *out, int len);
int sha_reset(sha_ctx_t *ctx);
void sha_destroy(sha_ctx_t *ctx);

aes_ctx_t *aes_ctr_init(const uint8_t *key, const uint8_t *iv);
int aes_ctr_encrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len);
void aes_ctr_start_fresh_block(aes_ctx_t *ctx);
int aes_ctr_decrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len);
void aes_ctr_reset(aes_ctx_t *ctx);
void aes_ctr_destroy(aes_ctx_t *ctx);

aes_ctx_t *aes_cbc_init(const uint8_t *key, const uint8_t *iv, aes_direction_t direction);
void aes_cbc_reset(aes_ctx_t *ctx);
int aes_cbc_encrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len);
int aes_cbc_decrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len);
void aes_cbc_destroy(aes_ctx_t *ctx);

int aes_gcm_encrypt(const unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext,
                    unsigned char *key, unsigned char *iv, unsigned char *tag);
int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext,
                    unsigned char *key, unsigned char *iv, unsigned char *tag);

int get_random_bytes(unsigned char *buf, int num);
void pk_to_base64(const unsigned char *pk, int pk_len, unsigned char *pk_base64, int len);
#ifdef __cplusplus
}
#endif
#endif
