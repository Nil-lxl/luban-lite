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
#include "hy4635.h"

#define DBG_TAG "hy4635"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client hy4635_client;
static rt_err_t hy4635_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data,
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

static rt_err_t hy4635_read_regs(struct rt_i2c_client *dev, rt_uint8_t *reg,
    rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = 1;  //8bit reg

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

// static rt_err_t hy4635_get_product_id(struct rt_i2c_client *dev,
//                                      rt_uint8_t *data, rt_uint8_t len)
// {
//     rt_uint8_t reg[2];

//     reg[0] = (rt_uint8_t)(HY4635_PRODUCT_ID >> 8);
//     reg[1] = (rt_uint8_t)(HY4635_PRODUCT_ID & 0xff);

//     if (hy4635_read_regs(dev, reg, data, len) != RT_EOK) {
//         LOG_E("read id failed");
//         return -RT_ERROR;
//     }
//     return RT_EOK;
// }

static rt_err_t hy4635_get_info(struct rt_i2c_client *dev, struct rt_touch_device *touch,
    struct rt_touch_info *info) {
    rt_uint8_t reg[2];
    rt_uint8_t out_info[7];
    rt_uint8_t out_len = 7;
    uint16_t angle = 0;

    // reg[0] = (rt_uint8_t)(HY4635_CONFIG_REG >> 8);
    // reg[1] = (rt_uint8_t)(HY4635_CONFIG_REG & 0xFF);

    // if (hy4635_read_regs(dev, reg, out_info, out_len) != RT_EOK) {
    //     LOG_E("read info failed");
    //     return -RT_ERROR;
    // }

    // info->range_x = (out_info[2] << 8) | out_info[1];
    // info->range_y = (out_info[4] << 8) | out_info[3];

    // rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_GET_DYNAMIC_ROTATE, &angle);
    // if (angle == 90 || angle == 270) {
    //     info->range_x = (rt_int16_t)AIC_TOUCH_Y_COORDINATE_RANGE;
    //     info->range_y = (rt_int16_t)AIC_TOUCH_X_COORDINATE_RANGE;
    // } else {
    info->range_x = (rt_int16_t)AIC_TOUCH_X_COORDINATE_RANGE;
    info->range_y = (rt_int16_t)AIC_TOUCH_Y_COORDINATE_RANGE;
// }

    info->point_num = out_info[5] & 0x0f;
    if (info->point_num > HY4635_MAX_TOUCH) {
        info->point_num = HY4635_MAX_TOUCH;
        rt_kprintf("Warning,tp support more than 5 points, limited to 5 points\n");
    }

    return RT_EOK;
}
#if 0
static rt_err_t hy4635_soft_reset(struct rt_i2c_client *dev) {
    rt_uint8_t buf[3];

    buf[0] = (rt_uint8_t)(HY4635_COMMAND_REG >> 8);
    buf[1] = (rt_uint8_t)(HY4635_COMMAND_REG & 0xFF);
    buf[2] = 0x02;

    if (hy4635_write_reg(dev, buf, 3) != RT_EOK) {
        LOG_E("soft reset failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t hy4635_soft_reset_finish(struct rt_i2c_client *dev) {
    rt_uint8_t buf[3];

    buf[0] = (rt_uint8_t)(HY4635_COMMAND_REG >> 8);
    buf[1] = (rt_uint8_t)(HY4635_COMMAND_REG & 0xFF);
    buf[2] = 0x00;

    if (hy4635_write_reg(dev, buf, 3) != RT_EOK) {
        LOG_E("soft reset failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t hy4635_set_irq_pin_int(struct rt_i2c_client *dev) {
    rt_uint8_t buf[3];

    buf[0] = (rt_uint8_t)(HY4635_MOD_SWT_REG >> 8);
    buf[1] = (rt_uint8_t)(HY4635_MOD_SWT_REG & 0xFF);
    buf[2] = 0x01;

    if (hy4635_write_reg(dev, buf, 3) != RT_EOK) {
        LOG_E("set up failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}
#endif

static int16_t pre_x[HY4635_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[HY4635_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
// static int16_t pre_w[HY4635_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t s_tp_dowm[HY4635_MAX_TOUCH] = { 0 };
static struct rt_touch_data *read_data = RT_NULL;

static void hy4635_touch_up(void *buf, int8_t id) {
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        s_tp_dowm[id] = 0;
        read_data[id].event = RT_TOUCH_EVENT_UP;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_NONE;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    // read_data[id].width = pre_w[id];
    read_data[id].x_coordinate = pre_x[id];
    read_data[id].y_coordinate = pre_y[id];
    read_data[id].track_id = id;

    pre_x[id] = -1; /* last point is none */
    pre_y[id] = -1;
    // pre_w[id] = -1;
}

static void hy4635_touch_down(void *buf, int8_t id, int16_t x, int16_t y) {
    read_data = (struct rt_touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_DOWN;
        s_tp_dowm[id] = 1;
    }

    read_data[id].timestamp = rt_touch_get_ts();
    // read_data[id].width = w;
    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    // pre_w[id] = w;
}

static rt_size_t hy4635_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num) {
    rt_uint8_t point_status = 0;
    rt_uint8_t point_id = 0;
    rt_uint8_t touch_num = 0;
    rt_uint8_t write_buf[2];
    // rt_uint8_t cmd[2];
    rt_uint8_t reg;
    // rt_uint8_t read_buf[8 * HY4635_MAX_TOUCH] = { 0 };
    rt_uint8_t read_buf[6 * HY4635_MAX_TOUCH + 3] = { 0 };

    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    // int16_t input_w = 0;

    static rt_uint8_t prev_touch = 0;
    static int8_t pre_id[HY4635_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    reg = HY4635_WORK_MODE;
    write_buf[0] = 0x00;
    write_buf[1] = 0x00;
    hy4635_write_reg(&hy4635_client, write_buf, 2);

    // if (hy4635_read_regs(&hy4635_client, &reg, read_buf, sizeof(read_buf)) != RT_EOK) {
    //     LOG_E("hy4635 read work mode failed");
    //     read_num = 0;
    // }
    reg = 0x02;
    hy4635_read_regs(&hy4635_client, &reg, &point_status, 1);
    touch_num = point_status;
    LOG_I("TOUCH Num:%d", touch_num);

    // touch_num = read_buf[2] & 0x0f;
    // if (touch_num >= 2) {
    //     touch_num = 2;
    // }

    reg = 0x03;
    hy4635_read_regs(&hy4635_client, &reg, read_buf, sizeof(read_buf));
    int point_event = read_buf[0] >> 6;
    int read_y_high = (read_buf[0] & 0x6f) << 8;

    reg = 0x04;
    hy4635_read_regs(&hy4635_client, &reg, read_buf, sizeof(read_buf));
    int read_y_low = read_buf[0];
    int read_y = read_y_high | read_y_low;

    reg = 0x05;
    hy4635_read_regs(&hy4635_client, &reg, read_buf, sizeof(read_buf));
    point_id = read_buf[0] >> 4;
    int read_x_high = (read_buf[0] & 0x4f) << 8;

    reg = 0x06;
    hy4635_read_regs(&hy4635_client, &reg, read_buf, sizeof(read_buf));
    int read_x_low = read_buf[0];
    int read_x = read_x_high | read_x_low;


    if (prev_touch > touch_num) /* point up */
    {
        for (int i = 0; i < prev_touch; i++) {

            for (int j = 0; j < touch_num; j++) /* this time touch num */
            {
                read_id = point_id;

                if (pre_id[i] == read_id) /* this id is not free */
                    break;

                if (j >= touch_num - 1) {
                    rt_uint8_t up_id = pre_id[i];
                    hy4635_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) /* point down */
    {
        rt_uint8_t off_set;
        // rt_uint8_t dynamic_flag = 0;
        // rt_uint8_t osd_flag = 0;

        // rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_GET_DYNAMIC_FLAG, &dynamic_flag);
        // rt_device_control((rt_device_t)touch, RT_TOUCH_CTRL_GET_OSD_FLAG, &osd_flag);

        for (int i = 0; i < touch_num; i++) {
            off_set = i * 8;
            read_id = (read_buf[off_set + 5] & 0x0f) >> 4;
            pre_id[i] = read_id;
            input_x = read_x;
            input_y = read_y;
            // input_x = read_buf[off_set + 1] | (read_buf[off_set + 2] << 8); /* x */
            // input_y = read_buf[off_set + 3] | (read_buf[off_set + 4] << 8); /* y */
            // input_w = read_buf[off_set + 5] | (read_buf[off_set + 6] << 8); /* size */

            // if (!dynamic_flag) {    // static rotate(osd || video)
            //     if (!osd_flag) {    // if osd flag enabled we should do nothing
            //         aic_touch_flip(&input_x, &input_y);
            //         aic_touch_rotate(&input_x, &input_y);
            //         aic_touch_scale(&input_x, &input_y);
            //         if (!aic_touch_crop(&input_x, &input_y))
            //             continue;
            //     }
            // } else {    // dynamic rotata (osd || video)
            //     if (!osd_flag)  // if osd flag enabled we should do nothing
            //         aic_touch_dynamic_rotate(touch, &input_x, &input_y);
            // }

            hy4635_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (prev_touch) {
        for (int i = 0; i < prev_touch; i++) {
            hy4635_touch_up(buf, pre_id[i]);
        }
    }

    prev_touch = touch_num;

    return read_num;
}

static rt_err_t hy4635_control(struct rt_touch_device *touch, int cmd, void *data) {
    // if (cmd == RT_TOUCH_CTRL_GET_ID) {
    //     return hy4635_get_product_id(&hy4635_client, arg, 6);
    // }
    struct rt_touch_info *info = RT_NULL;
    if (cmd == RT_TOUCH_CTRL_GET_INFO) {
        info = (struct rt_touch_info *)data;
        if (info == RT_NULL)
            return -RT_EINVAL;

        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;
        // break;
        return hy4635_get_info(&hy4635_client, touch, data);
    }
    // rt_uint8_t reg = 0x02;
    // rt_uint8_t point_status = 0;
    // hy4635_read_regs(&hy4635_client, &reg, &point_status, 1);
    // int touch_num = point_status;
    // LOG_I("TOUCH Num:%d", touch_num);

    return RT_EOK;
}

static struct rt_touch_ops hy4635_touch_ops = {
    .touch_readpoint = hy4635_read_point,
    .touch_control = hy4635_control,
};

static int rt_hw_hy4635_init(const char *name, struct rt_touch_config *cfg) {
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    /* hw init*/

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_delay(10);


    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);

    rt_thread_delay(2);

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(10);

    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_delay(1000);

    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->irq_pin.pin, PIN_LOW);


    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT);

    hy4635_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (hy4635_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)hy4635_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    hy4635_client.client_addr = HY4635_ADDRESS_HIGH;

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_GT;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &hy4635_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device hy4635 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device hy4635 init success");
    return RT_EOK;
}

static int rt_hy4635_gpio_cfg() {
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

static int rt_hw_hy4635_port(void) {
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rt_hy4635_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;
#ifdef AIC_PM_DEMO_TOUCH_WAKEUP
    rt_pm_set_pin_wakeup_source(cfg.irq_pin.pin);
#endif

    rt_hw_hy4635_init("hy4635", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_hy4635_port);
