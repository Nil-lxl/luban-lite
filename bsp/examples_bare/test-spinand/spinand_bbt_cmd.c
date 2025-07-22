/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.Chen <jiji.chenen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include <spinand_port.h>
#include <spinand_bbt.h>

#define SPINAND_BBT_HELP                       \
    "As spinand will generate bad blocks randomly, erase/read/write need to avoid bad blocks\n"\
    "or the data will be unstable. Spinand_bbt maintain a bad block table, it can translate \n"\
    "logic_addr to physic_addr and avoid bad blocks by itself.\n"    \
    "In this way, users can erase/read/write logic_addr without considering bad blocks.\n"    \
    "\n"                            \
    "spinand_bbt command:\n"        \
    "  spinand_bbt init  <spi_bus> <offset> <size>\n"       \
    "       init a bbt control, manage the address of spinand[spi_bus] with bad block management"\
    "       the truth size subcommands can use is (size - block_size * bad_block_reserved(default 8))"\
    "  spinand_bbt dump <offset size>\n"       \
    "       dump the data of spinand_bbt in logic_addr (offset, offset + size)"\
    "  spinand_bbt write <addr offset size>\n" \
    "       write data to spinand_bbt in logic_addr (offset, offset + size)"\
    "  spinand_bbt erase <offset size>\n"       \
    "       erase spinand_bbt in logic_addr (offset, offset + size)\n"\
    "  spinand_bbt deinit\n"\
    "       terminate the instance and reclaim its resources\n"\
    "example:\n"        \
    "  spinand_bbt init 0 0x4800000 0x800000\n"       \
    "  spinand_bbt erase 0x5000000 0x100000\n"       \
    "  spinand_bbt write 0x40400000 0x5000800 0x80000\n"       \
    "  spinand_bbt dump 0x5000800 0x80000\n"       \

struct aic_spinand_bbt *g_spinand_bbt_t = NULL;

static void spinand_bbt_help(void)
{
    puts(SPINAND_BBT_HELP);
}

static int do_spinand_init(int argc, char *argv[])
{
    unsigned long spi_bus;
    u32 offset, size;

    if (argc < 4) {
        spinand_bbt_help();
        return -1;
    }

    spi_bus = strtol(argv[1], NULL, 0);
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    g_spinand_bbt_t = malloc(sizeof(struct aic_spinand_bbt));
    if (!g_spinand_bbt_t) {
        printf("malloc failed\n");
        return -1;
    }

    g_spinand_bbt_t->spinand_flash = spinand_probe(spi_bus);

    return spinand_bbt_init(g_spinand_bbt_t, 0, offset, size);
}

static int do_spinand_deinit(int argc, char *argv[])
{
    if (g_spinand_bbt_t == NULL)
        return 0;

    spinand_bbt_deinit(g_spinand_bbt_t);
    free(g_spinand_bbt_t);
    g_spinand_bbt_t = NULL;

    return 0;
}

static int do_spinand_dump(int argc, char *argv[])
{
    unsigned long offset, size;
    uint8_t *data;
    int ret = 0;

    if (argc < 3) {
        spinand_bbt_help();
        return 0;
    }

    if (!g_spinand_bbt_t->spinand_flash) {
        printf("Error, please init before.\n");
        return -1;
    }

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);
    data = malloc(size);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, size);

    ret = spinand_bbt_read(g_spinand_bbt_t, data, offset, size);

    if (!ret)
        hexdump((void *)data, size, 1);

    free(data);
    return 0;
}

static int do_spinand_write(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long offset, size;

    if (argc < 4) {
        spinand_bbt_help();
        return 0;
    }

    if (!g_spinand_bbt_t->spinand_flash) {
        printf("Error, please init before.\n");
        return -1;
    }

    addr = strtol(argv[1], NULL, 0);
    offset = strtol(argv[2], NULL, 0);
    size = strtol(argv[3], NULL, 0);

    spinand_bbt_write(g_spinand_bbt_t, (u8 *)addr, offset, size);

    return 0;
}

