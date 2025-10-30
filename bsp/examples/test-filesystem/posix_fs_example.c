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

#define DEMO_PATH "/data"
static const char *g_file_name = DEMO_PATH "/posix_fs_demo.txt";

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
        pr_err("Seek to start failed");
        return -1;
    }
    size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        pr_err("Seek to end failed");
        return -1;
    }

    /* Return to beginning of file */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        pr_err("Seek to start failed");
        return -1;
    }

    /* Allocate buffer for file contents */
    buffer = (char *)aicos_malloc(MEM_CMA, size + 1);
    if (!buffer) {
        pr_err("Memory allocation failed");
        return -1;
    }

    /* Read entire file */
    bytes_read = read(fd, buffer, size);
    if (bytes_read == -1) {
        aicos_free(MEM_CMA, buffer);
        pr_err("File read failed");
        return -1;
    }
    buffer[bytes_read] = '\0';

    /* Display contents */
    rt_kprintf("Read %lu bytes: \"%s\"\n", (unsigned long)bytes_read, buffer);
    aicos_free(MEM_CMA, buffer);

    return 0;
}

/**
 * Appends content to the end of a file
 *
 * @param fd File descriptor to append to
 * @param content String to append
 * @return 0 on success, -1 on failure
 */
static int append_to_file(int fd, const char *content)
{
    ssize_t bytes_written = -1;

    /* Seek to end of file */
    if (lseek(fd, 0, SEEK_END) == (off_t)-1) {
        pr_err("Seek to end failed");
        return -1;
    }

    /* Write content at end position */
    bytes_written = write(fd, content, strlen(content));
    if (bytes_written == -1) {
        pr_err("Write failed");
        return -1;
    }

    rt_kprintf("Appended %lu bytes: \"%s\"\n", (unsigned long)bytes_written, content);
    return 0;
}

/**
 * Truncates a file to the specified length
 *
 * @param fd File descriptor to truncate
 * @param length Desired file length
 * @return 0 on success, -1 on failure
 */
static int truncate_file(int fd, off_t length)
{
    off_t current_size = -1;

    /* Execute truncation */
    if (ftruncate(fd, length) == -1) {
        pr_err("File truncation failed");
        return -1;
    }

    /* Verify new file size */
    current_size = lseek(fd, 0, SEEK_END);
    if (current_size == (off_t)-1) {
        pr_err("Size verification failed");
        return -1;
    }

    /* Return to beginning of file */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        pr_err("Seek to start failed");
        return -1;
    }

    rt_kprintf("Truncated file to %ld bytes\n", (long)current_size);
    return 0;
}

/**
 * POSIX File Operations Demonstration
 *
 * Demonstrates:
 * - Opening files using open()
 * - Closing files using close()
 * - Writing content using write()
 * - Reading files (wrapped function)
 * - Appending content (wrapped function)
 * - Truncating files (wrapped function)
 * - Sync file
 */
static int cmd_posix_fs_demo(int argc, char **argv)
{
    int fd = -1;
    const char *init_text = "This is an initial text. ";
    const char *append_text = "This is an append text";
    ssize_t bytes = -1;
    int ret = 0;

    rt_kprintf("Posix file system usage example start\n");

    /* Create or truncate a writable and readable file with permissions rw-r--r--. */
    rt_kprintf("Creating file (open) :%s\n", g_file_name);
    fd = open(g_file_name, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd == -1) {
        pr_err("File creation failed");
        return -1;
    }

    /* Write initial content using write() */
    rt_kprintf("Writing initial text (write)\n");
    bytes = write(fd, init_text, strlen(init_text));
    if (bytes == -1) {
        pr_err("Initial write failed");
        ret = -1;
        goto close_file;
    }
    rt_kprintf("Write %lu bytes: \"%s\"\n", (unsigned long)bytes, init_text);

    /* Append content using wrapped function */
    rt_kprintf("Appending text\n");
    ret = append_to_file(fd, append_text);
    if (ret) {
        pr_err("Append failed");
        goto close_file;
    }

    /* Verify file content */
    rt_kprintf("Verifying File Content\n");
    ret = read_file(fd);
    if (ret) {
        pr_err("Read verification failed");
        goto close_file;
    }

    /* Truncate file using wrapped function */
    rt_kprintf("Truncating File\n");
    ret = truncate_file(fd, 15);
    if (ret) {
        pr_err("Truncation failed");
        goto close_file;
    }

    /* Verify truncated content */
    rt_kprintf("Verifying Truncated Content\n");
    ret = read_file(fd);
    if (ret) {
        pr_err("Post-truncate read failed");
        goto close_file;
    }

    /* Make all changes done to FD actually appear on disk. */
    rt_kprintf("Sync File (fsync)\n");
    ret = fsync(fd);
    if (ret)
        pr_err("Sync File failed");

close_file:
    /* Close file using close() */
    rt_kprintf("Close File (close)\n");
    ret = close(fd);
    if (ret)
        pr_err("Final close failed");

    rt_kprintf("Posix file system example end\n");
    return ret;
}

MSH_CMD_EXPORT_ALIAS(cmd_posix_fs_demo, posix_fs_demo, Posix file system example);
