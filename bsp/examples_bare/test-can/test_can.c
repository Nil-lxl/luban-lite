/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <aic_hal_can.h>

#define CAN_TEST_ID                     0x123
volatile uint8_t rx_done = 0;

#define CAN_HELP                                            \
    "test_can <CAN BUS ID>:\n"                              \
    "This program is used to test self-test mode of CAN\n"  \
    "You should connect TX and RX\n"                        \
    "Example:\n"                                            \
    "test_can 0    test CAN0 self-test mode\n"              \
    "test_can 1    test CAN1 self-test mode\n"

void can_usage(void)
{
    puts(CAN_HELP);
}

void can_rx_callback(can_handle * phandle, void *arg)
{
    if ((long)arg == CAN_EVENT_RX_IND)
        rx_done = 1;
}

static int test_can_example(int argc, char *argv[])
{
    int idx;
    can_handle can;
    can_msg_t msg, rx_msg;
    can_filter_config_t filter_config;

    if (argc != 2)
    {
        can_usage();
        return 0;
    }

    idx = atoi(argv[1]);

    /* Initialize CAN */
    hal_can_init(&can, idx);
    /* Set can baudrate */
    hal_can_ioctl(&can, CAN_IOCTL_SET_BAUDRATE, (void *)1000000UL);
    /* Set can mode */
    hal_can_ioctl(&can, CAN_IOCTL_SET_MODE, (void *)CAN_SELFTEST_MODE);
    /* Configure interrupt routine */
    aicos_request_irq(CAN0_IRQn + idx, hal_can_isr_handler,
                      0, NULL, (void *)&can);
    hal_can_enable_interrupt(&can);
    /* Set can callback */
    hal_can_attach_callback(&can, can_rx_callback, NULL);
    /* Set can filter, only frame with ID CAN_TEST_ID is received */
    memset(&filter_config, 0, sizeof(filter_config));
    filter_config.filter_mode = SINGLE_FILTER_MODE;
    filter_config.is_eff = 0;
    filter_config.rxcode.sfs.id_filter = CAN_TEST_ID;
    filter_config.rxmask.sfs.id_filter = 0;
    filter_config.rxmask.sfs.rtr_filter = 1;
    filter_config.rxmask.sfs.data0_filter = 0xff;
    filter_config.rxmask.sfs.data1_filter = 0xff;
    hal_can_ioctl(&can, CAN_IOCTL_SET_FILTER, &filter_config);

    /* Send CAN frame */
    msg.id = CAN_TEST_ID;
    msg.ide = 0;
    msg.rtr = 0;
    msg.dlc = 8;
    msg.data[0] = 0x00;
    msg.data[1] = 0x11;
    msg.data[2] = 0x22;
    msg.data[3] = 0x33;
    msg.data[4] = 0x44;
    msg.data[5] = 0x55;
    msg.data[6] = 0x66;
    msg.data[7] = 0x77;
    hal_can_send_frame(&can, &msg, SELF_REQ);
    /* Receive CAN frame*/
    while (!rx_done)
    {

    }

    rx_done = 0;
    hal_can_receive_frame(&can, &rx_msg);
    printf("self-test received msg:\n\t");
    for (int i =0; i < rx_msg.dlc; i++)
    {
        printf("%02x ", rx_msg.data[i]);
    }
    printf("\n");

    hal_can_detach_callback(&can);
    hal_can_uninit(&can);

    return 0;
}

CONSOLE_CMD(test_can, test_can_example, "CAN self-test example");
