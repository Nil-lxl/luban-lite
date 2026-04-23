/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _SM2_API_H_
#define _SM2_API_H_

int aic_sm2_sign(int flen, unsigned char *from, unsigned char *to,
                     struct ak_options *opts);
int aic_sm2_verify(int flen, unsigned char *from, unsigned char *to,
                    struct ak_options *opts);
int aic_sm2_pub_enc(int flen, unsigned char *from, unsigned char *to,
                     struct ak_options *opts);
int aic_sm2_priv_dec(int flen, unsigned char *from, unsigned char *to,
                    struct ak_options *opts);
#endif
