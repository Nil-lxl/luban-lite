/*
 * Copyright (c) 2024-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <rtconfig.h>
#if defined(AICUPG_NAND_ARTINCHIP)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <upg_internal.h>
#include <aic_core.h>
#include <aicupg.h>
#include <fatfs.h>
#include <mtd.h>
#include <aic_utils.h>
#include "nand_fwc_spl.h"
#include <upg_fat_direct.h>
#include <spienc.h>

extern struct aic_spinand *spinand_probe(u32 spi_bus);
static struct mtd_dev *fat_direct_spinand_probe(u32 spi_id)
{
    char mtdname[16];
    static struct mtd_dev *mtd = NULL;

    spinand_probe(spi_id);
    memset(mtdname, 0, 16);
    snprintf(mtdname, 16, "nand%d", spi_id);
    mtd = mtd_get_device(mtdname);
    if (mtd == NULL) {
        pr_err("Failed to get MTD device: %s.\n", mtdname);
        return NULL;
    }

#ifdef AIC_SPIENC_BYPASS_IN_UPGMODE
        spienc_set_bypass(1);
#endif
    return mtd;
}

static int fat_direct_spinand_write_boot(char *fpath, struct mtd_dev *mtd)
{
    struct aicupg_nand_priv priv;
    struct fwc_info fwc = { 0 };
    u32 soffset;
    ulong dolen, actread;
    int ret = 0, endflag = 0;
    u8 *buf = NULL;

    memset(&priv, 0, sizeof(priv));
    priv.mtd[0] = mtd;
    ret = nand_fwc_spl_reserve_blocks(&priv);
    if (ret) {
        pr_err("Reserve blocks for SPL failed.\n");
        return -1;
    }
    ret = nand_fwc_spl_prepare(&priv, MAX_SPL_SIZE, mtd->erasesize);
    if (ret) {
        pr_err("Prepare to write SPL failed.\n");
        return -1;
    }

    buf = aicupg_malloc_align(MAX_WRITE_SIZE, FRAME_LIST_SIZE);
    soffset = 0;
    while (1) {
        actread = 0;
        dolen = MAX_WRITE_SIZE;
        ret = aic_fat_read_file(fpath, (void *)buf, soffset, dolen, &actread);
        if (ret) {
            break;
        }
        pr_debug("Going to read %ld from offset 0x%x, got 0x%ld\n", dolen,
                 soffset, actread);
        if (actread != dolen)
            endflag = 1;
        soffset += actread;
        if (!endflag) {
            fwc.meta.size = MAX_SPL_SIZE;
            nand_fwc_spl_write(&fwc, buf, actread);
        } else {
            /* Set flag to tell writer, the data is end. */
            fwc.meta.size = 0;
            nand_fwc_spl_write(&fwc, buf, actread);
            break;
        }
    }

    if (buf)
        aicupg_free_align(buf);
    return 0;
}

static int fat_direct_spinand_write_data(char *fpath, struct mtd_dev *mtd,
                                         u32 doffset)
{
    u8 *buf = NULL, *p;
    int ret = 0, endflag = 0;
    u32 soffset;
    ulong dolen, actread, writecnt;

    buf = aicupg_malloc_align(MAX_WRITE_SIZE, FRAME_LIST_SIZE);
    soffset = 0;
    writecnt = 0;

    while (1) {
        actread = 0;
        dolen = MAX_WRITE_SIZE;
        ret = aic_fat_read_file(fpath, (void *)buf, soffset, dolen, &actread);
        if (ret) {
            ret = 0;
            break;
        }
        pr_debug("Going to read %ld from offset 0x%x, got 0x%ld\n", dolen,
                 soffset, actread);
        if (actread != dolen)
            endflag = 1;
        p = buf;
        soffset += actread;
        while (actread > 0) {
            /* Write one page per loop */
            dolen = mtd->writesize;

            if ((doffset % mtd->erasesize) == 0) {
                if (mtd_block_isbad(mtd, doffset)) {
                    doffset += mtd->erasesize;
                    /* Skip bad block */
                    continue;
                }
                mtd_erase(mtd, doffset, mtd->erasesize);
            }
            ret = mtd_write(mtd, doffset, p, dolen);
            writecnt += dolen;
            printf("\r\t wrote count 0x%lx", writecnt);
            if (ret) {
                pr_err("Write mtd error.\n");
                break;
            }
            pr_debug("Going to Erase/Write at 0x%x, len %ld\n", doffset,
                     dolen);

            if (actread < dolen)
                break;
            p += dolen;
            doffset += dolen;
            actread -= dolen;
            pr_debug("  left %ld to write\n", actread);
        }
        if (endflag)
            break;
    }

    if (buf)
        aicupg_free_align(buf);
    return ret;
}

