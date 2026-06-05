/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
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
#include "hal_canfd.h"
#include "aic_utils.h"
#include "test_canfd_common.h"

/*
 * This program is used to test the sending and receiving of CANFD.
 * You must ensure that there are two CANFD modules on the demo board.
 * CANFD0 sends data and CANFD1 receives data.
 * canfd_rx needs to be executed before canfd_tx.
 */
#define CANFD_RX_FILTER_ENABLE            0
#define CANFD_DEV_COUNT                   5

#define CANFD_TX_BUFFER_SIZE              72
#define CANFD_RX_BUFFER_SIZE              72

static struct canfd_dev_info g_canfd_devs[CANFD_DEV_COUNT] = {0};

struct canfd_dev_info* canfd_get_dev_info(const char *dev_name)
{
    struct canfd_dev_info *new_info = NULL;
    int i = 0, ret = 0;

    for (i = 0; i < CANFD_DEV_COUNT; i++) {
        if (strlen(g_canfd_devs[i].dev_name) == 0) {
            if (new_info == NULL)
                new_info = &g_canfd_devs[i];
            continue;
        }
        if (strcmp(dev_name, g_canfd_devs[i].dev_name) == 0) {
            return &g_canfd_devs[i];
        }
    }

    new_info->dev = rt_device_find(dev_name);
    if (!new_info->dev) {
        rt_kprintf("'%s' does not exist!\n", dev_name);
        return NULL;
    }

    ret = rt_device_open(new_info->dev,
                         RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret) {
        rt_kprintf("Failed to open '%s'\n", dev_name);
        return NULL;
    }

    ret = rt_device_control(new_info->dev, RT_DEVICE_CTRL_SET_INT, NULL);
    if (ret) {
        rt_kprintf("Failed to set interrupt of '%s'\n", dev_name);
        return NULL;
    }

    rt_sem_init(&new_info->rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);
    strcpy(new_info->dev_name, dev_name);
    return new_info;
}

static struct canfd_dev_info* canfd_get_dev_info_with_dev_t(rt_device_t dev)
{
    int i = 0;

    for (i = 0; i < CANFD_DEV_COUNT; i++) {
        if (g_canfd_devs[i].dev == dev) {
            return &g_canfd_devs[i];
        }
    }
    return NULL;
}

#ifdef AIC_CANFD_GET_DATA_BY_DMA
/*
*rx_buf/tx_buf size = CAN HEADER(8 bytes) + Data size
*recommend size:8 + 8/12/16/20/24/32/48/64
*/
static u8 g_canfd_dma_rx_buf[CANFD_TX_BUFFER_SIZE] __attribute__((aligned(CACHE_LINE_SIZE))) = {0};
static u8 g_canfd_dma_tx_buf[CANFD_RX_BUFFER_SIZE] __attribute__((aligned(CACHE_LINE_SIZE))) = {0};
#endif

bool canfd_is_valid_data_length(unsigned int length)
{
    for (unsigned int i = 0; i < 16; ++i) {
        if (canfd_data_lengths[i] == length) {
            return true;
        }
    }
    return false;
}

#ifdef AIC_CANFD_GET_DATA_BY_DMA
static void canfd_dma_tx_cb(void *arg)
{
    rt_kprintf("canfd dma tx callback\n");
}

static void canfd_dma_rx_cb(void *arg)
{
    rt_kprintf("canfd dma rx callback\n");
}

static void test_canfd_config_tx_dma(rt_device_t g_canfd_dev)
{
    struct canfd_dma_transfer_info chan_info_tx;

    chan_info_tx.buf = g_canfd_dma_tx_buf;
    chan_info_tx.buf_size = sizeof(g_canfd_dma_tx_buf)/sizeof(u8);
    chan_info_tx.callback = canfd_dma_tx_cb;
    chan_info_tx.callback_param = NULL;
    rt_device_control(g_canfd_dev, RT_CAN_CONFIG_DMA_TX, &chan_info_tx);
}

static void test_canfd_config_rx_dma(rt_device_t g_canfd_dev)
{
    struct canfd_dma_transfer_info chan_info_rx;

    chan_info_rx.buf = g_canfd_dma_rx_buf;
    chan_info_rx.buf_size = sizeof(g_canfd_dma_rx_buf)/sizeof(u8);
    chan_info_rx.callback = canfd_dma_rx_cb;
    chan_info_rx.callback_param = NULL;
    rt_device_control(g_canfd_dev, RT_CAN_CONFIG_DMA_RX, &chan_info_rx);
}
#endif

