/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <hal_ce.h>
#include <ssram.h>
#include "akcipher.h"
#include "rsa.h"

#define FLG_ENC  BIT(0)
#define FLG_DEC  BIT(1)
#define FLG_SIGN BIT(2)
#define FLG_VERI BIT(3)

#define FLG_KEYOFFSET (16)
#define FLG_KEYMASK   GENMASK(19, 16) /* eFuse Key ID */

struct aic_akcipher_alg {
    struct akcipher_alg alg;
};

struct aic_akcipher_tfm_ctx {
    unsigned char *n;
    unsigned char *e;
    unsigned char *d;
    unsigned int n_sz;
    unsigned int e_sz;
    unsigned int d_sz;
};

struct aic_akcipher_req_ctx {
    struct crypto_task *task;
    unsigned char *wbuf;
    unsigned int ssram_addr;
    int tasklen;
    unsigned int wbuf_size;
    unsigned long flags;
};

static inline bool is_need_genhsk(unsigned long flg)
{
    return (flg & (FLG_KEYMASK));
}

static inline unsigned int rsa_key_size(unsigned int val)
{
    return val & (~0xF);
}

static int aic_rsa_task_cfg(struct akcipher_request *req)
{
    struct aic_akcipher_req_ctx *rctx;
    unsigned int in, out, n, prim;

    pr_debug("%s\n", __func__);
    rctx = akcipher_request_ctx(req);
    rctx->tasklen = sizeof(struct crypto_task);
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    memset(rctx->task, 0, rctx->tasklen);

    prim = (u32)(uintptr_t)rctx->wbuf;
    n = (u32)(uintptr_t)rctx->wbuf + req->src_len;
    in = (u32)(uintptr_t)rctx->wbuf + req->src_len * 2;
    out = (u32)(uintptr_t)rctx->wbuf + req->src_len * 3;

    rctx->task->alg.rsa.alg_tag = ALG_RSA;
    if (req->src_len == 256) {
        rctx->task->alg.rsa.op_siz = KEY_SIZE_2048;
    } else if (req->src_len == 128) {
        rctx->task->alg.rsa.op_siz = KEY_SIZE_1024;
    } else if (req->src_len == 64) {
        rctx->task->alg.rsa.op_siz = KEY_SIZE_512;
    } else {
        pr_err("Not supported key size %d.\n", req->src_len);
        return -1;
    }

    if (rctx->ssram_addr) {
        rctx->task->alg.rsa.d_e_addr = rctx->ssram_addr;
        rctx->task->alg.rsa.m_addr = rctx->ssram_addr + req->src_len;
    } else {
        rctx->task->alg.rsa.d_e_addr = prim;
        rctx->task->alg.rsa.m_addr = n;
    }

    rctx->task->data.in_addr = in;
    rctx->task->data.in_len = (u32)req->src_len;
    rctx->task->data.out_addr = out;
    rctx->task->data.out_len = (u32)req->dst_len;
    rctx->task->next = 0;

    aicos_dcache_clean_range((void *)(uintptr_t)rctx->wbuf, rctx->wbuf_size);
    aicos_dcache_clean_range((void *)(uintptr_t)rctx->task, rctx->tasklen);

    return 0;
}

static void aic_akcipher_rsa_clear_key(struct aic_akcipher_tfm_ctx *ctx);
static int aic_akcipher_unprepare_req(struct akcipher_tfm *tfm,
                                      struct akcipher_request *req)
{
    struct aic_akcipher_tfm_ctx *ctx;
    struct aic_akcipher_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    ctx = akcipher_tfm_ctx(tfm);
    rctx = akcipher_request_ctx(req);

    if (rctx->task) {
        aicos_free_align(0, rctx->task);
        rctx->task = NULL;
        rctx->tasklen = 0;
    }
    if (rctx->wbuf) {
        aicos_free_align(0, rctx->wbuf);
        rctx->wbuf = NULL;
        rctx->wbuf_size = 0;
    }
    if (rctx->ssram_addr) {
        aic_ssram_free(rctx->ssram_addr, ctx->n_sz * 3);
        rctx->ssram_addr = 0;
    }

