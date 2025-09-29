#pragma once

#include "touch.h"

#define HY4635_SLAVE_ADDR       0x38

#define HY4635_MAX_TOUCH        5

//Registers
#define HY4635_DEV_MODE         0x00
#define HY4635_TOUCH_FINGER_NUM 0x02

#define HY4635_TOUCH1_XH        0x03
#define HY4635_TOUCH1_XL        0x04
#define HY4635_TOUCH1_YH        0x05
#define HY4635_TOUCH1_YL        0x06

#define HY4635_TOUCH_THRESHOLD  0x80        //触摸阈值=(DATA*10)
#define HY4635_REPORT_SPEED     0x88        //报点率


