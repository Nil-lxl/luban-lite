#pragma once

#include "drivers/touch.h"
#include "hal_i2c.h"
#include <aic_core.h>
#include "drivers/touch.h"
#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>

#define ISTARIC_SLAVE_ADDRESS 0x55

#define ISTARIC_CHIP_ID         0x8400348C
#define ISTARIC_FIRM_VERSION    0x84003494

#define TOUCH_DATA_ADDR         0x840034FC

#define TOUCH_MAX_NUM 1
#define TOUCH_DATA_LEN  (2 + 1 + 8 * TOUCH_MAX_NUM)
 


