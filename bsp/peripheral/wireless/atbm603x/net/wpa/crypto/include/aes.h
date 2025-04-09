/*
 * AES functions
 * Copyright (c) 2003-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef AES_H
#define AES_H

#define AES_BLOCK_SIZE 16

atbm_void * atbmwifi_aes_encrypt_init(const atbm_uint8 *key, atbm_size_t len);
atbm_void atbmwifi_aes_encrypt(atbm_void *ctx, const atbm_uint8 *plain, atbm_uint8 *crypt);
atbm_void atbmwifi_aes_encrypt_deinit(atbm_void *ctx);
atbm_void * atbmwifi_aes_decrypt_init(const atbm_uint8 *key, atbm_size_t len);
atbm_void atbmwifi_aes_decrypt(atbm_void *ctx, const atbm_uint8 *crypt, atbm_uint8 *plain);
atbm_void atbmwifi_aes_decrypt_deinit(atbm_void *ctx);
int atbmwifi_omac1_aes_128(const atbm_uint8 *key, const atbm_uint8 *data, atbm_size_t data_len, atbm_uint8 *mac);
int atbmwifi_aes_128_cbc_encrypt(const atbm_uint8 *key, const atbm_uint8 *iv, atbm_uint8 *data, atbm_size_t data_len);
int atbmwifi_aes_128_cbc_decrypt(const atbm_uint8 *key, const atbm_uint8 *iv, atbm_uint8 *data, atbm_size_t data_len);
#endif /* AES_H */
