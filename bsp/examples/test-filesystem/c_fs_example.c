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
#include <unistd.h>

#define DEMO_PATH "/data"
static const char *g_file_name = DEMO_PATH "/c_fs_demo.txt";

/**
 * Reads and displays the entire contents of a file
 *
 * @param fp FILE pointer to read from
 * @return 0 on success, -1 on failure
 */
static int read_file(FILE *fp)
{
    long size = -1;
    size_t bytes_read = 0;
    char *buffer = NULL;

    /* Go to end to get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        pr_err("Seek to end failed");
        return -1;
    }

    size = ftell(fp);
    if (size < 0) {
        pr_err("Get file size failed");
        return -1;
    }

    /* Return to beginning of file */
    rewind(fp);

    /* Allocate buffer for file contents */
    buffer = (char *)aicos_malloc(MEM_CMA, size + 1);
    if (!buffer) {
        pr_err("Memory allocation failed");
        return -1;
    }

    /* Read entire file */
    bytes_read = fread(buffer, 1, size, fp);
    if (bytes_read == 0 && ferror(fp)) {
        aicos_free(MEM_CMA, buffer);
        pr_err("File read failed");
        return -1;
    }
    buffer[bytes_read] = '\0';

    /* Display contents */
    rt_kprintf("Read %lu bytes: \"%s\"\n", (unsigned long)bytes_read, buffer);
    aicos_free(MEM_CMA, buffer);

    /* Reset read pointer */
    rewind(fp);
    return 0;
}

/**
 * Appends content to the end of a file
 *
 * @param fp FILE pointer to append to
 * @param content String to append
 * @return 0 on success, -1 on failure
 */
static int append_to_file(FILE *fp, const char *content)
{
    size_t count = strlen(content);
    size_t bytes_written = 0;

    /* Go to end of file */
    if (fseek(fp, 0, SEEK_END) != 0) {
        pr_err("Seek to end failed");
        return -1;
    }

    /* Write content at end position */
    bytes_written = fwrite(content, 1, count, fp);
    if (bytes_written != count) {
        pr_err("Write failed. Expected:%zu, Actual:%zu", count, bytes_written);
        return -1;
    }

    rt_kprintf("Appended %lu bytes: \"%s\"\n", (unsigned long)bytes_written, content);
    return 0;
}

/**
 * Truncates a file to the specified length
 *
 * @param fp FILE pointer to truncate
 * @param length Desired file length
 * @return 0 on success, -1 on failure
 */
static int truncate_file(FILE *fp, long length)
{
    long current_size = -1;

    /* Convert to file descriptor for truncation */
    if (ftruncate(fileno(fp), length) == -1) {
        pr_err("File truncation failed");
        return -1;
    }

    /* Update file stream position */
    fseek(fp, 0, SEEK_SET);

    /* Verify new file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        pr_err("Size verification failed");
        return -1;
    }

    current_size = ftell(fp);
    if (current_size != length) {
        pr_err("Truncation failed. Size:%ld (Expected:%ld)", current_size, length);
        return -1;
    }

    rt_kprintf("Truncated file to %ld bytes\n", current_size);

    /* Return to beginning for future operations */
    rewind(fp);
    return 0;
}

/**
 * File Operations Demonstration using C Standard I/O
 *
 * Demonstrates:
 * - Opening files using fopen()
 * - Closing files using fclose()
 * - Writing content using fwrite()
 * - Reading files (wrapped function)
 * - Appending content (wrapped function)
 * - Truncating files (wrapped function)
 * - File flush using fflush()
 */
static int cmd_c_fs_demo(int argc, char **argv)
{
    FILE *fp = NULL;
    const char *init_text = "This is an initial text. ";
    const char *append_text = "This is an append text";
    size_t bytes = 0;
    int ret = 0;

    rt_kprintf("C standard file system usage example start\n");

    /* Create or truncate a writable and readable file */
    rt_kprintf("Creating file (fopen) :%s\n", g_file_name);
    fp = fopen(g_file_name, "w+"); // Create/Truncate for read/write
    if (!fp) {
        pr_err("File creation failed");
        return -1;
    }

    /* Write initial content */
    rt_kprintf("Writing initial text (fwrite)\n");
    bytes = fwrite(init_text, 1, strlen(init_text), fp);
    if (bytes != strlen(init_text)) {
        pr_err("Initial write failed. Expected:%zu, Actual:%zu", strlen(init_text), bytes);
        ret = -1;
        goto close_file;
    }
    rt_kprintf("Write %lu bytes: \"%s\"\n", (unsigned long)bytes, init_text);

    /* Append content */
    rt_kprintf("Appending text\n");
    ret = append_to_file(fp, append_text);
    if (ret) {
        pr_err("Append failed");
        goto close_file;
    }

    /* Verify file content */
    rt_kprintf("Verifying File Content\n");
    ret = read_file(fp);
    if (ret) {
        pr_err("Read verification failed");
        goto close_file;
    }

    /* Truncate file */
    rt_kprintf("Truncating File\n");
    ret = truncate_file(fp, 15);
    if (ret) {
        pr_err("Truncation failed");
        goto close_file;
    }

    /* Verify truncated content */
    rt_kprintf("Verifying Truncated Content\n");
    ret = read_file(fp);
    if (ret) {
        pr_err("Post-truncate read failed");
        goto close_file;
    }

    /* Make all changes appear on disk */
    rt_kprintf("Flushing File (fflush)\n");
    ret = fflush(fp);
    if (ret) {
        pr_err("Flushing File failed\n");
    }

close_file:
    /* Close file */
    rt_kprintf("Close File (fclose)\n");
    ret = fclose(fp);
    if (ret)
        pr_err("Final close failed");

    rt_kprintf("C standard file system example end\n");
    return ret;
}

MSH_CMD_EXPORT_ALIAS(cmd_c_fs_demo, c_fs_demo, C standard file system example);
