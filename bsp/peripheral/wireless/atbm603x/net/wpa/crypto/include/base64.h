/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef BASE64_H
#define BASE64_H

unsigned char * atbmwifi_base64_encode(unsigned char *src, atbm_size_t len,
			      atbm_size_t *out_len);
unsigned char * atbmwifi_base64_decode(unsigned char *src, atbm_size_t len,
			      atbm_size_t *out_len);

#endif /* BASE64_H */
