#include "string.h"
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"

#define LOG_TAG "I2C_TEST"

static struct rt_i2c_client my_i2c;

static rt_err_t my_i2c_read_reg(struct rt_i2c_client *dev, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = dev->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = 2; //16位寄存器地址

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

void i2c_start(void) {
    rt_uint8_t reg[2] = { 0 };
    rt_uint8_t read_buf[10 * 7 + 5] = { 0 };

    reg[0] = (rt_uint8_t)((0x0010 >> 8) & 0xff);
    reg[1] = (rt_uint8_t)(0x0010 & 0xff);

    my_i2c.bus = rt_i2c_bus_device_find("i2c3");
    my_i2c.client_addr = 0x55;

    if (my_i2c_read_reg(&my_i2c, reg, read_buf, 75) == RT_EOK) {
        LOG_D("read  data :");
    } else {
        LOG_E("read  data failed");
    }

    for (rt_uint8_t i = 0; i < 75; i++) {
        if (i && (i % 16 == 0))
            rt_kprintf("\n");
        if (i % 16 == 0)
            rt_kprintf("0x%02X: ", reg);
        else if (i % 8 == 0)
            rt_kprintf("   ");

        rt_kprintf("%02X ", read_buf[i]);
    }
}

MSH_CMD_EXPORT(i2c_start, my i2c tools);