static int do_spinand_erase(int argc, char *argv[])
{
    unsigned long offset, size;
    u32 block_size = 0;

    if (argc < 3) {
        spinand_bbt_help();
        return 0;
    }

    if (!g_spinand_bbt_t->spinand_flash) {
        printf("Error, please init before.\n");
        return -1;
    }

    block_size = g_spinand_bbt_t->spinand_flash->info->page_size * g_spinand_bbt_t->spinand_flash->info->pages_per_eraseblock;

    offset = strtol(argv[1], NULL, 0);
    size = strtol(argv[2], NULL, 0);
    if (offset % block_size || size % block_size) {
        printf("offset: %lu or size: %lu, not align with block_size(%u)", offset, size, block_size);
        return -1;
    }

    spinand_bbt_erase(g_spinand_bbt_t, offset, size);

    return 0;
}

static int verify_mem(u8 *src1, u8 *src2, u32 cnt) {
    int i = 0;
    for (i = 0; i < cnt; i++) {
        if (src1[i] != src2[i]) {
            printf("varify fail! i = %d, 0x%02x != 0x%02x\n", i, src1[i], src2[i]);
            return i;
        }
    }
    return 0;
}

#define BBT_REGION_START    0x04800000
#define BBT_REGION_SIZE     0x600000
#define BBT_BUFFER_SIZE     0x100000

/* make a env */
static int spinand_bbt_make_env(u32 spi_bus, u32 offset, u32 size)
{
    struct aic_spinand *s_spinand_flash;
    s_spinand_flash = spinand_probe(spi_bus);
    if (s_spinand_flash == NULL) {
        printf("Failed to probe spinand flash.\n");
        return -1;
    }
    spinand_erase(s_spinand_flash, offset, size);

    int block_num = rand() % 5;
    for (int i = 0; i < block_num; i++) {
        int bad_block = rand() % (size / 0x20000);  // block_size == 0x20000
        u32 bad_block_t = bad_block + offset / 0x20000;
        printf("mark bad block: %u.\n", bad_block_t);
        spinand_block_markbad(s_spinand_flash, bad_block_t);
    }

    return 0;

}

#define BBT_ROUND_UP(a,b)       ((a + b - 1) / b * b)
#define TEST_ADD_NEW_BAD_BLOCK  0

