/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_SSRAM_H_
#define _AIC_SSRAM_H_

#include <aic_core.h>
#include <hal_ce.h>

/* SSRAM allocate unit size is 16 bytes */
#define SSRAM_UNIT_SIZE 16

unsigned int aic_ssram_alloc(unsigned int siz);
int aic_ssram_free(unsigned int ssram_addr, unsigned int siz);
int aic_ssram_aes_genkey(unsigned long efuse_key, unsigned char *key_material,
                         unsigned int mlen, unsigned int ssram_addr);
int aic_ssram_des_genkey(unsigned long efuse_key, unsigned char *key_material,
                         unsigned int mlen, unsigned int ssram_addr);

#endif
