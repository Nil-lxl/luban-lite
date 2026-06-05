/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Siyao Li <siyao.li@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>
#include <finsh.h>
#include <getopt.h>
#include "hal_canfd.h"

#define CANFD_INPUT_FRAME_DATA    "11.22.33.44.55.66.77.88.99.11.22.33.44.55.66.77.88.99.11.22"
#define CANFD_INPUT_FRAME_ID      0x1a3
/*
 * This program is used to test CANFD selftest mode.
 */
static struct rt_semaphore g_rx_sem;
static rt_device_t g_canfd_dev;
static char g_canfd_dev_name[7];
static u8 g_canfd_dma_rx_buf[64] __attribute__((aligned(CACHE_LINE_SIZE)));
static u8 g_canfd_dma_tx_buf[64] __attribute__((aligned(CACHE_LINE_SIZE)));

static uint16_t generate_data(const char *str, uint8_t *index)
{
    static const char hex_chars[] = "0123456789abcdef";
    uint16_t value = 0;
    int digit_count = 0;

    while (str[*index] == '#' || str[*index] == '.') {
        (*index)++;
    }

    while (str[*index] != '\0' && str[*index] != '#' && str[*index] != '.') {
        char c = str[*index];
        const char *pos = strchr(hex_chars, c);
        if (pos != NULL) {
            value = (value << 4) | (pos - hex_chars);
            digit_count++;
        } else {
            break;
        }
        (*index)++;
    }

    if (digit_count == 0) {
        return 0;
    }
    return value;
}

static rt_err_t canfd_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&g_rx_sem);

    return RT_EOK;
}

bool test_canfd_is_valid_data_length(unsigned int length) {
    for (unsigned int i = 0; i < 16; ++i) {
        if (canfd_data_lengths[i] == length) {
            return true;
        }
    }
    return false;
}

static void canfd_rx_thread(void *parameter)
{
    int i;
    rt_size_t size;
    struct rt_can_msg rxmsg = {0};

    rt_sem_take(&g_rx_sem, RT_WAITING_FOREVER);

    rxmsg.hdr = -1;
    size = rt_device_read(g_canfd_dev, 0, &rxmsg, sizeof(rxmsg));
    if (!size)
    {
        rt_kprintf("CAN read error\n");
        goto __exit_canfd_rx;
    }

    rt_kprintf("%s received msg:\nID: 0x%x ", g_canfd_dev_name, rxmsg.id);

    if (rxmsg.len)
        rt_kprintf("DATA: ");
    for (i = 0; i < rxmsg.len; i++)
    {
        rt_kprintf("%02x ", rxmsg.data[i]);
    }

    rt_kprintf("\n");

__exit_canfd_rx:
    rt_sem_detach(&g_rx_sem);
    rt_device_close(g_canfd_dev);
}

static void parse_msg_data(rt_can_msg_t msg, char * optarg, int can_or_canfd)
{
    char *token;
    uint8_t i = 0, id_received = 0;

    token = strtok(optarg, "#.");

    while (token)
    {
        if (!id_received)
        {
            msg->id = strtoul(token, NULL, 16);
            if (msg->id > 0x7FF)
                msg->ide = 1;
            else
                msg->ide = 0;

            if (can_or_canfd == CANFD_TYPE) {
                msg->fd_frame = 1;
            }

            id_received = 1;
        }
        else
        {
            /* frame data */
            if (can_or_canfd == CAN_TYPE) {
                /* for CAN frame, CAN_FRAME_TYPE_DATA or CAN_FRAME_TYPE_REMOTE */
                msg->rtr = CAN_FRAME_TYPE_DATA;
                if (i >= CANFD_CAN_FRAME_MAX_BYTES) {
                    rt_kprintf("CAN only support %d bytes data, current data size is %d\n",
                               CANFD_CAN_FRAME_MAX_BYTES, msg->len);
                    break;
                }
            } else {
                /* for CANFD frame,  Only be CAN_FRAME_TYPE_DATA */
                msg->rtr = CAN_FRAME_TYPE_DATA;
            }
            msg->data[i++] = strtoul(token, NULL, 16);
        }

        token = strtok(NULL, "#.");
    }

    msg->len = i;
    rt_kprintf("Send data length: %d\n", msg->len);
    if (!test_canfd_is_valid_data_length(msg->len))
        rt_kprintf("Length is invalid\n", msg->len);
}

static void canfd_dma_tx_cb(void *arg)
{
    rt_kprintf("canfd dma tx callback\n");
}

static void canfd_dma_rx_cb(void *arg)
{
    rt_kprintf("canfd dma rx callback\n");
}

static void test_canfd_config_dma()
{
    struct canfd_dma_transfer_info chan_info_tx;
    struct canfd_dma_transfer_info chan_info_rx;

    chan_info_tx.buf = g_canfd_dma_tx_buf;
    chan_info_tx.buf_size = 16;
    chan_info_tx.callback = canfd_dma_tx_cb;
    chan_info_tx.callback_param = NULL;
    rt_device_control(g_canfd_dev, RT_CAN_CONFIG_DMA_TX, &chan_info_tx);

    chan_info_rx.buf = g_canfd_dma_rx_buf;
    chan_info_rx.buf_size = 16;
    chan_info_rx.callback = canfd_dma_rx_cb;
    chan_info_rx.callback_param = NULL;
    rt_device_control(g_canfd_dev, RT_CAN_CONFIG_DMA_RX, &chan_info_rx);
}

