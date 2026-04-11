/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <asn1_decoder.h>

extern const struct asn1_decoder rsaprivkey_decoder;

extern int rsa_get_d(void *, const void *, size_t);
extern int rsa_get_dp(void *, const void *, size_t);
extern int rsa_get_dq(void *, const void *, size_t);
extern int rsa_get_e(void *, const void *, size_t);
extern int rsa_get_n(void *, const void *, size_t);
extern int rsa_get_p(void *, const void *, size_t);
extern int rsa_get_q(void *, const void *, size_t);
extern int rsa_get_qinv(void *, const void *, size_t);
