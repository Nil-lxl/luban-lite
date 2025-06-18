/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <spinand.h>
#include <bbt.h>

int nand_bbt_init(struct aic_spinand *flash)
{
    u32 nblocks;

    nblocks = flash->info->block_per_lun;
    flash->bbt.cache = calloc(1, nblocks);
    if (!flash->bbt.cache)
        return -SPINAND_ERR;

    return SPINAND_SUCCESS;
}

bool nand_bbt_is_initialized(struct aic_spinand *flash)
{
    return !!flash->bbt.cache;
}

void nand_bbt_cleanup(struct aic_spinand *flash)
{
    free(flash->bbt.cache);
}

int nand_bbt_get_block_status(struct aic_spinand *flash, u32 block)
{
    u8 *cache = flash->bbt.cache + block;

    return (cache[0] & BBT_BLOCK_STATUS_MSK);
}

void nand_bbt_set_block_status(struct aic_spinand *flash, u32 block, u32 status)
{
    u8 *cache = flash->bbt.cache + block;

    cache[0] &= ~BBT_BLOCK_STATUS_MSK;
    cache[0] |= (status & BBT_BLOCK_STATUS_MSK);
}
