/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-10-18        the first version
 */

#ifndef __TOUCH_COMMON_H__
#define __TOUCH_COMMON_H__

#include <rtthread.h>
#include <rtdevice.h>

void aic_touch_flip(int16_t *x_coordinate, int16_t *y_coordinate);
void aic_touch_rotate(int16_t *x_coordinate, int16_t *y_coordinate);
void aic_touch_scale(int16_t *x_coordinate, int16_t *y_coordinate);
rt_int8_t aic_touch_crop(int16_t *x_coordinate, int16_t *y_coordinate);
#endif
