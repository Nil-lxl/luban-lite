/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include "crypto.h"

struct aes_ctx_s {
    mbedtls_aes_context cipher_ctx;
    uint8_t key[AES_128_BLOCK_SIZE];
    uint8_t iv[AES_128_BLOCK_SIZE];
    aes_direction_t direction;
    uint8_t block_offset;
    size_t ctr_nc_off;
    uint8_t ctr_nonce_counter[AES_128_BLOCK_SIZE];
    uint8_t ctr_encrypted_counter[AES_128_BLOCK_SIZE];
};

#define SALT_PK "UxPlay-Persistent-Not-Secure-Public-Key"

uint8_t waste[AES_128_BLOCK_SIZE];

sha_ctx_t *sha_init(void)
{
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t *ctx;
    int ret = 0;

    ctx = malloc(sizeof(mbedtls_md_context_t));
    assert(ctx != NULL);
    if (!ctx) {
        printf("Allocater context failure\n");
        return NULL;
    }

    mbedtls_md_init(ctx);
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    if (!md_info)
        goto cleanup;

    if ((ret = mbedtls_md_setup(ctx, md_info, 0)) != 0)
        goto cleanup;

    if ((ret = mbedtls_md_starts(ctx)) != 0)
        goto cleanup;

    return (sha_ctx_t *)ctx;

cleanup:
    if (ctx)
        free(ctx);
    return NULL;
}

int sha_update(sha_ctx_t *ctx, const uint8_t *in, int len)
{
    mbedtls_md_context_t *md_ctx = (mbedtls_md_context_t *)ctx;

    if (!ctx) {
        printf("Invalid parameter\n");
        return -1;
    }
    return mbedtls_md_update(md_ctx, (const unsigned char *)in, (size_t)len);
}

int sha_final(sha_ctx_t *ctx, const uint8_t *out, int len)
{
    mbedtls_md_context_t *md_ctx = (mbedtls_md_context_t *)ctx;
    if (!ctx || len != 64) {
        printf("Invalid parameter\n");
        return -1;
    }

    return mbedtls_md_finish(md_ctx, (unsigned char *)out);
}

int sha_reset(sha_ctx_t *ctx)
{
    mbedtls_md_context_t *md_ctx = (mbedtls_md_context_t *)ctx;
    int ret = 0;

    if (!ctx) {
        printf("Invalid parameter\n");
        return -1;
    }
    if ((ret = mbedtls_md_starts(md_ctx)) != 0)
        return ret;

    return 0;
}

void sha_destroy(sha_ctx_t *ctx)
{
    mbedtls_md_context_t *md_ctx = (mbedtls_md_context_t *)ctx;

    if (ctx) {
        mbedtls_md_free(md_ctx);
        free(ctx);
    }
}

aes_ctx_t *aes_ctr_init(const uint8_t *key, const uint8_t *iv)
{
    struct aes_ctx_s *ctx;
    int ret = 0;

    if (!key || !iv) {
        printf("Invalid parameter\n");
        return NULL;
    }

    ctx = malloc(sizeof(struct aes_ctx_s));
    if (!ctx) {
        printf("Fail to alloc context\n");
        return NULL;
    }
    memset(ctx, 0, sizeof(*ctx));
    memcpy(ctx->key, key, AES_128_BLOCK_SIZE);
    memcpy(ctx->iv, iv, AES_128_BLOCK_SIZE); /* Backup */
    memcpy(ctx->ctr_nonce_counter, iv, AES_128_BLOCK_SIZE);
    mbedtls_aes_init(&ctx->cipher_ctx);

    ret = mbedtls_aes_setkey_enc(&ctx->cipher_ctx, key, 128);
    if (ret) {
        printf("Set AES key failure.\n");
        goto cleanup;
    }

    return (aes_ctx_t *)ctx;

cleanup:
    if (ctx)
        free(ctx);
    return NULL;
}

int aes_ctr_encrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;
    int ret = 0;

    if (!ctx) {
        printf("AES CTR Invalid parameter\n");
        return -1;
    }

    ret = mbedtls_aes_crypt_ctr(&aes_ctx->cipher_ctx, len, &aes_ctx->ctr_nc_off,
                                aes_ctx->ctr_nonce_counter, aes_ctx->ctr_encrypted_counter,
                                in, out);
    if (ret)
        return ret;

    aes_ctx->block_offset = (aes_ctx->block_offset + len) % AES_128_BLOCK_SIZE;
    return ret;
}

void aes_ctr_start_fresh_block(aes_ctx_t *ctx)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;

    // Is there a better way to do this?
    if (aes_ctx->block_offset == 0)
        return;
    aes_ctr_encrypt(ctx, waste, waste, AES_128_BLOCK_SIZE - aes_ctx->block_offset);
}

int aes_ctr_decrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;
    int ret = 0;

    if (!ctx) {
        printf("AES CTR Invalid parameter\n");
        return -1;
    }

    ret = mbedtls_aes_crypt_ctr(&aes_ctx->cipher_ctx, len, &aes_ctx->ctr_nc_off,
                                aes_ctx->ctr_nonce_counter, aes_ctx->ctr_encrypted_counter,
                                in, out);
    return ret;
}

void aes_ctr_reset(aes_ctx_t *ctx)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;

    if (!ctx) {
        printf("AES CTR Invalid parameter\n");
        return;
    }
    aes_ctx->block_offset = 0;
    aes_ctx->ctr_nc_off = 0;
    memcpy(aes_ctx->ctr_nonce_counter, aes_ctx->iv, AES_128_BLOCK_SIZE);
    memset(aes_ctx->ctr_encrypted_counter, 0, AES_128_BLOCK_SIZE);
}

