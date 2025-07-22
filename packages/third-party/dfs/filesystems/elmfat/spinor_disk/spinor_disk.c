/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dfs_bare.h>
#include <rtconfig.h>
#include "aic_osal.h"
#include "spinor_disk.h"
#include "mtd.h"

/*******************************************************************************
 * Code
 ******************************************************************************/
#ifdef LPKG_USING_LEVELX
UINT  aic_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash);

static DRESULT spinor_disk_levelx_read(void *hdisk, uint8_t *buf, uint32_t sector,
                         uint8_t cnt)
{
    struct spinor_blk_device *dev = hdisk;
    LX_NOR_FLASH *lx_dev;
    rt_size_t ret = 0;

    if (!dev)
        return RES_NOTRDY;

    lx_dev = dev->lx_nor_flash;

    if (!lx_dev)
        return RES_NOTRDY;

    while (cnt) {
        ret =  lx_nor_flash_sector_read(lx_dev, sector, buf);
        if (ret) {
            pr_err("LeveX read sector %lu failed!\n", sector);
            return -RT_ERROR;
        }

        cnt--;
        sector++;
        buf += dev->info.bytes_per_sector;
    }

    return RES_OK;
}

DRESULT spinor_disk_levelx_write(void *hdisk, const uint8_t *buf, uint32_t sector,
                          uint8_t cnt)
{
    struct spinor_blk_device *dev = hdisk;
    LX_NOR_FLASH *lx_dev;
    rt_size_t ret = 0;

    if (!dev)
        return RES_NOTRDY;

    lx_dev = dev->lx_nor_flash;
    if (!lx_dev)
        return RES_NOTRDY;

    while (cnt) {
        ret =  lx_nor_flash_sector_write(lx_dev, sector, (void *)buf);
        if (ret) {
            pr_err("LeveX write sector %lu failed!\n", sector);
            return -RT_ERROR;
        }

        cnt--;
        sector++;
        buf += dev->info.bytes_per_sector;
    }

    return RES_OK;
}
#else
#ifdef AIC_FATFS_ENABLE_WRITE_IN_SPINOR
DRESULT spinor_disk_mtd_write(void *hdisk, const uint8_t *buf, uint32_t sector,
                          uint8_t cnt)
{
    struct spinor_blk_device *dev = hdisk;
    struct mtd_dev *mtd;
    rt_size_t phy_pos, pos;
    rt_size_t phy_size, size;
    rt_size_t ret = 0;
    uint32_t align_sector = 0;
    uint8_t align_cnt = 0;

    if (!dev)
        return RES_NOTRDY;

    mtd = dev->mtd_device;
    if (!mtd)
        return RES_NOTRDY;

    align_sector = sector - sector % 8;
    align_cnt = sector % 8 + cnt;
    align_cnt = (align_cnt + 7) / 8 * 8;

    pr_debug("sector = %ld cnt = %d!\n", sector, cnt);
    pr_debug("align_sector = %ld align_cnt = %d!\n", align_sector, align_cnt);

    phy_pos = align_sector * dev->info.bytes_per_sector;
    phy_size = align_cnt * dev->info.bytes_per_sector;

    memset(dev->buf, 0xFF, dev->length);
    ret = mtd_read(mtd, phy_pos, dev->buf, phy_size);
    if (ret) {
        pr_err("Mtd read data failed!\n");
        return -RT_ERROR;
    }

    pos = sector % 8 * dev->info.bytes_per_sector;
    size = cnt * dev->info.bytes_per_sector;

    memcpy(dev->buf + pos, buf, size);

    mtd_erase(mtd, phy_pos, phy_size);

    ret = mtd_write(mtd, phy_pos, dev->buf, phy_size);
    if (ret) {
        pr_err("Mtd write data failed!\n");
        return -RT_ERROR;
    }

    return RES_OK;
}
#endif

static DRESULT spinor_disk_mtd_read(void *hdisk, uint8_t *buf, uint32_t sector,
                         uint8_t cnt)
{
    struct spinor_blk_device *dev = hdisk;
    struct mtd_dev *mtd;
    rt_size_t ret = 0;
    rt_size_t phy_pos;
    rt_size_t phy_size;

    if (!dev)
        return RES_NOTRDY;

    mtd = dev->mtd_device;

    if (!mtd)
        return RES_NOTRDY;

    /* change the block device's logic address to physical address */
    phy_pos = sector * dev->info.bytes_per_sector;
    phy_size = cnt * dev->info.bytes_per_sector;

    ret = mtd_read(mtd, phy_pos, buf, phy_size);
    if (ret) {
        pr_err("Mtd read data failed!\n");
        return -RT_ERROR;
    }

    return RES_OK;
}
#endif

