/*
 * Copyright (C) 2022-2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Geo Dong <guojun.dong@artinchip.com>
 */
#include <rtthread.h>
#include "lvgl.h"
#include "dfs_posix.h"
#include <sys/errno.h>
#include <unistd.h>

// #define DM_EXCUTABLE_FILE "/sdcard/hello.mo"
#define DM_EXCUTABLE_FILE "/data/hello.mo"

int dm_daemon(void)
{
    if (access(DM_EXCUTABLE_FILE, 0) == 0) {
        printf(" '%s' exist\n", DM_EXCUTABLE_FILE);
        system(DM_EXCUTABLE_FILE);
    } else {
        printf("file '%s' does not exist!\n", DM_EXCUTABLE_FILE);
    }

    return 0;
}

