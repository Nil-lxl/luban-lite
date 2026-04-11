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
#include <rtdevice.h>
#include <rtthread.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_time.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include <userid.h>
#include "userid_internal.h"

static struct rt_mtd_nor_device *g_mtd = NULL;
static int spinor_userid_load(void)
{
    struct userid_storage_header *head;
    size_t total_len, offset, size;
    int ret = -1;
    u8 *head_buf = NULL, *buf = NULL;
    rt_device_t dev_id;

    if (!g_mtd) {
        dev_id = rt_device_find(USERID_PARTITION_NAME);
        if (!dev_id) {
            pr_err("Cannot find flash.\n");
            return -1;
        }
        ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open MTD %s failed.!\n", USERID_PARTITION_NAME);
            return ret;
        }
        g_mtd = (struct rt_mtd_nor_device *)dev_id;
    }

    head_buf = aicos_malloc_align(0, 256, CACHE_LINE_SIZE);
    if (!head_buf) {
        ret = -1;
        goto err;
    }

    offset = 0;
    size = 256;
    if (rt_mtd_nor_read(g_mtd, offset, head_buf, size) != size) {
        pr_err("read userid from offset 0x%x failed.\n", offset);
        ret = -1;
        goto err;
    }
    head = (void *)head_buf;
    if (head->magic != USERID_HEADER_MAGIC)
        goto err;

    total_len = head->total_length + 8;
    if (total_len < 256)
        total_len = 256;
    if (total_len & 0xFF) {
        total_len &= ~0xFF;
        total_len += 256;
    }
    buf = aicos_malloc_align(0, total_len, CACHE_LINE_SIZE);
    if (!buf) {
        ret = -1;
        goto err;
    }
    memcpy(buf, head_buf, 256);

    if (total_len > 256) {
        offset = 256;
        rt_mtd_nor_read(g_mtd, offset, (buf + 256), total_len - 256);
    }
    ret = userid_import(buf);

err:
    if (head_buf)
        aicos_free_align(0, head_buf);
    if (buf)
        aicos_free_align(0, buf);
    return ret;
}

static int spinor_userid_save(void)
{
    int ret = -1;
    u32 len;
    u8 *buf;
    rt_device_t dev_id;

    if (!g_mtd) {
        dev_id = rt_device_find(USERID_PARTITION_NAME);
        if (!dev_id) {
            pr_err("Cannot find flash.\n");
            return -1;
        }
        ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open MTD %s failed.!\n", USERID_PARTITION_NAME);
            return ret;
        }
        g_mtd = (struct rt_mtd_nor_device *)dev_id;
    }

    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
    if (!buf) {
        ret = -1;
        goto err;
    }
    memset(buf, 0xff, USERID_MAX_SIZE);
    ret = userid_export(buf);
    if (ret <= 0) {
        pr_err("Failed to export userid to buffer.\n");
        goto err;
    }
    len = ROUND(ret, 4096);

    pr_info("Erasing ...\n");
    if (rt_mtd_nor_erase_block(g_mtd, 0, len)) {
        pr_err("SPINOR: erase failed\n");
        ret = -1;
        goto err;
    }

    pr_info("Writing ...\n");
    if (rt_mtd_nor_write(g_mtd, 0, buf, len) != len) {
        pr_err("SPINOR: save userid failed.\n");
        ret = -2;
        goto err;
    }

err:
    if (buf)
        aicos_free_align(0, buf);
    return 0;
}

struct userid_driver userid_spinor = {
    .name    = "SPINOR",
    .load    = spinor_userid_load,
    .save    = spinor_userid_save,
};
