
#include "string.h"
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "hy4635.h"
#include "touch_common.h"

#define LOG_TAG "hy4635"

static struct rt_i2c_client hy4635_client;

static rt_err_t hy4635_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        LOG_D("I2c write reg success\n");
        return RT_EOK;
    } else {
        LOG_E("I2C write reg error\n");
        return -RT_ERROR;
    }
}

static rt_err_t hy4635_read_reg(struct rt_i2c_client *dev, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len) {
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
        LOG_D("I2c read reg success\n");
        return RT_EOK;
    } else {
        LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }
}

static int16_t pre_x[HY4635_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[HY4635_MAX_TOUCH] = { -1, -1, -1, -1, -1 };
static rt_uint8_t tp_dowm[HY4635_MAX_TOUCH] = { 0 };
static struct rt_touch_data *read_data = RT_NULL;

static void hy4635_touch_up(void *buf, int8_t id) {
    read_data = (struct rt_touch_data *)buf;

    if (tp_dowm[id] == 1) {
        tp_dowm[id] = 0;
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

static void hy4635_touch_down(void *buf, int8_t id, int16_t x, int16_t y) {
    read_data = (struct rt_touch_data *)buf;

    if (tp_dowm[id] == 1) {
        read_data[id].event = RT_TOUCH_EVENT_MOVE;
    } else {
        read_data[id].event = RT_TOUCH_EVENT_DOWN;
        tp_dowm[id] = 1;
    }

    read_data[id].timestamp = rt_touch_get_ts();

    read_data[id].x_coordinate = x;
    read_data[id].y_coordinate = y;
    read_data[id].track_id = id;

    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
}

static void hy4635_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num) {
    rt_uint8_t touch_num = 0;
    rt_uint8_t reg;
    rt_uint8_t touch_status;
    rt_uint8_t read_buf[6 * HY4635_MAX_TOUCH] = { 0 };
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    //获取当前触点数量
    reg = HY4635_TOUCH_FINGER_NUM;
    if (hy4635_read_reg(&hy4635_client, &reg, &touch_status, 1) == RT_EOK) {
        LOG_D("read touch states success,current finger num is %d", touch_status);
    } else {
        LOG_E("read touch states failed");
        read_num = 0;
        // rt_memset(read_buf, 0, sizeof(read_buf));
        return read_num;
    }

    touch_num = touch_status & 0x0f;
    if (touch_num == 0) {
        read_num = 0;
        // return read_num;
    }

    //获取第一个触点坐标
    reg = HY4635_TOUCH1_XH;
    if (hy4635_read_reg(&hy4635_client, &reg, read_buf, sizeof(read_buf)) == RT_EOK) {
        LOG_D("read touch data :");
    } else {
        LOG_E("read touch data failed");
        read_num = 0;
        return read_num;
    }

    if (touch_num) {
        rt_uint8_t off_set;
        rt_uint8_t point_state;

        for (rt_uint8_t i = 0; i < touch_num; i++) {
            off_set = i * 6;
            read_id = (read_buf[i * 6 + 2] & 0xf0) >> 4;
            point_state = (read_buf[off_set] & 0xf0) >> 6;  //1:up 2:down
            input_y = ((read_buf[off_set] & 0xf) << 8) | read_buf[off_set + 1];
            input_x = ((read_buf[off_set + 2] & 0xf) << 8) | read_buf[off_set + 3];

            aic_touch_flip(&input_x, &input_y);
            aic_touch_rotate(&input_x, &input_y);
            aic_touch_scale(&input_x, &input_y);
            if (!aic_touch_crop(&input_x, &input_y))
                continue;

            if (point_state == 1) {
                hy4635_touch_up(buf, read_id);
            }
            if (point_state == 2) {
                hy4635_touch_down(buf, read_id, input_x, input_y);
            }
        }
    } else {
        for (rt_uint8_t i = 0; i < HY4635_MAX_TOUCH; i++) {
            read_id = (read_buf[i * 6 + 2] & 0xf0) >> 4;
            hy4635_touch_up(buf, read_id);
        }
    }

}

static rt_err_t hy4635_control(struct rt_touch_device *touch, int cmd, void *data) {
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

const struct rt_touch_ops hy4635_touch_ops = {
    .touch_readpoint = hy4635_read_point,
    .touch_control = hy4635_control,
};

struct rt_touch_info hy4635_info = {
    .type = RT_TOUCH_TYPE_CAPACITANCE,
    .vendor = RT_TOUCH_VENDOR_UNKNOWN,
    .range_x = (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    .range_y = (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
    .point_num = 5,
};

void hy4635_hw_init(const char *name, struct rt_touch_config *cfg) {
    rt_touch_t touch_device = NULL;
    rt_uint8_t cmd[2] = { 0 };

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

    hy4635_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (hy4635_client.bus) {
        LOG_D("found %s device.", cfg->dev_name);
    } else {
        LOG_E("Can't find %s device", cfg->dev_name);
    }

    if (rt_device_open((rt_device_t)hy4635_client.bus, RT_DEVICE_FLAG_RDWR) == RT_EOK) {
        LOG_D("opened %s device.", cfg->dev_name);
    } else {
        LOG_E("open %s device failed.", cfg->dev_name);
        return -RT_ERROR;
    }

    hy4635_client.client_addr = HY4635_SLAVE_ADDR;

    //设置TP为工作模式
    cmd[0] = HY4635_DEV_MODE;
    cmd[1] = 0x00;
    if (hy4635_write_reg(&hy4635_client, cmd, 2) == RT_EOK) {
        LOG_D("hy4635 write reg dev_mode success");
    } else {
        LOG_E("hy4635 write reg dev_mode failed");
        return -RT_ERROR;
    }

    cmd[0] = HY4635_TOUCH_THRESHOLD;
    cmd[1] = 0x0C;
    if (hy4635_write_reg(&hy4635_client, cmd, 2) != RT_EOK)
        return -RT_ERROR;

    cmd[0] = HY4635_REPORT_SPEED;
    cmd[1] = 0x64;
    if (hy4635_write_reg(&hy4635_client, cmd, 2) != RT_EOK)
        return -RT_ERROR;

    /* register touch device */
    touch_device->info = hy4635_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &hy4635_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device hy4635 init failed !!!");
        return -RT_ERROR;
    }
    LOG_I("touch device hy4635 init success");

    return RT_EOK;

}

static int rt_hw_hy4635_port(void) {
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    rst_pin = rt_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = rt_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    hy4635_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_hy4635_port);