    aic_akcipher_rsa_clear_key(ctx);

    return 0;
}

static int aic_akcipher_prepare_req(struct akcipher_tfm *tfm,
                                    struct akcipher_request *req)
{
    struct aic_akcipher_tfm_ctx *ctx;
    struct aic_akcipher_req_ctx *rctx;
    unsigned char *p, *n, *data;
    unsigned int key_size;
    int ret;

    pr_debug("%s\n", __func__);
    ctx = akcipher_tfm_ctx(tfm);
    rctx = akcipher_request_ctx(req);

    key_size = rsa_key_size(ctx->n_sz);
    rctx->wbuf_size = key_size * 4; /* 3 operands + 1 output */
    rctx->wbuf = aicos_malloc_align(0, rctx->wbuf_size, CACHE_LINE_SIZE);

    if (!rctx->wbuf) {
        pr_err("Failed to alloc work buffer.\n");
        return -ENOMEM;
    }

    /* copy d/e, n, data to working buffer */
    p = rctx->wbuf;
    n = p + key_size;
    data = n + key_size;
    memcpy(data, req->src, req->src_len);
    hal_crypto_bignum_byteswap(data, req->src_len);

    if (ctx->d)
        hal_crypto_bignum_be2le(ctx->d, ctx->d_sz, p, key_size);
    if (ctx->e)
        hal_crypto_bignum_be2le(ctx->e, ctx->e_sz, p, key_size);
    hal_crypto_bignum_be2le(ctx->n, ctx->n_sz, n, key_size);

    if (DEBUG_CE)
        hexdump_msg("wbuf: ", (unsigned char *)rctx->wbuf, rctx->wbuf_size, 1);

    rctx->ssram_addr = 0;
    if (is_need_genhsk(rctx->flags)) {
        unsigned int km_siz;
        int efuse;

        km_siz = key_size * 2;
        rctx->ssram_addr = aic_ssram_alloc(km_siz);
        if (!rctx->ssram_addr) {
            pr_err("Failed to allocate ssram key\n");
            ret = -ENOMEM;
            goto err;
        }
        efuse = (rctx->flags & FLG_KEYMASK) >> FLG_KEYOFFSET;

        ret = aic_ssram_des_genkey(efuse, rctx->wbuf, km_siz, rctx->ssram_addr);
        if (ret) {
            pr_err("Failed to gen hsk\n");
            goto err;
        }
    }

    ret = aic_rsa_task_cfg(req);
    if (ret) {
        pr_err("Failed to cfg task\n");
        goto err;
    }

    return 0;

err:
    aic_akcipher_unprepare_req(tfm, req);
    return ret;
}

static int aic_akcipher_do_one_req(struct akcipher_request *req)
{
    struct aic_akcipher_req_ctx *rctx;
    unsigned char *outbuf;

    pr_debug("%s\n", __func__);
    rctx = akcipher_request_ctx(req);

    if (!hal_crypto_is_start()) {
        pr_err("Crypto engine is busy.\n");
        return -EBUSY;
    }

    if (DEBUG_CE)
        hal_crypto_dump_task(rctx->task, rctx->tasklen);

    hal_crypto_init();
    hal_crypto_irq_enable(ALG_AK_ACCELERATOR);
    hal_crypto_start_asym(rctx->task);
    while (!hal_crypto_poll_finish(ALG_AK_ACCELERATOR)) {
        continue;
    }
    hal_crypto_pending_clear(ALG_AK_ACCELERATOR);

    if (hal_crypto_get_err(ALG_AK_ACCELERATOR)) {
        pr_err("AK ACCELERATOR run error.\n");
        return -1;
    }

    aicos_dma_sync();
    outbuf = rctx->wbuf + 3 * req->dst_len;
    aicos_dcache_invalid_range((void *)(uintptr_t)outbuf, req->dst_len);
    memcpy(req->dst, outbuf, req->dst_len);
    hal_crypto_bignum_byteswap(req->dst, req->dst_len);

    if (DEBUG_CE)
        hal_crypto_dump_reg();

    hal_crypto_deinit();

    return 0;
}

