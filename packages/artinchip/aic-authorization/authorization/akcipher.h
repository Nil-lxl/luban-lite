/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef __AKCIPHER_H
#define __AKCIPHER_H

#include <aic_common.h>

#define CRYPTO_MAX_ALG_NAME 128

struct akcipher_tfm {
    struct akcipher_alg *alg;
    void *__crt_ctx;
};

struct akcipher_request {
    void *src;
    void *dst;
    unsigned int src_len;
    unsigned int dst_len;
    void *__ctx;
};

struct aic_akcipher_handle {
    struct akcipher_tfm *tfm;
    struct akcipher_request *req;
};

struct akcipher_crypto_alg {
    u32 cra_flags;
    unsigned int cra_blocksize;
    unsigned int cra_ctxsize;
    unsigned int cra_alignmask;

    int cra_priority;

    char cra_name[CRYPTO_MAX_ALG_NAME];
    char cra_driver_name[CRYPTO_MAX_ALG_NAME];

    int (*cra_init)(struct aic_akcipher_handle *handle);
    void (*cra_exit)(struct aic_akcipher_handle *handle);
    void (*cra_destroy)(struct akcipher_crypto_alg *alg);
};

struct akcipher_alg {
    int (*sign)(struct aic_akcipher_handle *handle);
    int (*verify)(struct aic_akcipher_handle *handle);
    int (*encrypt)(struct aic_akcipher_handle *handle);
    int (*decrypt)(struct aic_akcipher_handle *handle);
    int (*set_pub_key)(struct aic_akcipher_handle *handle, const void *key,
                       unsigned int keylen);
    int (*set_priv_key)(struct aic_akcipher_handle *handle, const void *key,
                        unsigned int keylen);
    unsigned int (*max_size)(struct aic_akcipher_handle *handle);
    int (*init)(struct aic_akcipher_handle *handle);
    void (*exit)(struct aic_akcipher_handle *handle);

    unsigned int reqsize;
    struct akcipher_crypto_alg base;
};



#define FLG_ENC  BIT(0)
#define FLG_DEC  BIT(1)
#define FLG_SIGN BIT(2)
#define FLG_VERI BIT(3)
#define FLG_RSA  BIT(4)
#define FLG_SM2  BIT(5)

#define FLG_KEYOFFSET (16)
#define FLG_KEYMASK   GENMASK(19, 16) /* eFuse Key ID */

struct aic_akcipher_alg {
    struct akcipher_alg alg;
};

struct aic_akcipher_rsa_alg_ctx {
    unsigned char *n;
    unsigned char *e;
    unsigned char *d;
    unsigned int n_sz;
    unsigned int e_sz;
    unsigned int d_sz;
};

struct aic_akcipher_sm2_alg_ctx {
    unsigned char *x;
    unsigned char *y;
    unsigned char *d;
    unsigned int x_sz;
    unsigned int y_sz;
    unsigned int d_sz;
};

union aic_akcipher_alg_ctx {
    struct aic_akcipher_rsa_alg_ctx rsa;
    struct aic_akcipher_sm2_alg_ctx sm2;
};

struct aic_akcipher_tfm_ctx {
    union aic_akcipher_alg_ctx alg_ctx;
};

struct aic_akcipher_req_ctx {
    struct crypto_task *task;
    unsigned char *wbuf;
    unsigned int ssram_addr;
    int tasklen;
    unsigned int wbuf_size;
    unsigned long flags;
};


/*
 * Transform internal helpers.
 */
static inline struct akcipher_tfm *
akcipher_handle_tfm(struct aic_akcipher_handle *handle)
{
    return handle->tfm;
}

static inline struct akcipher_request *
akcipher_handle_req(struct aic_akcipher_handle *handle)
{
    return handle->req;
}

static inline void *akcipher_request_ctx(struct akcipher_request *req)
{
    return req->__ctx;
}

static inline void *akcipher_tfm_ctx(struct akcipher_tfm *tfm)
{
    return tfm->__crt_ctx;
}

struct aic_akcipher_handle *aic_akcipher_init(const char *ciphername, u32 flags);
void aic_akcipher_destroy(struct aic_akcipher_handle *handle);

static inline int aic_akcipher_setpubkey(struct aic_akcipher_handle *handle,
                                         const void *key, unsigned int keylen)
{
    int ret;

    if (handle->tfm->alg->set_pub_key) {
        ret = handle->tfm->alg->set_pub_key(handle, key, keylen);
        if (!ret)
            return handle->tfm->alg->max_size(handle);
    }
    return -1;
}

static inline int aic_akcipher_setprivkey(struct aic_akcipher_handle *handle,
                                          const void *key, unsigned int keylen)
{
    int ret;

    if (handle->tfm->alg->set_priv_key) {
        ret = handle->tfm->alg->set_priv_key(handle, key, keylen);
        if (!ret)
            return handle->tfm->alg->max_size(handle);
    }
    return -1;
}

static inline int aic_akcipher_sign(struct aic_akcipher_handle *handle,
                                    const unsigned char *in, size_t inlen,
                                    unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->src = (void *)in;
    handle->req->dst = (void *)out;
    handle->req->src_len = inlen;
    handle->req->dst_len = outlen;
    if (handle->tfm->alg->sign) {
        ret = handle->tfm->alg->sign(handle);
        if (!ret)
            return handle->req->dst_len;
    }
    return -1;
}

static inline int aic_akcipher_verify(struct aic_akcipher_handle *handle,
                                    const unsigned char *in, size_t inlen,
                                    unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->src = (void *)in;
    handle->req->dst = (void *)out;
    handle->req->src_len = inlen;
    handle->req->dst_len = outlen;
    if (handle->tfm->alg->verify) {
        ret = handle->tfm->alg->verify(handle);
        if (!ret)
            return handle->req->dst_len;
    }
    return -1;
}

static inline int aic_akcipher_encrypt(struct aic_akcipher_handle *handle,
                                       const unsigned char *in, size_t inlen,
                                       unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->src = (void *)in;
    handle->req->dst = (void *)out;
    handle->req->src_len = inlen;
    handle->req->dst_len = outlen;
    if (handle->tfm->alg->encrypt) {
        ret = handle->tfm->alg->encrypt(handle);
        if (!ret)
            return handle->req->dst_len;
    }
    return -1;
}

static inline int aic_akcipher_decrypt(struct aic_akcipher_handle *handle,
                                       const unsigned char *in, size_t inlen,
                                       unsigned char *out, size_t outlen)
{
    int ret;

    handle->req->src = (void *)in;
    handle->req->dst = (void *)out;
    handle->req->src_len = inlen;
    handle->req->dst_len = outlen;
    if (handle->tfm->alg->decrypt) {
        ret = handle->tfm->alg->decrypt(handle);
        if (!ret)
            return handle->req->dst_len;
    }
    return -1;
}
#endif
