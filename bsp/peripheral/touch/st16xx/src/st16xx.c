/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2024-04-19        the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "st16xx.h"
#include "touch_common.h"

#define DBG_TAG "st16xx"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_i2c_client *st16xx_client;

/**
 * @brief st16xx_write_regs
 *
 * @param reg register to be written
 * @param buf buf to be written
 * @param len buf len
 * @return true
 * @return false
 */
static rt_err_t st16xx_write_regs(struct rt_i2c_client *dev, rt_uint8_t write_len,
                                    rt_uint8_t *write_data)
{
    struct rt_i2c_msg msgs;

    msgs.addr  = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = write_data;
    msgs.len   = write_len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

/**
 * @brief st16xx_read_regs
 *
 * @param reg register to be read
 * @param buf read data
 * @param len read len
 * @return true
 * @return false
 */
static rt_err_t st16xx_read_regs(struct rt_i2c_client *dev, rt_uint8_t *cmd_buf, rt_uint8_t cmd_len,
                                    rt_uint8_t read_len, rt_uint8_t *read_buf)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr  = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = cmd_buf;
    msgs[0].len   = cmd_len;

    msgs[1].addr  = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = read_buf;
    msgs[1].len   = read_len;

    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

/**
 * This function read the product id
 *
 * @param dev the pointer of device driver structure
 * @param reg the register
 * @param read data len
 * @param read data pointer
 *
 * @return the read status, RT_EOK reprensents  read the value of the register successfully.
 */
static rt_err_t st16xx_get_product_id(struct rt_i2c_client *dev,
                                        rt_uint8_t read_len, rt_uint8_t *read_data)
{
    return RT_EOK;
}

static rt_err_t st16xx_get_info(struct rt_i2c_client *dev, struct rt_touch_info *info)
{
    rt_uint8_t read_buf[4] = {0};
    rt_uint8_t cmd_buf[2];

    cmd_buf[0] = ST16XX_RESOLUTION_REG;
    /* read point num is read_num */
    if (st16xx_read_regs(st16xx_client, cmd_buf, 1, 3, read_buf) != RT_EOK) {
        LOG_I("read resolution info failed\n");
        return -RT_ERROR;
    }
    info->range_x = (read_buf[0] & 0x70) << 4 | read_buf[1];	/* x */
    info->range_y = (read_buf[0] & 0x07) << 8 | read_buf[2];	/* y */

    cmd_buf[0] = ST16XX_CONTACTS_NUM_REG;
    /* read point num is read_num */
    if (st16xx_read_regs(st16xx_client, cmd_buf, 1, 1, read_buf) != RT_EOK) {
        LOG_I("read contacts info failed\n");
        return -RT_ERROR;
    }
    info->point_num = read_buf[0];
    info->type      = RT_TOUCH_TYPE_CAPACITANCE;
    info->vendor    = RT_TOUCH_VENDOR_UNKNOWN;

    return RT_EOK;

}

static rt_err_t st16xx_soft_reset(struct rt_i2c_client *dev)
{
    rt_uint8_t buf[2];

    buf[0] = (rt_uint8_t)(ST16XX_RESET_REG);
    /* reset ic and disable multi-touch*/
    buf[1] = 0x41;

    if (st16xx_write_regs(dev, 2, buf) != RT_EOK) {
        LOG_D("soft reset st1633i failed\n");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t st16xx_control(struct rt_touch_device *device, int cmd, void *data)
{
    if (cmd == RT_TOUCH_CTRL_GET_ID) {
        return st16xx_get_product_id(st16xx_client, 6, data);
    }

    if (cmd == RT_TOUCH_CTRL_GET_INFO) {
        return st16xx_get_info(st16xx_client, data);
    }

    return RT_EOK;
}

static int16_t pre_x[ST16XX_MAX_TOUCH] = {-1, -1, -1, -1, -1};
static int16_t pre_y[ST16XX_MAX_TOUCH] = {-1, -1, -1, -1, -1};
static int16_t pre_w[ST16XX_MAX_TOUCH] = {-1, -1, -1, -1, -1};
static rt_uint8_t s_tp_dowm[ST16XX_MAX_TOUCH];
static struct rt_touch_data *read_data;

static void st16xx_touch_up(void *buf, int8_t id)
{
    read_data = (struct rt_touch_data *)buf;

    if(s_tp_dowm[id] == 1) {
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

    pre_x[id] = -1;  /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void st16xx_touch_down(void *buf, int8_t id, int16_t x, int16_t y, int16_t w)
{
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

static rt_size_t st16xx_read_points(struct rt_touch_device *touch, void *buf, rt_size_t read_num)
{
    rt_uint8_t point_status = 0;
    rt_uint8_t touch_num = 0;
    rt_uint8_t cmd[2];
    rt_uint8_t read_buf[(ST16XX_POINT_INFO_NUM * ST16XX_MAX_TOUCH) + 2] = {0};
    rt_uint8_t read_index;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    int16_t input_w = 0;

    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[ST16XX_MAX_TOUCH] = {0};
    /* point status register */
    cmd[0] = (ST16XX_STATUS_REG);
    if (st16xx_read_regs(st16xx_client, cmd, 1, 1, &point_status) != RT_EOK) {
        LOG_I("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if ((point_status & 0x0f) == 0x02) {         /* error code */
        read_num = 0;
        LOG_I("Touch error: %d\n",point_status & 0x0f);
        goto exit_;
    }

    cmd[0] = ST16XX_CONTACTS_NUM_REG;
    st16xx_read_regs(st16xx_client, cmd, 1, 1, &touch_num);
    if (touch_num > ST16XX_MAX_TOUCH) { /* point num is not correct */
        read_num = 0;
        LOG_I("Set contact num: %d,Get contact num: %d\n",ST16XX_MAX_TOUCH,point_status);
        goto exit_;
    }

    cmd[0] = (ST16XX_KEY_REG);
    /* read point num is read_num */
    if (st16xx_read_regs(st16xx_client, cmd, 1,
            read_num * ST16XX_POINT_INFO_NUM, read_buf) != RT_EOK) {
        LOG_I("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if(touch_num) {                                                 /* point down */
        rt_uint8_t off_set;
        for (read_index = 0; read_index < 1; read_index++) {
            off_set = read_index * 4;
            read_id = read_buf[off_set];
            pre_id[read_index] = read_id;
            if (read_buf[off_set + 1] & 0x80) {             //touch data valid detect
                input_x = (read_buf[off_set + 1] & 0x70) << 4 | read_buf[off_set + 2];	/* x */
                input_y = (read_buf[off_set + 1] & 0x07) << 8 | read_buf[off_set + 3];	/* y */
                if (input_y)
                    input_w = 4;	/* size */

                aic_touch_flip(&input_x, &input_y);
                aic_touch_rotate(&input_x, &input_y);
                aic_touch_scale(&input_x, &input_y);
                if (!aic_touch_crop(&input_x, &input_y))
                    continue;

                st16xx_touch_down(buf, read_id, input_x, input_y, input_w);
            }
        }
    }
    pre_touch = (read_buf[1] & 0x80);
    for (read_index = 0; read_index < 1; read_index++) {
        if (!pre_touch) {
            st16xx_touch_up(buf, pre_id[read_index]);
        }
    }

exit_:
    return read_num;
}

static struct rt_touch_ops touch_ops = {
    .touch_readpoint = st16xx_read_points,
    .touch_control = st16xx_control,
};

static int rt_st16xx_gpio_cfg()
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

/**
 * @brief tp init
 *
 */
int rt_hw_st16xx_init(const char *name, struct rt_touch_config *cfg)
{
    rt_touch_t touch_device = RT_NULL;
    rt_uint8_t cmd_buf[2];

    touch_device = (rt_touch_t)rt_calloc(1, sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        return -RT_ERROR;
    }

    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(*(rt_uint8_t *)cfg->user_data, PIN_MODE_OUTPUT);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(2);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_LOW);
    rt_thread_mdelay(5);
    rt_pin_write(*(rt_uint8_t *)cfg->user_data, PIN_HIGH);
    rt_thread_mdelay(50);

    /* interface bus */
    st16xx_client = (struct rt_i2c_client *)rt_calloc(1, sizeof(struct rt_i2c_client));
    st16xx_client->bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (st16xx_client->bus == RT_NULL) {
        LOG_I("Can't find device\n");
        return -RT_ERROR;
    }
    if (rt_device_open((rt_device_t)st16xx_client->bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_I("open device failed\n");
        return -RT_ERROR;
    }

    st16xx_client->client_addr = ST16XX_ADDR;
    st16xx_soft_reset(st16xx_client);                                   //Reset IC
    //Disable idle mode
    cmd_buf[0] = ST16XX_TIMOUT_REG;
    cmd_buf[1] = 0xff;
    if (st16xx_write_regs(st16xx_client, 2, cmd_buf) != RT_EOK) {
        LOG_I("st1633i set idle mode failed\n");
        return -RT_ERROR;
    }

    /* register touch device */
    touch_device->info.type = RT_TOUCH_TYPE_CAPACITANCE;
    touch_device->info.vendor = RT_TOUCH_VENDOR_UNKNOWN;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device st16xx init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device st16xx init success\n");
    return RT_EOK;
}

static int rt_hw_st16xx_port(void)
{
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rt_st16xx_gpio_cfg();

    rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    rt_hw_st16xx_init("st16xx", &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_st16xx_port);
