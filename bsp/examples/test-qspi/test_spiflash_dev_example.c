/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <finsh.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtdef.h>

#define PARTITION_NAME "blk_data" /* Target block device name */
#define TEST_READ_WRITE_SIZE \
    0x2000 /* Data length must be block-aligned since we're operating on block devices */
#define TEST_START_BLK_ID 0 /* Starting block number */

static rt_device_t s_flash_t = NULL;
static struct rt_device_blk_geometry s_flash_geometry;
static rt_uint8_t *s_read_buf = NULL;
static rt_uint8_t *s_write_buf = NULL;

static void address_map(rt_uint8_t *data, rt_uint32_t start_address, rt_uint32_t len)
{
    rt_uint32_t i;

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

static rt_err_t verify_data(rt_uint8_t *data_exp, rt_uint8_t *data_act, rt_uint32_t offset,
                            rt_uint32_t len)
{
    if (memcmp(data_exp, data_act, len) != 0) {
        printf("expected:-----------------------\n");
        address_map(data_exp, offset, len);
        printf("actual:-------------------------\n");
        address_map(data_act, offset, len);
        return -RT_ERROR;
    } else {
        if (len > 0x200) {
            address_map(data_act, offset, 0x100);
            printf("......\n");
            address_map(data_act + len - 0x50, offset + len - 0x50, 0x50);
        } else {
            address_map(data_act, offset, len);
        }
        return RT_EOK;
    }
}

/**
 * @brief Initialize the flash storage device with specified partition name.
 *        Searches for the device by name, opens it if not already opened,
 *        and retrieves its geometric parameters (block size, capacity etc.).
 *
 * @param part_name Partition name to locate the target flash device
 * @return Error code from RT-Thread subsystem operations
 */
static rt_err_t flash_device_init(const char *part_name)
{
    rt_err_t res = RT_ERROR;

    s_flash_t = rt_device_find(part_name);
    if (!s_flash_t) {
        rt_kprintf("find %s failed!\n", part_name);
        return -res;
    }

    if (!s_flash_t->ref_count) {
        res = rt_device_open(s_flash_t, RT_DEVICE_OFLAG_RDWR);
        if (res != RT_EOK) {
            rt_kprintf("open flash device failed!\n");
            return -res;
        }
    }

    return -rt_device_control(s_flash_t, RT_DEVICE_CTRL_BLK_GETGEOME, &s_flash_geometry);
}

/**
 * @brief Read data from flash memory starting at specified block ID.
 *        Requires buffer length to be aligned with flash block size.
 *
 * @param block_start_id Starting physical block index for read operation
 * @param buffer Data storage pointer for received bytes
 * @param buffer_len Total byte count to read (must be aligned with block size)
 * @return Error code from RT-Thread subsystem operations
 */
rt_err_t flash_device_read(rt_uint32_t block_start_id, rt_uint8_t *buffer, rt_uint32_t buffer_len)
{
    rt_uint32_t ret = 0;

    if (buffer_len % s_flash_geometry.block_size != 0) {
        rt_kprintf("Data length must be aligned to block size (0x%x)\n",
                   s_flash_geometry.block_size);
        return -RT_ERROR;
    }

    /* On success, it returns the number of blocks operated; on failure, it returns 0 */
    ret =
        rt_device_read(s_flash_t, block_start_id, buffer, buffer_len / s_flash_geometry.block_size);

    if (ret == 0)
        return -RT_ERROR;
    return RT_EOK;
}

/**
 * @brief Write data to flash memory starting at specified block ID.
 *        Requires buffer length to be aligned with flash block size.
 *
 * @param block_start_id Starting physical block index for write operation
 * @param buffer Source data pointer containing bytes to program
 * @param buffer_len Total byte count to write (must be aligned with block size)
 * @return Error code from RT-Thread subsystem operations
 */
rt_err_t flash_device_write(rt_uint32_t block_start_id, rt_uint8_t *buffer, rt_uint32_t buffer_len)
{
    rt_uint32_t ret = 0;

    if (buffer_len % s_flash_geometry.block_size != 0) {
        rt_kprintf("Data length must be aligned to block size (0x%x)\n",
                   s_flash_geometry.block_size);
        return -RT_ERROR;
    }

    /* On success, it returns the number of blocks operated; on failure, it returns 0 */
    ret = rt_device_write(s_flash_t, block_start_id, buffer,
                          buffer_len / s_flash_geometry.block_size);
    if (ret == 0)
        return -RT_ERROR;
    return RT_EOK;
}

static int cmd_spiflash_usage_example(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;

    /* Allocate test data buffer */
    s_write_buf = rt_malloc_align(TEST_READ_WRITE_SIZE, CACHE_LINE_SIZE);
    if (!s_write_buf) {
        printf("Error: No memory for test data!\n");
        ret = RT_ERROR;
        goto exit;
    }

    /* Allocate read buffer */
    s_read_buf = rt_malloc_align(TEST_READ_WRITE_SIZE, CACHE_LINE_SIZE);
    if (!s_read_buf) {
        printf("Error: No memory for read buffer!\n");
        rt_free_align(s_write_buf);
        ret = RT_ERROR;
        goto exit;
    }

    /* prepare some data */
    for (int i = 0; i < TEST_READ_WRITE_SIZE; i++) {
        s_write_buf[i] = i % 256;
    }

    ret = flash_device_init(PARTITION_NAME);
    if (ret != RT_EOK) {
        rt_kprintf("flash device init failed!\n");
        goto exit;
    } else {
        rt_kprintf("flash device init successful!\n");
    }

    /* 	Can be written to directly without first erasing. */
    ret = flash_device_write(TEST_START_BLK_ID, s_write_buf, TEST_READ_WRITE_SIZE);
    if (ret != RT_EOK) {
        rt_kprintf("data write failed!\n");
        goto exit;
    } else {
        rt_kprintf("data write successful!\n");
    }

    ret = flash_device_read(TEST_START_BLK_ID, s_read_buf, TEST_READ_WRITE_SIZE);
    if (ret != RT_EOK) {
        rt_kprintf("data read failed!\n");
        goto exit;
    } else {
        rt_kprintf("data read successful!\n");
    }

    ret = verify_data(s_write_buf, s_read_buf, TEST_START_BLK_ID, TEST_READ_WRITE_SIZE);
    if (ret != RT_EOK)
        goto exit;

exit:
    printf("flash device usage end.\n");

    if (s_flash_t)
        rt_device_close(s_flash_t);
    if (s_write_buf)
        rt_free_align(s_write_buf);
    if (s_read_buf)
        rt_free_align(s_read_buf);
    return -ret;
}

MSH_CMD_EXPORT_ALIAS(cmd_spiflash_usage_example, flash_device_usage, Flash block device example);
