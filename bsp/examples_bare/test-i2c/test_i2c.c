/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <console.h>
#include <aic_core.h>
#include <unistd.h>
#include <hal_i2c.h>

#define BUF_LEN             128
#define I2C_BASE_OFFSET     0x1000
#define SE_BUS_ID           (CLK_SE_I2C - CLK_I2C0)

#ifdef SE_I2C_BASE
#define I2C_REG_BASE(x)     (SE_I2C_BASE)
#define I2C_CLK_ID          CLK_SE_I2C
#else
#define I2C_REG_BASE(x)     (I2C0_BASE + (x * I2C_BASE_OFFSET))
#define I2C_CLK_ID          (CLK_I2C0 + x)
#endif

#define I2C_RATE            400000

aic_i2c_ctrl g_i2c_ctrl;

static void i2c_usage(void)
{
    printf("i2c write [I2C BUS ID] [slave addr] [-8 | -16] [reg] [send data]\n");
    printf("i2c read [I2C BUS ID] [slave addr] [-8 | -16] [reg]\n");
    printf("Example:\n");
    printf("16bit reg width:\n");
    printf("    write one byte: i2c write 0 0x50 -16 0x1234 0x11\n");
    printf("    read one byte: i2c read 0 0x50 -16 0x1234\n");
    printf("8bit reg width:\n");
    printf("    write one byte: i2c write 0 0x50 -8 0x00 0x11\n");
    printf("    read one byte: i2c read 0 0x50 -8 0x00\n");
    printf("tips:\n");
    printf("    if use se_i2c please input bus_id write 6\n");
}

static int32_t read_func(aic_i2c_ctrl *i2c_dev)
{
    uint32_t bytes_cnt = 0;

    bytes_cnt = hal_i2c_master_send_msg(i2c_dev, i2c_dev->msg, 1);
    if (bytes_cnt != i2c_dev->msg->len) {
        return bytes_cnt;
    }
    i2c_dev->msg->len = 1;
    bytes_cnt = hal_i2c_master_receive_msg(i2c_dev, i2c_dev->msg, 1);
    return bytes_cnt;
}

static int32_t write_func(aic_i2c_ctrl *i2c_dev)
{
    return hal_i2c_master_send_msg(i2c_dev, i2c_dev->msg, 1);
}

static int test_i2c_example(int argc, char *argv[])
{
    int bus_id = 0;
    uint16_t slave_addr;
    uint16_t data;
    uint8_t high_reg, low_reg, reg, write_data = 0;
    struct aic_i2c_msg *msgs = NULL;

    msgs = aicos_malloc(MEM_CMA, sizeof(struct aic_i2c_msg));
    if (!msgs) {
        hal_log_err("msgs malloc fail\n");
        goto __exit;
    }
    memset(msgs, 0, sizeof(struct aic_i2c_msg));

    msgs->buf = aicos_malloc(MEM_CMA, BUF_LEN);
    if (!msgs->buf) {
        hal_log_err("buf malloc fail\n");
        goto __exit;
    }
    memset(msgs->buf, 0, BUF_LEN);

    if (argc < 6) {
        i2c_usage();
        goto __exit;
    }

    bus_id = atoi(argv[2]);
    if (bus_id < 0 || bus_id > 6) {
        hal_log_err("bus id param error,please input range 0-6\n");
        goto __exit;
    }

    slave_addr = strtol(argv[3], NULL, 16);
    msgs->addr = slave_addr;

    if (strtol(argv[4], NULL, 10) == -8) {
        reg = strtol((argv[5]), NULL, 16);
        memcpy(&msgs->buf[0], &reg, 1);
        msgs->len = 1;

        if (argc == 7) {
            write_data = strtol(argv[6], NULL, 16);
            memcpy(&msgs->buf[1], &write_data, 1);
            msgs->len = 2;
        }
    } else {
        data = strtol((argv[5]), NULL, 16);
        high_reg = data >> 8;
        low_reg = data & 0xff;
        memcpy(&msgs->buf[0], &high_reg, 1);
        memcpy(&msgs->buf[1], &low_reg, 1);
        msgs->len = 2;

        if (argc == 7) {
            write_data = strtol(argv[6], NULL, 16);
            memcpy(&msgs->buf[2], &write_data, 1);
            msgs->len = 3;
        }
    }

#ifdef SE_I2C_BASE
    g_i2c_ctrl.index = SE_BUS_ID;
#else
    g_i2c_ctrl.index = bus_id;
#endif
    g_i2c_ctrl.reg_base = I2C_REG_BASE(bus_id);
    g_i2c_ctrl.msg = msgs;
    g_i2c_ctrl.addr_bit = I2C_7BIT_ADDR;
    g_i2c_ctrl.target_rate = I2C_RATE;
    g_i2c_ctrl.bus_mode = I2C_MASTER_MODE;
    g_i2c_ctrl.clk_id = I2C_CLK_ID;

    hal_i2c_init(&g_i2c_ctrl);

    if (!strcmp(argv[1], "write")) {
        if (write_func(&g_i2c_ctrl) != msgs->len) {
            printf("i2c write error\n");
            goto __exit;
        }
        printf("write_data: %#x\n", write_data);
    } else if (!strcmp(argv[1], "read")) {
        if (read_func(&g_i2c_ctrl) != 1) {
            printf("i2c read error\n");
            goto __exit;
        }
        printf("read_data: %#x\n", msgs->buf[0]);
    }

__exit:
    if (msgs != NULL) {
        if (msgs->buf != NULL) {
            aicos_free(MEM_CMA, msgs->buf);
        }
        aicos_free(MEM_CMA, msgs);
    }

    return 0;
}

CONSOLE_CMD(i2c, test_i2c_example, "i2c-tools");

