/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <rtconfig.h>
#if defined(AICUPG_NOR_ARTINCHIP)
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
#include <sfud.h>
#include <upg_fat_direct.h>
#include <spienc.h>

extern sfud_flash *sfud_probe(u32 spi_bus);
int fat_direct_write_spinor(u32 spi_id, char *fpath, u32 doffset)
{
    static struct mtd_dev *mtd[MAX_INTERFACE_NUM] = {0};
    char mtdname[16];
    u8 *buf = NULL, *p;
    int ret = 0, endflag = 0;
    u32 soffset;
    ulong dolen, actread, writecnt;

    if (spi_id >= MAX_INTERFACE_NUM) {
        pr_err("%s, invalid parameter, spi id = %d\n", __func__, spi_id);
        return -1;
    }
    if (mtd[spi_id] == NULL) {
        sfud_flash *flash;

        flash = sfud_probe(spi_id);
        if (flash == NULL) {
            printf("Failed to probe spinor flash.\n");
            return -1;
        }
        memset(mtdname, 0, 16);
        snprintf(mtdname, 16, "nor%d", spi_id);
        mtd[spi_id] = mtd_get_device(mtdname);
        if (mtd[spi_id] == NULL) {
            pr_err("Failed to get MTD device: %s.\n", mtdname);
            return -1;
        }
#ifdef AIC_SPIENC_BYPASS_IN_UPGMODE
        spienc_set_bypass(1);
#endif
    }

    printf("Programming %s to 0x%x\n", fpath, doffset);
    printf("\t");
    buf = aicupg_malloc_align(MAX_WRITE_SIZE, FRAME_LIST_SIZE);
    soffset = 0;
    writecnt = 0;

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
        p = buf;
        soffset += actread;
        while(actread > 0) {
            dolen = actread / mtd[spi_id]->erasesize;
            if (actread % mtd[spi_id]->erasesize)
                dolen++;
            dolen *= mtd[spi_id]->erasesize;
            mtd_erase(mtd[spi_id], doffset, dolen);
            ret = mtd_write(mtd[spi_id], doffset, p, dolen);
            writecnt += dolen;

            printf("\r\t wrote count 0x%lx", writecnt);
            if (ret) {
                pr_err("Write mtd %s error.\n", mtdname);
                break;
            }
            pr_debug("Going to Erase/Write at 0x%x, len %ld\n", doffset, dolen);
            // hexdump(p, 32, 1);
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
    printf("\n");
    return ret;
}
#endif
