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
#include <disk_part.h>
#include <userid.h>
#include "userid_internal.h"

static rt_device_t g_dev_id = RT_NULL;

static rt_device_t mmc_userid_get_device(void)
{
    rt_device_t dev_id;
    int ret;

    if (g_dev_id == RT_NULL) {
        dev_id = rt_device_find("mmc0");
        if (dev_id == RT_NULL) {
            LOG_E("Cannot find mmc0.\n");
            return RT_NULL;
        }
        ret = rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);
        if (ret) {
            LOG_E("Open mmc0 failed.!\n");
            return RT_NULL;
        }
        g_dev_id = dev_id;
    }

    return g_dev_id;
}

static unsigned long mmcsd_userid_write(struct blk_desc *blk_dev, u64 start,
                                        u64 blkcnt, void *buffer)
{
    int ret;

    ret = rt_device_read(blk_dev->priv, start, buffer, blkcnt);
    if (ret == blkcnt)
        return blkcnt;
    return 0;
}

static unsigned long mmcsd_userid_read(struct blk_desc *blk_dev, u64 start,
                                       u64 blkcnt, const void *buffer)
{
    int ret;

    ret= rt_device_write(blk_dev->priv, start, (void *)buffer, blkcnt);
    if (ret == blkcnt)
        return blkcnt;
    return 0;
}

static int mmc_get_userid_part(u32 *start_blk, u32 *blk_cnt)
{
    rt_device_t dev_id;
    struct aic_partition *part, *parts = NULL;
    struct blk_desc dev_desc;
    struct disk_blk_ops ops;
    int ret = 0;

    *start_blk = 0;
    *blk_cnt = 0;

    dev_id = mmc_userid_get_device();
    if (!dev_id)
        return -1;

    ops.blk_write = mmcsd_userid_write;
    ops.blk_read = mmcsd_userid_read;
    aic_disk_part_set_ops(&ops);
    dev_desc.blksz = 512;
    dev_desc.priv = dev_id;
    parts = aic_disk_get_parts(&dev_desc);
    if (!parts) {
        pr_err("create gpt part failed.\n");
        ret = -1;
        goto out;
    }

    part = parts;
    while (part) {
        if (!strcmp(part->name, USERID_PARTITION_NAME))
            break;

        part = part->next;
    }

    if (!part) {
        printf("Failed to get application partition.\n");
        ret = -1;
        goto out;
    }

    *start_blk = part->start / 512;
    *blk_cnt = part->size / 512;
out:
    if (parts)
        aic_part_free(parts);

    return ret;
}

static int mmc_userid_load(void)
{
	struct userid_storage_header head;
    rt_device_t dev_id;
	int ret = -1;
    u32 offset, cnt;
	u8 *buf;

    dev_id = mmc_userid_get_device();
    if (!dev_id)
        return -1;

    ret = mmc_get_userid_part(&offset, &cnt);
    if (ret)
        return -1;

    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
	if (!buf) {
		ret = -1;
		goto err;
	}

	cnt = 1;
	ret = rt_device_read(dev_id, offset, buf, cnt);
	if (ret != cnt) {
		pr_err("read userid from lba offset 0x%x failed.\n", offset);
		ret = -1;
		goto err;
	}

	memcpy(&head, buf, sizeof(head));
	if (head.magic != USERID_HEADER_MAGIC)
		goto err;

	offset += 1;
	cnt = USERID_MAX_SIZE / 512 - 1;
	ret = rt_device_read(dev_id, offset, (buf + 512), cnt);
	if (ret != cnt) {
		pr_err("read userid from lba offset 0x%x failed.\n", offset);
		ret = -EIO;
		goto err;
	}

	ret = userid_import(buf);

err:
	if (buf)
        aicos_free_align(0, buf);
	return ret;
}

static int mmc_userid_save(void)
{
    rt_device_t dev_id;
	int ret = -1;
	u32 len, offset, cnt;
	u8 *buf;

    dev_id = mmc_userid_get_device();
    if (!dev_id)
        return -1;

    ret = mmc_get_userid_part(&offset, &cnt);
    if (ret)
        return -1;
    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
	if (!buf) {
		pr_err("Failed to malloc buffer.\n");
		ret = -ENOMEM;
		goto err;
	}
	memset(buf, 0xff, USERID_MAX_SIZE);
	ret = userid_export(buf);
	if (ret <= 0) {
		pr_err("Failed to export userid to buffer.\n");
		goto err;
	}
	len = ret;

	pr_info("Writing ...\n");
	cnt = DIV_ROUND_UP(len, 512);
	ret = rt_device_write(dev_id, offset, buf, cnt);
	if (ret != cnt) {
		pr_err("mmc: save userid failed.\n");
		ret = -1;
		goto err;
	}

err:
	if (buf)
        aicos_free_align(0, buf);

	return 0;
}

struct userid_driver userid_mmc = {
	.name	= "mmc",
	.load	= mmc_userid_load,
	.save	= mmc_userid_save,
};
