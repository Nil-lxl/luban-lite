/*
 * Copyright (c) 2015-2026, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com>
 */

#include "sm2.h"
#include "sm2pubkey.asn1.h"
#include "sm2privkey.asn1.h"

int sm2_get_p(void *context, const void *value, size_t vlen)
{
	struct sm2_key *key = context;
	u8 *data = (u8 *)value + 2;

	/* invalid key provided */
	if (!value || !vlen)
		return -EINVAL;

	key->x = data;
	key->x_sz = 32;

	key->y = data + 32;
	key->y_sz = 32;

	return 0;
}

int sm2_get_d(void *context, const void *value, size_t vlen)
{
	struct sm2_key *key = context;

	/* invalid key provided */
	if (!value || !vlen)
		return -EINVAL;

	key->d = value;
	key->d_sz = vlen;

	return 0;
}

/**
 * sm2_parse_pub_key() - decodes the BER encoded buffer and stores in the
 *                       provided struct sm2_key, pointers to the raw key as is,
 *                       so that the caller can copy it or MPI parse it, etc.
 *
 * @sm2_key:	struct sm2_key key representation
 * @key:	key in BER format
 * @key_len:	length of key
 *
 * Return:	0 on success or error code in case of error
 */
int sm2_parse_pub_key(struct sm2_key *sm2_key, const void *key,
		              unsigned int key_len)
{
	return asn1_ber_decoder(&sm2pubkey_decoder, sm2_key, key, key_len);
}

/**
 * sm2_parse_priv_key() - decodes the BER encoded buffer and stores in the
 *                        provided struct sm2_key, pointers to the raw key
 *                        as is, so that the caller can copy it or MPI parse it,
 *                        etc.
 *
 * @sm2_key:	struct sm2_key key representation
 * @key:	key in BER format
 * @key_len:	length of key
 *
 * Return:	0 on success or error code in case of error
 */
int sm2_parse_priv_key(struct sm2_key *sm2_key, const void *key,
		               unsigned int key_len)
{
	return asn1_ber_decoder(&sm2privkey_decoder, sm2_key, key, key_len);
}
