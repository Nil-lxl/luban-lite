/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: qi.xu@artinchip.com
 * Desc: aac parser
 */

#ifndef __AIC_AAC_PARSER_H__
#define __AIC_AAC_PARSER_H__

#include "aic_parser.h"
#include "aic_stream.h"

s32 aic_aac_parser_create(unsigned char* uri, struct aic_parser **parser);

#endif
