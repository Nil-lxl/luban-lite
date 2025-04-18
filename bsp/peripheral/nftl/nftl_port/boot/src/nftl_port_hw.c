/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: mingfeng.li <mingfeng.li@artinchip.com>
 */

#define _NFTL_NFTL_PORT_HW_C_

#include "mtd.h"
#include <aic_common.h>
#include "nftl_api.h"

#if BARE_PORT_HW_DEBUG
#define PORT_HW_LOG printf
#else
#define PORT_HW_LOG(...)
#endif


void *nftl_memcpy(void *str1, const void *str2, int size)
{
    return memcpy(str1, str2, size);
}

void nftl_memset(void *str, int c, int size)
{
    memset(str, c, size);
}

void *nftl_malloc(u32 size)
{
    if (size > 0x180000)
        PORT_HW_LOG("[NE]malloc size too large %d!\n", size);

    return aicos_malloc_align(MEM_CMA, size, CACHE_LINE_SIZE);
}

void nftl_free(const void *ptr)
{
    aicos_free_align(MEM_CMA, (void *)ptr);
}

int _nftl_port_hw_erase_block(void *device, struct physical_op_info *p)
{
    struct mtd_dev *nand = (struct mtd_dev *)device;
    int offset = p->physical_page.block_num * nand->erasesize;
    mtd_erase(nand, offset, nand->erasesize);

    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return 0;
}

int _nftl_port_hw_read_page(void *device, struct physical_op_info *p)
{
    struct mtd_dev *nand = (struct mtd_dev *)device;
    u8 src_buf[64];
    int ret;
    int this_pages_per_block = nand->erasesize / nand->writesize;
    int page =
        p->physical_page.block_num * this_pages_per_block + p->physical_page.page_num;
    //NFTL_INFO("%s:%d ...p->physical_page.block_num=%d, p->physical_page.page_num=%d page=%d\n", __FUNCTION__, __LINE__, p->physical_page.block_num, p->physical_page.page_num, page);
    int offset = page * nand->writesize;
    ret = mtd_read_oob(nand, offset, p->user_data_addr, nand->writesize,
                           p->spare_data_addr, 64);
    memcpy(src_buf, p->spare_data_addr, 64);
    ret = mtd_unmap_oob_user_region(nand, p->spare_data_addr, src_buf, 0, 16);
    if (ret) {
        PORT_HW_LOG("[NE] failed to unmap data from oob user regions. ret = %d!\n", ret);
        return ret;
    }

    memset(p->spare_data_addr + 16, 0xFF, 8);

    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return ret;
}

int _nftl_port_hw_write_page(void *device, struct physical_op_info *p)
{
    struct mtd_dev *nand = (struct mtd_dev *)device;
    u8 src_buf[16];
    int ret = 0;
    int this_pages_per_block = nand->erasesize / nand->writesize;
    int page =
        p->physical_page.block_num * this_pages_per_block + p->physical_page.page_num;
    int offset = page * nand->writesize;
    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);

    memcpy(src_buf, p->spare_data_addr, 16);
    ret = mtd_map_oob_user_region(nand, p->spare_data_addr, src_buf, 0, 16);
    if (ret) {
        PORT_HW_LOG("[NE] failed to map data to oob user regions. ret = %d!\n", ret);
        return ret;
    }

    ret = mtd_write_oob(nand, offset, p->user_data_addr, nand->writesize,
                        p->spare_data_addr, 64);

    return ret;
}

#define IS_BAD          1
#define NFTL_BLOCK_GOOD 1
#define NFTL_BLOCK_BAD  0
int _nftl_port_hw_is_block_good(void *device, struct physical_op_info *p)
{
    struct mtd_dev *nand = (struct mtd_dev *)device;
    int ret = NFTL_BLOCK_GOOD;
    int offset = p->physical_page.block_num * nand->erasesize;
    ret = mtd_block_isbad(nand, offset);
    PORT_HW_LOG("%s:%d ...\n", __FUNCTION__, __LINE__);
    return (ret == IS_BAD) ? NFTL_BLOCK_BAD : NFTL_BLOCK_GOOD;
}

int _nftl_port_hw_mark_bad_block(void *device, struct physical_op_info *p)
{
    struct mtd_dev *nand = (struct mtd_dev *)device;
    //NFTL_INFO("%s:%d p->physical_page.block_num=%d...\n", __FUNCTION__, __LINE__, p->physical_page.block_num);
    int offset = p->physical_page.block_num * nand->erasesize;
    return mtd_block_markbad(nand, offset);
}
