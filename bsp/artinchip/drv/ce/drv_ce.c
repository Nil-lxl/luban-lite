/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <aic_log.h>
#include <hwcrypto.h>
#include <hw_hash.h>
#include <hw_symmetric.h>
#include <hw_bignum.h>
#include <hal_ce.h>
#ifdef AIC_DCE_DRV
#include <hal_dce.h>
#include <hw_crc.h>
#endif

#define AES_BLOCK_SIZE   16
#define AES_MAX_KEY_LEN  32
#define CE_WORK_BUF_LEN  1024
#define RSA_CHECK_OPSIZE(x) (((x) == 64) || ((x) == 128) || ((x) == 256) ? 0 : 1)
#define RSA_SELECT_OPSIZE(x) ((x) / 16)
struct aic_hwcrypto_device
{
    struct rt_hwcrypto_device dev;
    struct rt_mutex mutex;
};

#define SHA_MAX_OUTPUT_LEN 64

struct sha_priv {
    u8 digest[SHA_MAX_OUTPUT_LEN];
    u32 alg_tag;
    u32 out_len;
    u32 digest_len;
    u32 block_size;
};

typedef enum {
    SHA_MODE_1 = 1U,
    SHA_MODE_256,
    SHA_MODE_224,
    SHA_MODE_512,
    SHA_MODE_384,
    SHA_MODE_512_256,
    SHA_MODE_512_224
} sha_mode_t;

typedef struct {
    struct sha_priv priv;
    sha_mode_t mode;
    uint32_t total[2];
    uint32_t state[16];
    uint8_t buffer[128];
} aic_sha_context_t;

rt_err_t drv_aes_init(struct rt_hwcrypto_ctx *ctx)
{
    rt_err_t res = RT_EOK;

    res = hal_crypto_init();
    if (res)
        res = -RT_ERROR;

    return res;
}

void drv_aes_uninit(struct rt_hwcrypto_ctx *ctx)
{
    hal_crypto_deinit();
}

static s32 aes_ecb_crypto(u8 *key, u8 keylen, u8 dir, u8 *in, u8 *out, u32 len)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 *pin, *pout;
    u32 dolen;

    memset(&task, 0, sizeof(task));

    task.alg.aes_ecb.alg_tag = ALG_AES_ECB;
    task.alg.aes_ecb.direction = dir;
    task.alg.aes_ecb.key_src = CE_KEY_SRC_USER;
    if (keylen == AES128_KEY_LEN)
        task.alg.aes_ecb.key_siz = KEY_SIZE_128;
    else if (keylen == AES192_KEY_LEN)
        task.alg.aes_ecb.key_siz = KEY_SIZE_192;
    else if (keylen == AES256_KEY_LEN)
        task.alg.aes_ecb.key_siz = KEY_SIZE_256;
    else
        return RT_EINVAL;

    aicos_dcache_clean_range((void *)(unsigned long)key, keylen);
    task.alg.aes_ecb.key_addr = (u32)(uintptr_t)key;

    pin = in;
    pout = out;

    dolen = len;
    do {
        aicos_dcache_clean_range((void *)(unsigned long)pin, dolen);
        task.data.in_addr = (u32)(uintptr_t)(pin);
        task.data.in_len = dolen;
        task.data.out_addr = (u32)(uintptr_t)(pout);
        task.data.out_len = dolen;

        aicos_dcache_clean_range((void *)(unsigned long)&task, sizeof(task));
        hal_crypto_start_symm(&task);

        while (!hal_crypto_poll_finish(ALG_SK_ACCELERATOR)) {
            continue;
        }
        hal_crypto_pending_clear(ALG_SK_ACCELERATOR);

        if (hal_crypto_get_err(ALG_SK_ACCELERATOR)) {
            pr_err("AES run error.\n");
            return RT_ERROR;
        }
        aicos_dma_sync();
        aicos_dcache_invalid_range((void *)(unsigned long)pout, dolen);
        len -= dolen;
    } while (len);

    return RT_EOK;
}

