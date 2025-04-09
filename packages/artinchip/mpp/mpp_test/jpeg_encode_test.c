/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Author: <qi.xu@artinchip.com>
 *  Desc: jpeg encoder demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "artinchip_fb.h"
#include <getopt.h>
#include <rthw.h>
#include <rtthread.h>
#include "aic_core.h"

#include "dma_allocator.h"
#include "mpp_encoder.h"
#include "mpp_log.h"
#include "mpp_mem.h"

static void print_help(void)
{
	printf("Usage: jpeg_encode_test [OPTIONS] [SLICES PATH]\n\n"
		"Options:\n"
		" -i                             input stream file name\n"
		" -q				 set quality (value range: 0~100)\n"
		" -w				 width of input yuv data\n"
		" -g				 height of input yuv data\n"
		" -h                             help\n\n"
		"End:\n");
}

int jpeg_encode_test(int argc, char **argv)
{
	int i;
	int opt;
	char file_name[1024];
	int quality = 90;

	int width = 176;
	int height = 144;
	int jpeg_phy_addr = 0;

	if (argc < 2) {
		print_help();
		return -1;
	}

	while (1) {
		opt = getopt(argc, argv, "i:q:w:g:h");
		if (opt == -1) {
			break;
		}
		switch (opt) {
		case 'i':
			logd("file path: %s", optarg);
			strcpy(file_name, optarg);
			break;
		case 'w':
			width = atoi(optarg);
			break;
		case 'g':
			height = atoi(optarg);
			break;
		case 'q':
			quality = atoi(optarg);
			break;
		case 'h':
		default:
			print_help();
			return -1;
		}
	}

	FILE* fp = fopen(file_name, "rb");
	int phy_addr[3];
	unsigned char* vir_addr[3];
	int size[3] = {width*height, width*height/4, width*height/4};

	for (i=0; i<3; i++) {
		phy_addr[i] = mpp_phy_alloc(size[i]);
		vir_addr[i] = (unsigned char*)(unsigned long)phy_addr[i];
		fread(vir_addr[i], 1, size[i], fp);
	}
	fclose(fp);

	struct mpp_encoder *p_encoder = mpp_encoder_create(MPP_CODEC_VIDEO_ENCODER_MJPEG);
	if (!p_encoder) {
		loge("create encoder failed.\n");
		goto out;
	}
	struct encode_config config = {0};
	config.quality = quality;
	if (mpp_encoder_init(p_encoder, &config)) {
		loge("initial encoder failed.\n");
		goto out;
	}

	struct mpp_frame frame;
	struct mpp_packet packet;
	memset(&frame, 0, sizeof(struct mpp_frame));
	frame.buf.phy_addr[0] = phy_addr[0];
	frame.buf.phy_addr[1] = phy_addr[1];
	frame.buf.phy_addr[2] = phy_addr[2];
	frame.buf.size.width = width;
	frame.buf.size.height = height;
	frame.buf.stride[0] = width;
	frame.buf.stride[1] = frame.buf.stride[2] = width / 2;
	frame.buf.format = MPP_FMT_YUV420P;

	int len = 0;
	int buf_len = width * height * 4/5 * quality / 100;
	jpeg_phy_addr = mpp_phy_alloc(buf_len);
	unsigned char* jpeg_vir_addr = (unsigned char*)(unsigned long)jpeg_phy_addr;
	packet.phy_addr = jpeg_phy_addr;
	packet.size = buf_len;
	if (mpp_encoder_encode(p_encoder, &frame, &packet) < 0) {
		goto out;
	}

	logi("jpeg encode len: %d", len);
	FILE* fp_save = fopen("/sdcard/save.jpg", "wb");
	fwrite(jpeg_vir_addr, 1, packet.len, fp_save);
	fclose(fp_save);

out:
	if (p_encoder)
		mpp_encoder_destory(p_encoder);

	if (jpeg_phy_addr)
		mpp_phy_free(jpeg_phy_addr);

	mpp_phy_free(phy_addr[0]);
	mpp_phy_free(phy_addr[1]);
	mpp_phy_free(phy_addr[2]);

	return 0;
}

MSH_CMD_EXPORT_ALIAS(jpeg_encode_test, jpeg_encode_test, jpeg encode test);
