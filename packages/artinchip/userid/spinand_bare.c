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
#include <mtd.h>
#include <userid.h>
#include "userid_internal.h"

static struct mtd_dev *g_userid_mtd = NULL;

static bool is_aligned_with_block_size(struct mtd_dev *mtd, u64 size)
{

    return !do_div(size, mtd->erasesize);
}

static int read_userid(struct mtd_dev *mtd, size_t offset, u8 *buf, size_t size)
{
    size_t end = offset + size;
    size_t remain;
    u8 *ptr;

    if (mtd->erasesize < USERID_MAX_SIZE)
        remain = mtd->erasesize;
    else
        remain = size;
    ptr = buf;

    while ((remain > 0) && (offset < end)) {
        if (is_aligned_with_block_size(mtd, offset) &&
            mtd_block_isbad(mtd, offset)) {
            offset += mtd->erasesize;
            continue;
        }

        if (mtd_read(mtd, offset, ptr, remain))
            return 1;

        offset += remain;
        ptr += remain;
    }

    return 0;
}

static int spinand_userid_load(void)
{
    struct userid_storage_header head;
    struct mtd_dev *mtd;
    size_t offset, size;
    int ret = -1;
    u8 *buf;

    if (!g_userid_mtd) {
        mtd_probe();
        g_userid_mtd = mtd_get_device(USERID_PARTITION_NAME);
    }

    mtd = g_userid_mtd;

    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
    if (!buf) {
        ret = -1;
        goto err;
    }

    offset = 0;
    size = mtd->writesize;
    ret = read_userid(mtd, offset, buf, size);
    if (ret) {
        pr_err("read userid from offset 0x%x failed.\n", (unsigned int)offset);
        ret = -EIO;
        goto err;
    }

    memcpy(&head, buf, sizeof(head));
    if (head.magic != USERID_HEADER_MAGIC)
        goto err;

    offset += mtd->writesize;
    size = USERID_MAX_SIZE - mtd->writesize;
    ret = read_userid(mtd, offset, (buf + mtd->writesize), size);
    if (ret) {
        pr_err("read userid from offset 0x%x failed.\n", (unsigned int)offset);
        ret = -EIO;
        goto err;
    }

    ret = userid_import(buf);

err:
    if (buf)
        aicos_free_align(0, buf);
    return ret;
}

static int write_userid(struct mtd_dev *mtd, size_t offset, u8 *buf, u32 len)
{
    size_t end = offset + USERID_MAX_SIZE;
    size_t remain;
    u8 *ptr;

    remain = len;
    ptr = buf;

    while ((remain > 0) && (offset < end)) {
        if (mtd_block_isbad(mtd, offset)) {
            offset += mtd->erasesize;
            continue;
        }

        if (mtd_write(mtd, offset, ptr, remain))
            return 1;

        offset += remain;
        remain -= remain;
        ptr += remain;
    }

    return 0;
}

static int spinand_userid_save(void)
{
    struct mtd_dev *mtd;
    int ret = -1;
    u32 len;
    u8 *buf;

    if (!g_userid_mtd) {
        mtd_probe();
        g_userid_mtd = mtd_get_device(USERID_PARTITION_NAME);
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
    if (mtd_erase(mtd, 0, USERID_MAX_SIZE)) {
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
