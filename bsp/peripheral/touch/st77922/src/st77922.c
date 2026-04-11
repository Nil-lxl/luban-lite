/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-07-31        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "st77922.h"
#include "touch_common.h"

#define DBG_TAG     AIC_TOUCH_PANEL_NAME
#define DBG_LVL     DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client st77922_client;

/*
 * may be use this func to do something in the future
 */
#if 0
static rt_err_t st77922_write_reg(struct rt_i2c_client *dev,
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
        return -RT_ERROR;
    }
}
#endif

static rt_err_t st77922_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
                                rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = ST77922_REGITER_LEN;

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

static rt_err_t st77922_get_info(struct rt_i2c_client *dev,
                               struct rt_touch_info *info)
{
    rt_uint8_t reg[2];
    rt_uint8_t info_buf[5];

    if (info == RT_NULL)
        return -RT_EINVAL;

    reg[0] = (rt_uint8_t)(ST77922_MAX_X_COORD >> 8);
    reg[1] = (rt_uint8_t)(ST77922_MAX_X_COORD & 0xFF);

        if (st77922_read_regs(dev, reg, info_buf, sizeof(info_buf)) != RT_EOK) {
        rt_kprintf("read info failed!\n");
        return -RT_ERROR;
    }

    info->range_x = ((info_buf[0] & 0x3f) << 8 | info_buf[1]);
    info->range_y = ((info_buf[2] & 0x3f) << 8 | info_buf[3]);
    info->point_num = info_buf[4];
    info->type = RT_TOUCH_TYPE_CAPACITANCE;
    info->vendor = RT_TOUCH_VENDOR_UNKNOWN;

    return RT_EOK;
}

static int16_t pre_x[ST77922_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[ST77922_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t s_tp_dowm[ST77922_MAX_TOUCH] = {0};
static struct rt_touch_data *g_read_data = RT_NULL;

static void st77922_touch_up(void *buf, int8_t id)
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

static void st77922_touch_down(void *buf, int8_t id, int16_t x, int16_t y)
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

static rt_size_t st77922_read_point(struct rt_touch_device *touch,
                                  void *buf, rt_size_t read_num)
{
    rt_uint8_t point_status = 0;
    rt_uint8_t touch_num = 0;
    rt_uint8_t cmd[2], i, num_valid;
    rt_uint8_t read_buf[7 * ST77922_MAX_TOUCH + 5] = { 0 };
    rt_uint8_t read_index;
    rt_uint8_t dev_status;
    rt_uint8_t error_code;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    rt_uint8_t id[5] = {0};
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[ST77922_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    /* point status register */
    cmd[0] = (rt_uint8_t)((ST77922_DEV_STATUS >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST77922_DEV_STATUS & 0xFF);

    if (st77922_read_regs(&st77922_client, cmd, &point_status, 1) != RT_EOK) {
        rt_kprintf("read point status fail\n");
        read_num = 0;
        goto __exit;
    }

    dev_status = point_status & 0x0f;
    if (dev_status != 0) {
        rt_kprintf("tp status is error, status mode:%d\n", dev_status);
        read_num = 0;
        goto __exit;
    }

    error_code = (point_status & 0xf0) >> 4;
    if (error_code != 0) {
        rt_kprintf("tp status is error, error code:%d\n", error_code);
        read_num = 0;
        goto __exit;
    }

    cmd[0] = (rt_uint8_t)((ST77922_TOUCH_INFO >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST77922_TOUCH_INFO & 0xFF);
    /* read point num is touch_num */
    if (st77922_read_regs(&st77922_client, cmd, read_buf, sizeof(read_buf)) != RT_EOK) {
        rt_kprintf("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    for (i = 0; i < ST77922_MAX_TOUCH; i++) {
        num_valid = ((read_buf[7 * i + 4] & 0x80) != 0) ? 1 : 0;
        touch_num += num_valid;
    }

    if (touch_num > ST77922_MAX_TOUCH) {
        touch_num = 0;
        goto __exit;
    }

    for (int8_t i = 0; i < touch_num; i++) {
        id[i] = i;
    }

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
                    st77922_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) /* point down */
    {
        rt_uint8_t off_set;

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * 7;

            read_id = id[read_index];
            pre_id[read_index] = read_id;

            input_x = ((read_buf[off_set + 4] & 0x3f) << 8) | read_buf[off_set + 5];
            input_y = ((read_buf[off_set + 6] & 0x3f) << 8) | read_buf[off_set + 7];

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            st77922_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            st77922_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t st77922_control(struct rt_touch_device *touch, int cmd, void *arg)
{
    if (cmd == RT_TOUCH_CTRL_GET_INFO) {
        return st77922_get_info(&st77922_client, arg);
    }

    return RT_EOK;
}

static struct rt_touch_ops st77922_touch_ops = {
    .touch_readpoint = st77922_read_point,
    .touch_control = st77922_control,
};

static int st77922_hw_init(const char *name, struct rt_touch_config *cfg)
{
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    st77922_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (st77922_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)st77922_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    st77922_client.client_addr = ST77922_SALVE_ADDR;

    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &st77922_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device st77922 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device st77922 init success");
    return RT_EOK;
}

static int st77922_gpio_cfg()
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

static int rt_hw_st77922_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    st77922_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    st77922_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_st77922_port);
