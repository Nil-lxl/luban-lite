/*
* Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
*
* SPDX-License-Identifier: Apache-2.0
*
* author: <jun.ma@artinchip.com>
* Desc: aic_file_stream
*/


#include <stdlib.h>
#include <fcntl.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "aic_stream.h"
#include "aic_file_stream.h"

#define AIC_CLTBL_SIZE (32*1024)

#ifdef AIC_CHIP_D21X
#define EN_CLTBL_FILE_SIZE (1024*1024)
#else
#define EN_CLTBL_FILE_SIZE (128*1024*1024)
#endif

#ifdef AIC_MPP_PLAYER_FILE_CACHE
struct aic_file_cache {
	char *buffer;
	s64 buf_size;
	s64 buf_start;
	s64 buf_end;
	s64 buf_pos;
	s32 buf_dirty;
};
#endif

struct aic_file_stream {
	struct aic_stream base;
	s32 fd;
	s64 file_size;
	uint32_t *cltbl;
#ifdef AIC_MPP_PLAYER_FILE_CACHE
	s64 cur_pos;
	struct aic_file_cache cache_buf;
#endif
};


#ifdef AIC_MPP_PLAYER_FILE_CACHE
static s64 file_stream_read_cached(struct aic_stream *stream, void *buf, s64 len)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	s64 new_buf_start, buf_offset, read_len, copy_len;
	char *dest = (char *)buf;

	file_stream->cur_pos = lseek(file_stream->fd, 0, SEEK_CUR);

	//if requested data is in buffer
	if (file_stream->cache_buf.buffer &&
		file_stream->cache_buf.buf_start <= file_stream->cur_pos &&
		file_stream->cur_pos + len <= file_stream->cache_buf.buf_end) {
		buf_offset = file_stream->cur_pos - file_stream->cache_buf.buf_start;
		memcpy(dest, file_stream->cache_buf.buffer + buf_offset, len);
		file_stream->cur_pos += len;

		return len;
	}

	// If data is too large，direct read
	if (len > file_stream->cache_buf.buf_size / 2) {
		read_len = read(file_stream->fd, dest, len);
		if (read_len > 0) {
			file_stream->cur_pos += read_len;
		}
		return read_len;
	}

	// Fill buffer
	new_buf_start = file_stream->cur_pos;
	read_len = read(file_stream->fd, file_stream->cache_buf.buffer,
					file_stream->cache_buf.buf_size);
	if (read_len <= 0) {
		return read_len;
	}

	file_stream->cache_buf.buf_start = new_buf_start;
	file_stream->cache_buf.buf_end = new_buf_start + read_len;

	// copy data from buffer
	copy_len = (len < read_len) ? len : read_len;
	memcpy(dest, file_stream->cache_buf.buffer, copy_len);
	file_stream->cur_pos += copy_len;

	return copy_len;
}
#endif

static s64 file_stream_read(struct aic_stream *stream, void *buf, s64 len)
{
	s64 ret;
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	ret = read(file_stream->fd, buf, len);
	return ret;
}

static s64 file_stream_write(struct aic_stream *stream, void *buf, s64 len)
{
	s64 ret;
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	ret = write(file_stream->fd, buf, len);
	return ret;
}

static s64 file_stream_tell(struct aic_stream *stream)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	return lseek(file_stream->fd, 0, SEEK_CUR);
}

static s32 file_stream_close(struct aic_stream *stream)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	close(file_stream->fd);
	if (file_stream->cltbl) {
		mpp_free(file_stream->cltbl);
		file_stream->cltbl = NULL;
	}
#ifdef AIC_MPP_PLAYER_FILE_CACHE
	if (file_stream->cache_buf.buffer) {
		mpp_free(file_stream->cache_buf.buffer);
		file_stream->cache_buf.buffer = NULL;
	}
#endif
	mpp_free(file_stream);
	return 0;
}

static s64 file_stream_seek(struct aic_stream *stream, s64 offset, s32 whence)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	return lseek(file_stream->fd, offset, whence);
}

static s64 file_stream_size(struct aic_stream *stream)
{
	struct aic_file_stream *file_stream = (struct aic_file_stream *)stream;
	return file_stream->file_size;
}

s32 file_stream_open(const char* uri,struct aic_stream **stream, int flags)
{
	s32 ret = 0;

	struct aic_file_stream *file_stream = (struct aic_file_stream *)mpp_alloc(sizeof(struct aic_file_stream));
	if (file_stream == NULL) {
		loge("mpp_alloc aic_stream ailed!!!!!\n");
		ret = -1;
		goto exit;
	}

	memset(file_stream, 0x00, sizeof(struct aic_file_stream));

	file_stream->fd = open(uri, flags);
	if (file_stream->fd <= 0) {
		loge("open uri:%s failed!!!!!\n",uri);
		ret = -2;
		goto exit;
	}

	if (flags == O_RDONLY) {
		file_stream->file_size = lseek(file_stream->fd, 0, SEEK_END);
		if (file_stream->file_size <= 0) {
			loge("uri:%s file_size is illegality!!!!!\n",uri);
			ret = -2;
			goto exit;
		}
		lseek(file_stream->fd, 0, SEEK_SET);
		if (file_stream->file_size > EN_CLTBL_FILE_SIZE) {
			file_stream->cltbl = (uint32_t *)mpp_alloc(AIC_CLTBL_SIZE * sizeof(uint32_t));
			if (file_stream->cltbl == NULL) {
				loge("mpp_alloc fail !!!!!\n");
				ret = -1;
				goto exit;
			}
			memset(file_stream->cltbl, 0x00, AIC_CLTBL_SIZE * sizeof(uint32_t));

			file_stream->cltbl[0] = AIC_CLTBL_SIZE;
			fcntl(file_stream->fd, 0x52540001U, file_stream->cltbl);
#ifdef AIC_MPP_PLAYER_FILE_CACHE
			file_stream->cache_buf.buf_size = AIC_MPP_PLAYER_FILE_CACHE_SIZE;
			file_stream->cache_buf.buffer = mpp_alloc(file_stream->cache_buf.buf_size);
			if (file_stream->cache_buf.buffer) {
				memset(file_stream->cache_buf.buffer, 0, file_stream->cache_buf.buf_size);
				file_stream->cache_buf.buf_start = -1;
				file_stream->cache_buf.buf_end = -1;
				file_stream->cache_buf.buf_dirty = 0;
			}
#endif
		}
	}
#ifdef AIC_MPP_PLAYER_FILE_CACHE
	file_stream->base.read_cached = file_stream_read_cached;
#else
	file_stream->base.read_cached = NULL;
#endif
	file_stream->base.read = file_stream_read;
	file_stream->base.write = file_stream_write;
	file_stream->base.close = file_stream_close;
	file_stream->base.seek = file_stream_seek;
	file_stream->base.size =  file_stream_size;
	file_stream->base.tell = file_stream_tell;
	*stream = &file_stream->base;
	return ret;

exit:
	if (file_stream->fd > 0) {
		close(file_stream->fd);
		file_stream->fd = -1;
	}

	if (file_stream && file_stream->cltbl) {
		mpp_free(file_stream->cltbl);
		file_stream->cltbl = NULL;
	}

#ifdef AIC_MPP_PLAYER_FILE_CACHE
	if (file_stream->cache_buf.buffer) {
		mpp_free(file_stream->cache_buf.buffer);
		file_stream->cache_buf.buffer = NULL;
	}
#endif

	if (file_stream != NULL) {
		mpp_free(file_stream);
	}

	*stream = NULL;
	return ret;
}
