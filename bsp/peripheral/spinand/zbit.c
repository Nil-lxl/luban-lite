/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_ZBIT		0x5E

static int zb35q01a_ecc_get_status(struct aic_spinand *flash, u8 status)
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


static int zb35q0xb_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & STATUS_ECC_MASK) {
        case STATUS_ECC_NO_BITFLIPS:
            return 0;
        case STATUS_ECC_HAS_1_4_BITFLIPS:
            return 8;
        case STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;

        default:
            break;
    }

    return -SPINAND_ERR;
}

static int zb35q0xc_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & STATUS_ECC_MASK) {
        case STATUS_ECC_NO_BITFLIPS:
            return 0;
        case STATUS_ECC_HAS_1_4_BITFLIPS:
            return 4;
        case STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        case STATUS_ECC_HAS_5_8_BITFLIPS:
            return 8;
        default:
            break;
    }

    return -SPINAND_ERR;
}

static int zb35q01a_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 13;
    region->length = 3;

    return 0;
}

static int zb35q01b_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 0;
    region->length = 16;

    return 0;
}

static int zb35q04b_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    /* The ZB35Q04B lacks ECC protection for user metadata. */
    return -SPINAND_ERR;
}

static int zb35q01c_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 0)
      return -SPINAND_ERR;

    region->offset = 0;
    region->length = 51;

    return 0;
}

const struct aic_spinand_info zbit_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*ZB35Q01A*/
    { DEVID(0x41), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "zbit 128MB: 2048+64@64@1024", cmd_cfg_table,
      zb35q01a_ecc_get_status, zb35q01a_ooblayout_user }, //This device has no enough Spare Area for user, we do not support it.
    /*ZB35Q01B*/
    { DEVID(0xa1), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "zbit 128MB: 2048+64@64@1024", cmd_cfg_table,
      zb35q0xb_ecc_get_status, zb35q01b_ooblayout_user, 8 },
    /*ZB35Q02B*/
    { DEVID(0xa2), PAGESIZE(2048), OOBSIZE(64), BPL(2048), PPB(64), PLANENUM(1),
     DIE(0), "zbit 256MB: 2048+64@64@2048", cmd_cfg_table,
     zb35q0xb_ecc_get_status, zb35q01b_ooblayout_user, 8 },
    /*ZB35Q04B*/
    { DEVID(0xa3), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(128), PLANENUM(1),
     DIE(0), "zbit 512MB: 2048+128@128@2048", cmd_cfg_table,
     zb35q01a_ecc_get_status, zb35q04b_ooblayout_user }, //This device has no enough Spare Area for user, we do not support it.
    /*ZB35Q01C*/
    { DEVID(0xc1), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "zbit 128MB: 2048+64@64@1024", cmd_cfg_table,
      zb35q0xc_ecc_get_status, zb35q01c_ooblayout_user, 8 },
    /*ZB35Q02C*/
    { DEVID(0xc2), PAGESIZE(2048), OOBSIZE(64), BPL(2048), PPB(64), PLANENUM(1),
      DIE(0), "zbit 256MB: 2048+64@64@2048", cmd_cfg_table,
      zb35q0xc_ecc_get_status, zb35q01c_ooblayout_user, 8 },
    /*ZB35Q04C*/
    { DEVID(0xc3), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(128), PLANENUM(1),
      DIE(0), "zbit 512MB: 2048+128@128@2048", cmd_cfg_table,
      zb35q0xc_ecc_get_status, zb35q01c_ooblayout_user, 8 },
};

const struct aic_spinand_info *zbit_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_ZBIT)
        return NULL;

    return spinand_match_and_init(&id[1], zbit_spinand_table,
                                  ARRAY_SIZE(zbit_spinand_table));
};

static int zbit_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops zbit_spinand_manuf_ops = {
    .detect = zbit_spinand_detect,
    .init = zbit_spinand_init,
};

const struct spinand_manufacturer zbit_spinand_manufacturer = {
    .id = SPINAND_MFR_ZBIT,
    .name = "zbit",
    .ops = &zbit_spinand_manuf_ops,
};
