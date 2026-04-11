/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#ifndef __SPINAND_REFRESH_H__
#define __SPINAND_REFRESH_H__

#include <aic_core.h>
#include "spinand.h"

#define BLOCK_LIST_NUM           6
#define REFRESH_BLOCK_MAXNUM    64

#define REFRESH_OK              0
#define REFRESH_ERROR           -1

#define REFRESH_DBG_MESSAGE_ON 0
#define REFRESH_ERR_MESSAGE_ON 1

#if REFRESH_DBG_MESSAGE_ON
#define REFRESH_DBG printf
#else
#define REFRESH_DBG(...)
#endif

#if REFRESH_ERR_MESSAGE_ON
#define REFRESH_ERR printf
#else
#define REFRESH_ERR(...)
#endif

struct refresh_journal {
    u32 journal1_addr;
    u32 journal2_addr;
    u32 last_journal_addr;
    u32 last_journal_version;
    u32 last_journal_page;
    u8 block_is_good[REFRESH_BLOCK_MAXNUM];
    u8 *page_buff;
};

struct refresh_handle {
    struct aic_spinand *flash;
    u32 start;
    u32 size;
    struct refresh_journal *journal;
    u32 process_block_list[BLOCK_LIST_NUM];
    u32 bitflip_block_index;
    u32 backup_block_index;
    u8* buff;
};

typedef enum {
    REFRESH_BLOCK_DONE = 0,
    REFRESH_FOUND_BITFLIP = 1,
    REFRESH_BACKUP_DONE = 2,
    REFRESH_WRITE_ERROR = 3,    /* unexpected error status */
} refresh_page_state;

int spinand_refresh_init(struct aic_spinand *flash, u32 start, u32 size);
int spinand_refresh_report_bitflip(struct aic_spinand *flash, u32 page);

#endif /* __SPINAND_REFRESH_H__ */
