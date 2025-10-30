/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-04-19        the first version
 */

#ifndef __ST16XX_H__
#define __ST16XX_H__

#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "drivers/touch.h"

/*  ST16XX device reg */
#define ST16XX_ADDR                 0x55
#define ST16XX_STATUS_REG           0x01
#define ST16XX_RESET_REG            0x02
#define ST16XX_TIMOUT_REG           0x03
#define ST16XX_RESOLUTION_REG       0x04
#define ST16XX_TOUCH_INFO           0x10
#define ST16XX_KEY_REG              0x11
#define ST16XX_CONTACTS_NUM_REG     0x3f
#define ST16XX_MAX_TOUCH            5
#define ST16XX_POINT_INFO_NUM       4

#define ST16XX_RESET_PIN            AIC_TOUCH_PANEL_ST16XX_RST_PIN
#define ST16XX_IRQ_PIN              AIC_TOUCH_PANEL_ST16XX_INT_PIN
#define ST16XX_I2C_CHAN             AIC_TOUCH_PANEL_ST16XX_I2C_CHA

int rt_hw_st16xx_init(const char *name, struct rt_touch_config *cfg);

#endif

