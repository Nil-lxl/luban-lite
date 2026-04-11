/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-26        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "cst826.h"
#include "touch_common.h"

#define DBG_TAG "cst826"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client cst826_client;

#if 0
/*
 * may be use this func to do something in the future
 */
static rt_err_t cst826_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data,
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
        return -RT_ERROR;
    }
}
#endif

static rt_err_t cst826_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
                                rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = 1;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

static int16_t pre_x[CST826_MAX_TOUCH] = { -1, -1 };
static int16_t pre_y[CST826_MAX_TOUCH] = { -1, -1 };
static rt_uint8_t s_tp_dowm[CST826_MAX_TOUCH] = {0};
static struct rt_touch_data *read_data = RT_NULL;

static void cst826_touch_up(void *buf, int8_t id)
{
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        s_tp_dowm[id] = 0;
        read_data[id].event = RT_TOUCH_EVENT_UP;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_NONE;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    read_data[id].x_coordinate = pre_x[id];
    read_data[id].y_coordinate = pre_y[id];
    read_data[id].track_id = id;

    pre_x[id] = -1; /* last point is none */
    pre_y[id] = -1;
}

static void cst826_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
{
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_DOWN;
        s_tp_dowm[id] = 1;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
}

static rt_size_t cst826_read_point(struct rt_touch_device *touch, void *buf,
                                  rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t reg;
    rt_uint8_t read_buf[CST826_POINT_LEN * CST826_MAX_TOUCH + 4] = { 0 };
    rt_uint8_t read_index;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[CST826_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    reg = CST826_WORK_MODE;
    if (cst826_read_regs(&cst826_client, &reg, read_buf,
                                sizeof(read_buf)) != RT_EOK) {
        LOG_D("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    touch_num = read_buf[2] & 0x0F;
    if (touch_num >= 2) {
        touch_num = 2;
    }

    if (pre_touch > touch_num)
    {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            rt_uint8_t j;

            for (j = 0; j < touch_num; j++)
            {
                read_id = read_buf[5 + CST826_POINT_LEN * j] & 0xF0;

                if (pre_id[read_index] == read_id)
                    break;

                if (j >= touch_num - 1) {
                    rt_uint8_t up_id;
                    up_id = pre_id[read_index];
                    cst826_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num)
    {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * CST826_POINT_LEN;
            read_id = (read_buf[off_set + 5] & 0xf0) >> 4;
            pre_id[read_index] = read_id;
            input_x = ((read_buf[off_set + 3] & 0x0f) << 8) | read_buf[off_set + 4];
            input_y = ((read_buf[off_set + 5] & 0x0f) << 8) | read_buf[off_set + 6];

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            cst826_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            cst826_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t cst826_control(struct rt_touch_device *touch, int cmd, void *data)
{
    struct rt_touch_info *info = RT_NULL;

    switch(cmd)
    {
    case RT_TOUCH_CTRL_GET_ID:
        break;
    case RT_TOUCH_CTRL_GET_INFO:
        info = (struct rt_touch_info *)data;
        if (info == RT_NULL)
            return -RT_EINVAL;

        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;
        break;
    case RT_TOUCH_CTRL_SET_MODE:
    case RT_TOUCH_CTRL_SET_X_RANGE:
    case RT_TOUCH_CTRL_SET_Y_RANGE:
    case RT_TOUCH_CTRL_SET_X_TO_Y:
    case RT_TOUCH_CTRL_DISABLE_INT:
    case RT_TOUCH_CTRL_ENABLE_INT:
    default:
        break;
    }

    return RT_EOK;
}

const struct rt_touch_ops cst826_touch_ops =
{
    .touch_readpoint = cst826_read_point,
    .touch_control = cst826_control,
};

struct rt_touch_info cst826_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_UNKNOWN,
    2,
    (rt_int32_t)AIC_TOUCH_PANEL_CST826_X_RANGE,
    (rt_int32_t)AIC_TOUCH_PANEL_CST826_Y_RANGE,
};

int cst826_hw_init(const char *name, struct rt_touch_config *cfg)
{
    rt_touch_t touch_device = RT_NULL;

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ENOMEM;
    }

    /* Reset TP IC */
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(10);

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_delay(10);

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(100);

    cst826_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (cst826_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)cst826_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    cst826_client.client_addr = CST826_SALVE_ADDR;

    /* register touch device */

    touch_device->info = cst826_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &cst826_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device cst826 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device cst826 init success");
    return RT_EOK;

}

static int cst826_gpio_cfg()
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

static int rt_hw_cst826_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    cst826_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    cst826_hw_init("cst826", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_cst826_port);
