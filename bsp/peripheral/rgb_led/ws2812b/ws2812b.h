#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include "hal_adcim.h"

#include "rtthread.h"
#include "rtdevice.h"

#include "aic_core.h"
#include "aic_log.h"
#include "hal_gpai.h"
#include "aic_hal_gpio.h"

#define SPI_BUS_NAME           "spi2"          //  spi总线名称
#define WS2812_DEVICE_NAME      "ws2812b"

#define WS2812_0_CODE           0xC0    // 0码
#define WS2812_1_CODE           0xFC    // 1码
#define ws2812_reset_CODE       0x00    // 复位码

#define WS2812_MAX          8             // 彩灯最大个数
#define WS2812_NUMBER       6             // 彩灯个数

#define WS2812_RED          0xff0000      // 红色
#define WS2812_GREEN        0x00ff00      // 绿色
#define WS2812_BLUE         0x0000ff      // 蓝色
#define WS2812_WHITE        0xffffff      // 白色
#define WS2812_BLACK        0x000000      // 熄灭
#define WS2812_YELLOW       0xffff00      // 黄色
#define WS2812_CYAN         0x00ffff      // 青色
#define WS2812_PURPLE       0xff00ff      // 紫色
#define WS2812_ORANGE       0xffa500      // 橙色
#define WS2812_PINK         0xffc0cb      // 粉色
#define WS2812_BROWN        0xa52a2a      // 棕色
#define WS2812_LIGHTGREEN   0x90ee90      // 亮绿色
#define WS2812_LIGHTBLUE    0xadd8e6      // 亮蓝色
#define WS2812_GRAY         0x808080      // 灰色
#define WS2812_DARKGRAY     0xa9a9a9      // 深灰色
#define WS2812_LIGHTYELLOW  0xffffe0      // 亮黄色
#define WS2812_LIGHTCYAN    0xe0ffff      // 亮青色

int ws2812_SetColorIndex(uint8_t Index, uint64_t Color);

void ws2812_SetColorAll(uint64_t Color);

int ws2812_send_array(void);

