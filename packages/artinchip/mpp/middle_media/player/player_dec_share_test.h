/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Author: <che.jiang@artinchip.com>
 *  Desc: video decoder use extra buf
 */


#ifndef __PLAYER_DEC_SHARE_TEST_H__
#define __PLAYER_DEC_SHARE_TEST_H__

#include <stdio.h>
#include <string.h>
#include <stddef.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


#if defined(AIC_CHIP_D12X) && defined(AIC_MPP_PLAYER_VE_USE_FILL_FB)
//#define PLAYER_DEMO_USE_VE_FILL_FB
#endif

s32 player_vdec_share_frame_init(struct aic_player *player);

s32 player_vdec_share_frame_deinit();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif



