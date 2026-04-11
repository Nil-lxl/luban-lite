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
#include <aic_errno.h>
#include <aic_utils.h>
#include <spinand_port.h>

#define SPI_BUS_ID         0
#define SPINAND_BLOCK_SIZE 0x20000
#define SPINAND_PAGE_SIZE  0x800

/* It is preferable to be within the data partition. */
#define TEST_SIZE   0x20000
#define TEST_OFFSET 0x02180000

static struct aic_spinand *g_spinand_flash = NULL;
static u64 g_total_block = TEST_SIZE / SPINAND_BLOCK_SIZE;
static u64 g_block_offset = TEST_OFFSET / SPINAND_BLOCK_SIZE;
static u8 *g_write_buf = NULL;
static u8 *g_read_buf = NULL;

static void address_map(u8 *data, u32 start_address, u32 len)
{
    u32 i;

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

static int spinand_init(int spi_bus, u32 malloc_len)
{
    if (g_spinand_flash == NULL)
        g_spinand_flash = spinand_probe(spi_bus);

    if (g_spinand_flash == NULL) {
        printf("Failed to probe spinand flash.\n");
        return -1;
    }

    /* Allocate test data buffer */
    g_write_buf = aicos_malloc_align(0, malloc_len, CACHE_LINE_SIZE);
    if (!g_write_buf) {
        pr_err("No memory for test data!\n");
        return -1;
    }

    /* Allocate read buffer */
    g_read_buf = aicos_malloc_align(0, malloc_len, CACHE_LINE_SIZE);
    if (!g_read_buf) {
        pr_err("No memory for read buffer!\n");
        aicos_free_align(0, g_write_buf);
        return -1;
    }

    /* prepare some data */
    for (int i = 0; i < malloc_len; i++) {
        g_write_buf[i] = i % 256;
    }

    return 0;
}

static int spinand_badblock_check()
{
    u32 block = 0, bad_block_cnt = 0;

    if (g_spinand_flash == NULL)
        return -1;

    for (block = g_block_offset; block < g_block_offset + g_total_block; block++) {
        if (spinand_block_isbad(g_spinand_flash, block)) {
            printf("found bad block %u\n", block);
            spinand_block_markbad(g_spinand_flash, block);
            bad_block_cnt++;
        }
    }

    printf("There are %u bad blocks in the whole %llu blocks.\n", bad_block_cnt, g_total_block);

    return 0;
}

/**
 * @brief Skip bad blocks when performing bulk erase operation on SPINAND flash memory.
 *
 * @param[in] flash      Pointer to AIC SPINAND device structure
 * @param[in] offset     Starting byte address for erase operation
 * @param[in] size       Total bytes to process (must be multiple of block size)
 * @return              0 on full success, -1 if any erase attempt failed
 */
static int skip_badblock_erase(struct aic_spinand *flash, u32 offset, u32 size)
{
    u32 erase_block_num = size / SPINAND_BLOCK_SIZE;
    int err = 0;
    u32 flash_size = (flash->info->page_size) * (flash->info->pages_per_eraseblock) *
                     (flash->info->block_per_lun) * (flash->info->planes_per_lun);

    while (erase_block_num) {
        if (spinand_block_isbad(g_spinand_flash, offset / SPINAND_BLOCK_SIZE)) {
            offset += SPINAND_BLOCK_SIZE;
            if (offset > flash_size) {
                pr_err("Offset exceeds flash size\r\n");
                return -1;
            }
        } else {
            erase_block_num--;
            err = spinand_block_erase(g_spinand_flash, offset / SPINAND_BLOCK_SIZE);
            if (err != 0) {
                pr_err("Erase block %u failed\r\n", offset / SPINAND_BLOCK_SIZE);
                return -1;
            }
        }
    }
    return 0;
}

/**
 * @brief Perform write-then-read verification test on SPINAND device.
 *
 * @param[in] offset     Starting byte address for I/O operations
 * @param[in] len        Data length to process (subject to alignment adjustments)
 * @return              0 on success, negative error code otherwise
 */
static int spinand_write_read(u32 offset, u32 len)
{
    int err = 0;
    u32 erase_len = len;

    printf("\n === SPINAND write read example ===\n");

    /* Check whether the address range and offset are correct. */
    if (len % SPINAND_BLOCK_SIZE != 0) {
        pr_warn("len is not aligned with erase size! "
                "Adjusting to a larger erase block.\n");
        erase_len = (len / SPINAND_BLOCK_SIZE + 1) * SPINAND_BLOCK_SIZE;
    }

    if (len % SPINAND_PAGE_SIZE != 0) {
        pr_warn("len is not aligned with write size! "
                "Adjusting read and write len to a smaller write page.\n");
        len = (len / SPINAND_PAGE_SIZE) * SPINAND_PAGE_SIZE;
    }

    if (offset % SPINAND_BLOCK_SIZE != 0) {
        pr_warn("Offset is not aligned with erase size! "
                "Adjusting to the smaller erase block boundary.\n");
        offset = (offset / SPINAND_BLOCK_SIZE) * SPINAND_BLOCK_SIZE;
    }

    /*
    * Erase the specified region of the SPINAND device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = skip_badblock_erase(g_spinand_flash, offset, erase_len);
    if (err) {
        pr_err("Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = spinand_write(g_spinand_flash, g_write_buf, offset, len);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    memset(g_read_buf, 0, len);
    /* Read the specified region of the SPINAND device. */
    err = spinand_read(g_spinand_flash, g_read_buf, offset, len);
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

        printf("SUCCESS: SPINAND read/write test passed!\n");
    }

exit:
    return err;
}

