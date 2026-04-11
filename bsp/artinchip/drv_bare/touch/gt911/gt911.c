/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gt911.h"

#define TOUCH_NAME      "gt911"
#define I2C_BASE(n)     (I2C0_BASE + (n * 0x1000))
#define I2C_CLK_ID(x)   (CLK_I2C0 + x)
#define I2C_RATE        400000


aic_i2c_ctrl g_i2c_dev;
static struct aic_i2c_msg g_msgs;
static uint8_t g_buf[40];

static int gt911_write_reg(aic_i2c_ctrl *i2c_dev)
{
    uint32_t bytes_cnt = 0;

    bytes_cnt = hal_i2c_master_send_msg(i2c_dev, i2c_dev->msg, 1);
    if (bytes_cnt != i2c_dev->msg->len) {
        printf("i2c write gt911 error\n");
        return TOUCH_ERROR;
    } else {
        return TOUCH_OK;
    }
}

static int gt911_read_reg(aic_i2c_ctrl *i2c_dev)
{
    uint32_t bytes_cnt = 0;

    bytes_cnt = hal_i2c_master_receive_msg(i2c_dev, i2c_dev->msg, 1);
    if (bytes_cnt != i2c_dev->msg->len) {
        printf("i2c read gt911 error\n");
        return TOUCH_ERROR;
    } else {
        return TOUCH_OK;
    }
}

static int gt911_get_id(aic_i2c_ctrl *i2c_dev, uint8_t *data)
{
    uint8_t reg[2];

    reg[0] = (uint8_t)(GT911_PRODUCT_ID >> 8);
    reg[1] = (uint8_t)(GT911_PRODUCT_ID & 0xff);

    memset(&g_msgs, 0, sizeof(g_msgs));
    memset(&g_buf, 0, sizeof(g_buf));

    g_msgs.buf = g_buf;
    /* write operation */
    g_msgs.buf[0] = reg[0];
    g_msgs.buf[1] = reg[1];
    g_msgs.len = 2;
    g_msgs.addr = GT911_ADDRESS_HIGH;
    i2c_dev->msg = &g_msgs;

    gt911_write_reg(i2c_dev);

    /* read operation */
    memset(&g_buf, 0, sizeof(g_buf));
    g_msgs.len = 6;
    g_msgs.buf = data;
    i2c_dev->msg = &g_msgs;

    if (gt911_read_reg(i2c_dev)) {
        printf("i2c read gt911 product id fail\n");
        return TOUCH_ERROR;
    }

    return TOUCH_OK;
}

static int gt911_get_info(aic_i2c_ctrl *i2c_dev, struct touch_info *info)
{
    uint8_t reg[2];
    uint8_t out_info[7];

    reg[0] = (uint8_t)(GT911_CONFIG_REG >> 8);
    reg[1] = (uint8_t)(GT911_CONFIG_REG & 0xff);

    memset(&g_msgs, 0, sizeof(g_msgs));
    memset(&g_buf, 0, sizeof(g_buf));

    g_msgs.buf = g_buf;
    /* write operation */
    g_msgs.buf[0] = reg[0];
    g_msgs.buf[1] = reg[1];
    g_msgs.len = 2;
    g_msgs.addr = GT911_ADDRESS_HIGH;
    i2c_dev->msg = &g_msgs;

    gt911_write_reg(i2c_dev);

    /* read operation */
    memset(&g_buf, 0, sizeof(g_buf));
    g_msgs.len = 7;
    g_msgs.buf = out_info;
    i2c_dev->msg = &g_msgs;

    if (gt911_read_reg(i2c_dev)) {
        printf("i2c read gt911 product id fail\n");
        return TOUCH_ERROR;
    }

    info->range_x = (out_info[2] << 8) | out_info[1];
    info->range_y = (out_info[4] << 8) | out_info[3];
    info->point_num = out_info[5] & 0x0f;

    return TOUCH_OK;
}

