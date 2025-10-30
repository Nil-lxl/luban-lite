/*
 * SHA-256 internal definitions
 * Copyright (c) 2003-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SHA256_I_H
#define SHA256_I_H

#define SHA256_BLOCK_SIZE 64

struct sha256_state {
	atbm_uint64 length;
	atbm_uint32 state[8], curlen;
	atbm_uint8 buf[SHA256_BLOCK_SIZE];
};

atbm_void sha256_init(struct sha256_state *md);
int sha256_process(struct sha256_state *md, const atbm_uint8 *in,
		   unsigned long inlen);
int sha256_done(struct sha256_state *md, atbm_uint8 *out);

#endif /* SHA256_I_H */