static void aic_akcipher_rsa_clear_key(struct aic_akcipher_tfm_ctx *ctx)
{
    if (ctx->n)
        aicos_free_align(0, ctx->n);
    if (ctx->d)
        aicos_free_align(0, ctx->d);
    if (ctx->e)
        aicos_free_align(0, ctx->e);
    ctx->n = NULL;
    ctx->e = NULL;
    ctx->d = NULL;
}

static int aic_akcipher_rsa_crypt(struct aic_akcipher_handle *handle,
                                  unsigned long flag)
{
    struct aic_akcipher_tfm_ctx *ctx;
    struct aic_akcipher_req_ctx *rctx;
    struct akcipher_tfm *tfm;
    struct akcipher_request *req;
    unsigned int key_size;
    int ret;

    pr_debug("%s\n", __func__);
    tfm = akcipher_handle_tfm(handle);
    req = akcipher_handle_req(handle);
    ctx = akcipher_tfm_ctx(tfm);
    rctx = akcipher_request_ctx(req);

    if (!ctx) {
        pr_err("aic akcipher, device is null\n");
        return -ENODEV;
    }

    rctx->flags = flag;

    key_size = rsa_key_size(ctx->n_sz);
    if (req->src_len != key_size || req->dst_len != key_size) {
        pr_err("src length is not the same with key size\n");
        return -EINVAL;
    }

    ret = aic_akcipher_prepare_req(tfm, req);
    if (ret) {
        pr_err("akcipher prepare req failed.\n");
        return ret;
    }
    ret = aic_akcipher_do_one_req(req);
    if (ret) {
        pr_err("akcipher do one req failed.\n");
        return ret;
    }
    ret = aic_akcipher_unprepare_req(tfm, req);
    if (ret) {
        pr_err("akcipher unprepare req failed.\n");
        return ret;
    }

    return ret;
}

static int aic_akcipher_rsa_encrypt(struct aic_akcipher_handle *handle)
{
    return aic_akcipher_rsa_crypt(handle, FLG_ENC);
}

static int aic_akcipher_rsa_decrypt(struct aic_akcipher_handle *handle)
{
    return aic_akcipher_rsa_crypt(handle, FLG_DEC);
}

