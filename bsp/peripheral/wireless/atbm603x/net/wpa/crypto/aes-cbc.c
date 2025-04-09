/*
 * AES-128 CBC
 *
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"

//#include "mbedtls/config_atbm.h"
#include "atbm_type.h"
#include "aes.h"

#if(ATBM_HW_CHIPER==0)
/**
 * atbmwifi_aes_128_cbc_encrypt - AES-128 CBC encryption
 * @key: Encryption key
 * @iv: Encryption IV for CBC mode (16 bytes)
 * @data: Data to encrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
 */
int atbmwifi_aes_128_cbc_encrypt(const atbm_uint8 *key, const atbm_uint8 *iv, atbm_uint8 *data, atbm_size_t data_len)
{
	void *ctx;
	atbm_uint8 cbc[AES_BLOCK_SIZE];
	atbm_uint8 *pos = data;
	int i, j, blocks;

	ctx = atbmwifi_aes_encrypt_init(key, 16);
	if (ctx == NULL)
		return -1;
	atbm_memcpy(cbc, iv, AES_BLOCK_SIZE);

	blocks = data_len / AES_BLOCK_SIZE;
	for (i = 0; i < blocks; i++) {
		for (j = 0; j < AES_BLOCK_SIZE; j++)
			cbc[j] ^= pos[j];
		atbmwifi_aes_encrypt(ctx, cbc, cbc);
		atbm_memcpy(pos, cbc, AES_BLOCK_SIZE);
		pos += AES_BLOCK_SIZE;
	}
	atbmwifi_aes_encrypt_deinit(ctx);
	return 0;
}


/**
 * atbmwifi_aes_128_cbc_decrypt - AES-128 CBC decryption
 * @key: Decryption key
 * @iv: Decryption IV for CBC mode (16 bytes)
 * @data: Data to decrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
 */
