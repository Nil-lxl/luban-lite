/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-06-11        the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "touch.h"
#include "cst3240.h"
#include "touch_common.h"

#define DBG_TAG "cst3240"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static struct rt_i2c_client cst3240_client;

static rt_err_t cst3240_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data,
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

static rt_err_t cst3240_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
                                rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = 2;

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

static int16_t pre_x[CST3240_MAX_TOUCH_NUM] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[CST3240_MAX_TOUCH_NUM] = { -1, -1, -1, -1, -1 };
static rt_uint8_t s_tp_dowm[CST3240_MAX_TOUCH_NUM] = {0};
static struct rt_touch_data *g_read_data = RT_NULL;

static void cst3240_touch_up(void *buf, int8_t id)
{
    g_read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        s_tp_dowm[id] = 0;
        g_read_data[id].event = RT_TOUCH_EVENT_UP;
    } else {
        g_read_data[id].event = RT_TOUCH_EVENT_NONE;
    }

    g_read_data[id].timestamp = rt_touch_get_ts();
    g_read_data[id].x_coordinate = pre_x[id];
    g_read_data[id].y_coordinate = pre_y[id];
    g_read_data[id].track_id = id;

    pre_x[id] = -1; /* last point is none */
    pre_y[id] = -1;
}

static void cst3240_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
{
    g_read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        g_read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        g_read_data[id].event = RT_TOUCH_EVENT_DOWN;
        s_tp_dowm[id] = 1;
    }

    g_read_data[id].timestamp = rt_touch_get_ts();
    g_read_data[id].x_coordinate = x;
    g_read_data[id].y_coordinate = y;
    g_read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
}
static rt_size_t cst3240_readpoint(struct rt_touch_device *touch, void *buf, rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t reg[2];
    rt_uint8_t read_buf[CST3240_INFO_LEN * CST3240_MAX_TOUCH_NUM + 2] = { 0 };
    rt_uint8_t read_index, effective_fingers = 0;
    rt_uint8_t cmd[3];
    rt_uint8_t id[5] = {0};
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[CST3240_MAX_TOUCH_NUM] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    reg[0] = (rt_uint8_t)((CST3240_POINT1_REG >> 8) & 0xFF);
    reg[1] = (rt_uint8_t)(CST3240_POINT1_REG & 0xFF);

    if (cst3240_read_regs(&cst3240_client, reg, read_buf, 5) != RT_EOK) {
        LOG_D("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    reg[0] = (rt_uint8_t)((CST3240_POINT2_REG >> 8) & 0xFF);
    reg[1] = (rt_uint8_t)(CST3240_POINT2_REG & 0xFF);

    if (cst3240_read_regs(&cst3240_client, reg, &read_buf[5], 20) != RT_EOK) {
        LOG_D("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    for (int i = 0; i < CST3240_MAX_TOUCH_NUM; i++) {
        effective_fingers = ((read_buf[i * CST3240_MAX_TOUCH_NUM] & 0xf) == 0x6);
        touch_num += effective_fingers;
    }

    for (int recombine_id = 0; recombine_id < touch_num; recombine_id++)
        id[recombine_id] = recombine_id;

    if (pre_touch > touch_num)
    {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            rt_uint8_t j;

            for (j = 0; j < touch_num; j++)
            {
                read_id = id[read_index];

                if (pre_id[read_index] == read_id)
                    break;

                if (j >= touch_num - 1) {
                    rt_uint8_t up_id;
                    up_id = pre_id[read_index];
                    cst3240_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num)
    {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * CST3240_INFO_LEN;
            read_id = id[read_index];

            pre_id[read_index] = read_id;
            input_x = ((rt_uint16_t)read_buf[off_set + 1] << 4) | ((read_buf[off_set + 3] >> 4) & 0x0f);
            input_y = ((rt_uint16_t)read_buf[off_set + 2] << 4) | (read_buf[off_set + 3] & 0x0f);

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            cst3240_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            cst3240_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    cmd[0] = (rt_uint8_t)(CST3240_FIXED_REG >> 8);
    cmd[1] = (rt_uint8_t)(CST3240_FIXED_REG & 0xFF);
    cmd[2] = 0xAB;
    cst3240_write_reg(&cst3240_client, cmd, 3);

    return read_num;
}

static rt_err_t cst3240_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    struct rt_touch_info *info;

    switch(cmd)
    {
    case RT_TOUCH_CTRL_GET_ID:
        break;
    case RT_TOUCH_CTRL_GET_INFO:
        info = (struct rt_touch_info *)arg;
        if (info == RT_NULL)
            return -RT_EINVAL;

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

const struct rt_touch_ops cst3240_touch_ops =
{
    .touch_readpoint = cst3240_readpoint,
    .touch_control = cst3240_control,
};

struct rt_touch_info cst3240_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_UNKNOWN,
    5,
    (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
};

int rt_hw_cst3240_init(const char *name, struct rt_touch_config *cfg)
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
    rt_thread_delay(50);

    cst3240_client.bus =
        (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (cst3240_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)cst3240_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    cst3240_client.client_addr = CST3240_SLAVE_ADDR;

    /* register touch device */
    touch_device->info = cst3240_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &cst3240_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device cst3240 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device cst3240 init success");
    return RT_EOK;

}

static int rt_cst3240_gpio_cfg()
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

static int rt_hw_cst3240_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rt_cst3240_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    rt_hw_cst3240_init("cst3240", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_cst3240_port);
