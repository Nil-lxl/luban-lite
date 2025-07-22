/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../inc/sfud.h"
#include <string.h>
#include "sfud_flash_def.h"
#include <stdlib.h>

static const sfud_qspi_flash_private_info qspi_flash_private_info_table[] = SFUD_FLASH_PRIVATE_INFO_TABLE;

int spi_nor_get_unique_id(sfud_flash *flash, uint8_t *data)
{
    uint8_t id_len = 16;
    int ret = 0;
    uint8_t *send_byte;

    send_byte = malloc(5 * sizeof(uint8_t));
    if (!send_byte) {
        SFUD_INFO("Malloc failed!\n");
        return -1;
    }

    /*
     * Default situation of readding Unique ID Number.
     * SI byte0: instruction 0x4b
     * SI byte1 - byte4: 4 bytes dummy
     * SO 128-bit Unique Serial Number
     * */
    memset(send_byte, 0xff, 5 * sizeof(uint8_t));
    send_byte[0] = 0x4b;

    for (int i = 0;
        i < sizeof(qspi_flash_private_info_table) / sizeof(sfud_qspi_flash_private_info); i++) {
       if ((qspi_flash_private_info_table[i].mf_id == flash->chip.mf_id) &&
           (qspi_flash_private_info_table[i].type_id == flash->chip.type_id) &&
           (qspi_flash_private_info_table[i].capacity_id ==
            flash->chip.capacity_id)) {
            send_byte[0] = qspi_flash_private_info_table[i].rd_cmd;
            send_byte[1] = qspi_flash_private_info_table[i].byte1;
            send_byte[2] = qspi_flash_private_info_table[i].byte2;
            send_byte[3] = qspi_flash_private_info_table[i].byte3;
            send_byte[4] = qspi_flash_private_info_table[i].byte4;
            id_len = qspi_flash_private_info_table[i].id_len;
            break;
       }
   }

    ret = sfud_read_unique_id(flash, send_byte, id_len, data);
    free (send_byte);

    return ret;
}