int atbmwifi_aes_128_cbc_decrypt(const atbm_uint8 *key, const atbm_uint8 *iv, atbm_uint8 *data, atbm_size_t data_len)
{
	void *ctx;
	atbm_uint8 cbc[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	atbm_uint8 *pos = data;
	int i, j, blocks;

	ctx = atbmwifi_aes_decrypt_init(key, 16);
	if (ctx == NULL)
		return -1;
	atbm_memcpy(cbc, iv, AES_BLOCK_SIZE);

	blocks = data_len / AES_BLOCK_SIZE;
	for (i = 0; i < blocks; i++) {
		atbm_memcpy(tmp, pos, AES_BLOCK_SIZE);
		atbmwifi_aes_decrypt(ctx, pos, pos);
		for (j = 0; j < AES_BLOCK_SIZE; j++)
			pos[j] ^= cbc[j];
		atbm_memcpy(cbc, tmp, AES_BLOCK_SIZE);
		pos += AES_BLOCK_SIZE;
	}
	atbmwifi_aes_decrypt_deinit(ctx);
	return 0;
}

static void gf_mulx(atbm_uint8 *pad)
{
	int i, carry;

	carry = pad[0] & 0x80;
	for (i = 0; i < AES_BLOCK_SIZE - 1; i++)
		pad[i] = (pad[i] << 1) | (pad[i + 1] >> 7);
	pad[AES_BLOCK_SIZE - 1] <<= 1;
	if (carry)
		pad[AES_BLOCK_SIZE - 1] ^= 0x87;
}


int atbmwifi_omac1_aes_vector(const atbm_uint8 *key, atbm_size_t key_len, atbm_size_t num_elem,
		     const atbm_uint8 *addr[], const atbm_size_t *len, atbm_uint8 *mac)
{
	void *ctx;
	atbm_uint8 cbc[AES_BLOCK_SIZE], pad[AES_BLOCK_SIZE];
	const atbm_uint8 *pos, *end;
	atbm_size_t i, e, left, total_len;

	ctx = atbmwifi_aes_encrypt_init(key, key_len);
	if (ctx == NULL)
		return -1;
	atbm_memset(cbc, 0, AES_BLOCK_SIZE);

	total_len = 0;
	for (e = 0; e < num_elem; e++)
		total_len += len[e];
	left = total_len;

	e = 0;
	pos = addr[0];
	end = pos + len[0];

	while (left >= AES_BLOCK_SIZE) {
		for (i = 0; i < AES_BLOCK_SIZE; i++) {
			cbc[i] ^= *pos++;
			if (pos >= end) {
				/*
				 * Stop if there are no more bytes to process
				 * since there are no more entries in the array.
				 */
				if (i + 1 == AES_BLOCK_SIZE &&
				    left == AES_BLOCK_SIZE)
					break;
				e++;
				pos = addr[e];
				end = pos + len[e];
			}
		}
		if (left > AES_BLOCK_SIZE)
			atbmwifi_aes_encrypt(ctx, cbc, cbc);
		left -= AES_BLOCK_SIZE;
	}

	atbm_memset(pad, 0, AES_BLOCK_SIZE);
	atbmwifi_aes_encrypt(ctx, pad, pad);
	gf_mulx(pad);

	if (left || total_len == 0) {
		for (i = 0; i < left; i++) {
			cbc[i] ^= *pos++;
			if (pos >= end) {
				/*
				 * Stop if there are no more bytes to process
				 * since there are no more entries in the array.
				 */
				if (i + 1 == left)
					break;
				e++;
				pos = addr[e];
				end = pos + len[e];
			}
		}
		cbc[left] ^= 0x80;
		gf_mulx(pad);
	}

	for (i = 0; i < AES_BLOCK_SIZE; i++)
		pad[i] ^= cbc[i];
	atbmwifi_aes_encrypt(ctx, pad, mac);
	atbmwifi_aes_encrypt_deinit(ctx);
	return 0;
}

int atbmwifi_omac1_aes_128_vector(const atbm_uint8 *key, atbm_size_t num_elem,
		  const atbm_uint8 *addr[], const atbm_size_t *len, atbm_uint8 *mac)
{
	return atbmwifi_omac1_aes_vector(key, 16, num_elem, addr, len, mac);
}

int atbmwifi_omac1_aes_128(const atbm_uint8 *key, const atbm_uint8 *data, atbm_size_t data_len, atbm_uint8 *mac)
{
	return atbmwifi_omac1_aes_128_vector(key, 1, &data, &data_len, mac);
}

#else//(ATBM_HW_CHIPER==1)
/**
 * atbmwifi_aes_128_cbc_encrypt - AES-128 CBC encryption
 * @key: Encryption key
 * @iv: Encryption IV for CBC mode (16 bytes)
 * @data: Data to encrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
 */
int atbmwifi_aes_128_cbc_encrypt(const atbm_uint8 *key, const atbm_uint8 *iv, atbm_uint8 *data, atbm_size_t data_len)
{
    ATBM_set_hardware(ATBM_CHIPER_TYPE_AES128,ATBM_CHIPER_MODE_CBC,key,(128/8),iv,data,data,data_len,1,0);
	return 0;
}
/**
 * atbmwifi_aes_128_cbc_decrypt - AES-128 CBC decryption
 * @key: Decryption key
 * @iv: Decryption IV for CBC mode (16 bytes)
 * @data: Data to decrypt in-place
 * @data_len: Length of data in bytes (must be divisible by 16)
 * Returns: 0 on success, -1 on failure
 */
int atbmwifi_aes_128_cbc_decrypt(const atbm_uint8 *key, const atbm_uint8 *iv, atbm_uint8 *data, atbm_size_t data_len)
{
    ATBM_set_hardware(ATBM_CHIPER_TYPE_AES128,ATBM_CHIPER_MODE_CBC,key,(128/8),iv,data,data,data_len,0,0);
	return 0;

}

#endif //(ATBM_HW_CHIPER==0)
