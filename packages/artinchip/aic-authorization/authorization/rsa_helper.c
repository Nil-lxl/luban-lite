/*
 * Copyright (c) 2015-2024, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com>
 */

#include "rsa.h"
#include "rsapubkey.asn1.h"
#include "rsaprivkey.asn1.h"

int rsa_get_n(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !vlen)
        return -EINVAL;

    key->n = value;
    key->n_sz = vlen;

    return 0;
}

int rsa_get_e(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !key->n_sz || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->e = value;
    key->e_sz = vlen;

    return 0;
}

int rsa_get_d(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !key->n_sz || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->d = value;
    key->d_sz = vlen;

    return 0;
}

int rsa_get_p(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->p = value;
    key->p_sz = vlen;

    return 0;
}

int rsa_get_q(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->q = value;
    key->q_sz = vlen;

    return 0;
}

int rsa_get_dp(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->dp = value;
    key->dp_sz = vlen;

    return 0;
}

int rsa_get_dq(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->dq = value;
    key->dq_sz = vlen;

    return 0;
}

int rsa_get_qinv(void *context, const void *value, size_t vlen)
{
    struct rsa_key *key = context;

    /* invalid key provided */
    if (!value || !vlen || vlen > key->n_sz)
        return -EINVAL;

    key->qinv = value;
    key->qinv_sz = vlen;

    return 0;
}

/**
 * rsa_parse_pub_key() - decodes the BER encoded buffer and stores in the
 *                       provided struct rsa_key, pointers to the raw key as is,
 *                       so that the caller can copy it or MPI parse it, etc.
 *
 * @rsa_key:	struct rsa_key key representation
 * @key:	key in BER format
 * @key_len:	length of key
 *
 * Return:	0 on success or error code in case of error
 */
int rsa_parse_pub_key(struct rsa_key *rsa_key, const void *key,
                      unsigned int key_len)
{
    return asn1_ber_decoder(&rsapubkey_decoder, rsa_key, key, key_len);
}

/**
 * rsa_parse_priv_key() - decodes the BER encoded buffer and stores in the
 *                        provided struct rsa_key, pointers to the raw key
 *                        as is, so that the caller can copy it or MPI parse it,
 *                        etc.
 *
 * @rsa_key:	struct rsa_key key representation
 * @key:	key in BER format
 * @key_len:	length of key
 *
 * Return:	0 on success or error code in case of error
 */
int rsa_parse_priv_key(struct rsa_key *rsa_key, const void *key,
                       unsigned int key_len)
{
    return asn1_ber_decoder(&rsaprivkey_decoder, rsa_key, key, key_len);
}
