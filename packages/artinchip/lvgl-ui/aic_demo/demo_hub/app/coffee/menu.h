/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef COFFEE_MENU_H_
#define COFFEE_MENU_H_

#include "lvgl.h"

void menuInit();
void showMenu();
void menuDel();
lv_obj_t *getNowMenuScr(void);

#endif /* COFFEE_MENU_H_ */
