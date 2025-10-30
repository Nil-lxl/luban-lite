/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <chen.jiji@artinchip.com>
 */

#include "lx_api.h"
#include <stdio.h>
#include <aic_common.h>

/* This configuration is for one physical sector of overhead.  */
#define PHYSICAL_SECTORS_PER_BLOCK          8          /* Min value of 2, max value of 120 for 1 sector of overhead.  */
#define WORDS_PER_PHYSICAL_SECTOR           128
#define FREE_BIT_MAP_WORDS                  ((PHYSICAL_SECTORS_PER_BLOCK-1)/32)+1
#define USABLE_SECTORS_PER_BLOCK            (PHYSICAL_SECTORS_PER_BLOCK-1)
#define UNUSED_METADATA_WORDS_PER_BLOCK     (WORDS_PER_PHYSICAL_SECTOR-(3+FREE_BIT_MAP_WORDS+USABLE_SECTORS_PER_BLOCK))

typedef struct PHYSICAL_SECTOR_STRUCT
{
    unsigned long memory[WORDS_PER_PHYSICAL_SECTOR];
} PHYSICAL_SECTOR;


typedef struct FLASH_BLOCK_STRUCT
{
    unsigned long       erase_count;
    unsigned long       min_log_sector;
    unsigned long       max_log_sector;
    unsigned long       free_bit_map[FREE_BIT_MAP_WORDS];
    unsigned long       sector_metadata[USABLE_SECTORS_PER_BLOCK];
    unsigned long       unused_words[UNUSED_METADATA_WORDS_PER_BLOCK];
    PHYSICAL_SECTOR     physical_sectors[USABLE_SECTORS_PER_BLOCK];
} FLASH_BLOCK;

ULONG         nor_sector_memory[WORDS_PER_PHYSICAL_SECTOR];

UINT  aic_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash);
UINT  aic_lx_nor_flash_driver_erase_all(VOID);
#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  aic_lx_nor_flash_driver_read(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *destination, ULONG words);
UINT  aic_lx_nor_flash_driver_write(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *source, ULONG words);
UINT  aic_lx_nor_flash_driver_block_erase(LX_NOR_FLASH *nor_flash, ULONG block, ULONG erase_count);
UINT  aic_lx_nor_flash_driver_block_erased_verify(LX_NOR_FLASH *nor_flash, ULONG block);
UINT  aic_lx_nor_flash_driver_system_error(LX_NOR_FLASH *nor_flash, UINT error_code, ULONG block, ULONG sector);
#else
UINT  aic_lx_nor_flash_driver_read(ULONG *flash_address, ULONG *destination, ULONG words);
UINT  aic_lx_nor_flash_driver_write(ULONG *flash_address, ULONG *source, ULONG words);
UINT  aic_lx_nor_flash_driver_block_erase(ULONG block, ULONG erase_count);
UINT  aic_lx_nor_flash_driver_block_erased_verify(ULONG block);
UINT  aic_lx_nor_flash_driver_system_error(UINT error_code, ULONG block, ULONG sector);
#endif

