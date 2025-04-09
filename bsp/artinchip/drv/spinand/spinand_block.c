/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <string.h>
#include <rtconfig.h>
#include <assert.h>
#include "spinand.h"
#include "spinand_block.h"
#include "spinand_parts.h"
#include <bbt.h>

#ifdef AIC_NFTL_SUPPORT
#include <nftl_api.h>
#endif

#ifdef AIC_NFTL_SUPPORT
rt_size_t rt_spinand_read_nftl(rt_device_t dev, rt_off_t pos, void *buffer,
                                    rt_size_t size)
{
    rt_size_t ret;
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;

    ret = nftl_api_read(part->nftl_handler, pos, size, buffer);
    if (ret == 0) {
        return size;
    } else {
        return -1;
    }
}

rt_size_t rt_spinand_write_nftl(rt_device_t dev, rt_off_t pos,
                                     const void *buffer, rt_size_t size)
{
    rt_size_t ret;
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;

    ret = nftl_api_write(part->nftl_handler, pos, size, (u8 *)buffer);
    if (ret == 0) {
        return size;
    } else {
        return -1;
    }
}

rt_err_t rt_spinand_init_nftl(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    part->nftl_handler =
        aicos_malloc(MEM_CMA, sizeof(struct nftl_api_handler_t));
    //part->nftl_handler = (struct nftl_api_handler_t *)rt_malloc(sizeof(struct nftl_api_handler_t));
    if (!part->nftl_handler) {
        pr_err(
            "Error: no memory for create SPI NAND block device . nftl_handler");
        return RT_ERROR;
    }
    memset(part->nftl_handler, 0, sizeof(struct nftl_api_handler_t));

    part->nftl_handler->priv_mtd = (void *)part->mtd_device;
    part->nftl_handler->nandt =
        aicos_malloc(MEM_CMA, sizeof(struct nftl_api_nand_t));

    part->nftl_handler->nandt->page_size = part->mtd_device->page_size;
    part->nftl_handler->nandt->oob_size = part->mtd_device->oob_size;
    part->nftl_handler->nandt->pages_per_block =
        part->mtd_device->pages_per_block;
    part->nftl_handler->nandt->block_total = part->mtd_device->block_total;
    part->nftl_handler->nandt->block_start = part->mtd_device->block_start;
    part->nftl_handler->nandt->block_end = part->mtd_device->block_end;

    if (nftl_api_init(part->nftl_handler, dev->device_id)) {
        pr_err("[NE]nftl_initialize failed\n");
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t rt_spinand_nftl_close(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    return nftl_api_write_cache(part->nftl_handler, 0xffff);
}
#endif

static u32 spinand_start_page_calculate(rt_device_t dev, rt_off_t pos)
{
    struct spinand_blk_device *blk_dev = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *mtd_dev = blk_dev->mtd_device;
    const u8 SECTORS_PP = mtd_dev->page_size / blk_dev->geometry.bytes_per_sector;
    u32 start_page = 0, block, pos_block;

    start_page = pos / SECTORS_PP + mtd_dev->block_start * mtd_dev->pages_per_block;
    block = start_page / mtd_dev->pages_per_block;
    pos_block = mtd_dev->ops->get_block_status(mtd_dev, block);
    pr_debug("block = %d, pos_block = %d\n", block, pos_block);
    if (pos_block != 0) {
        block += pos_block;
        pos += pos_block * mtd_dev->pages_per_block * SECTORS_PP;
        start_page = pos / SECTORS_PP + mtd_dev->block_start * mtd_dev->pages_per_block;
    }

    return start_page;
}

static void spinand_nonftl_cache_write(struct spinand_blk_device *blk_dev, void *copybuf, u32 copy_cnt, rt_off_t pos)
{
    u32 cache_cnt = 0;
    u32 cur_pos = pos;

    pr_debug(" copy_cnt = %d", copy_cnt);
    copy_cnt = copy_cnt > BLOCK_CACHE_NUM ? BLOCK_CACHE_NUM : copy_cnt;
    for (cache_cnt = 0; cache_cnt < copy_cnt; cache_cnt++) {
        pr_debug(" blk_cache[%d].pos: %d", cache_cnt, cur_pos);
        rt_memcpy(blk_dev->blk_cache[cache_cnt].buf, copybuf, BLOCK_CACHE_SIZE);
        blk_dev->blk_cache[cache_cnt].pos = cur_pos++;
        copybuf += BLOCK_CACHE_SIZE;
    }
}

static rt_size_t spinand_nonftl_cache_read(struct spinand_blk_device *blk_dev, rt_off_t *pos, void *buffer, rt_size_t size)
{
    rt_size_t sectors_read = 0;
    u32 cnt = 0;

    for (cnt = 0; cnt < BLOCK_CACHE_NUM; cnt++) {
        if (blk_dev->blk_cache[cnt].pos == *pos) {
             break;
        }
    }

    if (cnt == BLOCK_CACHE_NUM)
        return 0;
    while (cnt < BLOCK_CACHE_NUM) {
        if (blk_dev->blk_cache[cnt].pos == *pos) {
            pr_debug(" copy[%d] pos = %d", cnt, *pos);
            rt_memcpy(buffer, blk_dev->blk_cache[cnt].buf, BLOCK_CACHE_SIZE);
            cnt++;
            *pos += 1;
            sectors_read++;
            buffer += BLOCK_CACHE_SIZE;
            if (sectors_read == size)
                return size;
        } else {
            break;
        }
    }
    return sectors_read;
}

static rt_size_t spinand_read_nonftl_nalign(rt_device_t dev, rt_off_t *pos, void *buffer, rt_size_t size)
{
    struct spinand_blk_device *blk_dev = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *mtd_dev = blk_dev->mtd_device;
    const u8 SECTORS_PP = mtd_dev->page_size / blk_dev->geometry.bytes_per_sector;
    const u32 POS_SECTOR_OFFS = (*pos) % SECTORS_PP;
    rt_size_t copysize = 0, sectors_read = 0;
    u32 start_page = 0;
    u8 *copybuf = NULL;
    int ret = 0;

    start_page = spinand_start_page_calculate(dev, *pos);

    memset(blk_dev->pagebuf, 0xFF, mtd_dev->page_size);
    pr_debug("read_page: %d\n", start_page);
    ret = mtd_dev->ops->read_page(mtd_dev, start_page, blk_dev->pagebuf,
                                    mtd_dev->page_size, NULL, 0);
    if (ret != RT_EOK) {
        pr_err("read_page failed!\n");
        return -RT_ERROR;
    }

    copybuf = blk_dev->pagebuf + POS_SECTOR_OFFS * blk_dev->geometry.bytes_per_sector;
    if (size > (SECTORS_PP - POS_SECTOR_OFFS))
        sectors_read += (SECTORS_PP - POS_SECTOR_OFFS);
    else
        sectors_read += size;

    copysize = sectors_read * blk_dev->geometry.bytes_per_sector;
    *pos += sectors_read;
    rt_memcpy(buffer, copybuf, copysize);

    /* if copy not end align, store the remaining part to cache */
    if (size < (SECTORS_PP - POS_SECTOR_OFFS)) {
        spinand_nonftl_cache_write(blk_dev, (copybuf + copysize), ((SECTORS_PP - POS_SECTOR_OFFS) - size),  *pos);
    }

    return sectors_read;
}

static rt_size_t spinand_read_nonftl_align(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size, rt_size_t sectors_read)
{
    struct spinand_blk_device *blk_dev = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *mtd_dev = blk_dev->mtd_device;
    const u8 SECTORS_PP = mtd_dev->page_size / blk_dev->geometry.bytes_per_sector;
    rt_size_t copysize = 0, copy_sectors = 0;
    u32 start_page = 0;
    int ret = 0;

    start_page = spinand_start_page_calculate(dev, pos);

    while (size > sectors_read) {
        start_page = spinand_start_page_calculate(dev, pos);

        memset(blk_dev->pagebuf, 0xFF, mtd_dev->page_size);
        pr_debug("read_page: %d\n", start_page);
        ret = mtd_dev->ops->read_page(mtd_dev, start_page, blk_dev->pagebuf,
                                        mtd_dev->page_size, NULL, 0);
        if (ret != RT_EOK) {
            pr_err("read_page failed!\n");
            return -RT_ERROR;
        }

        if ((size - sectors_read) > SECTORS_PP) {
            copysize = SECTORS_PP * blk_dev->geometry.bytes_per_sector;
            sectors_read += SECTORS_PP;
            pos += SECTORS_PP;
        } else {
            copy_sectors = (size - sectors_read);
            copysize = copy_sectors * blk_dev->geometry.bytes_per_sector;
            sectors_read += copy_sectors;
            pos += copy_sectors;
        }

        rt_memcpy(buffer, blk_dev->pagebuf, copysize);
        buffer += copysize;
        start_page++;
    }

    /* if copy not end align, store the remaining part to cache */
    if (copy_sectors) {
        spinand_nonftl_cache_write(blk_dev, (blk_dev->pagebuf + copysize), (SECTORS_PP - copy_sectors),  pos);
    }

    return sectors_read;
}

#ifdef AIC_SPINAND_CONT_READ
// to do: elm filesystem read 1K size once, do not use continuous reading
#define SPINAND_BLK_CONT_READ   0
#if SPINAND_BLK_CONT_READ
static rt_size_t spinand_continuous_read_nonftl(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size, rt_size_t sectors_read)
{
    struct spinand_blk_device *blk_dev = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *mtd_dev = blk_dev->mtd_device;
    const u8 SECTORS_PP = mtd_dev->page_size / blk_dev->geometry.bytes_per_sector;
    rt_size_t copysize = 0, read_size = 0;
    rt_uint8_t *data_ptr = RT_NULL;
    u32 start_page = 0;
    int ret = 0;

    /* size is less than a pagesize, not use continuous read */
    if ((size - sectors_read) > SECTORS_PP)
        return sectors_read;

    copysize =  (size - sectors_read) * blk_dev->geometry.bytes_per_sector;
    read_size = copysize > mtd_dev->page_size ? copysize : mtd_dev->page_size;
    data_ptr = (rt_uint8_t *)rt_malloc_align(read_size, CACHE_LINE_SIZE);
    if (data_ptr == RT_NULL) {
        pr_err("data_ptr: no memory\n");
        return sectors_read;
    }

    rt_memset(data_ptr, 0, read_size);
    start_page = spinand_start_page_calculate(dev, pos);
    ret = mtd_dev->ops->continuous_read(mtd_dev, start_page, data_ptr, read_size);
    if (ret != RT_EOK) {
        pr_err("continuous_read failed!\n");
        goto cont_read_error;
    }

    rt_memcpy(buffer, data_ptr, copysize);
    sectors_read += size - sectors_read;

cont_read_error:
    if (data_ptr)
        rt_free_align(data_ptr);

    return sectors_read;
}
#else
static rt_size_t spinand_continuous_read_nonftl(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size, rt_size_t sectors_read)
{
    return sectors_read;
}
#endif
#endif  // AIC_SPINAND_CONT_READ

rt_size_t rt_spinand_read_nonftl(rt_device_t dev, rt_off_t pos,
                                       void *buffer, rt_size_t size)
{
    struct spinand_blk_device *blk_dev = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *mtd_dev = blk_dev->mtd_device;
    const u8 SECTORS_PP = mtd_dev->page_size / blk_dev->geometry.bytes_per_sector;
    rt_size_t sectors_read = 0;

    assert(blk_dev != RT_NULL);

    pr_debug("pos = %d, size = %d\n", pos, size);

    /* pos is not aligned with page, read unalign part first */
    if (pos % SECTORS_PP) {
        sectors_read = spinand_nonftl_cache_read(blk_dev, &pos, buffer, size);
        if (sectors_read == 0) {
            sectors_read = spinand_read_nonftl_nalign(dev, &pos, buffer, size);
        }
        buffer += sectors_read * blk_dev->geometry.bytes_per_sector;

        if (sectors_read == size)
            return size;
    }

#ifdef AIC_SPINAND_CONT_READ
    sectors_read = spinand_continuous_read_nonftl(dev, pos, buffer, size, sectors_read);
    if (sectors_read == size)
        return size;
#endif

    /* pos is aligned with page */
    sectors_read = spinand_read_nonftl_align(dev, pos, buffer, size, sectors_read);
    if (sectors_read != size) {
        pr_err("ERROR: read not compete sectors_read: %d, size: %d.\n", sectors_read, size);
        return sectors_read;
    }

    return size;
}

rt_size_t rt_spinand_write_nonftl(rt_device_t dev, rt_off_t pos,
                                        const void *buffer, rt_size_t size)
{
    struct spinand_blk_device *blk_dev = (struct spinand_blk_device *)dev;
    pr_err("ERROR: %s write failed, please check!\n", blk_dev->name);
    return -RT_ERROR;
}

rt_err_t rt_spinand_init_nonftl(rt_device_t dev)
{
    u32 bad_block_pos = 0;
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    struct rt_mtd_nand_device *device = part->mtd_device;
    rt_uint32_t block;

    assert(part != RT_NULL);

    for (block = device->block_start; block < device->block_end; block++) {
        if (device->ops->check_block(device, block)) {
            pr_err("Find a bad block, block: %u.\n", block);
            /* Find next good block. */
            do {
                bad_block_pos++;
            } while (device->ops->check_block(device, block + bad_block_pos));
            device->ops->set_block_status(device, block, bad_block_pos, BBT_BLOCK_FACTORY_BAD);
        } else {
            device->ops->set_block_status(device, block, bad_block_pos, BBT_BLOCK_GOOD);
        }
    }
    return 0;
}

rt_err_t rt_spinand_nonftl_close(rt_device_t dev)
{
    return 0;
}

static rt_err_t rt_spinand_init(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_init_nftl(dev);
#else
        return rt_spinand_init_nonftl(dev);
#endif
    }
    return rt_spinand_init_nonftl(dev);
}

static rt_err_t rt_spinand_control(rt_device_t dev, int cmd, void *args)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;

    assert(part != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME) {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *)args;
        if (geometry == RT_NULL) {
            return -RT_ERROR;
        }

        memcpy(geometry, &part->geometry,
               sizeof(struct rt_device_blk_geometry));
    } else if (cmd == RT_DEVICE_CTRL_BLK_SYNC) {
        if (part->attr == PART_ATTR_NFTL) {
#ifdef AIC_NFTL_SUPPORT
            nftl_api_write_cache(part->nftl_handler, 0xffff);
#else
            pr_warn("Invaild cmd = %d\n", cmd);
#endif
        } else {
            pr_warn("Invaild cmd = %d\n", cmd);
        }
    } else {
        pr_warn("Invaild cmd = %d\n", cmd);
    }

    return RT_EOK;
}