static s32 aes_cbc_crypto(u8 *key, u8 keylen, u8 dir, u8 *iv, u8 *in, u8 *out,
                          u32 len)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 *pin, *pout, *iv_in;
    u32 dolen;

    memset(&task, 0, sizeof(task));

    task.alg.aes_cbc.alg_tag = ALG_AES_CBC;
    task.alg.aes_cbc.direction = dir;
    task.alg.aes_cbc.key_src = CE_KEY_SRC_USER;
    if (keylen == AES128_KEY_LEN)
        task.alg.aes_cbc.key_siz = KEY_SIZE_128;
    else if (keylen == AES192_KEY_LEN)
        task.alg.aes_cbc.key_siz = KEY_SIZE_192;
    else if (keylen == AES256_KEY_LEN)
        task.alg.aes_cbc.key_siz = KEY_SIZE_256;
    else
        return RT_EINVAL;

    aicos_dcache_clean_range((void *)(unsigned long)key, keylen);
    task.alg.aes_cbc.key_addr = (u32)(uintptr_t)key;
    iv_in = iv;
    memcpy(iv_in, iv, AES_BLOCK_SIZE);
    task.alg.aes_cbc.iv_addr = (u32)(uintptr_t)iv_in;

    pin = in;
    pout = out;
    dolen = len;

    do {
        aicos_dcache_clean_range((void *)(unsigned long)iv_in, AES_BLOCK_SIZE);
        aicos_dcache_clean_range((void *)(unsigned long)pin, dolen);
        aicos_dcache_clean_range((void *)(unsigned long)pout, dolen);
        task.data.in_addr = (u32)(uintptr_t)(pin);
        task.data.in_len = dolen;
        task.data.out_addr = (u32)(uintptr_t)(pout);
        task.data.out_len = dolen;

        aicos_dcache_clean_range((void *)(unsigned long)&task, sizeof(task));
        hal_crypto_start_symm(&task);

        while (!hal_crypto_poll_finish(ALG_SK_ACCELERATOR)) {
            continue;
        }
        hal_crypto_pending_clear(ALG_SK_ACCELERATOR);

        if (hal_crypto_get_err(ALG_SK_ACCELERATOR)) {
            pr_err("AES run error.\n");
            return RT_ERROR;
        }
        aicos_dma_sync();
        writel(0xA0, 0x19030FFCUL);
        aicos_dcache_invalid_range((void *)(unsigned long)pout, dolen);
        writel(0xA1, 0x19030FFCUL);
        /* prepare iv for next */
        if (dir == ALG_DIR_ENCRYPT)
            memcpy(iv_in, pout + dolen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
        else
            memcpy(iv_in, pin + dolen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);

        pout += dolen;
        pin += dolen;
        len -= dolen;
    } while (len);

    return RT_EOK;
}

rt_err_t drv_sha_init(struct rt_hwcrypto_ctx *ctx)
{
    if (!ctx)
        return RT_ERROR;

    ctx->contex =
        aicos_malloc_align(0, sizeof(aic_sha_context_t), CACHE_LINE_SIZE);
    if (!ctx->contex)
        return RT_ERROR;

    hal_crypto_init();

    return RT_EOK;
}

void drv_sha_uninit(struct rt_hwcrypto_ctx *ctx)
{
    if (!ctx)
        return;

    if (ctx->contex) {
        aicos_free_align(0, ctx->contex);
        ctx->contex = NULL;
    }

    hal_crypto_deinit();
}

