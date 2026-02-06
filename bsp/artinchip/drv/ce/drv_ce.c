/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
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
#define RSA_CHECK_OPSIZE(x) (((x) == 512) || ((x) == 1024) || ((x) == 2048) ? 0 : 1)
#define AES_CHECK_OPSIZE(x) (((x) == 16) || ((x) == 24) || ((x) == 32) ? 0 : 1)
struct aic_hwcrypto_device
{
    struct rt_hwcrypto_device dev;
    struct rt_mutex mutex;
};

#define SHA_MAX_OUTPUT_LEN 64
#define SHA_MAX_BLOCK_SIZE 128

struct sha_priv {
    u8 digest[SHA_MAX_OUTPUT_LEN];
    u8 remain_buf[SHA_MAX_BLOCK_SIZE];
    u32 alg_tag;
    u32 out_len;
    u32 digest_len;
    u32 block_size;
    u32 remain_len;
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

static int ref_count = 0;

static u32 select_opsize(u32 opsize)
{
    switch (opsize) {
    case 8:
        return 0x0;
    case 16:
        return 0x1;
    case 24:
        return 0x2;
    case 32:
        return 0x3;
    case 64:
        return 0x4;
    case 128:
        return 0x5;
    case 256:
        return 0x6;
    default:
        pr_err("not support opsize %d\n", opsize);
        return 0xFF;
    }
}
rt_err_t drv_aes_init(struct rt_hwcrypto_ctx *ctx)
{
    rt_err_t res = RT_EOK;

    if (ref_count++ == 0) {
        res = hal_crypto_init();
        if (res)
            res = -RT_ERROR;
    }

    return res;
}

void drv_aes_uninit(struct rt_hwcrypto_ctx *ctx)
{
    if (--ref_count <= 0) {
        ref_count = 0;
        hal_crypto_deinit();
    }
}

static s32 aes_ecb_crypto(u8 *key, u8 keylen, u8 dir, u8 *in, u8 *out, u32 len)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 *pin, *pout;
    u32 bytelen, remain, chunk = CE_CIPHER_MAX_DATA_SIZE;
    u32 timeout = 300 * 1000;
    int i = 0, task_cnt = 0;

    if (keylen != 16 && keylen != 24 && keylen != 32) {
        return -RT_EINVAL;
    }

    task_cnt = DIV_ROUND_UP(len, chunk);

    pin = in;
    pout = out;
    remain = len;
    for (i = 0; i < task_cnt; i++) {
        bytelen = min(remain, chunk);

        aicos_dcache_clean_range((void *)(unsigned long)key, keylen);
        aicos_dcache_clean_range((void *)(unsigned long)pin, bytelen);

        memset(&task, 0, sizeof(task));
        task.alg.aes_ecb.alg_tag = ALG_AES_ECB;
        task.alg.aes_ecb.direction = dir;
        task.alg.aes_ecb.key_siz = select_opsize(keylen);
        task.alg.aes_ecb.key_src = CE_KEY_SRC_USER;
        task.alg.aes_ecb.key_addr = (u32)(uintptr_t)key;

        task.data.in_addr = (u32)(uintptr_t)(pin);
        task.data.in_len = bytelen;
        task.data.out_addr = (u32)(uintptr_t)(pout);
        task.data.out_len = bytelen;

        aicos_dcache_clean_range((void *)(unsigned long)&task, sizeof(task));
        hal_crypto_start_symm(&task);

        if (hal_crypto_poll_finish(ALG_SK_ACCELERATOR, timeout)) {
            pr_err("AES run timeout.\n");
            return -ETIMEDOUT;
        }
        hal_crypto_pending_clear(ALG_SK_ACCELERATOR);

        if (hal_crypto_get_err(ALG_SK_ACCELERATOR)) {
            pr_err("AES run error.\n");
            return -RT_ERROR;
        }
        aicos_dma_sync();
        aicos_dcache_invalid_range((void *)(unsigned long)pout, bytelen);

        remain -= bytelen;
        pin += bytelen;
        pout += bytelen;
    };

    return RT_EOK;
}

