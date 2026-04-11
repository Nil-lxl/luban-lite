/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-03-10        the first version
 */

#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "chsc5xxx.h"
#include "touch_common.h"

#define DBG_TAG "chsc5xxx"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client chsc5xxx_client;

static rt_err_t chsc5xxx_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
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

static rt_err_t chsc5xxx_read_regs(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_RD;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }
}

static rt_err_t chsc5xxx_get_info(struct rt_i2c_client *dev, struct rt_touch_device *touch,
                                    struct rt_touch_info *info)
{
    rt_uint8_t reg[4] = {0};
    rt_uint8_t out_info[4] = {0};

    reg[0] = (rt_uint8_t)((CHSC5XXX_X_RESOLUTION & 0xff000000) >> 24);
    reg[1] = (rt_uint8_t)((CHSC5XXX_X_RESOLUTION & 0x00ff0000) >> 16);
    reg[2] = (rt_uint8_t)((CHSC5XXX_X_RESOLUTION & 0x0000ff00) >> 8);
    reg[3] = (rt_uint8_t)(CHSC5XXX_X_RESOLUTION & 0x000000ff);

    if (chsc5xxx_write_reg(dev, reg, 4) != RT_EOK) {
        LOG_E("read info error!\n");
        return -RT_ERROR;
    }

    if (chsc5xxx_read_regs(&chsc5xxx_client, out_info, sizeof(out_info)) != RT_EOK) {
        LOG_E("read info fail!\n");
        return -RT_ERROR;
    }

    info->range_x = (out_info[1] << 8) | out_info[0];
    info->range_y = (out_info[3] << 8) | out_info[2];
    info->point_num = CHSC5XXX_MAX_TOUCH;

    return RT_EOK;
}

static int16_t pre_x[CHSC5XXX_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[CHSC5XXX_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t s_tp_dowm[CHSC5XXX_MAX_TOUCH] = {0};
static struct rt_touch_data *g_read_data = RT_NULL;

static void chsc5xxx_touch_up(void *buf, int8_t id)
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

static void chsc5xxx_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
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

static rt_size_t chsc5xxx_read_point(struct rt_touch_device *touch, void *buf,
                                  rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t recombine_id;
    rt_uint8_t reg[4] = {0};
    rt_uint8_t touch_type;
    rt_uint8_t read_buf[CHSC5XXX_POINT_LEN * CHSC5XXX_MAX_TOUCH + 3] = { 0 };
    rt_uint8_t read_index, touch_id[CHSC5XXX_MAX_TOUCH] = {0};
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[CHSC5XXX_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);


    reg[0] = (rt_uint8_t)((CHSC5XXX_EVENT_TYPE & 0xff000000) >> 24);
    reg[1] = (rt_uint8_t)((CHSC5XXX_EVENT_TYPE & 0x00ff0000) >> 16);
    reg[2] = (rt_uint8_t)((CHSC5XXX_EVENT_TYPE & 0x0000ff00) >> 8);
    reg[3] = (rt_uint8_t)(CHSC5XXX_EVENT_TYPE & 0x000000ff);

    if (chsc5xxx_write_reg(&chsc5xxx_client, reg, 4) != RT_EOK) {
        LOG_E("read info error!\n");
        return -RT_ERROR;
    }

    if (chsc5xxx_read_regs(&chsc5xxx_client, read_buf, sizeof(read_buf)) != RT_EOK) {
        LOG_E("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    touch_type = read_buf[0];
    if (touch_type != CHSC5XXX_NORNAL_TOUCH) {
        LOG_D("error type:%d\n", touch_type);
        read_num = 0;
        goto __exit;
    }

    touch_num = read_buf[1] & 0x0f;
    if (touch_num > CHSC5XXX_MAX_TOUCH)
        touch_num = CHSC5XXX_MAX_TOUCH;

    for (recombine_id = 0; recombine_id < touch_num; recombine_id++)
        touch_id[recombine_id] = recombine_id;

    if (pre_touch > touch_num) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            bool found = false;

            for (int j = 0; j < touch_num; j++) {
                if (pre_id[read_index] == touch_id[j]) {
                    found = true;
                    break;
                }
            }
            if (!found)
                chsc5xxx_touch_up(buf, pre_id[read_index]);
        }
    }

    if (touch_num) /* point down */
    {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * CHSC5XXX_POINT_LEN;
            read_id = touch_id[read_index];
            pre_id[read_index] = read_id;
            input_x = read_buf[off_set + 2] | ((read_buf[off_set + 5] & 0x0f) << 8);
            input_y = read_buf[off_set + 3] | (((read_buf[off_set + 5] & 0xf0) >> 4) << 8);

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            chsc5xxx_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            chsc5xxx_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t chsc5xxx_control(struct rt_touch_device *touch, int cmd, void *data)
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
        chsc5xxx_get_info(&chsc5xxx_client, touch, info);

        break;
    case RT_TOUCH_CTRL_SET_MODE:
    case RT_TOUCH_CTRL_SET_X_RANGE:
    case RT_TOUCH_CTRL_SET_Y_RANGE:
    case RT_TOUCH_CTRL_SET_X_TO_Y:
    case RT_TOUCH_CTRL_DISABLE_INT:
    case RT_TOUCH_CTRL_ENABLE_INT:
    default:
        LOG_W("The current cmd operation:%d is not supported!\n", cmd);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

const struct rt_touch_ops chsc5xxx_touch_ops =
{
    .touch_readpoint = chsc5xxx_read_point,
    .touch_control = chsc5xxx_control,
};

struct rt_touch_info chsc5xxx_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_UNKNOWN,
    CHSC5XXX_MAX_TOUCH,
    (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
};

int chsc5xxx_hw_init(const char *name, struct rt_touch_config *cfg)
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
    rt_thread_delay(200);
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT);

    chsc5xxx_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (chsc5xxx_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        goto __exit;
    }

    if (rt_device_open((rt_device_t)chsc5xxx_client.bus, RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        goto __exit;
    }

    chsc5xxx_client.client_addr = CHSC5XXX_SALVE_ADDR;

    /* register touch device */

    touch_device->info = chsc5xxx_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &chsc5xxx_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device chsc5xxx init failed !!!");
        goto __exit;
    }

    LOG_I("touch device chsc5xxx init success");

__exit:
    rt_free(touch_device);
    return -RT_ERROR;
}

static int rt_hw_chsc5xxx_port(void)
{
    struct rt_touch_config cfg = {0};
    rt_uint8_t rst_pin = 0;

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    chsc5xxx_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_chsc5xxx_port);
