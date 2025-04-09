/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-05-20        the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "touch.h"
#include <rtdbg.h>
#include "axs15260.h"

#define DBG_TAG "axs15260"
#define DBG_LVL DBG_LOG

static struct rt_i2c_client axs15260_client;
static rt_uint8_t write_cmd[11] = {0xb5,0xab,0xa5,0x5a,0x00,0x00,0x00,0x01,0x00,0x00,0x00};

static rt_err_t axs15260_write_regs(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr  = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = data;
    msgs.len   = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        rt_kprintf("axs15260 write register error");
        return -RT_ERROR;
    }
}

static rt_err_t axs15260_read_regs(struct rt_i2c_client *dev,
                                    rt_uint8_t read_len, rt_uint8_t *read_buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr  = dev->client_addr;
    msgs.flags = RT_I2C_RD;
    msgs.buf   = read_buf;
    msgs.len   = read_len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        rt_kprintf("axs15260 read register error");
        return -RT_ERROR;
    }
}

static rt_size_t axs15260_readpoint(struct rt_touch_device *touch, void *data, rt_size_t touch_num)
{
    rt_uint8_t buf[14];
    struct rt_touch_data *temp_data;
    rt_uint8_t  esd_status = 0;
    rt_uint8_t point_num;

    temp_data = (struct rt_touch_data *)data;
    temp_data->event = RT_TOUCH_EVENT_NONE;
    temp_data->timestamp = rt_touch_get_ts();

    if (axs15260_write_regs(&axs15260_client, write_cmd, 11) != RT_EOK) {
        rt_kprintf("write cmd fail\n");
        goto __exit;
    }

    if (axs15260_read_regs(&axs15260_client, 14, buf) != RT_EOK) {
        rt_kprintf("read point fail\n");
        goto __exit;
    }

    /* ESD STATUS */
    esd_status = buf[0] >> 4;
    switch (esd_status)
    {
    case AXS_EDS_ERR:
        rt_kprintf("Rawdata detect anomaly!\n");
        break;
    case AXS_EDS_TIMEOUT:
        rt_kprintf("device scan timeout!\n");
        break;
    case AXS_EDS_HARDWARE:
        rt_kprintf("hardware detect flag!\n");
        break;
    default:
        break;
    }

    point_num = buf[1];
    if (point_num >= AXS_MAX_TOUCH_NUMBER)
        point_num = 1;

    temp_data->event = buf[2] >> 4;
    switch (temp_data->event)
    {
    case AXS_DOWN_EVENT:
        temp_data->event = RT_TOUCH_EVENT_DOWN;
        break;
     case AXS_UP_EVENT:
        temp_data->event = RT_TOUCH_EVENT_UP;
        break;
    case AXS_MOVE_EVENT:
        temp_data->event = RT_TOUCH_EVENT_MOVE;
        break;
    default:
        temp_data->event = RT_TOUCH_EVENT_NONE;
        break;
    }

    temp_data->x_coordinate = ((buf[2] & 0xF) << 8) | buf[3];
    temp_data->y_coordinate = ((buf[4] & 0xF) << 8) | buf[5];

    return touch_num;

__exit:
    return -RT_ERROR;
}

static rt_err_t axs15260_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    struct rt_touch_info *info;

    switch(cmd)
    {
    case RT_TOUCH_CTRL_GET_ID:
        break;
    case RT_TOUCH_CTRL_GET_INFO:
        info = (struct rt_touch_info *)arg;
        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
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

const struct rt_touch_ops axs15260_touch_ops =
{
    .touch_readpoint = axs15260_readpoint,
    .touch_control = axs15260_control,
};

int rt_hw_axs15260_init(const char *name, struct rt_touch_config *cfg)
{
    rt_touch_t touch_device = RT_NULL;

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ENOMEM;
    }
    // rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(10);
    // rst output 0
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_delay(10);
    // rst output 1
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(10);

    axs15260_client.bus =
        (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (axs15260_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)axs15260_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    axs15260_client.client_addr = AXS15260_SLAVE_ADDR;

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_UNKNOWN;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &axs15260_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device axs15260 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device axs15260 init success");
    return RT_EOK;

}

static int rt_axs15260_gpio_cfg()
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

static int rt_hw_axs15260_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rt_axs15260_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    rt_hw_axs15260_init("axs15260", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_axs15260_port);
