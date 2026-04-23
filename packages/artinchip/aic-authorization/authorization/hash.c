/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <hal_ce.h>
#include <ssram.h>
#include "hash.h"

#define CE_TOTAL_BITLEN_SIZE	32
#define MD5_CE_OUTPUT_LEN	    16
#define SM3_CE_OUTPUT_LEN       32
#define SHA1_CE_OUTPUT_LEN	    20
#define SHA224_CE_OUTPUT_LEN	32
#define SHA256_CE_OUTPUT_LEN	32
#define SHA384_CE_OUTPUT_LEN	64
#define SHA512_CE_OUTPUT_LEN	64

#define FLG_SM3         BIT(0)
#define FLG_HMAC		BIT(1)
#define FLG_MD5			BIT(2)
#define FLG_SHA1		BIT(3)
#define FLG_SHA224		BIT(4)
#define FLG_SHA256		BIT(5)
#define FLG_SHA384		BIT(6)
#define FLG_SHA512		BIT(7)
#define FLG_FIRST		BIT(8)
#define FLG_UPDATE		BIT(9)
#define FLG_FINAL		BIT(10)

#define aligned_addr64bit(ptr)	(((((dma_addr_t)(ptr)) + 7) >> 3) << 3)

struct aic_hash_tfm_ctx {
    bool sm3;
    bool hmac;
    void *remain_buf;
    unsigned int remain_len;
    unsigned total_len;
};

struct aic_hash_alg {
    struct hash_alg alg;
};

struct aic_hash_req_ctx {
    struct crypto_task *task;
    unsigned char *ivbuf;
    void *src_cpy_buf;
    int src_cpy_buf_len;
    int tasklen;
    int blocksize;
    unsigned int digest_size;
    unsigned long flags;
    unsigned char digest[CE_MAX_DIGEST_SIZE];
};

static inline bool is_sm3(unsigned long flg)
{
    return (flg & FLG_SM3);
}

static inline bool is_hmac(unsigned long flg)
{
    return (flg & FLG_HMAC);
}

static inline bool is_md5(unsigned long flg)
{
    return (flg & FLG_MD5);
}

static inline bool is_sha1(unsigned long flg)
{
    return (flg & FLG_SHA1);
}

static inline bool is_sha224(unsigned long flg)
{
    return (flg & FLG_SHA224);
}

static inline bool is_sha256(unsigned long flg)
{
    return (flg & FLG_SHA256);
}

static inline bool is_sha384(unsigned long flg)
{
    return (flg & FLG_SHA384);
}

static inline bool is_sha512(unsigned long flg)
{
    return (flg & FLG_SHA512);
}

static inline bool is_hmacsha1(unsigned long flg)
{
    return (flg & FLG_HMAC && (flg) & FLG_SHA1);
}

static inline bool is_hmacsha256(unsigned long flg)
{
    return (flg & FLG_HMAC && (flg) & FLG_SHA256);
}

static inline bool is_first(unsigned long flg)
{
    return (flg & FLG_FIRST);
}

static inline bool is_final(unsigned long flg)
{
    return (flg & FLG_FINAL);
}

