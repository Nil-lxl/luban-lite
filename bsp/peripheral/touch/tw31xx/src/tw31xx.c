/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-06-24        the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "tw31xx.h"
#define DBG_TAG "tw31xx"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client tw31xx_client;

static rt_err_t tw31xx_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data,
                                rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        rt_kprintf("i2c write tw31xx fail!\n");
        return -RT_ERROR;
    }
}

static rt_err_t tw31xx_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
                                rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = TW31XX_REGITER_LEN;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        rt_kprintf("i2c read tw31xx fail!\n");
        return -RT_ERROR;
    }
}

static rt_err_t tw31xx_get_product_id(struct rt_i2c_client *dev,
                                     rt_uint8_t *data, rt_uint8_t len)
{
    rt_uint8_t reg[2];

    reg[0] = (rt_uint8_t)(TW31XX_REG_CHIP_ID >> 8);
    reg[1] = (rt_uint8_t)(TW31XX_REG_CHIP_ID & 0xff);

    tw31xx_read_regs(dev, reg, data, len);

    return RT_EOK;
}

static void tw31xx_i2c_done(void)
{
    rt_uint8_t buf[3];

    buf[0] = (rt_uint8_t)(TW31XX_REG_COMMAND >> 8);
    buf[1] = (rt_uint8_t)(TW31XX_REG_COMMAND & 0xff);
    buf[2] = 0x01;

    tw31xx_write_reg(&tw31xx_client, buf, 3);
}

static rt_size_t tw31xx_read_point(struct rt_touch_device *touch, void *buf,
                                  rt_size_t read_num)
{
    rt_uint8_t reg[2];
    rt_uint32_t tp_buf;
    rt_uint8_t read_buf[40] = {0};
    struct rt_touch_data *temp_data;
    rt_uint8_t event = RT_TOUCH_EVENT_NONE;
    static rt_uint16_t judge = 0;

    rt_memset(read_buf, 0, sizeof(read_buf));

    temp_data = (struct rt_touch_data *)buf;
    temp_data->event = RT_TOUCH_EVENT_NONE;

    reg[0] = (rt_uint8_t)(TW31XX_REG_EVENT >> 8);
    reg[1] = (rt_uint8_t)(TW31XX_REG_EVENT & 0xff);

    tw31xx_read_regs(&tw31xx_client, reg, read_buf, sizeof(read_buf));

    if (read_buf[0] == TW31XX_EVENT_ABS) {
        reg[0] = (rt_uint8_t)(TW31XX_REG_POINT >> 8);
        reg[1] = (rt_uint8_t)(TW31XX_REG_POINT & 0xff);
        tw31xx_read_regs(&tw31xx_client, reg, read_buf, 4);

        tp_buf = read_buf[0] | read_buf[1] << 8 | read_buf[2] << 16 | read_buf[3] << 24;
        event = (read_buf[3] & 0x80) ? 1 : 0;    // 1: press, 0 : release
        if (event)
            judge++;

        if (event && judge == 1) {
            temp_data->event = RT_TOUCH_EVENT_DOWN;
        } else if (event && judge > 1) {
            temp_data->event = RT_TOUCH_EVENT_MOVE;
        } else if (!event) {
            temp_data->event = RT_TOUCH_EVENT_UP;
            judge = 0;
        } else {
            temp_data->event = RT_TOUCH_EVENT_NONE;
        }

        temp_data->y_coordinate = tp_buf & 0x1fff;
        temp_data->x_coordinate = (tp_buf >> 13) & 0x1fff;
    } else {
        goto __exit;
    }

__exit:
    tw31xx_i2c_done();
    return read_num;
}

static rt_err_t tw31xx_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    struct rt_touch_info *info;

    switch(cmd)
    {
    case RT_TOUCH_CTRL_GET_ID:
        tw31xx_get_product_id(&tw31xx_client, arg, 2);
        break;
    case RT_TOUCH_CTRL_GET_INFO:
        info = (struct rt_touch_info *)arg;
        info->point_num = 1;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;

        break;
    case RT_TOUCH_CTRL_SET_MODE:
        break;
    case RT_TOUCH_CTRL_SET_X_RANGE:
        break;
    case RT_TOUCH_CTRL_SET_Y_RANGE:
        break;
    case RT_TOUCH_CTRL_SET_X_TO_Y:
        break;
    case RT_TOUCH_CTRL_DISABLE_INT:
        break;
    case RT_TOUCH_CTRL_ENABLE_INT:
        break;
    default:
        break;
    }

    return RT_EOK;
}

static struct rt_touch_ops tw31xx_touch_ops = {
    .touch_readpoint = tw31xx_read_point,
    .touch_control = tw31xx_control,
};

static int tw31xx_hw_init(const char *name, struct rt_touch_config *cfg)
{
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device =
        (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));
    //rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    //irq output 1
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_HIGH);
    rt_thread_mdelay(100);
    //rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(100);
    //rst output 0
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_mdelay(100);
    //rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(100);

    tw31xx_client.bus =
        (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (tw31xx_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)tw31xx_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    tw31xx_client.client_addr = TW31XX_ADDR;

    /* get init even */
    rt_uint8_t reg[2];
    rt_uint8_t read_buf[2] = {0};
    reg[0] = (rt_uint8_t)(TW31XX_REG_EVENT >> 8);
    reg[1] = (rt_uint8_t)(TW31XX_REG_EVENT & 0xff);

    tw31xx_read_regs(&tw31xx_client, reg, read_buf, 1);
    if (read_buf[0] == TW31XX_EVENT_GES) {
        tw31xx_i2c_done();
    }

    //rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(100);
    //rst output 0
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_mdelay(100);
    //rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(100);
    //irq input
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT_PULLUP);
    // rst input
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_INPUT);

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_UNKNOWN;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &tw31xx_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device tw31xx init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device tw31xx init success");
    return RT_EOK;
}

static int tw31xx_gpio_cfg()
{
    unsigned int g, p;
    long pin;

    // RST
    pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_direction_input(g, p);

    // INT
    pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_direction_input(g, p);
    hal_gpio_set_irq_mode(g, p, 0);

    return 0;
}

static int rt_hw_tw31xx_init(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    tw31xx_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT_PULLUP;
    cfg.user_data = &rst_pin;

    tw31xx_hw_init("tw31xx", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_tw31xx_init);

