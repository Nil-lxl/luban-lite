/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <stdio.h>
#include <aic_core.h>
#include <rtdevice.h>
#include <boot_param.h>
#include "aic_common.h"

#ifdef AIC_SPINOR_DRV
#include "spi_flash.h"
#include "spi_flash_sfud.h"
static sfud_flash *sfud_dev = NULL;
static rt_spi_flash_device_t flash_dev = NULL;
#endif
#ifdef AIC_SPINAND_DRV
static struct rt_mtd_nand_device *nand = NULL;
#endif

int flash_init(void)
{
    switch (aic_get_boot_device()) {
        case BD_SPINOR:
#ifdef AIC_SPINOR_DRV
            flash_dev = rt_sfud_flash_probe("norflash0", "qspi01");
            if (!flash_dev) {
                pr_err("sfud probe flash fail!\n");
                return -1;
            }

            sfud_dev = (sfud_flash_t)flash_dev->user_data;
            if (!sfud_dev) {
                pr_err("No flash device selected.\n");
                return -1;
            }
#endif
            break;
        case BD_SPINAND:
#ifdef AIC_SPINAND_DRV
            nand = RT_MTD_NAND_DEVICE(rt_device_find("spl"));
            if (!nand) {
                pr_err("no nand device found!\n");
                return -1;
            }
#endif
            break;
        default:
            break;
    }

    return 0;
}

int flash_read(u32 offset, uint8_t *data, u32 len)
{
    int result = -1;

    switch (aic_get_boot_device()) {
        case BD_SPINOR:
#ifdef AIC_SPINOR_DRV
            if (sfud_dev != NULL) {
                result = sfud_read(sfud_dev, offset, len, data);
            }
#endif
            break;
        case BD_SPINAND:
#ifdef AIC_SPINAND_DRV
            if (nand != NULL) {
                result = rt_mtd_nand_read(nand, offset, data, len, NULL, 0);
            }
#endif
            break;
        default:
            break;
    }

    return result;
}

int flash_write(u32 offset, uint8_t *data, u32 len)
{
    int result = -1;

    switch (aic_get_boot_device()) {
        case BD_SPINOR:
#ifdef AIC_SPINOR_DRV
            if (sfud_dev != NULL) {
                result = sfud_write(sfud_dev, offset, len, data);
            }
#endif
            break;
        case BD_SPINAND:
#ifdef AIC_SPINAND_DRV
            if (nand != NULL) {
                result = rt_mtd_nand_write(nand, offset, data, len, NULL, 0);
            }
#endif
            break;
        default:
            break;
    }

    return result;
}

int flash_erase(u32 offset, u32 len)
{
    int result = -1;

    switch (aic_get_boot_device()) {
        case BD_SPINOR:
#ifdef AIC_SPINOR_DRV
            if (sfud_dev != NULL) {
                result = sfud_erase(sfud_dev, offset, len);
            }
#endif
            break;
        case BD_SPINAND:
#ifdef AIC_SPINAND_DRV
            if (nand != NULL) {
                result = rt_mtd_nand_erase_block(nand, offset);
            }
#endif
            break;
        default:
            break;
    }

    return result;
}