/**
 * @brief Update specific data segment in SPINAND while handling alignment constraints.
 *
 * @param[in] offset     Target starting byte address for update
 * @param[in] data       Pointer to new data to be written
 * @param[in] data_len   Length of data to update (bytes)
 * @return              0 on success, negative error code otherwise
 */
static int spinand_refresh(u32 offset, u8 *data, u32 data_len)
{
    u32 len, total_len;
    u32 start_address, offset_in_block;
    int err;

    printf("\n === SPINAND refresh example ===\n");

    /* Check whether the offset are correct. */
    if (offset % SPINAND_BLOCK_SIZE != 0) {
        pr_warn("Offset is not aligned with erase size! "
                "Adjusting to the smaller erase block boundary.\n");
        start_address = (offset / SPINAND_BLOCK_SIZE) * SPINAND_BLOCK_SIZE;
    } else {
        start_address = offset;
    }

    /* Calculate the offset within the block and the actual length of the update. */
    offset_in_block = offset % SPINAND_BLOCK_SIZE;
    total_len = offset_in_block + data_len;
    if (total_len % SPINAND_BLOCK_SIZE != 0) {
        len = (total_len / SPINAND_BLOCK_SIZE + 1) * SPINAND_BLOCK_SIZE;
    } else {
        len = total_len;
    }

    /* Allocate buffer */
    u8 *buffer = aicos_malloc_align(0, len, CACHE_LINE_SIZE);
    if (!buffer) {
        pr_err("No memory for buffer!\n");
        return -1;
    }

    /*
    * Read the specified region of the SPINAND device.
    */
    err = spinand_read(g_spinand_flash, buffer, start_address, len);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Replace data */
    memcpy(buffer + offset_in_block, data, data_len);

    /*
    * Erase the specified region of the SPINAND device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = skip_badblock_erase(g_spinand_flash, start_address, len);
    if (err) {
        pr_err("Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = spinand_write(g_spinand_flash, buffer, start_address, len);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    printf("The position of the data in the block:\n");
    memset(buffer, 0, len);
    err = spinand_read(g_spinand_flash, buffer, start_address, len);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    address_map(buffer + offset_in_block - 16, start_address + offset_in_block - 16, data_len + 32);

    printf("SUCCESS: SPINAND refresh data test passed!\n");

exit:
    if (buffer)
        aicos_free_align(0, buffer);
    return err;
}

/**
 * Execution Flow:
 *   - Attempts to initialize hardware (failure aborts early)
 *   - Scans for factory marked defective blocks
 *   - Verifies full-block storage reliability
 *   - Demonstrates incremental data modification capability
 *   - Ensures proper memory deallocation in all exit paths
 *
 * Note: You can use the spinand_block_markbad function to simulate bad blocks.
 */
static int cmd_spinand_example(int argc, char *argv[])
{
    int err = 0;

    if (spinand_init(SPI_BUS_ID, SPINAND_BLOCK_SIZE)) {
        pr_err("SPINAND init failed!\n");
        err = -1;
        goto exit;
    }

    spinand_badblock_check();

    if (spinand_write_read(TEST_OFFSET, TEST_SIZE)) {
        err = -1;
        goto exit;
    }

    u8 data[10] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
    if (spinand_refresh(TEST_OFFSET, data, 10)) {
        err = -1;
        goto exit;
    }

exit:
    printf("spinand usage end.\n");

    /* Free resources */
    if (g_write_buf)
        aicos_free_align(0, g_write_buf);
    if (g_read_buf)
        aicos_free_align(0, g_read_buf);
    return err;
}

CONSOLE_CMD(spinand_usage, cmd_spinand_example, "spinand example");
