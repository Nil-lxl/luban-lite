/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 * 2023-04-30     Geo          modified for ArtInChip
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "touch_common.h"
#include "gt9271.h"

#define DBG_TAG "gt9271"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client gt9271_client;

static rt_err_t gt9271_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data,
    rt_uint8_t len) {
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

static rt_err_t gt9271_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
    rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = GT9271_REGITER_LEN;

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

static rt_err_t gt9271_get_product_id(struct rt_i2c_client *dev,
    rt_uint8_t *data, rt_uint8_t len) {
    rt_uint8_t reg[2];

    reg[0] = (rt_uint8_t)(GT9271_PRODUCT_ID >> 8);
    reg[1] = (rt_uint8_t)(GT9271_PRODUCT_ID & 0xff);

    if (gt9271_read_regs(dev, reg, data, len) != RT_EOK) {
        LOG_E("read id failed");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t gt9271_get_info(struct rt_i2c_client *dev, struct rt_touch_device *touch,
    struct rt_touch_info *info) {
    rt_uint8_t reg[2];
    rt_uint8_t out_info[7];
    rt_uint8_t out_len = 7;
    uint16_t angle = 0;

    reg[0] = (rt_uint8_t)(GT9271_CONFIG_REG >> 8);
    reg[1] = (rt_uint8_t)(GT9271_CONFIG_REG & 0xFF);

    if (gt9271_read_regs(dev, reg, out_info, out_len) != RT_EOK) {
        LOG_E("read info failed");
        return -RT_ERROR;
    }

    info->range_x = (out_info[2] << 8) | out_info[1];
    info->range_y = (out_info[4] << 8) | out_info[3];
    info->point_num = out_info[5] & 0x0f;
    if (info->point_num > GT9271_MAX_TOUCH)
        info->point_num = GT9271_MAX_TOUCH;
    else if (info->point_num <= 0)
        info->point_num = GT9271_MAX_TOUCH;

    // rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_GET_DYNAMIC_ROTATE, &angle);
    // if (angle == 90 || angle == 270) {
    //     info->range_x = (rt_int16_t)AIC_TOUCH_REPORT_Y_COORDINATE;
    //     info->range_y = (rt_int16_t)AIC_TOUCH_REPORT_X_COORDINATE;
    // } else {
    //     info->range_x = (rt_int16_t)AIC_TOUCH_REPORT_X_COORDINATE;
    //     info->range_y = (rt_int16_t)AIC_TOUCH_REPORT_Y_COORDINATE;
    // }

    return RT_EOK;
}

static rt_err_t gt9271_power_down(struct rt_touch_device *touch) {
    rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    /* Pull down irq pin */
    rt_pin_mode(touch->config.irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(touch->config.irq_pin.pin, PIN_LOW);
    rt_thread_mdelay(10);

    return RT_EOK;
}

static rt_err_t gt9271_power_up(struct rt_touch_device *touch) {
    /* Pull up irq pin */
    rt_pin_mode(touch->config.irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(touch->config.irq_pin.pin, PIN_HIGH);
    rt_thread_mdelay(10);
    rt_pin_mode(touch->config.irq_pin.pin, PIN_MODE_INPUT);

    if (touch->parent.open_flag & RT_DEVICE_OFLAG_OPEN)
        rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

    return RT_EOK;
}

static int16_t pre_x[GT9271_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[GT9271_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_w[GT9271_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t s_tp_dowm[GT9271_MAX_TOUCH];
static struct rt_touch_data *read_data;

static void gt9271_touch_up(void *buf, int8_t id) {
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        s_tp_dowm[id] = 0;
        read_data[id].event = RT_TOUCH_EVENT_UP;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_NONE;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    read_data[id].width = pre_w[id];
    read_data[id].x_coordinate = pre_x[id];
    read_data[id].y_coordinate = pre_y[id];
    read_data[id].track_id = id;

    pre_x[id] = -1; /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void gt9271_touch_down(void *buf, int8_t id, int16_t x, int16_t y,
    int16_t w) {
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_DOWN;
        s_tp_dowm[id] = 1;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    read_data[id].width = w;
    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    pre_w[id] = w;
}

static rt_size_t gt9271_read_point(struct rt_touch_device *touch, void *buf,
    rt_size_t read_num) {
    rt_uint8_t point_status = 0;
    rt_uint8_t touch_num = 0;
    rt_uint8_t write_buf[3];
    rt_uint8_t cmd[2];
    rt_uint8_t read_buf[8 * GT9271_MAX_TOUCH] = { 0 };
    rt_uint8_t read_index;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    int16_t input_w = 0;

    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[GT9271_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    /* point status register */
    cmd[0] = (rt_uint8_t)((GT9271_READ_STATUS >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(GT9271_READ_STATUS & 0xFF);

    if (gt9271_read_regs(&gt9271_client, cmd, &point_status, 1) != RT_EOK) {
        LOG_D("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if (point_status == 0) /* no data */
    {
        read_num = 0;
        goto exit_;
    }

    if ((point_status & 0x80) == 0) /* data is not ready */
    {
        read_num = 0;
        goto exit_;
    }

    touch_num = point_status & 0x0f; /* get point num */

    if (touch_num > GT9271_MAX_TOUCH) /* point num is not correct */
    {
        read_num = 0;
        goto exit_;
    }

    cmd[0] = (rt_uint8_t)((GT9271_POINT1_REG >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(GT9271_POINT1_REG & 0xFF);

    /* read point num is touch_num */
    if (gt9271_read_regs(&gt9271_client, cmd, read_buf,
        read_num * GT9271_POINT_INFO_NUM) != RT_EOK) {
        LOG_D("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if (pre_touch > touch_num) /* point up */
    {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            rt_uint8_t j;

            for (j = 0; j < touch_num; j++) /* this time touch num */
            {
                read_id = read_buf[j * 8] & 0x0F;

                if (pre_id[read_index] == read_id) /* this id is not free */
                    break;

                if (j >= touch_num - 1) {
                    rt_uint8_t up_id;
                    up_id = pre_id[read_index];
                    gt9271_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) /* point down */
    {
        rt_uint8_t off_set;
        rt_uint8_t dynamic_flag = 0;
        rt_uint8_t osd_flag = 0;

        rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_GET_DYNAMIC_FLAG, &dynamic_flag);
        rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_GET_OSD_FLAG, &osd_flag);

        for (read_index = 0; read_index < touch_num; read_index++) {
            off_set = read_index * 8;
            read_id = read_buf[off_set] & 0x0f;
            pre_id[read_index] = read_id;
            input_x =
                read_buf[off_set + 1] | (read_buf[off_set + 2] << 8); /* x */
            input_y =
                read_buf[off_set + 3] | (read_buf[off_set + 4] << 8); /* y */
            input_w =
                read_buf[off_set + 5] | (read_buf[off_set + 6] << 8); /* size */

            // Step 1: Perform coordinate transformations only when osd mode is disabled
            if (!osd_flag) {
                aic_touch_flip(&input_x, &input_y);
                aic_touch_rotate(&input_x, &input_y);
                aic_touch_scale(&input_x, &input_y);

                if (dynamic_flag) {
                    // Apply dynamic rotation for dynamic mode
                    aic_touch_dynamic_rotate(touch, &input_x, &input_y);
                } else {
                    // Apply static cropping; skip further processing if out of bounds
                    if (!aic_touch_crop(&input_x, &input_y))
                        continue;
                }
            }
            // Step 2: Apply dynamic cropping check for dynamic mode
            if (dynamic_flag) {
                // Skip further processing if out of bounds
                if (aic_touch_dynamic_crop(touch, &input_x, &input_y) == RT_EINVAL)
                    continue;
            }

            gt9271_touch_down(buf, read_id, input_x, input_y, input_w);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            gt9271_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

exit_:
    write_buf[0] = (rt_uint8_t)((GT9271_READ_STATUS >> 8) & 0xFF);
    write_buf[1] = (rt_uint8_t)(GT9271_READ_STATUS & 0xFF);
    write_buf[2] = 0x00;
    gt9271_write_reg(&gt9271_client, write_buf, 3);

    return read_num;
}


static rt_err_t gt9271_control(struct rt_touch_device *touch, int cmd, void *arg) {
    if (cmd == RT_TOUCH_CTRL_GET_ID)
        return gt9271_get_product_id(&gt9271_client, arg, 6);

    if (cmd == RT_TOUCH_CTRL_GET_INFO)
        return gt9271_get_info(&gt9271_client, touch, arg);

    if (cmd == RT_TOUCH_CTRL_POWER_OFF)
        return gt9271_power_down(touch);

    if (cmd == RT_TOUCH_CTRL_POWER_ON)
        return gt9271_power_up(touch);

    return RT_EOK;
}

static struct rt_touch_ops gt9271_touch_ops = {
    .touch_readpoint = gt9271_read_point,
    .touch_control = gt9271_control,
};
static void gt9271_gpio_init(struct rt_touch_config *cfg) {
    // rst output 0
    rt_pin_mode(cfg->rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->rst_pin, PIN_LOW);
    // int output 0
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);
    rt_thread_mdelay(1);

    // rst output 1
    rt_pin_write(cfg->rst_pin, PIN_HIGH);
    rt_thread_mdelay(10);
    // int output 0
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);

    // irq input
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT);
}

static int rt_hw_gt9271_init(const char *name, struct rt_touch_config *cfg) {
    struct rt_touch_device *touch_device = RT_NULL;
    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    gt9271_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (gt9271_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)gt9271_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    // gt9271_client.client_addr = GT9271_ADDRESS_HIGH;
    gt9271_client.client_addr = GT9271_ADDRESS_LOW;

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_GT;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &gt9271_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device gt9271 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device gt9271 init success");
    return RT_EOK;
}

static int rt_hw_gt9271_port(void) {
    struct rt_touch_config cfg = {
        .dev_name = AIC_TOUCH_PANEL_I2C_CHAN,
        .irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN),
        .irq_pin.mode = PIN_MODE_INPUT,
        .rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN),
    };
    gt9271_gpio_init(&cfg);

    rt_hw_gt9271_init("gt9271", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_gt9271_port);
