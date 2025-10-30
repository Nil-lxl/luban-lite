/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.CHen <jiji.chen@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <aic_core.h>
#include "aic_time.h"
#include <aic_crc32.h>
#include <rtthread.h>
#include <dfs_posix.h>

#define SEARCH_PATH   "/data"

#define BUFFER_SIZE 4096
#define FS_READ_TIME_MS 3000

static int find_first_file(const char *path, char *found_path, int max_len) {
    DIR *dir;
    struct dirent *dirent;
    struct stat stat_buf;
    char fullpath[DFS_PATH_MAX];

    dir = opendir(path);
    if (dir == RT_NULL) {
        rt_kprintf("Open dir %s failed!\n", path);
        return 0;
    }

    while ((dirent = readdir(dir)) != RT_NULL) {
        if (rt_strcmp(dirent->d_name, ".") == 0 ||
            rt_strcmp(dirent->d_name, "..") == 0) {
            continue;
        }

        rt_snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dirent->d_name);

        if (stat(fullpath, &stat_buf) < 0) {
            continue;
        }

        if (S_ISREG(stat_buf.st_mode)) {
            rt_strncpy(found_path, fullpath, max_len);
            closedir(dir);
            return 1;
        } else if (S_ISDIR(stat_buf.st_mode)) {
            if (find_first_file(fullpath, found_path, max_len)) {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

static int check_file_crc32(char *file_path) {
    int fd = -1;
    uint8_t *buffer;
    u32 read_bytes = 0;
    u32 crc32_val_old = crc32(0, NULL, 0);
    u32 crc32_val = crc32(0, NULL, 0);


    buffer = (uint8_t *)aicos_malloc(MEM_CMA, BUFFER_SIZE);
    if (buffer == RT_NULL) {
        rt_kprintf("buffer: no memory\n");
        return -1;
    }

    uint64_t start_ms = aic_get_time_ms();
    while (1) {
        crc32_val_old = crc32_val;
        crc32_val = crc32(0, NULL, 0);
        fd = open(file_path, O_RDONLY);
        if (fd >= 0) {
            while((read_bytes = read(fd, buffer, BUFFER_SIZE))) {
                crc32_val = crc32(crc32_val, buffer, read_bytes);
            }
            static int print_flag = 0;
            if (print_flag == 0) {
                print_flag++;
                rt_kprintf("crc32_val: file: %s 0x%x\n", file_path, crc32_val);
            }

            close(fd);
            fd = -1;
        } else {
            rt_kprintf("open file error\n");
            goto error;
        }
        if (crc32_val_old == 0)
            continue;
        if (crc32_val != crc32_val_old) {
            rt_kprintf("crc32 error! crc32_val_old: 0x%x, crc32_val: 0x%x\n", crc32_val_old, crc32_val);
            goto error;
        }
        if ((aic_get_time_ms() - start_ms) > FS_READ_TIME_MS)
            break;
    }

error:
    if (buffer)
        aicos_free(MEM_CMA, buffer);

    return 0;
}

static void search_thread_entry(void *parameter) {
    char found_path[DFS_PATH_MAX] = {0};

    rt_kprintf("Start searching in: %s\n", SEARCH_PATH);

    if (find_first_file(SEARCH_PATH, found_path, sizeof(found_path))) {
        rt_kprintf("Found first file: %s\n", found_path);
    } else {
        rt_kprintf("No files found in directory tree!\n");
    }

    if (check_file_crc32(found_path)) {
        rt_kprintf("Error! read file data not correct.\n");
        return;
    }

    rt_kprintf("Check file completed!\n");
}

static void cmd_fs_read_thread(int argc, char **argv) {
    rt_thread_t tid;

    tid = rt_thread_create("file_search", search_thread_entry, RT_NULL, 4096, 20, 10);

    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    } else {
        rt_kprintf("Create thread failed!\n");
        return;
    }

    return;
}

MSH_CMD_EXPORT_ALIAS(cmd_fs_read_thread, fs_read_thread, Filesystem stress test);