rt_err_t drv_sha_start(struct rt_hwcrypto_ctx *ctx)
{
    if (!ctx || !ctx->contex)
        return RT_ERROR;

    aic_sha_context_t *context = ctx->contex;

    memset(context, 0, sizeof(aic_sha_context_t));
    context->mode = ctx->type;

    switch (ctx->type) {
        case HWCRYPTO_TYPE_MD5:
            context->priv.alg_tag = ALG_MD5;
            context->priv.out_len = MD5_CE_OUT_LEN;
            context->priv.digest_len = MD5_DIGEST_SIZE;
            context->priv.block_size = MD5_BLOCK_SIZE;
            u32 md5_iv[] = { MD5_H0, MD5_H1, MD5_H2, MD5_H3 };
            memcpy(context->priv.digest, md5_iv, sizeof(md5_iv));
            break;
        case HWCRYPTO_TYPE_SHA1:
            context->priv.alg_tag = ALG_SHA1;
            context->priv.out_len = SHA1_CE_OUT_LEN;
            context->priv.digest_len = SHA1_DIGEST_SIZE;
            context->priv.block_size = SHA1_BLOCK_SIZE;
            u32 sha1_iv[] = { BE_SHA1_H0, BE_SHA1_H1, BE_SHA1_H2,
                                       BE_SHA1_H3, BE_SHA1_H4 };
            memcpy(context->priv.digest, sha1_iv, sizeof(sha1_iv));
            break;
        case HWCRYPTO_TYPE_SHA256:
            context->priv.alg_tag = ALG_SHA256;
            context->priv.out_len = SHA256_CE_OUT_LEN;
            context->priv.digest_len = SHA256_DIGEST_SIZE;
            context->priv.block_size = SHA256_BLOCK_SIZE;
            u32 sha256_iv[] = { BE_SHA256_H0, BE_SHA256_H1,
                                         BE_SHA256_H2, BE_SHA256_H3,
                                         BE_SHA256_H4, BE_SHA256_H5,
                                         BE_SHA256_H6, BE_SHA256_H7 };
            memcpy(context->priv.digest, sha256_iv, sizeof(sha256_iv));
            break;
        case HWCRYPTO_TYPE_SHA224:
            context->priv.alg_tag = ALG_SHA224;
            context->priv.out_len = SHA224_CE_OUT_LEN;
            context->priv.digest_len = SHA224_DIGEST_SIZE;
            context->priv.block_size = SHA224_BLOCK_SIZE;
            u32 sha224_iv[] = { BE_SHA224_H0, BE_SHA224_H1,
                                         BE_SHA224_H2, BE_SHA224_H3,
                                         BE_SHA224_H4, BE_SHA224_H5,
                                         BE_SHA224_H6, BE_SHA224_H7 };
            memcpy(context->priv.digest, sha224_iv, sizeof(sha224_iv));
            break;
        case HWCRYPTO_TYPE_SHA512:
            context->priv.alg_tag = ALG_SHA512;
            context->priv.out_len = SHA512_CE_OUT_LEN;
            context->priv.digest_len = SHA512_DIGEST_SIZE;
            context->priv.block_size = SHA512_BLOCK_SIZE;
            u64 sha512_iv[] = { BE_SHA512_H0, BE_SHA512_H1,
                                          BE_SHA512_H2, BE_SHA512_H3,
                                          BE_SHA512_H4, BE_SHA512_H5,
                                          BE_SHA512_H6, BE_SHA512_H7 };
            memcpy(context->priv.digest, sha512_iv, sizeof(sha512_iv));
            break;
        case HWCRYPTO_TYPE_SHA384:
            context->priv.alg_tag = ALG_SHA384;
            context->priv.out_len = SHA384_CE_OUT_LEN;
            context->priv.digest_len = SHA384_DIGEST_SIZE;
            context->priv.block_size = SHA384_BLOCK_SIZE;
            u64 sha384_iv[] = { BE_SHA384_H0, BE_SHA384_H1,
                                          BE_SHA384_H2, BE_SHA384_H3,
                                          BE_SHA384_H4, BE_SHA384_H5,
                                          BE_SHA384_H6, BE_SHA384_H7 };
            memcpy(context->priv.digest, sha384_iv, sizeof(sha384_iv));
            break;
        default:
            return RT_EINVAL;
    }

    return RT_EOK;
}

