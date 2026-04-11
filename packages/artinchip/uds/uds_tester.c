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
#define CAN_TX_DEV_NAME      "can0"
#define CAN_RX_DEV_NAME      "can1"
#define CAN_RX_FILTER_ENABLE 1

// UDS  SID
#define SID_DiagSessionControl   0x10
#define SID_SecurityAccess       0x27
#define SID_CommunicationControl 0x28
#define SID_RoutineControl       0x31
#define SID_RequestDownload      0x34
#define SID_TransferData         0x36
#define SID_RequestTransferExit  0x37
#define SID_ControlDTCSetting    0x85
#define SID_ECUReset             0x11
#define SID_ReadDataByIdentifier 0x22

struct rt_semaphore g_flow_control_sem = { 0 };
static struct rt_semaphore g_rx_sem = { 0 };
static char uds_input_file_path[256] = { 0 };
static rt_device_t g_can_rx_dev = RT_NULL;
static rt_thread_t g_uds_thread = RT_NULL;
static rt_sem_t g_uds_sem = RT_NULL;
static FILE *g_fp = NULL;

char uds_output_file_path[256] = { 0 };
uds_state_t g_current_state = UDS_STATE_IDLE;
uint32_t g_max_block_size = 0;
uint32_t g_bytes_per_block = 0;
rt_device_t g_can_tx_dev = NULL;
rt_mutex_t g_uds_state_mutex = RT_NULL;

typedef enum { DEVICE_TYPE_ECU, DEVICE_TYPE_TESTER } DeviceType;

static void uds_timer_callback(void *parameter)
{
    rt_sem_release(g_uds_sem);
}

static void uds_thread_entry(void *parameter)
{
    while (1) {
        if (rt_sem_take(g_uds_sem, RT_WAITING_FOREVER) == RT_EOK) {
            uds_1ms_task();
        }
    }
}

static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&g_rx_sem);
    return RT_EOK;
}

static void can_tx_sem_init(void)
{
    rt_sem_init(&g_flow_control_sem, "fc_sem", 0, RT_IPC_FLAG_FIFO);
    if (g_uds_state_mutex == RT_NULL) {
        g_uds_state_mutex = rt_mutex_create("uds_mutex", RT_IPC_FLAG_FIFO);
        if (g_uds_state_mutex == RT_NULL) {
            rt_kprintf("uds state mutex create failed!\n");
        }
    }
}

static int uds_timer_init(void)
{
    rt_timer_t uds_timer = RT_NULL;

    g_uds_sem = rt_sem_create("g_uds_sem", 0, RT_IPC_FLAG_FIFO);
    if (g_uds_sem == RT_NULL) {
        rt_kprintf("create uds semaphore failed.\n");
        return -1;
    }

    g_uds_thread = rt_thread_create("g_uds_thread", uds_thread_entry, RT_NULL, 2048, 8, 10);
    if (g_uds_thread != RT_NULL) {
        rt_thread_startup(g_uds_thread);
    } else {
        rt_kprintf("create uds thread failed.\n");
        return -1;
    }

    uds_timer = rt_timer_create("uds_timer", uds_timer_callback, RT_NULL, 1,
                                RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (uds_timer != RT_NULL) {
        rt_timer_start(uds_timer);
    }

    return 0;
}

static void send_enter_extended_session(void)
{
    g_current_state = UDS_STATE_ENTER_EXT_SESSION;
    uint8_t uds_request[] = { 0x02, SID_DiagSessionControl, 0x03 };

    uds_send_frame(UDS_REQUEST_ID, uds_request, sizeof(uds_request), 1);
}

static long get_file_size(FILE *fp)
{
    long file_current_pos = ftell(fp);
    if (file_current_pos == -1) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END)) {
        return -1;
    }

    long size = ftell(fp);
    if (size == -1) {
        return -1;
    }

    if (fseek(fp, file_current_pos, SEEK_SET)) {
        return -1;
    }

    return size;
}

