/*
 * Copyright (C) 2020-2024 Artinchip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <errno.h>
#include <aic_core.h>
#include <aic_common.h>
#include "akcipher.h"
#include "authorization.h"
#include "rsa_pad.h"

/*
 * Get PKCS1 ASN1 public key from X509 public key
 * SEQUENCE {
 *     SEQUENCE {
 *         OBJECT IDENTIFIER
 *         rsaEncryption (1 2 840 113549 1 1 1)
 *         NULL
 *     }
 *     BIT STRING, encapsulates {
 *         SEQUENCE {
 *             INTEGER
 *             INTEGER
 *         }
 *     }
 * }
 */
#define TAG_SEQ       0x30
#define TAG_BITSTRING 0x03

static int get_rsa_public_key_from_x509(unsigned char *der,
                                        struct ak_options *opts)
{
    uint8_t val, *p, *pder = der;
    uint32_t len;
    int ret, i, cnt;

    /* First SEQ TAG */
    val = *(pder++);
    opts->pk_len--;
    if (val != TAG_SEQ) {
        ret = -1;
        goto out;
    }
    /* SEQ LENGTH */
    val = *(pder++);
    opts->pk_len--;
    len = 0;
    if (val & 0x80) {
        cnt = val & 0x7f;
        for (i = 0; i < cnt; i++) {
            val = *(pder++);
            opts->pk_len--;
            len |= (val << (i * 8));
        }
    } else {
        len = val;
    }
    /* Skip Second SEQ */
    val = *(pder++);
    opts->pk_len--;
    if (val != TAG_SEQ) {
        ret = -1;
        goto out;
    }
    /* SEQ LENGTH */
    val = *(pder++);
    opts->pk_len--;
    len = 0;
    if (val & 0x80) {
        cnt = val & 0x7f;
        for (i = 0; i < cnt; i++) {
            val = *(pder++);
            opts->pk_len--;
            len |= (val << (i * 8));
        }
    } else {
        len = val;
    }
    pder += len;
    opts->pk_len -= len;

    /* Now should be BIT STRING */
    val = *(pder++);
    opts->pk_len--;
    if (val != TAG_BITSTRING) {
        ret = -1;
        goto out;
    }
    /* BIT STRING LENGTH */
    val = *(pder++);
    opts->pk_len--;
    len = 0;
    if (val & 0x80) {
        cnt = val & 0x7f;
        for (i = 0; i < cnt; i++) {
            val = *(pder++);
            opts->pk_len--;
            len |= (val << (i * 8));
        }
    } else {
        len = val;
    }

    val = *(pder++);
    opts->pk_len--;
    if (val != 0) {
        val = *(pder++);
        opts->pk_len--;
    }
    p = opts->pk_buf;
    memmove(p, pder, opts->pk_len);

    ret = 0;
out:
    return ret;
}

static int aic_priv_enc(int flen, const unsigned char *from, unsigned char *to,
                        struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    unsigned char *buf = NULL;
    size_t pagesize = 2048;
    int ret = 0, maxsize = 0;

    buf = (unsigned char *)aicos_malloc_align(0, pagesize * 2, CACHE_LINE_SIZE);
    if (buf == NULL) {
        pr_err("Failed to allocate buf.\n");
        ret = -ENOMEM;
        goto out;
    }
    memset(buf, 0, pagesize);

    handle = aic_akcipher_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -1;
        goto out;
    }

    maxsize = aic_akcipher_setprivkey(handle, opts->esk_buf, opts->esk_len);
    if (maxsize < flen) {
        pr_err("Asymmetric cipher set private key failed\n");
        ret = -EFAULT;
        goto out;
    }

    ret = rsa_padding_add_pkcs1_type_1(buf, maxsize, from, flen);
    if (ret < 0)
        goto out;

    ret = aic_akcipher_encrypt(handle, buf, (size_t)maxsize, to,
                                   (size_t)maxsize);
    if (ret < 0)
        pr_err("aic priv enc failed.\n");

out:
    if (handle)
        aic_akcipher_destroy(handle);
    if (buf)
        aicos_free_align(0, buf);

    return ret;
}

static int aic_pub_dec(int flen, const unsigned char *from, unsigned char *to,
                       struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    unsigned char *buf = NULL;
    size_t pagesize = 2048;
    int ret = 0, maxsize = 0;

    buf = (unsigned char *)aicos_malloc_align(0, pagesize * 2, CACHE_LINE_SIZE);
    if (buf == NULL) {
        pr_err("Failed to allocate buf.\n");
        ret = -ENOMEM;
        goto out;
    }
    memset(buf, 0, pagesize);

    handle = aic_akcipher_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -1;
        goto out;
    }

    ret = get_rsa_public_key_from_x509(opts->pk_buf, opts);
    if (ret < 0) {
        pr_err("Get rsa public key from x509 failed.\n");
        goto out;
    }

    maxsize = aic_akcipher_setpubkey(handle, opts->pk_buf, opts->pk_len);
    if (maxsize < flen) {
        pr_err("Asymmetric cipher set public key failed\n");
        ret = -EFAULT;
        goto out;
    }

    ret = aic_akcipher_decrypt(handle, from, maxsize, buf, maxsize);
    if (ret < 0) {
        pr_err("aic pub dec failed.\n");
        goto out;
    }

    ret = rsa_padding_check_pkcs1_type_1(to, maxsize, buf, maxsize, maxsize);

out:
    if (handle)
        aic_akcipher_destroy(handle);
    if (buf)
        aicos_free_align(0, buf);

    return ret;
}

static int aic_pub_enc(int flen, const unsigned char *from, unsigned char *to,
                        struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    unsigned char *buf = NULL;
    size_t pagesize = 2048;
    int ret = 0, maxsize = 0;

    buf = (unsigned char *)aicos_malloc_align(0, pagesize * 2, CACHE_LINE_SIZE);
    if (buf == NULL) {
        pr_err("Failed to allocate buf.\n");
        ret = -ENOMEM;
        goto out;
    }
    memset(buf, 0, pagesize);

    handle = aic_akcipher_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -1;
        goto out;
    }

    ret = get_rsa_public_key_from_x509(opts->pk_buf, opts);
    if (ret < 0) {
        pr_err("Get rsa public key from x509 failed.\n");
        goto out;
    }

    maxsize = aic_akcipher_setpubkey(handle, opts->pk_buf, opts->pk_len);
    if (maxsize < flen) {
        pr_err("Asymmetric cipher set public key failed\n");
        ret = -EFAULT;
        goto out;
    }

    ret = rsa_padding_add_pkcs1_type_2(buf, maxsize, from, flen);
    if (ret < 0)
        goto out;

    ret = aic_akcipher_encrypt(handle, buf, (size_t)maxsize, to,
                                   (size_t)maxsize);
    if (ret < 0)
        pr_err("aic priv enc failed.\n");

out:
    if (handle)
        aic_akcipher_destroy(handle);
    if (buf)
        aicos_free_align(0, buf);

    return ret;
}

static int aic_priv_dec(int flen, const unsigned char *from, unsigned char *to,
                       struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    unsigned char *buf = NULL;
    size_t pagesize = 2048;
    int ret = 0, maxsize = 0;

    buf = (unsigned char *)aicos_malloc_align(0, pagesize * 2, CACHE_LINE_SIZE);
    if (buf == NULL) {
        pr_err("Failed to allocate buf.\n");
        ret = -ENOMEM;
        goto out;
    }
    memset(buf, 0, pagesize);

    handle = aic_akcipher_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -1;
        goto out;
    }

    maxsize = aic_akcipher_setprivkey(handle, opts->esk_buf, opts->esk_len);
    if (maxsize < flen) {
        pr_err("Asymmetric cipher set private key failed\n");
        ret = -EFAULT;
        goto out;
    }

    ret = aic_akcipher_decrypt(handle, from, maxsize, buf, maxsize);
    if (ret < 0) {
        pr_err("aic pub dec failed.\n");
        goto out;
    }

    ret = rsa_padding_check_pkcs1_type_2(to, maxsize, buf, maxsize, maxsize);

out:
    if (handle)
        aic_akcipher_destroy(handle);
    if (buf)
        aicos_free_align(0, buf);

    return ret;
}

int aic_rsa_priv_enc(int flen, unsigned char *from, unsigned char *to,
                     struct ak_options *opts)
{
    return aic_priv_enc(flen, from, to, opts, "rsa");
}

int aic_rsa_pub_dec(int flen, unsigned char *from, unsigned char *to,
                    struct ak_options *opts)
{
    return aic_pub_dec(flen, from, to, opts, "rsa");
}

int aic_rsa_pub_enc(int flen, unsigned char *from, unsigned char *to,
                     struct ak_options *opts)
{
    return aic_pub_enc(flen, from, to, opts, "rsa");
}

int aic_rsa_priv_dec(int flen, unsigned char *from, unsigned char *to,
                    struct ak_options *opts)
{
    return aic_priv_dec(flen, from, to, opts, "rsa");
}

int aic_hwp_rsa_priv_enc(int flen, unsigned char *from, unsigned char *to,
                         struct ak_options *opts, char *algo)
{
    return aic_priv_enc(flen, from, to, opts, algo);
}

int aic_hwp_rsa_priv_dec(int flen, unsigned char *from, unsigned char *to,
                         struct ak_options *opts, char *algo)
{
    return aic_priv_dec(flen, from, to, opts, algo);
}
