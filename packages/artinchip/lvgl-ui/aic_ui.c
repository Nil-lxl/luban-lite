/*
 * Copyright (C) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include "lvgl.h"
#include "aic_ui.h"
#include "aic_osal.h"
#include "lvgl/demos/lv_demos.h"

#ifdef AIC_USE_TOUCH_MONKEY_TEST
#if LV_USE_MONKEY
static void use_touch_monkey_test(void)
{
    /*Create encoder monkey test*/
    lv_monkey_config_t config;
    lv_monkey_config_init(&config);
    config.type = LV_INDEV_TYPE_POINTER;
    config.period_range.min = AIC_USE_TOUCH_MONKEY_TEST_PERIOD_RANG_MIN;
    config.period_range.max = AIC_USE_TOUCH_MONKEY_TEST_PERIOD_RANG_MAX;
    config.input_range.min = AIC_USE_TOUCH_MONKEY_TEST_INPUT_RANG_MAX;
    config.input_range.max = AIC_USE_TOUCH_MONKEY_TEST_PERIOD_RANG_MAX;
    lv_monkey_t * monkey = lv_monkey_create(&config);

    /*Set the default group*/
    lv_group_t * group = lv_group_create();
    lv_indev_set_group(lv_monkey_get_indev(monkey), group);
    lv_group_set_default(group);

    /*Start monkey test*/
    lv_monkey_set_enable(monkey, true);
}
#endif
#endif

#ifdef AIC_LVGL_DEMO
void aic_ui_init()
{
    extern void ui_init(void);
    ui_init();

#ifdef AIC_USE_TOUCH_MONKEY_TEST
    use_touch_monkey_test();
#endif
    return;
}
#endif
