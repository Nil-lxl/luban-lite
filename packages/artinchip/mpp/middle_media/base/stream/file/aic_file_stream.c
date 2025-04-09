/*
* Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
*
* SPDX-License-Identifier: Apache-2.0
*
* author: <jun.ma@artinchip.com>
* Desc: aic_file_stream
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "mpp_mem.h"
#include "mpp_log.h"
#include "aic_stream.h"
#include "aic_file_stream.h"

#define AIC_CLTBL_SIZE (32*1024)
#define EN_CLTBL_FILE_SIZE (128*1024*1024)

struct aic_file_stream {
	struct aic_stream base;
	s32 fd;
	s64 file_size;
	uint32_t *cltbl;
};

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
	if (file_stream->fd < 0) {
		loge("open uri:%s failed!!!!!\n",uri);
		ret = -2;
		goto exit;
	}

	file_stream->file_size = lseek(file_stream->fd, 0, SEEK_END);
	lseek(file_stream->fd, 0, SEEK_SET);
	if (flags == O_RDONLY && file_stream->file_size > EN_CLTBL_FILE_SIZE) {
		file_stream->cltbl = (uint32_t *)mpp_alloc(AIC_CLTBL_SIZE * sizeof(uint32_t));
		if (file_stream->cltbl == NULL) {
			loge("mpp_alloc fail !!!!!\n");
			ret = -1;
			goto exit;
		}
		memset(file_stream->cltbl, 0x00, AIC_CLTBL_SIZE * sizeof(uint32_t));

		file_stream->cltbl[0] = AIC_CLTBL_SIZE;
		fcntl(file_stream->fd, 0x52540001U, file_stream->cltbl);
	}

	file_stream->base.read = file_stream_read;
	file_stream->base.write = file_stream_write;
	file_stream->base.close = file_stream_close;
	file_stream->base.seek = file_stream_seek;
	file_stream->base.size =  file_stream_size;
	file_stream->base.tell = file_stream_tell;
	*stream = &file_stream->base;
	return ret;

exit:
	if (file_stream && file_stream->cltbl) {
		mpp_free(file_stream->cltbl);
		file_stream->cltbl = NULL;
	}

	if (file_stream != NULL) {
		mpp_free(file_stream);
	}

	*stream = NULL;
	return ret;
}
