/*
 * Copyright (c) 2026-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_utils.h>
#include <aic_common.h>
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#elif defined(KERNEL_BAREMETAL)
#include <console.h>
#endif
#include <aic_core.h>
#include "spienc_authorization.h"
#include "test_spienc_authorization.h"


int app_hw_authorization_check(unsigned char *from, int flen,
                               unsigned char *sign_key, int sign_key_len,
                               unsigned char *verify_key, int verify_key_len)
{
    uint8_t *inbuf = NULL, *outbuf = NULL;
    uint8_t sign_key_buf[sign_key_len];
    uint8_t verify_key_buf[verify_key_len];
    size_t pagesize = 4096;
    int ret = 0, rlen;

    inbuf = aicos_malloc_align(0, pagesize, CACHE_LINE_SIZE);
    if (inbuf == NULL) {
        printf("Failed to allocate inbuf.\n");
        ret = -ENOMEM;
        goto out;
    }
    outbuf = aicos_malloc_align(0, pagesize, CACHE_LINE_SIZE);
    if (outbuf == NULL) {
        printf("Failed to allocate outbuf.\n");
        ret = -ENOMEM;
        goto out;
    }

    // 1. Set key parameters
    memcpy(sign_key_buf, sign_key, sign_key_len);
    memcpy(verify_key_buf, verify_key, verify_key_len);

    // 2. Nonce key encryption
    rlen = aic_spie_sk_sign(flen, from, outbuf, sign_key_buf);
    if (rlen < 0) {
        printf("aic_spie_sign failed.\n");
        goto out;
    }
    memcpy(inbuf, outbuf, rlen);
    memset(outbuf, 0, pagesize);

    // 3. EncNonce key decryption
    rlen = aic_spie_vk_verify(rlen, inbuf, outbuf, verify_key_buf);
    if (rlen < 0) {
        printf("aic_spie_verify failed.\n");
        goto out;
    }

    // 4. compare Nonce and DecNonce
    if (memcmp(from, outbuf, rlen)) {
        hexdump_msg("Expect", (unsigned char *)from, rlen, 1);
        hexdump_msg("Got Result", (unsigned char *)outbuf, rlen, 1);
        printf("App stop.\n");
        ret = -1;
    } else {
        printf("App running.\n");
        ret = 0;
    }

out:
    if (outbuf)
        aicos_free_align(0, outbuf);
    if (inbuf)
        aicos_free_align(0, inbuf);

    return ret;
}

int aic_spienc_authorization_test(int argc, char **argv)
{
    int ret = 0;
    unsigned char nonce[16] = { 0 }, nlen = 16;

    while (1) {
        ret = aic_rng_get_bytes(nonce, 16);
        if (ret != nlen)
            pr_err("aic rng get bytes failed.\n");

        ret = app_hw_authorization_check(nonce, nlen, sign_key, sign_key_len, verify_key, verify_key_len);
        if (ret < 0) {
            printf("Application not authorization.\n");
        }

        aic_mdelay(2 * 1000);
    }

    return 0;
}
#if defined(KERNEL_RTTHREAD)
MSH_CMD_EXPORT_ALIAS(aic_spienc_authorization_test, aic_spienc_authorization_test, spienc authorization test);
#elif defined(KERNEL_BAREMETAL)
CONSOLE_CMD(aic_spienc_authorization_test, aic_spienc_authorization_test, "spienc authorization test.");
#endif
