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
#include <sfud.h>

#define SPI_BUS_ID           0
#define SPINOR_BLOCK_SIZE    0x1000
#define SPINOR_PAGE_SIZE     0x100
#define TEST_READ_WRITE_SIZE 0x1000
#define TEST_OFFSET          0

extern sfud_flash *sfud_probe(u32 spi_bus);

static sfud_flash *s_spinor_flash;
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

static int spinor_init(int spi_bus, u32 malloc_len)
{
    if (s_spinor_flash == NULL)
        s_spinor_flash = sfud_probe(spi_bus);

    if (s_spinor_flash == NULL) {
        printf("Failed to probe spinor flash.\n");
        return -1;
    }

    /* Allocate test data buffer */
    s_write_buf = aicos_malloc_align(0, malloc_len, CACHE_LINE_SIZE);
    if (!s_write_buf) {
        pr_err("No memory for test data!\n");
        return -1;
    }

    /* Allocate read buffer */
    s_read_buf = aicos_malloc_align(0, malloc_len, CACHE_LINE_SIZE);
    if (!s_read_buf) {
        pr_err("No memory for read buffer!\n");
        aicos_free_align(0, s_write_buf);
        return -1;
    }

    /* prepare some data */
    for (int i = 0; i < malloc_len; i++) {
        s_write_buf[i] = i % 256;
    }

    return 0;
}

/**
 * @brief      Perform write-then-read test on SPINOR flash memory.
 * @details    Erases, writes pattern data, reads back and verifies integrity.
 *             Automatically aligns offset/length to device constraints:
 *                 - Erase alignment (block size)
 *                 - Write alignment (page size)
 *             Includes data comparison between written and read buffers.
 *
 * @param[in]  offset  Starting address for operation (will be adjusted if needed)
 * @param[in]  len     Length of data to process (adjusted per device specs)
 *
 * @return     0 on success; negative error code on failure
 */
static int spinor_write_read(u32 offset, u32 len)
{
    int err = 0;
    u32 erase_len = len;

    printf("\n === SPINOR write read example ===\n");

    /* Check whether the address range and offset are correct. */
    if (len % SPINOR_BLOCK_SIZE != 0) {
        pr_warn("len is not aligned with erase size! "
                "Adjusting to a larger erase block.\n");
        erase_len = (len / SPINOR_BLOCK_SIZE + 1) * SPINOR_BLOCK_SIZE;
    }
    if (len % SPINOR_PAGE_SIZE != 0) {
        pr_warn("len is not aligned with write size! "
                "Adjusting read and write len to a smaller write page.\n");
        len = (len / SPINOR_PAGE_SIZE) * SPINOR_PAGE_SIZE;
    }
    if (offset % SPINOR_BLOCK_SIZE != 0) {
        pr_warn("Offset is not aligned with erase size! "
                "Adjusting to the smaller erase block boundary.\n");
        offset = (offset / SPINOR_BLOCK_SIZE) * SPINOR_BLOCK_SIZE;
    }

    /*
    * Erase the specified region of the SPINOR device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = sfud_erase(s_spinor_flash, offset, erase_len);
    if (err) {
        pr_err("Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = sfud_write(s_spinor_flash, offset, len, s_write_buf);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    memset(s_read_buf, 0, len);
    /* Read the specified region of the SPINOR device. */
    err = sfud_read(s_spinor_flash, offset, len, s_read_buf);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Data verification */
    if (memcmp(s_write_buf, s_read_buf, len) != 0) {
        pr_err("Data verification failed!\n");
        printf("expected:-----------------------\n");
        address_map(s_write_buf, offset, len);
        printf("actual:-------------------------\n");
        address_map(s_read_buf, offset, len);
    } else {
        if (len > 0x200) {
            address_map(s_read_buf, offset, 0x100);
            printf("......\n");
            address_map(s_read_buf + len - 0x50, offset + len - 0x50, 0x50);
        } else {
            address_map(s_read_buf, offset, len);
        }
        printf("SUCCESS: SPINOR read/write test passed!\n");
    }

exit:
    return err;
}

