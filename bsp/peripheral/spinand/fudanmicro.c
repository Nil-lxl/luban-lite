/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_FUDANMICRO 0xA1

#define FM_STATUS_ECC_MASK             GENMASK(6, 4)
#define FM_STATUS_ECC_NO_BITFLIPS      (0)
#define FM_STATUS_ECC_HAS_1_3_BITFLIPS BIT(4)
#define FM_STATUS_ECC_HAS_4_6_BITFLIPS GENMASK(5, 4)
#define FM_STATUS_ECC_HAS_7_8_BITFLIPS (BIT(6) | BIT(4))
#define FM_STATUS_ECC_UNCOR_ERROR      BIT(5)

int fudanmicro_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & FM_STATUS_ECC_MASK) {
        case FM_STATUS_ECC_NO_BITFLIPS:
            return 0;
        case FM_STATUS_ECC_HAS_1_3_BITFLIPS:
            return 3;
        case FM_STATUS_ECC_HAS_4_6_BITFLIPS:
            return 6;
        case FM_STATUS_ECC_HAS_7_8_BITFLIPS:
            return 8;
        case FM_STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        default:
            break;
    }

    return -SPINAND_ERR;
}

static int fm25s01b_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 4;
    region->length = 12;

    return 0;
}

const struct aic_spinand_info fudanmicro_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*FM25S01BI3 device*/
    { DEVID(0xD4), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "fudanmicro 128MB: 2048+128@64@1024", cmd_cfg_table,
      fudanmicro_ecc_get_status, fm25s01b_ooblayout_user },
};

const struct aic_spinand_info *
fudanmicro_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_FUDANMICRO)
        return NULL;

    return spinand_match_and_init(&id[1], fudanmicro_spinand_table,
                                  ARRAY_SIZE(fudanmicro_spinand_table));
};

static int fudanmicro_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops fudanmicro_spinand_manuf_ops = {
    .detect = fudanmicro_spinand_detect,
    .init = fudanmicro_spinand_init,
};

const struct spinand_manufacturer fudanmicro_spinand_manufacturer = {
    .id = SPINAND_MFR_FUDANMICRO,
    .name = "fudanmicro",
    .ops = &fudanmicro_spinand_manuf_ops,
};
