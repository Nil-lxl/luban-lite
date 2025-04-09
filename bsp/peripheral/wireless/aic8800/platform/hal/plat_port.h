/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#ifndef _PLAT_PORT_H_
#define _PLAT_PORT_H_

#include "rtconfig.h"

#define PLATFORM_CACHE_LINE_SIZE        (CACHE_LINE_SIZE)

void platform_pwr_wifi_pin_init(void);
void platform_pwr_wifi_pin_enable(void);
void platform_pwr_wifi_pin_disable(void);

#endif /* _PLAT_PORT_H_ */