static int do_spinand_bbt_benchmark(int argc, char *argv[])
{
    int ret = 0;

    ret = spinand_bbt_make_env(0, BBT_REGION_START, BBT_REGION_SIZE);
    if (ret)
        return -1;

    u8 *s_read_buf = NULL, *s_write_buf = NULL;
    s_read_buf = aicos_malloc_align(0, BBT_BUFFER_SIZE, CACHE_LINE_SIZE);
    if (!s_read_buf) {
        printf("malloc failed\n");
        return -1;
    }

    s_write_buf = aicos_malloc_align(0, BBT_BUFFER_SIZE, CACHE_LINE_SIZE);
    if (!s_write_buf) {
        printf("malloc failed\n");
        goto exit_free;
    }
    for (int index = 0; index < BBT_BUFFER_SIZE; index++) {
        s_write_buf[index] = rand() % 0xff;
    }

    struct aic_spinand_bbt *s_spinand_bbt_t = malloc(sizeof(struct aic_spinand_bbt));
    if (!s_spinand_bbt_t) {
        printf("malloc failed\n");
        goto exit_free;
    }
    memset(s_spinand_bbt_t, 0, sizeof(struct aic_spinand_bbt));

    s_spinand_bbt_t->spinand_flash = spinand_probe(0);
    ret = spinand_bbt_init(s_spinand_bbt_t, 0, BBT_REGION_START, BBT_REGION_SIZE);
    if (ret) {
        goto exit_spinand_bbt_benchmark;
    }

    if (NULL == s_spinand_bbt_t->spinand_flash) {
        printf("Error, please init before.\n");
        goto exit_spinand_bbt_benchmark;
    }

    u32 block_size = s_spinand_bbt_t->spinand_flash->info->page_size * s_spinand_bbt_t->spinand_flash->info->pages_per_eraseblock;
    u32 page_size = s_spinand_bbt_t->spinand_flash->info->page_size;

    for (int i = 0; i < 16; i++) {
        /* delay */
        aicos_mdelay(rand() % 1000);
        memset(s_read_buf, 0, BBT_BUFFER_SIZE);

        u32 start_offset = (rand() % (s_spinand_bbt_t->use_block_num - 1)) * block_size;
        printf(" ==== start block(%u) ==== \n", start_offset/block_size);
        u32 test_size = (rand() % (BBT_BUFFER_SIZE - page_size + 1)) + page_size;
        test_size = BBT_ROUND_UP(test_size, block_size);

        if ((test_size + start_offset) / block_size > s_spinand_bbt_t->use_block_num)
            test_size = (s_spinand_bbt_t->use_block_num * block_size) - start_offset;

        start_offset += BBT_REGION_START;

        if ((start_offset  + test_size) / block_size >= (s_spinand_bbt_t->use_block_num + s_spinand_bbt_t->start_block)) {
            printf(" start block(%u) out of range\n", start_offset/block_size);
            continue;
        }

        /* erase */
        printf("  ==  erase start block :%u size block :%u\n", start_offset / block_size, test_size / block_size);
        if (spinand_bbt_erase(s_spinand_bbt_t, start_offset, test_size)) {
            printf(" erase start:%u size:%u\n", (start_offset - BBT_REGION_START)/block_size, test_size/page_size);
            continue;
        }

        /* make a new bad block */
#if TEST_ADD_NEW_BAD_BLOCK
        static int do_flag = 0;
        int to_do_flag = 3;
        if (rand() % to_do_flag == 0) {
            if (do_flag == 0) {
                u32 new_bad = rand() % (test_size / block_size) + start_offset / block_size;
                printf(" mark a new bad block:%u.\n", new_bad);
                spinand_block_markbad(s_spinand_bbt_t->spinand_flash, new_bad);
                do_flag++;
            }
        }
#endif

        u32 temp_offset = (rand() % s_spinand_bbt_t->spinand_flash->info->pages_per_eraseblock) * page_size;
        start_offset += temp_offset;
        test_size -= temp_offset;

        printf("  ==  write/read/verify start page :%u size:%u\n", (start_offset % block_size) / page_size, test_size / page_size);
        if (spinand_bbt_write(s_spinand_bbt_t, (u8 *)s_write_buf, start_offset, test_size))
            continue;

        if (spinand_bbt_read(s_spinand_bbt_t, (u8 *)s_read_buf, start_offset, test_size))
            continue;
        int tt = verify_mem(s_write_buf, s_read_buf, test_size);
        if (tt) {
            u32 err_p = tt / page_size;
            u32 err_b = (tt / block_size) + start_offset / block_size + 1;
            printf("error_block:%u:%u, error_page:%u\n", err_b, start_offset / block_size, err_p);
            goto exit_spinand_bbt_benchmark;
        }
    }

exit_spinand_bbt_benchmark:
    if (s_spinand_bbt_t) {
        spinand_bbt_deinit(s_spinand_bbt_t);
        free(s_spinand_bbt_t);
        s_spinand_bbt_t = NULL;
    }
exit_free:
    if (s_read_buf)
        aicos_free_align(1, s_read_buf);
    if (s_write_buf)
        aicos_free_align(1, s_write_buf);

    return 0;
}

CONSOLE_CMD(spinand_bbt_benchmark, do_spinand_bbt_benchmark, "SPI NAND flash benchmark.");

static int do_spinand_bbt(int argc, char *argv[])
{
    if (argc < 2) {
        spinand_bbt_help();
        return 0;
    }

    if (!strncmp(argv[1], "init", 4))
        return do_spinand_init(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "dump", 4))
        return do_spinand_dump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "write", 5))
        return do_spinand_write(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "erase", 5))
        return do_spinand_erase(argc - 1, &argv[1]);
    spinand_bbt_help();
    if (!strncmp(argv[1], "deinit", 6))
        return do_spinand_deinit(argc - 1, &argv[1]);
    return 0;

}

CONSOLE_CMD(spinand_bbt, do_spinand_bbt, "SPI NAND flash R/W command.");
