/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#ifndef COFFEE_COFFEE_H_
#define COFFEE_COFFEE_H_

typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

lv_obj_t *coffee_ui_init(void);
void coffee_ui_del(void);
int pos_height_conversion(int height);
int pos_width_conversion(int width);

extern disp_size_t disp_size;
#endif /* COFFEE_COFFEE_H_ */
