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
#include "http_req/http_request.h"
#include "i2c/i2c_test.h"

#ifdef RT_USING_ULOG
#include <ulog.h>
#endif

#define LOG_TAG "MAIN"

#if defined AIC_USING_D213ECV_EzUIX1_DEMO_V1
#define BL_EN_PIN "PC.7"
#elif defined AIC_USING_HOT68_DEMO_A01_V0
#define BL_EN_PIN "PE.14"
#endif

int main(void) {
#ifdef ULOG_USING_FILTER
    ulog_global_filter_lvl_set(ULOG_OUTPUT_LVL);
#endif
    return 0;
}

void FunctionImp(void) {
    // rt_thread_mdelay(1000);    
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

void backlight_enable() {
    u32 backlight_en_pin = rt_pin_get(BL_EN_PIN);
    rt_pin_mode(backlight_en_pin, PIN_MODE_OUTPUT);
    rt_pin_write(backlight_en_pin, PIN_HIGH);
}

#ifdef BL_EN_PIN 
INIT_DEVICE_EXPORT(backlight_enable);
#endif 

INIT_APP_EXPORT(FunctionImp);