static s32 aes_cbc_crypto(u8 *key, u8 keylen, u8 dir, u8 *iv, u8 *in, u8 *out,
                          u32 len)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 *pin, *pout, *iv_in;
    u32 bytelen, remain, chunk = CE_CIPHER_MAX_DATA_SIZE;
    u32 timeout = 300 * 1000;
    int i = 0, task_cnt = 0;

    if (keylen != 16 && keylen != 24 && keylen != 32) {
        return -RT_EINVAL;
    }

    task_cnt = DIV_ROUND_UP(len, chunk);

    pin = in;
    pout = out;
    iv_in = iv;
    remain = len;
    for (i = 0; i < task_cnt; i++) {
        bytelen = min(remain, chunk);

        aicos_dcache_clean_range((void *)(unsigned long)key, keylen);
        aicos_dcache_clean_range((void *)(uintptr_t)iv_in, AES_BLOCK_SIZE);
        aicos_dcache_clean_range((void *)(uintptr_t)pin, bytelen);
        aicos_dcache_clean_range((void *)(uintptr_t)pout, bytelen);

        memset(&task, 0, sizeof(task));
        task.alg.aes_cbc.alg_tag = ALG_AES_CBC;
        task.alg.aes_cbc.direction = dir;
        task.alg.aes_cbc.key_siz = select_opsize(keylen);
        task.alg.aes_cbc.key_src = CE_KEY_SRC_USER;
        task.alg.aes_cbc.key_addr = (u32)(uintptr_t)key;
        task.alg.aes_cbc.iv_addr = (u32)(uintptr_t)iv_in;

        task.data.in_addr = (u32)(uintptr_t)(pin);
        task.data.in_len = bytelen;
        task.data.out_addr = (u32)(uintptr_t)(pout);
        task.data.out_len = bytelen;

        aicos_dcache_clean_range((void *)(uintptr_t)&task, sizeof(task));
        hal_crypto_start_symm(&task);

        if (hal_crypto_poll_finish(ALG_SK_ACCELERATOR, timeout)) {
            pr_err("AES run timeout.\n");
            return -ETIMEDOUT;
        }
        hal_crypto_pending_clear(ALG_SK_ACCELERATOR);

        if (hal_crypto_get_err(ALG_SK_ACCELERATOR)) {
            pr_err("AES run error.\n");
            return -RT_ERROR;
        }
        aicos_dma_sync();
        writel(0xA0, 0x19030FFCUL);
        aicos_dcache_invalid_range((void *)(unsigned long)pout, bytelen);
        writel(0xA1, 0x19030FFCUL);
        /* prepare iv for next */
        if (bytelen >= AES_BLOCK_SIZE) {
            if (dir == ALG_DIR_ENCRYPT)
                memcpy(iv_in, pout + bytelen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
            else
                memcpy(iv_in, pin + bytelen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
        } else {
            pr_err("CBC mode requires at least one full block\n");
            return -RT_ERROR;
        }

        pout += bytelen;
        pin += bytelen;
        remain -= bytelen;
    }

    return RT_EOK;
}

rt_err_t drv_sha_init(struct rt_hwcrypto_ctx *ctx)
{
    rt_err_t res = RT_EOK;

    if (!ctx)
        return RT_ERROR;

    if (ref_count++ == 0) {
        res = hal_crypto_init();
        if (res)
            res = -RT_ERROR;
    }

    return res;
}

void drv_sha_uninit(struct rt_hwcrypto_ctx *ctx)
{
    if (!ctx)
        return;

    if (ctx->contex) {
        aicos_free_align(0, ctx->contex);
        ctx->contex = NULL;
    }

    if (--ref_count <= 0) {
        ref_count = 0;
        hal_crypto_deinit();
    }
}

rt_err_t drv_sha_start(struct rt_hwcrypto_ctx *ctx)
{
    if (!ctx || !ctx->contex)
        return -RT_ERROR;

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
                        u32 size)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    struct sha_priv *priv = NULL;
    u8 *in = NULL, *out = NULL, *iv = NULL, *p = NULL;
    u32 dolen, bytelen, remain, chunk = CE_CIPHER_MAX_DATA_SIZE;
    u32 timeout = 300 * 1000;
    u64 total_len;
    int i = 0, task_cnt = 0;

    priv = &context->priv;
    if (!context || !input || !priv)
        return -RT_ERROR;

    dolen = priv->remain_len + size;
    if (dolen < priv->block_size) {
        /* Not enough data to start CE, backup data in remain buffer */
        p = priv->remain_buf;
        p += priv->remain_len;
        memcpy(p, input, size);
        priv->remain_len = dolen;

        return RT_EOK;
    }

    if (dolen > 0) {
        /* Final step or there is enough data to be processed */
        in = aicos_malloc_align(0, dolen, CACHE_LINE_SIZE);
        if (!in) {
            pr_err("Failed to allocate space for src.\n");
            return -ENOMEM;
        }

        p = in;
        if (priv->remain_len) {
            memcpy(p, priv->remain_buf, priv->remain_len);
            p += priv->remain_len;
        }

        memcpy(p, input, size);
        /* If this is not final step, backup the tail data */
        priv->remain_len = dolen % priv->block_size;
        if (priv->remain_len) {
            p = in;
            p += rounddown(dolen, priv->block_size);
            dolen -= priv->remain_len;
            memcpy(priv->remain_buf, p, priv->remain_len);
        }
    }

    task_cnt = DIV_ROUND_UP(dolen, chunk);

    memcpy(&total_len, context->total, 8);
    total_len += dolen;

    p = in;
    remain = dolen;
    for (i = 0; i < task_cnt; i++) {
        iv = priv->digest;
        out = priv->digest;

        bytelen = min(remain, chunk);

        aicos_dcache_clean_range((void *)(unsigned long)iv, priv->digest_len);
        aicos_dcache_clean_range((void *)(unsigned long)p, bytelen);

        memset(&task, 0, sizeof(struct crypto_task));
        task.alg.hash.alg_tag = priv->alg_tag;
        task.alg.hash.iv_mode = 1;
        task.alg.hash.iv_addr = (u32)(uintptr_t)iv;

        task.data.in_addr = (u32)(uintptr_t)(p);
        task.data.in_len = bytelen;
        task.data.out_addr = (u32)(uintptr_t)(out);
        task.data.out_len = priv->out_len;

        aicos_dcache_clean_range((void *)(uintptr_t)&task,
                                 sizeof(struct crypto_task));
        hal_crypto_start_hash(&task);

        if (hal_crypto_poll_finish(ALG_HASH_ACCELERATOR, timeout)) {
            pr_err("SHA run timeout.\n");
            return -ETIMEDOUT;
        }
        hal_crypto_pending_clear(ALG_HASH_ACCELERATOR);

        if (hal_crypto_get_err(ALG_HASH_ACCELERATOR)) {
            pr_err("SHA run error.\n");
            return -RT_ERROR;
        }

        aicos_dma_sync();
        aicos_dcache_invalid_range((void *)(uintptr_t)out, SHA_MAX_OUTPUT_LEN);

        remain -= bytelen;
        p += bytelen;
    }
    memcpy(context->total, &total_len, 8);

    if (in)
        aicos_free_align(0, in);

    return RT_EOK;
}

