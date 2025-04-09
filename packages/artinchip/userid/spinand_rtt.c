/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_time.h>
#include <aic_log.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <userid.h>
#include "userid_internal.h"

static struct rt_mtd_nand_device *g_userid_mtd = NULL;

static bool is_aligned_with_block_size(struct rt_mtd_nand_device *mtd, u64 size)
{
    size_t erasesize;

    erasesize = mtd->page_size * mtd->pages_per_block;
    return !do_div(size, erasesize);
}

static int read_userid(struct rt_mtd_nand_device *mtd, size_t offset, u8 *buf, size_t size)
{
    size_t end = offset + size;
    rt_uint32_t page_id, blk_id;
    size_t remain, erasesize;
    u8 *ptr;

    erasesize = mtd->page_size * mtd->pages_per_block;
    if (erasesize < USERID_MAX_SIZE)
        remain = erasesize;
    else
        remain = size;
    ptr = buf;

    while ((remain > 0) && (offset < end)) {
        blk_id = offset / erasesize;
        if (is_aligned_with_block_size(mtd, offset) &&
            rt_mtd_nand_check_block(mtd, blk_id)) {
            offset += erasesize;
            continue;
        }

        page_id = offset / mtd->page_size;
        if (rt_mtd_nand_read(mtd, page_id, ptr, mtd->page_size, RT_NULL, 0))
            return 1;

        offset += mtd->page_size;
        ptr += mtd->page_size;
        remain -= mtd->page_size;
    }

    return 0;
}

static int spinand_userid_load(void)
{
    struct userid_storage_header head;
    struct rt_mtd_nand_device *mtd;
    size_t offset, size;
    rt_device_t dev_id;
    int ret = -1;
    u8 *buf;

    if (!g_userid_mtd) {
        dev_id = rt_device_find(USERID_PARTITION_NAME);
        if (dev_id == RT_NULL) {
            LOG_E("Cannot find %s.\n", USERID_PARTITION_NAME);
            return -1;
        }
        ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open MTD %s failed.!\n", USERID_PARTITION_NAME);
            return ret;
        }
        g_userid_mtd = (struct rt_mtd_nand_device *)dev_id;
    }

    mtd = g_userid_mtd;

    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
    if (!buf) {
        ret = -1;
        goto err;
    }

    offset = 0;
    size = mtd->page_size;
    ret = read_userid(mtd, offset, buf, size);
    if (ret) {
        pr_err("read userid from offset 0x%x failed.\n", offset);
        ret = -EIO;
        goto err;
    }

    memcpy(&head, buf, sizeof(head));
    if (head.magic != USERID_HEADER_MAGIC)
        goto err;

    offset += mtd->page_size;
    size = USERID_MAX_SIZE - mtd->page_size;
    ret = read_userid(mtd, offset, (buf + mtd->page_size), size);
    if (ret) {
        pr_err("read userid from offset 0x%x failed.\n", offset);
        ret = -EIO;
        goto err;
    }

    ret = userid_import(buf);

err:
    if (buf)
        aicos_free_align(0, buf);
    return ret;
}

static int write_userid(struct rt_mtd_nand_device *mtd, size_t offset, u8 *buf, u32 len)
{
    size_t end = offset + USERID_MAX_SIZE;
    rt_uint32_t page_id, blk_id;
    size_t remain, erasesize;
    u8 *ptr;

    erasesize = mtd->page_size * mtd->pages_per_block;
    remain = len;
    ptr = buf;

    while ((remain > 0) && (offset < end)) {
        blk_id = offset / erasesize;
        if (rt_mtd_nand_check_block(mtd, blk_id)) {
            offset += erasesize;
            continue;
        }

        page_id = offset / mtd->page_size;
        if (rt_mtd_nand_write(mtd, page_id, ptr, mtd->page_size, RT_NULL, 0))
            return 1;

        offset += mtd->page_size;
        remain -= mtd->page_size;
        ptr += mtd->page_size;
    }

    return 0;
}

static int spinand_userid_save(void)
{
    struct rt_mtd_nand_device *mtd;
    rt_device_t dev_id;
    int ret = -1;
    u32 len;
    u8 *buf;

    if (!g_userid_mtd) {
        dev_id = rt_device_find(USERID_PARTITION_NAME);
        if (dev_id == RT_NULL) {
            LOG_E("Cannot find %s.\n", USERID_PARTITION_NAME);
            return -1;
        }
        ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open MTD %s failed.!\n", USERID_PARTITION_NAME);
            return ret;
        }
        g_userid_mtd = (struct rt_mtd_nand_device *)dev_id;
    }

    mtd = g_userid_mtd;
    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
    if (!buf) {
        pr_err("Failed to malloc buffer.\n");
        ret = -1;
        goto err;
    }
    memset(buf, 0xff, USERID_MAX_SIZE);
    ret = userid_export(buf);
    if (ret <= 0) {
        pr_err("Failed to export userid to buffer.\n");
        goto err;
    }
    len = ret;

    pr_info("Erasing ...\n");
    /* Aleast 1 block */
    if (rt_mtd_nand_erase_block(mtd, 0)) {
        pr_err("SPINAND: erase failed.\n");
        ret = 1;
        goto err;
    }

    pr_info("Writing ...\n");
    ret = write_userid(mtd, 0, buf, len);
    if (ret) {
        pr_err("SPINAND: save userid failed.\n");
        ret = 2;
        goto err;
    }

err:
    if (buf)
        aicos_free_align(0, buf);
    return 0;
}

struct userid_driver userid_spinand = {
    .name    = "SPINAND",
    .load    = spinand_userid_load,
    .save    = spinand_userid_save,
};
