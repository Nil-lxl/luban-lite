/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <rtthread.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_utils.h>
#include <aic_log.h>

static int do_mmc_dump(int argc, char **argv)
{
    unsigned long blk_cnt, blk_offset;
    rt_device_t mmc_dev = RT_NULL;
    char *name, *pe;
    u8 *p;

    if (argc != 4) {
        printf("Input error, reference: mmc dump mmc0p0 0 1\n");
        return -1;
    }

    name = argv[1];
    blk_offset = strtoul(argv[2], &pe, 0);
    blk_cnt = strtoul(argv[3], &pe, 0);
    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }
    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    p = malloc(512);

    for (int i = 0; i < blk_cnt; i++) {
        rt_device_read(mmc_dev, blk_offset, p, blk_cnt);
        hexdump(p, 512, 1);
    }
    rt_device_close(mmc_dev);

    free(p);
    return 0;
}

static int do_mmc_read(int argc, char **argv)
{
    unsigned long addr, blkcnt, blkoffset;
    rt_device_t mmc_dev = RT_NULL;
    char *name, *pe;
    void *p;

    if (argc != 5) {
        printf("Input error, reference: mmc read mmc0p0 0 1 0x100000\n");
        return -1;
    }

    name = argv[1];
    blkoffset = strtoul(argv[2], &pe, 0);
    blkcnt = strtoul(argv[3], &pe, 0);
    addr = strtoul(argv[4], &pe, 0);

    p = (void *)addr;
    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }
    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    rt_device_read(mmc_dev, blkoffset, p, blkcnt);

    rt_device_close(mmc_dev);
    return 0;
}

static int do_mmc_write(int argc, char **argv)
{
    unsigned long addr, blkcnt, blkoffset;
    rt_device_t mmc_dev = RT_NULL;
    char *name, *pe;
    void *p;

    if (argc != 5) {
        printf("Input error, reference: mmc write mmc0p0 0 1 0x100000\n");
        return -1;
    }

    name = argv[1];
    blkoffset = strtoul(argv[2], &pe, 0);
    blkcnt = strtoul(argv[3], &pe, 0);
    addr = strtoul(argv[4], &pe, 0);

    p = (void *)addr;
    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }
    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    rt_device_write(mmc_dev, blkoffset, p, blkcnt);

    rt_device_close(mmc_dev);
    return 0;
}

static int do_mmc_erase(int argc, char **argv)
{
    unsigned long blkcnt, blkoffset;
    rt_device_t mmc_dev = RT_NULL;
    char *name, *pe;
    unsigned long long p[2] = {0};

    if (argc != 4) {
        printf("Input error, reference: mmc erase mmc0p0 0 1\n");
        return -1;
    }

    name = argv[1];
    blkoffset = strtoul(argv[2], &pe, 0);
    blkcnt = strtoul(argv[3], &pe, 0);

    p[0] = blkoffset;
    p[1] = blkcnt;

    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }
    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    rt_device_control(mmc_dev, RT_DEVICE_CTRL_BLK_ERASE, (void *)p);

    rt_device_close(mmc_dev);
    return 0;
}

static int do_mmc_get_info(int argc, char **argv)
{
    rt_device_t mmc_dev = RT_NULL;
    char *name;
    struct rt_device_blk_geometry get_data;

    if (argc != 2) {
        printf("Input error, reference: mmc get_info mmc0p0\n");
        return -1;
    }

    name = argv[1];

    mmc_dev = rt_device_find(name);
    if (mmc_dev == RT_NULL) {
        LOG_E("can't find mmc device!");
        return RT_ERROR;
    }
    rt_device_open(mmc_dev, RT_DEVICE_FLAG_RDWR);

    rt_device_control(mmc_dev, RT_DEVICE_CTRL_BLK_GETGEOME, (void *)&get_data);

    printf("sector_count:%d bytes_per_sector:%d block_size:%d.\n",
        get_data.sector_count, get_data.bytes_per_sector, get_data.block_size);

    rt_device_close(mmc_dev);
    return 0;
}

static void do_mmc_help(void)
{
    printf("ArtInChip MMC read/write command\n\n");
    printf("mmc help                                  : Show this help\n");
    printf("mmc dump  name blkoffset blkcnt        : Dump data from block_dev\n");
    printf("mmc read  name blkoffset blkcnt addr   : Read data from block_dev to RAM address\n");
    printf("mmc write name blkoffset blkcnt addr   : Write data to block_dev from RAM address\n");
    printf("mmc erase name blkoffset blkcnt        : Erase data of block_dev (only support mmc,not sd)\n");
    printf("mmc get_info name                      : Get info from block_dev\n");
    printf("Example:\n");
    printf("    mmc dump sd0 0 1\n");
}

static int cmd_mmc_do(int argc, char **argv)
{
    if (argc <= 1)
        goto help;

    if (!strcmp(argv[1], "dump"))
        return do_mmc_dump(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "read"))
        return do_mmc_read(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "write"))
        return do_mmc_write(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "erase"))
        return do_mmc_erase(argc - 1, &argv[1]);
    else if (!strcmp(argv[1], "get_info"))
        return do_mmc_get_info(argc - 1, &argv[1]);

help:
    do_mmc_help();

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_mmc_do, mmc, ArtInChip MMC command);
