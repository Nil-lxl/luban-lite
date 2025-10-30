/*
 * Diffie-Hellman groups
 * Copyright (c) 2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef DH_GROUPS_H
#define DH_GROUPS_H

struct dh_group {
	int id;
	const atbm_uint8 *generator;
	atbm_size_t generator_len;
	const atbm_uint8 *prime;
	atbm_size_t prime_len;
	const atbm_uint8 *order;
	atbm_size_t order_len;
	unsigned int safe_prime:1;
};

const struct dh_group * dh_groups_get(int id);
struct wpabuf * dh_init(const struct dh_group *dh, struct wpabuf **priv);
struct wpabuf * dh_derive_shared(const struct wpabuf *peer_public,
				 const struct wpabuf *own_private,
				 const struct dh_group *dh);

#endif /* DH_GROUPS_H */
