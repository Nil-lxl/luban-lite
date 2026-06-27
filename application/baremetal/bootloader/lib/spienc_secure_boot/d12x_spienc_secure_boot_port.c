/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <boot_param.h>
#include <private_param.h>
#include <ChaCha20.h>

int fit_image_get_cipher_key(const char *algo, const char *key_name_hint,
                             u8 *key, int key_len)
{
    struct private_security_keydata_v1 *key_data;

    if (key == NULL)
        return -1;

    if (strncmp(algo, "chacha20", 8) != 0) {
        return -1;
    }
    key_data = get_security_keydata(aic_get_boot_resource(), KEYDATA_VERSION_V1);
    if (key_data == NULL) {
        return -1;
    }

    memcpy(key, key_data->key_enc, 32);
    return 0;
}

int fit_image_chacha20_decrypt(const u8 *key, const u8 *nonce, u8 *data,
                               unsigned int length)
{
    ChaCha20_Ctx ctx;

    ChaCha20_init(&ctx, key, nonce, 0);
    ChaCha20_xor(&ctx, data, length);
    memset(&ctx, 0, sizeof(ctx));

    return 0;
}

