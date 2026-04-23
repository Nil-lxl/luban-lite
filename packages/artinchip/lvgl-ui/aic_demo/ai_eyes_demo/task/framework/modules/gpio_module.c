/*
 * Copyright (C) 2022-2026 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  fangjie.wang <fangjie.wang@artinchip.com>
 */

#include "gpio_module.h"
#include "../core/module.h"
#include "../core/msg_def.h"
#include "../core/msg_bus.h"
#include <rtthread.h>
#include <aic_core.h>
#include "aic_hal_gpio.h"
#include "drivers/pin.h"

#define DBG_SECTION_NAME "GPIO_MOD"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#define KEY_BUTTON       "PE.13"
#define KEY_VOICE_WAKE   "PA.5"

/* HAL: GPIO function index and drive strength (match schematic/reference pinmux) */
#define GPIO_KEY_PIN_FUNC        1u
#define GPIO_KEY_DRIVE_STRENGTH  PIN_DRV_33V_45_OHM

#define GPIO_THREAD_NAME "gpio_module"
#define GPIO_THREAD_STACK_SIZE (1024 * 4)
#define GPIO_THREAD_PRIORITY 25
#define GPIO_THREAD_TICK 10

/* Key debounce time (ms) */
#define GPIO_KEY_DEBOUNCE_MS     50

/*
 * Periodically request XiaoZhi start-listen: one loop roughly every GPIO_THREAD_TICK ms.
 * Set to 0 to disable (recommended); non-zero sends once every N loops (legacy ~1000 => ~10s).
 */
#ifndef GPIO_PERIODIC_START_LISTEN_INTERVAL_LOOPS
#define GPIO_PERIODIC_START_LISTEN_INTERVAL_LOOPS  0
#endif

/* Pointer tags passed into ISR to avoid strcmp in interrupt context */
#define GPIO_IRQ_TAG_BUTTON  ((void *)(uintptr_t)0x01)
#define GPIO_IRQ_TAG_VOICE   ((void *)(uintptr_t)0x02)

#define GPIO_PENDING_BUTTON  (1u << 0)
#define GPIO_PENDING_VOICE   (1u << 1)

/* GPIO module private data */
typedef struct {
    u32 key_button_pin;
    u32 key_voice_wake_pin;
    rt_bool_t initialized;
} gpio_module_priv_t;

static gpio_module_priv_t gpio_priv = {0};

/* Only set flags in ISR; consumed by gpio thread */
static volatile rt_uint32_t gpio_key_pending;

/* Configure GPIO function */
static void set_gpio_function(const char *pin_name, unsigned char func,
                              unsigned char bias, unsigned char drive)
{
    long pin;
    unsigned int g;
    unsigned int p;

    pin = hal_gpio_name2pin(pin_name);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g, p, func);
    hal_gpio_set_bias_pull(g, p, bias);
    hal_gpio_set_drive_strength(g, p, drive);
}

static void send_xiaozhi_voice_abort(void)
{
    app_message_t abort_msg;

    rt_memset(&abort_msg, 0, sizeof(abort_msg));
    abort_msg.header.type = MSG_TYPE_XIAOZHI_ABORT_SPEAK;
    abort_msg.header.src_module = MODULE_ID_GPIO;
    abort_msg.header.dst_module = MODULE_ID_XIAOZHI;
    abort_msg.header.data_len = 0;
    module_send_msg(&abort_msg);
}

static void send_xiaozhi_key_abort(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_XIAOZHI_ABORT_SPEAK;
    msg.header.src_module = MODULE_ID_GPIO;
    msg.header.dst_module = MODULE_ID_XIAOZHI;
    msg.header.data_len = sizeof(msg_gpio_data_t);
    msg.data.gpio.pin = gpio_priv.key_button_pin;
    msg.data.gpio.state = 1;
    module_send_msg(&msg);
}

static void send_wifi_provision_request(void)
{
    app_message_t msg;
    rt_memset(&msg, 0, sizeof(msg));
    msg.header.type = MSG_TYPE_WIFI_PROVISION_REQUEST;
    msg.header.src_module = MODULE_ID_GPIO;
    msg.header.dst_module = MODULE_ID_WIFI;
    msg.header.data_len = 0;
    module_send_msg(&msg);
}

