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

#include "http_req/http_request.h"

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

/**
 * @brief 应用程序实现入口
 */
void FunctionImp(void) {
    rt_thread_mdelay(500);    
    LOG_I("--------------Function Implement--------------\n");

#ifdef APP_USE_LCD_TEST
    lcd_test_start();
#endif

#ifdef APP_USE_DRAW_LINE_TEST

#if defined AIC_USING_CTP
    panel_draw_start();
#elif defined AIC_USING_RTP
    rtp_draw_panel();
#endif

#endif
}

INIT_APP_EXPORT(FunctionImp);