/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef __SKCIPHER_H
#define __SKCIPHER_H

#include <aic_common.h>

#define CRYPTO_MAX_ALG_NAME 128

struct skcipher_tfm {
    struct skcipher_alg *alg;
    void *__crt_ctx;
};

struct skcipher_request {
    unsigned int cryptlen;
    unsigned char *iv;
    void *src;
    void *dst;
    void *__ctx;
};

struct aic_skcipher_handle {
    struct skcipher_tfm *tfm;
    struct skcipher_request *req;
};

struct crypto_alg {
    u32 cra_flags;
    unsigned int cra_blocksize;
    unsigned int cra_ctxsize;
    unsigned int cra_alignmask;

    int cra_priority;

    char cra_name[CRYPTO_MAX_ALG_NAME];
    char cra_driver_name[CRYPTO_MAX_ALG_NAME];

    int (*cra_init)(struct aic_skcipher_handle *handle);
    void (*cra_exit)(struct aic_skcipher_handle *handle);
    void (*cra_destroy)(struct crypto_alg *alg);
};

struct skcipher_alg {
    int (*setkey)(struct aic_skcipher_handle *handle, const u8 *key,
                  unsigned int keylen);
    int (*encrypt)(struct aic_skcipher_handle *handle);
    int (*decrypt)(struct aic_skcipher_handle *handle);
    int (*init)(struct aic_skcipher_handle *handle);
    void (*exit)(struct aic_skcipher_handle *handle);

    unsigned int min_keysize;
    unsigned int max_keysize;
    unsigned int ivsize;
    unsigned int chunksize;
    unsigned int walksize;

    struct crypto_alg base;
};

/*
 * Transform internal helpers.
 */
static inline struct skcipher_tfm *
skcipher_handle_tfm(struct aic_skcipher_handle *handle)
{
    return handle->tfm;
}

static inline struct skcipher_request *
skcipher_handle_req(struct aic_skcipher_handle *handle)
{
    return handle->req;
}

static inline void *skcipher_request_ctx(struct skcipher_request *req)
{
    return req->__ctx;
}

static inline void *skcipher_tfm_ctx(struct skcipher_tfm *tfm)
{
    return tfm->__crt_ctx;
}

static inline unsigned int skcipher_tfm_ivsize(struct skcipher_tfm *tfm)
{
    return tfm->alg->ivsize;
}

static inline unsigned int skcipher_tfm_blocksize(struct skcipher_tfm *tfm)
{
    return tfm->alg->base.cra_blocksize;
}

struct aic_skcipher_handle *aic_skcipher_init(const char *ciphername,
                                              u32 flags);
void aic_skcipher_destroy(struct aic_skcipher_handle *handle);

#endif
