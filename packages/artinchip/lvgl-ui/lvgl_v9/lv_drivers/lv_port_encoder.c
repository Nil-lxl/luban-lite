/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Zequan Liang <zequan.liang@artinchip.com>
 */

#include "lvgl.h"
#ifdef AIC_USING_ENCODER
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdevice.h>
#endif

#define ENC_DIFF_LIMIT_VALUE(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

typedef struct {
    int16_t enc_diff;
    lv_indev_state_t state;
    lv_indev_t *indev;
    lv_timer_t *event_timer;
#if defined(KERNEL_RTTHREAD)
    rt_device_t dev;
#endif
} lv_aic_encoder_t;

static int init_encoder(lv_aic_encoder_t *encoder);
static void read_encoder_input(lv_indev_t *indev, lv_indev_data_t *data);
static void cleanup_input_device(lv_event_t *e);
static void handle_encoder_event(lv_timer_t *t);

#if defined(KERNEL_RTTHREAD)
static int normal_device_init(lv_aic_encoder_t *encoder);
static int rtthread_encoder_init(lv_aic_encoder_t *encoder);
static void rtthread_device_deinit(lv_aic_encoder_t *dsc);
static int get_encoder_diff(lv_aic_encoder_t *encoder);
static int get_encoder_state(lv_aic_encoder_t *encoder);
#if USE_ADC_ENCODER_SIMULATOR
static int adc_simulator_init(lv_aic_encoder_t *encoder);
static void handle_adc_button(lv_aic_encoder_t *encoder);
static int simulator_encoder_input(int button_id);
static int read_adc_button(void);
#endif
#endif

#if USE_ADC_ENCODER_SIMULATOR
#define BUTTON_ID_1  0x00
#define BUTTON_ID_0  0x01

static rt_adc_device_t g_adc_dev;
static const char *g_adc_name = "gpai";
static const int g_adc_chan = 2;
#endif

lv_indev_t *lv_aic_encoder_create(void)
{
    lv_aic_encoder_t *dsc = lv_malloc(sizeof(lv_aic_encoder_t));
    LV_ASSERT_MALLOC(dsc);
    if (dsc == NULL) {
        return NULL;
    }

    lv_memzero(dsc, sizeof(lv_aic_encoder_t));

    if (init_encoder(dsc) < 0) {
        lv_free(dsc);
        return NULL;
    }

    lv_indev_t *indev = lv_indev_create();
    if (indev == NULL) {
        lv_free(dsc);
        return NULL;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, read_encoder_input);
    lv_indev_set_driver_data(indev, dsc);
    lv_indev_add_event_cb(indev, cleanup_input_device, LV_EVENT_DELETE, indev);

    dsc->event_timer = lv_timer_create(handle_encoder_event, 20, dsc);
    dsc->indev = indev;

    return indev;
}

static int init_encoder(lv_aic_encoder_t *encoder)
{
#if defined(KERNEL_RTTHREAD)
    if (rtthread_encoder_init(encoder) < 0) {
        return -1;
    }
#endif

    encoder->enc_diff = 0;
    encoder->state = LV_INDEV_STATE_RELEASED;
    return 0;
}

#if defined(KERNEL_RTTHREAD)
static int rtthread_encoder_init(lv_aic_encoder_t *encoder)
{
#if USE_ADC_ENCODER_SIMULATOR
    return adc_simulator_init(encoder);
#else
    return normal_device_init(encoder);
#endif
}

static int normal_device_init(lv_aic_encoder_t *encoder)
{
    encoder->dev = rt_device_find(AIC_ENCODER_NAME);
    if (!encoder->dev) {
        LV_LOG_ERROR("Device %s not found!", AIC_ENCODER_NAME);
        return -1;
    }

    if (rt_device_open(encoder->dev, RT_DEVICE_OFLAG_RDONLY) != RT_EOK) {
        LV_LOG_ERROR("Failed to open device %s!", AIC_ENCODER_NAME);
        return -1;
    }

    return 0;
}

#if USE_ADC_ENCODER_SIMULATOR
static int adc_simulator_init(lv_aic_encoder_t *encoder)
{
    g_adc_dev = (rt_adc_device_t)rt_device_find(g_adc_name);
    if (!g_adc_dev) {
        LV_LOG_ERROR("Failed to open %s device", g_adc_name);
        return -1;
    }

    rt_adc_enable(g_adc_dev, g_adc_chan);
    encoder->dev = NULL;
    return 0;
}
#endif
#endif

