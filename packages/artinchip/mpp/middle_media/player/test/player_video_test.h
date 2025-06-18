/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: player video special demo
 */


#ifndef __PLAYER_VIDEO_TEST_H__
#define __PLAYER_VIDEO_TEST_H__

#include <stdio.h>
#include <string.h>
#include <stddef.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


#if defined(AIC_MPP_PLAYER_VE_USE_FILL_FB)
#define PLAYER_DEMO_USE_VE_FILL_FB
#endif

s32 player_vdec_share_frame_init(struct aic_player *player);

s32 player_vdec_share_frame_deinit();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif



