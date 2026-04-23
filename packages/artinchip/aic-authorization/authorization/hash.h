/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef __HASH_H
#define __HASH_H

#include <aic_common.h>

#define CRYPTO_MAX_ALG_NAME 128

struct hash_tfm {
    struct hash_alg *alg;
    void *__crt_ctx;
};

struct hash_request {
    unsigned int nbytes;
    void *src;
    u8 *result;
    void *__ctx;
};

struct aic_hash_handle {
    struct hash_tfm *tfm;
    struct hash_request *req;
};

struct hash_crypto_alg {
    u32 cra_flags;
    unsigned int cra_blocksize;
    unsigned int cra_digestsize;
    unsigned int cra_ctxsize;
    unsigned int cra_alignmask;

    int cra_priority;

    char cra_name[CRYPTO_MAX_ALG_NAME];
    char cra_driver_name[CRYPTO_MAX_ALG_NAME];

    int (*cra_init)(struct aic_hash_handle *handle);
    void (*cra_exit)(struct aic_hash_handle *handle);
    void (*cra_destroy)(struct hash_crypto_alg *alg);
};

struct hash_alg {
    int (*init)(struct aic_hash_handle *handle);
    int (*update)(struct aic_hash_handle *handle);
    int (*final)(struct aic_hash_handle *handle);
    int (*finup)(struct aic_hash_handle *handle);
    int (*digest)(struct aic_hash_handle *handle);
    int (*export)(struct aic_hash_handle *handle, void *out);
    int (*import)(struct aic_hash_handle *handle, const void *in);
    int (*setkey)(struct aic_hash_handle *handle, const u8 *key, unsigned int keylen);

    struct hash_crypto_alg base;
};

/*
 * Transform internal helpers.
 */
static inline struct hash_tfm *hash_handle_tfm(struct aic_hash_handle *handle)
{
    return handle->tfm;
}

static inline struct hash_request *hash_handle_req(struct aic_hash_handle *handle)
{
    return handle->req;
}

static inline void *hash_request_ctx(struct hash_request *req)
{
    return req->__ctx;
}

static inline void *hash_tfm_ctx(struct hash_tfm *tfm)
{
    return tfm->__crt_ctx;
}

static inline unsigned int hash_tfm_blocksize(struct hash_tfm *tfm)
{
    return tfm->alg->base.cra_blocksize;
}

static inline unsigned int hash_tfm_digestsize(struct hash_tfm *tfm)
{
    return tfm->alg->base.cra_digestsize;
}

struct aic_hash_handle *aic_md_init(const char *ciphername, u32 flags);
void aic_md_destroy(struct aic_hash_handle *handle);

static inline int aic_md_setkey(struct aic_hash_handle *handle,
                                const void *key, unsigned int keylen)
{
    if (handle->tfm->alg->setkey) {
        return handle->tfm->alg->setkey(handle, key, keylen);
    }
    return -1;
}

static inline int aic_md_update(struct aic_hash_handle *handle,
                                const unsigned char *in, size_t inlen)
{
    int ret;

    handle->req->src = (void *)in;
    handle->req->nbytes = inlen;
    if (handle->tfm->alg->update) {
        ret = handle->tfm->alg->update(handle);
        if (!ret)
            return 0;
    }
    return -1;
}

static inline int aic_md_final(struct aic_hash_handle *handle,
                               unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->nbytes = 0;
    handle->req->result = (void *)out;
    if (handle->tfm->alg->final) {
        ret = handle->tfm->alg->final(handle);
        if (!ret)
            return outlen;
    }
    return -1;
}
#endif
