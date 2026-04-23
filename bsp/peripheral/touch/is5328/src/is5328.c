#include "is5328.h"

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "touch_common.h"

#define LOG_TAG "is5328"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>


static struct rt_i2c_client is5328_client;
static rt_err_t is5328_read(struct rt_i2c_client *dev, rt_uint32_t reg, rt_uint8_t *buf, rt_uint8_t len) {
    struct rt_i2c_msg msgs[2];
    rt_uint8_t addr_size = sizeof(uint32_t);
    rt_uint32_t wbuf = 0;
    int ret = 0;

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = (rt_uint8_t *)&wbuf;
    msgs[0].len = addr_size;

    msgs[1].addr = dev->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buf;
    msgs[1].len = len;
    wbuf = cpu_to_be32(reg);


    if (rt_i2c_transfer(dev->bus, msgs, 2) == 2) {
        LOG_D("I2c read reg success\n");
        return RT_EOK;
    } else {
        LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }

}

static rt_err_t is5328_write(struct rt_i2c_client *dev, rt_uint32_t reg, rt_uint8_t *buf, rt_uint8_t len) {
    struct rt_i2c_msg msg;
    rt_uint8_t *addr_buf;
    rt_uint8_t addr_size = sizeof(uint32_t);
    rt_uint8_t *buf_tmp = (rt_uint8_t *)buf;
    rt_uint32_t reg_tmp = reg;
    rt_uint16_t package_num;
    int ret = 0;

    if (len < 0) {
        LOG_E("buf is null\n");
        return -RT_ERROR;
    }

    msg.addr = dev->client_addr;
    msg.flags = RT_I2C_WR;
    msg.buf = addr_buf;

    /* 计算数据包数量：若数据长度小于246字节，则至少为1个包；否则按每包246字节进行向上取整分包 */
    package_num = len >= 246 ? (len % 246 == 0 ? (len / 246) : (len / 246 + 1)) : 1;
    // msgs.len = len;

    for (int i = 0;i < package_num;++i) {
        reg_tmp = reg + (i * 246);
        for (int j = 0;j < addr_size;j++) {
            addr_buf[j] = *(((uint8_t *)&reg_tmp + addr_size - 1 - j));
        }

        if (len >= 246) {
            memcpy(&addr_buf[addr_size], buf_tmp, 246);
            buf_tmp = buf_tmp + 246;
        } else {
            memcpy(&addr_buf[addr_size], buf_tmp, len);
        }

        msg.len = addr_size + (len >= 246 ? 246 : len);
        len = len >= 246 ? len - 246 : 0;
    }

    if (rt_i2c_transfer(dev->bus, &msg, 1) == 1) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }

    free(addr_buf);

    if (ret && reg != 0x180f0020 && reg != 0x180f0028) {
        LOG_E("Error writing %d bytes to 0x%04x:%d", len, reg, ret);
    }
    return ret;
}