rt_err_t drv_sha_update(aic_sha_context_t *context, const void *input,
                        uint32_t size)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    struct sha_priv *priv;
    u8 *in, *out, *iv;
    u32 dolen;
    u64 total_len;

    priv = &context->priv;
    if (!context || !input || !priv)
        return RT_ERROR;

    memcpy(&total_len, context->total, 8);
    total_len += size;

    memset(&task, 0, sizeof(struct crypto_task));

    iv = priv->digest;
    out = priv->digest;
    task.alg.hash.alg_tag = priv->alg_tag;
    task.alg.hash.iv_mode = 1;

    aicos_dcache_clean_range((void *)(unsigned long)iv, priv->digest_len);
    task.alg.hash.iv_addr = (u32)(uintptr_t)iv;

    in = (u8 *)input;

    if ((size % priv->block_size)) {
        task.data.last_flag = 1;
        task.data.total_bytelen = total_len;
    }

    dolen = size;
    do {
        aicos_dcache_clean_range((void *)(unsigned long)in, dolen);
        task.data.in_addr = (u32)(uintptr_t)(in);
        task.data.in_len = dolen;
        task.data.out_addr = (u32)(uintptr_t)(out);
        task.data.out_len = priv->out_len;

        aicos_dcache_clean_range((void *)(unsigned long)&task,
                                 sizeof(struct crypto_task));
        hal_crypto_start_hash(&task);

        while (!hal_crypto_poll_finish(ALG_HASH_ACCELERATOR)) {
            continue;
        }
        hal_crypto_pending_clear(ALG_HASH_ACCELERATOR);

        if (hal_crypto_get_err(ALG_HASH_ACCELERATOR)) {
            pr_err("SHA run error.\n");
            return RT_ERROR;
        }
        size -= dolen;
        input += dolen;
    } while (size > 0);

    aicos_dma_sync();
    aicos_dcache_invalid_range((void *)(unsigned long)out, SHA_MAX_OUTPUT_LEN);
    memcpy(context->total, &total_len, 8);

    return RT_EOK;
}

rt_err_t drv_sha_finish(struct hwcrypto_hash *ctx, void *output,
                        rt_size_t *out_size)
{
    aic_sha_context_t *context = ctx->parent.contex;
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    struct sha_priv *priv;
    u8 *in, *out, *iv;
    u64 total_len;

    if (!context || !output || !out_size)
        return RT_ERROR;

    priv = (struct sha_priv *)&context->priv;
    iv = priv->digest;
    in = priv->digest;
    out = priv->digest;
    memcpy(&total_len, context->total, 8);
    if ((total_len % priv->block_size) == 0) {
        aicos_dcache_clean_range((void *)(unsigned long)iv, priv->digest_len);
        memset(&task, 0, sizeof(struct crypto_task));
        task.alg.hash.alg_tag = priv->alg_tag;
        task.alg.hash.iv_mode = 1;
        task.alg.hash.iv_addr = (u32)(uintptr_t)iv;

        task.data.in_addr = (u32)(uintptr_t)(in);
        task.data.in_len = 0;
        task.data.out_addr = (u32)(uintptr_t)(out);
        task.data.out_len = priv->out_len;
        task.data.last_flag = 1;
        task.data.total_bytelen = total_len;

        aicos_dcache_clean_range((void *)(unsigned long)&task,
                                 sizeof(struct crypto_task));
        hal_crypto_start_hash(&task);

        while (!hal_crypto_poll_finish(ALG_HASH_ACCELERATOR)) {
            continue;
        }
        hal_crypto_pending_clear(ALG_HASH_ACCELERATOR);

        if (hal_crypto_get_err(ALG_HASH_ACCELERATOR)) {
            pr_err("SHA run error.\n");
            return RT_ERROR;
        }
        aicos_dma_sync();
        aicos_dcache_invalid_range((void *)(unsigned long)out, SHA_MAX_OUTPUT_LEN);
    }
    memcpy(output, out, priv->digest_len);
    *out_size = priv->digest_len;

    return RT_EOK;
}

static rt_err_t drv_aes_crypt(struct hwcrypto_symmetric *symmetric_ctx,
                              struct hwcrypto_symmetric_info *symmetric_info)
{
    struct aic_hwcrypto_device *hwcrypto =
        (struct aic_hwcrypto_device *)symmetric_ctx->parent.device;
    hwcrypto_mode mode;
    unsigned char *in, *out, *key, *iv;
    unsigned char data_align_flag = 0, iv_align_flag = 0, key_align_flag = 0;
    unsigned int klen = 0, dlen = 0, ivlen = 0;

