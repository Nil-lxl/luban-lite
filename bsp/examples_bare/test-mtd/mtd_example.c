/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_core.h>
#include <mtd.h>
#include <rtconfig.h>

#define PARTITION_NAME       "data"
#define TEST_READ_WRITE_SIZE 0x1000
#define TEST_OFFSET          0

static struct mtd_dev *g_mtd_dev;
static u8 *g_write_buf = NULL;
static u8 *g_read_buf = NULL;

static void address_map(u8 *data, u32 start_address, u32 len)
{
    u32 i = 0;

    for (i = 0; i < len; i++) {
        if (i && (i % 16) == 0)
            printf("\n");
        if ((i % 16) == 0) {
            printf("0x%08x : ", start_address);
            start_address += 16;
        }

        printf("%02x ", data[i]);
    }
    printf("\n");
}

/**
 * @brief      Perform MTD device write/read test procedure.
 *             Includes erase, write, readback and data verification steps.
 * @param[in]  mtd        Pointer to MTD device structure.
 * @param[in]  offset     Starting address (must align with erase block boundary).
 * @param[in]  len        Operation length (auto-adjusted for alignment constraints).
 * @return     0 on success; negative error code on failure.
 */
static int mtd_write_read(struct mtd_dev *mtd, u32 offset, u32 len)
{
    u32 block_size = mtd->erasesize;
#ifdef AIC_SPINAND_DRV
    u32 page_size = mtd->writesize;
#else
    u32 page_size = 0x100; /* There is no writesize member in NOR MTD. */
#endif
    int err = 0;
    u32 erase_len = len;

    printf("\n === MTD write read example ===\n");

    /* Check whether the address range and offset are correct. */
    if (len % block_size != 0) {
        pr_warn("len is not aligned with erase size! "
                "Adjusting to a larger erase block.\n");
        erase_len = (len / block_size + 1) * block_size;
    }
    if (len % page_size != 0) {
        pr_warn("len is not aligned with write size! "
                "Adjusting read and write len to a smaller write page.\n");
        len = (len / page_size) * page_size;
    }
    if (offset % block_size != 0) {
        pr_warn("Offset is not aligned with erase size! "
                "Adjusting to the smaller erase block boundary.\n");
        offset = (offset / block_size) * block_size;
    }

    /*
    * Erase the specified region of the MTD device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = mtd_erase(mtd, offset, erase_len);
    if (err) {
        pr_err("Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = mtd_write(mtd, offset, g_write_buf, len);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    memset(g_read_buf, 0, len);
    /* Read the specified region of the MTD device. */
    err = mtd_read(mtd, offset, g_read_buf, len);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Data verification */
    if (memcmp(g_write_buf, g_read_buf, len) != 0) {
        pr_err("Data verification failed!\n");
        printf("expected:-----------------------\n");
        address_map(g_write_buf, offset, len);
        printf("actual:-------------------------\n");
        address_map(g_read_buf, offset, len);
    } else {
        if (len > 0x200) {
            address_map(g_read_buf, offset, 0x100);
            printf("......\n");
            address_map(g_read_buf + len - 0x50, offset + len - 0x50, 0x50);
        } else {
            address_map(g_read_buf, offset, len);
        }
        printf("SUCCESS: SPINAND bbt read/write test passed!\n");
    }

exit:
    return err;
}

/**
 * @brief      Refresh MTD device by updating specific data segment.
 *             The function reads existing content, modifies target region with new data,
 *             erases the entire affected block, then writes back updated content.
 * @param[in]  mtd        Pointer to MTD device structure.
 * @param[in]  offset     Starting address of target update location.
 * @param[in]  data       Pointer to new data buffer for replacement.
 * @param[in]  data_len   Length of new data to write (bytes).
 * @return     0 on success; negative error code on failure.
 */
static int mtd_refresh(struct mtd_dev *mtd, u32 offset, u8 *data, u32 data_len)
{
    u32 block_size = mtd->erasesize;
    u32 len = 0, total_len = 0;
    u32 start_address = 0, offset_in_block = 0;
    u8 *buffer = NULL;
    int err = 0;

    printf("\n === MTD refresh example ===\n");

    /* Check whether the offset are correct. */
    if (offset % block_size != 0) {
        pr_warn("Offset is not aligned with erase size! "
                "Adjusting to the smaller erase block boundary.\n");
        start_address = (offset / block_size) * block_size;
    } else {
        start_address = offset;
    }

    /* Calculate the offset within the block and the actual length of the update. */
    offset_in_block = offset % block_size;
    total_len = offset_in_block + data_len;
    if (total_len % block_size != 0) {
        len = (total_len / block_size + 1) * block_size;
    } else {
        len = total_len;
    }

    /* Allocate buffer */
    buffer = aicos_malloc_align(0, len, CACHE_LINE_SIZE);
    if (!buffer) {
        pr_err("No memory for buffer!\n");
        return -1;
    }

    /*
    * Read the specified region of the MTD device.
    */
    err = mtd_read(mtd, start_address, buffer, len);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Replace data */
    memcpy(buffer + offset_in_block, data, data_len);

    /*
    * Erase the specified region of the MTD device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = mtd_erase(mtd, start_address, len);
    if (err) {
        pr_err("Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = mtd_write(mtd, start_address, buffer, len);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    printf("The position of the data in the block:\n");
    memset(buffer, 0, len);
    err = mtd_read(mtd, start_address, buffer, len);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }
    address_map(buffer + offset_in_block - 16, start_address + offset_in_block - 16, data_len + 32);

    printf("SUCCESS: MTD refresh data test passed!\n");

exit:
    if (buffer)
        aicos_free_align(0, buffer);
    return err;
}

/**
 * @brief      Demonstrate basic MTD (Memory Technology Device) usage patterns including:
 *             - Allocation of aligned buffers for read/write operations
 *             - Device initialization and partition selection
 *             - Sequential write/read verification cycle
 *             - Partial update via refresh mechanism
 *             - Proper resource cleanup on success or failure paths
 */
static int cmd_mtd_example(int argc, char *argv[])
{
    int err = 0;

    /* Allocate test data buffer */
    g_write_buf = aicos_malloc_align(0, TEST_READ_WRITE_SIZE, CACHE_LINE_SIZE);
    if (!g_write_buf) {
        pr_err("No memory for test data!\n");
        err = -1;
        goto exit;
    }

    /* Allocate read buffer */
    g_read_buf = aicos_malloc_align(0, TEST_READ_WRITE_SIZE, CACHE_LINE_SIZE);
    if (!g_read_buf) {
        pr_err("No memory for read buffer!\n");
        err = -1;
        goto exit;
    }

    /* prepare some data */
    for (int i = 0; i < TEST_READ_WRITE_SIZE; i++) {
        g_write_buf[i] = i % 256;
    }

    g_mtd_dev = mtd_get_device(PARTITION_NAME);
    if (!g_mtd_dev) {
        pr_err("MTD partition '%s' not found!\n", PARTITION_NAME);
        err = -1;
        goto exit;
    }

    if (mtd_write_read(g_mtd_dev, TEST_OFFSET, TEST_READ_WRITE_SIZE)) {
        err = -1;
        goto exit;
    }

    u8 data[10] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
    if (mtd_refresh(g_mtd_dev, 4094, data, 10)) {
        err = -1;
        goto exit;
    }

exit:
    printf("mtd example end.\n");

    if (g_write_buf)
        aicos_free_align(0, g_write_buf);
    if (g_read_buf)
        aicos_free_align(0, g_read_buf);
    return err;
}

CONSOLE_CMD(mtd_usage, cmd_mtd_example, "mtd example");
