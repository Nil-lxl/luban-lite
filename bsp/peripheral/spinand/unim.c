/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_UNIM 0xB0

#define UNIM_STATUS_ECC_MASK        GENMASK(6, 4)
#define UNIM_STATUS_ECC_NO_BITFLIPS (0 << 4)
#define UNIM_STATUS_ECC_CORRECTED   (1 << 4)
#define UNIM_STATUS_ECC_UNCOR_ERROR (2 << 4)
#define UNIM_STATUS_ECC_REFRESH     (3 << 4)
#define UNIM_STATUS_ECC_MANDATORY   (5 << 4)
#define UNIM_ECC_REFRESH_LEVEL      10

static int um19c0hisw_ecc_get_status(struct aic_spinand *flash, u8 status)
{
    switch (status & UNIM_STATUS_ECC_MASK) {
        case UNIM_STATUS_ECC_NO_BITFLIPS:
            return 0;
        case UNIM_STATUS_ECC_CORRECTED:
            return 1;
        case UNIM_STATUS_ECC_UNCOR_ERROR:
            return -SPINAND_ERR_ECC;
        case UNIM_STATUS_ECC_REFRESH:
            return UNIM_ECC_REFRESH_LEVEL;
        case UNIM_STATUS_ECC_MANDATORY:
            return flash->info->ecc_strength;
        default:
            break;
    }

    return -SPINAND_ERR;
}

static int um19c0hisw_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = section * 16;
    region->length = 16;

    return 0;
}

const struct aic_spinand_info unim_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    /*UM19C0HISW*/
    { DEVID(0x1C), PAGESIZE(2048), OOBSIZE(152), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "UM19C0HISW UNIM 128MB: 2048+152@64@1024", cmd_cfg_table,
      um19c0hisw_ecc_get_status, um19c0hisw_ooblayout_user, 13 },
};

const struct aic_spinand_info *unim_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_UNIM)
        return NULL;

    return spinand_match_and_init(&id[1], unim_spinand_table,
                                  ARRAY_SIZE(unim_spinand_table));
};

static int unim_spinand_init(struct aic_spinand *flash)
{
    return 0;
};

static const struct spinand_manufacturer_ops unim_spinand_manuf_ops = {
    .detect = unim_spinand_detect,
    .init = unim_spinand_init,
};

const struct spinand_manufacturer unim_spinand_manufacturer = {
    .id = SPINAND_MFR_UNIM,
    .name = "UNIM",
    .ops = &unim_spinand_manuf_ops,
};