static void send_file_thread(void *parameter)
{
    uint8_t *data_buffer = NULL;
    uint8_t block_counter = 1;
    static uint32_t current_pos = 0;
    static uint32_t total_size = 0;
    char *input_path = (char *)parameter;
    while (1) {
        switch (g_current_state) {
            case UDS_STATE_REQUEST_UPLOAD:
                g_fp = fopen(input_path, "rb");
                if (g_fp == NULL) {
                    rt_kprintf("Error: open ota file failed\n");
                    g_current_state = UDS_STATE_REQUEST_UPLOAD_EXIT;
                    break;
                }
                total_size = get_file_size(g_fp);
                if (total_size == 0) {
                    rt_kprintf("Error: invalid file size\n");
                    fclose(g_fp);
                    g_fp = NULL;
                    g_current_state = UDS_STATE_REQUEST_UPLOAD_EXIT;
                    break;
                }
                uint8_t download_request[11] = { SID_RequestDownload,
                                                 0x00,
                                                 0x44,
                                                 0x08,
                                                 0x00,
                                                 0x00,
                                                 0x00,
                                                 (total_size >> 24) & 0xFF,
                                                 (total_size >> 16) & 0xFF,
                                                 (total_size >> 8) & 0xFF,
                                                 total_size & 0xFF };
                uint16_t request_len = sizeof(download_request);
                g_current_state = UDS_STATE_WAIT_DOWNLOAD_RESPONSE;

                if (network_send_udsmsg(download_request, request_len) == 0) {
                    rt_kprintf("0x34 send success...\n");
                } else {
                    rt_kprintf("Error: send failed\n");
                    fclose(g_fp);
                    g_fp = NULL;
                    g_current_state = UDS_STATE_REQUEST_UPLOAD_EXIT;
                }
                break;
            case UDS_STATE_SEND_UPLOAD:
                if (data_buffer == NULL) {
                    data_buffer = rt_malloc(g_bytes_per_block + 2);
                    if (!data_buffer) {
                        rt_kprintf("Error: memory allocation failed\n");
                        g_current_state = UDS_STATE_REQUEST_UPLOAD_EXIT;
                        break;
                    }
                }
                if (current_pos >= total_size) {
                    rt_kprintf("File transfer completed, sending exit request\n");
                    rt_free(data_buffer);
                    data_buffer = NULL;
                    g_current_state = UDS_STATE_REQUEST_UPLOAD_EXIT;
                    break;
                }
                if (fseek(g_fp, current_pos, SEEK_SET) != 0) {
                    rt_kprintf("Error: fseek failed\n");
                    g_current_state = UDS_STATE_ERROR;
                    break;
                }

                size_t bytes_to_read = total_size - current_pos;
                if (bytes_to_read > g_bytes_per_block) {
                    bytes_to_read = g_bytes_per_block;
                }

                size_t read_len = fread(data_buffer + 2, 1, bytes_to_read, g_fp);
                if (read_len == 0) {
                    rt_kprintf("Error: file read error\n");
                    g_current_state = UDS_STATE_ERROR;
                    break;
                }

                data_buffer[0] = SID_TransferData;
                data_buffer[1] = block_counter;

                uint16_t frame_len = 2 + read_len;
                rt_mutex_take(g_uds_state_mutex, RT_WAITING_FOREVER);
                if (network_send_udsmsg(data_buffer, frame_len) == 0) {
                    current_pos += read_len;
                    block_counter++;
                    g_current_state = UDS_STATE_WAIT_TRANSFER_RESPONSE;
                    printf("set g_current_state to UDS_STATE_WAIT_TRANSFER_RESPONSE\n");
                } else {
                    rt_kprintf("Error: failed to send data block\n");
                    g_current_state = UDS_STATE_ERROR;
                }
                rt_mutex_release(g_uds_state_mutex);
                break;
            case UDS_STATE_WAIT_TRANSFER_RESPONSE:
                if (rt_sem_take(&g_flow_control_sem, 2000) == RT_EOK) {
                    g_current_state = UDS_STATE_SEND_UPLOAD;
                } else {
                    rt_kprintf("Error: flow control timeout\n");
                    g_current_state = UDS_STATE_ERROR;
                }
                break;
            case UDS_STATE_REQUEST_UPLOAD_EXIT: {
                uint8_t exit_request[2] = { 0x01, SID_RequestTransferExit };
                uint8_t exit_len = 2;
                uds_send_frame(UDS_REQUEST_ID, exit_request, exit_len, 1);
                rt_kprintf("file transfer completed!\n");
                g_current_state = UDS_STATE_IDLE;
                break;
            }
            case UDS_STATE_APP_CHECK:
                rt_thread_mdelay(1);
                break;
            case UDS_STATE_ERROR:
                rt_kprintf("file transfer failed!\n");
                if (g_fp) {
                    fclose(g_fp);
                    g_fp = NULL;
                }
                g_current_state = UDS_STATE_IDLE;
                break;
            default:
                break;
        }
        rt_thread_mdelay(1);
    }
}

