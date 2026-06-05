/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include "aic_common.h"

#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

int flash_init(void);
int flash_read(u32 offset, uint8_t *data, u32 len);
int flash_write(u32 offset, uint8_t *data, u32 len);
int flash_erase(u32 offset, u32 len);

#ifdef __cplusplus
}
#endif

#endif