static rt_err_t canfd_rx_call(rt_device_t dev, rt_size_t size)
{
    struct canfd_dev_info *dev_info = canfd_get_dev_info_with_dev_t(dev);

    if (dev_info == NULL) {
        printf("Warning: canfd_rx_call dev_info is NULL\n");
        return -RT_ERROR;
    }

    rt_sem_release(&dev_info->rx_sem);
    return RT_EOK;
}

static void canfd_rx_thread(void *parameter)
{
    struct canfd_dev_info *dev_info = NULL;
    struct rt_can_msg rxmsg = {0};
    rt_size_t size;
    int ret = 0;

    dev_info = (struct canfd_dev_info *)parameter;

    while (1) {
        rt_sem_take(&dev_info->rx_sem, RT_WAITING_FOREVER);

        rxmsg.hdr = -1;
        size = rt_device_read(dev_info->dev, 0, &rxmsg, sizeof(rxmsg));
        if (!size) {
            rt_kprintf("CANFD read error\n");
            continue;
        }

        ret = dev_info->rx_cb(dev_info, &rxmsg, 1);
        if (ret)
            break;
    }

    dev_info->rx_inited = 0;
    rt_kprintf("canfd rx thread exit\n");
}

int canfd_setup_rx(struct canfd_dev_info *dev_info,
                   struct rt_can_filter_item *items,
                   int filter_cnt, canfd_rx_cb rx_cb)
{
    rt_err_t ret = RT_EOK;
    rt_thread_t thread;

    if (!dev_info->rx_inited) {
        rt_device_set_rx_indicate(dev_info->dev, canfd_rx_call);

        thread = rt_thread_create("can_rx", canfd_rx_thread, dev_info,
                                  2048, 25, 10);
        if (thread != RT_NULL) {
            rt_thread_startup(thread);
        } else {
            rt_kprintf("failed to create canfd_rx thread !\n");
            ret = -RT_ERROR;
            goto out;
        }

        dev_info->rx_inited = 1;
        rt_kprintf("The received thread of %s is ready...\n", dev_info->dev_name);
    } else {
        rt_kprintf("The received thread of %s is running...\n", dev_info->dev_name);
    }

#ifdef AIC_CANFD_GET_DATA_BY_DMA
    test_canfd_config_rx_dma(dev_info->dev);
#endif

    if (items == NULL)
        goto out;
    /* config can rx filter */
    struct rt_can_filter_config cfg = {filter_cnt, 1, items};

    ret = rt_device_control(dev_info->dev, RT_CAN_CMD_SET_FILTER, &cfg);
    if (ret) {
        rt_kprintf("Setting canfd filter failed!\n");
        return ret;
    }

out:
    dev_info->rx_cb = rx_cb;
    return ret;
}

int canfd_setup(struct canfd_dev_info *dev_info, u32 baudrate, u32 dbaudrate, int mode)
{
    struct aic_canfd_mode_info set_mode_info = {0};
    struct aic_canfd_allbaud_info set_baud = {0};
    int ret = 0;

#ifdef AIC_CANFD_GET_DATA_BY_DMA
    test_canfd_config_tx_dma(dev_info->dev);
#endif

    set_baud.baud_type = CANFD_BAUD_FD;
    set_baud.slow_baud.baudrate = baudrate;
    set_baud.slow_baud.duty = 80;
    set_baud.fast_baud.baudrate = dbaudrate;
    set_baud.fast_baud.duty = 80;
    ret = rt_device_control(dev_info->dev, RT_CAN_CMD_SET_BAUD_FD, &set_baud);
    if (ret) {
        rt_kprintf("Failed to set baudrate of '%s'\n", dev_info->dev_name);
        return ret;
    }

    // Set tx_type and tx_mode
    set_mode_info.tx_type = CANFD_TXTYPE_TSONE;
    set_mode_info.tx_mode = CANFD_TXMODE_PTB;
    set_mode_info.run_mode = mode;
    ret = rt_device_control(dev_info->dev, RT_CAN_CMD_SET_MODE, &set_mode_info);
    if (ret) {
        rt_kprintf("Failed to set mode of '%s'\n", dev_info->dev_name);
        return ret;
    }

    return ret;
}


static void usage_rx(const char *name)
{
    rt_kprintf("Usage: %s <dev_name>\n", name);
    rt_kprintf("Available devices:\n");
#ifdef AIC_USING_CANFD0
    rt_kprintf("\tcanfd0\n");
#endif
#ifdef AIC_USING_CANFD1
    rt_kprintf("\tcanfd1\n");
#endif
}

