/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <asn1_decoder.h>

extern const struct asn1_decoder rsapubkey_decoder;

extern int rsa_get_e(void *, const void *, size_t);
extern int rsa_get_n(void *, const void *, size_t);
