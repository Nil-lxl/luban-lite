/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Author: <qi.xu@artinchip.com>
 *  Desc: unzip test demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <rthw.h>
#include <rtthread.h>
#include "aic_core.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_zlib.h"

static void print_help(void)
{
    printf("Usage: unzip_test [OPTIONS] [SLICES PATH]\n\n"
        "Options:\n"
        " -i                             input stream file name\n"
        " -h                             help\n\n"
        "End:\n");
}

static int get_file_size(int fd, char* path)
{
    struct stat st;
    stat(path, &st);

    logi("size: %ld", st.st_size);

    return st.st_size;
}

static int save_output(void* dst, int len)
{
    FILE* fp_save = fopen("/sdcard/save.bin", "wb");
    if (fp_save == NULL) {
        loge("open save file failed");
        return -1;
    }
    fwrite(dst, 1, len, fp_save);
    fclose(fp_save);
    return 0;
}

static int unzip(char *file_name)
{
    int i;
    int ret = 0;
    int fd = 0;
    int file_len = 0;
    void *mpp_unzip_ctx = NULL;
    void *src = NULL;
    void *dst = NULL;
    int src_len = 32*1024;
    int dst_len = 1024*1024;
    src = aicos_malloc_align(MEM_CMA, src_len, CACHE_LINE_SIZE);
    if (src == NULL) {
        loge("malloc src failed");
        ret = -1;
        goto out;
    }
    aicos_dcache_clean_invalid_range(src, src_len);

    dst = aicos_malloc_align(MEM_CMA, dst_len, CACHE_LINE_SIZE);
    if (dst == NULL) {
        loge("malloc dst failed");
        ret = -1;
        goto out;
    }
    aicos_dcache_clean_invalid_range(dst, dst_len);

    fd = open(file_name, O_RDONLY);
    if(fd < 0) {
        loge("open file(%s) failed, %d", file_name, fd);
        ret = -1;
        goto out;
    }
    file_len = get_file_size(fd, file_name);
    if (file_len <= 0) {
        loge("get file size failed");
        ret = -1;
        goto out;
    }

    mpp_unzip_ctx = mpp_unzip_create();
    if (mpp_unzip_ctx == NULL) {
        loge("mpp_unzip_create failed");
        goto out;
    }

    int out_len = 0;
    int loop_times = (file_len + src_len -1)/ src_len;
    int read_len;
    for (i=0; i<loop_times; i++) {
        read_len = read(fd, src, src_len);
        if (read_len < 0) {
            loge("read data failed");
            ret = -1;
            goto out;
        }
        aicos_dcache_clean_range(src, read_len);

        out_len = mpp_unzip_uncompressed(mpp_unzip_ctx, MPP_GZIP,
            src, read_len,
            dst, dst_len, i==0, i==(loop_times-1));

        printf("out_len: %d\n", out_len);
    }

    save_output(dst, out_len);

out:
    if (mpp_unzip_ctx)
        mpp_unzip_destroy(mpp_unzip_ctx);
    if (fd)
        close(fd);
    if (src)
        aicos_free_align(MEM_CMA, src);
    if (dst)
        aicos_free_align(MEM_CMA, dst);
    return ret;
}

int unzip_test(int argc, char **argv)
{
    int opt;
    char file_name[128] = {0};

    if (argc < 2) {
        print_help();
        return -1;
    }

    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:h");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            logd("file path: %s", optarg);
            strcpy(file_name, optarg);
            break;
        case 'h':
        default:
            print_help();
            return -1;
        }
    }

    if (unzip(file_name) < 0) {
        loge("unzip failed");
        return -1;
    }

    return 0;
}

MSH_CMD_EXPORT_ALIAS(unzip_test, unzip_test, unzip test);