DRESULT spinor_disk_write(void *hdisk, const uint8_t *buf, uint32_t sector,
                          uint8_t cnt)
{
#ifndef AIC_FATFS_ENABLE_WRITE_IN_SPINOR
    pr_warn("This config only supports read!\n");
    return RES_OK;
#else
#ifdef LPKG_USING_LEVELX
    return spinor_disk_levelx_write(hdisk, buf, sector, cnt);
#else
    return spinor_disk_mtd_write(hdisk, buf, sector, cnt);
#endif
#endif
}

DRESULT spinor_disk_read(void *hdisk, uint8_t *buf, uint32_t sector,
                         uint8_t cnt)
{
#ifdef LPKG_USING_LEVELX
    return spinor_disk_levelx_read(hdisk, buf, sector, cnt);
#else
    return spinor_disk_mtd_read(hdisk, buf, sector, cnt);
#endif
}

DRESULT spinor_disk_ioctl(void *hdisk, uint8_t command, void *buf)
{
    struct spinor_blk_device *dev = hdisk;
    DRESULT result = RES_OK;

    if (!dev)
        return RES_NOTRDY;

    switch (command) {
        case GET_SECTOR_COUNT:
            if (buf) {
                *(uint32_t *)buf = dev->info.sector_count;
            } else {
                result = RES_PARERR;
            }

            break;

        case GET_SECTOR_SIZE:
            if (buf) {
                *(uint32_t *)buf = dev->info.bytes_per_sector;
            } else {
                result = RES_PARERR;
            }

            break;

        case GET_BLOCK_SIZE: /* Get erase block size in unit of sectors (DWORD) */
            if (buf) {
                *(uint32_t *)buf = dev->info.block_size / dev->info.bytes_per_sector;
            } else {
                result = RES_PARERR;
            }

            break;

        case CTRL_SYNC:
            result = RES_OK;
            break;

        default:
            result = RES_PARERR;
            break;
    }

    return result;
}

DSTATUS spinor_disk_status(void *hdisk)
{
    return RES_OK;
}

void *spinor_disk_initialize(const char *device_name)
{
    struct spinor_blk_device *dev;

    dev = (void *)malloc(sizeof(*dev));
    if (!dev) {
        pr_err("Error: no memory for create SPI NOR block device");
        return NULL;
    }

    /*Obtain devices by part name*/
    dev->mtd_device = mtd_get_device(device_name);
    if (!dev->mtd_device) {
        pr_err("Failed to get mtd %s\n", device_name);
        free(dev);
        return NULL;
    }

#ifdef AIC_FATFS_ENABLE_WRITE_IN_SPINOR
    dev->length = AIC_USING_FS_IMAGE_TYPE_FATFS_CLUSTER_SIZE * 512 * 2;
    dev->buf = (uint8_t *)aicos_malloc(MEM_CMA, dev->length);
    if (!dev->buf) {
        pr_err("Error: no memory for create SPI NOR block buf");
        free(dev);
        return NULL;
    }

#endif

    dev->info.bytes_per_sector = 512;
    dev->info.block_size = dev->info.bytes_per_sector;
    dev->info.sector_count = dev->mtd_device->size / dev->info.bytes_per_sector;

#ifdef LPKG_USING_LEVELX
    if (dev->mtd_device->attr == PART_ATTR_LEVELX) {
        lx_nor_flash_initialize();
        dev->lx_nor_flash = (LX_NOR_FLASH *)malloc(sizeof(LX_NOR_FLASH));
        memset(dev->lx_nor_flash, 0, sizeof(LX_NOR_FLASH));
        dev->lx_nor_flash->mtd_device = dev->mtd_device;
        if(lx_nor_flash_open(dev->lx_nor_flash, dev->mtd_device->name, aic_lx_nor_flash_driver_initialize)) {
            pr_err("%s: %d lx_nor_flash_open failed\n", __func__, __LINE__);
            free(dev);
            return NULL;
        }

        dev->info.bytes_per_sector = 512;
        dev->info.block_size = dev->lx_nor_flash->lx_nor_flash_words_per_block;
        dev->info.sector_count = dev->lx_nor_flash->lx_nor_flash_total_physical_sectors - 64;
    }
#endif

    return dev;
}
