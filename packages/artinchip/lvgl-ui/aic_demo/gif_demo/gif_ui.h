/*
 * Copyright (c) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef GIF_UI_H
#define GIF_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

void gif_ui_init(void);

int gif_check_finish(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // GIF_UI_H
