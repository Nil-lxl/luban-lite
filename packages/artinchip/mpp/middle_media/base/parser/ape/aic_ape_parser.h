/*
* Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
* 
* SPDX-License-Identifier: Apache-2.0
*
*  author: <xiaodong.zhao@artinchip.com>
*  Desc: aic_ape_parser
*/

#ifndef __AIC_APE_PARSER_H__
#define __AIC_APE_PARSER_H__

#include "aic_parser.h"
#include "aic_stream.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

s32 aic_ape_parser_create(unsigned char* uri, struct aic_parser **parser);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif
