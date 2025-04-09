/*
 * Dynamic data buffer
 * Copyright (c) 2007-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"

 static atbm_void wpabuf_overflow(const struct wpabuf *buf, atbm_size_t len)
{
	wpa_printf(MSG_ERROR, "wpabuf %p (size=%lu used=%lu) overflow len=%lu",
		   buf, (unsigned long) buf->size, (unsigned long) buf->used,
		   (unsigned long) len);
}


 int wpabuf_resize(struct wpabuf **_buf, atbm_size_t add_len)
{
	struct wpabuf *buf = *_buf;

	if (buf == NULL) {
		*_buf = wpabuf_alloc(add_len);
		return *_buf == NULL ? -1 : 0;
	}


	if (buf->used + add_len > buf->size) {
		atbm_uint8 *nbuf;
		if (buf->flags & WPABUF_FLAG_EXT_DATA) {
			nbuf = realloc(buf->buf, buf->used + add_len);
			if (nbuf == NULL)
				return -1;
			atbm_memset(nbuf + buf->used, 0, add_len);
			buf->buf = nbuf;
		} else {
			nbuf = realloc(buf, sizeof(struct wpabuf) +
					  buf->used + add_len);
			if (nbuf == NULL)
				return -1;
			buf = (struct wpabuf *) nbuf;
			atbm_memset(nbuf + sizeof(struct wpabuf) + buf->used, 0,
				  add_len);
			buf->buf = (atbm_uint8 *) (buf + 1);
			*_buf = buf;
		}
		buf->size = buf->used + add_len;
	}

	return 0;
}


/**
 * wpabuf_alloc - Allocate a wpabuf of the given size
 * @len: Length for the allocated buffer
 * Returns: Buffer to the allocated wpabuf or %NULL on failure
 */
 struct wpabuf * wpabuf_alloc_1(atbm_size_t len,atbm_uint32 call)
{
	struct wpabuf *buf = (struct wpabuf *)atbm_kzalloc(sizeof(struct wpabuf) + len, GFP_KERNEL);
		
	if(len > 10*1024){
	}
	if (buf == NULL){
		return NULL;
	}

	buf->size = len;
	buf->buf = (atbm_uint8 *) (buf + 1);
	return buf;
}


 struct wpabuf * wpabuf_alloc_ext_data(atbm_uint8 *data, atbm_size_t len)
{
	struct wpabuf *buf = (struct wpabuf *)atbm_kzalloc(sizeof(struct wpabuf), GFP_KERNEL);
	if (buf == NULL)
		return NULL;

	buf->size = len;
	buf->used = len;
	buf->buf = data;
	buf->flags |= WPABUF_FLAG_EXT_DATA;

	return buf;
}


 struct wpabuf * wpabuf_alloc_copy(const atbm_void *data, atbm_size_t len)
{
	struct wpabuf *buf = wpabuf_alloc(len);
	if (buf)
		wpabuf_put_data(buf, data, len);
	return buf;
}


 struct wpabuf * wpabuf_dup(const struct wpabuf *src)
{
	struct wpabuf *buf = wpabuf_alloc(wpabuf_len(src));
	if (buf)
		wpabuf_put_data(buf, wpabuf_head(src), wpabuf_len(src));
	return buf;
}


/**
 * wpabuf_free - Free a wpabuf
 * @buf: wpabuf buffer
 */
 atbm_void wpabuf_free(struct wpabuf *buf)
{
	if (buf == NULL)
		return;
	if (buf->flags & WPABUF_FLAG_EXT_DATA)
		atbm_kfree(buf->buf);
	atbm_kfree(buf);
}


 atbm_void wpabuf_clear_free(struct wpabuf *buf)
{
	if (buf) {
		atbm_memset(wpabuf_mhead(buf), 0, wpabuf_len(buf));
		wpabuf_free(buf);
	}
}

 atbm_void wpabuf_put_u8(struct wpabuf *buf, atbm_uint8 data)
{
	atbm_uint8 *pos = wpabuf_put(buf, 1);
	*pos = data;
}

 atbm_void wpabuf_put_le16(struct wpabuf *buf, atbm_uint16 data)
{
	atbm_uint8 *pos = wpabuf_put(buf, 2);
	ATBM_WPA_PUT_LE16(pos, data);
}

 atbm_void wpabuf_put_le32(struct wpabuf *buf, atbm_uint32 data)
{
	atbm_uint8 *pos = wpabuf_put(buf, 4);
	ATBM_WPA_PUT_LE32(pos, data);
}

 atbm_void wpabuf_put_be16(struct wpabuf *buf, atbm_uint16 data)
{
	atbm_uint8 *pos = wpabuf_put(buf, 2);
	ATBM_WPA_PUT_BE16(pos, data);
}

 atbm_void wpabuf_put_be24(struct wpabuf *buf, atbm_uint32 data)
{
	atbm_uint8 *pos = wpabuf_put(buf, 3);
	ATBM_WPA_PUT_BE24(pos, data);
}

 atbm_void wpabuf_put_be32(struct wpabuf *buf, atbm_uint32 data)
{
	atbm_uint8 *pos = wpabuf_put(buf, 4);
	ATBM_WPA_PUT_BE32(pos, data);
}

 atbm_void wpabuf_put_data(struct wpabuf *buf, const atbm_void *data,
				   atbm_size_t len)
{
	if (data)
		atbm_memcpy(wpabuf_put(buf, len), data, len);
}

 atbm_void wpabuf_put_buf(struct wpabuf *dst,
				  const struct wpabuf *src)
{
	wpabuf_put_data(dst, wpabuf_head(src), wpabuf_len(src));
}

 atbm_void * wpabuf_put(struct wpabuf *buf, atbm_size_t len)
{
	void *tmp = wpabuf_mhead_u8(buf) + wpabuf_len(buf);
	buf->used += len;
	if (buf->used > buf->size) {
		wpabuf_overflow(buf, len);
	}
	return tmp;
}


/**
 * wpabuf_concat - Concatenate two buffers into a newly allocated one
 * @a: First buffer
 * @b: Second buffer
 * Returns: wpabuf with concatenated a + b data or %NULL on failure
 *
 * Both buffers a and b will be freed regardless of the return value. Input
 * buffers can be %NULL which is interpreted as an empty buffer.
 */
 struct wpabuf * wpabuf_concat(struct wpabuf *a, struct wpabuf *b)
{
	struct wpabuf *n = NULL;
	atbm_size_t len = 0;

	if (b == NULL)
		return a;

	if (a)
		len += wpabuf_len(a);
	if (b)
		len += wpabuf_len(b);

	n = wpabuf_alloc(len);
	if (n) {
		if (a)
			wpabuf_put_buf(n, a);
		if (b)
			wpabuf_put_buf(n, b);
	}

	wpabuf_free(a);
	wpabuf_free(b);

	return n;
}


/**
 * wpabuf_zeropad - Pad buffer with 0x00 octets (prefix) to specified length
 * @buf: Buffer to be padded
 * @len: Length for the padded buffer
 * Returns: wpabuf padded to len octets or %NULL on failure
 *
 * If buf is longer than len octets or of same size, it will be returned as-is.
 * Otherwise a new buffer is allocated and prefixed with 0x00 octets followed
 * by the source data. The source buffer will be freed on error, i.e., caller
 * will only be responsible on freeing the returned buffer. If buf is %NULL,
 * %NULL will be returned.
 */
 struct wpabuf * wpabuf_zeropad(struct wpabuf *buf, atbm_size_t len)
{
	struct wpabuf *ret;
	atbm_size_t blen;

	if (buf == NULL)
		return NULL;

	blen = wpabuf_len(buf);
	if (blen >= len)
		return buf;

	ret = wpabuf_alloc(len);
	if (ret) {
		atbm_memset(wpabuf_put(ret, len - blen), 0, len - blen);
		wpabuf_put_buf(ret, buf);
	}
	wpabuf_free(buf);

	return ret;
}

