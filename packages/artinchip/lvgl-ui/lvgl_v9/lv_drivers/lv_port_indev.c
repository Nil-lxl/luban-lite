/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <rtconfig.h>
#ifdef KERNEL_RTTHREAD
#include <lvgl.h>
#include <stdbool.h>
#include <rtthread.h>
#include <../components/drivers/include/drivers/touch.h>

#define LV_USE_BUTTON_ADC_COMMON_INDEV      0
#define LV_USE_BUTTON_ADC_SIMULATOR_ENCODER 0
#if LV_USE_BUTTON_ADC_INDEV || LV_USE_BUTTON_ADC_SIMULATOR_ENCODER
#define LV_USE_BUTTON_ADC_INDEV 1
#endif

#if LV_USE_BUTTON_ADC_INDEV
static void lv_port_indev_button_adc_init(void);
#endif

static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
static rt_int16_t last_x = 0;
static rt_int16_t last_y = 0;
static lv_indev_t *indev_touchpad;

static void input_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_state;
}



void aic_touch_inputevent_cb(rt_int16_t x, rt_int16_t y, rt_uint8_t state)
{
#ifdef AIC_BT_BT8858A
	extern int bt_hid_set_touch_event(int, unsigned short, unsigned short);
	bt_hid_set_touch_event(state, y, x);
#endif
    switch (state)
    {
    case RT_TOUCH_EVENT_UP:
        last_state = LV_INDEV_STATE_RELEASED;
        break;
    case RT_TOUCH_EVENT_MOVE:
    case RT_TOUCH_EVENT_DOWN:
        last_x = x;
        last_y = y;
        last_state = LV_INDEV_STATE_PRESSED;
        break;
#ifdef AIC_MONKEY_TEST
    case RT_TOUCH_MONKEY_TEST:
        last_x = x;
        last_y = y;
        last_state = LV_INDEV_STATE_PRESSED;
        break;
#endif
    }
}

void lv_port_indev_init(void)
{
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, input_read);

#if LV_USE_BUTTON_ADC_INDEV
    lv_port_indev_button_adc_init();
#endif
}

#if LV_USE_BUTTON_ADC_INDEV
#include "rtdevice.h"
static rt_adc_device_t gpai_dev;
static const char *gpai_name = "gpai";
static const int adc_chan = 2;

static int button_debounce_deal(int button_id)
{
    const uint32_t debounce_delay = 40; /* ms */
    static uint32_t debounce_timer;
    static int last_button_id = -1;

    if (last_button_id != button_id) {
        debounce_timer = lv_tick_get();
        last_button_id = button_id;
    }

    if ((lv_tick_get() - debounce_timer) > debounce_delay) {
        return button_id;
    }

    return -1;
}

static int button_adc_read(void)
{
    const int button_adc_scale = 100;
    static int button_adc_arr[] = {497, 3340, 1351, 2456, 2256, 2156, 1234, 2234};
    static int button_id[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7}; /* ID must start from 0 */

    int adc_val = 0;

    adc_val = rt_adc_read(gpai_dev, adc_chan);

    for (int i = 0; i < sizeof(button_adc_arr) / sizeof(button_adc_arr[0]); i++)
        if ((button_adc_arr[i] - button_adc_scale) <= adc_val && adc_val <= (button_adc_arr[i] + button_adc_scale)) {
            LV_LOG_INFO("[button%d] ch%d: %d\n", button_id[i], adc_chan,
                           adc_val);
            return button_debounce_deal(button_id[i]);
    }
    return -1;
}

#if LV_USE_BUTTON_ADC_COMMON_INDEV
static void button_adc_input_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    static uint32_t last_btn = 0;

    int btn_pr = button_adc_read();
    if (btn_pr >= 0) {
        last_btn = btn_pr;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->btn_id = last_btn;
}
#endif

#if LV_USE_BUTTON_ADC_SIMULATOR_ENCODER
#define BUTTON_ID_ENCODER_LEFT 0x00
#define BUTTON_ID_ENCODER_MID  0x01

static int button_encode_simulator(int button_id)
{
    static uint32_t press_time = 0;
    static uint32_t start_time = 0;
    static uint8_t long_press = 0;
    static uint8_t last_button_id = 0;
    int ret = 0;

    if (last_button_id != button_id || button_id < 0) {
        start_time = lv_tick_get();
    }
    press_time = lv_tick_get() - start_time;

    if (button_id == BUTTON_ID_ENCODER_MID) {
        return 0;
    }

    if (last_button_id == button_id) {
        if (press_time > 150) {
            start_time = lv_tick_get();
            ret = button_id == BUTTON_ID_ENCODER_LEFT ? -1 : 1;
            long_press = 1;
        }
    } else {
        /* short click */
        if (button_id < 0 && last_button_id >= 0 && long_press == 0) {
            ret = last_button_id == BUTTON_ID_ENCODER_LEFT ? -1 : 1;
        } else if (button_id >= 0 && last_button_id >= 0) {
            ret = button_id == BUTTON_ID_ENCODER_LEFT ? -1 : 1;
        }
        long_press = 0;
    }

    last_button_id = button_id;
    return ret;
}

static void button_adc_input_read_encoder(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    static int last_button_id = 0;
    int btn_pr = button_adc_read();
    if (btn_pr >= 0) {
        if (btn_pr == BUTTON_ID_ENCODER_MID) {
            data->state = LV_INDEV_STATE_PRESSED;
        }
    } else {
        if (last_button_id == BUTTON_ID_ENCODER_MID) {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    }

    data->enc_diff = button_encode_simulator(btn_pr);
    last_button_id = btn_pr;
}
#endif

static void lv_port_indev_button_adc_init(void)
{
    gpai_dev = (rt_adc_device_t)rt_device_find(gpai_name);
    if (!gpai_dev) {
        LV_LOG_ERROR("Failed to open %s device\n", gpai_name);
        return;
    }
    rt_adc_enable(gpai_dev, adc_chan);

#if LV_USE_BUTTON_ADC_COMMON_INDEV
    lv_indev_t *button_indev = lv_indev_create();
    lv_indev_set_type(button_indev, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(button_indev, button_adc_input_read);

    /* press the button to simulate pressing the coordinates */
    static const lv_point_t btn_points[8] = {
        {65,  40},  /*Button id 0 -> x:65;  y:40*/
        {160, 40},  /*Button id 1 -> x:160; y:40*/
        {250, 40},  /*Button id 2 -> x:250; y:40*/
        {350, 40},  /*Button id 3 -> x:350; y:40*/
        {450, 40},  /*Button id 4 -> x:450; y:40*/
        {550, 40},  /*Button id 5 -> x:550; y:40*/
        {650, 40},  /*Button id 6 -> x:650; y:40*/
        {735, 40},  /*Button id 7 -> x:735; y:40*/
    };
    lv_indev_set_button_points(button_indev, btn_points);
#endif

#if LV_USE_BUTTON_ADC_SIMULATOR_ENCODER
    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t *button_encoder_indev = lv_indev_create();
    lv_indev_set_type(button_encoder_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(button_encoder_indev, button_adc_input_read_encoder);
    lv_indev_set_group(button_encoder_indev, g);
#endif
}
#endif
#endif
