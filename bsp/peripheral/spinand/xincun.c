/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_XINCUN 0x8C

int xincun_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & STATUS_ECC_MASK) {
        case STATUS_ECC_NO_BITFLIPS:
            return 0;
        case STATUS_ECC_HAS_1_4_BITFLIPS:
            return 4;
        case STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        case STATUS_ECC_MASK:
            return 8;
        default:
            break;
    }

    return -SPINAND_ERR;
}

static int xcsp1a_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 0;
    region->length = 16;

    return 0;
}

const struct aic_spinand_info xincun_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*XCSP1AAPK-IT device*/
    { DEVID(0x01), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "xincun 128MB: 2048+64@64@1024", cmd_cfg_table,
      xincun_ecc_get_status, xcsp1a_ooblayout_user },
};

const struct aic_spinand_info *
xincun_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_XINCUN)
        return NULL;

    return spinand_match_and_init(&id[1], xincun_spinand_table,
                                  ARRAY_SIZE(xincun_spinand_table));
};

static int xincun_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops xincun_spinand_manuf_ops = {
    .detect = xincun_spinand_detect,
    .init = xincun_spinand_init,
};

const struct spinand_manufacturer xincun_spinand_manufacturer = {
    .id = SPINAND_MFR_XINCUN,
    .name = "xincun",
    .ops = &xincun_spinand_manuf_ops,
};
