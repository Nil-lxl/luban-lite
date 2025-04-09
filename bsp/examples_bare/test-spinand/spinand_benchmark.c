/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.Chen <jiji.chen@artinchip.com>
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <spinand_port.h>
#include <aic_utils.h>

#define SPINAND_HELP                       \
    "spinand_benchmark command:\n"        \
    "!!! DANGER:\n"       \
    "  This command will erase all data of spinand.\n"  \
    "  spinand_benchmark 0\n"

static struct aic_spinand *s_spinand_flash = NULL;
static uint64_t s_total_block = 0;
static u8 *s_read_buf = NULL, *s_write_buf = NULL;

static void spinand_help(void)
{
    puts(SPINAND_HELP);
}

static int do_spinand_init(int spi_bus)
{
    int index;
    u32 total_page_size;
    s_spinand_flash = spinand_probe(spi_bus);
    if (s_spinand_flash == NULL) {
        printf("Failed to probe spinand flash.\n");
        return -1;
    }

    total_page_size = s_spinand_flash->info->page_size + s_spinand_flash->info->oob_size;

    s_read_buf = aicos_malloc_align(0, total_page_size, CACHE_LINE_SIZE);
    s_write_buf = aicos_malloc_align(0, total_page_size, CACHE_LINE_SIZE);

    if (s_read_buf && s_write_buf) {
        for (index = 0; index < total_page_size; index++) {
            s_write_buf[index] = rand() % 0xff;
        }
    }

    return 0;
}

static int do_spinand_info_show()
{
    uint64_t total_size;

    if (s_spinand_flash == NULL)
        return -1;

    total_size = s_spinand_flash->info->block_per_lun * s_spinand_flash->info->pages_per_eraseblock
                     * s_spinand_flash->info->page_size * s_spinand_flash->info->planes_per_lun;
    s_total_block = s_spinand_flash->info->block_per_lun * s_spinand_flash->info->planes_per_lun;

    printf("Device infomation:\n");
    printf("spinand id: %02x%02x%02x%02x\n", s_spinand_flash->id.data[0], s_spinand_flash->id.data[1],
                                             s_spinand_flash->id.data[2], s_spinand_flash->id.data[3]);
    printf("spinand data lines width: %u\n", s_spinand_flash->qspi_dl_width);
    printf("spinand device size: %"PRIu64"\n", total_size);
    printf("erase size: %u ", s_spinand_flash->info->page_size * s_spinand_flash->info->pages_per_eraseblock);
    printf("page size: %u\n", s_spinand_flash->info->page_size);
    printf("eraseblocks count: %"PRIu64" ", s_total_block);
    printf("oob size: %u\n", s_spinand_flash->info->oob_size);

    return 0;
}

static int do_spinand_badblock_check()
{
    u32 block = 0, bad_block_cnt = 0;
    u32 erase_fail_cnt = 0;
    int ret = 0;

    if (s_spinand_flash == NULL)
        return -1;

    printf("\n === 1. scan all blocks ===\n");
    for (block = 0; block < s_total_block; block++) {
        if (spinand_block_isbad(s_spinand_flash, block)) {
            spinand_block_markbad(s_spinand_flash, block);
            bad_block_cnt++;
        }
    }
    printf("There are %u bad blocks in the whole %"PRIu64" blocks.\n", bad_block_cnt, s_total_block);

    printf("\n === 2. erase all blocks and scan ===\n");
    for (block = 0; block < s_total_block; block++) {
        ret = spinand_block_erase(s_spinand_flash, block);
        if (ret) {
            spinand_block_markbad(s_spinand_flash, block);
            erase_fail_cnt++;
        }
    }
    printf("Erase the whole falsh:\n");
    printf("There are %u bad blocks when erase whole %"PRIu64" blocks.\n", erase_fail_cnt, s_total_block);
    printf("Check the whole falsh again:\n");
    bad_block_cnt = 0;
    for (block = 0; block < s_total_block; block++) {
        if (spinand_block_isbad(s_spinand_flash, block)) {
            spinand_block_markbad(s_spinand_flash, block);
            bad_block_cnt++;
        }
    }
    printf("There are %u bad blocks in the whole %"PRIu64" blocks.\n", bad_block_cnt, s_total_block);

    return 0;
}

