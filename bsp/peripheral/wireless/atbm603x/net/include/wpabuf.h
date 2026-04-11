/*
 * Dynamic data buffer
 * Copyright (c) 2007-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPABUF_H
#define WPABUF_H





/* wpabuf::buf is a pointer to external data */
#define WPABUF_FLAG_EXT_DATA BIT(0)

/*
 * Internal data structure for wpabuf. Please do not touch this directly from
 * elsewhere. This is only defined in header file to allow inline functions
 * from this file to access data.
 */
struct wpabuf {
	atbm_uint16 size; /* total size of the allocated buffer */
	atbm_uint16 used; /* length of data in the buffer */
	atbm_uint8 *buf; /* pointer to the head of the buffer */
	atbm_uint32 flags;
	/* optionally followed by the allocated buffer */
};


int wpabuf_resize(struct wpabuf **buf, atbm_size_t add_len);
#define wpabuf_alloc(l) wpabuf_alloc_1(l,(unsigned long)__builtin_return_address(0)) 
struct wpabuf * wpabuf_alloc_1(atbm_size_t len,atbm_uint32 call);
struct wpabuf * wpabuf_alloc_ext_data(atbm_uint8 *data, atbm_size_t len);
struct wpabuf * wpabuf_alloc_copy(const atbm_void *data, atbm_size_t len);
struct wpabuf * wpabuf_dup(const struct wpabuf *src);
void wpabuf_free(struct wpabuf *buf);
void wpabuf_clear_free(struct wpabuf *buf);
void * wpabuf_put(struct wpabuf *buf, atbm_size_t len);
struct wpabuf * wpabuf_concat(struct wpabuf *a, struct wpabuf *b);
struct wpabuf * wpabuf_zeropad(struct wpabuf *buf, atbm_size_t len);
void wpabuf_printf(struct wpabuf *buf, char *fmt, ...);


/**
 * wpabuf_size - Get the currently allocated size of a wpabuf buffer
 * @buf: wpabuf buffer
 * Returns: Currently allocated size of the buffer
 */
static inline atbm_size_t wpabuf_size(const struct wpabuf *buf)
{
	return buf->size;
}

/**
 * wpabuf_len - Get the current length of a wpabuf buffer data
 * @buf: wpabuf buffer
 * Returns: Currently used length of the buffer
 */
static inline atbm_size_t wpabuf_len(const struct wpabuf *buf)
{
	return buf->used;
}

/**
 * wpabuf_tailroom - Get size of available tail room in the end of the buffer
 * @buf: wpabuf buffer
 * Returns: Tail room (in bytes) of available space in the end of the buffer
 */
static inline atbm_size_t wpabuf_tailroom(const struct wpabuf *buf)
{
	return buf->size - buf->used;
}

/**
 * wpabuf_head - Get pointer to the head of the buffer data
 * @buf: wpabuf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline const atbm_void * wpabuf_head(const struct wpabuf *buf)
{
	return buf->buf;
}

static inline const atbm_uint8 * wpabuf_head_u8(const struct wpabuf *buf)
{
	return wpabuf_head(buf);
}

/**
 * wpabuf_mhead - Get modifiable pointer to the head of the buffer data
 * @buf: wpabuf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline atbm_void * wpabuf_mhead(struct wpabuf *buf)
{
	return buf->buf;
}

static inline atbm_uint8 * wpabuf_mhead_u8(struct wpabuf *buf)
{
	return wpabuf_mhead(buf);
}

void wpabuf_put_le32(struct wpabuf *buf, atbm_uint32 data);
void wpabuf_put_be16(struct wpabuf *buf, atbm_uint16 data);
void wpabuf_put_u8(struct wpabuf *buf, atbm_uint8 data);
void wpabuf_put_le16(struct wpabuf *buf, atbm_uint16 data);

void wpabuf_put_be24(struct wpabuf *buf, atbm_uint32 data);

void wpabuf_put_be32(struct wpabuf *buf, atbm_uint32 data);
void wpabuf_put_data(struct wpabuf *buf, const atbm_void *data,
				   atbm_size_t len);
void wpabuf_put_buf(struct wpabuf *dst,
				  const struct wpabuf *src);

static inline atbm_void wpabuf_set(struct wpabuf *buf, const atbm_void *data, atbm_size_t len)
{
	buf->buf = (atbm_uint8 *) data;
	buf->flags = WPABUF_FLAG_EXT_DATA;
	buf->size = buf->used = len;
}

static inline atbm_void wpabuf_put_str(struct wpabuf *dst, const char *str)
{
	wpabuf_put_data(dst, str, strlen(str));
}

#endif /* WPABUF_H */
