/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: middle media riff common interface
 */

#ifndef __AIC_RIFF_H__
#define __AIC_RIFF_H__

#include "aic_stream.h"
#include "aic_tag.h"

int aic_get_extradata(struct aic_codec_param *par, struct aic_stream *pb, int size);
int aic_get_bmp_header(struct aic_stream *pb, struct aic_codec_param *par, unsigned int *size);
int aic_get_wav_header(struct aic_stream *pb, struct aic_codec_param *par, int size,
                       int big_endian, int get_extradata);
#endif /* __AIC_RIFF_H__ */
