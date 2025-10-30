/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_utils.h>
#include "zip_reader.h"

#define SOURCE_FILE "/rodata/zipfs_test/much_file.zip"
#define SOURCE_BLK  "blk_rodata_r"

/**
 * Demonstrates how to process a ZIP archive by extracting and
 * printing contents of each entry.
 */
static int zip_reader_demo()
{
    /* Open zip reader by zip file */
    struct aic_zip_reader *reader = aic_zip_reader_open(SOURCE_FILE);

    /* Or you can open zip reader by zip blk */
    // struct aic_zip_reader *reader = aic_zip_reader_open(SOURCE_BLK);
    if (!reader) {
        pr_err("Failed to open zip reader by file: %s\n", SOURCE_FILE);
        return -1;
    }

    /* Process each file */
    for (int i = 0; i < reader->file_count; i++) {
        size_t file_size = reader->file_info[i].size;
        char *file_name = reader->file_info[i].name;

        rt_kprintf("Processing %d: %s, size:%zu\n", i + 1, file_name, file_size);

        if (file_name[strlen(file_name) - 1] == '/') {
            rt_kprintf("  Skipping directory\n");
            continue;
        }

        void *content = malloc(file_size);
        if (!content) {
            perror("Memory allocation failed");
            aic_zip_reader_close(reader);
            return -1;
        }
        uint64_t start_us = aic_get_time_us();
        /* Read the file specified by index file_info. */
        aic_zip_reader_read(reader, reader->file_info[i], content);
        show_speed("zip file read speed", file_size, aic_get_time_us() - start_us);
        for (size_t j = 0; j < file_size; j++) {
            putchar(((char *)content)[j]);
        }
        rt_kprintf("\n");
        free(content);
    }

    /* Close zip reader */
    aic_zip_reader_close(reader);
    return 0;
}

MSH_CMD_EXPORT(zip_reader_demo, zip reader demo);
