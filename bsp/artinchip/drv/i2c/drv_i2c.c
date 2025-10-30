/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <drivers/i2c.h>
#include <hal_i2c.h>

#ifndef AIC_DEV_I2C0_10BIT
#define AIC_DEV_I2C0_10BIT   0
#endif
#ifndef AIC_DEV_I2C1_10BIT
#define AIC_DEV_I2C1_10BIT   0
#endif
#ifndef AIC_DEV_I2C2_10BIT
#define AIC_DEV_I2C2_10BIT   0
#endif
#ifndef AIC_DEV_I2C3_10BIT
#define AIC_DEV_I2C3_10BIT   0
#endif
#ifndef AIC_DEV_I2C4_10BIT
#define AIC_DEV_I2C4_10BIT   0
#endif
#ifndef AIC_DEV_SP_I2C_10BIT
#define AIC_DEV_SP_I2C_10BIT 0
#endif
#ifndef AIC_DEV_R_I2C0_10BIT
#define AIC_DEV_R_I2C0_10BIT 0
#endif
#ifndef AIC_DEV_R_I2C1_10BIT
#define AIC_DEV_R_I2C1_10BIT 0
#endif

#ifndef AIC_DEV_I2C0_SPEED
#define AIC_DEV_I2C0_SPEED      400000
#endif
#ifndef AIC_DEV_I2C1_SPEED
#define AIC_DEV_I2C1_SPEED      400000
#endif
#ifndef AIC_DEV_I2C2_SPEED
#define AIC_DEV_I2C2_SPEED      400000
#endif
#ifndef AIC_DEV_I2C3_SPEED
#define AIC_DEV_I2C3_SPEED      400000
#endif
#ifndef AIC_DEV_I2C4_SPEED
#define AIC_DEV_I2C4_SPEED      400000
#endif
#ifndef AIC_DEV_SP_I2C_SPEED
#define AIC_DEV_SP_I2C_SPEED    400000
#endif
#ifndef AIC_DEV_R_I2C0_SPEED
#define AIC_DEV_R_I2C0_SPEED    400000
#endif
#ifndef AIC_DEV_R_I2C1_SPEED
#define AIC_DEV_R_I2C1_SPEED    400000
#endif

#ifndef AIC_DEV_I2C0_SLAVE_MODE
#define AIC_DEV_I2C0_SLAVE_MODE   0
#endif
#ifndef AIC_DEV_I2C1_SLAVE_MODE
#define AIC_DEV_I2C1_SLAVE_MODE   0
#endif
#ifndef AIC_DEV_I2C2_SLAVE_MODE
#define AIC_DEV_I2C2_SLAVE_MODE   0
#endif
#ifndef AIC_DEV_I2C3_SLAVE_MODE
#define AIC_DEV_I2C3_SLAVE_MODE   0
#endif
#ifndef AIC_DEV_I2C4_SLAVE_MODE
#define AIC_DEV_I2C4_SLAVE_MODE   0
#endif
#ifndef AIC_DEV_SP_I2C_SLAVE_MODE
#define AIC_DEV_SP_I2C_SLAVE_MODE 0
#endif
#ifndef AIC_DEV_R_I2C0_SLAVE_MODE
#define AIC_DEV_R_I2C0_SLAVE_MODE 0
#endif
#ifndef AIC_DEV_R_I2C1_SLAVE_MODE
#define AIC_DEV_R_I2C1_SLAVE_MODE 0
#endif

#ifndef AIC_DEV_I2C0_SLAVE_ADDR
#define AIC_DEV_I2C0_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_I2C1_SLAVE_ADDR
#define AIC_DEV_I2C1_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_I2C2_SLAVE_ADDR
#define AIC_DEV_I2C2_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_I2C3_SLAVE_ADDR
#define AIC_DEV_I2C3_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_I2C4_SLAVE_ADDR
#define AIC_DEV_I2C4_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_SP_I2C_SLAVE_ADDR
#define AIC_DEV_SP_I2C_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_R_I2C0_SLAVE_ADDR
#define AIC_DEV_R_I2C0_SLAVE_ADDR 0
#endif
#ifndef AIC_DEV_R_I2C1_SLAVE_ADDR
#define AIC_DEV_R_I2C1_SLAVE_ADDR 0
#endif

