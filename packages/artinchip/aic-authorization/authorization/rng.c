/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <hal_ce.h>
#include "rng.h"

#define TRNG_SEED_SIZE 32

struct aic_rng_alg {
    struct rng_alg alg;
};

struct aic_rng_tfm_ctx {
    unsigned int seedsize;
};

struct aic_rng_req_ctx {
    struct crypto_task *task;
    int tasklen;
    unsigned long flags;
    void *dst_cpy_buf;
    unsigned int seedsize;
};

static int aic_rng_task_cfg(struct crypto_task *task,
                            struct aic_rng_req_ctx *rctx, u32 dout, u32 dlen)
{
    pr_debug("%s\n", __func__);

    task->alg.trng.alg_tag = ALG_TRNG;

    task->data.in_len = 4; // Default is 4 double words
    task->data.out_addr = cpu_to_le32(dout);
    task->data.out_len = cpu_to_le32(dlen);

    aicos_dcache_clean_range((void *)(uintptr_t)task, sizeof(struct crypto_task));

    return 0;
}

static inline bool is_need_copy_dst(struct rng_request *req)
{
    if ((u32)(uintptr_t)req->dst % CACHE_LINE_SIZE) {
        pr_debug("%s, offset(0x%x) is not aligned.\n", __func__, (u32)(uintptr_t)req->dst);
        return true;
    }

    /*
	 * if data length is not block size alignment,
	 * still need to use dst copy buffer.
	 */
    if (req->dst_len % CACHE_LINE_SIZE) {
        pr_debug("%s, dst_len(%d) is not aligned.\n", __func__, req->dst_len);
        return true;
    }

    return false;
}

static int aic_rng_unprepare_req(struct rng_tfm *tfm, struct rng_request *req)
{
    struct aic_rng_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    rctx = rng_request_ctx(req);

    if (rctx->task) {
        aicos_free_align(0, rctx->task);
        rctx->task = NULL;
        rctx->tasklen = 0;
    }

    if (rctx->dst_cpy_buf) {
        memset(req->dst, 0, req->dst_len);
        memcpy(req->dst, rctx->dst_cpy_buf, req->dst_len);

        aicos_free_align(0, rctx->dst_cpy_buf);
        rctx->dst_cpy_buf = NULL;
    }

    return 0;
}

static int aic_rng_prepare_req(struct rng_tfm *tfm, struct rng_request *req)
{
    struct aic_rng_req_ctx *rctx;
    struct crypto_task *task;
    unsigned int dout, next_addr;
    unsigned int bytelen, remain;
    int i, pages, task_cnt;
    u32 last_flag;

    pr_debug("%s\n", __func__);
    rctx = rng_request_ctx(req);

    rctx->seedsize = rng_tfm_seedsize(tfm);

    if (is_need_copy_dst(req)) {
        pages = ALIGN_UP(req->dst_len, CACHE_LINE_SIZE);
        rctx->dst_cpy_buf = (void *)aicos_malloc_align(0, pages, CACHE_LINE_SIZE);
        if (!rctx->dst_cpy_buf) {
            pr_err("Failed to allocate pages for dst.\n");
            return -ENOMEM;
        }
    }

    task_cnt = DIV_ROUND_UP(req->dst_len, rctx->seedsize);
    rctx->tasklen = sizeof(struct crypto_task) * task_cnt;
    rctx->task = aicos_malloc_align(0, rctx->tasklen, CACHE_LINE_SIZE);
    if (!rctx->task) {
        pr_err("Failed to allocate task.\n");
        goto err;
    }
    memset(rctx->task, 0, rctx->tasklen);

    if (rctx->dst_cpy_buf)
        dout = (u32)(uintptr_t)rctx->dst_cpy_buf;
    else
        dout = (u32)(uintptr_t)req->dst;

    remain = req->dst_len;
    for (i = 0; i < task_cnt; i++) {
        task = &rctx->task[i];
        next_addr = (u32)(uintptr_t)rctx->task + ((i + 1) * sizeof(struct crypto_task));

        last_flag = ((i + 1) == task_cnt);
        bytelen = min(remain, rctx->seedsize);
        bytelen = ALIGN_UP(bytelen, rctx->seedsize);
        if (last_flag)
            task->next = 0;
        else
            task->next = cpu_to_le32(next_addr);
        aic_rng_task_cfg(task, rctx, dout, bytelen);

        dout += bytelen;
    }

    return 0;

err:
    aic_rng_unprepare_req(tfm, req);
    return -1;
}

