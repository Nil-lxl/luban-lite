/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <fal.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <finsh.h>
#include <aic_core.h>
#include <aic_crc32.h>

#define DEMO_PARTITION_NAME "data"         /* Flash partition name */
#define PARTITION_SIZE      0x2000         /* Size per AB data backup */
#define A_OFFSET            0x0            /* data backup A offset */
#define B_OFFSET            PARTITION_SIZE /* data backup B offset */
#define DATA_LEN_SIZE       sizeof(uint32_t)
#define VERSION_SIZE        sizeof(uint32_t)
#define CRC_SIZE            sizeof(uint32_t)
#define DATA_SIZE \
    0x20 /* It should be smaller than (PARTITION_SIZE-VERSION_SIZE-CRC_SIZE-DATA_LEN_SIZE) */
#define CRC_KEY 0x123

/* Data structure for each data backup */
typedef struct {
    uint32_t data_len;       /* data_len */
    uint32_t version;        /* Version number */
    uint32_t crc;            /* CRC32 checksum */
    uint8_t data[DATA_SIZE]; /* Actual data */
} __attribute__((aligned(4))) partition_data_t;

static const struct fal_partition *s_part_dev = NULL;
static partition_data_t *s_part_data = NULL;
static u8 *s_write_buf = NULL;
static u8 *s_read_buf = NULL;

/**
 * @brief Checks if the given  backup data is valid.
 * @param part Pointer to a constant partition_data_t structure to be validated.
 * @return int Returns 1 if the backup data is valid, 0 otherwise.
 */
static int is_valid(const partition_data_t *part)
{
    return (part->version != 0xFFFFFFFF) &&
           (crc32(CRC_KEY, part->data, part->data_len) == part->crc);
}

static void data_log(char *tag)
{
    u32 i;

    rt_kprintf("[%s]:data len=%u, version=%u, crc=%u, data:\n", tag, s_part_data->data_len,
               s_part_data->version, s_part_data->crc);
    for (i = 0; i < s_part_data->data_len; i++) {
        if (i && (i % 16) == 0)
            rt_kprintf("\n");
        rt_kprintf("%02x ", s_part_data->data[i]);
    }
    rt_kprintf("\n\n");
}

/**
 * @brief Retrieves the latest valid data backup and its version.
 *
 * This function reads and checks the validity of two data backup (A and B). It then determines
 * which data backup is the latest valid one and updates the provided pointers with the active data backup
 * index and the corresponding version number.
 *
 * @param latest_idx Pointer to where the index of the latest data backup (0 for A, 1 for B) will be stored.
 * @param version Pointer to where the version number of the latest valid data backup will be stored.
 * @return int Returns 0 on success, -1 on failure.
 */
static int get_latest_data(int *latest_idx, u32 *version)
{
    uint32_t a_ver = 0, b_ver = 0;
    int a_valid = 0, b_valid = 0, ret;

    /* Check data backup A */
    ret = fal_partition_read(s_part_dev, A_OFFSET, (u8 *)s_part_data, sizeof(partition_data_t));
    if (ret != sizeof(partition_data_t)) {
        rt_kprintf("Read data failed\n");
        return -1;
    }
    a_valid = is_valid(s_part_data);
    a_ver = a_valid ? s_part_data->version : 0;
    rt_kprintf("a_valid=%s, a_version=%d, ", a_valid == 0 ? "false" : "true", a_ver);

    /* Check data backup B */
    ret = fal_partition_read(s_part_dev, B_OFFSET, (u8 *)s_part_data, sizeof(partition_data_t));
    if (ret != sizeof(partition_data_t)) {
        rt_kprintf("Read data failed\n");
        return -1;
    }
    b_valid = is_valid(s_part_data);
    b_ver = b_valid ? s_part_data->version : 0;
    rt_kprintf("b_valid=%s, b_version=%d\n", b_valid == 0 ? "false" : "true", b_ver);

    /* Select the complete and latest data backup. */
    if (latest_idx != NULL) {
        if (a_valid && b_valid) {
            *latest_idx = (a_ver >= b_ver) ? 0 : 1;
        } else if (a_valid) {
            *latest_idx = 0;
        } else {
            /**
             * If both A and B data backup are damaged, by default,
             * the B data backup will be treated as the latest data backup.
             */
            *latest_idx = 1;
        }
        rt_kprintf("latest part:%c", *latest_idx == 0 ? 'A' : 'B');
    }

    if (version != NULL) {
        if (a_valid && b_valid) {
            *version = (a_ver >= b_ver) ? a_ver : b_ver;
        } else if (a_valid) {
            *version = a_ver;
        } else if (b_valid) {
            *version = b_ver;
        } else {
            *version = 1;
        }
        rt_kprintf(" version:%u", *version);
    }
    rt_kprintf("\n");
    return 0;
}

/**
 * @brief Reads data from the latest data backup into a buffer.
 *
 * This function reads data from the latest available data backup (either A or B)
 * into the provided buffer. It ensures that the read size does not exceed
 * the predefined DATA_SIZE. If the latest data backup is invalid or the read size
 * is too large, it returns an error.
 *
 * @param buf Pointer to the buffer where data will be read into.
 * @param size The size of the data to be read.
 * @return int Returns 0 on success, -1 on failure.
 */
static int read_data(uint8_t *buf, uint32_t size)
{
    int latest_part, ret;

    if (size > DATA_SIZE)
        return -1;

    ret = get_latest_data(&latest_part, NULL);
    if (ret != 0)
        return -1;

    uint32_t offset = (latest_part == 0) ? A_OFFSET : B_OFFSET;
    ret = fal_partition_read(s_part_dev, offset, (u8 *)s_part_data, sizeof(partition_data_t));
    if (ret != sizeof(partition_data_t)) {
        rt_kprintf("Read data failed\n");
        return -1;
    }

    if (is_valid(s_part_data)) {
        rt_kprintf("Data is valid\n");
    } else {
        rt_kprintf("ERR: Data is not valid\n");
        return -1;
    }

    rt_kprintf("read data from part %c success\n", offset == A_OFFSET ? 'A' : 'B');
    data_log("read data");
    memcpy(buf, s_part_data->data, size);
    return 0;
}

/**
 * @brief Writes data to the appropriate data backup with CRC verification.
 *
 * This function writes data to the latest available data backup (either A or B)
 * after preparing it with a version number and CRC checksum. It ensures that
 * the write size does not exceed the predefined DATA_SIZE. If the write operation
 * fails, it returns an error.
 *
 * @param data Pointer to the data to be written.
 * @param size The size of the data to be written.
 * @return int Returns 0 on success, -1 on failure.
 */
static int write_data(const uint8_t *data, uint32_t size)
{
    int latest_part, ret;
    u32 version, offset;
    partition_data_t verify;

    if (size > DATA_SIZE)
        return -1;

    /* Write to the old data backup.To calculate the offset */
    get_latest_data(&latest_part, &version);
    offset = (latest_part == 0) ? B_OFFSET : A_OFFSET;

    /* Prepare data with CRC */
    s_part_data->version = (latest_part >= 0) ? version + 1 : 1;
    memset(s_part_data->data, 0, DATA_SIZE);
    memcpy(s_part_data->data, data, size);
    s_part_data->crc = crc32(CRC_KEY, s_part_data->data, DATA_SIZE);
    s_part_data->data_len = DATA_SIZE;

    /* Erase and write */
    ret = fal_partition_erase(s_part_dev, offset, PARTITION_SIZE);
    if (ret != PARTITION_SIZE) {
        rt_kprintf("Erase data failed\n");
        return -1;
    }

    ret = fal_partition_write(s_part_dev, offset, (u8 *)s_part_data, sizeof(partition_data_t));
    if (ret != sizeof(partition_data_t)) {
        rt_kprintf("Write data failed\n");
        return -1;
    }

    /* Verify data */
    verify = *s_part_data;
    ret = fal_partition_read(s_part_dev, offset, (u8 *)s_part_data, sizeof(partition_data_t));
    if (ret != sizeof(partition_data_t)) {
        rt_kprintf("Read data failed\n");
        return -1;
    }
    if (memcmp(&verify, s_part_data, sizeof(partition_data_t)) == 0) {
        rt_kprintf("write data into part %c success\n", offset == A_OFFSET ? 'A' : 'B');
        data_log("write data");
    } else {
        rt_kprintf("write data into part %c failed\n", offset == A_OFFSET ? 'A' : 'B');
        return -1;
    }

    return 0;
}

static int erase_latest_part()
{
    int latest_part, ret;
    u32 offset;

    get_latest_data(&latest_part, NULL);
    offset = (latest_part == 0) ? A_OFFSET : B_OFFSET;

    /* Erase */
    ret = fal_partition_erase(s_part_dev, offset, PARTITION_SIZE);
    if (ret != PARTITION_SIZE) {
        rt_kprintf("Erase data failed\n");
        return -1;
    }
    rt_kprintf("erase data from part %c success\n\n", offset == A_OFFSET ? 'A' : 'B');

    return 0;
}

/**
 * @brief Initializes the storage data backup and prepares initial data.
 *
 * This function initializes the storage by finding the specified data backup,
 * allocating necessary memory buffers, and preparing initial data for both
 * data backup (A and B). It sets up version numbers and CRC checksums for
 * verification purposes. If any step fails, it returns an error.
 *
 * @return int Returns 0 on success, -1 on failure.
 */
static int storage_init(void)
{
    s_part_dev = fal_partition_find(DEMO_PARTITION_NAME);
    if (!s_part_dev)
        return -1;

    rt_kprintf("Check the initial AB data backup.\n");
    fal_partition_read(s_part_dev, A_OFFSET, (u8 *)s_part_data, sizeof(partition_data_t));
    if (!is_valid(s_part_data)) {
        memset(s_part_data->data, 'A', DATA_SIZE);
        s_part_data->crc = crc32(CRC_KEY, s_part_data->data, DATA_SIZE);
        s_part_data->version = 1;
        s_part_data->data_len = DATA_SIZE;
        fal_partition_erase(s_part_dev, A_OFFSET, PARTITION_SIZE);
        fal_partition_write(s_part_dev, A_OFFSET, (u8 *)s_part_data, sizeof(partition_data_t));
        data_log("A part data error, init");
    } else {
        data_log("found A part data");
    }

    fal_partition_read(s_part_dev, B_OFFSET, (u8 *)s_part_data, sizeof(partition_data_t));
    if (!is_valid(s_part_data)) {
        memset(s_part_data->data, 'B', DATA_SIZE);
        s_part_data->crc = crc32(CRC_KEY, s_part_data->data, DATA_SIZE);
        s_part_data->version = 2;
        s_part_data->data_len = DATA_SIZE;
        fal_partition_erase(s_part_dev, B_OFFSET, PARTITION_SIZE);
        fal_partition_write(s_part_dev, B_OFFSET, (u8 *)s_part_data, sizeof(partition_data_t));
        data_log("B part data error, init");
    } else {
        data_log("found B part data");
    }

    /* prepare some write data */
    for (int i = 0; i < DATA_SIZE; i++) {
        s_write_buf[i] = i % 256;
    }

    return 0;
}

/*
 * This test simulates power loss causing the latest backup to be corrupted,
 * verifying if the system can correctly fall back to the alternate backup and recover data.
 */
static void fal_pwr_loss(void)
{
    int ret;

    s_part_data = (partition_data_t *)rt_malloc(sizeof(partition_data_t));
    if (!s_part_data) {
        rt_kprintf("Error: No memory for part data!\n");
        goto free;
    }

    s_write_buf = (u8 *)rt_malloc(DATA_SIZE);
    if (!s_write_buf) {
        rt_kprintf("Error: No memory for write buffer!\n");
        goto free;
    }

    s_read_buf = (u8 *)rt_malloc(DATA_SIZE);
    if (!s_read_buf) {
        rt_kprintf("Error: No memory for read buffer!\n");
        goto free;
    }

    ret = storage_init();
    if (ret) {
        rt_kprintf("storage init failed!\n");
        goto free;
    }

    /**
     * Read the data. Since the data version in data backup B is higher,
     * the data from data backup B will be read.
     */
    rt_kprintf("Read the latest version of the partition data. The expected is B.\n");
    ret = read_data(s_read_buf, DATA_SIZE);
    if (ret) {
        rt_kprintf("read data failed!\n");
        goto free;
    }

    rt_kprintf("To simulate backup data corruption or power off, erase the latest data backup.\n");
    ret = erase_latest_part();
    if (ret) {
        rt_kprintf("erase data failed!\n");
        goto free;
    }

    /**
     * Read the data backup. Since the latest data backup (B data backup) is damaged (erased),
     * it will read from data backup A.
     */
    rt_kprintf("Read the backup data. The expected is A.\n");
    ret = read_data(s_read_buf, DATA_SIZE);
    if (ret) {
        rt_kprintf("read data failed!\n");
        goto free;
    }

    /* Write to the data backup, prioritizing the damaged data backup (B data backup). */
    rt_kprintf("Write data. The expected is B.\n");
    ret = write_data(s_write_buf, DATA_SIZE);
    if (ret) {
        rt_kprintf("write data failed!\n");
        goto free;
    }

free:
    if (s_part_data != NULL)
        rt_free(s_part_data);
    if (s_write_buf != NULL)
        rt_free(s_write_buf);
    if (s_read_buf != NULL)
        rt_free(s_read_buf);
    return;
}

MSH_CMD_EXPORT(fal_pwr_loss, FAL power loss protection demo);
