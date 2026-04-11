/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: mingfeng.li <mingfeng.li@artinchip.com>
 */

#ifndef _NFTL_API_H
#define _NFTL_API_H

#include <aic_common.h>
#include <aic_core.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#define NFTL_BURN 0
#define BARE_PORT_HW_DEBUG 0
#define DRV_PORT_HW_DEBUG 0
#define NFTL_SECTOR_SIZE 512

//this for nand page
struct nand_page{
    u16 page_num;
    u16 block_num;
};

// this is for physicals side
struct physical_op_info{
    struct nand_page physical_page;
    u16 page_bitmap;
    u8 *user_data_addr;
    u8 *spare_data_addr;
};

struct nftl_api_nand_t {
    u16 page_size; /* The Page size in the flash */
    u16 oob_size;  /* Out of bank size */
    u16 oob_free;  /* the free area_node in oob that flash driver not use */
    u16 plane_num; /* the number of plane in the NAND Flash */

    u32 pages_per_block; /* The number of page a block */
    u16 block_total;

    /* Only be touched by driver */
    u32 block_start; /* The start of available block*/
    u32 block_end;   /* The end of available block */
};

struct nftl_api_handler_t {
    void *priv;
    void *priv_mtd;
    struct nftl_api_nand_t *nandt;
};

int weak_debug(void);
int nftl_api_init(struct nftl_api_handler_t *handler, int index);
int nftl_api_write(struct nftl_api_handler_t *handler, u32 start_sector, u32 len, unsigned char *buffer);
int nftl_api_read(struct nftl_api_handler_t *handler, u32 start_sector, u32 len, unsigned char *buffer);
int nftl_api_write_cache(struct nftl_api_handler_t *handler, u32 num);
void nftl_api_print_nftl_area_node(struct nftl_api_handler_t *handler);
void nftl_api_print_free_list(struct nftl_api_handler_t *handler);
void nftl_api_print_block_invalid_list(struct nftl_api_handler_t *handler);
void nftl_api_print_logic_page_map(struct nftl_api_handler_t *handler);
void nftl_api_lib_info(void);
void nftl_spare_info(u8 *buffer);

/**
 * @brief Get the I/O block status of NFTL
 *
 * This function retrieves the current I/O block status flag from the NFTL area node.
 * The I/O block status indicates whether the I/O operations are blocked or allowed.
 *
 * @param handler Pointer to the NFTL API handler structure
 * @return int Returns the I/O block status flag (1 = blocked, 0 = not blocked)
 */
int nftl_api_get_io_block_status(struct nftl_api_handler_t *handler);

/**
 * @brief Set the IO block status for NFTL
 *
 * This function sets the IO block flag for NFTL flash operations.
 * When status is non-zero, IO operations will be blocked; otherwise, they are unblocked.
 *
 * @param handler Pointer to the NFTL API handler structure
 * @param status IO block status (non-zero to block IO, zero to unblock IO)
 *
 * @return None
 */
void nftl_api_set_io_block_status(struct nftl_api_handler_t *handler, int status);

/**
 * @brief Get the OOB verification status from NFTL API handler
 *
 * This function retrieves the current status of Out-Of-Band (OOB) data verification
 * for the NFTL area. The verification status is determined by checking the least
 * significant bit of the verify_enable flag in the I/O control structure.
 *
 * @param handler Pointer to the NFTL API handler structure containing private data
 * @return int Returns 1 if OOB verification is enabled, 0 if disabled
 */
int nftl_api_get_oob_verify_status(struct nftl_api_handler_t *handler);

/**
 * @brief Enable or disable NFTL OOB verification functionality
 *
 * This function controls the Out-of-Band (OOB) data verification feature
 * in the NFTL (NAND Flash Translation Layer) API by setting the verify_enable
 * flag in the io_ctrl structure.
 *
 * @param handler Pointer to the NFTL API handler structure containing private data
 * @param enable Verification enable flag - non-zero to enable, zero to disable
 *
 * @return None
 */

void nftl_api_enable_oob_verify(struct nftl_api_handler_t *handler, int enable);


/* Legacy API for backward compatibility - Not recommended for new usage */

/**
 * @brief Get the I/O block status of NFTL
 *
 * This function retrieves the current I/O block status flag from the NFTL area node.
 * The I/O block status indicates whether the I/O operations are blocked or allowed.
 *
* Note: This is a legacy interface maintained for backward compatibility only.
 *       When multiple NFTL instances exist, this function only returns the status
 *       of the instance that most recently had its status updated.
 *       It is recommended to use nftl_api_get_io_block_status interface in preference to this one.
 *
 * @param handler Pointer to the NFTL API handler structure
 * @return int Returns the I/O block status flag (1 = blocked, 0 = not blocked)
 */
int nftl_api_check_io_error(void);
#endif /*_NFTL_API_H*/
