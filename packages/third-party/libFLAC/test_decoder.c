/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: flac decoder test
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <rtthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h> /* for stat() */
#include "share/compat.h"
#include "FLAC/stream_decoder.h"

#define FLAC_DECODER_SINGLE_MODE
static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
#ifndef FLAC_DECODER_SINGLE_MODE
static FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
#endif
static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

static FLAC__uint64 total_samples = 0;
static unsigned sample_rate = 0;
static unsigned channels = 0;
static unsigned bps = 0;
static FLAC__off_t flacfilesize = 0;

typedef struct {
	FILE *fin_file;
	FILE *fout_file;
	FLAC__bool eos;
} StreamDecoderClientData;

FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 x)
{
	return ((fputc(x, f) != EOF) && (fputc(x >> 8, f) != EOF));
}

FLAC__bool write_little_endian_int16(FILE *f, FLAC__int16 x)
{
	return write_little_endian_uint16(f, (FLAC__uint16)x);
}

FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 x)
{
	return ((fputc(x, f) != EOF) && \
		(fputc(x >> 8, f) != EOF) && \
		(fputc(x >> 16, f) != EOF) && \
		(fputc(x >> 24, f) != EOF));
}

int flac_decoder_file_demo(int argc, char *argv[])
{
	FLAC__bool ok = true;
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	StreamDecoderClientData decoder_client_data;
	FILE *fout;

	if(argc != 3) {
		fprintf(stderr, "usage: %s infile.flac outfile.wav\n", argv[0]);
		return 1;
	}

	if((fout = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for output\n", argv[2]);
		return 1;
	}

	if((decoder = FLAC__stream_decoder_new()) == NULL) {
		fprintf(stderr, "ERROR: allocating decoder\n");
		fclose(fout);
		return 1;
	}
	(void)FLAC__stream_decoder_set_md5_checking(decoder, true);

	decoder_client_data.fout_file = fout;
	init_status = FLAC__stream_decoder_init_file(decoder, argv[1], write_callback, metadata_callback, error_callback, /*client_data=*/&decoder_client_data);
	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		fprintf(stderr, "ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
		ok = false;
	}
	if(ok) {
		ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
		fprintf(stderr, "decoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf(stderr, "   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
	}

	FLAC__stream_decoder_delete(decoder);
	fclose(fout);

	return 0;
}

int flac_decoder_stream_demo(int argc, char *argv[])
{
	FLAC__bool ok = true;
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	FILE *fin_file, *fout_file;
	struct flac_stat_s filestats;
	StreamDecoderClientData decoder_client_data;
	struct timespec tm_before, tm_after;

	if(argc != 3) {
		fprintf(stderr, "usage: %s infile.flac outfile.wav\n", argv[0]);
		return 1;
	}

	if(flac_stat(argv[1], &filestats) != 0)
		return 1;
	else
		flacfilesize = filestats.st_size;

	if((fin_file = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for infile\n", argv[1]);
		return 1;
	}
	if((fout_file = fopen(argv[2], "wb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for output\n", argv[2]);
		fclose(fin_file);
		return 1;
	}
	if((decoder = FLAC__stream_decoder_new()) == NULL) {
		fprintf(stderr, "ERROR: allocating decoder\n");
 		fclose(fin_file);
		fclose(fout_file);
		return 1;
	}
	(void)FLAC__stream_decoder_set_md5_checking(decoder, true);

	decoder_client_data.eos = 0;
	decoder_client_data.fin_file = fin_file;
	decoder_client_data.fout_file = fout_file;

#ifdef FLAC_DECODER_SINGLE_MODE
	init_status = FLAC__stream_decoder_init_stream(decoder, read_callback, NULL, NULL, NULL, /*eof_callback*/NULL,
													write_callback, metadata_callback, error_callback, /*client_data=*/&decoder_client_data);
#else
	init_status = FLAC__stream_decoder_init_stream(decoder, read_callback, seek_callback, tell_callback, length_callback, eof_callback,
													write_callback, metadata_callback, error_callback, /*client_data=*/&decoder_client_data);
#endif
	if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		fprintf(stderr, "ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
		ok = false;
	}
	if(ok) {
#ifdef FLAC_DECODER_SINGLE_MODE
		while(!decoder_client_data.eos) {
			clock_gettime(CLOCK_REALTIME, &tm_before);
			if(!FLAC__stream_decoder_process_single(decoder))
				printf("ERROR:  decoder single failed\n");
			clock_gettime(CLOCK_REALTIME, &tm_after);
			printf("decoder state: %s, time_cost: %" PRIu64 "us\n",
				FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)],
				(tm_after.tv_sec - tm_before.tv_sec) * 1000000 +
				(tm_after.tv_nsec - tm_before.tv_nsec) / 1000);
		}
#else
		ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
		fprintf(stderr, "decoding: %s\n", ok? "succeeded" : "FAILED");
		fprintf(stderr, "   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
#endif
	}

	FLAC__stream_decoder_delete(decoder);
	fclose(fout_file);
	fclose(fin_file);
	return 0;
}

static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	FILE *f = (FILE*)((StreamDecoderClientData*)client_data)->fin_file;
	const size_t requested_bytes = *bytes;

	(void)decoder;

	if(0 == f) {
		printf("ERROR: client_data in read callback is NULL\n");
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

	if(feof(f)) {
		*bytes = 0;
		((StreamDecoderClientData*)client_data)->eos = 1;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	} else if(requested_bytes > 0) {
		*bytes = fread(buffer, 1, requested_bytes, f);
		if(*bytes == 0) {
			if(feof(f)) {
				((StreamDecoderClientData*)client_data)->eos = 1;
				printf("FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM\n");
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			} else {
				return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
			}
		} else {
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	} else {
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
	}
}

#ifndef  FLAC_DECODER_SINGLE_MODE
static FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FILE *f = (FILE*)((StreamDecoderClientData*)client_data)->fin_file;

	(void)decoder;

	if(0 == f) {
		printf("ERROR: client_data in seek callback is NULL\n");
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	if(fseeko(f, (FLAC__off_t)absolute_byte_offset, SEEK_SET) < 0) {
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FILE *f = (FILE*)((StreamDecoderClientData*)client_data)->fin_file;
	FLAC__off_t offset;

	(void)decoder;

	if(0 == f) {
		printf("ERROR: client_data in tell callback is NULL\n");
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}


	offset = ftello(f);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}


static FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	FILE *f = (FILE*)((StreamDecoderClientData*)client_data)->fin_file;

	(void)decoder;

	if(0 == f) {
		printf("ERROR: client_data in length callback is NULL\n");
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	}

	*stream_length = (FLAC__uint64)flacfilesize;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data)
{
	FILE *f = (FILE*)((StreamDecoderClientData*)client_data)->fin_file;
	FLAC__bool eof;
	(void)decoder;

	if(0 == f) {
		printf("ERROR: client_data in eof callback is NULL\n");
		return true;
	}
	eof = feof(f);
	if (eof)
		((StreamDecoderClientData*)client_data)->eos = 1;
	return eof;
}
#endif

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FILE *f = (FILE*)((StreamDecoderClientData*)client_data)->fout_file;
	const FLAC__uint32 total_size = (FLAC__uint32)(total_samples * channels * (bps/8));
	size_t i;

	(void)decoder;

	if(total_samples == 0) {
		fprintf(stderr, "ERROR: this example only works for FLAC files that have a total_samples count in STREAMINFO\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(channels != 2 || bps != 16) {
		fprintf(stderr, "ERROR: this example only supports 16bit stereo streams\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(frame->header.channels != 2) {
		fprintf(stderr, "ERROR: This frame contains %"PRIu32" channels (should be 2)\n", frame->header.channels);
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(buffer[0] == NULL) {
		fprintf(stderr, "ERROR: buffer [0] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(buffer[1] == NULL) {
		fprintf(stderr, "ERROR: buffer [1] is NULL\n");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	/* write WAVE header before we write the first frame */
	if(frame->header.number.sample_number == 0) {
		if(
			fwrite("RIFF", 1, 4, f) < 4 ||
			!write_little_endian_uint32(f, total_size + 36) ||
			fwrite("WAVEfmt ", 1, 8, f) < 8 ||
			!write_little_endian_uint32(f, 16) ||
			!write_little_endian_uint16(f, 1) ||
			!write_little_endian_uint16(f, (FLAC__uint16)channels) ||
			!write_little_endian_uint32(f, sample_rate) ||
			!write_little_endian_uint32(f, sample_rate * channels * (bps/8)) ||
			!write_little_endian_uint16(f, (FLAC__uint16)(channels * (bps/8))) || /* block align */
			!write_little_endian_uint16(f, (FLAC__uint16)bps) ||
			fwrite("data", 1, 4, f) < 4 ||
			!write_little_endian_uint32(f, total_size)
		) {
			fprintf(stderr, "ERROR: write error\n");
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	/* write decoded PCM samples */
	for(i = 0; i < frame->header.blocksize; i++) {
		if(
			!write_little_endian_int16(f, (FLAC__int16)buffer[0][i]) ||  /* left channel */
			!write_little_endian_int16(f, (FLAC__int16)buffer[1][i])     /* right channel */
		) {
			fprintf(stderr, "ERROR: write error\n");
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)decoder, (void)client_data;

	/* print some stats */
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		/* save for later */
		total_samples = metadata->data.stream_info.total_samples;
		sample_rate = metadata->data.stream_info.sample_rate;
		channels = metadata->data.stream_info.channels;
		bps = metadata->data.stream_info.bits_per_sample;

		fprintf(stderr, "sample rate    : %u Hz\n", sample_rate);
		fprintf(stderr, "channels       : %u\n", channels);
		fprintf(stderr, "bits per sample: %u\n", bps);
		fprintf(stderr, "total samples  : %" PRIu64 "\n", total_samples);
	}
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	(void)decoder, (void)client_data;

	fprintf(stderr, "Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

MSH_CMD_EXPORT(flac_decoder_file_demo, flac decoder file test);
MSH_CMD_EXPORT(flac_decoder_stream_demo, flac decoder stream test);
