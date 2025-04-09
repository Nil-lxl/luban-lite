/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __SPI_NAND_BLOCK_H__
#define __SPI_NAND_BLOCK_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "spinand_parts.h"

#ifndef AIC_SPINAND_BLOCK_CACHE_SIZE
#define BLOCK_CACHE_SIZE    512
#else
#define BLOCK_CACHE_SIZE    AIC_SPINAND_BLOCK_CACHE_SIZE
#endif

#ifndef AIC_SPINAND_BLOCK_CACHE_NUM
#define BLOCK_CACHE_NUM     4
#else
#define BLOCK_CACHE_NUM     AIC_SPINAND_BLOCK_CACHE_NUM
#endif

struct blk_cache {
    u32 pos;
    char buf[BLOCK_CACHE_SIZE];
};

struct spinand_blk_device {
    struct rt_device parent;
    struct rt_device_blk_geometry geometry;
    struct rt_mtd_nand_device *mtd_device;
#ifdef AIC_NFTL_SUPPORT
    struct nftl_api_handler_t *nftl_handler;
#endif
    char name[32];
    enum part_attr attr;
    u8 *pagebuf;
    struct blk_cache blk_cache[BLOCK_CACHE_NUM];
};

int rt_blk_nand_register_device(const char *name, struct rt_mtd_nand_device *mtd_device);

#endif /* __SPI_NAND_BLOCK_H__ */
