/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 * 2021-04-20     RiceChen      added support for bus control api
 */

#include <rtdevice.h>

#define DBG_TAG               "I2C"
#ifdef RT_I2C_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

rt_err_t rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name)
{
    rt_err_t res = RT_EOK;

    rt_mutex_init(&bus->lock, "i2c_bus_lock", RT_IPC_FLAG_PRIO);

    if (bus->timeout == 0) bus->timeout = RT_TICK_PER_SECOND;

    res = rt_i2c_bus_device_device_init(bus, bus_name);

    LOG_I("I2C bus [%s] registered", bus_name);

    return res;
}

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name)
{
    struct rt_i2c_bus_device *bus;
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == RT_NULL || dev->type != RT_Device_Class_I2CBUS)
    {
        LOG_E("I2C bus %s not exist", bus_name);

        return RT_NULL;
    }

    bus = (struct rt_i2c_bus_device *)dev->user_data;

    return bus;
}

rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num)
{
    rt_size_t ret;

    if (bus->ops->master_xfer)
    {
#ifdef RT_I2C_DEBUG
        for (ret = 0; ret < num; ret++)
        {
            LOG_D("msgs[%d] %c, addr=0x%02x, len=%d", ret,
                  (msgs[ret].flags & RT_I2C_RD) ? 'R' : 'W',
                  msgs[ret].addr, msgs[ret].len);
        }
#endif

        rt_mutex_take(&bus->lock, RT_WAITING_FOREVER);
        ret = bus->ops->master_xfer(bus, msgs, num);
        rt_mutex_release(&bus->lock);

        return ret;
    }
    else
    {
        LOG_E("I2C bus operation not supported");

        return 0;
    }
}

rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus,
                        rt_uint32_t               cmd,
                        rt_uint32_t               arg)
{
    rt_err_t ret;

    if(bus->ops->i2c_bus_control)
    {
        ret = bus->ops->i2c_bus_control(bus, cmd, arg);

        return ret;
    }
    else
    {
        LOG_E("I2C bus operation not supported");

        return 0;
    }
}

rt_err_t rt_i2c_slave_control(struct rt_i2c_bus_device *bus,
                              rt_uint32_t               cmd,
                              void                     *arg)
{
    rt_err_t ret;

    if(bus->ops->i2c_slave_control)
    {
        ret = bus->ops->i2c_slave_control(bus, cmd, arg);

        return ret;
    }
    else
    {
        LOG_E("I2C bus operation not supported");

        return -RT_ERROR;
    }
}

rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count)
{
    rt_size_t ret;
    struct rt_i2c_msg msg;

    msg.addr  = addr;
    msg.flags = flags;
    msg.len   = count;
    msg.buf   = (rt_uint8_t *)buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret > 0) ? count : ret;
}

rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count)
{
    rt_size_t ret;
    struct rt_i2c_msg msg;
    RT_ASSERT(bus != RT_NULL);

    msg.addr   = addr;
    msg.flags  = flags | RT_I2C_RD;
    msg.len    = count;
    msg.buf    = buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret > 0) ? count : ret;
}

#ifdef AIC_I2C_DRV
rt_size_t rt_i2c_write_reg(struct rt_i2c_bus_device *bus,
                           rt_uint16_t               addr,
                           rt_uint8_t                reg,
                           rt_uint8_t               *buf,
                           rt_uint32_t               count)
{
    rt_size_t ret = 0;
    rt_uint8_t cmd[AIC_I2C_CMD_BUF_LEN] = {0};
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    if (count == 0 || count > (AIC_I2C_CMD_BUF_LEN - 1)) {
        LOG_E("rt_i2c_write_reg: write buf len out of range\n");
        return 0;
    }

    cmd[0] = reg;
    rt_memcpy(&cmd[1], buf, count);

    ret = rt_i2c_master_send(bus, addr, RT_I2C_WR, cmd, count + 1);
    if (ret != (count + 1))
        return 0;

    return count;
}

rt_size_t rt_i2c_write_reg16(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               reg,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count)
{
    rt_size_t ret = 0;
    rt_uint8_t cmd[AIC_I2C_CMD_BUF_LEN] = {0};
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    if (count == 0 || count > (AIC_I2C_CMD_BUF_LEN - 2)) {
        LOG_E("rt_i2c_write_reg16: write buf len out of range\n");
        return 0;
    }

    cmd[0] = (rt_uint8_t)(reg >> 8);
    cmd[1] = (rt_uint8_t)(reg & 0xff);
    rt_memcpy(&cmd[2], buf, count);

    ret = rt_i2c_master_send(bus, addr, RT_I2C_WR, cmd, count + 2);
    if (ret != (count + 2))
        return 0;

    return count;
}

rt_size_t rt_i2c_read_reg(struct rt_i2c_bus_device *bus,
                          rt_uint16_t               addr,
                          rt_uint8_t                reg,
                          rt_uint8_t               *buf,
                          rt_uint32_t               count)
{
    rt_size_t ret;
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    if (count == 0) {
        LOG_E("rt_i2c_read_reg: read buf len out of range\n");
        return 0;
    }

    ret = rt_i2c_master_send(bus, addr, RT_I2C_WR, &reg, 1);
    if (ret != 1)
        return ret;

    return rt_i2c_master_recv(bus, addr, RT_I2C_RD, buf, count);
}

rt_size_t rt_i2c_read_reg16(struct rt_i2c_bus_device *bus,
                            rt_uint16_t               addr,
                            rt_uint16_t               reg,
                            rt_uint8_t               *buf,
                            rt_uint32_t               count)
{
    rt_size_t ret;
    rt_uint8_t cmd[2] = {0};
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    if (count == 0) {
        LOG_E("rt_i2c_read_reg16: read buf len out of range\n");
        return 0;
    }

    cmd[0] = (rt_uint8_t)(reg >> 8);
    cmd[1] = (rt_uint8_t)(reg & 0xff);

    ret = rt_i2c_master_send(bus, addr, RT_I2C_WR, cmd, 2);
    if (ret != 2)
        return ret;

    return rt_i2c_master_recv(bus, addr, RT_I2C_RD, buf, count);
}
#endif

int rt_i2c_core_init(void)
{
    return 0;
}
INIT_COMPONENT_EXPORT(rt_i2c_core_init);