    if ((symmetric_ctx->key_bitlen >> 3) != 16 ||
        (symmetric_info->length % 16) != 0) {
        return -RT_EINVAL;
    }

    in = (unsigned char *)symmetric_info->in;
    out = (unsigned char *)symmetric_info->out;
    key = (unsigned char *)symmetric_ctx->key;
    iv = (unsigned char *)symmetric_ctx->iv;
    ivlen = symmetric_ctx->iv_len;
    klen = symmetric_ctx->key_bitlen >> 3;
    dlen = symmetric_info->length;

#if !defined(AIC_HWCRYPTO_NOT_ALIGN_CHECK)
    if (((rt_uint32_t)(uintptr_t)in % CACHE_LINE_SIZE) != 0 ||
        ((rt_uint32_t)(uintptr_t)out % CACHE_LINE_SIZE) != 0) {
        in = aicos_malloc_align(0, symmetric_info->length, CACHE_LINE_SIZE);
        if (in) {
            memcpy(in, symmetric_info->in, symmetric_info->length);
            out = in;
            data_align_flag = 1;
        } else {
            return -RT_ENOMEM;
        }
    }
    if (((rt_uint32_t)(uintptr_t)key % CACHE_LINE_SIZE) != 0) {
        key = aicos_malloc_align(0, klen, CACHE_LINE_SIZE);
        if (key) {
            memcpy(key, symmetric_ctx->key, klen);
            key_align_flag = 1;
        } else {
            return -RT_ENOMEM;
        }
    }
    if (((rt_uint32_t)(uintptr_t)iv % CACHE_LINE_SIZE) != 0) {
        iv = aicos_malloc_align(0, ivlen, CACHE_LINE_SIZE);
        if (iv) {
            memcpy(iv, symmetric_ctx->iv, ivlen);
            iv_align_flag = 1;
        } else {
            return -RT_ENOMEM;
        }
    }
#endif

    mode = (symmetric_info->mode == 0x1) ? 0x0 : 0x1;

#if !defined(AIC_HWCRYPTO_NOT_LOCK)
    rt_mutex_take(&hwcrypto->mutex, RT_WAITING_FOREVER);
#endif
    switch (symmetric_ctx->parent.type &
            (HWCRYPTO_MAIN_TYPE_MASK | HWCRYPTO_SUB_TYPE_MASK)) {
        case HWCRYPTO_TYPE_AES_ECB:
            aes_ecb_crypto(key, klen, mode, in, out, dlen);
            break;
        case HWCRYPTO_TYPE_AES_CBC:
            aes_cbc_crypto(key, klen, mode, iv, in, out, dlen);
            break;
        case HWCRYPTO_TYPE_AES_CTR:
            break;
        default:
            return -RT_ERROR;
    }
#if !defined(AIC_HWCRYPTO_NOT_LOCK)
    rt_mutex_release(&hwcrypto->mutex);
#endif
#if !defined(AIC_HWCRYPTO_NOT_ALIGN_CHECK)
    if (data_align_flag) {
        memcpy(symmetric_info->out, out, symmetric_info->length);
        aicos_free_align(0, in);
    }
    if (key_align_flag) {
        aicos_free_align(0, key);
    }
    if (iv_align_flag) {
        aicos_free_align(0, iv);
    }
#endif

    return RT_EOK;
}

static rt_err_t drv_hash_update(struct hwcrypto_hash *ctx, const rt_uint8_t *in,
                                rt_size_t length)
{
    rt_err_t err = RT_EOK;
    struct aic_hwcrypto_device *hwcrypto =
        (struct aic_hwcrypto_device *)ctx->parent.device;
    unsigned char align_flag = 0;

#if !defined(AIC_HWCRYPTO_NOT_ALIGN_CHECK)
    if (((rt_uint32_t)(uintptr_t)in % CACHE_LINE_SIZE) != 0) {
        void *temp;
        temp = aicos_malloc_align(0, length, CACHE_LINE_SIZE);
        if (temp) {
            memcpy(temp, in, length);
            in = temp;
            align_flag = 1;
        } else {
            return -RT_ENOMEM;
        }
    }
#endif

#if !defined(AIC_HWCRYPTO_NOT_LOCK)
    rt_mutex_take(&hwcrypto->mutex, RT_WAITING_FOREVER);
#endif
    switch (ctx->parent.type & HWCRYPTO_MAIN_TYPE_MASK) {
        case HWCRYPTO_TYPE_MD5:
        case HWCRYPTO_TYPE_SHA1:
        case HWCRYPTO_TYPE_SHA2:
            drv_sha_update(ctx->parent.contex, in, length);
            break;
        default:
            err = -RT_ERROR;
            break;
    }
#if !defined(AIC_HWCRYPTO_NOT_LOCK)
    rt_mutex_release(&hwcrypto->mutex);
#endif

#if !defined(AIC_HWCRYPTO_NOT_ALIGN_CHECK)
    if (align_flag) {
        aicos_free_align(0, (rt_uint8_t *)in);
    }
#endif

    return err;
}

