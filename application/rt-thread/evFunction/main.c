/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"

#include "draw_panel.h"

#ifdef RT_USING_ULOG
#include <ulog.h>
#endif

#define LOG_TAG "MAIN"
#define BUZZER_PIN "PC.7"

int main(void) {
#ifdef ULOG_USING_FILTER
    ulog_global_filter_lvl_set(ULOG_OUTPUT_LVL);
#endif
    return 0;
}


void FunctionImp(void) {
    LOG_I("--------------Function Implement--------------\n");

    #ifdef USE_DRAW_LINE_TEST
    panel_draw_start();
    #endif

}

INIT_APP_EXPORT(FunctionImp);


void set_buzzer(int level) {
    u32 buzzer_pin = rt_pin_get(BUZZER_PIN);
    rt_pin_mode(buzzer_pin, PIN_MODE_OUTPUT);
    if (level == 1) {
        rt_pin_write(buzzer_pin, PIN_HIGH);
        LOG_I("蜂鸣器打开\n");

    } else if (level == 0) {
        rt_pin_write(buzzer_pin, PIN_LOW);
        LOG_I("蜂鸣器关闭\n");
    }
}
