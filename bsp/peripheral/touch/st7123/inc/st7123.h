#pragma once

#include "touch.h"

#define ST7123_SLAVE_ADDR       0x55

#define ST7123_MAX_TOUCH        10
#define ST7123_REGISTER_LEN     2

#define ST7123_FW_VERSION       0x0000
#define ST7123_DEV_STATUS       0x0001
#define ST7123_DEV_CONTROL      0x0002
#define ST7123_MAX_X_COORD      0x0005
#define ST7123_TOUCH_INFO       0x0010


// #define ST7123_TOUCH1_XH        0x03
// #define ST7123_TOUCH1_XL        0x04
// #define ST7123_TOUCH1_YH        0x05
// #define ST7123_TOUCH1_YL        0x06

typedef struct touch_point{
    rt_uint8_t id;
    rt_uint8_t status;
    int16_t touch_x;
    int16_t touch_y;
}touch_point_t;