/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-10-18        the first version
 */

#include "touch_common.h"

void aic_touch_flip(int16_t *x_coordinate, int16_t *y_coordinate)
{
#ifdef AIC_TOUCH_X_FLIP
    *x_coordinate = (rt_int16_t)AIC_TOUCH_X_COORDINATE_RANGE - *x_coordinate;
#endif
#ifdef AIC_TOUCH_Y_FLIP
    *y_coordinate = (rt_int16_t)AIC_TOUCH_Y_COORDINATE_RANGE - *y_coordinate;
#endif
}

void aic_touch_rotate(int16_t *x_coordinate, int16_t *y_coordinate)
{
    rt_uint16_t temp = 0;
    temp = temp;
#ifdef AIC_TOUCH_90_DEGREE_ROTATION
    temp = *x_coordinate;
    *x_coordinate = *y_coordinate;
    *y_coordinate = (rt_int16_t)AIC_TOUCH_X_COORDINATE_RANGE - temp;
#endif
#ifdef AIC_TOUCH_270_DEGREE_ROTATION
    temp = *x_coordinate;
    *x_coordinate = (rt_int16_t)AIC_TOUCH_Y_COORDINATE_RANGE - *y_coordinate;
    *y_coordinate = temp;
#endif
}

void aic_touch_scale(int16_t *x_coordinate, int16_t *y_coordinate)
{
#ifdef AIC_TOUCH_CROP
    rt_int32_t temp_x;
    rt_int32_t temp_y;

    temp_x = (*x_coordinate) * AIC_SCREEN_REAL_X_RESOLUTION * 10 / AIC_TOUCH_X_COORDINATE_RANGE / 10;
    temp_y = (*y_coordinate) * AIC_SCREEN_REAL_Y_RESOLUTION * 10 / AIC_TOUCH_Y_COORDINATE_RANGE / 10;
    *x_coordinate = (int16_t)temp_x;
    *y_coordinate = (int16_t)temp_y;
#endif
}


rt_int8_t aic_touch_crop(int16_t *x_coordinate, int16_t *y_coordinate)
{
#ifdef AIC_TOUCH_CROP
    if (*x_coordinate < AIC_TOUCH_CROP_POS_X || *y_coordinate < AIC_TOUCH_CROP_POS_Y
        || *x_coordinate > (AIC_TOUCH_CROP_POS_X + AIC_TOUCH_CROP_WIDTH)
        || *y_coordinate > (AIC_TOUCH_CROP_POS_Y + AIC_TOUCH_CROP_HEIGHT)) {
        return RT_FALSE;
    }

    *x_coordinate -= AIC_TOUCH_CROP_POS_X;
    *y_coordinate -= AIC_TOUCH_CROP_POS_Y;
#endif
    return RT_TRUE;
}