static rt_err_t drv_hash_finish(struct hwcrypto_hash *ctx, rt_uint8_t *out,
                                rt_size_t length)
{
    rt_err_t err = RT_EOK;
    struct aic_hwcrypto_device *hwcrypto =
        (struct aic_hwcrypto_device *)ctx->parent.device;

#if !defined(AIC_HWCRYPTO_NOT_LOCK)
    rt_mutex_take(&hwcrypto->mutex, RT_WAITING_FOREVER);
#endif
    switch (ctx->parent.type & HWCRYPTO_MAIN_TYPE_MASK) {
        case HWCRYPTO_TYPE_MD5:
        case HWCRYPTO_TYPE_SHA1:
        case HWCRYPTO_TYPE_SHA2:
            drv_sha_finish(ctx, out, &length);
            break;
        default:
            err = -RT_ERROR;
            break;
    }
#if !defined(AIC_HWCRYPTO_NOT_LOCK)
    rt_mutex_release(&hwcrypto->mutex);
#endif

    return err;
}

rt_err_t drv_rsa_init(struct rt_hwcrypto_ctx *ctx)
{
    rt_err_t res = RT_EOK;

    res = hal_crypto_init();
    if (res)
        res = -RT_ERROR;

    return res;
}

void drv_rsa_uninit(struct rt_hwcrypto_ctx *ctx)
{
    hal_crypto_deinit();
}

static s32 rsa_calc(void *mod, void *prime, void *src, u32 src_size, void *out)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 *pn, *pp, *data, *pout;
    u32 opsize;
    int ret = 0;

    if (RSA_CHECK_OPSIZE(src_size)) {
        pr_err("opsize %d error\n", src_size);
        return -1;
    }

    /* Use aligned buffer to CE */
    pp = aicos_malloc_align(0, CE_WORK_BUF_LEN, CACHE_LINE_SIZE);
    if (pp == NULL) {
        pr_err("malloc aligned buf failed.\n");
        return -1;
    }
    memset(pp, 0, CE_WORK_BUF_LEN);
    opsize = src_size;
    pn = pp + opsize;
    data = pn + opsize;
    pout = data + opsize;

    hal_crypto_bignum_be2le(src, opsize, data, opsize);
    hal_crypto_bignum_be2le(mod, opsize, pn, opsize);
    hal_crypto_bignum_be2le(prime, opsize, pp, opsize);

    memset(&task, 0, sizeof(task));
    task.alg.rsa.alg_tag = ALG_RSA;

    task.alg.rsa.op_siz = RSA_SELECT_OPSIZE(opsize);
    task.alg.rsa.m_addr = (u32)(uintptr_t)pn;
    task.alg.rsa.d_e_addr = (u32)(uintptr_t)pp;
    task.data.in_addr = (u32)(uintptr_t)data;
    task.data.in_len = opsize;
    task.data.out_addr = (u32)(uintptr_t)pout;
    task.data.out_len = opsize;

    aicos_dcache_clean_range((void *)(unsigned long)pp, CE_WORK_BUF_LEN);
    aicos_dcache_clean_range((void *)(unsigned long)&task, sizeof(task));

    hal_crypto_start_asym(&task);

    while (!hal_crypto_poll_finish(ALG_AK_ACCELERATOR)) {
        continue;
    }
    hal_crypto_pending_clear(ALG_AK_ACCELERATOR);

    if (hal_crypto_get_err(ALG_AK_ACCELERATOR)) {
        pr_err("RSA run error.\n");
        ret = RT_ERROR;
        goto out;
    }

    aicos_dma_sync();
    aicos_dcache_invalid_range((void *)(unsigned long)pout, opsize);
    hal_crypto_bignum_le2be(pout, opsize, out, opsize);

