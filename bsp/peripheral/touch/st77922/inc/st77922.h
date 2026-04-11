/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-31        the first version
 */

#ifndef __ST77922_H__
#define __ST77922_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

#define ST77922_REGITER_LEN         2
#define ST77922_SALVE_ADDR          0x55
#define ST77922_MAX_TOUCH           5

/* ST77922 REGISTER */
#define ST77922_FW_VERSION          0x0000
#define ST77922_DEV_STATUS          0x0001
#define ST77922_DEV_CONTROL         0x0002
#define ST77922_MAX_X_COORD         0x0005
#define ST77922_TOUCH_INFO          0x0010
#define ST77922_X0_COORD_HIGH       0x0014

#endif
