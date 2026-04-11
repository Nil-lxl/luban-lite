#include "string.h"
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
        // LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }
}
static rt_err_t st77922_get_info(struct rt_i2c_client *dev, struct rt_touch_info *info) {
    rt_uint8_t reg[2];
    rt_uint8_t read_buf[16];

    reg[0] = (rt_uint8_t)(0x0000 >> 8);
    reg[1] = (rt_uint8_t)(0x0000 & 0xFF);

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

    pre_x[id] = 0; /* last point is none */
    pre_y[id] = 0;
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

rt_uint8_t st7123_get_touch_num(rt_uint8_t *buf, touch_point_t *touch_point) {
    rt_uint8_t touch_num = 0;
    for (int i = 0;i < ST7123_MAX_TOUCH; i++) {
        touch_point[i].id = i;
        touch_point[i].status = buf[4 + i * 7] >> 7;
        if (touch_point[i].status == 1) {
            touch_num++;
        }
    }

    return touch_num;
}
static void st7123_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num) {
#if 1
    rt_uint8_t touch_num = 0;
    rt_uint8_t reg[2];
    rt_uint8_t read_buf[ST7123_MAX_TOUCH * 7 + 5] = { 0 };
    touch_point_t *touch_point;

    reg[0] = (rt_uint8_t)((ST7123_TOUCH_INFO >> 8) & 0xff);
    reg[1] = (rt_uint8_t)(ST7123_TOUCH_INFO & 0xff);

    if (st7123_read_reg(&st7123_client, reg, read_buf, sizeof(read_buf)) == RT_EOK) {
        LOG_D("read touch data :");
    } else {
        // LOG_E("read touch data failed");
    }

    touch_num = st7123_get_touch_num(&read_buf, touch_point);

    if (touch_num) {

        for (int i = 0;i < ST7123_MAX_TOUCH;i++) {
            touch_point[i].touch_x = 0;
            touch_point[i].touch_y = 0;     //init point

            touch_point[i].touch_x = ((read_buf[touch_point[i].id * 7 + 4] & 0x0f) << 8) | read_buf[touch_point[i].id * 7 + 5];
            touch_point[i].touch_y = ((read_buf[touch_point[i].id * 7 + 6] & 0x0f) << 8) | read_buf[touch_point[i].id * 7 + 7];

            aic_touch_flip(&touch_point[i].touch_x, &touch_point[i].touch_y);
            aic_touch_rotate(&touch_point[i].touch_x, &touch_point[i].touch_y);
            aic_touch_scale(&touch_point[i].touch_x, &touch_point[i].touch_y);
            // if (!aic_touch_crop(&touch_point[i].touch_x, &touch_point[i].touch_y))
            //     continue;

            if (touch_point[i].status == 1) {
                st7123_touch_down(buf, touch_point[i].id, touch_point[i].touch_x, touch_point[i].touch_y);
            }
            if (touch_point[i].status == 0) {
                st7123_touch_up(buf, touch_point[i].id);
            }
        }
    } else {
        for (int i = 0; i < ST7123_MAX_TOUCH; i++) {
            st7123_touch_up(buf, touch_point[i].id);
        }
    }
#else

#endif

}

static rt_err_t st7123_control(struct rt_touch_device *touch, int cmd, void *data) {
    struct rt_touch_info *info = RT_NULL;

    if (cmd == RT_TOUCH_CTRL_GET_INFO) {

        // return st7123_get_info(&st7123_client, data);

        info = (struct rt_touch_info *)data;
        if (info == RT_NULL)
            return -RT_EINVAL;

        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;
    }

    return RT_EOK;
}

const struct rt_touch_ops st7123_touch_ops = {
    .touch_readpoint = st7123_read_point,
    .touch_control = st7123_control,
};

struct rt_touch_info st7123_info = {
    .type = RT_TOUCH_TYPE_CAPACITANCE,
    .vendor = RT_TOUCH_VENDOR_UNKNOWN,
    .range_x = (rt_int32_t)AIC_TOUCH_X_COORDINATE_RANGE,
    .range_y = (rt_int32_t)AIC_TOUCH_Y_COORDINATE_RANGE,
    .point_num = 10,
};


static int st7123_hw_init(const char *name, struct rt_touch_config *cfg) {
    rt_touch_t touch_device = NULL;
    rt_uint8_t cmd[3] = { 0 };

    touch_device = (rt_touch_t)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    st7123_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);
    if (st7123_client.bus) {
        LOG_D("found %s device.", cfg->dev_name);
    } else {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)st7123_client.bus, RT_DEVICE_FLAG_RDWR) == RT_EOK) {
        LOG_D("opened %s device.", cfg->dev_name);
    } else {
        LOG_E("open %s device failed.", cfg->dev_name);
        return -RT_ERROR;
    }

    st7123_client.client_addr = ST7123_SLAVE_ADDR;

    touch_device->info = st7123_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &st7123_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device st7123 init failed !!!");
        return -RT_ERROR;
    }
    LOG_I("touch device st7123 init success");

    return RT_EOK;
}

static int st7123_gpio_cfg() {
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

static int rt_hw_st7123_port(void) {
    struct rt_touch_config cfg;
    rt_uint8_t rst_pin;

    st7123_gpio_cfg();

    rst_pin = rt_pin_get(AIC_TOUCH_PANEL_RST_PIN);
    cfg.dev_name = AIC_TOUCH_PANEL_I2C_CHAN;
    cfg.irq_pin.pin = rt_pin_get(AIC_TOUCH_PANEL_INT_PIN);
    cfg.irq_pin.mode = PIN_MODE_INPUT;
    cfg.user_data = &rst_pin;

    st7123_hw_init(AIC_TOUCH_PANEL_NAME, &cfg);

    return 0;
}

INIT_DEVICE_EXPORT(rt_hw_st7123_port);

