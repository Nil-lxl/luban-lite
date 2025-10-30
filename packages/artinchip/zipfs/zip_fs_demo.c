/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <aic_core.h>
#include <rtthread.h>
#include <dfs_posix.h>
#include <aic_common.h>
#include <aic_utils.h>
#include "zip_fs.h"

/**
 * Note: An empty directory must be prepared as the mount point. It is recommended
 * to add {ROMFS_DIRENT_DIR, "zipfs", RT_NULL, 0} in _mountpoint_root within
 * ./target/xxx/demoxx-xxx/board.c.
 */
#define MOUNT_PATH  "/zipfs"
#define SOURCE_FILE "/rodata/zipfs_test/big_file.zip"
#define SOURCE_BLK  "blk_rodata_r"

static const char *g_file_name = MOUNT_PATH "/large_file.txt";

/**
 * Reads and displays the entire contents of a file
 *
 * @param fd File descriptor to read from
 * @return 0 on success, -1 on failure
 */
static int read_file(int fd)
{
    off_t size = -1;
    ssize_t bytes_read = -1;
    char *buffer = NULL;

    /* Get file size by seeking from start to end */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        pr_err("Seek to start failed\n");
        return -1;
    }
    size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        pr_err("Seek to end failed\n");
        return -1;
    }

    /* Return to beginning of file */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        pr_err("Seek to start failed\n");
        return -1;
    }

    /* Allocate buffer for file contents */
    buffer = (char *)aicos_malloc(MEM_CMA, size + 1);
    if (!buffer) {
        pr_err("Memory allocation failed\n");
        return -1;
    }
    uint64_t start_us = aic_get_time_us();
    /* Read entire file */
    bytes_read = read(fd, buffer, size);
    show_speed("zip file read speed", bytes_read, aic_get_time_us() - start_us);

    if (bytes_read == -1) {
        aicos_free(MEM_CMA, buffer);
        pr_err("File read failed\n");
        return -1;
    }
    buffer[bytes_read] = '\0';

    /* Display contents */
    rt_kprintf("Read %lu bytes:\n", (unsigned long)bytes_read);
    for (int i = 0; i < bytes_read; i++)
        putchar(buffer[i]);
    putchar('\n');

    aicos_free(MEM_CMA, buffer);

    return 0;
}

/**
 * zip File Operations Demonstration
 *
 * Demonstrates:
 * - Opening files using open()
 * - Closing files using close()
 * - Reading files (wrapped function)
 */
static int zip_fs_demo(int argc, char **argv)
{
    int fd = -1, err = 0;

    rt_kprintf("zip file system usage example start\n");

    /* Mount zip file system */
    err = zipfs_mount(SOURCE_FILE, MOUNT_PATH);
    /* Or you can mount zip file system by zip blk */
    // err = zipfs_mount(SOURCE_BLK, MOUNT_PATH);
    if (err) {
        pr_err("zipfs mount failed\n");
        return err;
    }

    /* Open file */
    rt_kprintf("Open file (open) :%s\n", g_file_name);
    fd = open(g_file_name, O_RDONLY, 0644);
    if (fd == -1) {
        pr_err("File open failed\n");
        err = -1;
        goto exit;
    }

    /* Read file content */
    rt_kprintf("Read File Content\n");
    err = read_file(fd);
    if (err)
        pr_err("Read file failed\n");

exit:
    /* Close file using close() */
    rt_kprintf("Close File (close)\n");
    if (close(fd) == -1)
        pr_err("Final close failed");

    /* Unmount zip file system */
    zipfs_unmount(MOUNT_PATH);
    rt_kprintf("zip file system example end\n");
    return err;
}

MSH_CMD_EXPORT(zip_fs_demo, Zip file system example);
