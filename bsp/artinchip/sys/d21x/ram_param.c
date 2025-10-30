/*
 * Copyright (C) 2024-2025 ArtInChip Technology Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <aic_core.h>
#include <aic_common.h>
#include <ram_param.h>

#define EFUSE_CMU_REG ((void *)0x18020904)
#define EFUSE_SHADOW_FEATURE_REG ((void *)0x19010224)
#define DDR2_32MB 0xA
#define DDR2_64MB 0xB
#define DDR3_128MB 0xC
#define DDR3_256MB 0xD

static u32 efuse_get_ddr_size(void)
{
    u32 val, mem, size = 0;

    writel(0x1100, EFUSE_CMU_REG);
    val = readl(EFUSE_SHADOW_FEATURE_REG);

    mem = (val >> 20) & 0xF;
    switch (mem) {
        case DDR2_32MB:
            pr_info("DDR2 32MB\n");
            size = 0x2000000;
            break;
        case DDR2_64MB:
            pr_info("DDR2 64MB\n");
            size = 0x4000000;
            break;
        case DDR3_128MB:
            pr_info("DDR3 128MB\n");
            size = 0x8000000;
            break;
        case DDR3_256MB:
            pr_info("DDR3 256MB\n");
            size = 0x10000000;
            break;
        default:
            pr_info("No DDR info\n");
    }
    writel(0x0, EFUSE_CMU_REG);

    return size;
}

u32 aic_get_ram_size(void)
{
    return efuse_get_ddr_size();
}
