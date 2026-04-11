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
#include <spinand_bbt.h>

#define SPI_BUS_ID 0

#define SPINAND_BLOCK_SIZE 0x20000
#define SPINAND_PAGE_SIZE  0x800

/* Reserved blocks should account for 10%~20% of the bad blocks managed by the bad block table. */
#define BBT_BLOCK_NUM 0x005

/*
* It is best for the scope of bad block table management to lie within the data partition,
* so as not to compromise the system firmware.
*/
#define BBT_OFFSET 0x3A00000 /* start offset of spinand which data will be operation */
#define BBT_SIZE   0x600000  /* length of spinand which data will operation */

/* Attention:The range of read/write erasure should be within the bbt table. */
#define TEST_READ_WRITE_SIZE 0x8000
#define TEST_OFFSET          0x3A00000

static struct aic_spinand_bbt *s_spinand_bbt_t = NULL;
static u8 *s_write_buf = NULL;
static u8 *s_read_buf = NULL;

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

static int verify_data(u8 *data_1, u8 *data_2, u32 offset, u32 len)
{
    if (memcmp(data_1, data_2, len) != 0) {
        printf("expected:-----------------------\n");
        address_map(data_1, offset, len);
        printf("actual:-------------------------\n");
        address_map(data_2, offset, len);
        return -1;
    } else {
        if (len > 0x200) {
            address_map(data_2, offset, 0x100);
            printf("......\n");
            address_map(data_2 + len - 0x50, offset + len - 0x50, 0x50);
        } else {
            address_map(data_2, offset, len);
        }
        return 0;
    }
}

static int data_init(u32 malloc_len)
{
    /* Allocate test data buffer */
    s_write_buf = aicos_malloc_align(0, malloc_len, CACHE_LINE_SIZE);
    if (!s_write_buf) {
        printf("Error: No memory for test data!\n");
        return -1;
    }

    /* Allocate read buffer */
    s_read_buf = aicos_malloc_align(0, malloc_len, CACHE_LINE_SIZE);
    if (!s_read_buf) {
        printf("Error: No memory for read buffer!\n");
        aicos_free_align(0, s_write_buf);
        return -1;
    }

    /* prepare some data */
    for (int i = 0; i < malloc_len; i++) {
        s_write_buf[i] = i % 256;
    }
    return 0;
}

static void check_adjust(u32 *offset, u32 *len, u32 *erase_len)
{
    if (*len % SPINAND_BLOCK_SIZE != 0) {
        printf("Warning: len is not aligned with erase size! "
               "Adjusting to a larger erase block.\n");
        *erase_len = (*len / SPINAND_BLOCK_SIZE + 1) * SPINAND_BLOCK_SIZE;
    }

    if (*len % SPINAND_PAGE_SIZE != 0) {
        printf("Warning: len is not aligned with write size! "
               "Adjusting read and write len to a smaller write page.\n");
        *len = (*len / SPINAND_PAGE_SIZE) * SPINAND_PAGE_SIZE;
    }

    if (*offset % SPINAND_BLOCK_SIZE != 0) {
        printf("Warning: Offset is not aligned with erase size! "
               "Adjusting to the smaller erase block boundary.\n");
        *offset = (*offset / SPINAND_BLOCK_SIZE) * SPINAND_BLOCK_SIZE;
    }
}

/**
 * @brief      Initialize SPI NAND device and build Bad Block Table (BBT)
 *
 * @param[in]  spi_bus     SPI bus number to use
 *
 * @return                 Returns 0 on success, -1 on failure
 */
static int spinand_init(int spi_bus)
{
    s_spinand_bbt_t = malloc(sizeof(struct aic_spinand_bbt));
    if (!s_spinand_bbt_t) {
        printf("malloc failed\n");
        return -1;
    }

    s_spinand_bbt_t->spinand_flash = spinand_probe(spi_bus);

    if (s_spinand_bbt_t->spinand_flash == NULL) {
        printf("Failed to probe spinand flash.\n");
        return -1;
    }

    return spinand_bbt_init(s_spinand_bbt_t, BBT_BLOCK_NUM, BBT_OFFSET, BBT_SIZE);
}

/**
 * @brief Performs a full cycle of operations on a SPINAND flash memory region:
 *        1. Checks and adjusts alignment of length and offset.
 *        2. Erases the target blocks.
 *        3. Writes test data to the pages within those blocks.
 *        4. Reads back the data.
 *        5. Verifies the integrity of the written data.
 *
 * @param[in] offset The starting address in flash memory where the operation begins.
 *                  Will be adjusted to align with a block boundary if necessary.
 * @param[in] len    The total number of bytes to process (erase/write/read).
 *                  Both 'len' and 'offset' may get modified by this function for alignment.
 *
 * @return An integer error code (0 on success, non-zero otherwise).
 */
