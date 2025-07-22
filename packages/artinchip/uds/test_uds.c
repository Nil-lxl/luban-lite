/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtthread.h>
#include "rtdevice.h"
#include <aic_core.h>
#include "aic_hal_can.h"
#include "uds_def.h"

/*
 * This program is used to test the sending and receiving of CAN.
 * You must ensure that there are two CAN modules on the demo board.
 * CAN0 sends data and CAN1 receives data.
 * can_rx needs to be executed before can_tx.
 */
#define CAN_TX_DEV_NAME                 "can0"
#define CAN_RX_DEV_NAME                 "can1"
#define CAN_RX_FILTER_ENABLE            0

static struct rt_semaphore rx_sem = {0};
static rt_device_t can_rx_dev = RT_NULL;
uint8_t rxThreadInited = 0;

static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

static void can_rx_thread(void *parameter)
{
    struct rt_can_msg rxmsg = {0};
    size_t size;

    while (1) {
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);

        rxmsg.hdr = -1;
        size = rt_device_read(can_rx_dev, 0, &rxmsg, sizeof(rxmsg));
        if (!size)
        {
            rt_kprintf("CAN read error\n");
            break;
        }
        //print raw CAN frame for debug
        rt_kprintf("[CAN] RX ID:0x%03X LEN:%d DATA: ", rxmsg.id, rxmsg.len);
        for (int i = 0; i < rxmsg.len; i++) 
            rt_kprintf("%02X ", rxmsg.data[i]);
        rt_kprintf("\n");

        uds_recv_frame(rxmsg.id, rxmsg.data, 8);

        if (rxmsg.id == UDS_RESPONSE_ID || rxmsg.id == (UDS_REQUEST_ID + 0x8) ||
           (rxmsg.data[0] & 0xF0) == 0x40) {
            print_uds_response(rxmsg.id, rxmsg.data, rxmsg.len);
        }
    }
}