static void aic_hash_task_cfg(struct crypto_task *task,
			      struct aic_hash_req_ctx *rctx, u32 din,
			      u32 iv_addr, u32 dout, u32 dlen,
			      u32 total, u32 last_flag)
{
    if (last_flag)
        task->data.total_bytelen = total;

    task->data.last_flag = last_flag;
    task->data.in_addr = cpu_to_le32(din);
    task->data.in_len = cpu_to_le32(dlen);
    task->data.out_addr = cpu_to_le32(dout);
    task->alg.hash.iv_mode = 1;
    task->alg.hash.iv_addr = cpu_to_le32(iv_addr);

    if (is_sm3(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_SM3;
        task->data.out_len = SM3_CE_OUTPUT_LEN;
    } else if (is_md5(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_MD5;
        task->data.out_len = MD5_CE_OUTPUT_LEN;
    } else if (is_sha1(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_SHA1;
        task->data.out_len = SHA1_CE_OUTPUT_LEN;
    } else if (is_sha224(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_SHA224;
        task->data.out_len = SHA224_CE_OUTPUT_LEN;
    } else if (is_sha256(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_SHA256;
        task->data.out_len = SHA256_CE_OUTPUT_LEN;
    } else if (is_sha384(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_SHA384;
        task->data.out_len = SHA384_CE_OUTPUT_LEN;
    } else if (is_sha512(rctx->flags)) {
        task->alg.hash.alg_tag = ALG_SHA512;
        task->data.out_len = SHA512_CE_OUTPUT_LEN;
    }

    aicos_dcache_clean_range((void *)(uintptr_t)din, dlen);
    aicos_dcache_clean_range((void *)(uintptr_t)iv_addr, rctx->digest_size);
    aicos_dcache_clean_range((void *)(uintptr_t)task, sizeof(struct crypto_task));
}

static int prepare_task_with_src_buf(struct hash_tfm *tfm, struct hash_request *req)
{
    unsigned int bytelen, remain, todo, cpycnt, chunk = CE_CIPHER_MAX_DATA_SIZE;
    unsigned int din, dout, next_addr;
    struct aic_hash_tfm_ctx *ctx;
    struct aic_hash_req_ctx *rctx;
    struct crypto_task *task;
    int i, task_cnt;
    u32 last_task, final;

    pr_debug("%s\n", __func__);
    ctx = hash_tfm_ctx(tfm);
    rctx = hash_request_ctx(req);

    if (is_final(rctx->flags))
        todo = req->nbytes;
    else
        todo = rounddown(req->nbytes, rctx->blocksize);

    task_cnt = DIV_ROUND_UP(req->nbytes, chunk);

    cpycnt = req->nbytes - todo;
    if (cpycnt) {
        /* Backup not block aligned tail data in remain buffer, if this
         * is not final step
         */
        memcpy(ctx->remain_buf, req->src + todo, cpycnt);
        ctx->remain_len = cpycnt;
    }
    if (0 == todo)
        return 0;

    rctx->tasklen = sizeof(struct crypto_task) * task_cnt;
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;

    memset(rctx->task, 0, rctx->tasklen);
    remain = todo;
    dout = (u32)(uintptr_t)rctx->ivbuf;
    final = 0;
    for (i = 0; i < task_cnt; i++) {
        task = &rctx->task[i];
        next_addr = (u32)(uintptr_t)rctx->task + ((i + 1) * sizeof(*task));

        bytelen = min(remain, chunk);
        remain -= bytelen;
        ctx->total_len += bytelen;
        last_task = (remain == 0);

        din = (u32)(uintptr_t)req->src + (i * chunk);
        if (last_task)
            task->next = 0;
        else
            task->next = cpu_to_le32(next_addr);
        aic_hash_task_cfg(task, rctx, din, dout, dout, bytelen, ctx->total_len, final);
    }

    return 0;
}

static int prepare_task_with_src_cpy_buf(struct hash_tfm *tfm, struct hash_request *req)
{
    struct aic_hash_tfm_ctx *ctx;
    struct aic_hash_req_ctx *rctx;
    unsigned int total, todo;
    unsigned char *p;

    pr_debug("%s\n", __func__);

    ctx = hash_tfm_ctx(tfm);
    rctx = hash_request_ctx(req);

    total = ctx->remain_len + req->nbytes;
    todo = rounddown(total, rctx->blocksize);
    if (total < rctx->blocksize && is_final(rctx->flags) == false) {
        /* Not enough data to start CE, backup data in remain buffer */
        p = ctx->remain_buf;
        p += ctx->remain_len;
        memcpy(p, req->src, req->nbytes);
        ctx->remain_len = total;

        return 0;
    }
    if (total > 0) {
        /* Final step or there is enough data to be processed */
        rctx->src_cpy_buf = aicos_malloc_align(0, total, CACHE_LINE_SIZE);
        if (!rctx->src_cpy_buf) {
            pr_err("Failed to allocate space for src.\n");
            return -ENOMEM;
        }
        rctx->src_cpy_buf_len = total;
        p = rctx->src_cpy_buf;
        if (ctx->remain_len) {
            memcpy(p, ctx->remain_buf, ctx->remain_len);
            p += ctx->remain_len;
        }
        if (is_final(rctx->flags)) {
            /* If this is a final step, process all data */
            ctx->remain_len = 0;
            todo = total;
        } else {
            memcpy(p, req->src, req->nbytes);
            /* If this is not final step, backup the tail data */
            ctx->remain_len = total % rctx->blocksize;
            if (ctx->remain_len) {
                p = rctx->src_cpy_buf;
                p += todo;
                memcpy(ctx->remain_buf, p, ctx->remain_len);
            }
        }
    } else {
        rctx->src_cpy_buf = NULL;
    }

    rctx->tasklen = sizeof(struct crypto_task);
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task)
        return -ENOMEM;
    memset(rctx->task, 0, rctx->tasklen);

    ctx->total_len += todo;
    aic_hash_task_cfg(rctx->task, rctx, (u32)(uintptr_t)rctx->src_cpy_buf,
                      (u32)(uintptr_t)rctx->ivbuf, (u32)(uintptr_t)rctx->ivbuf,
                      todo, ctx->total_len, is_final(rctx->flags));
    if (is_final(rctx->flags)) {
        ctx->total_len = 0;
    }

    return 0;
}

static inline bool is_hash_block_aligned(unsigned int val, unsigned long flg)
{
    if (is_sm3(flg)) {
        if (val % SM3_BLOCK_SIZE)
            return false;
        return true;
    }
    if (is_md5(flg)) {
        if (val % MD5_BLOCK_SIZE)
            return false;
        return true;
    }
    if (is_sha1(flg)) {
        if (val % SHA1_BLOCK_SIZE)
            return false;
        return true;
    }
    if (is_sha224(flg)) {
        if (val % SHA224_BLOCK_SIZE)
            return false;
        return true;
    }
    if (is_sha256(flg)) {
        if (val % SHA256_BLOCK_SIZE)
            return false;
        return true;
    }
    if (is_sha384(flg)) {
        if (val % SHA384_BLOCK_SIZE)
            return false;
        return true;
    }
    if (is_sha512(flg)) {
        if (val % SHA512_BLOCK_SIZE)
            return false;
        return true;
    }

    return false;
}

static inline bool can_use_src_buf(struct hash_request *req)
{
	struct aic_hash_req_ctx *rctx;

	if (!req->src)
		return false;

	rctx = hash_request_ctx(req);

	if (!is_hash_block_aligned((u32)(uintptr_t)req->src, rctx->flags)) {
        pr_debug("%s, offset(0x%x) is not aligned.\n", __func__, (u32)(uintptr_t)req->src);
		return false;
    }

	/* Only one task buffer, but the data length is not block aligned,
	 * cannot be used directly
	 */
	if (!is_hash_block_aligned(req->nbytes, rctx->flags)) {
        pr_debug("%s, nbytes(0x%x) is not aligned.\n", __func__, req->nbytes);
		return false;
    }

	return true;
}

static int aic_hash_unprepare_req(struct hash_tfm *tfm, struct hash_request *req)
{
    struct aic_hash_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    rctx = hash_request_ctx(req);

    if (rctx->task) {
        aicos_free_align(0, rctx->task);
        rctx->task = NULL;
        rctx->tasklen = 0;
    }

    if (rctx->src_cpy_buf) {
        aicos_free_align(0, rctx->src_cpy_buf);
        rctx->src_cpy_buf = NULL;
    }

    if (rctx->ivbuf) {
        aicos_free_align(0, rctx->ivbuf);
        rctx->ivbuf = NULL;
    }

    return 0;
}

static int aic_hash_prepare_req(struct hash_tfm *tfm, struct hash_request *req)
{
    struct aic_hash_tfm_ctx *ctx;
    struct aic_hash_req_ctx *rctx;
    int ret;

    pr_debug("%s\n", __func__);
    rctx = hash_request_ctx(req);
    ctx = hash_tfm_ctx(tfm);
    rctx = hash_request_ctx(req);

    if (is_first(rctx->flags))
        rctx->ivbuf = aicos_memdup_align(0, rctx->digest, CE_MAX_DIGEST_SIZE, CACHE_LINE_SIZE);
    if (!rctx->ivbuf) {
        ret = -ENOMEM;
        pr_err("No mem for ivbuf\n");
        goto err;
    }

    rctx->blocksize = hash_tfm_blocksize(tfm);
    if (ctx->remain_len == 0 && can_use_src_buf(req))
        ret = prepare_task_with_src_buf(tfm, req);
    else
        ret = prepare_task_with_src_cpy_buf(tfm, req);

    if (ret) {
        pr_err("Failed to prepare task\n");
        goto err;
    }

    return 0;

err:
    aic_hash_unprepare_req(tfm, req);
    return ret;
}

static int aic_hash_do_one_req(struct hash_tfm *tfm, struct hash_request *req)
{
    struct aic_hash_req_ctx *rctx;
    u32 timeout = 300 * 1000;

    pr_debug("%s\n", __func__);
    rctx = hash_request_ctx(req);

    if (!rctx->task) {
        /* Not enough data to start CE, just finalize current request */
        return 0;
    }

    if (!hal_crypto_is_start()) {
        pr_err("Crypto engine is busy.\n");
        return -EBUSY;
    }

    if (DEBUG_CE)
        hal_crypto_dump_task(rctx->task, rctx->tasklen);

    hal_crypto_init();
    hal_crypto_irq_enable(ALG_HASH_ACCELERATOR);
    hal_crypto_start_hash(rctx->task);
    if (hal_crypto_poll_finish(ALG_HASH_ACCELERATOR, timeout)) {
        pr_err("HASH ACCELERATOR run timeout.\n");
        return -ETIMEDOUT;
    }
    hal_crypto_pending_clear(ALG_HASH_ACCELERATOR);

    if (hal_crypto_get_err(ALG_HASH_ACCELERATOR)) {
        pr_err("HASH ACCELERATOR run error, ret:%d.\n", hal_crypto_get_err(ALG_HASH_ACCELERATOR));
        hal_crypto_err_clear(ALG_HASH_ACCELERATOR);
        return -1;
    }

    aicos_dma_sync();
    if (rctx->ivbuf) {
        aicos_dcache_invalid_range((void *)(uintptr_t)rctx->ivbuf, CE_MAX_DIGEST_SIZE);
        memcpy(rctx->digest, rctx->ivbuf, CE_MAX_DIGEST_SIZE);
    }

    if (is_final(rctx->flags) && req->result)
        memcpy(req->result, rctx->digest, rctx->digest_size);

    if (DEBUG_CE)
        hal_crypto_dump_reg();

    hal_crypto_deinit();

    return 0;
}

static int aic_hash_alg_init(struct aic_hash_handle *handle)
{
    struct hash_tfm *tfm;
    struct aic_hash_tfm_ctx *ctx;

    pr_debug("%s\n", __func__);
    tfm = hash_handle_tfm(handle);
    ctx = hash_tfm_ctx(tfm);

    ctx->remain_buf = aicos_malloc(0, hash_tfm_blocksize(tfm));
    if (!ctx->remain_buf)
        return -ENOMEM;


    return 0;
}

static int aic_sm3_alg_init(struct aic_hash_handle *handle)
{
    struct hash_tfm *tfm;
    struct aic_hash_tfm_ctx *ctx;

    pr_debug("%s\n", __func__);
    tfm = hash_handle_tfm(handle);
    ctx = hash_tfm_ctx(tfm);

    ctx->sm3 = true;
    return aic_hash_alg_init(handle);
}

static int aic_hash_hmac_alg_init(struct aic_hash_handle *handle)
{
    struct hash_tfm *tfm;
    struct aic_hash_tfm_ctx *ctx;

    pr_debug("%s\n", __func__);
    tfm = hash_handle_tfm(handle);
    ctx = hash_tfm_ctx(tfm);

    ctx->hmac = true;
    return aic_hash_alg_init(handle);
}

static void aic_hash_alg_exit(struct aic_hash_handle *handle)
{
    struct hash_tfm *tfm;
    struct aic_hash_tfm_ctx *ctx;

    pr_debug("%s\n", __func__);
    tfm = hash_handle_tfm(handle);
    ctx = hash_tfm_ctx(tfm);

    aicos_free(0, ctx->remain_buf);
}

static u32 md5_iv[] = {
    MD5_H0,
    MD5_H1,
    MD5_H2,
    MD5_H3,
};

static u32 md5_iv_len = 16;

static u32 sm3_iv[] = {
    BE_SM3_IVA,
    BE_SM3_IVB,
    BE_SM3_IVC,
    BE_SM3_IVD,
    BE_SM3_IVE,
    BE_SM3_IVF,
    BE_SM3_IVG,
    BE_SM3_IVH,
};

static u32 sm3_iv_len = 32;

static u32 sha1_iv[] = {
    BE_SHA1_H0,
    BE_SHA1_H1,
    BE_SHA1_H2,
    BE_SHA1_H3,
    BE_SHA1_H4,
};

static u32 sha1_iv_len = 20;

static u32 sha224_iv[] = {
    BE_SHA224_H0,
    BE_SHA224_H1,
    BE_SHA224_H2,
    BE_SHA224_H3,
    BE_SHA224_H4,
    BE_SHA224_H5,
    BE_SHA224_H6,
    BE_SHA224_H7,
};

static u32 sha224_iv_len = 32;

static u32 sha256_iv[] = {
    BE_SHA256_H0,
    BE_SHA256_H1,
    BE_SHA256_H2,
    BE_SHA256_H3,
    BE_SHA256_H4,
    BE_SHA256_H5,
    BE_SHA256_H6,
    BE_SHA256_H7,
};

static u32 sha256_iv_len = 32;

static u64 sha384_iv[] = {
    BE_SHA384_H0,
    BE_SHA384_H1,
    BE_SHA384_H2,
    BE_SHA384_H3,
    BE_SHA384_H4,
    BE_SHA384_H5,
    BE_SHA384_H6,
    BE_SHA384_H7,
};

static u64 sha384_iv_len = 64;

static u64 sha512_iv[] = {
    BE_SHA512_H0,
    BE_SHA512_H1,
    BE_SHA512_H2,
    BE_SHA512_H3,
    BE_SHA512_H4,
    BE_SHA512_H5,
    BE_SHA512_H6,
    BE_SHA512_H7,
};

static u64 sha512_iv_len = 64;

static int aic_hash_init(struct aic_hash_handle *handle)
{
    struct hash_tfm *tfm = hash_handle_tfm(handle);
    struct hash_request *req = hash_handle_req(handle);
    struct aic_hash_tfm_ctx *ctx = hash_tfm_ctx(tfm);
    struct aic_hash_req_ctx *rctx = hash_request_ctx(req);

    pr_debug("%s\n", __func__);
    memset(rctx, 0, sizeof(*rctx));

    rctx->flags |= FLG_FIRST;
    if (ctx->hmac)
        rctx->flags |= FLG_HMAC;
    rctx->digest_size = hash_tfm_digestsize(tfm);

    switch (rctx->digest_size) {
        case MD5_DIGEST_SIZE:
            rctx->flags |= FLG_MD5;
            memcpy(rctx->digest, md5_iv, md5_iv_len);
            break;
        case SHA1_DIGEST_SIZE:
            rctx->flags |= FLG_SHA1;
            memcpy(rctx->digest, sha1_iv, sha1_iv_len);
            break;
        case SHA224_DIGEST_SIZE:
            rctx->flags |= FLG_SHA224;
            memcpy(rctx->digest, sha224_iv, sha224_iv_len);
            break;
        case SHA256_DIGEST_SIZE:
            if (ctx->sm3) {
                rctx->flags |= FLG_SM3;
                memcpy(rctx->digest, sm3_iv, sm3_iv_len);
            } else {
                rctx->flags |= FLG_SHA256;
                memcpy(rctx->digest, sha256_iv, sha256_iv_len);
            }
            break;
        case SHA384_DIGEST_SIZE:
            rctx->flags |= FLG_SHA384;
            memcpy(rctx->digest, sha384_iv, sha384_iv_len);
            break;
        case SHA512_DIGEST_SIZE:
            rctx->flags |= FLG_SHA512;
            memcpy(rctx->digest, sha512_iv, sha512_iv_len);
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

static int aic_hash_process(struct aic_hash_handle *handle, unsigned long flg)
{
	struct hash_tfm *tfm;
	struct hash_request *req;
	struct aic_hash_req_ctx *rctx;
    int ret = 0;

    pr_debug("%s\n", __func__);
	tfm = hash_handle_tfm(handle);
	req = hash_handle_req(handle);
	rctx = hash_request_ctx(req);

	rctx->flags |= flg;

    ret += aic_hash_prepare_req(tfm, req);
	ret += aic_hash_do_one_req(tfm, req);
	ret += aic_hash_unprepare_req(tfm, req);

    return 0;
}
/*
 * one data request is incoming, need to transfer to queue
 */
static int aic_hash_update(struct aic_hash_handle *handle)
{
    pr_debug("%s\n", __func__);
	return aic_hash_process(handle, FLG_UPDATE);
}

/*
 * These is no data, just want to get the result.
 */
static int aic_hash_final(struct aic_hash_handle *handle)
{
	pr_debug("%s\n", __func__);
	return aic_hash_process(handle, FLG_FINAL);
}

/*
 * update and final
 */
static int aic_hash_finup(struct aic_hash_handle *handle)
{
	pr_debug("%s\n", __func__);
	return aic_hash_process(handle, FLG_UPDATE | FLG_FINAL);
}

static int aic_hash_setkey(struct aic_hash_handle *handle, const u8 *key, unsigned int keylen)
{
    pr_debug("%s\n", __func__);
    return 0;
}

static struct aic_hash_alg hash_algs[] = {
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.base = {
			.cra_name = "sm3",
			.cra_driver_name = "sm3-aic",
			.cra_blocksize = SM3_BLOCK_SIZE,
		    .cra_digestsize = SM3_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_sm3_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.base = {
			.cra_name = "md5",
			.cra_driver_name = "md5-aic",
			.cra_blocksize = MD5_BLOCK_SIZE,
		    .cra_digestsize = MD5_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
        .finup = aic_hash_finup,
		.base = {
			.cra_name = "sha1",
			.cra_driver_name = "sha1-aic",
			.cra_blocksize = SHA1_BLOCK_SIZE,
		    .cra_digestsize = SHA1_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.base = {
			.cra_name = "sha224",
			.cra_driver_name = "sha224-aic",
			.cra_blocksize = SHA224_BLOCK_SIZE,
		    .cra_digestsize = SHA224_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.base = {
			.cra_name = "sha256",
			.cra_driver_name = "sha256-aic",
			.cra_blocksize = SHA256_BLOCK_SIZE,
		    .cra_digestsize = SHA256_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.base = {
			.cra_name = "sha384",
			.cra_driver_name = "sha384-aic",
			.cra_blocksize = SHA384_BLOCK_SIZE,
		    .cra_digestsize = SHA384_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.base = {
			.cra_name = "sha512",
			.cra_driver_name = "sha512-aic",
			.cra_blocksize = SHA512_BLOCK_SIZE,
		    .cra_digestsize = SHA512_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.setkey = aic_hash_setkey,
		.base = {
			.cra_name = "hmac(sha1)",
			.cra_driver_name = "hmac-sha1-aic",
			.cra_blocksize = SHA1_BLOCK_SIZE,
		    .cra_digestsize = SHA1_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_hmac_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
{
	.alg = {
		.init = aic_hash_init,
		.update = aic_hash_update,
		.final = aic_hash_final,
		.finup = aic_hash_finup,
		.setkey = aic_hash_setkey,
		.base = {
			.cra_name = "hmac(sha256)",
			.cra_driver_name = "hmac-sha256-aic",
			.cra_blocksize = SHA256_BLOCK_SIZE,
		    .cra_digestsize = SHA256_DIGEST_SIZE,
			.cra_ctxsize = sizeof(struct aic_hash_tfm_ctx),
			.cra_alignmask = 3,
			.cra_init = aic_hash_hmac_alg_init,
			.cra_exit = aic_hash_alg_exit,
		}
	}
},
};

void aic_md_destroy(struct aic_hash_handle *handle)
{
    pr_debug("%s\n", __func__);
    if (handle->tfm->alg->base.cra_exit) {
        handle->tfm->alg->base.cra_exit(handle);
    }

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

struct aic_hash_handle *aic_md_init(const char *ciphername, u32 flags)
{
    struct aic_hash_handle *handle = NULL;
    struct aic_hash_tfm_ctx *ctx = NULL;
    struct aic_hash_req_ctx *rctx = NULL;
    struct hash_request *req = NULL;
    struct hash_tfm *tfm = NULL;

    pr_debug("%s\n", __func__);
    handle = aicos_malloc(0, sizeof(struct aic_hash_handle));
    if (handle == NULL) {
        pr_err("malloc hash handle failed.\n");
        return NULL;
    }
    memset(handle, 0, sizeof(struct aic_hash_handle));

    tfm = aicos_malloc(0, sizeof(struct hash_tfm));
    if (tfm == NULL) {
        pr_err("malloc hash tfm failed.\n");
        goto out;
    }
    memset(tfm, 0, sizeof(struct hash_tfm));
    handle->tfm = tfm;

    ctx = aicos_malloc(0, sizeof(struct aic_hash_tfm_ctx));
    if (ctx == NULL) {
        pr_err("malloc hash ctx failed.\n");
        goto out;
    }
    memset(ctx, 0, sizeof(struct aic_hash_tfm_ctx));
    tfm->__crt_ctx = (void *)ctx;

    req = aicos_malloc(0, sizeof(struct hash_request));
    if (req == NULL) {
        pr_err("malloc hash req failed.\n");
        goto out;
    }
    memset(req, 0, sizeof(struct hash_request));
    handle->req = req;

    rctx = aicos_malloc(0, sizeof(struct aic_hash_req_ctx));
    if (rctx == NULL) {
        pr_err("malloc hash rctx failed.\n");
        goto out;
    }
    memset(rctx, 0, sizeof(struct aic_hash_req_ctx));
    req->__ctx = (void *)rctx;

    for (int i = 0; i < ARRAY_SIZE(hash_algs); i++) {
        if (!strcmp(hash_algs[i].alg.base.cra_name, ciphername) ||
            !strcmp(hash_algs[i].alg.base.cra_driver_name, ciphername)) {
            tfm->alg = &hash_algs[i].alg;
            if (handle->tfm->alg->init) {
                if (!handle->tfm->alg->base.cra_init(handle) &&
                    !handle->tfm->alg->init(handle))
                    return handle;
            }
        }
    }
    pr_debug("not found %s algo\n", ciphername);

out:
    aic_md_destroy(handle);

    return NULL;
}
