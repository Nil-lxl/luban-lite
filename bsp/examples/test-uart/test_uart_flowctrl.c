/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <aic_core.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#define SAMPLE_UART_NAME       "uart2"
#define MAX_BUFFER_LEN          256

struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};

static char uart_name[RT_NAME_MAX] = SAMPLE_UART_NAME;
static rt_device_t fc_serial;
static struct rt_semaphore rx_fc_sem;
static int fc_g_exit = 0;
static uint32_t baud = 0;

static const struct option fc_longopts[] = {
    {"uart",      optional_argument,  NULL, 'u'},
    {"normal",    no_argument,  NULL, 'n'},
    {"get",       no_argument,  NULL, 'g'},
    {"receive",   no_argument,  NULL, 'r'},
    {"baudrate",  optional_argument,  NULL, 'b'},
    {"help",      no_argument,  NULL, 'h'},
    {NULL, 0, NULL, 0},
};

static void test_uart_flowctrl_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -u, --uart\t\tSelect a serial port. Default as uart2\n");
    printf("\t -n, --normal\t\tThis function is to send the received data\n");
    printf("\t -g, --get\t\tThis function is to get the received data\n");
    printf("\t -r, --received\t\tThis function is to detect whether data is received\n");
    printf("\t -b, --baudrate\t\tThis function is to set the baudrate\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: Serial port flowctrl test\n");
    printf("         %s -u uart2 -n  \n", program);
}

rt_err_t uart_input_set(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
        rt_sem_release(&rx_fc_sem);

    return RT_EOK;
}

rt_err_t uart_receive_data_print(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("The data has been received\n");
    return 0;
}

void serial_normal_test_entry(void *parameter)
{
    char ch;
    int ret = 0;
    char str_receive[MAX_BUFFER_LEN] = {0};
    int index = 0;

    while (1) {
        ret = rt_device_read(fc_serial, 0, &ch, 1);
        if (ret == 1) {
            str_receive[index] = ch;
            index ++;
            if (index <= MAX_BUFFER_LEN -1 || ch == '\n') {
                rt_device_write(fc_serial, 0, str_receive, index);
                index = 0;
            }
        } else {
            //rt_kprintf("waiting for uart input\n");
            rt_sem_take(&rx_fc_sem, RT_WAITING_FOREVER);
        }
    }

    rt_sem_detach(&rx_fc_sem);
    fc_g_exit = 1;
}

void serial_get_data_entry(void *parameter)
{
    char ch;
    int ret = 0;
    char str_receive[MAX_BUFFER_LEN] = {0};
    int index = 0;

    while (1) {
        ret = rt_device_read(fc_serial, 0, &ch, 1);
        if (ret == 1) {
            str_receive[index] = ch;
            index ++;

            if(index <= MAX_BUFFER_LEN -1 || ch == '\n')
            {
                rt_device_write(fc_serial, 0, str_receive, index);
                index = 0;
            }

        } else {
            rt_kprintf("data send done!\n");
            break;
        }
    }
}

void uart_normal_test (void)
{

    fc_serial = rt_device_find(uart_name);
    if (!fc_serial) {
        rt_kprintf("find %s failed!\n", uart_name);
        return;
    }

    rt_sem_init(&rx_fc_sem, "fc_rx_sem", 0, RT_IPC_FLAG_FIFO);

    if (rt_device_open(fc_serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("open failed!\n");
        return;
    }

    if (rt_device_set_rx_indicate(fc_serial, uart_input_set)!= RT_EOK) {
        rt_kprintf("indicate failed!\n");
        return;
    }

    rt_thread_t thread = rt_thread_create("fc_normal_serial", serial_normal_test_entry,
                                            RT_NULL, 1024 * 8, 25, 10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    } else {
        rt_kprintf("thread create fail!\n");
    }
}

void uart_receive_data_test(void)
{
    fc_serial = rt_device_find(uart_name);

    if (!fc_serial) {
        rt_kprintf("find %s failed!\n", uart_name);
        return;
    }

    if (rt_device_open(fc_serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("open failed!\n");
        return;
    }

    if (rt_device_set_rx_indicate(fc_serial, uart_receive_data_print)!= RT_EOK) {
        rt_kprintf("indicate failed!\n");
        return;
    }
}

void uart_get_data_test(void)
{
    fc_serial = rt_device_find(uart_name);
    if (!fc_serial) {
        rt_kprintf("find %s failed!\n", uart_name);
        return;
    }

    if (rt_device_open(fc_serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("open failed!\n");
        return;
    }

    rt_thread_t thread = rt_thread_create("fc_get_data_serial", serial_get_data_entry,
                                            RT_NULL, 1024 * 8, 25, 10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    } else {
        rt_kprintf("thread create fail!\n");
    }
}

void uart_set_baudrate(uint32_t baud)
{
    fc_serial = rt_device_find(uart_name);
    if (!fc_serial) {
        rt_kprintf("find %s failed!\n", uart_name);
        return;
    }
    if (rt_device_control(fc_serial, RT_SERIAL_SET_BAUDRATE, &baud) != RT_EOK)
        rt_kprintf("uart set baudrate fail!\n");
}

static int test_uart_flowctrl(int argc, char *argv[])
{
    int fc_test_mode = -1;

    optind = 0;
    while ((fc_test_mode = getopt_long(argc, argv, "u:b:ngrh", fc_longopts, NULL)) != -1) {
        rt_kprintf("flow control test mode: %c\n", fc_test_mode);

        switch (fc_test_mode) {
        case 'u':
            rt_kprintf("flow control uart: %s\n", optarg);
            rt_strncpy(uart_name, optarg, RT_NAME_MAX);
            break;
        case 'n':
            uart_normal_test();
            break;
        case 'g':
            uart_get_data_test();
            break;
        case 'r':
            uart_receive_data_test();
            break;
        case 'b':
            baud = atoi(optarg);
            rt_kprintf("baud:   %ld\n", baud);
            rt_thread_mdelay(100);
            uart_set_baudrate(baud);
            break;
        case 'h':
        default:
            test_uart_flowctrl_usage(argv[0]);
            return 0;
        }
    }
    return 0;
}
MSH_CMD_EXPORT(test_uart_flowctrl, uart device flow control sample);

