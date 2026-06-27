/*
 * Copyright (C) 2024-2026 ArtInChip Technology Co.,Ltd
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

struct _psram_id {
    u8 psram_fuse_id;
    u32 psram_chip_id;
};

struct _psram_info {
    u8 mark_id;
    u8 psram_num;
    u32 psram_size;
    struct _psram_id psram_id;
    char *chip_name;
};

/* Type 	Mark ID	Package ID	PSRAM ID	Flash IOMAP	DIS_MIPI	DIS_CANFD	DIS_PWMCS */
/* D125CBS	0x01	0xC	0x4	0x00	0x0	0x1	0x0 */
/* D125CCS	0x02	0xC	0x2	0x00	0x0	0x1	0x0 */
/* D125DDS	0x03	0xD	0x3	0x00	0x0	0x1	0x0 */
/* D125EDS	0x04	0xE	0x3	0x00	0x0	0x1	0x0 */
/* D126FBV	0x05	0xF	0x4	0x00	0x0	0x1	0x0 */
/* D126FCV	0x06	0xF	0x2	0x00	0x0	0x1	0x0 */
/* D125CCS-U	0x07	0xC	0x2	0x00	0x0	0x1	0x0 */
/* G720ADU	0x21	0xA	0x0	0x55	0x1	0x1	0x0 */
/* G720AFU	0x22	0xA	0x0	0x55	0x1	0x1	0x0 */

/* PSRAM ID */
/* 0x0:  APS3208K */
/* 0x1:  SCKW18X128160AAE1 */
/* 0x2:  APS12816O-OB9 */
/* 0x3:  32MB, APS25616N-OBX9 */
/* 0x4:  4MB x 2, APS3208K */
/* 0xF:  No PSRAM" */

enum psram_fuse {
    PSRAM_FUSE_APS3208K = 0x00,
    PSRAM_FUSE_APS12816O_OB9 = 0x02,	//
    PSRAM_FUSE_APS25616N_OBX9 = 0x03,	// 32M
    PSRAM_FUSE_APS3208K_X2 = 0x04,	// 4M * 2, APS3208K * 2
    PSRAM_FUSE_NO_PSRAM = 0x0F,		// no psram
};

enum psram_size {
    PSRAM_SIZE_4M = 0x400000,
    PSRAM_SIZE_8M = 0x800000,
    PSRAM_SIZE_16M = 0x1000000,
    PSRAM_SIZE_32M = 0x2000000,
};

#define PSRAM_TABLE_INFO							\
{											        \
    /* default D125CCS/D126FCV */                   \
    {0x00, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D125CCS/D126FCV"},\
    /* D125CBS */									\
    {0x01, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_APS3208K_X2, 0x80c980c9}, "D125CBS"},	    \
    /* D125CCS */									\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D125CCS"},	\
    /* D125DDS */									\
    {0x03, PSRAM_PARALLEL, PSRAM_SIZE_32M, {PSRAM_FUSE_APS25616N_OBX9, 0xdf8ddf8d}, "D125DDS"}, \
    /* D125EDS */									\
    {0x04, PSRAM_PARALLEL, PSRAM_SIZE_32M, {PSRAM_FUSE_APS25616N_OBX9, 0xdf8ddf8d}, "D125EDS"},	\
    /* D126FBV */									\
    {0x05, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_APS3208K_X2, 0x80c980c9}, "D126FBV"},	    \
    /* D126FCV */									\
    {0x06, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D126FCV"},	\
    /* D125CCS-U */									\
    {0x07, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D125CCS-U"},\
    /* G720ADU  */									\
    {0x21, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_APS3208K, 0x80c980c9}, "G720ADU"},		    \
    /* G720AFU */									\
    {0x22, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_APS3208K, 0x80c980c9}, "G720AFU"},		    \
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
