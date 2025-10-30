/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include "inc/spinand.h"
#include "inc/manufacturer.h"

#define SPINAND_MFR_WINBOND 0xEF

#define WINBOND_CFG_BUF_READ BIT(3)

int winbond_ecc_get_status(struct aic_spinand *flash, u8 status)
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

static int w25n01gv_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 4;
    region->length = 4;

    return 0;
}

static int w25n02kv_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 4;
    region->length = 12;

    return 0;
}

static int w25n01jw_ooblayout_user(struct aic_spinand *flash, int section,
                            struct aic_oob_region *region)
{
    if (section > 3)
      return -SPINAND_ERR;

    region->offset = (16 * section) + 8;
    region->length = 4;

    return 0;
}


const struct aic_spinand_info winbond_spinand_table[] = {
    /*devid page_size oob_size block_per_lun pages_per_eraseblock planes_per_lun
    is_die_select*/
    { DEVID(0xAA, 0x21), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "W25N01GVxxxG/T/R Winbond 128MB: 2048+64@64@1024", cmd_cfg_table,
      winbond_ecc_get_status, w25n01gv_ooblayout_user },
    { DEVID(0xAA, 0x22), PAGESIZE(2048), OOBSIZE(128), BPL(2048), PPB(64), PLANENUM(1),
      DIE(0), "W25N02KVxxIR/U Winbond 256MB: 2048+128@64@2048", cmd_cfg_table,
      winbond_ecc_get_status, w25n02kv_ooblayout_user },
    { DEVID(0xAA, 0x23), PAGESIZE(2048), OOBSIZE(128), BPL(4096), PPB(64), PLANENUM(1),
      DIE(0), "W25N04KVxxIR/U Winbond 512MB: 2048+128@64@4096", cmd_cfg_table,
      winbond_ecc_get_status, w25n02kv_ooblayout_user },
    { DEVID(0xBF, 0x22), PAGESIZE(2048), OOBSIZE(64), BPL(2048), PPB(64), PLANENUM(1),
      DIE(0), "W25N02JWxxxF/C Winbond 256MB: 2048+64@64@2048", cmd_cfg_table,
      NULL, w25n01jw_ooblayout_user },  /* 1.8V */
    { DEVID(0xAB, 0x21), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "W25M02GV Winbond 256MB: 2048+64@64@1024, MCP", cmd_cfg_table,
      winbond_ecc_get_status, w25n01gv_ooblayout_user },
    { DEVID(0xBA, 0x22), PAGESIZE(2048), OOBSIZE(128), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "W25N02KW Winbond 128MB: 2048+128@64@1024", cmd_cfg_table,
      NULL, w25n02kv_ooblayout_user },  /* 1.8V */
    { DEVID(0xAE, 0x21), PAGESIZE(2048), OOBSIZE(64), BPL(1024), PPB(64), PLANENUM(1),
      DIE(0), "W25N01KVxxxE Winbond 128MB: 2048+64@64@1024", cmd_cfg_table,
      NULL, w25n02kv_ooblayout_user },
};

const struct aic_spinand_info *winbond_spinand_detect(struct aic_spinand *flash)
{
    u8 *id = flash->id.data;

    if (id[0] != SPINAND_MFR_WINBOND)
        return NULL;

    return spinand_match_and_init(&id[1], winbond_spinand_table,
                                  ARRAY_SIZE(winbond_spinand_table));
};

static int winbond_spinand_init(struct aic_spinand *flash)
{
    return spinand_config_set(flash, WINBOND_CFG_BUF_READ,
                              WINBOND_CFG_BUF_READ);
};

static const struct spinand_manufacturer_ops winbond_spinand_manuf_ops = {
    .detect = winbond_spinand_detect,
    .init = winbond_spinand_init,
};

const struct spinand_manufacturer winbond_spinand_manufacturer = {
    .id = SPINAND_MFR_WINBOND,
    .name = "Winbond",
    .ops = &winbond_spinand_manuf_ops,
};
