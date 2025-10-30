/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Cui Jiawei <jiawei.cui@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include "nftl_api.h"
#include <mtd.h>

#define PARTITION_NAME "data_r"

/* Their sum should be less than 0x10D00 */
#define TEST_SIZE   0x10
#define TEST_OFFSET 0x0

static struct nftl_api_handler_t *g_nftl_t = NULL;
static u8 *g_write_buf = NULL;
static u8 *g_read_buf = NULL;

static void address_map(u8 *data, u32 start_address, u32 len)
{
    u32 i;

    for (i = 0; i < len; i++) {
        if (i && (i % 16) == 0)
            printf("\n");
        if ((i % 16) == 0) {
            printf("0x%08x : ", start_address);
            start_address += 16;
        }

        printf("%02x ", data[i]);
    }
    printf("\n");
}

static int nftl_write_read(u32 offset, u32 len)
{
    int err = 0;

    printf("=== SPINAND nftl write read example ===\n");

    /* Write the data within the address range managed by nftl. */
    err = nftl_api_write(g_nftl_t, offset, len, g_write_buf);
    if (err) {
        pr_err("Write failed (code=%d)!\n", err);
        goto exit;
    }

    memset(g_read_buf, 0, len);
    /* Read the data within the address range managed by nftl. */
    err = nftl_api_read(g_nftl_t, offset, len, g_read_buf);
    if (err) {
        pr_err("Read failed (code=%d)!\n", err);
        goto exit;
    }

    /* Data verification */
    if (memcmp(g_write_buf, g_read_buf, len) != 0) {
        pr_err("Data verification failed!\n");
        printf("expected:-----------------------\n");
        address_map(g_write_buf, offset, len);
        printf("actual:-------------------------\n");
        address_map(g_read_buf, offset, len);
    } else {
        if (len > 0x100) {
            address_map(g_read_buf, offset, 0x100);
            printf("......\n");
            address_map(g_read_buf + len - 0x50, offset + len - 0x50, 0x50);
        } else {
            address_map(g_read_buf, offset, len);
        }

        printf("SUCCESS: SPINAND nftl read/write test passed!\n");
    }

exit:
    return err;
}

static int nftl_init(const char *part_name)
{
    struct mtd_dev *mtd = NULL;
    u32 part_cnt, i;

    mtd_probe();

    part_cnt = mtd_get_device_count();
    for (i = 0; i < part_cnt; i++) {
        mtd = mtd_get_device_by_id(i);
        if (mtd == NULL)
            break;

        if (!strcmp(part_name, mtd->name))
            break;
        mtd = NULL;
    }

    if (!mtd) {
        pr_err("MTD partition '%s' not found!\n", part_name);
        return -1;
    }

    g_nftl_t = aicos_malloc(0, sizeof(struct nftl_api_handler_t));
    if (!g_nftl_t) {
        pr_err("Out of memory, failed to malloc for NFTL.\n");
        return -1;
    }

    memset(g_nftl_t, 0, sizeof(struct nftl_api_handler_t));
    g_nftl_t->priv_mtd = (void *)mtd;

    g_nftl_t->nandt = aicos_malloc(0, sizeof(struct nftl_api_nand_t));
    if (!(g_nftl_t->nandt)) {
        pr_err("Out of memory, failed to malloc for nandt.\n");
        return -1;
    }

    g_nftl_t->nandt->page_size = mtd->writesize;
    g_nftl_t->nandt->oob_size = mtd->oobsize;
    g_nftl_t->nandt->pages_per_block = mtd->erasesize / mtd->writesize;
    g_nftl_t->nandt->block_total = mtd->size / mtd->erasesize;
    g_nftl_t->nandt->block_start = mtd->start / mtd->erasesize;
    g_nftl_t->nandt->block_end = (mtd->start + mtd->size) / mtd->erasesize;

    /* The second parameter is the index number of this MTD device in the MTD table. */
    if (nftl_api_init(g_nftl_t, i)) {
        pr_err("[NE]nftl_initialize failed\n");
        return -1;
    }

    return 0;
}

static int cmd_spinand_nftl_example(int argc, char *argv[])
{
    static bool first_run = true;

    /* Create an instance. Note that this instance cannot be destroyed. */
    if (first_run) {
        if (nftl_init(PARTITION_NAME)) {
            pr_err("SPINAND nftl init failed!\n");
            goto err;
        }
        first_run = false;
    }

    /* Allocate test data buffer */
    g_write_buf = aicos_malloc(0, TEST_SIZE);
    if (!g_write_buf) {
        pr_err("No memory for test data!\n");
        return -1;
    }

    /* Allocate read buffer */
    g_read_buf = aicos_malloc(0, TEST_SIZE);
    if (!g_read_buf) {
        pr_err("No memory for read buffer!\n");
        aicos_free(0, g_write_buf);
        return -1;
    }

    /* prepare some data */
    for (int i = 0; i < TEST_SIZE; i++) {
        g_write_buf[i] = i % 256;
    }

    /* Conduct read/write tests and verify data consistency. */
    if (nftl_write_read(TEST_OFFSET, TEST_SIZE))
        goto err;

    printf("spinand nftl usage success!\n");
    goto deinit;

err:
    pr_err("spinand nftl usage failed!\n");

deinit:
    if (g_write_buf)
        aicos_free(0, g_write_buf);
    if (g_read_buf)
        aicos_free(0, g_read_buf);

    return 0;
}

CONSOLE_CMD(nftl_usage, cmd_spinand_nftl_example, "spinand nftl example");