/**
 * @brief      Refresh data in SPINOR flash memory.
 * @details    Updates a segment of SPINOR starting at 'offset' with new data.
 *             Aligns unaligned offsets to block boundaries automatically.
 *             Reads->Modifies->Erase->Writes the affected blocks.
 *             Validates success after write operation.
 *
 * @param[in]  offset      Starting address (bytes)
 * @param[in]  data        Pointer to new data buffer
 * @param[in]  data_len    Length of new data in bytes
 *
 * @return     0 on success; negative error code on failure
 */
static int spinor_refresh(u32 offset, u8 *data, u32 data_len)
{
    u32 len, total_len;
    u32 start_address, offset_in_block;
    int err;

    printf("\n === SPINOR refresh example ===\n");

    /* Check whether the offset are correct. */
    if (offset % SPINOR_BLOCK_SIZE != 0) {
        pr_warn("Offset is not aligned with erase size! "
                "Adjusting to the smaller erase block boundary.\n");
        start_address = (offset / SPINOR_BLOCK_SIZE) * SPINOR_BLOCK_SIZE;
    } else {
        start_address = offset;
    }

    /* Calculate the offset within the block and the actual length of the update. */
    offset_in_block = offset % SPINOR_BLOCK_SIZE;
    total_len = offset_in_block + data_len;
    if (total_len % SPINOR_BLOCK_SIZE != 0) {
        len = (total_len / SPINOR_BLOCK_SIZE + 1) * SPINOR_BLOCK_SIZE;
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
    * Read the specified region of the SPINOR device.
    */
    err = sfud_read(s_spinor_flash, start_address, len, buffer);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Replace data */
    memcpy(buffer + offset_in_block, data, data_len);

    /*
    * Erase the specified region of the SPINOR device.
    * Note: The erase operation can only be performed in blocks,
    * so ensure that the offset and len parameters are integer multiples of the block size.
    */
    err = sfud_erase(s_spinor_flash, start_address, len);
    if (err) {
        pr_err("Erase failed (code=%d)!\n", err);
        goto exit;
    }

    /*
    * Write test data,it must be erased first.
    * Note: The write operation can only be performed in pages,
    * so ensure that the offset and len parameters are integer multiples of the page size.
    */
    err = sfud_write(s_spinor_flash, start_address, len, buffer);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    printf("The block where the data is located:\n");
    memset(buffer, 0, len);
    err = sfud_read(s_spinor_flash, start_address, len, buffer);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }
    address_map(buffer, start_address, len);

    printf("SUCCESS: SPINOR refresh data test passed!\n");

exit:
    if (buffer)
        aicos_free_align(0, buffer);
    return err;
}

/**
 * @brief Configure the protection area range
 *
 * @param is_top     true: protect top address range, false: protect bottom address range
 * @param block_size Protection size (0: no protection, 1: 64KB, 2: 128KB,..., 7: full chip protection)
 *
 * @return     0 on success; negative error code on failure
 */
static int spinor_set_protect_area(bool is_top, u8 block_size)
{
    sfud_err err;
    u8 sr1, sr1_read;

    err = sfud_read_reg(s_spinor_flash, 0x05, &sr1);
    if (err) {
        printf("Read status register1 failure.\n");
        return err;
    }

    /* Configure protection direction (TB bit) */
    if (is_top) {
        sr1 &= ~(1 << 5); // TB=0 (protect top)
    } else {
        sr1 |= (1 << 5); // TB=1 (protect buttom)
    }

    /* Configure protection range (BP2-BP0 bits) */
    sr1 &= ~0x1C;             // Clear BP2-BP0 bits
    sr1 |= (block_size << 2); // Set BP2-BP0 bits

    /* Write to status registers */
    printf("Write Value: 0x%x into sr1\n", sr1);
    err = sfud_write_status_ext(s_spinor_flash, false, 0x05, sr1);
    if (err) {
        printf("Write status register1 failure.\n");
        return err;
    }

    /* Check status registers */
    sfud_read_reg(s_spinor_flash, 0x05, &sr1_read);
    if (sr1_read != sr1)
        printf("Setting the protected area failed! The register is in a write-protected state.");

    return 0;
}