static void parse_long_msg_data(rt_can_msg_t msg, int frame_len, int can_or_canfd)
{
    if (msg->id > 0x7FF)
        msg->ide = 1;
    else
        msg->ide = 0;

    if (can_or_canfd == CANFD_TYPE) {
        msg->fd_frame = 1;
    }

    /* frame data */
    if (can_or_canfd == CAN_TYPE) {
        /* for CAN frame, CAN_FRAME_TYPE_DATA or CAN_FRAME_TYPE_REMOTE */
        msg->rtr = CAN_FRAME_TYPE_DATA;
        if (frame_len >= CANFD_CAN_FRAME_MAX_BYTES) {
            rt_kprintf("CAN only support %d bytes data, current data size is %d\n",
                       CANFD_CAN_FRAME_MAX_BYTES, msg->len);
            return;
        }
    } else {
        /* for CANFD frame,  Only be CAN_FRAME_TYPE_DATA */
        msg->rtr = CAN_FRAME_TYPE_DATA;
    }

    msg->len = frame_len;
    rt_kprintf("Send data length: %d\n", msg->len);
    if (!test_canfd_is_valid_data_length(msg->len))
        rt_kprintf("Length is invalid\n", msg->len);
}

static void usage(char * program)
{
    printf("\n");
    printf("%s - test CAN loopback.\n\n", program);
    printf("Usage: %s CAN_DEV CAN_FRAME\n", program);
    printf("\tCAN_FRAME format: frame_id#frame_data\n");
    printf("For example:\n");
    printf("\t%s canfd0 fd int file\n", program);
    printf("\t%s canfd0 can int 1a3#11.22.9a.88.ef.00\n", program);
    printf("\t%s canfd0 can ext 1a3#11.22.9a.88.ef.00\n", program);
    printf("\t%s canfd1 fd  int 1a3#11.22.33.44.55.66.77.88.11.22.33.44.55.66.77.88\n", program);
    printf("\n");
}

int cmd_test_canfd_loopback(int argc, char *argv[])
{
    rt_err_t ret = 0;
    uint8_t msg_len = 0;
    uint8_t input_cnt = 0;
    const char *input_data = NULL;
    struct rt_can_msg msg = {0};
    rt_thread_t thread;
    struct aic_canfd_mode_info set_mode_info = {0};
    struct aic_canfd_allbaud_info set_baud = {0};
    int can_or_canfd = -1;

    if (argc != 5)
    {
        usage(argv[0]);
        return -RT_EINVAL;
    }

    snprintf(g_canfd_dev_name, 7, "%s", argv[1]);
    g_canfd_dev = rt_device_find(g_canfd_dev_name);
    if (!g_canfd_dev)
    {
        rt_kprintf("find %s failed!\n", g_canfd_dev_name);
        return -RT_EINVAL;
    }

    ret = rt_device_open(g_canfd_dev,
                         RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret)
    {
        rt_kprintf("%s open failed!\n", g_canfd_dev_name);
        return -RT_ERROR;
    }

    set_baud.baud_type = CANFD_BAUD_FD;
    set_baud.slow_baud.baudrate = CAN1MBaud;
    set_baud.slow_baud.duty = 80;
    set_baud.fast_baud.baudrate = CAN1MBaud * 2;
    set_baud.fast_baud.duty = 80;
    ret = rt_device_control(g_canfd_dev, RT_CAN_CMD_SET_BAUD_FD, &set_baud);
    if (ret)
    {
        rt_kprintf("%s set baudrate failed!\n", g_canfd_dev_name);
        ret = -RT_ERROR;
        goto __exit;
    }

    test_canfd_config_dma();
    //enable CAN TX interrupt
    rt_device_control(g_canfd_dev, RT_DEVICE_CTRL_SET_INT, NULL);

    if (strcmp(argv[3], "int") == 0) {
        set_mode_info.run_mode = CANFD_RUNMODE_INTERNAL;
    } else if (strcmp(argv[3], "ext") == 0) {
        set_mode_info.run_mode = CANFD_RUNMODE_EXTERNAL;
    } else {
        rt_kprintf("Chose the true run mode type\n");
        goto __exit;
    }

    set_mode_info.tx_type = CANFD_TXTYPE_TSONE;
    set_mode_info.tx_mode = CANFD_TXMODE_PTB;

    rt_device_control(g_canfd_dev, RT_CAN_CMD_SET_MODE, &set_mode_info);

    rt_device_set_rx_indicate(g_canfd_dev, canfd_rx_call);

    rt_sem_init(&g_rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);

    thread = rt_thread_create("canfd_rx", canfd_rx_thread, NULL,
                                  2048, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    } else {
        rt_kprintf("Create canfd_rx thread failed!\n");
        ret = -RT_ERROR;
        goto __exit;
    }

    if (strcmp(argv[2], "fd") == 0) {
        can_or_canfd = CANFD_TYPE;
    } else if (strcmp(argv[2], "can") == 0) {
        can_or_canfd = CAN_TYPE;
    } else {
        rt_kprintf("Enter the correct frame type command as instructed\n");
        goto __exit;
    }

    if (strcmp(argv[4], "file") == 0) {
        input_data = CANFD_INPUT_FRAME_DATA;
        msg.id = CANFD_INPUT_FRAME_ID;
        while (input_data[input_cnt] != '\0') {
            msg.data[msg_len++] = generate_data(input_data, &input_cnt);
        }
        for (int msg_index = 0; msg_index < msg_len; msg_index++) {
            printf("msg->data[%d] = 0x%#x\n", msg_index, msg.data[msg_index]);
        }
        parse_long_msg_data(&msg, msg_len, can_or_canfd);
    } else {
        parse_msg_data(&msg, argv[4], can_or_canfd);
    }

    rt_device_write(g_canfd_dev, 0, &msg, sizeof(msg));

    return RT_EOK;
__exit:
    rt_device_close(g_canfd_dev);
    return ret;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_canfd_loopback, test_canfd_loopback, Canfd device loopback sample);