static rt_size_t rt_spinand_write(rt_device_t dev, rt_off_t pos,
                                  const void *buffer, rt_size_t size)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_write_nftl(dev, pos, buffer, size);
#else
        return rt_spinand_write_nonftl(dev, pos, buffer, size);
#endif
    }
    return rt_spinand_write_nonftl(dev, pos, buffer, size);
}

static rt_size_t rt_spinand_read(rt_device_t dev, rt_off_t pos, void *buffer,
                                 rt_size_t size)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_read_nftl(dev, pos, buffer, size);
#else
        return rt_spinand_read_nonftl(dev, pos, buffer, size);
#endif
    }
    return rt_spinand_read_nonftl(dev, pos, buffer, size);
}

static rt_err_t rt_spinand_close(rt_device_t dev)
{
    struct spinand_blk_device *part = (struct spinand_blk_device *)dev;
    if (part->attr == PART_ATTR_NFTL) {

#ifdef AIC_NFTL_SUPPORT
        return rt_spinand_nftl_close(dev);
#else
        return rt_spinand_nonftl_close(dev);
#endif
    }
    return rt_spinand_nonftl_close(dev);
}

#ifdef RT_USING_DEVICE_OPS
static struct rt_device_ops blk_dev_ops = {
    rt_spinand_init, RT_NULL,          rt_spinand_close,
    rt_spinand_read, rt_spinand_write, rt_spinand_control
};
#endif