struct aic_i2c_bus {
    struct rt_i2c_bus_device bus;
    aic_i2c_ctrl aic_bus;
    struct rt_completion cmd_complete;
};

static struct aic_i2c_bus g_aic_i2c_dev[I2C_MAX_CHAN];

irqreturn_t aic_i2c_slave_irqhandler(int irq, void * data)
{
    int i, index = 0;
    uint32_t intr_stat;
    uint8_t val;
    uint8_t fifo_num;
    struct slave_param parm;
    aic_i2c_ctrl *i2c_dev;

    for (i = 0; i < I2C_MAX_CHAN; i++) {
        if (irq == g_aic_i2c_dev[i].aic_bus.irq_index) {
            index = i;
            break;
        }
    }

    i2c_dev = &g_aic_i2c_dev[index].aic_bus;
    intr_stat = hal_i2c_get_raw_interrupt_state(i2c_dev);
    fifo_num = hal_i2c_get_receive_fifo_num(i2c_dev);

    if (!i2c_dev->slave.slave_cb)
    {
        /*
         * if the slave is not registered, we need to read the fifo empty
         * to avoid interrupts from being triggered continuously
         */
        for (int i = 0; i < fifo_num; i++)
            hal_i2c_get_receive_data(i2c_dev);

        hal_i2c_clear_all_irq_flags(i2c_dev);
        pr_debug("slave callback not register!\n");
        return IRQ_NONE;
    }


    if (intr_stat & I2C_INTR_START_DET) {
        parm.cmd = I2C_SLAVE_WRITE_REQUESTED;
        parm.arg = &val;
        i2c_dev->slave.slave_cb((void *)&parm);
        hal_i2c_clear_irq_flags(i2c_dev, I2C_INTR_START_DET);
    }

    if (intr_stat & I2C_INTR_RX_FULL) {
        val = hal_i2c_get_receive_data(i2c_dev);
        parm.cmd = I2C_SLAVE_WRITE_RECEIVED;
        parm.arg = &val;
        if (!i2c_dev->slave.slave_cb((void *)&parm)) {
            pr_debug("Byte 0x%02x acked!\n", val);
        }
        hal_i2c_clear_irq_flags(i2c_dev, I2C_INTR_RX_FULL);
    }

    if (intr_stat & I2C_INTR_RD_REQ) {
        parm.cmd = I2C_SLAVE_READ_REQUESTED;
        parm.arg = &val;
        i2c_dev->slave.slave_cb((void *)&parm);
        hal_i2c_transmit_data(i2c_dev, *parm.arg);

        parm.cmd = I2C_SLAVE_READ_PROCESSED;
        parm.arg = &val;
        i2c_dev->slave.slave_cb((void *)&parm);
        hal_i2c_clear_irq_flags(i2c_dev, I2C_INTR_RD_REQ);
    }

    if (intr_stat & I2C_INTR_RX_DONE) {
        parm.cmd = I2C_SLAVE_STOP;
        parm.arg = &val;
        i2c_dev->slave.slave_cb((void *)&parm);
        hal_i2c_clear_irq_flags(i2c_dev, I2C_INTR_RX_DONE);
    }

    if (intr_stat & I2C_INTR_RX_UNDER) {
        hal_i2c_clear_irq_flags(i2c_dev, I2C_INTR_RX_UNDER);
    }

    if (intr_stat & I2C_INTR_TX_ABRT) {
        hal_i2c_clear_irq_flags(i2c_dev, I2C_INTR_TX_ABRT);
    }

    return IRQ_HANDLED;
}

