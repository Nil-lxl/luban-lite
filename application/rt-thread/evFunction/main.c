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

void testFunction() {


}


INIT_APP_EXPORT(FunctionImp);