/*
 * Copyright (C) 2020-2026 Artinchip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <errno.h>
#include <aic_core.h>
#include <aic_common.h>
#include <hal_ce.h>
#include "hash.h"
#include "akcipher.h"
#include "authorization.h"

#define PUTU32(p,V) \
    ((p)[0] = (uint8_t)((V) >> 24), \
     (p)[1] = (uint8_t)((V) >> 16), \
     (p)[2] = (uint8_t)((V) >>  8), \
     (p)[3] = (uint8_t)(V))

static u32 sm2_rand_k[8] = {
    0x276E2759, 0x1A8606D5, 0x3A0F6816, 0xCC2DC0D9,
    0xFAC13CEF, 0xCEE4DB3C, 0x0DB8546D, 0x21BCC1EA};

static void gmssl_memxor(void *r, const void *a, const void *b, u32 len)
{
	u8 *pr = r;
	const u8 *pa = a;
	const u8 *pb = b;
	u32 i;

	for (i = 0; i < len; i++) {
		pr[i] = pa[i] ^ pb[i];
	}
}

static int aic_hash(char *cipher_name, u8 *data, u32 dlen, u8 *digest)
{
    struct aic_hash_handle *handle = NULL;
    u32 olen = CE_MAX_DIGEST_SIZE;
    int ret = -1;

    handle = aic_md_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -ENOMEM;
        goto out;
    }

    ret = aic_md_update(handle, data, dlen);
    if (ret) {
        pr_err("Update failed\n");
        goto out;
    }

    ret = aic_md_final(handle, digest, olen);
    if (ret <= 0) {
        pr_err("Get digest failed\n");
        goto out;
    }
    ret = 0;

out:
    if (handle)
        aic_md_destroy(handle);

    return ret;
}

static int sm2_kdf(const u8 *in, u32 inlen, u32 outlen, u8 *out)
{
	u8 counter_be[4], dgst[32], buf[128];
	u32 counter = 1;
	u32 len;

	memcpy(buf, in, inlen);
	while (outlen) {
		PUTU32(counter_be, counter);
		memcpy(&buf[inlen], counter_be, sizeof(counter_be));
		counter++;

		aic_hash("sm3", buf, inlen + sizeof(counter_be), dgst);

		len = outlen < 32 ? outlen : 32;
		memcpy(out, dgst, len);
		out += len;
		outlen -= len;
	}

	return 0;
}

static int aic_priv_enc(int flen, const u8 *from, u8 *to,
                        struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    int ret = 0, maxsize = 0;

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

    ret = aic_akcipher_sign(handle, from, (size_t)flen, to, (size_t)128);
    if (ret < 0)
        pr_err("aic sm2 sign failed.\n");

out:
    if (handle)
        aic_akcipher_destroy(handle);

    return ret;
}

static int aic_pub_dec(int flen, const u8 *from, u8 *to,
                       struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    int ret = 0, maxsize = 0;

    handle = aic_akcipher_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -1;
        goto out;
    }

    maxsize = aic_akcipher_setpubkey(handle, opts->pk_buf, opts->pk_len);
    if (maxsize < flen) {
        pr_err("Asymmetric cipher set public key failed\n");
        ret = -EFAULT;
        goto out;
    }

    ret = aic_akcipher_verify(handle, from, flen, to, 128);
    if (ret < 0) {
        pr_err("aic sm2 verify failed.\n");
        goto out;
    }

out:
    if (handle)
        aic_akcipher_destroy(handle);

    return ret;
}

static int aic_pub_enc(int flen, const u8 *from, u8 *to,
                       struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    int ret = 0, maxsize = 0;
    u8 x1y1x2y2[128], x2y2[64] __attribute__((aligned(64)));
    u8 x2my2[128], dgst[32] __attribute__((aligned(64)));
    u8 t[flen];
    u8 *pout = to;

    handle = aic_akcipher_init(cipher_name, 0);
    if (handle == NULL) {
        pr_err("Allocation of %s cipher failed\n", cipher_name);
        ret = -1;
        goto out;
    }

    maxsize = aic_akcipher_setpubkey(handle, opts->pk_buf, opts->pk_len);
    if (maxsize < flen) {
        pr_err("Asymmetric cipher set public key failed\n");
        ret = -EFAULT;
        goto out;
    }

    ret = aic_akcipher_encrypt(handle, (const u8 *)sm2_rand_k, 0x20, x1y1x2y2, 0x80);
    if (ret < 0) {
        pr_err("aic pub enc failed.\n");
        goto out;
    }

    // output C1 = k * G = (x1, y1)
    memcpy(pout, x1y1x2y2, 64);

    // output C3 = Hash(x2 || m || y2)
    memcpy(x2y2, &x1y1x2y2[64], 64);
    memcpy(x2my2, x2y2, 32);
    memcpy(&x2my2[32], from, flen);
    memcpy(&x2my2[32 +  flen], &x2y2[32], 32);
    aic_hash("sm3", x2my2, 64 + flen, dgst);
    memcpy(&pout[64], dgst, 32);

    // output C2 = M xor t, t = KDF(x2 || y2, inlen)
    sm2_kdf(x2y2, 64, flen, t);
    gmssl_memxor(&pout[96], t, from, flen);

out:
    if (handle)
        aic_akcipher_destroy(handle);

    return ret;
}

static int aic_priv_dec(int flen, const unsigned char *from, unsigned char *to,
                       struct ak_options *opts, char *cipher_name)
{
    struct aic_akcipher_handle *handle = NULL;
    int ret = 0, maxsize = 0;
    u8 x2y2[128], x2my2[128] __attribute__((aligned(64)));
    u8 C2[32], C3[32], dgst[32] __attribute__((aligned(64)));
    u8 t[flen], M[32] __attribute__((aligned(64)));
    u8 *pout = to;

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

    // check C1 is on sm2 curve
    ret = aic_akcipher_decrypt(handle, from, 0x40, x2y2, 0x80);
    if (ret < 0) {
        pr_err("aic pub dec failed.\n");
        goto out;
    }

    // t = KDF(x2 || y2, inlen - 96)
    sm2_kdf(x2y2, 64, flen - 96, t);

    // output M = C2 xor t
    memcpy(C2, &from[96], flen - 96);
    gmssl_memxor(M, t, C2, flen - 96);

    // output C3 = Hash(x2 || m || y2)
    memcpy(C3, &from[64], 32);
    memcpy(x2my2, x2y2, 32);
    memcpy(&x2my2[32], M, flen - 96);
    memcpy(&x2my2[32 + flen - 96], &x2y2[32], 32);
    aic_hash("sm3", x2my2, flen - 32, dgst);

    if (memcmp(C3, dgst, flen - 96)) {
        pr_err("sm2 decrypt failed.\n");
        goto out;
    }
    memcpy(pout, M, flen - 96);

out:
    if (handle)
        aic_akcipher_destroy(handle);

    return ret;
}

int aic_sm2_sign(int flen, unsigned char *from, unsigned char *to,
                     struct ak_options *opts)
{
    return aic_priv_enc(flen, from, to, opts, "sm2");
}

int aic_sm2_verify(int flen, unsigned char *from, unsigned char *to,
                    struct ak_options *opts)
{
    return aic_pub_dec(flen, from, to, opts, "sm2");
}

int aic_sm2_pub_enc(int flen, unsigned char *from, unsigned char *to,
                     struct ak_options *opts)
{
    return aic_pub_enc(flen, from, to, opts, "sm2");
}

int aic_sm2_priv_dec(int flen, unsigned char *from, unsigned char *to,
                    struct ak_options *opts)
{
    return aic_priv_dec(flen, from, to, opts, "sm2");
}