out:
    if (pp)
        aicos_free_align(0, pp);

    return ret;
}

/* x = a ^ b (mod c) */
static rt_err_t drv_exptmod(struct hwcrypto_bignum *ctx,
                        struct hw_bignum_mpi *x,
                        const struct hw_bignum_mpi *a,
                        const struct hw_bignum_mpi *b,
                        const struct hw_bignum_mpi *c)
{
    return rsa_calc(c->p, b->p, a->p, a->total, x->p);
}

static const struct hwcrypto_bignum_ops rsa_ops = {
    .exptmod = drv_exptmod,
};

static const struct hwcrypto_symmetric_ops aes_ops = {
    .crypt = drv_aes_crypt,
};

static const struct hwcrypto_hash_ops hash_ops = {
    .update = drv_hash_update,
    .finish = drv_hash_finish,
};

#ifdef AIC_DCE_DRV
#define CRC_32_POLY     0x04C11DB7
rt_err_t drv_crc_init(struct rt_hwcrypto_ctx *ctx)
{
    rt_err_t res = RT_EOK;

    res = hal_dce_init();
    if (res)
        res = -RT_ERROR;

    return res;
}

static rt_uint32_t drv_crc_update(struct hwcrypto_crc *ctx, const rt_uint8_t *in,
                                rt_size_t length)
{
    rt_uint32_t crc_result = 0, ret;

    if (ctx->crc_cfg.poly != CRC_32_POLY) {
        pr_err("Artinchip hardware crc only support CRC_32_POLY.\n");
        return -RT_ERROR;
    }

    if (ctx->crc_cfg.xorout)
        hal_dce_crc32_xor_val(ctx->crc_cfg.xorout);
    if (ctx->crc_cfg.flags)
        hal_dce_crc32_cfg((ctx->crc_cfg.flags & CRC_FLAG_REFIN), 0,
            (ctx->crc_cfg.flags & CRC_FLAG_REFIN),
            (ctx->crc_cfg.flags & CRC_FLAG_REFOUT));

    hal_dce_crc32_start(ctx->crc_cfg.last_val, (u8 *)in, length);
    ret = hal_dce_crc32_wait();
    if (!ret) {
        crc_result = hal_dce_crc32_result();
    } else {
        pr_err("\t%s error: time out\n", __func__);
        return -RT_ERROR;
    }

    ctx->crc_cfg.last_val = crc_result;
    return crc_result;
}

void drv_crc_uninit(struct rt_hwcrypto_ctx *ctx)
{
    if (!ctx)
        return;

    if (ctx->contex) {
        aicos_free_align(0, ctx->contex);
        ctx->contex = NULL;
    }
    hal_dce_deinit();
}

static const struct hwcrypto_crc_ops crc_ops = {
    .update = drv_crc_update,
};
#endif

static rt_err_t aic_hwcrypto_create(struct rt_hwcrypto_ctx *ctx)
{
    rt_err_t res = RT_EOK;
    RT_ASSERT(ctx != RT_NULL);

    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK) {
        case HWCRYPTO_TYPE_AES:
            drv_aes_init(ctx);
            hal_crypto_init();

            /* Setup AES operation */
            ((struct hwcrypto_symmetric *)ctx)->ops = &aes_ops;
            break;
        case HWCRYPTO_TYPE_BIGNUM:
            drv_rsa_init(ctx);

            /* Setup RSA operation */
            ((struct hwcrypto_bignum *)ctx)->ops = &rsa_ops;
            break;
        case HWCRYPTO_TYPE_MD5:
        case HWCRYPTO_TYPE_SHA1:
        case HWCRYPTO_TYPE_SHA2:
            drv_sha_init(ctx);
            drv_sha_start(ctx);

            /* Setup HASH operation */
            ((struct hwcrypto_hash *)ctx)->ops = &hash_ops;
            break;
#ifdef AIC_DCE_DRV
        case HWCRYPTO_TYPE_CRC:
            drv_crc_init(ctx);
            ((struct hwcrypto_crc *)ctx)->ops = &crc_ops;
            break;
#endif
        default:
            res = -RT_ERROR;
            break;
    }

    return res;
}

