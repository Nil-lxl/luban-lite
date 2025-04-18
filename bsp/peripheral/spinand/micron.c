/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_MICRON 0x2c

#define MICRON_STATUS_ECC_MASK             GENMASK(6, 4)
#define MICRON_STATUS_ECC_NO_BITFLIPS      (0)
#define MICRON_STATUS_ECC_HAS_1_3_BITFLIPS BIT(4)
#define MICRON_STATUS_ECC_UNCOR_ERROR      BIT(5)
#define MICRON_STATUS_ECC_HAS_4_6_BITFLIPS GENMASK(5, 4)
#define MICRON_STATUS_ECC_HAS_7_8_BITFLIPS (BIT(4) | BIT(6))

static int mt29f1g01_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & MICRON_STATUS_ECC_MASK) {
        case MICRON_STATUS_ECC_NO_BITFLIPS:
            return 0;
        case MICRON_STATUS_ECC_HAS_1_3_BITFLIPS:
            return 3;
        case MICRON_STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        case MICRON_STATUS_ECC_HAS_4_6_BITFLIPS:
            return 6;
        case MICRON_STATUS_ECC_HAS_7_8_BITFLIPS:
            return 8;
        default:
            break;
    }

    return -SPINAND_ERR;
}

static int mt29f1g_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (8 * section) + 32;
    region->length = 8;

    return 0;
}

const struct aic_spinand_info micron_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*MT29F1G01ABAFD*/
    { DEVID(0x14), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64),
      PLANENUM(1), DIE(0), "micron 128MB: 2048+128@64@1024", cmd_cfg_table,
      mt29f1g01_ecc_get_status, mt29f1g_ooblayout_user},
    /*MT29F2G01ABAGD*/
    /*ZD35Q2GC-IB*/
    /*XT26G02E*/
    { DEVID(0x24), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(64),
      PLANENUM(2), DIE(0), "micron 256MB: 2048+128@64@2048", cmd_cfg_table,
      mt29f1g01_ecc_get_status, mt29f1g_ooblayout_user },
};

const struct aic_spinand_info *micron_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_MICRON)
        return NULL;

    return spinand_match_and_init(&id[1], micron_spinand_table,
                                  ARRAY_SIZE(micron_spinand_table));
};

static int micron_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops micron_spinand_manuf_ops = {
    .detect = micron_spinand_detect,
    .init = micron_spinand_init,
};

const struct spinand_manufacturer micron_spinand_manufacturer = {
    .id = SPINAND_MFR_MICRON,
    .name = "micron",
    .ops = &micron_spinand_manuf_ops,
};