static int verify_mem(u8 *src1, u8 *src2, u32 cnt) {
    int i = 0;
    for (i = 0; i < cnt; i++) {
        if (src1[i] != src2[i]) {
            printf("varify fail! i = %d, 0x%02x != 0x%02x\n", i, src1[i], src2[i]);
            return -1;
        }
    }
    return 0;
}

static int do_spinand_write_read()
{
    u32 block = 0, page = 0, p;
    int ret = 0;
    uint64_t start_us;

    if (s_spinand_flash == NULL)
        return -1;

    printf("\n === 3. test write read speed ===\n");
    start_us = aic_get_time_us();
    ret = spinand_write_page(s_spinand_flash, page, s_write_buf, s_spinand_flash->info->page_size, NULL, 0);
    show_speed("page write speed", s_spinand_flash->info->page_size, aic_get_time_us() - start_us);

    start_us = aic_get_time_us();
    ret = spinand_read_page(s_spinand_flash, page, s_read_buf, s_spinand_flash->info->page_size, NULL, 0);
    show_speed("page read speed", s_spinand_flash->info->page_size, aic_get_time_us() - start_us);
    ret = verify_mem(s_write_buf, s_read_buf, s_spinand_flash->info->page_size);
    if (ret) {
        printf("%s: %d, data verify faild! page: %u\n", __func__, __LINE__, page);
        return ret;
    }

    printf("write read all pages\n");
    for (block = 1; block < s_total_block; block++) {
        page = block * s_spinand_flash->info->pages_per_eraseblock;
        for (p = 0; p < s_spinand_flash->info->pages_per_eraseblock; p++) {
            if (!spinand_block_isbad(s_spinand_flash, block)) {
                ret = spinand_write_page(s_spinand_flash, page + p, s_write_buf, s_spinand_flash->info->page_size, NULL, 0);
                if (ret)
                    return ret;
                ret = spinand_read_page(s_spinand_flash, page + p, s_read_buf, s_spinand_flash->info->page_size, NULL, 0);
                if (ret)
                    return ret;
            }
        }
    }

    /* verify 5 pages */
    int i;
    for (i = 0; i < 5; i++) {
        p = page + rand() % s_spinand_flash->info->pages_per_eraseblock;
        ret = spinand_read_page(s_spinand_flash, p, s_read_buf, s_spinand_flash->info->page_size, NULL, 0);
        if (ret)
                return ret;
        ret = verify_mem(s_write_buf, s_read_buf, s_spinand_flash->info->page_size);
        if (ret) {
            printf("%s: %d, data verify faild! page: %u\n", __func__, __LINE__, p);
            return ret;
        }
    }

    return 0;
}

#ifdef AIC_SPINAND_CONT_READ
#define CONT_NUM 16
static int do_spinand_contread()
{
    u32 block = 0, page = 0;
    int ret = 1, i;
    u8 *r_buf, *w_buf;
    uint64_t start_us;

    if (s_spinand_flash == NULL)
        return -1;

    do {
        block = rand() % s_total_block;
        if (!spinand_block_isbad(s_spinand_flash, block))
            ret = spinand_block_erase(s_spinand_flash, block);
    } while(ret);

    r_buf = aicos_malloc_align(0, s_spinand_flash->info->page_size * CONT_NUM, CACHE_LINE_SIZE);
    w_buf = aicos_malloc_align(0, s_spinand_flash->info->page_size * CONT_NUM, CACHE_LINE_SIZE);
    if (!r_buf || !w_buf) {
        printf("malloc failed!\n");
        goto err;
    }

    printf("\n === 4. test continue read speed ===\n");
    for (i = 0; i < s_spinand_flash->info->page_size * CONT_NUM; i++) {
        w_buf[i] = rand() % 0xff;
    }

    page = block * s_spinand_flash->info->pages_per_eraseblock;
    u32 w_page = page;
    for (i = 0; i < CONT_NUM; i++) {
        spinand_write_page(s_spinand_flash, w_page++, w_buf, s_spinand_flash->info->page_size, NULL, 0);
    }

    start_us = aic_get_time_us();
    spinand_continuous_read(s_spinand_flash, page, r_buf, s_spinand_flash->info->page_size * CONT_NUM);
    show_speed("cont_read speed", s_spinand_flash->info->page_size * CONT_NUM, aic_get_time_us() - start_us);
    ret = verify_mem(r_buf, w_buf, s_spinand_flash->info->page_size * CONT_NUM);
    if (ret) {
        printf("%s: %d, data verify faild! page: %u\n", __func__, __LINE__, page);
        goto err;
    }

err:
    if (r_buf)
        aicos_free_align(0, r_buf);
    if (w_buf)
        aicos_free_align(0, w_buf);

    return ret;
}
#else
static int do_spinand_contread()
{
    printf("\n === 4. test continue read speed ===\n");
    printf("do not support spinand continue read\n");
    return 0;
}
#endif

