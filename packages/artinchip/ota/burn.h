/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#ifndef __BURN_H__
#define __BURN_H__

#ifdef __cplusplus
extern "C" {
#endif

int aic_ota_find_part(char *partname);
int aic_ota_erase_part(void);
int aic_ota_part_write(uint32_t addr, const uint8_t *buf, size_t size);
int aic_ota_part_read(uint32_t addr, uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif
