/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rtthread.h>
#include <disk_part.h>
#include <aic_utils.h>

#define PART_HELP          \
    "Partition command:\n" \
    "  part list <name>\n" \
    "  part dump <name>\n" \
    "  e.g.: \n"           \
    "    part list sd0\n"      \
    "    part dump sd0\n"

static void cmd_part_help(void)
{
    puts(PART_HELP);
}

static unsigned long mmc_write(struct blk_desc *block_dev, u64 start,
                               u64 blkcnt, void *buffer)
{
    return rt_device_write(block_dev->priv, start, (void *)buffer, blkcnt);
}

static unsigned long mmc_read(struct blk_desc *block_dev, u64 start, u64 blkcnt,
                              const void *buffer)
{
    unsigned long ret;

    ret = rt_device_read(block_dev->priv, start, (void *)buffer, blkcnt);
    printf("read:start %lld, cnt %lld\n", start, blkcnt);
    hexdump((void *)buffer, 512 * blkcnt, 1);
    return ret;
}

static int cmd_part_list(char *name)
{
    struct blk_desc dev_desc;
    rt_device_t mmc_dev = RT_NULL;
    struct aic_partition *parts, *p;
    struct disk_blk_ops ops;

    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);

    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }

    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);
    dev_desc.blksz = 512;
    dev_desc.priv = mmc_dev;

    parts = aic_disk_get_parts(&dev_desc);

    p = parts;
    while (p) {
        printf("Start: 0x%08llx; Size: 0x%08llx; %s\n", p->start, p->size, p->name);
        p = p->next;
    }
    if (parts)
        aic_part_free(parts);
    rt_device_close(mmc_dev);
    return 0;
}

static int cmd_part_dump_gpt(char *name)
{
    struct blk_desc dev_desc;
    rt_device_t mmc_dev = RT_NULL;
    struct disk_blk_ops ops;

    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);

    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }
    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    dev_desc.blksz = 512;
    dev_desc.priv = mmc_dev;

    aic_disk_dump_parts(&dev_desc);
    rt_device_close(mmc_dev);
    return 0;
}

static int do_part(int argc, char *argv[])
{
    char *cmd = NULL;

    cmd = argv[1];
    if (argc == 3) {
        if (!strcmp(cmd, "list")) {
            cmd_part_list(argv[2]);
            return 0;
        }
        if (!strcmp(cmd, "dump")) {
            cmd_part_dump_gpt(argv[2]);
            return 0;
        }
    }

    cmd_part_help();

    return 0;
}

MSH_CMD_EXPORT_ALIAS(do_part, part, ArtInChip partition command);
