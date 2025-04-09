/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */
#define DBG_ENABLE
#define DBG_SECTION_NAME "ota.burn"
#define DBG_COLOR
#include <rtconfig.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>
#include <string.h>
#include "aic_core.h"
#include <boot_param.h>
#include <burn.h>

#ifdef AIC_SPINOR_DRV
#include <fal.h>

/* the address offset of download partition */
#ifndef RT_USING_FAL
#error "Please enable and confirgure FAL part."
#endif /* RT_USING_FAL */

const struct fal_partition *dl_part = RT_NULL;
#endif

#ifdef AIC_SPINAND_DRV
struct rt_mtd_nand_device *nand_mtd;
u8 g_nftl_flag = 0;
rt_device_t nand_dev;
#endif

rt_device_t mmc_dev;
u32 block_size;

int aic_ota_find_part(char *partname)
{
    switch (aic_get_boot_device()) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
            /* Get download partition information and erase download partition data */
            if ((dl_part = fal_partition_find(partname)) == RT_NULL) {
                LOG_E("Firmware download failed! Partition (%s) find error!",
                      partname);
                return -RT_ERROR;
            }
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
            nand_dev = rt_device_find(partname);
            if (nand_dev == RT_NULL) {
                LOG_E("Firmware download failed! Partition (%s) find error!",
                      partname);
                return -RT_ERROR;
            }

            if ((strncmp(partname, "blk_data", 9) == 0) || (strncmp(partname, "blk_data_r", 11) == 0)) {
                g_nftl_flag = 1;
                char *nftl_name = strstr(partname, "data");
                nand_mtd = (struct rt_mtd_nand_device *)rt_device_find(nftl_name);
                if (nand_mtd == RT_NULL) {
                    LOG_E("Firmware download failed! nftl Partition (%s) find error!",
                        nftl_name);
                    return -RT_ERROR;
                }
            } else {
                nand_mtd = (struct rt_mtd_nand_device *)nand_dev;
                g_nftl_flag = 0;
            }
            break;
#endif
        case BD_SDMC0:
            mmc_dev = rt_device_find(partname);
            if (mmc_dev == RT_NULL) {
                LOG_E("can't find %s device!", partname);
                return RT_ERROR;
            }
            break;
        default:
            return -RT_ERROR;
            break;
    }

    LOG_I("Partition (%s) find success!", partname);
    return 0;
}

#ifdef AIC_SPINOR_DRV
int aic_ota_nor_erase_part(void)
{
    LOG_I("Start erase flash (%s) partition!", dl_part->name);
    if (fal_partition_erase(dl_part, 0, dl_part->len) < 0) {
        LOG_E("Firmware download failed! Partition (%s) erase error! len = %d",
              dl_part->name, dl_part->len);
        return -RT_ERROR;
    }
    LOG_I("Erase flash (%s) partition success! len = %d", dl_part->name,
          dl_part->len);
    return 0;
}
#endif

#ifdef AIC_SPINAND_DRV
int aic_ota_nand_erase_part(void)
{
    unsigned long blk_offset = 0;

    LOG_I("Start erase nand flash partition!");

    while (nand_mtd->block_total > blk_offset) {
        if (rt_mtd_nand_check_block(nand_mtd, blk_offset) != RT_EOK) {
            LOG_W("Erase block is bad, skip it.\n");
            blk_offset++;
            continue;
        }

        rt_mtd_nand_erase_block(nand_mtd, blk_offset);
        blk_offset++;
    }

    LOG_I("Erase nand flash partition success! len = %d",
          nand_mtd->block_total);

    return 0;
}

