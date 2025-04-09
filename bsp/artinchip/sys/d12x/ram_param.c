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
#define EFUSE_218_REG ((void *)0x19010218)
#define EFUSE_21C_REG ((void *)0x1901021c)

#define PSRAM_SINGLE   0
#define PSRAM_PARALLEL 1

enum PSRAM_FUSE_ID {
    APS3208K = 0x00,
    SCKW18_12816O = 0x01,
    APS12816O = 0x02,
};

struct _psram_id {
    u8 psram_fuse_id;
    u32 psram_chip_id;
};

struct _psram_info {
    u8 mark_id;
    u8 psram_num;
    u32 psram_size;
    struct _psram_id psram_id;
};

#define PSRAM_TABLE_INFO                                                        \
{                                                                               \
    /* default 8M */                                                            \
    {0x00, PSRAM_PARALLEL, 0x800000, {APS3208K, 0x80c980c9}},                   \
    /* D122BAV 4M */                                                            \
    {0x01, PSRAM_SINGLE, 0x400000, {APS3208K, 0x80c980c9}},                     \
    /* D122BBV 8M */                                                            \
    {0x02, PSRAM_PARALLEL, 0x800000, {APS3208K, 0x80c980c9}},                   \
    /* D121BAV 4M */                                                            \
    {0x03, PSRAM_SINGLE, 0x400000, {APS3208K, 0x80c980c9}},                     \
    /* D121BBV 8M */                                                            \
    {0x04, PSRAM_PARALLEL, 0x800000, {APS3208K, 0x80c980c9}},                   \
    /* D122BCV1 16M SCKW18X128160AAE1 */                                        \
    {0x05, PSRAM_PARALLEL, 0x1000000, {SCKW18_12816O, 0xc59ac59a}},             \
    /* D122BCV2 16M AP12816 */                                                  \
    {0x05, PSRAM_PARALLEL, 0x1000000, {APS12816O, 0xdd8ddd8d}},                 \
    /* D123BAV 4M */                                                            \
    {0x07, PSRAM_SINGLE, 0x400000, {APS3208K, 0x80c980c9}},          \
    /* D123BBV 8M */                                                            \
    {0x08, PSRAM_PARALLEL, 0x800000, {APS3208K, 0x80c980c9}},        \
    /* D123BCV 16M */                                                           \
    {0x09, PSRAM_PARALLEL, 0x1000000, {APS12816O, 0xdd8ddd8d}},      \
    /* TR230 8M */                                                              \
    {0xA1, PSRAM_PARALLEL, 0x800000, {APS3208K, 0x80c980c9}},                   \
    /* JYX58 8M */                                                              \
    {0xB1, PSRAM_PARALLEL, 0x800000, {APS3208K, 0x80c980c9}},                   \
}

struct _psram_info psram_table_info[] = PSRAM_TABLE_INFO;

u8 psram_get_mark_id(void)
{
    u32 fuse_218 = readl(EFUSE_218_REG);
    u8 mark_id = fuse_218 & 0xff;

    pr_info("fuse_218(0x19010218)=0x%x, mark_id=0x%x\n", fuse_218, mark_id);
    return mark_id;
}

u8 psram_get_psram_id(void)
{
    u32 fuse_21c = readl(EFUSE_21C_REG);
    u8 psram_id = (fuse_21c & 0xff) >> 4;

    pr_info("fuse_21c(0x1901021c)=0x%x, psram_id=0x%x\n", fuse_21c, psram_id);
    return psram_id;
}

struct _psram_info *psram_get_info(u8 mark_id, u8 psram_fuse_id)
{
    u32 len = ARRAY_SIZE(psram_table_info);

    for (int i = 0; i < len; i++) {
        if (((mark_id == psram_table_info[i].mark_id) &&
             (psram_fuse_id == psram_table_info[i].psram_id.psram_fuse_id))) {
            return &psram_table_info[i];
        }
    }
    pr_info("can't get the psram table, return the default info.\n");
    return &psram_table_info[0]; //if not find anyone, return the default info.
}

u32 aic_get_ram_size(void)
{
    struct _psram_info *psram_info;
    u8 mark_id, psram_id;

    writel(0x1100, EFUSE_CMU_REG);
    mark_id = psram_get_mark_id();
    psram_id = psram_get_psram_id();
    psram_info = psram_get_info(mark_id, psram_id);
    writel(0x0, EFUSE_CMU_REG);

    return psram_info->psram_size;
}
