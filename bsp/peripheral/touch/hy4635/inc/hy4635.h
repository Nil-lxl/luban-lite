/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 */

#pragma once

#include "drivers/touch.h"
#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>

#define HY4635_WORK_MODE 0x00
#define HY4635_TEST_MODE 0xC0


#define HY4635_ADDR_LEN       2
#define HY4635_REGITER_LEN    1
#define HY4635_MAX_TOUCH      5
#define HY4635_POINT_INFO_NUM 8

#define HY4635_ADDRESS_HIGH 0x38