void aes_ctr_destroy(aes_ctx_t *ctx)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;

    if (!ctx) {
        printf("AES CTR Invalid parameter\n");
        return;
    }
    mbedtls_aes_free(&aes_ctx->cipher_ctx);
    free(aes_ctx);
}

aes_ctx_t *aes_cbc_init(const uint8_t *key, const uint8_t *iv, aes_direction_t direction)
{
    struct aes_ctx_s *ctx;
    int ret = 0;

    if (!key || !iv) {
        printf("Invalid parameter\n");
        return NULL;
    }

    ctx = malloc(sizeof(struct aes_ctx_s));
    if (!ctx) {
        printf("Fail to alloc context\n");
        return NULL;
    }
    memset(ctx, 0, sizeof(*ctx));
    memcpy(ctx->key, key, AES_128_BLOCK_SIZE);
    memcpy(ctx->iv, iv, AES_128_BLOCK_SIZE);
    mbedtls_aes_init(&ctx->cipher_ctx);
    ctx->direction = direction;

    if (direction == AES_ENCRYPT)
        ret = mbedtls_aes_setkey_enc(&ctx->cipher_ctx, key, 128);
    else
        ret = mbedtls_aes_setkey_dec(&ctx->cipher_ctx, key, 128);
    if (ret) {
        printf("Set AES key failure.\n");
        goto cleanup;
    }

    return (aes_ctx_t *)ctx;

cleanup:
    if (ctx)
        free(ctx);
    return NULL;
}

int aes_cbc_encrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;
    int ret = 0;

    if (!ctx) {
        printf("AES CBC Invalid parameter\n");
        return -1;
    }

    if (aes_ctx->direction == AES_ENCRYPT) {
        ret = mbedtls_aes_crypt_cbc(&aes_ctx->cipher_ctx, MBEDTLS_AES_ENCRYPT, len, aes_ctx->iv,
                                    in, out);
    } else {
        printf("AES CBC crypt direction is wrong.\n");
        return -1;
    }
    return ret;
}

int aes_cbc_decrypt(aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int len)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;
    int ret = 0;

    if (!ctx) {
        printf("AES CBC Invalid parameter\n");
        return -1;
    }

    if (aes_ctx->direction == AES_DECRYPT) {
        ret = mbedtls_aes_crypt_cbc(&aes_ctx->cipher_ctx, MBEDTLS_AES_DECRYPT, len, aes_ctx->iv,
                                    in, out);
    } else {
        printf("AES CBC crypt direction is wrong.\n");
        return -1;
    }
    return ret;
}

void aes_cbc_reset(aes_ctx_t *ctx)
{
}

void aes_cbc_destroy(aes_ctx_t *ctx)
{
    struct aes_ctx_s *aes_ctx = (struct aes_ctx_s *)ctx;

    if (!ctx) {
        printf("AES CBC Invalid parameter\n");
        return;
    }
    mbedtls_aes_free(&aes_ctx->cipher_ctx);
    free(aes_ctx);
}

int aes_gcm_encrypt(const unsigned char *plaintext, int plaintext_len, unsigned char *ciphertext,
                    unsigned char *key, unsigned char *iv, unsigned char *tag)
{
    mbedtls_gcm_context gcm_ctx;
    int ret = 0;

    mbedtls_gcm_init(&gcm_ctx);
    ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret) {
        printf("Set key failed.\n");
        return 0;
    }

    /*IV and tag size is 16 */
    ret = mbedtls_gcm_crypt_and_tag(&gcm_ctx, MBEDTLS_GCM_ENCRYPT, plaintext_len, iv, 16, NULL, 0,
                                    plaintext, ciphertext, 16, tag);
    if (ret) {
        printf("GCM crypt failed.\n");
        return 0;
    }

    mbedtls_gcm_free(&gcm_ctx);
    return plaintext_len;
}

int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *plaintext,
                    unsigned char *key, unsigned char *iv, unsigned char *tag)
{
    mbedtls_gcm_context gcm_ctx;
    int ret = 0;

    mbedtls_gcm_init(&gcm_ctx);
    ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret) {
        printf("Set key failed.\n");
        return 0;
    }

    /*IV and tag size is 16 */
    ret = mbedtls_gcm_crypt_and_tag(&gcm_ctx, MBEDTLS_GCM_DECRYPT, ciphertext_len, iv, 16, NULL, 0,
                                    ciphertext, plaintext, 16, tag);
    if (ret) {
        printf("GCM crypt failed.\n");
        return 0;
    }

    mbedtls_gcm_free(&gcm_ctx);
    return ciphertext_len;
}

int get_random_bytes(unsigned char *buf, int num)
{
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    char *per = "personalized string for crypto";
    int ret;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                mbedtls_entropy_func,
                                &entropy,
                                (const unsigned char *)per,
                                strlen(per));
    if (ret) {
        goto cleanup;
    }

    ret = mbedtls_ctr_drbg_random(&ctr_drbg, buf, num);
    if (ret) {
        goto cleanup;
    }

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return 1;

cleanup:
    return 0;
}

void pk_to_base64(const unsigned char *pk, int pk_len, unsigned char *pk_base64, int len)
{
    size_t dlen, slen;

    dlen = len;
    slen = pk_len;
    mbedtls_base64_encode(pk_base64, dlen, &dlen, pk, slen);
}