#ifdef AIC_I2C_INTERRUPT_MODE
static void aic_i2c_handle_read(struct aic_i2c_ctrl *i2c_dev)
{
    struct aic_i2c_msg *msg = i2c_dev->msg;
    int rx_valid;
    uint32_t idx;

    idx = i2c_dev->index;
    rx_valid = hal_i2c_get_receive_fifo_num(i2c_dev);

    while (rx_valid--) {
        msg->buf[i2c_dev->buf_read_idx++] = hal_i2c_get_receive_data(i2c_dev);
    }

    /* message transfer done if it is a read message */
    if (i2c_dev->buf_read_idx == msg->len) {
        hal_i2c_disable_all_irq(i2c_dev);
        rt_completion_done(&g_aic_i2c_dev[idx].cmd_complete);
    }
}

static void aic_i2c_handle_write(struct aic_i2c_ctrl *i2c_dev)
{
    struct aic_i2c_msg *msg = i2c_dev->msg;
    int rx_valid, tx_valid, buf_len;
    uint32_t intr_mask = I2C_INTR_MASTER_MASK;
    uint32_t idx;

    idx = i2c_dev->index;

    if ((i2c_dev->msg_status == MSG_IDLE) && (msg->len == 0)
             && (i2c_dev->is_first_message) && (i2c_dev->is_last_message)) {
        uint32_t cmd = 0;

        cmd |= I2C_DATA_CMD_STOP;
        /* Write operation */
        hal_i2c_transmit_data_with_cmd(i2c_dev, cmd);
        hal_i2c_disable_all_irq(i2c_dev);
        rt_completion_done(&g_aic_i2c_dev[idx].cmd_complete);
    }

    i2c_dev->msg_status = MSG_IN_PROCESS;
    rx_valid = I2C_FIFO_DEPTH - hal_i2c_get_receive_fifo_num(i2c_dev);
    tx_valid = I2C_FIFO_DEPTH - hal_i2c_get_transmit_fifo_num(i2c_dev);
    buf_len = msg->len - i2c_dev->buf_write_idx;

    while (buf_len > 0 && rx_valid > 0 && tx_valid > 0) {
        uint32_t cmd = 0;

        if (buf_len == 1 && i2c_dev->is_last_message) {
            cmd |= I2C_DATA_CMD_STOP;
        }

        if (!i2c_dev->is_first_message && !i2c_dev->buf_write_idx) {
            cmd |= I2C_DATA_CMD_RESTART;
        }

        if (msg->flags & RT_I2C_RD) {
            cmd |= I2C_DATA_CMD_READ;
            hal_i2c_transmit_data_with_cmd(i2c_dev, cmd);
            rx_valid--;
        } else {
            hal_i2c_transmit_data_with_cmd(i2c_dev, cmd | msg->buf[i2c_dev->buf_write_idx]);
            /* ensure the data is sent out */
            while (hal_i2c_get_transmit_fifo_num(i2c_dev) != 0) {};
        }

        i2c_dev->buf_write_idx++;
        tx_valid--;
        buf_len--;

        if (i2c_dev->buf_write_idx == msg->len) {
            intr_mask &= ~I2C_INTR_TX_EMPTY;
            hal_i2c_flags_mask(i2c_dev, intr_mask);
            /* message transfer done if it is a write message */
            if (!(msg->flags & RT_I2C_RD)) {
                hal_i2c_disable_all_irq(i2c_dev);
                rt_completion_done(&g_aic_i2c_dev[idx].cmd_complete);
            }
        }
    }

}