static int aic_rng_do_one_req(struct rng_request *req)
{
    struct aic_rng_req_ctx *rctx;

    pr_debug("%s\n", __func__);
    rctx = rng_request_ctx(req);

    if (!hal_crypto_is_start()) {
        pr_err("Crypto engine is busy.\n");
        return -EBUSY;
    }

    if (DEBUG_CE)
        hal_crypto_dump_task(rctx->task, rctx->tasklen);

    hal_crypto_init();
    hal_crypto_irq_enable(ALG_HASH_ACCELERATOR);
    hal_crypto_start_hash(rctx->task);
    while (!hal_crypto_poll_finish(ALG_HASH_ACCELERATOR)) {
        continue;
    }
    hal_crypto_pending_clear(ALG_HASH_ACCELERATOR);

    if (hal_crypto_get_err(ALG_HASH_ACCELERATOR)) {
        pr_err("HASH ACCELERATOR run error.\n");
        return -1;
    }

    aicos_dma_sync();
    if (rctx->dst_cpy_buf) {
        aicos_dcache_invalid_range((void *)(uintptr_t)rctx->dst_cpy_buf, req->dst_len);
        memcpy(req->dst, rctx->dst_cpy_buf, req->dst_len);
    } else {
        aicos_dcache_invalid_range((void *)(uintptr_t)req->dst, req->dst_len);
    }

    if (DEBUG_CE)
        hal_crypto_dump_reg();

    hal_crypto_deinit();

    return 0;
}

int aic_trng_generate(struct aic_rng_handle *handle)
{
    struct aic_rng_tfm_ctx *ctx;
    struct aic_rng_req_ctx *rctx;
    struct rng_tfm *tfm;
    struct rng_request *req;
    int ret = 0;

    pr_debug("%s\n", __func__);
    tfm = rng_handle_tfm(handle);
    req = rng_handle_req(handle);
    ctx = rng_tfm_ctx(tfm);
    rctx = rng_request_ctx(req);

    if (!ctx) {
        pr_err("aic rng, device is null\n");
        return -ENODEV;
    }

    memset(rctx, 0, sizeof(struct aic_rng_req_ctx));

    ret = aic_rng_prepare_req(tfm, req);
    if (ret) {
        pr_err("rng prepare req failed.\n");
        return ret;
    }
    ret = aic_rng_do_one_req(req);
    if (ret) {
        pr_err("rng do one req failed.\n");
        return ret;
    }
    ret = aic_rng_unprepare_req(tfm, req);
    if (ret) {
        pr_err("rng unprepare req failed.\n");
        return ret;
    }

    return ret;
}

static struct aic_rng_alg rng_algs[] = {
	{
		.alg = {
			.base.cra_name = "trng",
			.base.cra_driver_name = "trng-aic",
			.base.cra_ctxsize = sizeof(struct aic_rng_tfm_ctx),
			.base.cra_alignmask = 0,
			.generate = aic_trng_generate,
			.seedsize = TRNG_SEED_SIZE,
		},
	},
};

void aic_rng_destroy(struct aic_rng_handle *handle)
{
    pr_debug("%s\n", __func__);
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

struct aic_rng_handle *aic_rng_init(const char *ciphername, u32 flags)
{
    struct aic_rng_handle *handle = NULL;
    struct aic_rng_tfm_ctx *ctx = NULL;
    struct aic_rng_req_ctx *rctx = NULL;
    struct rng_request *req = NULL;
    struct rng_tfm *tfm = NULL;

    handle = aicos_malloc(0, sizeof(struct aic_rng_handle));
    if (handle == NULL) {
        pr_err("malloc rng handle failed.\n");
        return NULL;
    }
    memset(handle, 0, sizeof(struct aic_rng_handle));

    tfm = aicos_malloc(0, sizeof(struct rng_tfm));
    if (tfm == NULL) {
        pr_err("malloc rng tfm failed.\n");
        goto out;
    }
    memset(tfm, 0, sizeof(struct rng_tfm));
    handle->tfm = tfm;

    ctx = aicos_malloc(0, sizeof(struct aic_rng_tfm_ctx));
    if (ctx == NULL) {
        pr_err("malloc rng ctx failed.\n");
        goto out;
    }
    memset(ctx, 0, sizeof(struct aic_rng_tfm_ctx));
    tfm->__crt_ctx = (void *)ctx;

    req = aicos_malloc(0, sizeof(struct rng_request));
    if (req == NULL) {
        pr_err("malloc rng req failed.\n");
        goto out;
    }
    memset(req, 0, sizeof(struct rng_request));
    handle->req = req;

    rctx = aicos_malloc(0, sizeof(struct aic_rng_req_ctx));
    if (rctx == NULL) {
        pr_err("malloc rng rctx failed.\n");
        goto out;
    }
    memset(rctx, 0, sizeof(struct aic_rng_req_ctx));
    req->__ctx = (void *)rctx;

    for (int i = 0; i < ARRAY_SIZE(rng_algs); i++) {
        if (!strcmp(rng_algs[i].alg.base.cra_name, ciphername) ||
            !strcmp(rng_algs[i].alg.base.cra_driver_name, ciphername)) {
            tfm->alg = &rng_algs[i].alg;
            return handle;
        }
    }
    pr_warn("not found %s algo\n", ciphername);

out:
    aic_rng_destroy(handle);

    return NULL;
}
