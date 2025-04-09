/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include "ssram.h"

static unsigned int ssram_bitmap = 0;
unsigned int aic_ssram_alloc(unsigned int siz)
{
    u64 mask;
    int i, unitcnt;
    unsigned int addr;

    if (siz <= 0)
        return 0;

    unitcnt = DIV_ROUND_UP(siz, SSRAM_UNIT_SIZE);

    mask = GENMASK(unitcnt - 1, 0);

    for (i = 0; i < (SECURE_SRAM_SIZE / SSRAM_UNIT_SIZE); i++) {
        mask = (mask << i);
        if (!(ssram_bitmap & mask)) {
            ssram_bitmap |= mask;
            break;
        }
    }

    if (i >= (SECURE_SRAM_SIZE / SSRAM_UNIT_SIZE))
        return 0;

    addr = (unsigned int)SECURE_SRAM_BASE;
    addr += (SSRAM_UNIT_SIZE * i);
    return addr;
}

int aic_ssram_free(unsigned int ssram_addr, unsigned int siz)
{
    unsigned int addr;
    int i, unitcnt;
    unsigned int mask;

    if (siz <= 0)
        return 0;

    addr = (unsigned int)SECURE_SRAM_BASE;
    if (ssram_addr < addr)
        return -EINVAL;

    i = (ssram_addr - addr) / SSRAM_UNIT_SIZE;
    if (i >= SECURE_SRAM_SIZE / SSRAM_UNIT_SIZE)
        return -EINVAL;

    unitcnt = DIV_ROUND_UP(siz, SSRAM_UNIT_SIZE);
    mask = GENMASK(unitcnt - 1 + i, i);

    ssram_bitmap &= (~mask);

    return 0;
}
