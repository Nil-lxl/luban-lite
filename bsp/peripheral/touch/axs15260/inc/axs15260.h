/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-05-20        the first version
 */

#ifndef __AXS15260_H__
#define __AXS15260_H__

#include "drivers/touch.h"
#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>

#define AXS15260_SLAVE_ADDR                 0x3B

#define AXS_MAX_TOUCH_NUMBER                2
#define AXS_MAX_KEYS                        4
#define AXS_KEY_DIM                         10
#define AXS_ONE_TCH_LEN                     6
#define AXS_MAX_ID                          0x05
#define AXS_READ_POINT_REG                  0xB

#define AXS_EDS_ERR                         0xA
#define AXS_EDS_TIMEOUT                     0xB
#define AXS_EDS_HARDWARE                    0xC
#define AXS_EDS_DEFAULT                     0x0

#define AXS_DOWN_EVENT                      0x0
#define AXS_UP_EVENT                        0x4
#define AXS_MOVE_EVENT                      0x8

#define AXS_TOUCH_GESTURE_POS               0
#define AXS_TOUCH_POINT_NUM                 1
#define AXS_TOUCH_EVENT_POS                 2
#define AXS_TOUCH_X_H_POS                   2
#define AXS_TOUCH_X_L_POS                   3
#define AXS_TOUCH_ID_POS                    4
#define AXS_TOUCH_Y_H_POS                   4
#define AXS_TOUCH_Y_L_POS                   5
#define AXS_TOUCH_WEIGHT_POS                6
#define AXS_TOUCH_AREA_POS                  7

int rt_hw_axs15260_init(const char *name, struct rt_touch_config *cfg);

#endif /* _AXS15260_H_ */