static int canfd_rx_handler(struct canfd_dev_info *dev_info,
                            struct rt_can_msg *msg_buf, int msg_cnt)
{
    struct rt_can_msg *rxmsg = NULL;
    int i = 0;

    for (i = 0; i < msg_cnt; i++) {
        rxmsg = msg_buf + i;
        rt_kprintf("%s received msg:\nID: 0x%x \n", dev_info->dev_name, rxmsg->id);
        hexdump((void *)rxmsg->data, rxmsg->len, 1);
    }

    return 0;
}

int test_can_rx(int argc, char *argv[])
{
    struct rt_can_filter_item *filters = NULL;
    struct canfd_dev_info *dev_info = NULL;
    char *dev_name = NULL;
    int filter_cnt = 0;
    rt_err_t ret = 0;

    if (argc != 2) {
        usage_rx(argv[0]);
        return -RT_EINVAL;
    }

    dev_name = argv[1];
    dev_info = canfd_get_dev_info(dev_name);
    if (dev_info == NULL)
        return -RT_EINVAL;

#if CANFD_RX_FILTER_ENABLE
    /* config can rx filter */
    struct rt_can_filter_item items[2] =
    {
        //Only receive standard data frame with ID 0x100~0x1FF
        RT_CAN_FILTER_ITEM_INIT(0x100, 0, 0, 0, 0x700, RT_NULL, RT_NULL),
        //Only receive standard data frame with ID 0x345
        RT_CAN_FILTER_ITEM_INIT(0x345, 0, 0, 0, 0x7FF, RT_NULL, RT_NULL),
    };
    filters = items;
    filter_cnt = 2;
#endif

    ret = canfd_setup_rx(dev_info, filters, filter_cnt, canfd_rx_handler);
    if (ret)
        goto __exit;

    ret = canfd_setup(dev_info, CAN1MBaud, CAN1MBaud * 2, CANFD_RUNMODE_NORMAL);

__exit:
    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_can_rx, canfd_rx, CAN rx sample. Usage: canfd_rx canfd0);

static void parse_msg_data(rt_can_msg_t msg, char * optarg, int can_or_canfd)
{
    char *token;
    uint8_t i = 0, id_received = 0;

    token = strtok(optarg, "#.");

    while (token) {
        if (!id_received) {
            msg->id = strtoul(token, NULL, 16);
            if (msg->id > 0x7FF)
                msg->ide = 1;
            else
                msg->ide = 0;

            if (can_or_canfd == CANFD_TYPE) {
                msg->fd_frame = 1;
            }

            id_received = 1;
        } else {
            /* frame data */
            if (can_or_canfd == CAN_TYPE) {
                /* for CAN frame, CAN_FRAME_TYPE_DATA or CAN_FRAME_TYPE_REMOTE */
                msg->rtr = CAN_FRAME_TYPE_DATA;
                if (i >= 8) {
                    rt_kprintf("CAN only support 8 bytes data\n", msg->len);
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
    if (!canfd_is_valid_data_length(msg->len))
        rt_kprintf("Length is invalid\n", msg->len);
}

static void usage_tx(char * program)
{
    printf("\n");
    printf("%s - test CANFD0 send CANFD-frame.\n\n", program);
    printf("Usage: %s CANFD_FRAME\n", program);
    printf("\tCANFD_FRAME format: frame_id#frame_data\n");
    printf("For example:\n");
    printf("\t%s canfd0 1a3#11.22.9a.88.ef.00.11.99\n", program);
    printf("\n");
}

int test_can_tx(int argc, char *argv[])
{
    struct canfd_dev_info *dev_info = NULL;
    struct rt_can_msg msg = {0};
    char *default_msg = NULL;
    char *dev_name = NULL;
    rt_err_t ret = 0;

    if (argc < 2) {
        usage_tx(argv[0]);
        return -RT_EINVAL;
    }

    dev_name = argv[1];
    dev_info = canfd_get_dev_info(dev_name);
    if (dev_info == NULL)
        return -RT_EINVAL;

    default_msg = "1a3#11.22.9a.88.ef.00.11.99\n";
    if (argc > 2)
        default_msg = argv[2];
    parse_msg_data(&msg, default_msg, CAN_TYPE);

    ret = canfd_setup(dev_info, CAN1MBaud, CAN1MBaud * 2, CANFD_RUNMODE_NORMAL);
    if (ret)
        goto __exit;

    rt_device_write(dev_info->dev, 0, &msg, sizeof(msg));

__exit:
    return ret;
}

MSH_CMD_EXPORT_ALIAS(test_can_tx, canfd_tx, CAN tx sample);
