/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu <lv.wu@artinchip.com>
 */

#ifndef __GENERAL_WIRELESS_PIN_H__
#define __GENERAL_WIRELESS_PIN_H__

int aic_platform_wlan_hw_reset(void);
int aic_platform_wlan_power_on(void);
int aic_platform_wlan_power_off(void);


int aic_platform_bt_hw_reset(void);
int aic_platform_bt_power_on(void);
int aic_platform_bt_power_off(void);

#endif
