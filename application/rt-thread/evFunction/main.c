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
#include "aic_core.h"

#include "touch/draw_panel.h"
#include "uart/uart_control.h"
#include "cir/cir_control.h"
#include "can/can_demo.h"

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

#ifdef APP_USE_DRAW_LINE_TEST
    panel_draw_start();
#endif

#ifdef APP_USE_UART_TEST
    uart_startup();
#endif

#ifdef APP_USE_CIR_TEST
    cir_control();
#endif

#ifdef APP_USE_CAN_TEST
    can_start();
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
