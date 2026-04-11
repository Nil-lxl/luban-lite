/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-04-11        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include "aic_hal_gpio.h"

#define DBG_TAG "hec40"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_device g_hec40_dev = {0};
static int32_t g_encoder_a = 0;
static int32_t g_encoder_b = 0;
static int32_t g_encoder_cnt = 0;

static void hec40_pina_calculate_cb(void *args)
{
    int32_t encoder_a = rt_pin_read(g_encoder_a);
    int32_t encoder_b = rt_pin_read(g_encoder_b);

    if (encoder_a ^ encoder_b)
        g_encoder_cnt++;
    else
        g_encoder_cnt--;
}

static void hec40_pinb_calculate_cb(void *args)
{
    int32_t encoder_a = rt_pin_read(g_encoder_a);
    int32_t encoder_b = rt_pin_read(g_encoder_b);

    if (encoder_a ^ encoder_b)
        g_encoder_cnt--;
    else
        g_encoder_cnt++;
}

static void hec40_pin_init(void)
{
    uint32_t encodec_pina_g, encodec_pina_p, encodec_pinb_g, encodec_pinb_p;

    g_encoder_a = rt_pin_get(AIC_ENCODER_PIN_A);
    g_encoder_b = rt_pin_get(AIC_ENCODER_PIN_B);

    encodec_pina_g = g_encoder_a / 32;
    encodec_pinb_g = g_encoder_b / 32;
    encodec_pina_p = g_encoder_a % 32;
    encodec_pinb_p = g_encoder_b % 32;

    rt_pin_mode((uint32_t)g_encoder_a, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode((uint32_t)g_encoder_b, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq((uint32_t)g_encoder_a, PIN_IRQ_MODE_RISING_FALLING, hec40_pina_calculate_cb, &g_encoder_a);
    rt_pin_attach_irq((uint32_t)g_encoder_b, PIN_IRQ_MODE_RISING_FALLING, hec40_pinb_calculate_cb, &g_encoder_b);

    hal_gpio_set_debounce(encodec_pina_g, encodec_pina_p, 0xFFF);
    hal_gpio_set_debounce(encodec_pinb_g, encodec_pinb_p, 0xFFF);

    rt_pin_irq_enable((uint32_t)g_encoder_a, PIN_IRQ_ENABLE);
    rt_pin_irq_enable((uint32_t)g_encoder_b, PIN_IRQ_ENABLE);
}

static rt_err_t hec40_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t hec40_open(rt_device_t dev, rt_uint16_t flag)
{
    return RT_EOK;
}

static rt_err_t hec40_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t hec40_read(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t size)
{
    rt_base_t level;
    rt_size_t pulse_cnt = 0;

    if (buf == RT_NULL)
        return -RT_ERROR;

    level = rt_hw_interrupt_disable();
    pulse_cnt = g_encoder_cnt;

    if (pulse_cnt / 4) {
        pulse_cnt /= 4;
        rt_memcpy(buf, &pulse_cnt, sizeof(int32_t));
        g_encoder_cnt -= pulse_cnt * 4;
    }

    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t hec40_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops hec40_ops =
{
    .init = hec40_init,
    .open = hec40_open,
    .close = hec40_close,
    .read = hec40_read,
    .control = hec40_control,
};
#endif

int rt_hw_hec40_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_hec40_dev.ops = &hec40_ops;
#else
    g_hec40_dev.init = hec40_init;
    g_hec40_dev.open = hec40_open;
    g_hec40_dev.close = hec40_close;
    g_hec40_dev.read = hec40_read;
    g_hec40_dev.control = hec40_control;
#endif
    hec40_pin_init();
    rt_device_register(&g_hec40_dev, AIC_ENCODER_NAME, RT_DEVICE_FLAG_STANDALONE);
    g_encoder_cnt = 0;
    LOG_I("hec40 init");

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_hec40_init);



/* Test sample */
#define TEST_THREAD_STACK_SIZE 1024 * 2
#define TEST_THREAD_PRIORITY   25

static void test_hec40_thread_entry(void *parameter)
{
    rt_device_t dev = RT_NULL;
    int32_t buffer[1] = {0};

    dev = rt_device_find(AIC_ENCODER_NAME);
    if (!dev) {
        LOG_E("Device %s not found!", AIC_ENCODER_NAME);
        return;
    }

    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDONLY) != RT_EOK) {
        LOG_E("Failed to open device %s!", AIC_ENCODER_NAME);
        return;
    }

    LOG_I("Start reading from device %s every 20ms...", AIC_ENCODER_NAME);

    while (1) {
        rt_device_read(dev, 0, buffer, sizeof(buffer));
        if (buffer[0] != 0)
            LOG_I("Pulse count: %d", buffer[0]);

        rt_thread_mdelay(20);
    }

    rt_device_close(dev);
}


static void test_hec40(int argc, char *argv[])
{

    rt_thread_t tid = rt_thread_create("test_hec40", test_hec40_thread_entry, RT_NULL,
                                        TEST_THREAD_STACK_SIZE, TEST_THREAD_PRIORITY, 10);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
        LOG_I("Test thread 'test_hec40' started.");
    } else {
        LOG_E("Failed to create test thread!");
    }

    return;
}
MSH_CMD_EXPORT(test_hec40, test hec40 sample);

