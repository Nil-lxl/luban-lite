/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date         Author          Notes
 * 2018-08-15   misonyo         first implementation.
 * 2023-05-25   geo.dong        ArtInChip
 */

#include <getopt.h>
#include <string.h>
#include <rtthread.h>
#include <aic_core.h>

#define TEST_UART_DEBUG 0
#define TEST_UART_CONFIG 0
#define TEST_UART_PORT "uart4"
#define TEST_UART_BUF_LEN 128
#define TEST_UART_COUNT 10
static int g_exit = 0;
static int g_test_mode;
static rt_device_t g_serial;
static char g_msg_pool[256] = {0};
static struct rt_semaphore g_rx_sem;
static struct rt_messagequeue g_rx_mq;

typedef enum {
    TEST_UART_MODE_SEMAPHORE,
    TEST_UART_MODE_MESSAGEQUEUE,
    TEST_UART_MODE_UNKNOW,
 } aic_test_uart_mode;

struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};

static const char sopts[] = "p:h";
static const struct option lopts[] = {
    {"port",       optional_argument,  NULL, 'p'},
    {"help",       no_argument,        NULL, 'h'},
    {0, 0, 0}
};

static void test_uart_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -p, --port\t\tSelect a serial port. Default as uart4\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example: Send and receive test\n");
    printf("         %s -p uart4 \n", program);
}

rt_err_t serial_input_sem(rt_device_t dev, rt_size_t size)
{
    if (size > 0)
        rt_sem_release(&g_rx_sem);

    return RT_EOK;
}

static rt_err_t serial_input_mq(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;
    result = rt_mq_send(&g_rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL) {
        rt_kprintf("message queue full!\n");
    }
    return result;
}

static int serial_config_uart(rt_device_t serial)
{
#if TEST_UART_CONFIG
    rt_serial_t *uart = container_of(serial, rt_serial_t, parent);
    uart->config.baud_rate = BAUD_RATE_115200;
    uart->config.data_bits = 8;
    uart->config.stop_bits = 1;
    uart->config.parity = PARITY_NONE;

    if (rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &uart->config) != RT_EOK) {
        rt_kprintf("uart set baudrate fail!\n");
        return -1;
    }
#endif
    return 0;
}

static void serial_init_variable(void)
{
    g_exit = 0;
    if(g_test_mode == TEST_UART_MODE_SEMAPHORE) {
        rt_device_set_rx_indicate(g_serial, serial_input_sem);
        rt_sem_init(&g_rx_sem, "uart_rx_sem", 0, RT_IPC_FLAG_FIFO);
    }

    if(g_test_mode == TEST_UART_MODE_MESSAGEQUEUE) {
        rt_device_set_rx_indicate(g_serial, serial_input_mq);
        rt_mq_init(&g_rx_mq, "rx_mq",
                   g_msg_pool,
                   sizeof(struct rx_msg),
                   sizeof(g_msg_pool),
                   RT_IPC_FLAG_FIFO);
    }
}

static int serial_send_msg(rt_device_t serial, char *msg)
{
#if TEST_UART_DEBUG
    printf("%s: %s len = %ld \n", __func__, msg, strlen(msg));
#endif

    return rt_device_write(serial, 0, msg, strlen(msg));
}

static int serial_receive_via_sem(rt_device_t serial, int len, char *buf)
{
    int ret = 0;
    int count = 0;

    while (1) {
        rt_sem_take(&g_rx_sem, 1000);
        ret = rt_device_read(serial, -1, buf, len);
#if TEST_UART_DEBUG
        printf("%s: %d: %s\n", __func__, ret, buf);
#endif
        if(ret > 0)
            break;

        count ++;
        if(count > TEST_UART_COUNT)
            break;
    }

    return ret;
}

static int serial_receive_via_mq(rt_device_t serial, int len, char *buf)
{
    struct rx_msg msg;
    int ret = 0;
    int count = 0;

    while (1) {
        rt_memset(&msg, 0, sizeof(msg));
        ret = rt_mq_recv(&g_rx_mq, &msg, sizeof(msg), 10000);
#if TEST_UART_DEBUG
        printf("%s: %d: %ld\n", __func__, ret, msg.size);
#endif
        if (ret == 0) {
            len = rt_device_read(msg.dev, 0, buf, msg.size);
            break;
        }
        count ++;
        if(count > 10)
            break;
    }
    return len;
}

static void serial_thread_entry(void *parameter)
{
    int ret = 0;
    char str_buf[TEST_UART_BUF_LEN + 1] = {0};

    serial_send_msg(g_serial,"Welcome to Uart Test \n");
    while (1) {
        memset(str_buf, 0 , TEST_UART_BUF_LEN);
        if(g_test_mode == TEST_UART_MODE_SEMAPHORE)
            ret = serial_receive_via_sem(g_serial, TEST_UART_BUF_LEN, str_buf);
        else if(g_test_mode == TEST_UART_MODE_MESSAGEQUEUE)
            ret = serial_receive_via_mq(g_serial, TEST_UART_BUF_LEN, str_buf);

        rt_thread_mdelay(10);
        if(ret >0) {
            if(strstr(str_buf, "exit")) {
                serial_send_msg(g_serial, "Bye");
                g_exit = 1;
            }
            else
                serial_send_msg(g_serial, str_buf);
        }

        if(g_exit)
            break;
    }
    printf("test_uart exit\n");
    rt_sem_detach(&g_rx_sem);
    rt_device_close(g_serial);
    rt_thread_delete(rt_thread_self());
}

int test_uart(int argc, char *argv[])
{
    int c = 0;
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX] = TEST_UART_PORT;

    g_test_mode = TEST_UART_MODE_SEMAPHORE;
    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'p':
            rt_strncpy(uart_name, optarg, RT_NAME_MAX);
            break;
        case 'h':
        default:
            test_uart_usage(argv[0]);
            return 0;
        }
    }

    g_serial = rt_device_find(uart_name);
    if (!g_serial)
    {
        printf("find %s failed!\n", uart_name);
        return -RT_ERROR;
    }

    serial_config_uart(g_serial);
    serial_init_variable();

    ret = rt_device_open(g_serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK)
    {
        printf("open %s failed !\n", uart_name);
        return -RT_ERROR;
    }

    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024*4, 25, 10);
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    } else {
        rt_device_close(g_serial);
        return -RT_ERROR;
    }

    printf("Test Ready: Please send msg to %s\n", uart_name);
    return 0;
}

MSH_CMD_EXPORT(test_uart, ArtInChip Uart Test);