/* Key business logic called in main thread (after debounce) */
static void dispatch_key_button_pressed(void)
{

    send_xiaozhi_key_abort();
    //send_wifi_provision_request();
}

static void dispatch_key_voice_wake(void)
{
    send_xiaozhi_voice_abort();
}

/* GPIO IRQ handler: mark pending only, no messaging/logging */
static void gpio_input_irq_handler(void *args)
{
    u32 pin;
    unsigned int ret;
    uintptr_t tag = (uintptr_t)args;

    if (tag == (uintptr_t)GPIO_IRQ_TAG_BUTTON) {
        pin = gpio_priv.key_button_pin;
    } else if (tag == (uintptr_t)GPIO_IRQ_TAG_VOICE) {
        pin = gpio_priv.key_voice_wake_pin;
    } else {
        return;
    }

    ret = rt_pin_read(pin);
    if (ret != PIN_LOW) {
        return;
    }

    if (tag == (uintptr_t)GPIO_IRQ_TAG_BUTTON) {
        gpio_key_pending |= GPIO_PENDING_BUTTON;
    } else {
        gpio_key_pending |= GPIO_PENDING_VOICE;
    }
}

/* GPIO init: assign pin IDs before enable to avoid IRQ firing before assignment */
static void gpio_interrupt_init(void)
{
    set_gpio_function(KEY_BUTTON, GPIO_KEY_PIN_FUNC, PIN_PULL_UP, GPIO_KEY_DRIVE_STRENGTH);
    set_gpio_function(KEY_VOICE_WAKE, GPIO_KEY_PIN_FUNC, PIN_PULL_UP, GPIO_KEY_DRIVE_STRENGTH);

    gpio_priv.key_button_pin = (u32)rt_pin_get(KEY_BUTTON);
    gpio_priv.key_voice_wake_pin = (u32)rt_pin_get(KEY_VOICE_WAKE);

    rt_pin_mode((rt_base_t)gpio_priv.key_button_pin, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode((rt_base_t)gpio_priv.key_voice_wake_pin, PIN_MODE_INPUT_PULLUP);

    rt_pin_attach_irq((rt_int32_t)gpio_priv.key_button_pin, PIN_IRQ_MODE_FALLING,
                      gpio_input_irq_handler, GPIO_IRQ_TAG_BUTTON);
    rt_pin_attach_irq((rt_int32_t)gpio_priv.key_voice_wake_pin, PIN_IRQ_MODE_FALLING,
                      gpio_input_irq_handler, GPIO_IRQ_TAG_VOICE);

    rt_pin_irq_enable((rt_base_t)gpio_priv.key_button_pin, PIN_IRQ_ENABLE);
    rt_pin_irq_enable((rt_base_t)gpio_priv.key_voice_wake_pin, PIN_IRQ_ENABLE);

    gpio_priv.initialized = RT_TRUE;
}

static void gpio_interrupt_deinit(void)
{
    if (gpio_priv.initialized != RT_TRUE) {
        return;
    }

    if (gpio_priv.key_button_pin != 0) {
        rt_pin_irq_enable((rt_base_t)gpio_priv.key_button_pin, PIN_IRQ_DISABLE);
        rt_pin_detach_irq((rt_int32_t)gpio_priv.key_button_pin);
    }
    if (gpio_priv.key_voice_wake_pin != 0) {
        rt_pin_irq_enable((rt_base_t)gpio_priv.key_voice_wake_pin, PIN_IRQ_DISABLE);
        rt_pin_detach_irq((rt_int32_t)gpio_priv.key_voice_wake_pin);
    }

    gpio_priv.key_button_pin = 0;
    gpio_priv.key_voice_wake_pin = 0;
    gpio_priv.initialized = RT_FALSE;
    gpio_key_pending = 0;
}

static void gpio_thread_process_keys(void)
{
    rt_uint32_t pending;
    rt_base_t level;
    static rt_tick_t last_btn_tick;
    static rt_tick_t last_voice_tick;
    const rt_tick_t debounce_ticks = rt_tick_from_millisecond(GPIO_KEY_DEBOUNCE_MS);
    rt_tick_t now;

    level = rt_hw_interrupt_disable();
    pending = gpio_key_pending;
    gpio_key_pending = 0;
    rt_hw_interrupt_enable(level);

    now = rt_tick_get();

    if (pending & GPIO_PENDING_BUTTON) {
        if ((rt_tick_t)(now - last_btn_tick) >= debounce_ticks) {
            last_btn_tick = now;
            LOG_D("key button pressed, pin=%u", (unsigned int)gpio_priv.key_button_pin);
            dispatch_key_button_pressed();
        }
    }

    if (pending & GPIO_PENDING_VOICE) {
        if ((rt_tick_t)(now - last_voice_tick) >= debounce_ticks) {
            last_voice_tick = now;
            LOG_D("key voice wake pressed, pin=%u", (unsigned int)gpio_priv.key_voice_wake_pin);
            dispatch_key_voice_wake();
        }
    }
}

/* GPIO module message handler */
static int gpio_msg_handler(const app_message_t *msg)
{
    if (msg == RT_NULL) {
        return -RT_EINVAL;
    }

    switch (msg->header.type) {
        case MSG_TYPE_SYS_INIT:
            LOG_I("GPIO module received init message");
            break;

        default:
            LOG_D("GPIO module received unhandled msg type: 0x%02X", msg->header.type);
            break;
    }

    return RT_EOK;
}

/* GPIO module init */
static int gpio_init(void)
{
    rt_memset(&gpio_priv, 0, sizeof(gpio_priv));
    gpio_priv.initialized = RT_FALSE;
    gpio_key_pending = 0;

    LOG_I("GPIO module registered (hardware init in gpio thread)");

    msg_bus_subscribe(MODULE_ID_GPIO, MSG_TYPE_SYS_INIT);

    return RT_EOK;
}

/* GPIO module deinit */
static int gpio_deinit(void)
{
    gpio_interrupt_deinit();
    msg_bus_unsubscribe(MODULE_ID_GPIO, MSG_TYPE_SYS_INIT);

    LOG_I("GPIO module deinitialized");

    return RT_EOK;
}

/* GPIO module thread entry */
static void gpio_thread_entry(void *parameter)
{
#if GPIO_PERIODIC_START_LISTEN_INTERVAL_LOOPS > 0
    static int tick;
#endif

    (void)parameter;
    LOG_I("GPIO module thread started");

    gpio_interrupt_init();

    while (1) {
        app_message_t msg;

        if (msg_bus_try_receive(MODULE_ID_GPIO, &msg) == RT_EOK) {
            gpio_msg_handler(&msg);
        }

        gpio_thread_process_keys();

#if GPIO_PERIODIC_START_LISTEN_INTERVAL_LOOPS > 0
        if (tick++ % GPIO_PERIODIC_START_LISTEN_INTERVAL_LOOPS == 0) {
            app_message_t listen_msg;

            rt_memset(&listen_msg, 0, sizeof(listen_msg));
            listen_msg.header.type = MSG_TYPE_XIAOZHI_START_LISTEN;
            listen_msg.header.src_module = MODULE_ID_GPIO;
            listen_msg.header.dst_module = MODULE_ID_XIAOZHI;
            listen_msg.header.data_len = 0;
            module_send_msg(&listen_msg);
        }
#endif

        rt_thread_mdelay(GPIO_THREAD_TICK);
    }
}

/* GPIO module operations */
static module_ops_t gpio_ops = {
    .init = gpio_init,
    .deinit = gpio_deinit,
    .msg_handler = gpio_msg_handler,
    .name = "GPIO Module",
    .id = MODULE_ID_GPIO,
};

/* GPIO module instance */
static module_t gpio_module = {
    .ops = &gpio_ops,
    .active = RT_FALSE,
    .thread = RT_NULL,
    .priv_data = &gpio_priv,
};

/* Get GPIO module instance */
module_t *gpio_module_get(void)
{
    return &gpio_module;
}

/* Initialize GPIO module thread */
int gpio_module_thread_start(void)
{
    gpio_module.thread = rt_thread_create(GPIO_THREAD_NAME,
                                          gpio_thread_entry,
                                          RT_NULL,
                                          GPIO_THREAD_STACK_SIZE,
                                          GPIO_THREAD_PRIORITY,
                                          GPIO_THREAD_TICK);
    if (gpio_module.thread != RT_NULL) {
        rt_thread_startup(gpio_module.thread);
        return RT_EOK;
    }
    return -RT_ERROR;
}