static void aic_hwcrypto_destroy(struct rt_hwcrypto_ctx *ctx)
{
    RT_ASSERT(ctx != RT_NULL);

    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK) {
        case HWCRYPTO_TYPE_AES:
            drv_aes_uninit(ctx);
            break;
        case HWCRYPTO_TYPE_DES:
            break;
        case HWCRYPTO_TYPE_BIGNUM:
            drv_rsa_uninit(ctx);
            break;
        case HWCRYPTO_TYPE_MD5:
        case HWCRYPTO_TYPE_SHA1:
        case HWCRYPTO_TYPE_SHA2:
            drv_sha_uninit(ctx);
            break;
#ifdef AIC_DCE_DRV
        case HWCRYPTO_TYPE_CRC:
            drv_crc_uninit(ctx);
            break;
#endif
        default:
            break;
    }
}

static rt_err_t aic_hwcrypto_clone(struct rt_hwcrypto_ctx *des,
                                   const struct rt_hwcrypto_ctx *src)
{
    rt_err_t res = RT_EOK;

    switch (src->type & HWCRYPTO_MAIN_TYPE_MASK) {
        case HWCRYPTO_TYPE_AES:
        case HWCRYPTO_TYPE_RC4:
        case HWCRYPTO_TYPE_RNG:
        case HWCRYPTO_TYPE_CRC:
        case HWCRYPTO_TYPE_BIGNUM:
            break;
        case HWCRYPTO_TYPE_MD5:
        case HWCRYPTO_TYPE_SHA1:
        case HWCRYPTO_TYPE_SHA2:
            if (des->contex && src->contex) {
                rt_memcpy(des->contex, src->contex, sizeof(aic_sha_context_t));
            }
            break;
        default:
            res = -RT_ERROR;
            break;
    }

    return res;
}

static void aic_hwcrypto_reset(struct rt_hwcrypto_ctx *ctx)
{
    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK) {
        case HWCRYPTO_TYPE_AES:
            drv_aes_init(ctx);
            break;
        case HWCRYPTO_TYPE_RC4:
        case HWCRYPTO_TYPE_RNG:
            break;
        case HWCRYPTO_TYPE_BIGNUM:
            drv_rsa_init(ctx);
            break;
        case HWCRYPTO_TYPE_MD5:
        case HWCRYPTO_TYPE_SHA1:
        case HWCRYPTO_TYPE_SHA2:
            drv_sha_init(ctx);
            drv_sha_start(ctx);
            break;
#ifdef AIC_DCE_DRV
        case HWCRYPTO_TYPE_CRC:
            drv_crc_init(ctx);
            break;
#endif
        default:
            break;
    }
}

static const struct rt_hwcrypto_ops aic_ops = {
    .create = aic_hwcrypto_create,
    .destroy = aic_hwcrypto_destroy,
    .copy = aic_hwcrypto_clone,
    .reset = aic_hwcrypto_reset,
};

int aic_hw_crypto_device_init(void)
{
    static struct aic_hwcrypto_device crypto_dev;

    crypto_dev.dev.ops = &aic_ops;
    crypto_dev.dev.id = 0;
    crypto_dev.dev.user_data = &crypto_dev;

    if (rt_hwcrypto_register(&crypto_dev.dev, RT_HWCRYPTO_DEFAULT_NAME) !=
        RT_EOK) {
        return -1;
    }
    rt_mutex_init(&crypto_dev.mutex, RT_HWCRYPTO_DEFAULT_NAME,
                  RT_IPC_FLAG_PRIO);

    return 0;
}
INIT_DEVICE_EXPORT(aic_hw_crypto_device_init);
