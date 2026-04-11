/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef _AIC_TEST_DEMO_COMP_NAVIG_FOOTER_UI_H
#define _AIC_TEST_DEMO_COMP_NAVIG_FOOTER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "aic_ui.h"

#define FOOTER_TURN_ON  1
#define FOOTER_TURN_OFF 0

lv_obj_t * footer_ui_create(lv_obj_t *parent);
void footer_ui_switch(lv_obj_t *obj, int act);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
