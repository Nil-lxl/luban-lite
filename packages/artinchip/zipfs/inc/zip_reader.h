/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#ifndef __ZIP_READER_H_
#define __ZIP_READER_H_

#include <stddef.h>
#include "miniz.h"
#include "miniz_zip.h"

#define READ_BUF_SIZE 4096

struct aic_file_info {
    char *name;
    size_t size;
};

struct aic_zip_reader {
    void *zip_buffer;       /* Memory buffer containing ZIP file */
    size_t zip_size;        /* Size of ZIP file in bytes */
    mz_zip_archive archive; /* miniz ZIP archive structure */
    uint32_t file_count;    /* Number of files (including directories) in the ZIP archive */
    struct aic_file_info *file_info; /* Array of structures containing file information */
};

/**
 * @brief Open a ZIP file from RAMDisk
 *
 * @param source Full path to ZIP file in RAMDisk, or block name where ZIP data in
 *               example: "/rodata/zipfs_test/much_file.zip", "blk_rodata_r"
 *
 * @return aic_zip_reader* Pointer to ZIP reader instance, NULL on failure
 */
struct aic_zip_reader *aic_zip_reader_open(const char *source);

/**
 * @brief Close ZIP reader and release all resources
 *
 * @param reader ZIP reader pointer
 */
void aic_zip_reader_close(struct aic_zip_reader *reader);

/**
 * @brief Extract file content directly into provided buffer
 *
 * @param reader ZIP reader pointer
 * @param file_info Struct representing the information of files to be read.
 * @param buffer Destination buffer, Set the buffer size based on the size field of aic_file_info.
 * @return int Number 0 on success, -1 on failure
 */
int aic_zip_reader_read(struct aic_zip_reader *reader, struct aic_file_info file_info,
                        void *buffer);

#endif // __ZIP_READER_H_