UINT  aic_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash)
{

    /* Setup the base address of the flash memory.  */
    nor_flash -> lx_nor_flash_base_address =                (ULONG *)0;

    if (!nor_flash -> mtd_device) {
        printf("[ERROR] mtd_get_device failer.\n");
        return(LX_ERROR);
    }

    /* Setup geometry of the flash.  */
    nor_flash -> lx_nor_flash_total_blocks =                nor_flash->mtd_device->size / nor_flash->mtd_device->erasesize;
    nor_flash -> lx_nor_flash_words_per_block =             nor_flash->mtd_device->erasesize / sizeof(ULONG);

    /* Setup function pointers for the NOR flash services.  */
    nor_flash -> lx_nor_flash_driver_read =                 aic_lx_nor_flash_driver_read;
    nor_flash -> lx_nor_flash_driver_write =                aic_lx_nor_flash_driver_write;
    nor_flash -> lx_nor_flash_driver_block_erase =          aic_lx_nor_flash_driver_block_erase;
    nor_flash -> lx_nor_flash_driver_block_erased_verify =  aic_lx_nor_flash_driver_block_erased_verify;

    /* Setup local buffer for NOR flash operation. This buffer must be the sector size of the NOR flash memory.  */
    nor_flash -> lx_nor_flash_sector_buffer =  &nor_sector_memory[0];

    if (nor_flash->blk_buf == NULL)
        nor_flash->blk_buf = (UCHAR *)malloc(nor_flash->mtd_device->erasesize);

    if (nor_flash->blk_buf == NULL) {
        printf("[ERROR] malloc failed.\n");
        return(LX_ERROR);
    }

    /* Return success.  */
    return(LX_SUCCESS);
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  aic_lx_nor_flash_driver_read(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *destination, ULONG words)
#else
UINT  aic_lx_nor_flash_driver_read(ULONG *flash_address, ULONG *destination, ULONG words)
#endif
{
    struct mtd_dev *mtd;
    UINT ret;

    mtd = nor_flash->mtd_device;

    ret = mtd_read(mtd, (UINT)(flash_address), (UCHAR *)destination, words * sizeof(UINT));
    if (ret) {
        printf("[ERROR:] Mtd read data failed! addr: %d\n", (UINT)(flash_address));
        return(LX_ERROR);
    }

    return(LX_SUCCESS);
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  aic_lx_nor_flash_driver_write(LX_NOR_FLASH *nor_flash, ULONG *flash_address, ULONG *source, ULONG words)
#else
UINT  aic_lx_nor_flash_driver_write(ULONG *flash_address, ULONG *source, ULONG words)
#endif
{
    struct mtd_dev *mtd;
    UINT ret;
    UINT  page_addr;

    mtd = nor_flash->mtd_device;

    if (words != 1) {
        /* Write 1 sector. */
        if (words * sizeof(UINT) != 512) {
            printf("[ERROR:] Write size unspect %lu\n", words * sizeof(UINT));
            return(LX_ERROR);
        }

        memcpy(nor_flash->blk_buf, source, words * sizeof(UINT));
        ret = mtd_write(mtd, (UINT)flash_address, nor_flash->blk_buf, words * sizeof(UINT));
    } else {
        /* Write 1 word, read, modify and write for 1 sector. */
        page_addr = ((UINT)flash_address / mtd->erasesize) * mtd->erasesize;
        ret = mtd_read(mtd, page_addr, nor_flash->blk_buf, LX_NOR_SECTOR_SIZE * sizeof(UINT));
        if (ret)
            printf("[ERROR:]mtd_read\n");

        memcpy((nor_flash->blk_buf + (UINT)flash_address - page_addr), source, words * sizeof(UINT));
        ret = mtd_write(mtd, page_addr, nor_flash->blk_buf, LX_NOR_SECTOR_SIZE * sizeof(UINT));
    }

    if (ret) {
        printf("[ERROR:] Mtd write data failed!\n");
        return(LX_ERROR);
    }

    return(LX_SUCCESS);
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  aic_lx_nor_flash_driver_block_erase(LX_NOR_FLASH *nor_flash, ULONG block, ULONG erase_count)
#else
UINT  aic_lx_nor_flash_driver_block_erase(ULONG block, ULONG erase_count)
#endif
{
    struct mtd_dev *mtd;
    int ret;

    mtd = nor_flash->mtd_device;

    ret = mtd_erase(mtd, block * mtd->erasesize, mtd->erasesize);
    if (ret) {
        printf("[ERROR] erase block %lu failed.", block);
    }

    return(LX_SUCCESS);
}


UINT  aic_lx_nor_flash_driver_erase_all(VOID)
{
    /* Do not work. */
    return(LX_SUCCESS);
}


#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  aic_lx_nor_flash_driver_block_erased_verify(LX_NOR_FLASH *nor_flash, ULONG block)
#else
UINT  aic_lx_nor_flash_driver_block_erased_verify(ULONG block)
#endif
{
    /* Return success.  */
    return(LX_SUCCESS);
}

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT  aic_lx_nor_flash_driver_system_error(LX_NOR_FLASH *nor_flash, UINT error_code, ULONG block, ULONG sector)
#else
UINT  aic_lx_nor_flash_driver_system_error(UINT error_code, ULONG block, ULONG sector)
#endif
{
    printf("%s %d\n", __func__, error_code);

#ifdef LX_NOR_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nor_flash);
#endif
    LX_PARAMETER_NOT_USED(error_code);
    LX_PARAMETER_NOT_USED(block);
    LX_PARAMETER_NOT_USED(sector);

    /* Custom processing goes here...  all errors are fatal.  */
    return(LX_ERROR);
}