int rt_blk_nand_register_device(const char *name,
                                struct rt_mtd_nand_device *device)
{
    char str[32] = { 0 };
    struct spinand_blk_device *blk_dev;

    blk_dev = (struct spinand_blk_device *)rt_malloc(
        sizeof(struct spinand_blk_device));
    if (!blk_dev) {
        pr_err("Error: no memory for create SPI NAND block device");
    }
    rt_memset(blk_dev, 0, sizeof(struct spinand_blk_device));

    blk_dev->mtd_device = device;
    blk_dev->parent.type = RT_Device_Class_Block;
    blk_dev->attr = device->attr;

#ifdef RT_USING_DEVICE_OPS
    blk_dev->parent.ops = &blk_dev_ops;
#else
    /* register device */
    blk_dev->parent.init = rt_spinand_init;
    blk_dev->parent.open = NULL;
    blk_dev->parent.close = rt_spinand_close;
    blk_dev->parent.read = rt_spinand_read;
    blk_dev->parent.write = rt_spinand_write;
    blk_dev->parent.control = rt_spinand_control;
#endif

    blk_dev->geometry.bytes_per_sector = 512;
    blk_dev->geometry.block_size = blk_dev->geometry.bytes_per_sector;
    blk_dev->geometry.sector_count =
        device->block_total * device->pages_per_block * device->page_size /
        blk_dev->geometry.bytes_per_sector;

    blk_dev->pagebuf =
        aicos_malloc_align(0, device->page_size, CACHE_LINE_SIZE);
    if (!blk_dev->pagebuf) {
        pr_err("malloc buf failed\n");
        return -1;
    }

    rt_sprintf(str, "blk_%s", name);
    memset(blk_dev->name, 0, 32);
    rt_memcpy(blk_dev->name, str, 32);
    /* register the device */
    rt_device_register(RT_DEVICE(blk_dev), str,
                       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
    return 0;
}
