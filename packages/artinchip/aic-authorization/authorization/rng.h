/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef __RNG_H
#define __RNG_H

#include <aic_common.h>

#define CRYPTO_MAX_ALG_NAME 128

struct rng_tfm {
	struct rng_alg *alg;
	void *__crt_ctx;
};

struct rng_request {
    void *src;
    void *dst;
    unsigned int src_len;
    unsigned int dst_len;
    void *__ctx;
};

struct aic_rng_handle {
    struct rng_tfm *tfm;
    struct rng_request *req;
};

struct crypto_alg {
	u32 cra_flags;
	unsigned int cra_blocksize;
	unsigned int cra_ctxsize;
	unsigned int cra_alignmask;

	int cra_priority;

	char cra_name[CRYPTO_MAX_ALG_NAME];
	char cra_driver_name[CRYPTO_MAX_ALG_NAME];

	int (*cra_init)(struct aic_rng_handle *handle);
	void (*cra_exit)(struct aic_rng_handle *handle);
	void (*cra_destroy)(struct crypto_alg *alg);
};

struct rng_alg {
    int (*generate)(struct aic_rng_handle *handle);
    int (*seed)(struct aic_rng_handle *handle, const u8 *seed, unsigned int slen);
    void (*set_ent)(struct aic_rng_handle *handle, const u8 *data,unsigned int len);

    unsigned int seedsize;
    struct crypto_alg base;
};

static inline struct rng_tfm *rng_handle_tfm(struct aic_rng_handle *handle)
{
    return handle->tfm;
}

static inline struct rng_request *rng_handle_req(struct aic_rng_handle *handle)
{
    return handle->req;
}

static inline void *rng_request_ctx(struct rng_request *req)
{
    return req->__ctx;
}

static inline void *rng_tfm_ctx(struct rng_tfm *tfm)
{
    return tfm->__crt_ctx;
}

static inline unsigned int rng_tfm_seedsize(struct rng_tfm *tfm)
{
    return tfm->alg->seedsize;
}

struct aic_rng_handle *aic_rng_init(const char *ciphername, u32 flags);
void aic_rng_destroy(struct aic_rng_handle *handle);

static inline int aic_rng_generate(struct aic_rng_handle *handle, u8 *buf,
                                   unsigned int len)
{
    int ret;

    handle->req->dst = (void *)buf;
    handle->req->dst_len = len;

    if (handle->tfm->alg->generate) {
        ret = handle->tfm->alg->generate(handle);
        if (!ret)
            return handle->req->dst_len;
    }

    return -1;
}

static inline int aic_rng_get_bytes(u8 *buf, unsigned int len)
{
    struct aic_rng_handle *handle;
    int ret = 0;

    handle = aic_rng_init("trng", 0);
    if (handle == NULL) {
        return -1;
    }

    ret = aic_rng_generate(handle, buf, len);

    if (handle)
        aic_rng_destroy(handle);

    return ret;
}

#endif