#define OOB_USER_DATA_NUM   16

static int do_spinand_oob_verify()
{
    u32 block = 0, page = 0, p;
    int ret = 1, i;
    u8 r_buf[OOB_USER_DATA_NUM], w_buf[OOB_USER_DATA_NUM];
    u8 *r_oob_buf, *w_oob_buf;

    if (s_spinand_flash == NULL)
        return -1;

    do {
        block = rand() % s_total_block;
        if (!spinand_block_isbad(s_spinand_flash, block))
            ret = spinand_block_erase(s_spinand_flash, block);
    } while(ret);

    if (!s_read_buf || !s_write_buf) {
        printf("malloc failed!\n");
        goto err;
    }
    r_oob_buf = s_read_buf + s_spinand_flash->info->page_size;
    w_oob_buf = s_write_buf + s_spinand_flash->info->page_size;

    printf("\n === 5. test oob write read ===\n");
    for (i = 1; i < OOB_USER_DATA_NUM; i++) {
        w_buf[i] = rand() % 0xff;
    }
    w_buf[0] = 0xff;

    page = block * s_spinand_flash->info->pages_per_eraseblock;
    printf("write read and verify %u bytes on oob user region\n", OOB_USER_DATA_NUM);
    for (i = 0; i < 10; i++) {
        p = page + rand() % s_spinand_flash->info->pages_per_eraseblock;
        ret = spinand_ooblayout_map_user(s_spinand_flash, w_oob_buf, w_buf, 0, OOB_USER_DATA_NUM);
        spinand_write_page(s_spinand_flash, p, NULL, 0, w_oob_buf, s_spinand_flash->info->oob_size);
        spinand_read_page(s_spinand_flash, p, NULL, 0, r_oob_buf, s_spinand_flash->info->oob_size);
        ret = spinand_ooblayout_unmap_user(s_spinand_flash, r_buf, r_oob_buf, 0, OOB_USER_DATA_NUM);
        ret = verify_mem(r_buf, w_buf, OOB_USER_DATA_NUM);
        if (ret) {
            printf("%s: %d, data verify faild! page: %u\n", __func__, __LINE__, p);
            goto err;
        }
    }

err:
    return ret;
}

static int do_spinand_benchmark(int argc, char *argv[])
{
    if (argc < 2) {
        spinand_help();
        return 0;
    }

    printf("\n");
    printf("=====================================================\n");
/* command: spinand_benchmark 0 */

    if (do_spinand_init(strtol(argv[1], NULL, 0)))
        goto err;

    do_spinand_info_show();

    /* check all bad blocks and erase */
    do_spinand_badblock_check();
    /* write read and verify */
    do_spinand_write_read();
    /* continue read verify */
    do_spinand_contread();
    /* oob user region write read verify */
    do_spinand_oob_verify();

err:
    if (s_read_buf)
        aicos_free_align(0, s_read_buf);
    if (s_write_buf)
        aicos_free_align(0, s_write_buf);
    return 0;
}

CONSOLE_CMD(spinand_benchmark, do_spinand_benchmark, "SPI NAND flash benchmark.");
