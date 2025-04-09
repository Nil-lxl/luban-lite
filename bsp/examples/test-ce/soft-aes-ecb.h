/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef _SOFT_AES_ECB_H_
#define _SOFT_AES_ECB_H_

#include <stdint.h>
#include <stddef.h>

#define AES128 1
#define AES_BLOCKLEN 16 // Block length in bytes - AES is 128b block only
#define AES_KEYLEN 16 // Key length in bytes
#define AES_ROUNDKEYSIZE 176

struct aes_ctx {
	unsigned char round_key[AES_ROUNDKEYSIZE];
};

void aes_init_ctx(struct aes_ctx *ctx, const unsigned char *key);
void aes_128_ecb_encrypt(const struct aes_ctx *ctx, unsigned char *buf);
void aes_128_ecb_decrypt(const struct aes_ctx *ctx, unsigned char *buf);

#endif // _SOFT_AES_ECB_H_
