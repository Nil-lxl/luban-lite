/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_UMTEK		0x52

static int gss01ga_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & STATUS_ECC_MASK) {
        case STATUS_ECC_NO_BITFLIPS:
            return 0;
        case STATUS_ECC_HAS_1_4_BITFLIPS:
            return 4;
        case STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        default:
            break;
    }

    return -SPINAND_ERR;
}

static int gss01ga_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 0)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 0;
    region->length = 32;

    return 0;
}

const struct aic_spinand_info umtek_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*GSS01GAK1*/
    { DEVID(0xBA), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "umtek 128MB: 2048+64@64@1024", cmd_cfg_table,
      gss01ga_ecc_get_status, gss01ga_ooblayout_user },
};

const struct aic_spinand_info *umtek_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_UMTEK)
        return NULL;

    return spinand_match_and_init(&id[1], umtek_spinand_table,
                                  ARRAY_SIZE(umtek_spinand_table));
};

static int umtek_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops umtek_spinand_manuf_ops = {
    .detect = umtek_spinand_detect,
    .init = umtek_spinand_init,
};

const struct spinand_manufacturer umtek_spinand_manufacturer = {
    .id = SPINAND_MFR_UMTEK,
    .name = "umtek",
    .ops = &umtek_spinand_manuf_ops,
};
