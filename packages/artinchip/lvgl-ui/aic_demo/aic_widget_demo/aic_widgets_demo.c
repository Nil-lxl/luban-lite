/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>

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
    // you can add more demo here

}
