/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef __SM2_HELPER_
#define __SM2_HELPER_

#include <aic_core.h>
#include "akcipher.h"

/**
 * sm2_key - SM2 key structure
 * @x           : SM2 public x raw byte stream
 * @y           : SM2 public y raw byte stream
 * @d           : SM2 private exponent raw byte stream
 * @x_sz        : length in bytes of SM2 public x
 * @y_sz        : length in bytes of SM2 public y
 * @d_sz        : length in bytes of SM2 private exponent
 */
struct sm2_key {
	const u8 *x;
	const u8 *y;
	const u8 *d;
	size_t x_sz;
	size_t y_sz;
	size_t d_sz;
};

int sm2_parse_pub_key(struct sm2_key *sm2_key, const void *key,
		      unsigned int key_len);

int sm2_parse_priv_key(struct sm2_key *sm2_key, const void *key,
		       unsigned int key_len);

int aic_akcipher_sm2_sign(struct aic_akcipher_handle *handle);
int aic_akcipher_sm2_verify(struct aic_akcipher_handle *handle);
int aic_akcipher_sm2_encrypt(struct aic_akcipher_handle *handle);
int aic_akcipher_sm2_decrypt(struct aic_akcipher_handle *handle);
int aic_akcipher_sm2_set_pub_key(struct aic_akcipher_handle *handle,
                                 const void *key, unsigned int keylen);
int aic_akcipher_sm2_set_priv_key(struct aic_akcipher_handle *handle,
                                  const void *key, unsigned int keylen);
u32 aic_akcipher_sm2_max_size(struct aic_akcipher_handle *handle);

#endif