static int16_t pre_x[GT911_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[GT911_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_w[GT911_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static uint8_t s_tp_dowm[GT911_MAX_TOUCH];
static struct touch_data *read_data;

static void gt911_touch_up(void *buf, int8_t id)
{
    read_data = (struct touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        s_tp_dowm[id] = 0;
        read_data[id].event = TOUCH_EVENT_UP;
    } else {
        read_data[id].event = TOUCH_EVENT_NONE;
    }

    read_data[id].width = pre_w[id];
    read_data[id].x_coordinate = pre_x[id];
    read_data[id].y_coordinate = pre_y[id];
    read_data[id].track_id = id;

    pre_x[id] = -1; /* last point is none */
    pre_y[id] = -1;
    pre_w[id] = -1;
}

static void gt911_touch_down(void *buf, int8_t id, int16_t x, int16_t y,
                             int16_t w)
{
    read_data = (struct touch_data *)buf;

    if (s_tp_dowm[id] == 1) {
        read_data[id].event = TOUCH_EVENT_MOVE;
    } else {
        read_data[id].event = TOUCH_EVENT_DOWN;
        s_tp_dowm[id] = 1;
    }

    read_data[id].width = w;
    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    pre_w[id] = w;
}

static int gt911_read_point(struct touch *touch, void *buf,
                                  unsigned int read_num)
{
    uint8_t point_status = 0;
    uint8_t touch_num = 0;
    uint8_t write_buf[3];
    uint8_t reg[2];
    uint8_t read_buf[8 * GT911_MAX_TOUCH] = { 0 };
    uint8_t read_index;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    int16_t input_w = 0;
    static uint8_t pre_touch = 0;
    static int8_t pre_id[GT911_MAX_TOUCH] = { 0 };

    memset(buf, 0, sizeof(struct touch_data) * read_num);
    memset(&g_msgs, 0, sizeof(g_msgs));
    memset(&g_buf, 0, sizeof(g_buf));

    g_msgs.buf = g_buf;
    /* point status register */
    reg[0] = (uint8_t)((GT911_READ_STATUS >> 8) & 0xFF);
    reg[1] = (uint8_t)(GT911_READ_STATUS & 0xFF);

    g_msgs.buf[0] = reg[0];
    g_msgs.buf[1] = reg[1];
    g_msgs.len = 2;
    g_msgs.addr = GT911_ADDRESS_HIGH;
    g_i2c_dev.msg = &g_msgs;

    gt911_write_reg(&g_i2c_dev);

    /* read operation */
    memset(&g_buf, 0, sizeof(g_buf));
    g_msgs.len = 1;
    g_msgs.buf = &point_status;
    g_i2c_dev.msg = &g_msgs;

    if (gt911_read_reg(&g_i2c_dev)) {
        printf("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if (point_status == 0) {     /* no data */
        read_num = 0;
        goto exit_;
    }

    if ((point_status & 0x80) == 0) {       /* data is not ready */
        read_num = 0;
        goto exit_;
    }

    touch_num = point_status & 0x0f; /* get point num */

    if (touch_num > GT911_MAX_TOUCH) /* point num is not correct */
    {
        read_num = 0;
        goto exit_;
    }

    reg[0] = (uint8_t)((GT911_POINT1_REG >> 8) & 0xFF);
    reg[1] = (uint8_t)(GT911_POINT1_REG & 0xFF);

    memset(&g_buf, 0, sizeof(g_buf));
    g_msgs.buf[0] = reg[0];
    g_msgs.buf[1] = reg[1];
    g_msgs.len = 2;
    g_msgs.addr = GT911_ADDRESS_HIGH;
    g_i2c_dev.msg = &g_msgs;

    gt911_write_reg(&g_i2c_dev);

    memset(&g_buf, 0, sizeof(g_buf));
    g_msgs.len = 40;
    g_msgs.buf = read_buf;
    g_i2c_dev.msg = &g_msgs;

    /* read point num is touch_num */
    if (gt911_read_reg(&g_i2c_dev)) {
        printf("read point failed\n");
        read_num = 0;
        goto exit_;
    }

    if (pre_touch > touch_num) /* point up */
    {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            uint8_t j;

            for (j = 0; j < touch_num; j++) /* this time touch num */
            {
                read_id = read_buf[j * 8] & 0x0F;

                if (pre_id[read_index] == read_id) /* this id is not free */
                    break;

                if (j >= touch_num - 1) {
                    uint8_t up_id;
                    up_id = pre_id[read_index];
                    gt911_touch_up(buf, up_id);
                }
            }
        }
    }

    if (touch_num) /* point down */
    {
        uint8_t off_set;

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

            gt911_touch_down(buf, read_id, input_x, input_y, input_w);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            gt911_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

exit_:
    write_buf[0] = (uint8_t)((GT911_READ_STATUS >> 8) & 0xFF);
    write_buf[1] = (uint8_t)(GT911_READ_STATUS & 0xFF);
    write_buf[2] = 0x00;

    memset(&g_buf, 0, sizeof(g_buf));
    g_msgs.buf[0] = write_buf[0];
    g_msgs.buf[1] = write_buf[1];
    g_msgs.buf[2] = write_buf[2];
    g_msgs.len = 3;
    g_msgs.addr = GT911_ADDRESS_HIGH;
    g_i2c_dev.msg = &g_msgs;

    gt911_write_reg(&g_i2c_dev);

    return read_num;
}

static int gt911_control(struct touch *touch, int cmd, void *arg)
{
    if (cmd == TOUCH_CTRL_GET_ID) {
        return gt911_get_id(&g_i2c_dev, arg);
    }

    if (cmd == TOUCH_CTRL_GET_INFO) {
        return gt911_get_info(&g_i2c_dev, arg);
    }

    return TOUCH_OK;
}

static struct touch_ops gt911_ops = {
    .control = gt911_control,
    .readpoint = gt911_read_point,
};


int gt911_init(struct touch *touch)
{
    int id, ret;
    long rst_pin,irq_pin;

    memset(&g_msgs, 0, sizeof(g_msgs));
    memset(&g_buf, 0, sizeof(g_buf));

    ret = strcmp(touch->name, TOUCH_NAME);
    if (ret) {
        ret = TOUCH_ERROR;
        printf("can not find %s\n", touch->name);
        return TOUCH_ERROR;
    }

    id = touch_get_i2c_chan_id(AIC_TOUCH_GT911_I2C_CHA);

    g_msgs.buf = g_buf;
    g_i2c_dev.index = id;
    g_i2c_dev.reg_base = I2C_BASE(id);
    g_i2c_dev.msg = &g_msgs;
    g_i2c_dev.addr_bit = I2C_7BIT_ADDR;
    g_i2c_dev.target_rate = I2C_RATE;
    g_i2c_dev.bus_mode = I2C_MASTER_MODE;
    g_i2c_dev.clk_id = I2C_CLK_ID(id);

    hal_i2c_init(&g_i2c_dev);

    rst_pin = touch_pin_get(GT911_RST_PIN);
    irq_pin = touch_pin_get(GT911_INT_PIN);

    /* hw init */
    touch_set_pin_mode(rst_pin, PIN_OUTPUT_MODE);
    touch_pin_write(rst_pin, LOW_POWER);
    aicos_mdelay(10);

    touch_set_pin_mode(irq_pin, PIN_OUTPUT_MODE);
    touch_pin_write(irq_pin, LOW_POWER);
    aicos_mdelay(10);

    touch_pin_write(rst_pin, HIGH_POWER);
    aicos_mdelay(10);
    touch_set_pin_mode(rst_pin, PIN_INPUT_MODE);

    touch_pin_write(irq_pin, LOW_POWER);
    aicos_mdelay(50);
    touch_set_pin_mode(irq_pin, PIN_INPUT_MODE);

    touch->info.type = TOUCH_TYPE_CAPACITANCE;
    touch->info.type = TOUCH_VENDOR_GT;
    touch->ops = &gt911_ops;

    touch_register(touch);

    return ret;
}