irqreturn_t aic_i2c_irqhandler(int irq, void * data)
{
    int i, index = 0;
    uint32_t status = 0;
    aic_i2c_ctrl *i2c_dev;

    for (i = 0; i < I2C_MAX_CHAN; i++) {
        if (irq == g_aic_i2c_dev[i].aic_bus.irq_index) {
            index = i;
            break;
        }
    }

    i2c_dev = &g_aic_i2c_dev[index].aic_bus;
    status = hal_i2c_get_raw_interrupt_state(i2c_dev);
    /* clear all interrupt flags */
    hal_i2c_clear_all_irq_flags(i2c_dev);

    if (status & I2C_INTR_TX_ABRT) {
        i2c_dev->msg_err = -I2C_INTR_ERROR_ABRT;
        goto i2c_err;
    }

    if (status & I2C_INTR_RX_UNDER) {
        i2c_dev->msg_err = -I2C_INTR_ERROR_RX;
        goto i2c_err;
    }

    if (status & I2C_INTR_RX_FULL) {
        aic_i2c_handle_read(i2c_dev);
    }

    if (status & I2C_INTR_TX_EMPTY) {
        aic_i2c_handle_write(i2c_dev);
    }

i2c_err:
    if (status & (I2C_INTR_TX_ABRT | I2C_INTR_RX_UNDER | I2C_INTR_STOP_DET)) {
        hal_i2c_disable_all_irq(i2c_dev);
        rt_completion_done(&g_aic_i2c_dev[index].cmd_complete);
    }

    return IRQ_HANDLED;
}

static void aic_i2c_xfer_msg_init(struct aic_i2c_ctrl *i2c_dev)
{
    i2c_dev->buf_write_idx = 0;
    i2c_dev->buf_read_idx = 0;
    i2c_dev->msg_err = 0;
    i2c_dev->abort_source = 0;
    i2c_dev->msg_status = MSG_IDLE;

    /* clear and enable interrupts */
    hal_i2c_clear_all_irq_flags(i2c_dev);
    hal_i2c_master_enable_irq(i2c_dev);
}

static int aic_i2c_xfer_msg(struct aic_i2c_ctrl *i2c_dev, struct aic_i2c_msg *msg,
                            bool is_first, bool is_last)
{
    int ret = 0;
    rt_err_t timeout;
    uint32_t idx;

    i2c_dev->msg = msg;
    i2c_dev->is_first_message = is_first;
    i2c_dev->is_last_message = is_last;
    idx = i2c_dev->index;

    rt_completion_init(&g_aic_i2c_dev[idx].cmd_complete);
    if (is_first) {
        /* Set the slave address */
        hal_i2c_target_addr(i2c_dev, msg->addr);
        /* Enable i2c dev */
        hal_i2c_module_enable(i2c_dev);

        ret = hal_i2c_wait_bus_free(i2c_dev, 10);
        if (ret)
            return ret;
    }

    aic_i2c_xfer_msg_init(i2c_dev);

    timeout = rt_completion_wait(&g_aic_i2c_dev[idx].cmd_complete, 1000);
    if (timeout) {
        pr_err("message xfer timeout\n");
        ret = I2C_TIMEOUT;
    }

    if (i2c_dev->msg_err) {
        ret = I2C_ERR;
    }

    return ret;
}

static rt_size_t aic_i2c_master_xfer(struct rt_i2c_bus_device *bus,
                                     struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    CHECK_PARAM(bus, -EINVAL);

    aic_i2c_ctrl *i2c_dev;
    int ret = 0, index;
    uint8_t idx;

    idx = bus->parent.device_id;
    i2c_dev = &g_aic_i2c_dev[idx].aic_bus;

    for (index = 0; index < num; index++) {
        i2c_dev->msg = (struct aic_i2c_msg*)msgs;
        ret = aic_i2c_xfer_msg(i2c_dev, &i2c_dev->msg[index], index == 0, index == (num-1));
    }
    hal_i2c_module_disable(i2c_dev);

    return (ret < 0) ? 0 : num;
}
#else
static rt_size_t aic_i2c_master_xfer(struct rt_i2c_bus_device *bus,
                                     struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    CHECK_PARAM(bus, -EINVAL);

    struct rt_i2c_msg *msg = NULL;
    aic_i2c_ctrl *i2c_dev;
    int ret_msg_len = 0;
    int32_t bytes_cnt = 0;
    int8_t is_last_message = 0;
    uint8_t index;

    index = bus->parent.device_id;
    i2c_dev = &g_aic_i2c_dev[index].aic_bus;

    for (uint32_t i = 0; i < num; i++) {
        msg = &msgs[i];
        if (i == num -1)
            is_last_message = 1;
        else
            is_last_message = 0;

        if ((msg->flags & RT_I2C_RD)) {
            bytes_cnt = hal_i2c_master_receive_msg(i2c_dev,
                            (struct aic_i2c_msg*)msg, is_last_message);
        } else {
            bytes_cnt = hal_i2c_master_send_msg(i2c_dev,
                            (struct aic_i2c_msg*)msg, is_last_message);
        }

        if (bytes_cnt == msg->len) {
            ret_msg_len++;
        }
    }

    return ret_msg_len;
}
#endif

