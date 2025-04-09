/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
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
#include "authorization.h"
#include "rng.h"
#include "test_aic_hw_authorization.h"

int app_hw_authorization_check(unsigned char *from, int flen,
                               unsigned char *esk, int esk_len,
                               unsigned char *pk, int pk_len, char *algo)
{
    struct ak_options opts = { 0 };
    uint8_t *inbuf = NULL, *outbuf = NULL;
    uint8_t esk_buf[esk_len];
    uint8_t pk_buf[pk_len];
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

    // 1. Set RSA key parameters
    memcpy(esk_buf, esk, esk_len);
    memcpy(pk_buf, pk, pk_len);
    opts.esk_buf = esk_buf;
    opts.esk_len = esk_len;
    opts.pk_buf = pk_buf;
    opts.pk_len = pk_len;

#if 1
    // 2. Nonce private key encryption
    rlen = aic_hwp_rsa_priv_enc(flen, from, outbuf, &opts, algo);
    if (rlen < 0) {
        printf("aic_hwp_rsa_priv_enc failed.\n");
        goto out;
    }
    memcpy(inbuf, outbuf, rlen);
    memset(outbuf, 0, pagesize);

    // 3. EncNonce public key decryption
    rlen = aic_rsa_pub_dec(rlen, inbuf, outbuf, &opts);
    if (rlen < 0) {
        printf("aic_rsa_pub_dec failed.\n");
        goto out;
    }
#else
    // 2. Nonce public key encryption
    rlen = aic_rsa_pub_enc(flen, from, outbuf, &opts);
    if (rlen < 0) {
        printf("aic_rsa_pub_enc failed.\n");
        goto out;
    }
    memcpy(inbuf, outbuf, rlen);
    memset(outbuf, 0, 2 * pagesize);

    // 3. EncNonce private key decryption
    rlen = aic_hwp_rsa_priv_dec(rlen, inbuf, outbuf, &opts, algo);
    if (rlen < 0) {
        printf("aic_hwp_rsa_priv_dec failed.\n");
        goto out;
    }
#endif

    // 4. compare Nonce and DecNonce
    if (memcmp(from, outbuf, rlen)) {
        hexdump_msg("Expect", (unsigned char *)from, rlen, 1);
        hexdump_msg("Got Result", (unsigned char *)outbuf, rlen, 1);
        printf("App %s stop.\n", algo);
        ret = -1;
    } else {
        printf("App %s running.\n", algo);
        ret = 0;
    }

out:
    if (outbuf)
        aicos_free_align(0, outbuf);
    if (inbuf)
        aicos_free_align(0, inbuf);

    return ret;
}

int aic_hw_authorization_test(int argc, char **argv)
{
    int ret = 0;
    int esk_len, pk_len;
    unsigned char *esk, *pk, nonce[16] = { 0 }, nlen = 16;
    char *algo;

    esk = rsa_private_key2048_encrypted_der;
    esk_len = rsa_private_key2048_encrypted_der_len;
    pk = rsa_public_key2048_der;
    pk_len = rsa_public_key2048_der_len;
    while (1) {
        ret = aic_rng_get_bytes(nonce, 16);
        if (ret != nlen)
            pr_err("aic rng get bytes failed.\n");

        algo = PNK_PROTECTED_RSA;
        ret = app_hw_authorization_check(nonce, nlen, esk, esk_len, pk, pk_len, algo);
        if (ret < 0) {
            printf("Application %s not authorization.\n", algo);
        }

        ret = aic_rng_get_bytes(nonce, 16);
        if (ret != nlen)
            pr_err("aic rng get bytes failed.\n");

        algo = PSK0_PROTECTED_RSA;
        ret = app_hw_authorization_check(nonce, nlen, esk, esk_len, pk, pk_len, algo);
        if (ret < 0) {
            printf("Application %s not authorization.\n", algo);
        }

        ret = aic_rng_get_bytes(nonce, 16);
        if (ret != nlen)
            pr_err("aic rng get bytes failed.\n");

        algo = PSK1_PROTECTED_RSA;
        ret = app_hw_authorization_check(nonce, nlen, esk, esk_len, pk, pk_len, algo);
        if (ret < 0) {
            printf("Application %s not authorization.\n", algo);
        }

        ret = aic_rng_get_bytes(nonce, 16);
        if (ret != nlen)
            pr_err("aic rng get bytes failed.\n");

        algo = PSK2_PROTECTED_RSA;
        ret = app_hw_authorization_check(nonce, nlen, esk, esk_len, pk, pk_len, algo);
        if (ret < 0) {
            printf("Application %s not authorization.\n", algo);
        }

        ret = aic_rng_get_bytes(nonce, 16);
        if (ret != nlen)
            pr_err("aic rng get bytes failed.\n");

        algo = PSK3_PROTECTED_RSA;
        ret = app_hw_authorization_check(nonce, nlen, esk, esk_len, pk, pk_len, algo);
        if (ret < 0) {
            printf("Application %s not authorization.\n", algo);
        }

        aic_mdelay(2 * 1000);
    }

    return 0;
}
#if defined(KERNEL_RTTHREAD)
MSH_CMD_EXPORT_ALIAS(aic_hw_authorization_test, aic_hw_authorization_test, hardware authorization test);
#elif defined(KERNEL_BAREMETAL)
CONSOLE_CMD(aic_hw_authorization_test, aic_hw_authorization_test, "hardware authorization test.");
#endif
