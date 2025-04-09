/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <spinand_port.h>
#include <mtd.h>
#include <image.h>
#include <boot.h>
#include <aic_utils.h>
#include "aic_time.h"

#define APPLICATION_PART "os"
#define PAGE_MAX_SIZE    4096

static int do_nand_boot(int argc, char *argv[])
{
    int ret = 0;
    struct image_header *head;
    struct mtd_dev *mtd;
    void *la;
    u64 start_us;

    mtd_probe();
    mtd = mtd_get_device(APPLICATION_PART);
    if (!mtd) {
        printf("Failed to get application partition.\n");
        return -1;
    }

    head = malloc(mtd->writesize);
    ret = mtd_read(mtd, 0, (void *)head, mtd->writesize);
    if (ret < 0) {
        free((void *)head);
        printf("Read image header failed.\n");
        return -1;
    }

    ret = image_verify_magic((void *)head, AIC_IMAGE_MAGIC);
    if (ret) {
        free((void *)head);
        printf("Application header is unknown.\n");
        return -1;
    }

    la = (void *)(unsigned long)head->load_address;

    start_us =  aic_get_time_us();
#ifdef AIC_SPINAND_CONT_READ
    ret = mtd_contread(mtd, 0, la, ROUNDUP(head->image_len, mtd->writesize));
    if (ret < 0)
#endif
        ret = mtd_read(mtd, 0, la, ROUNDUP(head->image_len, mtd->writesize));

    show_speed("nand read speed", head->image_len, aic_get_time_us() - start_us);
    if (ret < 0) {
        free((void *)head);
        printf("Read image failed.\n");
        return -1;
    }

    boot_app(la);

    return ret;
}

CONSOLE_CMD(nand_boot, do_nand_boot, "Boot from NAND.");
