/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <asn1_decoder.h>

extern const struct asn1_decoder sm2privkey_decoder;

extern int sm2_get_d(void *, const void *, size_t);
extern int sm2_get_p(void *, const void *, size_t);

