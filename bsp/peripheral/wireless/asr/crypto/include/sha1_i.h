/*
 * SHA1 hash implementation and interface functions
 * Copyright (c) 2003-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SHA1_I_H
#define SHA1_I_H

#define SHA1_MAC_LEN 20

int sha1_prf(const u8 *key, size_t key_len, const char *label,
             const u8 *data, size_t data_len, u8 *buf, size_t buf_len);
#endif /* SHA1_I_H */
