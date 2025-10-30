/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "aic_ui.h"
#include "lvgl.h"

int button_adc_open(void);
void button_adc_close(void);
void button_adc_read(int *btn_id, uint32_t *btn_state);