static int aic_akcipher_pnk_rsa_encrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_ENC | (CE_KEY_SRC_PNK << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_pnk_rsa_decrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_DEC | (CE_KEY_SRC_PNK << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk0_rsa_encrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_ENC | (CE_KEY_SRC_PSK0 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk0_rsa_decrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_DEC | (CE_KEY_SRC_PSK0 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk1_rsa_encrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_ENC | (CE_KEY_SRC_PSK1 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk1_rsa_decrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_DEC | (CE_KEY_SRC_PSK1 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk2_rsa_encrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_ENC | (CE_KEY_SRC_PSK2 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk2_rsa_decrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_DEC | (CE_KEY_SRC_PSK2 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk3_rsa_encrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_ENC | (CE_KEY_SRC_PSK3 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_psk3_rsa_decrypt(struct aic_akcipher_handle *handle)
{
    unsigned long flags;

    flags = FLG_DEC | (CE_KEY_SRC_PSK3 << FLG_KEYOFFSET);
    return aic_akcipher_rsa_crypt(handle, flags);
}

static int aic_akcipher_rsa_set_pub_key(struct aic_akcipher_handle *handle,
                                        const void *key, unsigned int keylen)
{
    struct akcipher_tfm *tfm = handle->tfm;
    struct aic_akcipher_tfm_ctx *ctx = akcipher_tfm_ctx(tfm);
    struct rsa_key rsa_key;
    int ret;

    pr_debug("%s\n", __func__);
    aic_akcipher_rsa_clear_key(ctx);
    if (DEBUG_CE)
        hexdump_msg("pubkey: ", (unsigned char *)key, keylen, 1);
    ret = rsa_parse_pub_key(&rsa_key, key, keylen);
    if (ret) {
        pr_err("Parse pub key failed.\n");
        goto err;
    }

    if (DEBUG_CE) {
        hexdump_msg("n: ", (unsigned char *)rsa_key.n, rsa_key.n_sz, 1);
        // hexdump_msg("d: ", (unsigned char *)rsa_key.d, rsa_key.d_sz, 1);
        hexdump_msg("e: ", (unsigned char *)rsa_key.e, rsa_key.e_sz, 1);
    }
    ctx->n = aicos_memdup_align(0, (void *)rsa_key.n,
                                ALIGN_UP(rsa_key.n_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->n) {
        pr_err("Copy RSA Key N failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->n_sz = rsa_key.n_sz;
    ctx->e = aicos_memdup_align(0, (void *)rsa_key.e,
                                ALIGN_UP(rsa_key.e_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->e) {
        pr_err("Copy RSA Key E failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->e_sz = rsa_key.e_sz;
    return 0;
err:
    if (ctx->n)
        aicos_free_align(0, ctx->n);
    if (ctx->e)
        aicos_free_align(0, ctx->e);
    ctx->n = NULL;
    ctx->e = NULL;
    return ret;
}

static int aic_akcipher_rsa_set_priv_key(struct aic_akcipher_handle *handle,
                                         const void *key, unsigned int keylen)
{
    struct akcipher_tfm *tfm = handle->tfm;
    struct aic_akcipher_tfm_ctx *ctx = akcipher_tfm_ctx(tfm);
    struct rsa_key rsa_key;
    int ret;

    pr_debug("%s\n", __func__);
    aic_akcipher_rsa_clear_key(ctx);
    if (DEBUG_CE)
        hexdump_msg("privkey: ", (unsigned char *)key, keylen, 1);

    ret = rsa_parse_priv_key(&rsa_key, key, keylen);
    if (ret) {
        pr_err("parse priv key error.\n");
        goto err;
    }

    if (DEBUG_CE) {
        hexdump_msg("n: ", (unsigned char *)rsa_key.n, rsa_key.n_sz, 1);
        hexdump_msg("d: ", (unsigned char *)rsa_key.d, rsa_key.d_sz, 1);
        // hexdump_msg("e: ", (unsigned char *)rsa_key.e, rsa_key.e_sz, 1);
    }
    ctx->n = aicos_memdup_align(0, (void *)rsa_key.n,
                                ALIGN_UP(rsa_key.n_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->n) {
        pr_err("Copy RSA Key N failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->n_sz = rsa_key.n_sz;
    ctx->d = aicos_memdup_align(0, (void *)rsa_key.d,
                                ALIGN_UP(rsa_key.d_sz, CACHE_LINE_SIZE),
                                CACHE_LINE_SIZE);
    if (!ctx->d) {
        pr_err("Copy RSA Key D failed.\n");
        ret = -ENOMEM;
        goto err;
    }
    ctx->d_sz = rsa_key.d_sz;
    return 0;
err:
    if (ctx->n)
        aicos_free_align(0, ctx->n);
    if (ctx->e)
        aicos_free_align(0, ctx->e);
    ctx->n = NULL;
    ctx->e = NULL;
    return ret;
}

static unsigned int aic_akcipher_rsa_max_size(struct aic_akcipher_handle *handle)
{
    struct akcipher_tfm *tfm = handle->tfm;
    struct aic_akcipher_tfm_ctx *ctx = akcipher_tfm_ctx(tfm);

    return rsa_key_size(ctx->n_sz);
}

static struct aic_akcipher_alg ak_algs[] = {
{
	.alg = {
		.encrypt = aic_akcipher_rsa_encrypt,
		.decrypt = aic_akcipher_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "rsa",
			.cra_driver_name = "rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_pnk_rsa_encrypt,
		.decrypt = aic_akcipher_pnk_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "pnk-protected(rsa)",
			.cra_driver_name = "pnk-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk0_rsa_encrypt,
		.decrypt = aic_akcipher_psk0_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk0-protected(rsa)",
			.cra_driver_name = "psk0-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk1_rsa_encrypt,
		.decrypt = aic_akcipher_psk1_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk1-protected(rsa)",
			.cra_driver_name = "psk1-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk2_rsa_encrypt,
		.decrypt = aic_akcipher_psk2_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk2-protected(rsa)",
			.cra_driver_name = "psk2-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
{
	.alg = {
		.encrypt = aic_akcipher_psk3_rsa_encrypt,
		.decrypt = aic_akcipher_psk3_rsa_decrypt,
		.set_pub_key = aic_akcipher_rsa_set_pub_key,
		.set_priv_key = aic_akcipher_rsa_set_priv_key,
		.max_size = aic_akcipher_rsa_max_size,
		.reqsize = sizeof(struct aic_akcipher_req_ctx),
		.base = {
			.cra_name = "psk3-protected(rsa)",
			.cra_driver_name = "psk3-protected-rsa-aic",
			.cra_priority = 400,
			.cra_ctxsize = sizeof(struct aic_akcipher_tfm_ctx),
		},
	},
},
};

void aic_akcipher_destroy(struct aic_akcipher_handle *handle)
{
    if (handle->req->__ctx) {
        aicos_free(0, handle->req->__ctx);
        handle->req->__ctx = NULL;
    }

    if (handle->req) {
        aicos_free(0, handle->req);
        handle->req = NULL;
    }

    if (handle->tfm->__crt_ctx) {
        aicos_free(0, handle->tfm->__crt_ctx);
        handle->tfm->__crt_ctx = NULL;
    }

    if (handle->tfm) {
        aicos_free(0, handle->tfm);
        handle->tfm = NULL;
    }

    if (handle) {
        aicos_free(0, handle);
        handle = NULL;
    }
}

struct aic_akcipher_handle *aic_akcipher_init(const char *ciphername, u32 flags)
{
    struct aic_akcipher_handle *handle = NULL;
    struct aic_akcipher_tfm_ctx *ctx = NULL;
    struct aic_akcipher_req_ctx *rctx = NULL;
    struct akcipher_request *req = NULL;
    struct akcipher_tfm *tfm = NULL;

    handle = aicos_malloc(0, sizeof(struct aic_akcipher_handle));
    if (handle == NULL) {
        pr_err("malloc handle failed.\n");
        return NULL;
    }
    memset(handle, 0, sizeof(struct aic_akcipher_handle));

    tfm = aicos_malloc(0, sizeof(struct akcipher_tfm));
    if (tfm == NULL) {
        pr_err("malloc tfm failed.\n");
        goto out;
    }
    memset(tfm, 0, sizeof(struct akcipher_tfm));
    handle->tfm = tfm;

    ctx = aicos_malloc(0, sizeof(struct aic_akcipher_tfm_ctx));
    if (ctx == NULL) {
        pr_err("malloc ctx failed.\n");
        goto out;
    }
    memset(ctx, 0, sizeof(struct aic_akcipher_tfm_ctx));
    tfm->__crt_ctx = (void *)ctx;

    req = aicos_malloc(0, sizeof(struct akcipher_request));
    if (req == NULL) {
        pr_err("malloc req failed.\n");
        goto out;
    }
    memset(req, 0, sizeof(struct akcipher_request));
    handle->req = req;

    rctx = aicos_malloc(0, sizeof(struct aic_akcipher_req_ctx));
    if (rctx == NULL) {
        pr_err("malloc rctx failed.\n");
        goto out;
    }
    memset(rctx, 0, sizeof(struct aic_akcipher_req_ctx));
    req->__ctx = (void *)rctx;

    for (int i = 0; i < ARRAY_SIZE(ak_algs); i++) {
        if (!strcmp(ak_algs[i].alg.base.cra_name, ciphername) ||
            !strcmp(ak_algs[i].alg.base.cra_driver_name, ciphername)) {
            tfm->alg = &ak_algs[i].alg;
            return handle;
        }
    }
    pr_warn("not found %s algo.\n", ciphername);

out:
    aic_akcipher_destroy(handle);

    return NULL;
}
