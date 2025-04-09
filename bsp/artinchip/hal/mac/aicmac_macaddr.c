/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aicmac_macaddr.h"
#include "hal_efuse.h"
#include "aic_log.h"
#include <string.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "md5.h"
#define AICMAC_CHIPID_LENGTH    8


static int aicmac_efuse_read(u32 addr, void *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    u8 *pd, *pw;
    int ret;

    if (hal_efuse_clk_enable()) {
        return -1;
    }

    if (hal_efuse_wait_ready()) {
        pr_err("eFuse is not ready.\n");
        return -1;
    }

    pd = data;
    rest = size;
    while (rest > 0) {
        wid = addr >> 2;
        ret = hal_efuse_read(wid, &wval);
        if (ret)
            break;
        pw = (u8 *)&wval;
        cnt = rest;
        if (addr % 4) {
            if (rest > (4 - (addr % 4)))
                cnt = (4 - (addr % 4));
            memcpy(pd, pw + (addr % 4), cnt);
        } else {
            if (rest > 4)
                cnt = 4;
            memcpy(pd, pw, cnt);
        }
        pd += cnt;
        addr += cnt;
        rest -= cnt;
    }

    hal_efuse_clk_disable();

    return (int)(size - rest);
}

static inline int aicmac_get_chipid(unsigned char out_chipid[AICMAC_CHIPID_LENGTH])
{
    return aicmac_efuse_read(0x10, out_chipid, AICMAC_CHIPID_LENGTH);
}

void aicmac_get_macaddr_from_chipid(int port, unsigned char out_addr[6])
{
    unsigned char hex_chipid[AICMAC_CHIPID_LENGTH] = { 0 };
    char char_chipid[AICMAC_CHIPID_LENGTH * 2 + 1] = { 0 };
    uint8_t md5_ahash[16] = { 0 };
    int i;

    if (!aicmac_get_chipid(hex_chipid))
        return;

    for (i = 0; i < AICMAC_CHIPID_LENGTH ; i++) {
        sprintf(&char_chipid[i * 2], "%02X", hex_chipid[i]);
    }

    if (port)
        char_chipid[15] = 'a';
    else
        char_chipid[15] = 'A';

    MD5Buffer(char_chipid, 16, md5_ahash);

    /* Choose md5 result's [0][2][4][6][8][10] byte as mac address */
    for (i = 0; i < 6; i++)
        out_addr[i] = md5_ahash[2 * i];

    out_addr[0] &= 0xfe; /* clear multicast bit */
    out_addr[0] |= 0x02; /* set local assignment bit (IEEE802) */
}