int aic_ota_nand_write(uint32_t addr, const uint8_t *buf, size_t size)
{
    unsigned long blk = 0, offset = 0, page = 0, sector = 0, sector_total = 0;
    static unsigned long bad_block_off = 0;
    unsigned long blk_size = 0, page_size = 0;
    rt_err_t ret = 0;

    if (size > 2048) {
        LOG_E("OTA_BURN_LEN need set 2048! size = %d", size);
        return -RT_ERROR;
    }

    if (g_nftl_flag == 0) {
        ret = rt_device_open(nand_dev, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open MTD device failed.!\n");
            return ret;
        }

        blk_size = nand_mtd->pages_per_block * nand_mtd->page_size;

        offset = addr + bad_block_off;

        /* Search for the first good block after the given offset */
        if (offset % blk_size == 0) {
            blk = offset / blk_size;
            while (rt_mtd_nand_check_block(nand_mtd, blk) != RT_EOK) {
                LOG_W("find a bad block(%d), off adjust to the next block\n", blk);
                bad_block_off += nand_mtd->pages_per_block;
                offset = addr + bad_block_off;
                blk = offset / blk_size;
            }
        }

        page = offset / nand_mtd->page_size;
        ret = rt_mtd_nand_write(nand_mtd, page, buf, size, RT_NULL, 0);
        if (ret) {
            LOG_E("Failed to write data to NAND.\n");
            ret = -RT_ERROR;
        }

        rt_device_close(nand_dev);

    } else {

        ret = rt_device_open(nand_dev, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open MTD device failed.!\n");
            return ret;
        }
        page_size = 2048;//default size:nand_blk->mtd_device->page_size;

        page = addr / page_size;
        sector = page * 4;
       sector_total = (size / page_size) * 4;

        ret = rt_device_write(nand_dev, sector, buf, sector_total);
        if (ret < 0) {
            LOG_E("Failed to write data to NAND.\n");
            ret = -RT_ERROR;
        } else {
            ret = 0;
        }

        rt_device_close(nand_dev);
    }

    return ret;
}
#endif

int aic_ota_mmc_erase_part(void)
{
    struct rt_device_blk_geometry get_data;
    unsigned long long p[2] = {0};
    rt_err_t ret = 0;

    LOG_I("Start erase mmc partition!");

    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    rt_device_control(mmc_dev, RT_DEVICE_CTRL_BLK_GETGEOME, (void *)&get_data);

    p[0] = 0;//offset is 0
    p[1] = get_data.sector_count;
    block_size = get_data.block_size;

    ret = rt_device_control(mmc_dev, RT_DEVICE_CTRL_BLK_ERASE, (void *)p);
    if (ret != RT_EOK) {
        LOG_I("Erase mmc partition failed!");
        return ret;
    }

    LOG_I("Erase mmc partition success!");

    return 0;
}

int aic_ota_mmc_write(uint32_t addr, const uint8_t *buf, size_t size)
{
    unsigned long blkcnt, blkoffset;

    if (size > 2048) {
        LOG_E("OTA_BURN_LEN need set 2048! size = %d", size);
        return -RT_ERROR;
    }

    blkcnt = size / block_size;
    blkoffset = addr / block_size;

    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    rt_device_write(mmc_dev, blkoffset, (void *)buf, blkcnt);

    rt_device_close(mmc_dev);

    return 0;
}

int aic_ota_erase_part(void)
{
    int ret = 0;

    switch (aic_get_boot_device()) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
            ret = aic_ota_nor_erase_part();
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
            ret = aic_ota_nand_erase_part();
            break;
#endif
        case BD_SDMC0:
            ret = aic_ota_mmc_erase_part();
            break;
        default:
            break;
    }

    return ret;
}

int aic_ota_part_write(uint32_t addr, const uint8_t *buf, size_t size)
{
    int ret = 0;

    switch (aic_get_boot_device()) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
            ret = fal_partition_write(dl_part, addr, buf, size);
            if (ret < 0) {
                LOG_E(
                    "Firmware download failed! Partition (%s) write data error!",
                    dl_part->name);
                return -RT_ERROR;
            } else {
                ret = RT_EOK;
            }
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
            ret = aic_ota_nand_write(addr, buf, size);
            if (ret < 0) {
                LOG_E(
                    "Firmware download failed! nand partition write data error!");
                return -RT_ERROR;
            }
            break;
#endif
        case BD_SDMC0:
            ret = aic_ota_mmc_write(addr, buf, size);
            if (ret < 0) {
                LOG_E(
                    "Firmware download failed! mmc partition write data error!");
                return -RT_ERROR;
            }
            break;
        default:
            return -RT_ERROR;
            break;
    }

    return ret;
}

