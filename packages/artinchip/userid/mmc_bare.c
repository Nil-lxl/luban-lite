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
#include <mmc.h>
#include <disk_part.h>
#include <userid.h>
#include "userid_internal.h"

static int mmc_get_userid_part(u32 *start_blk, u32 *blk_cnt)
{
    struct aic_partition *part, *parts = NULL;
    int ret = 0;

    *start_blk = 0;
    *blk_cnt = 0;

    parts = mmc_create_gpt_part();
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
        mmc_free_partition(parts);

    return ret;
}

static int mmc_userid_load(void)
{
	struct userid_storage_header head;
    struct aic_sdmc *mmc;
	int ret = -1;
    u32 offset, cnt, mmc_id;
	u8 *buf;

    mmc_id = 0;
    mmc_init(mmc_id);
    mmc = find_mmc_dev_by_index(mmc_id);
    
    if (!mmc) {
        printf("Failed to get mmc device.\n");
        return -1;
    }
    ret = mmc_get_userid_part(&offset, &cnt);
    if (ret)
        return -1;

    buf = aicos_malloc_align(0, USERID_MAX_SIZE, CACHE_LINE_SIZE);
	if (!buf) {
		ret = -1;
		goto err;
	}

	cnt = 1;
	ret = mmc_bread(mmc, offset, cnt, buf);
	if (ret != cnt) {
		pr_err("read userid from lba offset 0x%x failed.\n", offset);
		ret = -EIO;
		goto err;
	}

	memcpy(&head, buf, sizeof(head));
	if (head.magic != USERID_HEADER_MAGIC)
		goto err;

	offset += 1;
	cnt = USERID_MAX_SIZE / 512 - 1;
    ret = mmc_bread(mmc, offset, cnt, (buf + 512));
    if (ret != cnt) {
		pr_err("read userid from lba offset 0x%x failed.\n", offset);
		ret = -1;
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
    struct aic_sdmc *mmc;
	int ret = -1;
	u32 len, offset, cnt, mmc_id;
	u8 *buf;

    mmc_id = 0;
    mmc_init(mmc_id);
    mmc = find_mmc_dev_by_index(mmc_id);
    
    if (!mmc) {
        printf("Failed to get mmc device.\n");
        return -1;
    }
    ret = mmc_get_userid_part(&offset, &cnt);
    if (ret)
        return -1;
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

	pr_info("Writing ...\n");
	cnt = DIV_ROUND_UP(len, 512);
	ret = mmc_bwrite(mmc, offset, cnt, buf);
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
