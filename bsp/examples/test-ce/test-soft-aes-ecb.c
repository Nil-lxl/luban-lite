/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author          Notes
 * 2018-08-15   misonyo         first implementation.
 * 2023-05-25   geo.dong        ArtInChip
 */

#include <string.h>
#include <rtthread.h>
#include <aic_core.h>
#include "soft-aes-ecb.h"

#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

static void dump_hex(const unsigned char *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;

    for (i = 0; i < buflen; i += 16) {
        rt_kprintf("%08X: ", i);

        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                rt_kprintf("%02X ", buf[i + j]);
            } else {
                rt_kprintf("   ");
            }
        }
        rt_kprintf(" ");

        for (j = 0; j < 16; j++) {
            if (i + j < buflen) {
                rt_kprintf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
}


/* key */
static unsigned char cryp_key[16] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
    0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
};

static unsigned char data[16] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
    0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF
};

static int test_soft_aes(int argc, char **argv)
{
	struct aes_ctx ctx;

	aes_init_ctx(&ctx, cryp_key);
    aes_128_ecb_encrypt(&ctx, data);
    dump_hex(data, 16);
    aes_128_ecb_decrypt(&ctx, data);
    dump_hex(data, 16);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(test_soft_aes, test_aes_ecb, Test soft aes ecb command);
