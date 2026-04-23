/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
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

struct skcipher_crypto_alg {
    u32 cra_flags;
    unsigned int cra_blocksize;
    unsigned int cra_ctxsize;
    unsigned int cra_alignmask;

    int cra_priority;

    char cra_name[CRYPTO_MAX_ALG_NAME];
    char cra_driver_name[CRYPTO_MAX_ALG_NAME];

    int (*cra_init)(struct aic_skcipher_handle *handle);
    void (*cra_exit)(struct aic_skcipher_handle *handle);
    void (*cra_destroy)(struct skcipher_crypto_alg *alg);
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

    struct skcipher_crypto_alg base;
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

struct aic_skcipher_handle *aic_skcipher_init(const char *ciphername, u32 flags);
void aic_skcipher_destroy(struct aic_skcipher_handle *handle);

static inline int aic_skcipher_setkey(struct aic_skcipher_handle *handle,
                                      const void *key, unsigned int keylen)
{
    if (handle->tfm->alg->setkey) {
        return handle->tfm->alg->setkey(handle, key, keylen);
    }
    return -1;
}

static inline int aic_skcipher_encrypt(struct aic_skcipher_handle *handle,
                                       const unsigned char *in, size_t inlen,
				       unsigned char *iv, size_t ivlen,
                                       unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->iv = (void *)iv;
    handle->req->src = (void *)in;
    handle->req->dst = (void *)out;
    handle->req->cryptlen = inlen;
    if (handle->tfm->alg->encrypt) {
        ret = handle->tfm->alg->encrypt(handle);
        if (!ret)
            return handle->req->cryptlen;
    }
    return -1;
}

static inline int aic_skcipher_decrypt(struct aic_skcipher_handle *handle,
                                       const unsigned char *in, size_t inlen,
				       unsigned char *iv, size_t ivlen,
                                       unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->iv = (void *)iv;
    handle->req->src = (void *)in;
    handle->req->dst = (void *)out;
    handle->req->cryptlen = inlen;
    if (handle->tfm->alg->decrypt) {
        ret = handle->tfm->alg->decrypt(handle);
        if (!ret)
            return handle->req->cryptlen;
    }
    return -1;
}
#endif
