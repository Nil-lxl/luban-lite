/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __BBT_H__
#define __BBT_H__

#include <spinand.h>

#define BBT_BLOCK_UNKNOWN		0x00
#define BBT_BLOCK_GOOD		    0x01
#define BBT_BLOCK_RESERVED	    0x02
#define BBT_BLOCK_FACTORY_BAD	0x03

int nand_bbt_init(struct aic_spinand *flash);
bool nand_bbt_is_initialized(struct aic_spinand *flash);
void nand_bbt_cleanup(struct aic_spinand *flash);
int nand_bbt_get_block_status(struct aic_spinand *flash, u32 block);
void nand_bbt_set_block_status(struct aic_spinand *flash, u32 block, u32 pos_block,
                               u32 status);

#endif /* __BBT_H__ */