#ifdef AIC_NFTL_SUPPORT
static int fat_direct_spinand_write_nftl(char *fpath, u32 doffset)
{
    u8 *buf = NULL;
    int ret = 0, endflag = 0;
    u32 soffset, i, part_cnt, start_sector, sector_total;
    ulong dolen, actread, writecnt;
    struct mtd_dev *mtd;
    struct nftl_api_handler_t *nftl;

    part_cnt = mtd_get_device_count();
    for (i = 0; i < part_cnt; i++) {
        mtd = mtd_get_device_by_id(i);
        if (mtd == NULL)
            break;
        if (doffset == mtd->start)
            break;
        mtd = NULL;
    }

    if (mtd == NULL) {
        pr_err("Cannot find partition at offset 0x%x.\n", doffset);
        return -1;
    }
    nftl = aicos_malloc(MEM_CMA, sizeof(struct nftl_api_handler_t));
    if (nftl == NULL) {
        pr_err("Out of memory, failed to malloc for NFTL.\n");
        return -1;
    }

    memset(nftl, 0, sizeof(struct nftl_api_handler_t));
    nftl->priv_mtd = (void *)mtd;

    nftl->nandt = aicos_malloc(MEM_CMA, sizeof(struct nftl_api_nand_t));
    if (!nftl->nandt) {
        pr_err("Out of memory, failed to malloc for nandt.\n");
        ret = -1;
        goto out;
    }
    nftl->nandt->page_size = mtd->writesize;
    nftl->nandt->oob_size = mtd->oobsize;
    nftl->nandt->pages_per_block = mtd->erasesize / mtd->writesize;
    nftl->nandt->block_total = mtd->size / mtd->erasesize;
    nftl->nandt->block_start = mtd->start / mtd->erasesize;
    nftl->nandt->block_end =  (mtd->start + mtd->size) / mtd->erasesize;

    for (int offset_e = 0; offset_e < mtd->size;) {
        mtd_erase(mtd, offset_e, mtd->erasesize);
        offset_e += mtd->erasesize;
    }

    if (nftl_api_init(nftl, i)) {
        pr_err("[NE]nftl_initialize failed\n");
        ret = -1;
        goto out;
    }

    buf = aicupg_malloc_align(MAX_WRITE_SIZE, FRAME_LIST_SIZE);
    if (!buf) {
        pr_err("Failed to malloc buffer.\n");
        ret = -1;
        goto out;
    }
    soffset = 0;
    writecnt = 0;

    while (1) {
        actread = 0;
        dolen = MAX_WRITE_SIZE;
        ret = aic_fat_read_file(fpath, (void *)buf, soffset, dolen, &actread);
        if (ret) {
            ret = 0;
            break;
        }
        pr_debug("Going to read %ld from offset 0x%x, got 0x%ld\n", dolen,
                 soffset, actread);
        if (actread != dolen)
            endflag = 1;
        writecnt += actread;
        printf("\r\t wrote count 0x%lx", writecnt);

        start_sector = soffset / NFTL_SECTOR_SIZE;
        sector_total = actread / NFTL_SECTOR_SIZE;
        if (dolen % NFTL_SECTOR_SIZE)
            sector_total++;

        nftl_api_write(nftl, start_sector, sector_total, buf);
        nftl_api_write_cache(nftl, 0xffff);
        soffset += actread;

        if (endflag)
            break;
    }

out:
    if (buf)
        aicupg_free_align(buf);
    if (nftl && nftl->nandt)
        aicos_free(MEM_CMA, nftl->nandt);
    if (nftl)
        aicos_free(MEM_CMA, nftl);

    return ret;
}
#endif

int fat_direct_write_spinand(u32 spi_id, char *fpath, u32 doffset,
                             u32 boot_flag, char *attr)
{
    static struct mtd_dev *mtd[MAX_INTERFACE_NUM] = {0};
    int ret = 0;

    if (spi_id >= MAX_INTERFACE_NUM) {
        pr_err("%s, invalid parameter, spi id = %d\n", __func__, spi_id);
        return -1;
    }
    if (mtd[spi_id] == NULL) {
        mtd[spi_id] = fat_direct_spinand_probe(spi_id);
        if (mtd[spi_id] == NULL)
            return -1;
    }

    printf("Programming %s to 0x%x\n", fpath, doffset);
    printf("\t");
    if (boot_flag) {
        ret = fat_direct_spinand_write_boot(fpath, mtd[spi_id]);
        if (ret)
            goto out;
#ifdef AIC_NFTL_SUPPORT
    } else if (attr && (strcmp(attr, "nftl") == 0)) {
        ret = fat_direct_spinand_write_nftl(fpath, doffset);
        if (ret)
            goto out;
#endif
    } else {
        ret = fat_direct_spinand_write_data(fpath, mtd[spi_id], doffset);
        if (ret)
            goto out;
    }

out:
    printf("\n");
    return ret;
}
#endif
