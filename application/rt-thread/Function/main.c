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
#include "aic_common.h"
#include "aic_hal.h"

#ifdef RT_USING_ULOG
#include <ulog.h>
#endif

#define LOG_TAG "Function"
#define BUZZER_PIN "PC.7"

int main(void) {
#ifdef ULOG_USING_FILTER
    ulog_global_filter_lvl_set(ULOG_OUTPUT_LVL);
#endif
    rt_kprintf("------------------main function------------------\n");
    helloworld();
    return 0;
}
void helloworld(void){
    rt_kprintf("hello world\n");

    u32 buzzer_pin=rt_pin_get(BUZZER_PIN);

    rt_pin_mode(buzzer_pin,PIN_MODE_OUTPUT);
    rt_pin_write(buzzer_pin,PIN_HIGH);
    LOG_W("打开蜂鸣器\n");

    rt_thread_mdelay(30); 

    rt_pin_write(buzzer_pin,PIN_LOW);
    LOG_I("关闭蜂鸣器\n");


}

// INIT_APP_EXPORT(helloworld);

