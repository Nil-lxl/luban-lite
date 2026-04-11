/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include "zip_reader.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include <aic_utils.h>
#include <aic_core.h>
#include <string.h>

#define ZIP_FILE_HEADER    0x04034B50
#define ZIP_FILE_EOCD_SIGN 0x06054B50

#pragma pack(push, 1)
struct zip_eocd {
    rt_uint32_t signature;
    rt_uint16_t disk_number;
    rt_uint16_t cd_disk;
    rt_uint16_t cd_records;
    rt_uint16_t total_cd_records;
    rt_uint32_t cd_size;
    rt_uint32_t cd_offset;
    rt_uint16_t comment_len;
};
#pragma pack(pop)

static size_t get_zip_size_from_blk(const char *blk_name)
{
    rt_device_t flash_dev;
    struct rt_device_blk_geometry flash_geo;
    struct zip_eocd *eocd = NULL;
    size_t len = 0, i;
    uint32_t blk_count = 0;
    uint8_t *blk_data = NULL;

    flash_dev = rt_device_find(blk_name);
    if (!flash_dev)
        return 0;

    if (rt_device_open(flash_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        return 0;

    rt_device_control(flash_dev, RT_DEVICE_CTRL_BLK_GETGEOME, &flash_geo);

    blk_data = (uint8_t *)malloc(flash_geo.block_size);

    rt_device_read(flash_dev, 0, blk_data, 1);
    hexdump(blk_data, flash_geo.block_size, 16);

    if (*(uint32_t *)blk_data != ZIP_FILE_HEADER) {
        rt_kprintf("head:%d\n", *(uint32_t *)blk_data);
        goto exit;
    }

    for (i = 4; i < flash_geo.block_size - sizeof(struct zip_eocd); i++) {
        eocd = (struct zip_eocd *)(blk_data + i);
        if (eocd->signature == ZIP_FILE_EOCD_SIGN) {
            len = i + sizeof(struct zip_eocd) + eocd->comment_len;
            goto exit;
        }
    }

    blk_count = flash_geo.bytes_per_sector * flash_geo.sector_count / flash_geo.block_size;

    for (i = 1; i < blk_count; i++) {
        rt_device_read(flash_dev, i, blk_data, 1);
        for (int j = 0; j < flash_geo.block_size - sizeof(struct zip_eocd); j++) {
            eocd = (struct zip_eocd *)(blk_data + j);
            if (eocd->signature == ZIP_FILE_EOCD_SIGN) {
                len = j + sizeof(struct zip_eocd) + eocd->comment_len + i * flash_geo.block_size;
                goto exit;
            }
        }
    }

exit:
    rt_device_close(flash_dev);
    if (blk_data)
        free(blk_data);
    return len;
}

static int get_zip_data_from_blk(const char *blk_name, void *buffer, size_t len)
{
    rt_device_t flash_dev;
    struct rt_device_blk_geometry flash_geo;
    size_t read_size = 0;
    uint8_t *blk_data = NULL;

    flash_dev = rt_device_find(blk_name);
    if (!flash_dev)
        return -1;

    if (rt_device_open(flash_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        return -1;

    rt_device_control(flash_dev, RT_DEVICE_CTRL_BLK_GETGEOME, &flash_geo);

    read_size = RT_ALIGN(len, flash_geo.block_size);
    blk_data = (uint8_t *)malloc(read_size);

    rt_device_read(flash_dev, 0, blk_data, read_size / flash_geo.block_size);

    memcpy(buffer, blk_data, len);

    rt_device_close(flash_dev);
    if (blk_data)
        free(blk_data);
    return 0;
}

struct aic_zip_reader *aic_zip_reader_open(const char *source)
{
    int fd = -1;
    struct aic_zip_reader *reader = NULL;
    void *buffer = NULL;
    size_t zip_size = 0;
    struct stat st;
    bool is_file = false;

    fd = open(source, O_RDONLY);
    if (fd == -1) {
        zip_size = get_zip_size_from_blk(source);
        if (zip_size == 0) {
            pr_err("zip file data get failed\n");
            return NULL;
        }
    } else {
        is_file = true;
        if (fstat(fd, &st) == -1) {
            close(fd);
            pr_err("Stat file failed\n");
            return NULL;
        }
        zip_size = st.st_size;
    }

    buffer = (char *)malloc(zip_size);
    if (!buffer) {
        if (is_file)
            close(fd);
        pr_err("Memory allocation failed\n");
        return NULL;
    }

    if (is_file) {
        if (read(fd, buffer, zip_size) != zip_size) {
            free(buffer);
            close(fd);
            pr_err("File read failed\n");
            return NULL;
        }

        if (close(fd) == -1) {
            free(buffer);
            pr_err("File close failed\n");
            return NULL;
        }
    } else {
        if (get_zip_data_from_blk(source, buffer, zip_size) == -1) {
            free(buffer);
            return NULL;
        }
    }

    // Create reader instance
    reader = (struct aic_zip_reader *)malloc(sizeof(struct aic_zip_reader));
    if (!reader) {
        free(buffer);
        return NULL;
    }
    memset(reader, 0, sizeof(struct aic_zip_reader));

    // Initialize miniz ZIP archive
    memset(&reader->archive, 0, sizeof(mz_zip_archive));
    if (!mz_zip_reader_init_mem(&reader->archive, buffer, zip_size, 0)) {
        pr_err("Failed to init ZIP archive: %s\n",
               mz_zip_get_error_string(mz_zip_get_last_error(&reader->archive)));
        aic_zip_reader_close(reader);
        return NULL;
    }

    reader->zip_buffer = buffer;
    reader->zip_size = zip_size;
    reader->file_count = mz_zip_reader_get_num_files(&reader->archive);
    reader->file_info = malloc(sizeof(struct aic_file_info) * reader->file_count);
    memset(reader->file_info, 0, sizeof(struct aic_file_info) * reader->file_count);

    for (int i = 0; i < reader->file_count; i++) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&reader->archive, i, &stat)) {
            aic_zip_reader_close(reader);
            return NULL;
        }

        reader->file_info[i].name = malloc(strlen(stat.m_filename) + 2);
        if (!reader->file_info[i].name) {
            aic_zip_reader_close(reader);
            return NULL;
        }
        reader->file_info[i].name[0] = '/';
        memcpy(reader->file_info[i].name + 1, stat.m_filename, strlen(stat.m_filename) + 1);
        reader->file_info[i].size = stat.m_uncomp_size;
    }

    return reader;
}

void aic_zip_reader_close(struct aic_zip_reader *reader)
{
    if (reader) {
        if (reader->file_info) {
            for (int i = 0; i < reader->file_count; i++)
                if (reader->file_info[i].name)
                    free(reader->file_info[i].name);
            free(reader->file_info);
        }

        if (reader->zip_buffer)
            free(reader->zip_buffer);

        mz_zip_reader_end(&reader->archive);

        free(reader);
    }
}

int aic_zip_reader_read(struct aic_zip_reader *reader, struct aic_file_info file_info, void *buffer)
{
    if (!reader || !buffer) {
        return -1;
    }
    const char *file_name = file_info.name + 1;
    size_t buffer_size = file_info.size;
    size_t extacted_size = 0;
    mz_zip_reader_extract_iter_state *iter =
        mz_zip_reader_extract_file_iter_new(&reader->archive, file_name, 0);

    while (extacted_size < buffer_size) {
        size_t byte_read =
            mz_zip_reader_extract_iter_read(iter, buffer + extacted_size, READ_BUF_SIZE);
        if (byte_read == 0)
            break;
        extacted_size += byte_read;
    }

    mz_zip_reader_extract_iter_free(iter);

    return 0;
}
