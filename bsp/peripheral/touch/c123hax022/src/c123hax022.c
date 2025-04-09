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
#include "c123hax022.h"
#include "touch_common.h"

#define DBG_TAG "c123hax022"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client g_c123hax022_client = {0};

static rt_err_t c123hax022_read_regs(struct rt_i2c_client *dev, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr  = dev->client_addr;
    msgs.flags = RT_I2C_RD;
    msgs.buf   = buf;
    msgs.len   = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        LOG_E("c123hax022 read register error");
        return -RT_ERROR;
    }
}

static int16_t g_pre_x[C123HAX022_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t g_pre_y[C123HAX022_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t g_tp_dowm[C123HAX022_MAX_TOUCH] = {0};
static struct rt_touch_data *g_read_data = RT_NULL;

static void c123hax022_touch_up(void *buf, int8_t id)
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

static void c123hax022_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
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

static rt_size_t c123hax022_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t recombine_id;
    rt_uint8_t status_buf[4] = {0};
    rt_uint8_t read_buf[C123HAX022_POINT_LEN * C123HAX022_MAX_TOUCH + 1] = { 0 };
    rt_uint8_t read_index, touch_id[C123HAX022_MAX_TOUCH] = {0};
    int8_t num, read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[C123HAX022_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    if (c123hax022_read_regs(&g_c123hax022_client, 4, status_buf) != RT_EOK) {
        LOG_E("read touch status fail\n");
        read_num = 0;
        goto __exit;
    }

    if (status_buf[0] == 0xA5 && status_buf[1] == 0x11) {
        num = status_buf[2] + 2;
        if (num > 35)
            num = 35;
    } else {
        read_num = 0;
        LOG_D("touch status error\n");
        goto __exit;
    }

    touch_num = status_buf[2] / 7;

    if (c123hax022_read_regs(&g_c123hax022_client, num, read_buf) != RT_EOK) {
        LOG_E("read touch fail\n");
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
                    c123hax022_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * C123HAX022_POINT_LEN;
            read_id = touch_id[read_index];
            pre_id[read_index] = read_id;
            input_x = ((read_buf[off_set + 4] & 0xf) << 8) | read_buf[off_set + 3];
            input_y = ((read_buf[off_set + 6] & 0xf) << 8) | read_buf[off_set + 5];
            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            c123hax022_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            c123hax022_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t c123hax022_control(struct rt_touch_device *touch, int cmd, void *data)
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

const struct rt_touch_ops c123hax022_touch_ops =
{
    .touch_readpoint = c123hax022_read_point,
    .touch_control = c123hax022_control,
};

struct rt_touch_info c123hax022_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_FT,
    5,
    (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
};

int c123hax022_hw_init(const char *name, struct rt_touch_config *cfg)
{
    rt_touch_t touch_device = RT_NULL;

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ENOMEM;
    }

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_INPUT);
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT);

    g_c123hax022_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (g_c123hax022_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)g_c123hax022_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    g_c123hax022_client.client_addr = C123HAX022_SALVE_ADDR;

    /* register touch device */
    touch_device->info = c123hax022_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &c123hax022_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device c123hax022 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device c123hax022 init success");
    return RT_EOK;
}

static int rt_hw_c123hax022_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rst_pin = rt_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = rt_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    c123hax022_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_c123hax022_port);