static void can_rx_thread(void *parameter)
{
    struct rt_can_msg rxmsg = { 0 };
    size_t size;
    DeviceType device_type = *(DeviceType *)parameter;
    if (device_type == DEVICE_TYPE_TESTER) {
        send_enter_extended_session();
    }
    while (1) {
        rt_sem_take(&g_rx_sem, RT_WAITING_FOREVER);

        rxmsg.hdr = -1;
        size = rt_device_read(g_can_rx_dev, 0, &rxmsg, sizeof(rxmsg));
        if (!size) {
            rt_kprintf("CAN read error\n");
            break;
        }
        uds_recv_frame(rxmsg.id, rxmsg.data, 8);
    }
}

static int can_device_rx_init(uint32_t filter_id)
{
    rt_err_t ret = 0;
    g_can_rx_dev = rt_device_find(CAN_RX_DEV_NAME);
    if (!g_can_rx_dev) {
        rt_kprintf("find %s failed!\n", CAN_RX_DEV_NAME);
        return -RT_ERROR;
    }

    ret = rt_device_open(g_can_rx_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret) {
        rt_kprintf("%s open failed!\n", CAN_RX_DEV_NAME);
        return -RT_ERROR;
    }

    ret = rt_device_control(g_can_rx_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    if (ret) {
        rt_kprintf("%s set baudrate failed!\n", CAN_RX_DEV_NAME);
        return -RT_ERROR;
    }

    rt_device_control(g_can_rx_dev, RT_DEVICE_CTRL_SET_INT, NULL);

#if CAN_RX_FILTER_ENABLE
    struct rt_can_filter_item items[1] = {
        RT_CAN_FILTER_ITEM_INIT(filter_id, 0, 0, 0, 0x7FF, RT_NULL, RT_NULL),
    };

    struct rt_can_filter_config cfg = { 1, 1, items };

    ret = rt_device_control(g_can_rx_dev, RT_CAN_CMD_SET_FILTER, &cfg);
    if (ret) {
        rt_kprintf("Setting can filter failed!\n");
        return ret;
    }
#endif
    return RT_EOK;
}

static int can_device_tx_init(void)
{
    if (g_can_tx_dev != NULL) {
        return 0;
    }

    g_can_tx_dev = rt_device_find(CAN_TX_DEV_NAME);
    if (g_can_tx_dev == NULL) {
        rt_kprintf("Error: CAN device %s not found!\n", CAN_TX_DEV_NAME);
        return -1;
    }

    rt_err_t ret = rt_device_open(g_can_tx_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        rt_kprintf("Error: CAN device open failed! %d\n", ret);
        return -1;
    }

    ret = rt_device_control(g_can_tx_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    if (ret != RT_EOK) {
        rt_kprintf("Error: CAN set baud failed! %d\n", ret);
        return -1;
    }

    rt_device_control(g_can_tx_dev, RT_DEVICE_CTRL_SET_INT, NULL);
    rt_kprintf("CAN device initialized successfully.\n");
    return 0;
}

int test_uds_ecu(int argc, char *argv[])
{
    char *output_path = NULL;
    rt_thread_t thread;
    rt_err_t ret = 0;
    int i = 0;

    if (argc < 3) {
        rt_kprintf("Usage: test_uds_ecu -o <output_file_path>\n");
        return -RT_ERROR;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[i + 1];
            break;
        }
    }
    if (output_path == NULL) {
        rt_kprintf("Usage: test_uds_ecu -o <output_file_path>\n");
        return -RT_ERROR;
    }

    strncpy(uds_output_file_path, output_path, sizeof(uds_output_file_path) - 1);
    uds_output_file_path[sizeof(uds_output_file_path) - 1] = '\0';

    can_device_tx_init();
    ret = can_device_rx_init(0x7E0);
    uds_timer_init();
    uds_init();

    if (ret == -RT_ERROR)
        goto __exit;
    rt_device_set_rx_indicate(g_can_rx_dev, can_rx_call);
    rt_sem_init(&g_rx_sem, "g_rx_sem ", 0, RT_IPC_FLAG_PRIO);

    DeviceType device_type = DEVICE_TYPE_ECU;
    thread = rt_thread_create("can_rx", can_rx_thread, &device_type, 5 * 1024, 15, 10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    } else {
        rt_kprintf("create can_rx thread failed!\n");
        ret = -RT_ERROR;
    }

    return RT_EOK;

__exit:
    rt_device_close(g_can_rx_dev);
    return ret;
}

MSH_CMD_EXPORT(test_uds_ecu, UDS ecu test.Usage : test_uds_ecu - o "output_file_path");

int test_uds_tester(int argc, char *argv[])
{
    rt_thread_t uds_send_thread;
    rt_thread_t uds_rx_thread;
    char *input_path = NULL;
    int i = 0;
    if (argc < 3) {
        rt_kprintf("Usage: test_uds_tester -i <input_file_path>\n");
        return -RT_ERROR;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_path = argv[i + 1];
            break;
        }
    }
    if (input_path == NULL) {
        rt_kprintf("Usage: test_uds_tester -i <input_file_path>\n");
        return -RT_ERROR;
    }

    strncpy(uds_input_file_path, input_path, sizeof(uds_input_file_path) - 1);
    uds_input_file_path[sizeof(uds_input_file_path) - 1] = '\0';

    printf("uds_input_file_path: %s\n", uds_input_file_path);

    uds_init();
    uds_timer_init();
    can_tx_sem_init();
    can_device_tx_init();
    can_device_rx_init(0x7E8);

    rt_sem_init(&g_rx_sem, "g_rx_sem ", 0, RT_IPC_FLAG_PRIO);
    rt_device_set_rx_indicate(g_can_rx_dev, can_rx_call);

    DeviceType device_type = DEVICE_TYPE_TESTER;
    uds_rx_thread = rt_thread_create("can_rx", can_rx_thread, &device_type, 2048, 10, 15);
    uds_send_thread =
        rt_thread_create("usd_send", send_file_thread, uds_input_file_path, 3 * 1024, 15, 10);

    if (uds_rx_thread != RT_NULL && uds_send_thread != RT_NULL) {
        rt_thread_startup(uds_rx_thread);
        rt_thread_startup(uds_send_thread);
    } else {
        rt_kprintf("create can_rx thread failed!\n");
        return -RT_ERROR;
    }

    rt_kprintf("The " CAN_RX_DEV_NAME " received thread is ready...\n");
    return RT_EOK;
}

MSH_CMD_EXPORT_ALIAS(test_uds_tester, test_uds_tester,
                     "UDS file transfer test. Usage: test_uds_tester -i <input_file_path>");
