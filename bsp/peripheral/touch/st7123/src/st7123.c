#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <aic_hal_gpio.h>
#include <aic_drv_gpio.h>
#include "rtdbg.h"
#include "st7123.h"
#include "touch_common.h"

#define LOG_TAG "st7123"

static struct rt_i2c_client st7123_client;

static rt_err_t st7123_write_reg(struct rt_i2c_client *dev, rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs;

    msgs.addr = dev->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(dev->bus, &msgs, 1) == 1) {
        LOG_D("I2c write reg success\n");
        return RT_EOK;
    } else {
        // LOG_E("I2C write reg error\n");
        return -RT_ERROR;
    }
}

static rt_err_t st7123_read_reg(struct rt_i2c_client *dev, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs[2];
    rt_size_t read_size;

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = ST7123_REGISTER_LEN;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    read_size = rt_i2c_transfer(dev->bus, msgs, 2);
    if (read_size == 2) {
        LOG_D("I2c read reg success\n");
        return RT_EOK;
    } else {
        LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }
}

static rt_err_t st7123_get_info(struct rt_i2c_client *dev, struct rt_touch_info *info) {
    rt_uint8_t reg[2];
    rt_uint8_t info_buf[5];

    if (info == RT_NULL)
        return -RT_EINVAL;

    reg[0] = (rt_uint8_t)(ST7123_MAX_X_COORD >> 8);
    reg[1] = (rt_uint8_t)(ST7123_MAX_X_COORD & 0xFF);

    if (st7123_read_reg(dev, reg, info_buf, sizeof(info_buf)) != RT_EOK) {
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

static int16_t pre_x[ST7123_MAX_TOUCH] = { 0 };
static int16_t pre_y[ST7123_MAX_TOUCH] = { 0 };
static rt_uint8_t tp_dowm[ST7123_MAX_TOUCH] = { 0 };
static struct rt_touch_data *read_data = RT_NULL;

static void st7123_touch_up(void *buf, int8_t id) {
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

static void st7123_touch_down(void *buf, int8_t id, int16_t x, int16_t y) {
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

static rt_size_t st7123_read_point(struct rt_touch_device *touch,
    void *buf, rt_size_t read_num) {
    rt_uint8_t point_status = 0;
    rt_uint8_t touch_num = 0;
    rt_uint8_t cmd[2], i, num_valid;
    rt_uint8_t read_buf[7 * ST7123_MAX_TOUCH + 5] = { 0 };
    rt_uint8_t read_index;
    rt_uint8_t dev_status;
    rt_uint8_t error_code;
    int8_t read_id = 0;
    int16_t input_x = 0;
    int16_t input_y = 0;
    rt_uint8_t id[5] = { 0 };
    static rt_uint8_t pre_touch = 0;
    static int8_t pre_id[ST7123_MAX_TOUCH] = { 0 };

    rt_memset(buf, 0, sizeof(struct rt_touch_data) * read_num);

    /* point status register */
    cmd[0] = (rt_uint8_t)((ST7123_DEV_STATUS >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7123_DEV_STATUS & 0xFF);

    if (st7123_read_reg(&st7123_client, cmd, &point_status, 1) != RT_EOK) {
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

    cmd[0] = (rt_uint8_t)((ST7123_TOUCH_INFO >> 8) & 0xFF);
    cmd[1] = (rt_uint8_t)(ST7123_TOUCH_INFO & 0xFF);
    /* read point num is touch_num */
    if (st7123_read_reg(&st7123_client, cmd, read_buf, sizeof(read_buf)) != RT_EOK) {
        rt_kprintf("read point failed\n");
        read_num = 0;
        goto __exit;
    }

    for (i = 0; i < ST7123_MAX_TOUCH; i++) {
        num_valid = ((read_buf[7 * i + 4] & 0x80) != 0) ? 1 : 0;
        touch_num += num_valid;
    }

    if (touch_num > ST7123_MAX_TOUCH) {
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
                    st7123_touch_up(buf, up_id);
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

            st7123_touch_down(buf, read_id, input_x, input_y);
        }
    } else if (pre_touch) {
        for (read_index = 0; read_index < pre_touch; read_index++) {
            st7123_touch_up(buf, pre_id[read_index]);
        }
    }

    pre_touch = touch_num;

__exit:
    return read_num;
}

static rt_err_t st7123_control(struct rt_touch_device *touch, int cmd, void *arg) {
    if (cmd == RT_TOUCH_CTRL_GET_INFO) {
        return st7123_get_info(&st7123_client, arg);
    }

    return RT_EOK;
}

static struct rt_touch_ops st7123_touch_ops = {
    .touch_readpoint = st7123_read_point,
    .touch_control = st7123_control,
};

static int st7123_hw_init(const char *name, struct rt_touch_config *cfg) {
    struct rt_touch_device *touch_device = RT_NULL;

    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    st7123_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (st7123_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)st7123_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    st7123_client.client_addr = ST7123_SLAVE_ADDR;

    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &st7123_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device st7123 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device st7123 init success");
    return RT_EOK;
}

static void st7123_gpio_init(struct rt_touch_config *cfg) {

    rt_thread_mdelay(20);

    /* rst output 0 */
    rt_pin_mode(cfg->rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->rst_pin, PIN_HIGH);
    rt_thread_mdelay(5);

    /* irq output 0 */
    // rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_OUTPUT);
    // rt_pin_write(cfg->irq_pin.pin, PIN_LOW);
    // rt_thread_mdelay(10);

    /* rst output 1 */
    rt_pin_mode(cfg->rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->rst_pin, PIN_HIGH);
    rt_thread_mdelay(10);

    /* irq input float */
    rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT);
}

static int rt_hw_st7123_port(void) {
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN);

    st7123_gpio_init(&cfg);

    st7123_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_st7123_port);

