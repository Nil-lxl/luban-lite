/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.Chen <jiji.chenen@artinchip.com>
 */

#include <spinand.h>

/*
 * // *(bad block), #(reserved block)
 * --->                                           total_block                                            <---
 *|__________________________________________________________________________________________________________|
 *|_0_|_1_|_2_|_3_|_*_|_5_|_6_|_7_|_8_|_9_|_10_|_11_|_12_|_13_|_14_|_15_|_16_|_#_|_#_|_#_|_#_|_#_|_#_|_#_|_#_|
 *
 * --->      logic block( = total block - reserved block)                <---
 *|___________________________________________________________________________|     ______________
 *|_0_|_1_|_2_|_3_|_5_|_6_|_7_|_8_|_9_|_10_|_11_|_12_|_13_|_14_|_15_|_16_|_17_| // |_physic block_|
 *
 *
 * Note: When a new bad block is detected during the spinand_bbt life cycle, it triggers an operation to move all
 *       data from the affected block to the next available block. Additionally, the reserved_block_free count
 *       decreases by one. If there are no remaining reserved blocks in this situation, an error will be returned.
 */

struct aic_spinand_bbt {
    struct aic_spinand *spinand_flash;
    u32 start_block;
    u32 end_block;
    u32 *bbt;
    u32 bad_block_num;
    u32 total_block_num;
    u32 reserved_block_num;
    u32 use_block_num;          // = (total_block_num - reserved_block_num)
};

enum bbt_ret_code {
    BBT_OK,
    BBT_ERR,
    BBT_BAD_OVER,   /* too many bad blocks */
    BBT_NEW_BAD,    /* Found a new bad block, when we can handle it. */
    BBT_ERR_UNKNOW,
};

int spinand_bbt_init(struct aic_spinand_bbt *spinand_bbt, u32 block_reserved, u32 offset, u32 size);
int spinand_bbt_read(struct aic_spinand_bbt *spinand_bbt, u8 *data, u32 offset, u32 size);
int spinand_bbt_write(struct aic_spinand_bbt *spinand_bbt, u8 *addr, u32 offset, u32 size);
int spinand_bbt_erase(struct aic_spinand_bbt *spinand_bbt, u32 offset, u32 size);
void spinand_bbt_deinit(struct aic_spinand_bbt *spinand_bbt);
