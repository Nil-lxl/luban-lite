/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
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
#ifdef AIC_SPINOR_DRV
int aic_ota_nor_erase_part(void);
#endif
#ifdef AIC_SPINAND_DRV
int aic_ota_nand_erase_part(void);
int aic_ota_nand_write(uint32_t addr, const uint8_t *buf, size_t size);
#endif
int aic_ota_erase_part(void);
int aic_ota_part_write(uint32_t addr, const uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif
