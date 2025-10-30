/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-07-03        The first version
 * 2025-07-04        Need to change touch.c c`s falling edge trigger to low level trigger
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "jd9366.h"
#include "touch_common.h"

#define DBG_TAG     AIC_TOUCH_PANEL_NAME
#define DBG_LVL     DBG_INFO
#include <rtdbg.h>

static int16_t g_pre_x[JD9366TS_MAX_TOUCH] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static int16_t g_pre_y[JD9366TS_MAX_TOUCH] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static rt_uint8_t g_s_tp_dowm[JD9366TS_MAX_TOUCH] = {0};
static struct rt_touch_data *g_read_data = RT_NULL;
static struct rt_i2c_client g_jd9366ts_client = {0};

static rt_err_t jd9366ts_write_regs(struct rt_i2c_client *dev,
                                    rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        LOG_E("JD9366 write reg fail!\n");
        return -RT_ERROR;
    }
}

static rt_err_t jd9366ts_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
                                rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2] = {0};

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = JD9366TS_REGITER_LEN;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        LOG_E("JD9366 read point fail!\n");
        return -RT_ERROR;
    }
}

static void jd9366ts_touch_up(void *buf, int8_t id)
{
    g_read_data = (struct rt_touch_data *)buf;

    if (g_s_tp_dowm[id] == 1) {
        g_s_tp_dowm[id] = 0;
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

static void jd9366ts_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
{
    g_read_data = (struct rt_touch_data *)buf;

    if (g_s_tp_dowm[id] == 1) {
        g_read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        g_read_data[id].event = RT_TOUCH_EVENT_DOWN;
        g_s_tp_dowm[id] = 1;
    }

    g_read_data[id].timestamp = rt_touch_get_ts();
    g_read_data[id].x_coordinate = x;
    g_read_data[id].y_coordinate = y;
    g_read_data[id].track_id = id;

    g_pre_x[id] = x; /* save last point */
    g_pre_y[id] = y;
}

static rt_size_t jd9366ts_read_point(struct rt_touch_device *touch,
                                            void *buf, rt_size_t read_num)
{
    rt_uint8_t touch_num = 0;
    rt_uint8_t read_buf[JD9366TS_MAX_TOUCH * JD9366TS_INTO_LEN + 10] = { 0 };
    rt_uint8_t read_index;
    rt_uint8_t reg[6] = {0};
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    rt_uint8_t id[JD9366TS_MAX_TOUCH] = {0};
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[JD9366TS_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    reg[0] = 0xf2;
    reg[1] = 0xaa;
    reg[2] = 0x55;
    reg[3] = 0x0f;
    reg[4] = 0xf0;
    reg[5] = 0x68;
    if(jd9366ts_write_regs(&g_jd9366ts_client, reg, 6) != RT_EOK) {
        LOG_E("write point failed\n");
        read_num = 0;
        goto __exit;
    }

    reg[0] = 0xf3;
    reg[1] = 0x20;
    reg[2] = 0x02;
    reg[3] = 0x11;
    reg[4] = 0x20;
    reg[5] = 0x03;
    if (jd9366ts_read_regs(&g_jd9366ts_client, reg, read_buf, sizeof(read_buf)) != RT_EOK) {
        LOG_E("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    touch_num = read_buf[0] & 0xf;
    if (touch_num > JD9366TS_MAX_TOUCH) {
        read_num = 0;
        goto __exit;
    }

    for (int8_t i = 0; i < touch_num; i++)
        id[i] = i;

    if (pre_touch > touch_num) /* point up */
    {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            rt_uint8_t j;

            for (j = 0; j < touch_num; j++) {
                read_id = id[read_index];

                if (pre_id[read_index] == read_id) /* this id is not free */
                    break;

                if (j >= touch_num - 1) {
                    rt_uint8_t up_id;
                    up_id = pre_id[read_index];
                    jd9366ts_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) /* point down */
    {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * JD9366TS_INTO_LEN;

            read_id = id[read_index];
            pre_id[read_index] = read_id;

            input_x = (read_buf[off_set + 3] << 8) | read_buf[off_set + 4];
            input_y = (read_buf[off_set + 5] << 8) | read_buf[off_set + 6];
            if (input_x == 65535 || input_y == 65535 || input_x == -1 || input_y == -1)
                continue;

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            jd9366ts_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            jd9366ts_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t jd9366ts_control(struct rt_touch_device *touch, int cmd, void *data)
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
        LOG_E("This cmd:%d is not support\n", cmd);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static struct rt_touch_ops jd9366ts_touch_ops = {
    .touch_readpoint = jd9366ts_read_point,
    .touch_control = jd9366ts_control,
};

struct rt_touch_info jd9366ts_info =
{
    RT_TOUCH_TYPE_CAPACITANCE,
    RT_TOUCH_VENDOR_UNKNOWN,
    10,
    (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
};

static int jd9366ts_hw_init(const char *name, struct rt_touch_config *cfg)
{
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    /* RESET IC */
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(5);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_delay(50);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(100);

    g_jd9366ts_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (g_jd9366ts_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)g_jd9366ts_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    g_jd9366ts_client.client_addr = JD9366TS_SALVE_ADDR;

    touch_device->info = jd9366ts_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &jd9366ts_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device jd9366ts init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device jd9366ts init success");
    return RT_EOK;
}

static int rt_hw_jd9366ts_port(void)
{
    struct rt_touch_config cfg = {0};
    rt_uint8_t rst_pin = 0;

    rst_pin = rt_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = rt_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT_PULLUP;
    cfg.user_data = &rst_pin;

    jd9366ts_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_jd9366ts_port);