rt_err_t drv_sha_finish(struct hwcrypto_hash *ctx, void *output,
                        rt_size_t *out_size)
{
    aic_sha_context_t *context = ctx->parent.contex;
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    struct sha_priv *priv;
    u8 *in = NULL, *out = NULL, *iv = NULL, *p = NULL;
    u32 dolen = 0, timeout = 300 * 1000;
    u64 total_len;

    if (!context || !output || !out_size)
        return -RT_ERROR;

    priv = (struct sha_priv *)&context->priv;

    if (priv->remain_len > 0) {
        dolen = priv->remain_len;

        /* Final step or there is enough data to be processed */
        in = aicos_malloc_align(0, dolen, CACHE_LINE_SIZE);
        if (!in) {
            pr_err("Failed to allocate space for src.\n");
            return -ENOMEM;
        }

        p = in;
        if (priv->remain_len) {
            memcpy(p, priv->remain_buf, priv->remain_len);
        }

        priv->remain_len = 0;
    }

    iv = priv->digest;
    out = priv->digest;

    memcpy(&total_len, context->total, 8);
    total_len += dolen;

    aicos_dcache_clean_range((void *)(unsigned long)in, dolen);
    aicos_dcache_clean_range((void *)(unsigned long)iv, priv->digest_len);
    memset(&task, 0, sizeof(struct crypto_task));
    task.alg.hash.alg_tag = priv->alg_tag;
    task.alg.hash.iv_mode = 1;
    task.alg.hash.iv_addr = (u32)(uintptr_t)iv;

    task.data.in_addr = (u32)(uintptr_t)(in);
    task.data.in_len = dolen;
    task.data.out_addr = (u32)(uintptr_t)(out);
    task.data.out_len = priv->out_len;
    task.data.last_flag = 1;
    task.data.total_bytelen = total_len;

    aicos_dcache_clean_range((void *)(unsigned long)&task,
                             sizeof(struct crypto_task));
    hal_crypto_start_hash(&task);

    if (hal_crypto_poll_finish(ALG_HASH_ACCELERATOR, timeout)) {
        pr_err("SHA run timeout.\n");
        return -ETIMEDOUT;
    }
    hal_crypto_pending_clear(ALG_HASH_ACCELERATOR);

    if (hal_crypto_get_err(ALG_HASH_ACCELERATOR)) {
        pr_err("SHA run error.\n");
        return -RT_ERROR;
    }
    aicos_dma_sync();
    aicos_dcache_invalid_range((void *)(unsigned long)out, SHA_MAX_OUTPUT_LEN);
    memcpy(output, out, priv->digest_len);

    *out_size = priv->digest_len;

    if (in)
        aicos_free_align(0, in);

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

    if ((symmetric_info->length % 16) != 0) {
        pr_err("Error: Data len(%d) must be aligned to 16 bytes\n", symmetric_info->length);
        return -RT_EINVAL;
    }

    if (AES_CHECK_OPSIZE(symmetric_ctx->key_bitlen >> 3)) {
        pr_err("opsize %d error\n", symmetric_ctx->key_bitlen >> 3);
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

    if (ref_count++ == 0) {
        res = hal_crypto_init();
        if (res)
            res = -RT_ERROR;
    }

    return res;
}

void drv_rsa_uninit(struct rt_hwcrypto_ctx *ctx)
{
    if (--ref_count <= 0) {
        ref_count = 0;
        hal_crypto_deinit();
    }
}

#define biL    (1 << 3)
size_t hw_bignum_clz(const u8 x)
{
    size_t j;
    u8 mask = 1 << (biL - 1);

    if (x == 0) {
        return biL;
    }

    for( j = 0; j < biL; j++ ) {
        if( x & mask )
            break;
        mask >>= 1;
    }

    return j;
}
size_t hw_bignum_mpi_bitlen(const struct hw_bignum_mpi *x)
{
    size_t i, j;

    if (x->total == 0)
        return 0;

    for (i = x->total - 1; i > 0; i--)
        if (x->p[i] != 0)
            break;

    j = biL - hw_bignum_clz(x->p[i]);

    return((i * biL) + j);
}

static s32 rsa_calc(void *mod, void *prime, void *src, u32 mod_size, u32 prime_size, u32 src_size, void *out)
{
    struct crypto_task task __attribute__((aligned(CACHE_LINE_SIZE)));
    u8 *pn, *pp, *data, *pout;
    u32 opsize, timeout = 300 * 1000;
    int ret = 0;

    /* Use aligned buffer to CE */
    pp = aicos_malloc_align(0, CE_WORK_BUF_LEN, CACHE_LINE_SIZE);
    if (pp == NULL) {
        pr_err("malloc aligned buf failed.\n");
        return -1;
    }
    memset(pp, 0, CE_WORK_BUF_LEN);
    opsize = mod_size;
    if (opsize * 4 > CE_WORK_BUF_LEN) {
        pr_err("opsize too large: %u, exceeds buffer limit\n", opsize);
        ret = -1;
        goto out;
    }

    pn = pp + opsize;
    data = pn + opsize;
    pout = data + opsize;

    memcpy(data, src, src_size);
    memcpy(pn, mod, mod_size);
    memcpy(pp, prime, prime_size);

    memset(&task, 0, sizeof(task));
    task.alg.rsa.alg_tag = ALG_RSA;

    task.alg.rsa.op_siz = select_opsize(opsize);
    task.alg.rsa.m_addr = (u32)(uintptr_t)pn;
    task.alg.rsa.d_e_addr = (u32)(uintptr_t)pp;
    task.data.in_addr = (u32)(uintptr_t)data;
    task.data.in_len = opsize;
    task.data.out_addr = (u32)(uintptr_t)pout;
    task.data.out_len = opsize;

    aicos_dcache_clean_range((void *)(unsigned long)pp, CE_WORK_BUF_LEN);
    aicos_dcache_clean_range((void *)(unsigned long)&task, sizeof(task));

    hal_crypto_start_asym(&task);

    if (hal_crypto_poll_finish(ALG_AK_ACCELERATOR, timeout)) {
        pr_err("RSA run timeout.\n");
        ret = -ETIMEDOUT;
        goto out;
    }
    hal_crypto_pending_clear(ALG_AK_ACCELERATOR);

    ret = hal_crypto_get_err(ALG_AK_ACCELERATOR);
    if (ret) {
        pr_err("RSA run error, ret=0x%x.\n", ret);
        goto out;
    }

    aicos_dma_sync();
    aicos_dcache_invalid_range((void *)(unsigned long)pout, opsize);
    memcpy(out, pout, opsize);

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
    int ret = 0;

    if (RSA_CHECK_OPSIZE(hw_bignum_mpi_bitlen(c))) {
        pr_err("opsize %d error\n", hw_bignum_mpi_bitlen(c));
        return -1;
    }

    if ((x->total == 0) || (x->p = NULL)) {
        x->total = c->total;
        x->sign = c->sign;
        x->p = rt_malloc(c->total);
        memset(x->p, 0, x->total);
    }

    ret = rsa_calc(c->p, b->p, a->p, (hw_bignum_mpi_bitlen(c) + 7) / 8,
                                     (hw_bignum_mpi_bitlen(b) + 7) / 8,
                                     (hw_bignum_mpi_bitlen(a) + 7) / 8,
                                     x->p);

    x->total = ((hw_bignum_mpi_bitlen(x) + 7) / 8);

    return ret;
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
            ctx->contex = aicos_malloc_align(0, sizeof(aic_sha_context_t), CACHE_LINE_SIZE);
            if (!ctx->contex) {
                pr_err("malloc hash context failed.\n");
                break;
            }
            memset(ctx->contex, 0, sizeof(aic_sha_context_t));

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
