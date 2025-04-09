/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-11-11        the first version
 */

#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "ft6336.h"
#include "touch_common.h"

#define DBG_TAG "ft6336"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client g_ft6336_client = {0};

static rt_err_t ft6336_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {

        return RT_EOK;
    } else {
        LOG_E("I2C write reg error\n");
        return -RT_ERROR;
    }
}

static rt_err_t ft6336_read_reg(struct rt_i2c_client *dev, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len)
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
        LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }
}

static int16_t g_pre_x[FT6336_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t g_pre_y[FT6336_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t g_tp_dowm[FT6336_MAX_TOUCH] = {0};
static struct rt_touch_data *g_read_data = RT_NULL;

static void ft6336_touch_up(void *buf, int8_t id)
{
    g_read_data = (struct rt_touch_data *)buf;

    if (g_tp_dowm[id] == 1) {
        g_tp_dowm[id] = 0;
        g_read_data[id].event = RT_TOUCH_EVENT_UP;
    } else {
        g_read_data[id].event = RT_TOUCH_EVENT_NONE;
    }

    g_read_data[id].timestamp = rt_touch_get_ts();
    g_read_data[id].x_coordinate = g_pre_x[id];
    g_read_data[id].y_coordinate = g_pre_y[id];
    g_read_data[id].track_id = id;

    g_pre_x[id] = -1; /* last point is none */
    g_pre_y[id] = -1;
}

static void ft6336_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
{
    g_read_data = (struct rt_touch_data *)buf;

    if (g_tp_dowm[id] == 1) {
        g_read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        g_read_data[id].event = RT_TOUCH_EVENT_DOWN;
        g_tp_dowm[id] = 1;
    }

    g_read_data[id].timestamp = rt_touch_get_ts();
    g_read_data[id].x_coordinate = x;
    g_read_data[id].y_coordinate = y;
    g_read_data[id].track_id = id;

    g_pre_x[id] = x; /* save last point */
    g_pre_y[id] = y;
}

static rt_size_t ft6336_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t reg, recombine_id;
    rt_uint8_t touch_status;
    rt_uint8_t read_buf[FT6336_POINT_LEN * FT6336_MAX_TOUCH] = { 0 };
    rt_uint8_t read_index, touch_id[FT6336_MAX_TOUCH] = {0};
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[FT6336_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    reg = FT6336_TD_STATUS;
    if (ft6336_read_reg(&g_ft6336_client, &reg, &touch_status, 1) != RT_EOK) {
        LOG_E("read touch status fail\n");
        read_num = 0;
        goto __exit;
    }

    touch_num = touch_status & 0xf;

    reg = FT6336_TOUCH1_XH;
    if (ft6336_read_reg(&g_ft6336_client, &reg, read_buf, sizeof(read_buf)) != RT_EOK) {
        LOG_E("read touch status fail\n");
        read_num = 0;
        goto __exit;
    }

    for (recombine_id = 0; recombine_id < touch_num; recombine_id++)
        touch_id[recombine_id] = recombine_id;

    if (pre_touch > touch_num) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            rt_uint8_t j;

            for (j = 0; j < touch_num; j++) {
                read_id = touch_id[read_index];

                if (pre_id[read_index] == read_id)
                    break;

                if (j >= touch_num - 1) {
                    rt_uint8_t up_id;
                    up_id = pre_id[read_index];
                    ft6336_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * FT6336_POINT_LEN;
            read_id = touch_id[read_index];
            pre_id[read_index] = read_id;
            input_x = ((read_buf[off_set] & 0xf) << 8) | read_buf[off_set + 1];
            input_y = ((read_buf[off_set + 2] & 0xf) << 8) | read_buf[off_set + 3];

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            ft6336_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            ft6336_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t ft6336_control(struct rt_touch_device *touch, int cmd, void *data)
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

const struct rt_touch_ops ft6336_touch_ops =
{
    .touch_readpoint = ft6336_read_point,
    .touch_control = ft6336_control,
};

struct rt_touch_info ft6336_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_FT,
    5,
    (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
};

int ft6336_hw_init(const char *name, struct rt_touch_config *cfg)
{
    rt_touch_t touch_device = RT_NULL;
    rt_uint8_t cmd[2] = {0};

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ENOMEM;
    }

    /* Reset TP IC */
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);
    rt_thread_delay(50);

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);
    rt_thread_delay(100);

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_INPUT);
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT);

    g_ft6336_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (g_ft6336_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)g_ft6336_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    g_ft6336_client.client_addr = FT6336_SALVE_ADDR;

    cmd[0] = FT6336_DEVICE_MODE;
    cmd[1] = 0;
    if (ft6336_write_reg(&g_ft6336_client, cmd, 2) != RT_EOK)
        return -RT_ERROR;

    cmd[0] = FT6336_G_THGROUP;
    cmd[1] = 12;
    if (ft6336_write_reg(&g_ft6336_client, cmd, 2) != RT_EOK)
        return -RT_ERROR;

    cmd[0] = FT6336_G_PERIODACTIVE;
    cmd[1] = 12;
    if (ft6336_write_reg(&g_ft6336_client, cmd, 2) != RT_EOK)
        return -RT_ERROR;

    /* register touch device */
    touch_device->info = ft6336_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &ft6336_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device ft6336 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device ft6336 init success");
    return RT_EOK;
}

static int rt_hw_ft6336_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rst_pin = rt_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = rt_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    ft6336_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_ft6336_port);
