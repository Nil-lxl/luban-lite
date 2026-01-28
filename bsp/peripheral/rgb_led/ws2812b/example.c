#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include "hal_adcim.h"

#include "rtthread.h"
#include "rtdevice.h"

#include "ws2812b.h"

static rt_thread_t ws2812_thread = RT_NULL;

const uint32_t colors[7] = { 0x080000, 0x080400, 0x080800, 0x000800, 0x000808, 0x000008, 0x040008 };

/**
 * @brief ws2812测试
 * @param param 参数
 */
static void ws2812_test(void *param) {

    ws2812_SetColorIndex(0, WS2812_RED);
    ws2812_SetColorIndex(1, WS2812_GREEN);
    ws2812_SetColorIndex(2, WS2812_BLUE);
    ws2812_SetColorIndex(3, WS2812_RED);
    ws2812_SetColorIndex(4, WS2812_GREEN);
    ws2812_SetColorIndex(5, WS2812_BLUE);

    ws2812_send_array(); // 立即发送更新
    rt_thread_mdelay(1000);

    ws2812_SetColorAll(WS2812_WHITE);
    ws2812_send_array();
    rt_thread_mdelay(1000);

    ws2812_SetColorAll(WS2812_BLACK);
    ws2812_send_array();
}
/**
 * @brief ws2812单个循环灯线程入口
 * @param param 参数
 */
static void ws2812_single_loop(void *param) {
    int current_led = 0;

    while (1) {
        for (int i = 0;i < WS2812_NUMBER;i++) {
            ws2812_SetColorIndex(i, WS2812_BLACK);
            ws2812_send_array();
        }
        uint32_t color = colors[current_led % 7];
        ws2812_SetColorIndex(current_led, color);
        ws2812_send_array();
        current_led = (current_led + 1) % WS2812_NUMBER;
        rt_thread_mdelay(100);
    }
}

/**
 * @brief ws2812彩虹流水灯线程入口
 * @param param 参数
 */
static void ws2812_rainbow_wave(void *param) {
    int offset = 0;
    while (1) {
        for (int i = 0;i < WS2812_NUMBER;i++) {
            int color_index = (i + offset) % 6;
            uint32_t color = colors[color_index];
            ws2812_SetColorIndex(i, color);
        }
        ws2812_send_array();
        offset = (offset + 1) % 6;
        rt_thread_mdelay(100);
    }
}

/**
 * @brief ws2812追逐灯效果线程入口
 * @param param 参数
 */
static void ws2812_chase_effect(void *param) {

}


/**
 * @brief 创建ws2812 demo线程
 */
static void ws2812_demo(void) {
    if (ws2812_thread) {
        LOG_W("ws2812 demo is running");
    } else {
        ws2812_thread = rt_thread_create("ws2812_demo", ws2812_rainbow_wave, RT_NULL, 4 * 1024, 25, 25);
        rt_thread_startup(ws2812_thread);
    }
}

/**
 * @brief 退出ws2812 demo线程
 */
static void ws2812_demo_exit(void) {
    if (ws2812_thread) {
        rt_thread_delete(ws2812_thread);
        ws2812_thread = RT_NULL;
    } else {
        LOG_W("ws2812 demo is not running");
    }
}

/**
 * @brief 清除所有ws2812灯
 */
static void ws2812_clean(void) {
    ws2812_SetColorAll(WS2812_BLACK);
    ws2812_send_array();
}

MSH_CMD_EXPORT(ws2812_demo, ws2812 test);
MSH_CMD_EXPORT(ws2812_demo_exit, exit ws2812 test);
MSH_CMD_EXPORT(ws2812_clean, ws2812 clean all leds);