int test_uds_can_rx(int argc, char *argv[])
{
    rt_err_t ret = 0;
    rt_thread_t thread;
    rt_timer_t uds_timer = RT_NULL;

    if (!rxThreadInited) {
        /* can rx configuration */
        can_rx_dev = rt_device_find(CAN_RX_DEV_NAME);
        if (!can_rx_dev)
        {
            rt_kprintf("find %s failed!\n", CAN_RX_DEV_NAME);
            return -RT_ERROR;
        }

        ret = rt_device_open(can_rx_dev,
                             RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
        if (ret)
        {
            rt_kprintf("%s open failed!\n", CAN_RX_DEV_NAME);
            return -RT_ERROR;
        }

        ret = rt_device_control(can_rx_dev, RT_CAN_CMD_SET_BAUD,
                                (void *)CAN1MBaud);
        if (ret)
        {
            rt_kprintf("%s set baudrate failed!\n", CAN_RX_DEV_NAME);
            ret = -RT_ERROR;
            goto __exit;
        }

        //enable CAN RX interrupt
        rt_device_control(can_rx_dev, RT_DEVICE_CTRL_SET_INT, NULL);

#if CAN_RX_FILTER_ENABLE
        /* config can rx filter */
        struct rt_can_filter_item items[2] =
        {
            //Only receive standard data frame with ID 0x100~0x1FF
            RT_CAN_FILTER_ITEM_INIT(0x100, 0, 0, 0, 0x700, RT_NULL, RT_NULL),
            //Only receive standard data frame with ID 0x345
            RT_CAN_FILTER_ITEM_INIT(0x345, 0, 0, 0, 0x7FF, RT_NULL, RT_NULL),
        };

        struct rt_can_filter_config cfg = {2, 1, items};

        ret = rt_device_control(can_rx_dev, RT_CAN_CMD_SET_FILTER, &cfg);
        if (ret)
        {
            rt_kprintf("Setting can filter failed!\n");
            return ret;
        }
#endif
        uds_init();
        uds_timer = rt_timer_create("uds_timer", (void (*)(void*))uds_1ms_task, RT_NULL, 1,
                                    RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        if (uds_timer != RT_NULL)
            rt_timer_start(uds_timer);

        rt_device_set_rx_indicate(can_rx_dev, can_rx_call);

        rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);

        thread = rt_thread_create("can_rx", can_rx_thread, RT_NULL,
                                  2048, 15, 10);
        if (thread != RT_NULL)
        {
            rt_thread_startup(thread);
        }
        else
        {
            rt_kprintf("create can_rx thread failed!\n");
            ret = -RT_ERROR;
        }

        rxThreadInited = 1;
        rt_kprintf("The "CAN_RX_DEV_NAME" received thread is ready...\n");
    }
    else
    {
        rt_kprintf("The "CAN_RX_DEV_NAME" received thread is running...\n");
    }

    return RT_EOK;

__exit:
    rt_device_close(can_rx_dev);
    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_uds_can_rx, uds_can_rx, CAN rx sample. Usage: can_rx);

void test_uds_parse_msg_data(rt_can_msg_t msg, char * optarg)
{
    char *token;
    uint8_t i = 0, id_received = 0;

    token = strtok(optarg, "#.");

    while (token) {
        if (!id_received) {
            msg->id = strtoul(token, NULL, 16);
            if (msg->id > 0x7FF) {
                msg->ide = 1;
            } else {
                msg->ide = 0;
            }
            id_received = 1;
        } else {
            msg->rtr = CAN_FRAME_TYPE_DATA;
            msg->data[i++] = strtoul(token, NULL, 16);
            if (i >= 8)
                break;
        }

        token = strtok(NULL, "#.");
    }

    msg->len = i;
}

static void usage_tx(char * program)
{
    printf("\n");
    printf("%s - test CAN0 send CAN-frame.\n\n", program);
    printf("Usage: %s CAN_FRAME\n", program);
    printf("\tCAN_FRAME format: frame_id#frame_data\n");
    printf("For example:\n");
    printf("\t%s 1a3#11.22.9a.88.ef.00\n", program);
    printf("\n");
}

int test_uds_can_tx(int argc, char *argv[])
{
    rt_err_t ret = 0;
    rt_size_t  size;
    struct rt_can_msg msg = {0};
    rt_device_t can_tx_dev = RT_NULL;

    if (argc != 2)
    {
        usage_tx(argv[0]);
        return -RT_EINVAL;
    }

    if (!rxThreadInited)
    {
        rt_kprintf("Please execute can_rx at first!\n");
        return -RT_EINVAL;
    }

    test_uds_parse_msg_data(&msg, argv[1]);

    /* can tx configuration */
    can_tx_dev = rt_device_find(CAN_TX_DEV_NAME);
    if (!can_tx_dev)
    {
        rt_kprintf("find %s failed!\n", CAN_TX_DEV_NAME);
        return -RT_EINVAL;
    }

    ret = rt_device_open(can_tx_dev,
                         RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret)
    {
        rt_kprintf("%s open failed!\n", CAN_TX_DEV_NAME);
        return -RT_ERROR;
    }

    ret = rt_device_control(can_tx_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    if (ret)
    {
        rt_kprintf("%s set baudrate failed!\n", CAN_TX_DEV_NAME);
        ret = -RT_ERROR;
        goto __exit;
    }

    //enable CAN TX interrupt
    rt_device_control(can_tx_dev, RT_DEVICE_CTRL_SET_INT, NULL);

    size = rt_device_write(can_tx_dev, 0, &msg, sizeof(msg));
    if (size != sizeof(msg))
    {
        rt_kprintf("can dev write data failed!\n");
        ret = -RT_EIO;
    }

__exit:
    rt_device_close(can_tx_dev);
    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_uds_can_tx, uds_can_tx, CAN tx sample);


void parse_uds_msg_data(rt_can_msg_t msg, char * optarg)
{
    char *token;
    uint8_t i = 0;

    token = strtok(optarg, ".");

    while (token) {
        msg->data[i++] = strtoul(token, NULL, 16);

        token = strtok(NULL, ".");
    }

    msg->len = i;
}

int test_uds_service(int argc, char *argv[])
{
    struct rt_can_msg msg = {0};
    if (argc < 2)
    {
        rt_kprintf("Usage: test_uds_service <service>\n");
        rt_kprintf("Available services:\n");
        rt_kprintf("  session - Test session control\n");
        rt_kprintf("  send - Test send consecutive frame\n");
        return -RT_ERROR;
    }

    if (strcmp(argv[1], "session") == 0) {
        parse_uds_msg_data(&msg, argv[2]);
        uds_send_frame(UDS_REQUEST_ID, msg.data, sizeof(msg.data[0]) * msg.len, 1);
    }

    if (strcmp(argv[1], "send") == 0) {
        uint8_t ff_data[] = {0x10, 0x0A, 0x36, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        uds_send_frame(UDS_REQUEST_ID, ff_data, 8, 0);

        uint8_t cf1_data[] = {0x21, 0x11, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00};
        uds_send_frame(UDS_REQUEST_ID, cf1_data, 8, 0);

        uint8_t cf2_data[] = {0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};
        uds_send_frame(UDS_REQUEST_ID, cf2_data, 8, 0);
        assemble_complete_frame();
    }
    return RT_EOK;
}

MSH_CMD_EXPORT(test_uds_service, UDS service test);
