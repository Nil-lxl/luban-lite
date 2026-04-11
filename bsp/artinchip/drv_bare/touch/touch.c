/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "touch.h"

struct touch *register_touch = NULL;

int touch_control(struct touch *touch, int cmd, void *arg)
{
    CHECK_PARAM(touch, -1);

    if (touch->ops->control)
        return touch->ops->control(touch, cmd , arg);

    return TOUCH_OK;
}

int touch_readpoint(struct touch *touch, void *buf, unsigned int len)
{
    CHECK_PARAM(touch, -1);

    if (touch->ops->readpoint)
        return touch->ops->readpoint(touch, buf, len);

    return TOUCH_OK;
}

int touch_get_i2c_chan_id(const char *name)
{

    uint8_t name_len;
    int chan_id = -1;

    name_len = strlen(name);

    if (name_len != 4)
        return TOUCH_ERROR;

    if ((name[0] != 'i') || (name[1] != '2') || (name[2] != 'c'))
        return TOUCH_ERROR;

    chan_id = (int)(name[3] - '0');

    return chan_id;
}

long touch_pin_get(const char *name)
{
    return hal_gpio_name2pin(name);
}

void touch_set_pin_mode(long pin, long mode)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    switch (mode)
    {
    case PIN_INPUT_MODE:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_INPUT_PULLUP_MODE:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_UP);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_INPUT_PULLDOWN_MODE:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DOWN);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_OUTPUT_MODE:
    case PIN_OUTPUT_OD_MODE:
    default:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
        hal_gpio_direction_output(g, p);
        break;
    }
}

void touch_pin_write(long pin, long value)
{
    int ret;
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    if (LOW_POWER == value)
    {
        hal_gpio_clr_output(g, p);
    }
    else
    {
        hal_gpio_set_output(g, p);
        ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_OE);
        if (ret < 0) {
            pr_err("Set the output pin failed\n");
        } else {
            pr_debug("Set the output pin successfully\n");
        }
    }
}

void touch_register(struct touch *touch)
{
    CHECK_PARAM_RET(touch);

    register_touch = touch;
}

void touch_unregister(void)
{
    register_touch = NULL;
}