/**
 * @brief Configure write protection (via SR1 and SR2)
 *
 * @param enable            true: enable write protection, false: disable
 * @param protection_level  Protection level (0: no protection, 1: hardware pin protection, 2: software lock)
 *
 * @return     0 on success; negative error code on failure
 */
static int spinor_write_protect(bool enable, uint8_t protection_level)
{
    sfud_err err;
    uint8_t sr1, sr2;
    uint8_t sr1_read, sr2_read;

    /* Read current status registers */
    err = sfud_read_reg(s_spinor_flash, 0x05, &sr1);
    if (err) {
        printf("Read status register1 failure.\n");
        return err;
    }
    err = sfud_read_reg(s_spinor_flash, 0x35, &sr2);
    if (err) {
        printf("Read status register2 failure.\n");
        return err;
    }

    if (enable) {
        // Enable write protection
        switch (protection_level) {
            case 1:               // Hardware pin protection (SRP0=1, SRP1=0)
                sr1 |= (1 << 7);  // Set SRP0=1
                sr2 &= ~(1 << 0); // Clear SRP1=0
                break;
            case 2:              // Software lock (SRP1=1)
                sr2 |= (1 << 0); // Set SRP1=1
                break;
            default:              // Default: disable protection
                sr1 &= ~(1 << 7); // Clear SRP0=0
                sr2 &= ~(1 << 0); // Clear SRP1=0
                break;
        }
    } else {
        // Disable write protection (unlock)
        sr1 &= ~(1 << 7); // Clear SRP0=0
        sr2 &= ~(1 << 0); // Clear SRP1=0
    }

    /* Write to status registers */
    err = sfud_write_status_ext(s_spinor_flash, false, 0x05, sr1);
    if (err) {
        printf("Write status register1 failure.\n");
        return err;
    }
    err = sfud_write_status_ext(s_spinor_flash, false, 0x35, sr2);
    if (err) {
        printf("Write status register2 failure.\n");
        return err;
    }

    /* Check status registers */
    sfud_read_reg(s_spinor_flash, 0x35, &sr2_read);
    sfud_read_reg(s_spinor_flash, 0x05, &sr1_read);
    if (sr1_read != sr1 || sr2_read != sr2)
        printf("Setting the protection mode failed! The register is in a write-protected state.");

    return 0;
}

static int cmd_spinor_example(int argc, char *argv[])
{
    int err = 0;

    if (spinor_init(SPI_BUS_ID, TEST_READ_WRITE_SIZE)) {
        pr_err("SPINOR init failed!\n");
        err = -1;
        goto exit;
    }

    printf("Enable software protected.Protected area:000000h~03FFFFh(64KB)\n");
    spinor_set_protect_area(false, 1);
    spinor_write_protect(true, 2);

    printf("Read write test, it should be failed, area is protected!\n");
    if (spinor_write_read(TEST_OFFSET, TEST_READ_WRITE_SIZE)) {
        err = -1;
        goto exit;
    }

    /*
     * Attention:After enabling software write protection,
     * it can only be disabled by power rebooting the device.
    */
    printf("Disable write protected.\n");
    if(spinor_write_protect(false, 0)) {
        err = -1;
        goto exit;
    }
    if(spinor_set_protect_area(false, 0)) {
        err = -1;
        goto exit;
    }

    /*
    * Enable QE mode, which is more recommended to be configured in menuconfig.

    ```
    quad_enable_func qe = s_spinor_flash->quad_enable;
    qe(s_spinor_flash);
    ```
    */

    /* Update the specified length of data at a specified address in NOR Flash. */
    u8 data[10] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
    if (spinor_refresh(4094, data, 10)) {
        err = -1;
        goto exit;
    }

exit:
    printf("spinor usage end.\n");

    /* Free resources */
    if (s_write_buf)
        aicos_free_align(0, s_write_buf);
    if (s_read_buf)
        aicos_free_align(0, s_read_buf);
    return err;
}

CONSOLE_CMD(spinor_usage, cmd_spinor_example, "spinor api example");
