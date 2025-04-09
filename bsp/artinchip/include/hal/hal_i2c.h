/*
 * Copyright (c) 2022-2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_I2C_
#define _AIC_HAL_I2C_

#include <stdint.h>
#include <stdbool.h>
#include "aic_errno.h"
#include "aic_common.h"
#include "aic_hal_clk.h"
#include "aic_log.h"
#include "aic_io.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aic_i2c_msg
{
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t  *buf;
};

enum aic_msg_status {
    MSG_IDLE        = 0,
    MSG_IN_PROCESS  = 1,
};

struct slave_param
{
    uint32_t cmd;
    uint8_t *arg;
};

typedef int (*i2c_slave_cb_t) (void *callback_param);

struct aic_i2c_slave_info
{
    void *callback_param;
    i2c_slave_cb_t slave_cb;
};

typedef struct aic_i2c_ctrl aic_i2c_ctrl;

struct aic_i2c_ctrl
{
    int32_t index;
    char *device_name;
    unsigned long reg_base;
    uint32_t addr_bit;
    uint32_t target_rate;
    uint32_t bus_mode;
    struct aic_i2c_msg *msg;
    struct aic_i2c_slave_info slave;
    enum aic_msg_status msg_status;
    uint32_t slave_addr;
    uint32_t abort_source;
    uint32_t msg_err;
    uint32_t buf_write_idx;
    uint32_t buf_read_idx;
    uint32_t irq_index;
    uint32_t clk_id;
    uint32_t module_clk;
    bool is_first_message;
    bool is_last_message;
};

typedef enum {
    I2C_OK = 0,
    I2C_ERR = -1,
    I2C_BUSY = -2,
    I2C_TIMEOUT = -3,
    I2C_UNSPUPPORTED = -4,
} i2c_error_t;

enum i2c_slave_event {
    I2C_SLAVE_READ_REQUESTED,
    I2C_SLAVE_WRITE_REQUESTED,
    I2C_SLAVE_READ_PROCESSED,
    I2C_SLAVE_WRITE_RECEIVED,
    I2C_SLAVE_STOP,
};

#define I2C_CTL               0x00
#define I2C_TAR               0x04
#define I2C_SAR               0x08
#define I2C_ACK_GEN_CALL      0x0C
#define I2C_DATA_CMD          0x10
#define I2C_SS_SCL_HCNT       0x20
#define I2C_SS_SCL_LCNT       0x24
#define I2C_FS_SCL_HCNT       0x28
#define I2C_FS_SCL_LCNT       0x2C
#define I2C_SDA_HOLD          0x30
#define I2C_SDA_SETUP         0x34
#define I2C_INTR_MASK         0x38
#define I2C_INTR_CLR          0x3C
#define I2C_INTR_RAW_STAT     0x40
#define I2C_ENABLE            0x48
#define I2C_ENABLE_STATUS     0x4C
#define I2C_STATUS            0x50
#define I2C_TX_ABRT_SOURCE    0x54
#define I2C_RX_TL             0x90
#define I2C_TX_TL             0x94
#define I2C_TXFLR             0x98
#define I2C_RXFLR             0x9C
#define I2C_SCL_STUCK_TIMEOUT 0xA0
#define I2C_SDA_STUCK_TIMEOUT 0xA4
#define I2C_FS_SPIKELEN       0xB0
#define I2C_VERSION           0xFC

#define I2C_CTL_MASTER_SLAVE_SELECT_MASK (3)
#define I2C_CTL_10BIT_SELECT_MASTER      (1 << 2)
#define I2C_CTL_10BIT_SELECT_SLAVE       (1 << 3)
#define I2C_CTL_SPEED_MODE_SELECT_MASK   (3 << 4)
#define I2C_CTL_SPEED_MODE_SS            (0x1 << 4)
#define I2C_CTL_SPEED_MODE_FS            (0x2 << 4)
#define I2C_CTL_RESTART_ENABLE           (1 << 6)
#define I2C_CTL_STOP_DET_IFADDR          (1 << 7)
#define I2C_CTL_TX_EMPTY_CTL             (1 << 8)
#define I2C_CTL_RX_FIFO_FULL_HLD         (1 << 9)
#define I2C_CTL_BUS_CLEAR_FEATURE        (1 << 10)

#define I2C_TAR_ADDR_MASK    (0x3FF)
#define I2C_TAR_START_BYTE   (1 << 10)
#define I2C_TAR_GEN_CALL_CTL (1 << 11)

#define I2C_DATA_CMD_DAT_MASK (0xFF)
#define I2C_DATA_CMD_READ     (1 << 8)
#define I2C_DATA_CMD_STOP     (1 << 9)
#define I2C_DATA_CMD_RESTART  (1 << 10)

#define I2C_SDA_TX_HOLD_MASK (0xFFFF)
#define I2C_SDA_RX_HOLD_MASK (0xFF << 16)

#define I2C_INTR_RX_UNDER         (1 << 0)
#define I2C_INTR_RX_FULL          (1 << 2)
#define I2C_INTR_TX_EMPTY         (1 << 4)
#define I2C_INTR_RD_REQ           (1 << 5)
#define I2C_INTR_TX_ABRT          (1 << 6)
#define I2C_INTR_RX_DONE          (1 << 7)
#define I2C_INTR_ACTIVITY         (1 << 8)
#define I2C_INTR_STOP_DET         (1 << 9)
#define I2C_INTR_START_DET        (1 << 10)
#define I2C_INTR_GEN_CALL         (1 << 11)
#define I2C_INTR_MASTER_ON_HOLD   (1 << 13)
#define I2C_INTR_SCL_STUCK_AT_LOW (1 << 14)

#define I2C_ENABLE_BIT                (1 << 0)
#define I2C_ENABLE_ABORT              (1 << 1)
#define I2C_TX_CMD_BLOCK              (1 << 2)
#define I2C_SDA_STUCK_RECOVERY_ENABLE (1 << 3)

#define I2C_STATUS_ACTIVITY (1 << 0)

#define ABRT_7BIT_ADDR_NOACK   0
#define ABRT_10BIT_ADDR1_NOACK 1
#define ABRT_10BIT_ADDR2_NOACK 2
#define ABRT_TXDATA_NOACK      3
#define ABRT_GCALL_NOACK       4
#define ABRT_GCALL_READ        5
#define ABRT_SBYTE_ACKDET      7
#define ABRT_SBYTE_NORSTRT     9
#define ABRT_10BIT_RD_NORSTRT  10
#define ABRT_MASTER_DIS        11
#define ABRT_LOST              12
#define ABRT_SLVFLUSH_TXFIFO   13
#define ABRT_SLV_ARBLOST       14
#define ABRT_SLVRD_INTX        15
#define ABRT_USER_ABRT         16
#define ABRT_SDA_STUCK_AT_LOW  17

#define I2C_ABRT_7BIT_ADDR_NOACK   (1 << 0)
#define I2C_ABRT_10BIT_ADDR1_NOACK (1 << 1)
#define I2C_ABRT_10BIT_ADDR2_NOACK (1 << 2)
#define I2C_ABRT_TXDATA_NOACK      (1 << 3)
#define I2C_ABRT_GCALL_NOACK       (1 << 4)
#define I2C_ABRT_GCALL_READ        (1 << 5)
#define I2C_ABRT_SBYTE_ACKDET      (1 << 7)
#define I2C_ABRT_SBYTE_NORSTRT     (1 << 9)
#define I2C_ABRT_10BIT_RD_NORSTRT  (1 << 10)
#define I2C_ABRT_MASTER_DIS        (1 << 11)
#define I2C_ABRT_LOST              (1 << 12)
#define I2C_ABRT_SLVFLUSH_TXFIFO   (1 << 13)
#define I2C_ABRT_SLV_ARBLOST       (1 << 14)
#define I2C_ABRT_SLVRD_INTX        (1 << 15)
#define I2C_ABRT_USER_ABRT         (1 << 16)
#define I2C_ABRT_SDA_STUCK_AT_LOW  (1 << 17)

#define I2C_ENABLE_MASTER_DISABLE_SLAVE (0x3)

#define I2C_FIFO_DEPTH          8
#define I2C_TXFIFO_THRESHOLD    (I2C_FIFO_DEPTH / 2 - 1)
#define I2C_RXFIFO_THRESHOLD    0

#define I2C_TIMEOUT_DEF_VAL     1000
#define I2C_INTR_ERROR_RX       0x0001
#define I2C_INTR_ERROR_ABRT     0x0002

#define I2C_MASTER_MODE         0
#define I2C_SLAVE_MODE          1
#define I2C_7BIT_ADDR           0
#define I2C_10BIT_ADDR          1

#define I2C_MAX_CHAN            8

#define I2C_INTR_MASTER_MASK    (I2C_INTR_RX_UNDER |\
                        I2C_INTR_RX_FULL |\
                        I2C_INTR_TX_EMPTY |\
                        I2C_INTR_TX_ABRT |\
                        I2C_INTR_STOP_DET)

#define I2C_INTR_SLAVE_MASK     (I2C_INTR_RX_UNDER |\
                        I2C_INTR_RX_FULL |\
                        I2C_INTR_RD_REQ |\
                        I2C_INTR_TX_ABRT |\
                        I2C_INTR_RX_DONE |\
                        I2C_INTR_START_DET)

static inline void hal_i2c_module_enable(aic_i2c_ctrl *i2c_dev)
{
    uint32_t reg_val;

    reg_val = readl(i2c_dev->reg_base + I2C_CTL);
    reg_val |= I2C_CTL_RESTART_ENABLE;
    writel(reg_val, i2c_dev->reg_base + I2C_CTL);

    reg_val = readl(i2c_dev->reg_base + I2C_ENABLE);
    reg_val |= I2C_ENABLE_BIT | I2C_SDA_STUCK_RECOVERY_ENABLE;
    writel(reg_val, i2c_dev->reg_base + I2C_ENABLE);
}

static inline void hal_i2c_module_disable(aic_i2c_ctrl *i2c_dev)
{
    uint32_t reg_val;

    writel(0x100, i2c_dev->reg_base + I2C_INTR_CLR);
    reg_val = readl(i2c_dev->reg_base + I2C_ENABLE);
    reg_val &= ~I2C_ENABLE_BIT;
    writel(reg_val, i2c_dev->reg_base + I2C_ENABLE);
}

static inline unsigned long hal_i2c_module_status(aic_i2c_ctrl *i2c_dev)
{
    return readl(i2c_dev->reg_base + I2C_ENABLE_STATUS) & 1;
}

static inline void hal_i2c_transmit_data(aic_i2c_ctrl *i2c_dev, uint8_t data)
{
    writel(data, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline void hal_i2c_transmit_data_with_cmd(aic_i2c_ctrl *i2c_dev,
                                                  unsigned long data)
{
    writel(data, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline void hal_i2c_transmit_data_with_stop_bit(aic_i2c_ctrl *i2c_dev,
                                                       uint8_t data)
{
    uint32_t reg_val;

    reg_val = I2C_DATA_CMD_STOP | data;
    writel(reg_val, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline unsigned long
hal_i2c_get_transmit_fifo_num(aic_i2c_ctrl *i2c_dev)
{
    return readl(i2c_dev->reg_base + I2C_TXFLR);
}

static inline void hal_i2c_transfer_stop_bit(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_DATA_CMD_STOP, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline void hal_i2c_read_data_cmd(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_DATA_CMD_READ, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline void hal_i2c_read_data_cmd_with_stop_bit(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_DATA_CMD_READ | I2C_DATA_CMD_STOP, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline unsigned long hal_i2c_get_receive_fifo_num(aic_i2c_ctrl *i2c_dev)
{
    return readl(i2c_dev->reg_base + I2C_RXFLR);
}

static inline uint8_t hal_i2c_get_receive_data(aic_i2c_ctrl *i2c_dev)
{
    return readb(i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline void
hal_i2c_read_data_cmd_with_restart_stop_bit(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_DATA_CMD_READ | I2C_DATA_CMD_STOP | I2C_DATA_CMD_RESTART,
           i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline void
hal_i2c_read_data_cmd_with_restart_bit(aic_i2c_ctrl *i2c_dev)
{
    uint32_t reg_val;

    reg_val = readl(i2c_dev->reg_base + I2C_CTL);
    reg_val |= I2C_CTL_RESTART_ENABLE;
    writel(reg_val, i2c_dev->reg_base + I2C_CTL);
    writel(I2C_DATA_CMD_READ | I2C_DATA_CMD_RESTART, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline unsigned long hal_i2c_set_read_cmd(unsigned long reg_val)
{
    return (reg_val | I2C_DATA_CMD_READ);
}

static inline unsigned long hal_i2c_set_stop_bit(unsigned long reg_val)
{
    return (reg_val | I2C_DATA_CMD_STOP);
}

static inline unsigned long hal_i2c_set_restart_bit(unsigned long reg_val)
{
    return (reg_val | I2C_DATA_CMD_RESTART);
}

static inline void hal_i2c_set_restart_bit_with_data(aic_i2c_ctrl *i2c_dev,
                                                     uint8_t data)
{
    uint32_t reg_val;

    reg_val = readl(i2c_dev->reg_base + I2C_CTL);
    reg_val |= I2C_CTL_RESTART_ENABLE;
    writel(reg_val, i2c_dev->reg_base + I2C_CTL);
    writel(data | I2C_DATA_CMD_RESTART, i2c_dev->reg_base + I2C_DATA_CMD);
}

static inline unsigned long
hal_i2c_get_raw_interrupt_state(aic_i2c_ctrl *i2c_dev)
{
    return readl(i2c_dev->reg_base + I2C_INTR_RAW_STAT);
}

static inline unsigned long hal_i2c_get_interrupt_state(aic_i2c_ctrl *i2c_dev)
{
    return readl(i2c_dev->reg_base + I2C_INTR_CLR);
}

static inline void hal_i2c_disable_all_irq(aic_i2c_ctrl *i2c_dev)
{
    writel(0, i2c_dev->reg_base + I2C_INTR_MASK);
}

static inline void hal_i2c_clear_irq_flags(aic_i2c_ctrl *i2c_dev,
                                           unsigned long flags)
{
    writel(flags, i2c_dev->reg_base + I2C_INTR_CLR);
}

static inline void hal_i2c_clear_all_irq_flags(aic_i2c_ctrl *i2c_dev)
{
    writel(0xffff, i2c_dev->reg_base + I2C_INTR_CLR);
}

static inline void hal_i2c_clear_rx_full_flag(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_INTR_RX_FULL, i2c_dev->reg_base + I2C_INTR_CLR);
}

static inline void hal_i2c_clear_tx_empty_flag(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_INTR_TX_EMPTY, i2c_dev->reg_base + I2C_INTR_CLR);
}

static inline void hal_i2c_master_enable_irq(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_INTR_MASTER_MASK, i2c_dev->reg_base + I2C_INTR_MASK);
}

static inline void hal_i2c_slave_enable_irq(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_INTR_SLAVE_MASK, i2c_dev->reg_base + I2C_INTR_MASK);
}

static inline void hal_i2c_set_transmit_fifo_threshold(aic_i2c_ctrl *i2c_dev)
{
    writel(I2C_TXFIFO_THRESHOLD, i2c_dev->reg_base + I2C_TX_TL);
}

static inline void hal_i2c_set_receive_fifo_threshold(aic_i2c_ctrl *i2c_dev,
                                                      uint8_t level)
{
    writel(level - 1, i2c_dev->reg_base + I2C_RX_TL);
}

static inline void hal_i2c_config_fifo_slave(aic_i2c_ctrl *i2c_dev)
{
    writel(0, i2c_dev->reg_base + I2C_TX_TL);
    writel(0, i2c_dev->reg_base + I2C_RX_TL);
}

static inline int hal_i2c_bus_status(aic_i2c_ctrl *i2c_dev)
{
    uint32_t status;

    status = (readl(i2c_dev->reg_base + I2C_STATUS) & I2C_STATUS_ACTIVITY);
    return status;
}

static inline void hal_i2c_flags_mask(aic_i2c_ctrl *i2c_dev, unsigned long flags)
{
    writel(flags, i2c_dev->reg_base + I2C_INTR_MASK);
}

static inline void hal_i2c_enable_sda_scl_stuck_timeout_restore(aic_i2c_ctrl *i2c_dev)
{
    uint32_t reg_val;

    reg_val = readl(i2c_dev->reg_base + I2C_CTL);
    reg_val |= I2C_CTL_BUS_CLEAR_FEATURE;
    writel(reg_val, i2c_dev->reg_base + I2C_CTL);

    reg_val = readl(i2c_dev->reg_base + I2C_ENABLE);
    reg_val |= I2C_SDA_STUCK_RECOVERY_ENABLE;
    writel(reg_val, i2c_dev->reg_base + I2C_ENABLE);
}

static inline void hal_i2c_set_sda_scl_stuck_time(aic_i2c_ctrl *i2c_dev)
{
    writel(0x1000, i2c_dev->reg_base + I2C_SCL_STUCK_TIMEOUT);
    writel(0x1000, i2c_dev->reg_base + I2C_SDA_STUCK_TIMEOUT);
}

int hal_i2c_init(aic_i2c_ctrl *i2c_dev);
int hal_i2c_clk_init(aic_i2c_ctrl *i2c_dev);
int hal_i2c_set_master_slave_mode(aic_i2c_ctrl *i2c_dev);
int hal_i2c_master_10bit_addr(aic_i2c_ctrl *i2c_dev);
int hal_i2c_slave_10bit_addr(aic_i2c_ctrl *i2c_dev);
int hal_i2c_slave_own_addr(aic_i2c_ctrl *i2c_dev, uint32_t addr);
void hal_i2c_target_addr(aic_i2c_ctrl *i2c_dev, uint32_t addr);
void hal_i2c_set_hold(aic_i2c_ctrl *i2c_dev, u32 val);
int hal_i2c_set_speed(aic_i2c_ctrl *i2c_dev);
int32_t hal_i2c_wait_bus_free(aic_i2c_ctrl *i2c_dev, uint32_t timeout);
int32_t hal_i2c_master_send_msg(aic_i2c_ctrl *i2c_dev,
                                struct aic_i2c_msg *msg, uint8_t is_last_message);
int32_t hal_i2c_master_receive_msg(aic_i2c_ctrl *i2c_dev,
                                struct aic_i2c_msg *msg, uint8_t is_last_message);

#ifdef __cplusplus
}
#endif

#endif /* _AIC_HAL_I2C_ */
