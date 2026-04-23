/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef __AIC_UPG_NAND_FWC_SPL_H__
#define __AIC_UPG_NAND_FWC_SPL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include "upg_internal.h"

#define SPL_NAND_IMAGE_BACKUP_NUM 4
#define PAGE_CNT_PER_BLOCK        64
#define PAGE_TABLE_MAX_ENTRY      101
#define SLICE_DEFAULT_SIZE        2048
#define PAGE_TABLE_USE_SIZE       2048
#define PAGE_MAX_SIZE             4096
#define SPL_CANDIDATE_BLOCK_NUM   18
#define SPL_INVALID_BLOCK_IDX     0xFFFFFFFF
#define SPL_INVALID_PAGE_ADDR     0xFFFFFFFF

#define ROUNDUP(a, b) ((((a)-1) / (b) + 1) * (b))

#define MAX_DUPLICATED_PART 4

#ifdef AIC_NFTL_SUPPORT
#include <nftl_api.h>
#endif
struct aicupg_nand_priv {
    struct mtd_dev *mtd[MAX_DUPLICATED_PART];
#ifdef AIC_NFTL_SUPPORT
    struct nftl_api_handler_t *nftl_handler[MAX_DUPLICATED_PART];
#endif
    unsigned long start_offset[MAX_DUPLICATED_PART];
    unsigned long erase_offset[MAX_DUPLICATED_PART];
    unsigned char remain_data[PAGE_MAX_SIZE];
    unsigned int remain_len;
    int spl_flag;
};

s32 nand_fwc_spl_reserve_blocks(struct aicupg_nand_priv *priv);
s32 nand_fwc_spl_prepare(struct aicupg_nand_priv *priv, u32 datasiz,
                         u32 blksiz);
s32 nand_fwc_spl_write(struct fwc_info *fwc, u8 *buf, s32 len);
s32 nand_fwc_spl_end(struct aicupg_nand_priv *priv);
int nand_spl_get_candidate_blocks(u32 *blks, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_UPG_NAND_FWC_SPL_H__ */