static int spinand_write_read(u32 offset, u32 len)
{
    int err = 0;
    u32 erase_len = len;

    printf("\n === SPINAND bbt write read example ===\n");

    /* Check whether the address range and offset are correct */
    check_adjust(&offset, &len, &erase_len);

    /*
    * Erase the specified region of the SPINAND device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = spinand_bbt_erase(s_spinand_bbt_t, offset, erase_len);
    if (err) {
        printf("Error: Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = spinand_bbt_write(s_spinand_bbt_t, s_write_buf, offset, len);
    if (err) {
        printf("Error: Write failed (code=%d)!\n", err);
        goto exit;
    }

    memset(s_read_buf, 0, len);
    /* Read the specified region of the SPINAND device. */
    err = spinand_bbt_read(s_spinand_bbt_t, s_read_buf, offset, len);
    if (err) {
        printf("Error: Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Data verification */
    if (verify_data(s_read_buf, s_write_buf, offset, len))
        printf("Error: Data verification failed!\n");
    else
        printf("SUCCESS: SPINAND bbt read/write test passed!\n");

exit:
    return err;
}

/**
 * @brief      Perform refresh operation on SPI NAND flash memory
 *             Writes provided data buffer to specified memory region of the device.
 *
 * @param[in]  offset       Starting address in bytes from memory origin
 * @param[in]  data         Pointer to source data buffer containing update payload
 * @param[in]  data_len     Length of data to write (number of bytes)
 *
 * @return An integer error code (0 on success, non-zero otherwise).
 */
static int spinand_refresh(u32 offset, u8 *data, u32 data_len)
{
    u32 len, total_len;
    u32 start_address, offset_in_block;
    int err;

    printf("\n === SPINAND bbt refresh example ===\n");

    /* Check whether the offset are correct. */
    if (offset % SPINAND_BLOCK_SIZE != 0) {
        printf("Warning: Offset is not aligned with erase size! "
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
        printf("Error: No memory for buffer!\n");
        return -1;
    }

    /*
    * Read the specified region of the SPINAND device.
    */
    err = spinand_bbt_read(s_spinand_bbt_t, buffer, start_address, len);
    if (err) {
        printf("Error: Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Replace data */
    memcpy(buffer + offset_in_block, data, data_len);

    /*
    * Erase the specified region of the SPINAND device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = spinand_bbt_erase(s_spinand_bbt_t, start_address, len);
    if (err) {
        printf("Error: Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = spinand_bbt_write(s_spinand_bbt_t, buffer, start_address, len);
    if (err) {
        printf("Error: Write failed (code=%d)!\n", err);
        goto exit;
    }

    printf("The position of the data in the block:\n");
    memset(buffer, 0, len);
    err = spinand_bbt_read(s_spinand_bbt_t, buffer, start_address, len);
    if (err) {
        printf("Error: Read failed (code=%d)!\n", err);
        goto exit;
    }

    address_map(buffer + offset_in_block - 16, start_address + offset_in_block - 16, data_len + 32);

    printf("SUCCESS: SPINAND bbt refresh data test passed!\n");

exit:
    if (buffer)
        aicos_free_align(0, buffer);
    return err;
}

static int cmd_spinand_example(int argc, char *argv[])
{
    int err = 0;

    /* Allocate memory for the test array and prepare test data. */
    err = data_init(SPINAND_BLOCK_SIZE);
    if (err) {
        pr_err("data init failed!\n");
        goto exit;
    }

    /* Initialize the bad block table */
    err = spinand_init(SPI_BUS_ID);
    if (err) {
        pr_err("SPINAND init failed!\n");
        goto exit;
    }

    /* Erase the data, rewrite it, and then read back to verify data consistency. */
    err = spinand_write_read(TEST_OFFSET, SPINAND_BLOCK_SIZE);
    if (err) {
        pr_err("SPINAND write read failed!\n");
        goto exit;
    }

    /* Update the data in the specified area. */
    u8 data[10] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
    err = spinand_refresh(TEST_OFFSET, data, 10);
    if (err)
        pr_err("SPINAND refresh failed!\n");

exit:
    printf("spinand bbt usage end\n");

    /* Free resources */
    if (s_spinand_bbt_t) {
        spinand_bbt_deinit(s_spinand_bbt_t);
        free(s_spinand_bbt_t);
        s_spinand_bbt_t = NULL;
    }

    if (s_write_buf)
        aicos_free_align(0, s_write_buf);
    if (s_read_buf)
        aicos_free_align(0, s_read_buf);

    return err;
}

CONSOLE_CMD(spinand_bbt_example, cmd_spinand_example, "spinand bbt example");
