/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-26        the first version
 */

#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "ili21xx.h"
#include "touch_common.h"

#define DBG_TAG "ILI21XX"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client ili21xx_client;

static rt_err_t ili21xx_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len) {
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

static int16_t pre_x[ILI21XX_MAX_TOUCH] = { 0 };
static int16_t pre_y[ILI21XX_MAX_TOUCH] = { 0 };
static rt_uint8_t s_tp_dowm[ILI21XX_MAX_TOUCH] = { 0 };
static struct rt_touch_data *read_data = RT_NULL;

static void ili21xx_touch_up(void *buf, int8_t id) {
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

static void ili21xx_touch_down(void *buf, int8_t id, int16_t x, int16_t y) {
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

rt_uint8_t ili21xx_get_touch_num(rt_uint8_t *buf, touch_point_t *touch_point) {
    rt_uint8_t touch_num = 0;
    for (int i = 0;i < ILI21XX_MAX_TOUCH; i++) {
        touch_point[i].id = buf[i * ILI21XX_POINT_LEN + 1] & 0x3f;;
        touch_point[i].status = buf[i * ILI21XX_POINT_LEN + 1] & 0x40;
        if (touch_point[i].status) {
            touch_num++;
        }
    }
    return touch_num;
}

static rt_size_t ili21xx_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num) {
    rt_uint8_t touch_num = 0;
    rt_uint8_t read_buf[ILI21XX_POINT_LEN * ILI21XX_MAX_TOUCH + 1] = { 0 };
    touch_point_t touch_point[ILI21XX_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    rt_uint8_t reg = ILI21XX_TOUCH_INFO;
    if (ili21xx_read_regs(&ili21xx_client, &reg, read_buf, sizeof(read_buf)) != RT_EOK) {
        LOG_E("read point failed\n");
        read_num = 0;
    }

    touch_num = ili21xx_get_touch_num(&read_buf, &touch_point);

    if (touch_num) {
        rt_uint8_t off_set;

        for (int i = 0; i < touch_num; i++) {
            off_set = i * ILI21XX_POINT_LEN;

            touch_point[i].touch_x = 0;
            touch_point[i].touch_y = 0;     //init point

            touch_point[i].touch_x = ((read_buf[off_set + 3]) << 8) | read_buf[off_set + 2];
            touch_point[i].touch_y = ((read_buf[off_set + 5]) << 8) | read_buf[off_set + 4];

            aic_touch_flip(&touch_point[i].touch_x, &touch_point[i].touch_y);
            aic_touch_rotate(&touch_point[i].touch_x, &touch_point[i].touch_y);
            aic_touch_scale(&touch_point[i].touch_x, &touch_point[i].touch_y);
            // if (!aic_touch_crop(&touch_point[i].touch_x, &touch_point[i].touch_y))
            //     continue;

            if (touch_point[i].status) {
                ili21xx_touch_down(buf, touch_point[i].id, touch_point[i].touch_x, touch_point[i].touch_y);
            } else {
                ili21xx_touch_up(buf, touch_point[i].id);
            }
        }
    } else {
        for (int i = 0; i < ILI21XX_MAX_TOUCH; i++) {
            ili21xx_touch_up(buf, i);
        }
    }

    return read_num;
}

static rt_err_t ili21xx_control(struct rt_touch_device *touch, int cmd, void *data) {
    struct rt_touch_info *info = RT_NULL;

    switch (cmd) {
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

const struct rt_touch_ops ili21xx_touch_ops =
{
    .touch_readpoint = ili21xx_read_point,
    .touch_control = ili21xx_control,
};

struct rt_touch_info ili21xx_info =
{
    .type = RT_TOUCH_TYPE_CAPACITANCE,
    .vendor = RT_TOUCH_VENDOR_UNKNOWN,
    .range_x = (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    .range_y = (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
    .point_num = 5,
};

int ili21xx_hw_init(const char *name, struct rt_touch_config *cfg) {
    rt_touch_t touch_device = RT_NULL;

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ENOMEM;
    }

    /* Reset TP IC */
    // rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    // rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    // rt_thread_delay(10);

    // rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    // rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    // rt_thread_delay(10);

    // rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    // rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    // rt_thread_delay(125);

    ili21xx_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (ili21xx_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)ili21xx_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    ili21xx_client.client_addr = ILI21XX_SALVE_ADDR;

    /* register touch device */

    touch_device->info = ili21xx_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &ili21xx_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device ili21xx init failed !!!");
        return -RT_ERROR;
    }

    rt_uint8_t data_buf[4] = { 0 };
    rt_uint8_t reg = ILI21XX_MAX_X_H_COORDINATE;
    rt_uint16_t max_x, max_y;
    if (ili21xx_read_regs(&ili21xx_client, &reg, data_buf, sizeof(data_buf)) != RT_EOK) {
        LOG_E("read point range failed!\n");
        return -1;
    } else {
        max_x = ((data_buf[1] << 8) | data_buf[0]);
        max_y = ((data_buf[3] << 8) | data_buf[2]);
        LOG_I("Touch panel max-x:%d max-y:%d", max_x, max_y);
    }

    LOG_I("touch device ili21xx init success");
    return RT_EOK;

}

static int ili21xx_gpio_cfg() {
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

static int rt_hw_ili21xx_port(void) {
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    ili21xx_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    ili21xx_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_ili21xx_port);
