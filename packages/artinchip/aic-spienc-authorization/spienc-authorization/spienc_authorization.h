/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */
#ifndef _AIC_SPIENC_AUTHORIZATION_SPIE_H_
#define _AIC_SPIENC_AUTHORIZATION_SPIE_H_

#include <aic_core.h>

#ifdef __cplusplus
extern "C" {
#endif

int aic_rng_get_bytes(u8 *buf, unsigned int len);
int aic_spie_sk_sign(int dlen, unsigned char *data, unsigned char *output, unsigned char *sign_key);
int aic_spie_vk_verify(int dlen, unsigned char *data, unsigned char *output, unsigned char *verify_key);
int aic_spie_vk_sign(int dlen, unsigned char *data, unsigned char *output, unsigned char *verify_key);
int aic_spie_sk_verify(int dlen, unsigned char *data, unsigned char *output, unsigned char *sign_key);

#ifdef __cplusplus
}
#endif

#endif /* _AIC_SPIENC_AUTHORIZATION_SPIE_H_ */
