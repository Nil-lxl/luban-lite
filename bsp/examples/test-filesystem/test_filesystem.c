/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.CHen <jiji.chen@artinchip.com>
 */

#include <string.h>
#include <finsh.h>
#include <aic_core.h>

#define     DATA_LEN_64K    64*1024
#define     DATA_LEN_16K    16*1024
#define     DATA_LEN_4K     4*1024
#define     DATA_LEN_1K     1*1024
#define     DATA_LEN_512B   512
#define     DATA_LEN_256B   256
#define     DATA_LEN_128B   128
#define     DATA_LEN_64B    64

u32 ENUM_DATA_LEN[] = {
    DATA_LEN_64K,
    DATA_LEN_16K,
    DATA_LEN_4K,
    DATA_LEN_1K,
    DATA_LEN_512B,
    DATA_LEN_256B,
    DATA_LEN_128B,
    DATA_LEN_64B
};


#define USAGE \
    "fs help : Get this information.\n" \
    "fs read <filename> : Test filesystem write speed.\n" \
    "fs write <filename> : Test filesystem write speed.\n" \
    "fs open <filename> : Test filesystem open speed.\n" \
    "fs close : Test filesystem close speed.\n" \
    "example:\n" \
    "fs write data auto\n" \
    "fs read data auto\n" \
    "fs write data/testfile.txt 2048\n" \
    "fs read data/testfile.txt\n" \
    "fs read rodata/testfile.txt\n" \
    "fs open /data/testfile.txt\n" \
    "fs close \n" \
    "fs write\n"

static void fs_usage(void)
{
    printf("%s", USAGE);
}

static void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("Filesystem %s speed: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

static int write_read_auto(char *file_path, char *dir, char *data, bool read_en)
{
    uint32_t start_us;
    uint32_t i;
    FILE *fp;
    char *open_action;
    char *read_write;
    char *names[] = {
        "64k.txt",
        "16k.txt",
        "4k.txt",
        "1k.txt",
        "512b.txt",
        "256b.txt",
        "128b.txt",
        "64b.txt"
    };

    for (i = 1; i < sizeof(ENUM_DATA_LEN) / sizeof(ENUM_DATA_LEN[0]); i++) {
        rt_sprintf(file_path, "%s/%s", dir, names[i]);
        printf("Open file auto, file name: %s\n",file_path);

        open_action = read_en ? "rb" : "w";
        read_write = read_en ? "read" : "write";
        fp = fopen(file_path, open_action);
        if (fp == NULL) {
            printf("open file %s failed!\n", file_path);
            aicos_free_align(0, data);
            return -1;
        }

        start_us = aic_get_time_us();
        if (read_en) {
            if (ENUM_DATA_LEN[i] != fread(data, sizeof(char), ENUM_DATA_LEN[i], fp)) {
                printf("Fs %s data failed!\n", read_write);
                return -1;
            }
        } else {
            if (ENUM_DATA_LEN[i] != fwrite(data, sizeof(char), ENUM_DATA_LEN[i], fp)) {
                printf("Fs %s data failed!\n", read_write);
                return -1;
            }
        }

        show_speed(read_write, ENUM_DATA_LEN[i], aic_get_time_us() - start_us);
        fclose(fp);
    }
    return 0;
}

static void test_fs_read(int argc, char **argv)
{
    uint32_t start_us;
    char *data;
    FILE *fp;
    char file_path[128] = { 0 };

    data = aicos_malloc_align(0, DATA_LEN_64K, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Low memory");
        return;
    }

    if (argc > 2 && !rt_strcmp(argv[2], "auto")) {
        write_read_auto(file_path, argv[1], data, 1);
        return;
    }

    if (argc > 1)
        rt_sprintf(file_path, "%s", argv[1]);
    else
        rt_sprintf(file_path, "%s", "data/testfile.txt");

    fp = fopen(file_path, "rb");
    if (fp == NULL) {
        printf("Open file %s failed!\n", file_path);
        aicos_free_align(0, data);
        return;
    }

    start_us = aic_get_time_us();
    if (DATA_LEN_64K != fread(data, sizeof(char), DATA_LEN_64K, fp)) {
        printf("Fs read data success \n");
        goto read_err;
    }
    show_speed("read", DATA_LEN_64K, aic_get_time_us() - start_us);

read_err:
    fclose(fp);
    aicos_free_align(0, data);
    return;
}

/* fs write data/testfile.txt 2048 */
static void test_fs_write(int argc, char **argv)
{
    uint32_t start_us, write_len;
    char *data;
    FILE *fp;
    char file_path[128] = { 0 };

    data = aicos_malloc_align(0, DATA_LEN_64K, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Low memory");
        return;
    }

    if (argc > 2 && !rt_strcmp(argv[2], "auto")) {
        write_read_auto(file_path, argv[1], data, 0);
        return;
    }

    if (argc > 1)
        rt_sprintf(file_path, "%s", argv[1]);
    else
        rt_sprintf(file_path, "%s", "data/testfile.txt");

    fp = fopen(file_path, "w");
    if (fp == NULL) {
        printf("Open file %s failed!\n", file_path);
        aicos_free_align(0, data);
        return;
    }
    write_len = DATA_LEN_64K;
    if (argc > 2)
        write_len = strtoul(argv[2], NULL, 0);

    start_us = aic_get_time_us();
    if (write_len != fwrite(data, sizeof(char), write_len, fp)) {
        printf("Fs write data failed!");
        goto write_err;
    }
    show_speed("write", write_len, aic_get_time_us() - start_us);

write_err:
    fclose(fp);
    aicos_free_align(0, data);
    return;
}

static FILE *g_fp = NULL;

static void test_fs_open(int argc, char **argv)
{
    u64 start_us;
    char file_path[128] = { 0 };

    if (argc > 1)
        rt_sprintf(file_path, "%s", argv[1]);
    else
        rt_sprintf(file_path, "%s", "data/testfile.txt");

    printf("start open file %s\n", file_path);
    start_us = aic_get_time_us();
    g_fp = fopen(file_path, "rb");
    if (g_fp == NULL) {
        printf("Open file %s failed!\n", file_path);
        return;
    }
    printf("open a new file take: %llu us\n", (aic_get_time_us() - start_us));

    return;
}

static void test_fs_close(int argc, char **argv)
{
    u64 start_us;

    if (g_fp == NULL) {
        printf("Error: there is not file opened.\n");
        return;
    }

    start_us = aic_get_time_us();
    fclose(g_fp);
    printf("close file take: %llu us\n", (aic_get_time_us() - start_us));

    return;
}

static void cmd_test_fs(int argc, char **argv)
{
    if (argc < 2)
        goto help;

    if (!rt_strcmp(argv[1], "help")) {
        goto help;
    } else if (!rt_strcmp(argv[1], "read")) {
        test_fs_read(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "write")) {
        test_fs_write(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "open")) {
        test_fs_open(argc - 1, &argv[1]);
        return;
    } else if (!rt_strcmp(argv[1], "close")) {
        test_fs_close(argc - 1, &argv[1]);
        return;
    }
help:
    fs_usage();
}

MSH_CMD_EXPORT_ALIAS(cmd_test_fs, fs, Test fs);