static rt_err_t  aic_i2c_slave_control(struct rt_i2c_bus_device *bus,
                                        rt_uint32_t cmd, void *arg)
{
    RT_ASSERT(bus != RT_NULL);

    aic_i2c_ctrl *i2c_dev;

    uint8_t index = bus->parent.device_id;
    i2c_dev = &g_aic_i2c_dev[index].aic_bus;

    struct aic_i2c_slave_info *slave_info = (struct aic_i2c_slave_info *)arg;

    i2c_dev->slave.slave_cb = slave_info->slave_cb;
    i2c_dev->slave.callback_param = slave_info->callback_param;

    return RT_EOK;
}

static rt_err_t aic_i2c_bus_control(struct rt_i2c_bus_device *bus,
                                rt_uint32_t cmd, rt_uint32_t value)
{
    RT_ASSERT(bus != RT_NULL);

    aic_i2c_ctrl *i2c_dev;

    uint8_t index = bus->parent.device_id;
    i2c_dev = &g_aic_i2c_dev[index].aic_bus;

    switch(cmd)
    {
    case RT_I2C_DEV_CTRL_CLK:
        i2c_dev->target_rate = value;
        hal_i2c_set_speed(i2c_dev);
        break;
    default:
        return -RT_EIO;
    }

    return RT_EOK;
}

const struct rt_i2c_bus_device_ops i2c_ops = {
    aic_i2c_master_xfer,
    RT_NULL,
    aic_i2c_bus_control,
    aic_i2c_slave_control,
};

static struct aic_i2c_bus aic_i2c_dev[] = {
#ifdef AIC_USING_I2C0
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 0,
            .reg_base = I2C0_BASE,
            .device_name = "i2c0",
            .addr_bit = AIC_DEV_I2C0_10BIT,
            .target_rate = AIC_DEV_I2C0_SPEED,
            .bus_mode = AIC_DEV_I2C0_SLAVE_MODE,
            .slave_addr = AIC_DEV_I2C0_SLAVE_ADDR,
            .irq_index = I2C0_IRQn,
            .clk_id = CLK_I2C0,
        },
    },
#endif

#ifdef AIC_USING_I2C1
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 1,
            .reg_base = I2C1_BASE,
            .device_name = "i2c1",
            .addr_bit = AIC_DEV_I2C1_10BIT,
            .target_rate = AIC_DEV_I2C1_SPEED,
            .bus_mode = AIC_DEV_I2C1_SLAVE_MODE,
            .slave_addr = AIC_DEV_I2C1_SLAVE_ADDR,
            .irq_index = I2C1_IRQn,
            .clk_id = CLK_I2C1,
        },
    },
#endif

#ifdef AIC_USING_I2C2
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 2,
            .reg_base = I2C2_BASE,
            .device_name = "i2c2",
            .addr_bit = AIC_DEV_I2C2_10BIT,
            .target_rate = AIC_DEV_I2C2_SPEED,
            .bus_mode = AIC_DEV_I2C2_SLAVE_MODE,
            .slave_addr = AIC_DEV_I2C2_SLAVE_ADDR,
            .irq_index = I2C2_IRQn,
            .clk_id = CLK_I2C2,
        },
    },
#endif

