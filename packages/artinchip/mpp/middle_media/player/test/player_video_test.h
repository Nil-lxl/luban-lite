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


#if defined(AIC_MPP_PLAYER_VIDEO_EXT_RENDER)
#define PLAYER_DEMO_VIDEO_EXT_RENDER
#endif

s32 player_video_ext_render_init(struct aic_player *player);

s32 player_video_ext_render_deinit();

void player_video_ext_render_debug(bool debug_en);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif



