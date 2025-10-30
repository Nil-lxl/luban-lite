/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.Chen <jiji.chenen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include <spinand_bbt.h>

#define BBT_RESERVED_BLOCKS   8

static int spinand_bbt_map_blocks(struct aic_spinand_bbt *spinand_bbt)
{
    u32 block_temp = spinand_bbt->start_block;
    u32 good_block = 0, bad_block_count = 0;

    for (int i = 0; i < spinand_bbt->use_block_num; i++) {
        for (int j = bad_block_count + i; j < spinand_bbt->total_block_num; j++) {
            block_temp = j + spinand_bbt->start_block;
            if (!spinand_block_isbad(spinand_bbt->spinand_flash, block_temp))
                good_block++;
            else
                bad_block_count += 1;

            /* Find the (i)th good block. */
            if (good_block - i == 1) {
                spinand_bbt->bbt[i] = block_temp;
                block_temp++;
                break;
            }

            if (j == spinand_bbt->total_block_num) {
                pr_err("There are too many bad blocks: %u\n", bad_block_count);
                return -BBT_BAD_OVER;
            }
        }
    }

    /* Debug info only. */
    for (int j = 0; j < spinand_bbt->use_block_num; j++) {
        pr_debug("logic_block: %u --> physic block: %u\n", j + spinand_bbt->start_block, spinand_bbt->bbt[j]);
    }

    spinand_bbt->bad_block_num = bad_block_count;

    return BBT_OK;
}

static u16 spinand_bbt_get_phyblk_by_logblk(struct aic_spinand_bbt *spinand_bbt, u32 logic_block)
{
    u16 physic_block = (u16)spinand_bbt->bbt[logic_block - spinand_bbt->start_block];
    return physic_block;
}

static int spinand_bbt_block_copy(struct aic_spinand *flash, u32 src_block, u32 dst_block)
{
    u32 page_size = flash->info->page_size;
    u32 ppb = flash->info->pages_per_eraseblock;
    u8 *data = (u8 *)malloc(page_size);
    int ret = BBT_OK;

    if (!data) {
        pr_err("malloc failed, size: %u\n", page_size);
        free(data);
        ret = -BBT_ERR;
        goto copy_done;
    }

    if (src_block >= dst_block) {
        pr_err("Illegal parameter(%u:%u)\n", src_block, dst_block);
        ret = -BBT_ERR_UNKNOW;
        goto copy_done;
    }

    u32 src_page = src_block * ppb;
    u32 dst_page = dst_block * ppb;

    for (int i = 0; i < ppb; i++) {
        spinand_read_page(flash, src_page, (uint8_t *)data, page_size, NULL, 0);
        spinand_write_page(flash, dst_page, (uint8_t *)data, page_size, NULL, 0);
        src_page++;
        dst_page++;
    }

copy_done:
    if (data)
        free(data);

    return ret;
}

static int spinand_bbt_block_data_transfer(struct aic_spinand_bbt *spinand_bbt, u32 physic_block)
{
    int ret = 0;

    /* Step 1: check if the block num if enough */
    if (spinand_bbt->bad_block_num == spinand_bbt->reserved_block_num) {
        pr_err("There are too many bad blocks: %u\n", spinand_bbt->bad_block_num + 1);
        return -BBT_BAD_OVER;
    }

    /* Step 2: found a good block in reserved blocks */
    u32 block_temp = spinand_bbt->end_block - spinand_bbt->reserved_block_num + spinand_bbt->bad_block_num;

    for (; block_temp < spinand_bbt->end_block; block_temp++) {
        if (!spinand_block_isbad(spinand_bbt->spinand_flash, block_temp)) {
            /* found a good block */
            if (!spinand_block_erase(spinand_bbt->spinand_flash, block_temp))
                break;
            /* erase failed it is a bad block */
            pr_err("Failed to erase block: %u\n", block_temp);
        }
    }
    if (block_temp == spinand_bbt->end_block) {
        pr_err("There are too many bad blocks\n");
        return -BBT_BAD_OVER;
    }

    u32 logic_block = spinand_bbt->end_block - spinand_bbt->reserved_block_num - 1;
    u32 block_src = (u32)spinand_bbt_get_phyblk_by_logblk(spinand_bbt, logic_block);

    /* Step 3: copy blocks data */
    while (1) {
        ret = spinand_bbt_block_copy(spinand_bbt->spinand_flash, block_src, block_temp);
        if (ret)
            return ret;
        logic_block--;
        block_temp--;
        block_src = (u32)spinand_bbt_get_phyblk_by_logblk(spinand_bbt, logic_block);
        if (block_src < physic_block)
            break;
        while (spinand_block_isbad(spinand_bbt->spinand_flash, block_temp))
        {
            block_temp--;
        }
        if (spinand_block_erase(spinand_bbt->spinand_flash, block_temp)) {
            pr_err("Failed to erase block: %u, un handle!\n", block_temp);
            return -BBT_NEW_BAD;
        }
    }

    if (!spinand_block_isbad(spinand_bbt->spinand_flash, physic_block)) {
        spinand_block_markbad(spinand_bbt->spinand_flash, physic_block);
    }

    /* Step last: remap blocks */
    if (spinand_bbt_map_blocks(spinand_bbt)) {
        pr_err("Fail to map blocks.\n");
        return -BBT_ERR;
    }

    return BBT_OK;
}

/**
 * @spinand_bbt: device with which will be init
 * @block_reserved: reserve blocks for bad block management
 * @offset: start offset of spinand which data will be operation
 * @size: length of spinand which data will operation
 *
 * Return: zero on success, else a negative error code.
 */
int spinand_bbt_init(struct aic_spinand_bbt *spinand_bbt, u32 block_reserved, u32 offset, u32 size)
{
    u32 use_block_num;

    if (spinand_bbt->spinand_flash == NULL) {
        pr_err("Failed to probe spinand flash.\n");
        return -BBT_ERR;
    }

    u32 block_size = spinand_bbt->spinand_flash->info->page_size * spinand_bbt->spinand_flash->info->pages_per_eraseblock;
    u32 total_block_num = size / block_size;
    if (offset % block_size || size % block_size) {
        pr_err("Not support parameters block_size: %u, offset: %u, size: %u\n",
                block_size, offset, size);
        return -BBT_ERR;
    }

    if (block_reserved == 0)
        spinand_bbt->reserved_block_num = BBT_RESERVED_BLOCKS;
    else
        spinand_bbt->reserved_block_num = block_reserved;

    if (total_block_num < spinand_bbt->reserved_block_num) {
        pr_err("Not support argvs: total blocks: %u, reserved blocks: %d\n",
                total_block_num, spinand_bbt->reserved_block_num);
        return -BBT_ERR;
    }

    spinand_bbt->start_block = offset / block_size;
    spinand_bbt->end_block = total_block_num + spinand_bbt->start_block;
    spinand_bbt->total_block_num = total_block_num;
    use_block_num = total_block_num - spinand_bbt->reserved_block_num;
    spinand_bbt->use_block_num = use_block_num;
    spinand_bbt->bbt = (u32 *)malloc(sizeof(u32) * use_block_num);
    if (spinand_bbt->bbt == NULL) {
        pr_err("Fail to malloc memory for %zu bytes.\n", (sizeof(u32) * use_block_num));
        return -BBT_ERR;
    }

    if (spinand_bbt_map_blocks(spinand_bbt)) {
        pr_err("Fail to map blocks.\n");
        return -BBT_ERR;
    }

    pr_info("spinand_bbt init success! \n");
    pr_info("start_block: %u, end_block: %u, total_block_num: %u\n",
            spinand_bbt->start_block, spinand_bbt->end_block, total_block_num);
    pr_info("bad blocks: %u, reserved blocks: %u, blocks can use: %u\n",
        spinand_bbt->bad_block_num, spinand_bbt->reserved_block_num, spinand_bbt->use_block_num);

    return BBT_OK;
}

/**
 * @spinand_bbt: device with which data will be read
 * @data: memory address which host the read to
 * @offset: start offset of spinand which data will read
 * @size: length of spinand which data will read
 *
 * Return: zero on success, else a negative error code.
 */
int spinand_bbt_read(struct aic_spinand_bbt *spinand_bbt, u8 *data, u32 offset, u32 size)
{
    u32 block_size = 0, start_block, block_len;
    uint8_t *data_temp;
    unsigned long offset_temp, size_temp;
    int ret = 0;

    block_size = spinand_bbt->spinand_flash->info->page_size * spinand_bbt->spinand_flash->info->pages_per_eraseblock;

    data_temp = data;
    offset_temp = offset;
    size_temp = size;

    start_block = offset / block_size;
    block_len = (size + block_size) / block_size;
    if (start_block + block_len > spinand_bbt->use_block_num + spinand_bbt->start_block ||
        start_block < spinand_bbt->start_block) {
        pr_err("Spinand_bbt out of range, please check.%u:%u\n", start_block, block_len);
        return -BBT_ERR;
    }

read_a_block:
    while (block_len)
    {
        u16 physic_block = spinand_bbt_get_phyblk_by_logblk(spinand_bbt, start_block);
        if (spinand_block_isbad(spinand_bbt->spinand_flash, physic_block)) {
            pr_err("Exceed bad block: %u\n", physic_block);
            ret = spinand_bbt_block_data_transfer(spinand_bbt, physic_block);
            if (ret)
                return ret;
            goto read_a_block;
        }

        /* The first time read the left of the block most, others read whole block. */
        u32 current_block_size_left = block_size - offset_temp % block_size;
        u32 do_size = size_temp > current_block_size_left ? current_block_size_left : size_temp;
        u32 do_offset = offset_temp % block_size + physic_block * block_size;
        if (spinand_read(spinand_bbt->spinand_flash, (uint8_t *)data_temp, do_offset, do_size)) {
            pr_err("Read failed, offset: %u, size: %u\n", do_offset, do_size);
            return -BBT_ERR;
        }

        data_temp += do_size;
        size_temp -= do_size;
        block_len--;
        start_block++;
        /* The first time read the left of the block most, others read whole block. */
        offset_temp = 0;
    }

    return BBT_OK;
}

/**
 * @spinand_bbt: device with which data will be written
 * @addr: memory address which host the data to write
 * @offset: start offset of spinand which data will be written
 * @size: length of spinand which data will be written
 *
 * Return: zero on success, else a negative error code.
 */
int spinand_bbt_write(struct aic_spinand_bbt *spinand_bbt, u8 *addr, u32 offset, u32 size)
{
    u32 block_size = 0, start_block, block_len;
    u8 *addr_temp;
    unsigned long offset_temp, size_temp;
    int ret = 0;

    block_size = spinand_bbt->spinand_flash->info->page_size * spinand_bbt->spinand_flash->info->pages_per_eraseblock;

    addr_temp = addr;
    offset_temp = offset;
    size_temp = size;

    start_block = offset / block_size;
    block_len = (size + block_size) / block_size;
    if (start_block + block_len > spinand_bbt->use_block_num + spinand_bbt->start_block ||
        start_block < spinand_bbt->start_block) {
        pr_err("Spinand_bbt out of range, please check.%u:%u\n", start_block, block_len);
        return -BBT_ERR;
    }

write_a_block:
    while (block_len)
    {
        u16 physic_block = spinand_bbt_get_phyblk_by_logblk(spinand_bbt, start_block);
        if (spinand_block_isbad(spinand_bbt->spinand_flash, physic_block)) {
            pr_err("Exceed bad block: %u\n", physic_block);
            ret = spinand_bbt_block_data_transfer(spinand_bbt, physic_block);
            if (ret)
                return ret;
            goto write_a_block;
        }

        /* The first time write the left of the block most, others write whole block. */
        u32 current_block_size_left = block_size - offset_temp % block_size;
        u32 do_size = size_temp > current_block_size_left ? current_block_size_left : size_temp;
        u32 do_offset = offset_temp % block_size + physic_block * block_size;
        if (spinand_write(spinand_bbt->spinand_flash, (uint8_t *)addr_temp, do_offset, do_size)) {
            pr_err("Write failed, offset: %u, size: %u\n", do_offset, do_size);
            return -BBT_ERR;
        }

        addr_temp += do_size;
        size_temp -= do_size;
        block_len--;
        start_block++;
        /* The first time write the left of the block most, others write whole block. */
        offset_temp = 0;
    }

    return BBT_OK;
}

/**
 * @spinand_bbt: device with which data will be erased
 * @offset: start offset of spinand which data will be erased
 * @size: length of spinand which data will be erased
 *
 * Return: zero on success, else a negative error code.
 */
int spinand_bbt_erase(struct aic_spinand_bbt *spinand_bbt, u32 offset, u32 size)
{
    u32 block_size = 0, start_block, block_len;
    int ret = 0;

    block_size = spinand_bbt->spinand_flash->info->page_size * spinand_bbt->spinand_flash->info->pages_per_eraseblock;

    start_block = offset / block_size;
    block_len = size / block_size;
    if (start_block + block_len > spinand_bbt->use_block_num + spinand_bbt->start_block ||
        start_block < spinand_bbt->start_block) {
            pr_err("Spinand_bbt out of range, please check.%u:%u\n", start_block, block_len);
        return -BBT_ERR;
    }

erase_a_block:
    while (block_len)
    {
        u16 physic_block = spinand_bbt_get_phyblk_by_logblk(spinand_bbt, start_block);
        if (spinand_block_isbad(spinand_bbt->spinand_flash, physic_block)) {
            pr_err("Exceed bad block: %u\n", physic_block);
            ret = spinand_bbt_block_data_transfer(spinand_bbt, physic_block);
            if (ret)
                return ret;
            goto erase_a_block;
        }

        if (spinand_block_erase(spinand_bbt->spinand_flash, physic_block)) {
            pr_err("Failed to erase block: %u\n", physic_block);
            ret = spinand_bbt_block_data_transfer(spinand_bbt, physic_block);
            if (ret)
                return ret;
            goto erase_a_block;
        }

        block_len--;
        start_block++;
    }

    return BBT_OK;
}

void spinand_bbt_deinit(struct aic_spinand_bbt *spinand_bbt)
{
    if (spinand_bbt->bbt)
        free(spinand_bbt->bbt);

    memset(spinand_bbt, 0x00, sizeof(struct aic_spinand_bbt));
}