#ifdef AIC_USING_I2C3
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 3,
            .reg_base = I2C3_BASE,
            .device_name = "i2c3",
            .addr_bit = AIC_DEV_I2C3_10BIT,
            .target_rate = AIC_DEV_I2C3_SPEED,
            .bus_mode = AIC_DEV_I2C3_SLAVE_MODE,
            .slave_addr = AIC_DEV_I2C3_SLAVE_ADDR,
            .irq_index = I2C3_IRQn,
            .clk_id = CLK_I2C3,
        },
    },
#endif

#ifdef AIC_USING_I2C4
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 4,
            .reg_base = I2C4_BASE,
            .device_name = "i2c4",
            .addr_bit = AIC_DEV_I2C4_10BIT,
            .target_rate = AIC_DEV_I2C4_SPEED,
            .bus_mode = AIC_DEV_I2C4_SLAVE_MODE,
            .slave_addr = AIC_DEV_I2C4_SLAVE_ADDR,
            .irq_index = I2C4_IRQn,
            .clk_id = CLK_I2C4,
        },
    },
#endif

#ifdef AIC_USING_SP_I2C
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 5,
            .reg_base = I2C_BASE,
            .device_name = "sp_i2c",
            .addr_bit = AIC_DEV_SP_I2C_10BIT,
            .target_rate = AIC_DEV_SP_I2C_SPEED,
            .bus_mode = AIC_DEV_SP_I2C_SLAVE_MODE,
            .slave_addr = AIC_DEV_SP_I2C_SLAVE_ADDR,
            .irq_index = I2C_IRQn,
            .clk_id = CLK_SP_I2C,
        },
    },
#endif
#ifdef AIC_USING_R_I2C0
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 6,
            .reg_base = R_I2C0_BASE,
            .device_name = "r_i2c0",
            .addr_bit = AIC_DEV_R_I2C0_10BIT,
            .target_rate = AIC_DEV_R_I2C0_SPEED,
            .bus_mode = AIC_DEV_R_I2C0_SLAVE_MODE,
            .slave_addr = AIC_DEV_R_I2C0_SLAVE_ADDR,
            .irq_index = R_I2C0_IRQn,
            .clk_id = CLK_R_I2C0,
        },
    },
#endif
#ifdef AIC_USING_R_I2C1
    {
        .bus.ops = &i2c_ops,
        .aic_bus = {
            .index = 7,
            .reg_base = R_I2C1_BASE,
            .device_name = "r_i2c1",
            .addr_bit = AIC_DEV_R_I2C1_10BIT,
            .target_rate = AIC_DEV_R_I2C1_SPEED,
            .bus_mode = AIC_DEV_R_I2C1_SLAVE_MODE,
            .slave_addr = AIC_DEV_R_I2C1_SLAVE_ADDR,
            .irq_index = R_I2C1_IRQn,
            .clk_id = CLK_R_I2C1,
        },
    },
#endif
};

static int aic_hw_i2c_register()
{
    int ret = -1;
    uint8_t index;

    for (uint8_t i = 0; i < ARRAY_SIZE(aic_i2c_dev); i++) {
        index = aic_i2c_dev[i].aic_bus.index;
        aic_i2c_dev[i].bus.parent.device_id = index;

        ret = hal_i2c_init(&aic_i2c_dev[i].aic_bus);
        if (ret) {
            return ret;
        }
        g_aic_i2c_dev[index] = aic_i2c_dev[i];

#ifdef AIC_I2C_INTERRUPT_MODE
        aicos_request_irq(aic_i2c_dev[i].aic_bus.irq_index, aic_i2c_irqhandler, 0, "i2c", NULL);
#endif
        if (aic_i2c_dev[i].aic_bus.bus_mode) {
            aicos_request_irq(aic_i2c_dev[i].aic_bus.irq_index, aic_i2c_slave_irqhandler, 0, "i2c_slave", NULL);
        }

        ret = rt_i2c_bus_device_register(&aic_i2c_dev[i].bus,
                                         aic_i2c_dev[i].aic_bus.device_name);
        if (ret) {
            return ret;
        }
    }
    return 0;
}
INIT_BOARD_EXPORT(aic_hw_i2c_register);