static void read_encoder_input(lv_indev_t *indev, lv_indev_data_t *data)
{
    lv_aic_encoder_t *dsc = lv_indev_get_driver_data(indev);
    data->state = dsc->state;
    data->enc_diff = dsc->enc_diff;
    dsc->enc_diff = 0;
}

static void cleanup_input_device(lv_event_t *e)
{
    lv_indev_t *indev = (lv_indev_t *)lv_event_get_user_data(e);
    lv_aic_encoder_t *dsc = lv_indev_get_driver_data(indev);

    if (dsc) {
        lv_indev_set_driver_data(indev, NULL);
        lv_indev_set_read_cb(indev, NULL);

        if (dsc->event_timer) {
            lv_timer_delete(dsc->event_timer);
        }

#if defined(KERNEL_RTTHREAD)
        rtthread_device_deinit(dsc);
#endif

        lv_free(dsc);
        LV_LOG_INFO("Device released");
    }
}

#if defined(KERNEL_RTTHREAD)
static void rtthread_device_deinit(lv_aic_encoder_t *dsc)
{
    if (dsc->dev) {
        rt_device_close(dsc->dev);
    }

#if USE_ADC_ENCODER_SIMULATOR
    if (g_adc_dev) {
        rt_adc_disable(g_adc_dev, g_adc_chan);
    }
#endif
}
#endif

static void handle_encoder_event(lv_timer_t *t)
{
    lv_aic_encoder_t *encoder = t->user_data;
    int32_t enc_diff = 0;
    lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
#if defined(KERNEL_RTTHREAD)
    enc_diff = get_encoder_diff(encoder);
    state = get_encoder_state(encoder);
#endif
    encoder->enc_diff = enc_diff;
    encoder->state = state;
#if USE_ADC_ENCODER_SIMULATOR
    handle_adc_button(encoder);
#endif
}

#if defined(KERNEL_RTTHREAD)
static int get_encoder_diff(lv_aic_encoder_t *encoder)
{
    if (!encoder->dev) {
        return 0;
    }

    int32_t buffer[1] = {0};
    rt_device_read(encoder->dev, 0, buffer, sizeof(buffer));
    return ENC_DIFF_LIMIT_VALUE(buffer[0]);
}

/* To do */
static int get_encoder_state(lv_aic_encoder_t *encoder)
{
    return LV_INDEV_STATE_RELEASED;
}
#endif

#if USE_ADC_ENCODER_SIMULATOR
static void handle_adc_button(lv_aic_encoder_t *encoder)
{
    static int last_button_id = 0;
    int btn_pr = read_adc_button();

    if (btn_pr >= 0) {
        if (btn_pr == BUTTON_ID_0) {
            encoder->state = LV_INDEV_STATE_PRESSED;
        }
    } else {
        if (last_button_id == BUTTON_ID_0) {
            encoder->state = LV_INDEV_STATE_RELEASED;
        }
    }

    encoder->enc_diff = simulator_encoder_input(btn_pr);
    last_button_id = btn_pr;
}

static int simulator_encoder_input(int button_id)
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

    if (button_id == BUTTON_ID_0) {
        return 0;
    }

    if (last_button_id == button_id) {
        if (press_time > 150) {
            start_time = lv_tick_get();
            ret = (button_id == BUTTON_ID_1) ? -1 : 1;
            long_press = 1;
        }
    } else {
        if (button_id < 0 && last_button_id >= 0 && !long_press) {
            ret = (last_button_id == BUTTON_ID_1) ? -1 : 1;
        } else if (button_id >= 0 && last_button_id >= 0) {
            ret = (button_id == BUTTON_ID_1) ? -1 : 1;
        }
        long_press = 0;
    }

    last_button_id = button_id;
    return ret;
}

static int read_adc_button(void)
{
    const int threshold_range = 100;
    static int button_adc_arr[] = {497, 3340, 1351, 2456, 2256, 2156, 1234, 2234};
    static int button_id[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};

    int adc_val = rt_adc_read(g_adc_dev, g_adc_chan);

    for (int i = 0; i < sizeof(button_adc_arr)/sizeof(button_adc_arr[0]); i++) {
        if ((button_adc_arr[i] - threshold_range) <= adc_val &&
            adc_val <= (button_adc_arr[i] + threshold_range)) {
            LV_LOG_INFO("[button%d] ch%d: %d", button_id[i], g_adc_chan, adc_val);
            return button_id[i];
        }
    }
    return -1;
}
#endif
#endif