static int16_t pre_x[5] = { -1, -1, -1, -1, -1 };
static int16_t pre_y[5] = { -1, -1, -1, -1, -1 };
static rt_uint8_t tp_dowm[5] = { 0 };
static struct rt_touch_data *read_data = RT_NULL;
static void is5328_touch_up(void *buf, int8_t id) {
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

static void is5328_touch_down(void *buf, int8_t id, int16_t x, int16_t y) {
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
static void is5328_read_point(struct rt_touch_device *touch, void *buf, rt_size_t read_num) {
    rt_uint8_t touch_buf[TOUCH_DATA_LEN] = { 0 };
    int touch_id[TOUCH_MAX_NUM];
    int touch_x[TOUCH_MAX_NUM];
    int touch_y[TOUCH_MAX_NUM];
    int touch_num;

    if (is5328_read(&is5328_client, TOUCH_DATA_ADDR, touch_buf, TOUCH_DATA_LEN) != RT_EOK) {

        LOG_E("Read touch data error\n");
        return -RT_ERROR;

    } else {

        touch_num = touch_buf[4];
        // printf("touch num: %d\n", touch_num);

    }

    if (touch_num) {
        for (int i = 0; i < touch_num; i++) {
            touch_id[i] = i;
            touch_x[i] = (touch_buf[8] << 8) | touch_buf[7];
            touch_y[i] = (touch_buf[10] << 8) | touch_buf[9];

            if (touch_x[i] != 0 || touch_y[i] != 0) {
                is5328_touch_down(buf, touch_id[i], touch_x[i], touch_y[i]);
                // printf("Touch ID: %d, X: %d, Y: %d\n", touch_id[i], touch_x[i], touch_y[i]);
            } else {
                // is5328_touch_up(buf, touch_id[i]);
            }
        }
    } else {
        for (int i = 0;i < TOUCH_MAX_NUM;i++) {
            is5328_touch_up(buf, touch_id[i]);
        }
    }


    // return NULL;
}
static rt_err_t is5328_control(struct rt_touch_device *touch, int cmd, void *arg) {
    struct rt_touch_info *info = RT_NULL;

    if (cmd == RT_TOUCH_CTRL_GET_INFO) {

        // return st7123_get_info(&st7123_client, data);

        info = (struct rt_touch_info *)arg;
        if (info == RT_NULL)
            return -RT_EINVAL;

        info->point_num = touch->info.point_num;
        info->range_x = touch->info.range_x;
        info->range_y = touch->info.range_y;
        info->type = touch->info.type;
        info->vendor = touch->info.vendor;
    }

    return RT_EOK;    return NULL;
}

static struct rt_touch_ops is5328_touch_ops = {
    .touch_readpoint = is5328_read_point,
    .touch_control = is5328_control,
};

struct rt_touch_info is5328_info = {
    .type = RT_TOUCH_TYPE_CAPACITANCE,
    .vendor = RT_TOUCH_VENDOR_UNKNOWN,
    .range_x = (rt_int32_t)540,
    .range_y = (rt_int32_t)540,
    .point_num = 5,
};

/**
 * @brief 初始化 IS5328 触摸芯片设备
 *
 * 该函数负责分配触摸设备内存，查找并打开对应的 I2C 总线，
 * 配置触摸设备信息，并将其注册到 RT-Thread 设备框架中。
 *
 * @param name 触摸设备的名称，用于在系统中唯一标识该设备
 * @param cfg 指向触摸设备配置结构的指针，包含设备引脚、中断等配置信息
 *
 * @return int 返回 RT_EOK 表示初始化成功，返回 -RT_ERROR 表示初始化失败
 */
static int rt_is5328_init(const char *name, struct rt_touch_config *cfg) {
    struct rt_touch_device *touch_device = RT_NULL;
    touch_device = (struct rt_touch_device *)rt_malloc(sizeof(struct rt_touch_device));
    if (touch_device == RT_NULL) {
        LOG_E("touch device malloc fail");
        return -RT_ERROR;
    }
    rt_memset((void *)touch_device, 0, sizeof(struct rt_touch_device));

    /* 查找并验证 I2C 总线设备 */
    is5328_client.bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->dev_name);

    if (is5328_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", cfg->dev_name);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)is5328_client.bus, RT_DEVICE_FLAG_RDWR) !=
        RT_EOK) {
        LOG_E("open %s device failed", cfg->dev_name);
        return -RT_ERROR;
    }

    is5328_client.client_addr = ISTARIC_SLAVE_ADDRESS;

    /* 配置触摸设备属性并注册到系统 */
    touch_device->info = is5328_info;
    rt_memcpy(&touch_device->config, cfg, sizeof(struct rt_touch_config));
    touch_device->ops = &is5328_touch_ops;

    if (RT_EOK != rt_hw_touch_register(touch_device, name, RT_DEVICE_FLAG_INT_RX, RT_NULL)) {
        LOG_E("touch device is5328 init failed !!!");
        return -RT_ERROR;
    }

    LOG_I("touch device is5328 init success");
    return RT_EOK;
}

static void is5328_chip_reset() {


}
static void is5328_gpio_init(struct rt_touch_config *cfg) {
    /* irq input 0 */
    // rt_pin_mode(cfg->irq_pin.pin, PIN_MODE_INPUT_PULLUP);

    /* pin rst */
    rt_thread_mdelay(10);

    rt_pin_mode(cfg->rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(cfg->rst_pin, PIN_HIGH);

    /* 保持高电平100ms */
    rt_thread_mdelay(100);
    rt_pin_write(cfg->rst_pin, PIN_LOW);
    /* 保持低电平10ms */
    rt_thread_mdelay(10);

    rt_pin_write(cfg->rst_pin, PIN_HIGH);
    // rt_thread_mdelay(100);



}

static int is5328_get_chip_info(struct rt_i2c_client *dev) {
    rt_uint8_t ret;
    rt_uint8_t chip_info[4] = { 0 };    //芯片料号
    rt_uint8_t firm_ver[7] = { 0 };     //固件版本

    rt_uint8_t *data = (rt_uint8_t *)rt_malloc(64);

    ret = is5328_read(dev, ISTARIC_CHIP_ID, data, 7);

    //获取芯片料号信息
    for (int i = 0;i < 4;i++) {
        chip_info[i] = data[i] & 0x0F;
    }
    LOG_I("Current chip info: IS%x%x%x%x", chip_info[0], chip_info[1], chip_info[2], chip_info[3]);

    //获取固件版本信息
    ret = is5328_read(dev, ISTARIC_FIRM_VERSION, data, 4);
    LOG_I("firmware version: %02x.%02x.%02x.%02x", data[0], data[1], data[2], data[3]);

    // while(1){
    //     is5328_read(dev, 0x840034FC, data, 20);
    //     printf("touch data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n",
    //         data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19]);
    //     rt_thread_mdelay(10);
    // }

    return ret;

}
static int rt_hw_is5328_port(void) {
    struct rt_touch_config cfg = {
        .dev_name = AIC_TOUCH_PANEL_I2C_CHAN,
        .irq_pin.pin = drv_pin_get(AIC_TOUCH_PANEL_INT_PIN),
        .irq_pin.mode = PIN_MODE_INPUT,
        .rst_pin = drv_pin_get(AIC_TOUCH_PANEL_RST_PIN),
    };
    is5328_gpio_init(&cfg);

    rt_is5328_init(AIC_TOUCH_PANEL_NAME, &cfg);

    is5328_get_chip_info(&is5328_client);

    return 0;
}



INIT_DEVICE_EXPORT(rt_hw_is5328_port);