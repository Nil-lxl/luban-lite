/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>

#define AIC_LYRICS_DEMO_ON 0

#ifdef AIC_IMG_ROLLER_DEMO
#include "lv_img_roller.h"
void aic_img_roller(void);
#endif

#ifdef AIC_PLAYER_DEMO
#include "lv_aic_player.h"
extern void aic_player_demo(void);
#endif

#ifdef AIC_IMAGE_USAGE_DEMO
extern void aic_img_usage(void);
#endif

#ifdef AIC_SWIPE_V1_DEMO
extern void swipe_v1_init(void);
#endif

void ui_init(void)
{
#ifdef AIC_IMG_ROLLER_DEMO
    aic_img_roller();
#endif
#ifdef AIC_PLAYER_DEMO
    aic_player_demo();
#endif

#ifdef AIC_IMAGE_USAGE_DEMO
    aic_img_usage();
#endif

#if defined(AIC_LYRICS_DEMO) && AIC_LYRICS_DEMO_ON
    extern void lyrics_ui_init(void);
    lyrics_ui_init();
#endif

#ifdef AIC_SWIPE_V1_DEMO
    swipe_v1_init();
#endif
    // you can add more demo here

}
