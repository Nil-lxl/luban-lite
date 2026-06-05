/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-16     Lenoyan      the first version
 */

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <ipc/ringbuffer.h>

#include <zephyr/drivers/bt_uart.h>
#include <aic_utils.h>
#include "hci_init.h"

#define BT_UART_RX_FIFO_SIZE    4096
#define BT_UART_TX_FIFO_SIZE    4096

/* Dedicated workqueue for Bluetooth UART processing */
#define BT_UART_WORKQ_STACK_SIZE    4096
#define BT_UART_WORKQ_PRIORITY      8

static struct rt_workqueue *bt_uart_workq = NULL;
static struct rt_work rx_work;
static struct rt_work tx_work;

static struct rt_device *uart;
static struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

static int irq_rx_enable = 0, irq_tx_enable = 0;
static uart_irq_callback_user_data_t uart_irq_callback = NULL;
static void *uart_irq_callback_user_data = NULL;
static struct rt_ringbuffer *rx_ringbuf = NULL;
static struct rt_ringbuffer *tx_ringbuf = NULL;

int uart_fifo_read(const struct rt_device *uart, u8 *data, const int size)
{
    int ret;

    if (!rx_ringbuf || !data || size <= 0) {
        return -1;
    }

    ret = rt_ringbuffer_get(rx_ringbuf, data, size);
    if (ret < size) {
        /* Normal if buffer has less data than requested, but log if significant */
        if (ret == 0 && rt_ringbuffer_data_len(rx_ringbuf) == 0) {
            /* Buffer empty, expected behavior */
        }
    }
    //hexdump_msg("read:", data, ret, 1);

    return ret;
}

void bt_uart_drain(const struct rt_device *uart)
{
    uint8_t c;

    while (uart_fifo_read(uart, &c, 1) > 0) {
        continue;
    }
}

int uart_fifo_fill(const struct rt_device *uart, const u8 *data, int size)
{
    size_t wrote;
    uint8_t tx_buf[256];
    rt_size_t to_send;

    if (!tx_ringbuf || !data || size <= 0) {
        return 0;
    }

    //hexdump_msg("write:", data, size, 1);

    /* Put data to ringbuffer for tracking */
    wrote = rt_ringbuffer_put(tx_ringbuf, data, size);
    if (wrote < size) {
        printf("[BT_UART] TX ringbuffer overflow! wrote %d/%d, space %d\n",
               (int)wrote, size, (int)rt_ringbuffer_space_len(tx_ringbuf));
    }

    /* Immediately drain TX ringbuffer to UART (synchronous send) */
    while (irq_tx_enable && tx_ringbuf) {
        to_send = rt_ringbuffer_data_len(tx_ringbuf);
        if (to_send == 0) {
            break;
        }
        to_send = to_send > sizeof(tx_buf) ? sizeof(tx_buf) : to_send;
        to_send = rt_ringbuffer_get(tx_ringbuf, tx_buf, to_send);
        if (to_send == 0) {
            break;
        }
        rt_device_write(uart, 0, tx_buf, to_send);
    }

    return (int)wrote;
}

int uart_poll_out(const struct rt_device *dev, u8 c)
{
    //hexdump_msg("write:", &c, 1, 1);
    return rt_device_write(uart, 0, &c, 1);
}

void uart_irq_rx_enable(struct rt_device *uart)
{
	irq_rx_enable = 1;
	/* Submit RX work to dedicated workqueue if not already pending */
	if (bt_uart_workq) {
		rt_err_t ret = rt_workqueue_submit_work(bt_uart_workq, &rx_work, 0);
		if (ret == -RT_EBUSY) {
			/* Work already in queue, skip */
		}
	}
}

void uart_irq_rx_disable(struct rt_device *uart)
{
    irq_rx_enable = 0;
}

int uart_irq_rx_status_get(struct rt_device *uart)
{
    return irq_rx_enable;
}

void uart_irq_tx_enable(struct rt_device *uart)
{
	irq_tx_enable = 1;
	/* Submit TX work to dedicated workqueue if not already pending */
	if (bt_uart_workq) {
		rt_err_t ret = rt_workqueue_submit_work(bt_uart_workq, &tx_work, 0);
		if (ret == -RT_EBUSY) {
			/* Work already in queue, skip */
		}
	}
}

void uart_irq_tx_disable(struct rt_device *uart)
{
    irq_tx_enable = 0;
}

int uart_irq_rx_ready(struct rt_device *uart)
{
    if (!rx_ringbuf) {
        return 0;
    }

    return (rt_ringbuffer_data_len(rx_ringbuf) > 0) && irq_rx_enable;
}

int uart_irq_tx_ready(struct rt_device *uart)
{
    if (!tx_ringbuf) {
        return 0;
    }

    return (rt_ringbuffer_space_len(tx_ringbuf) > 0) && irq_tx_enable;
}

/*
 * uart_irq_update - Update/ACK interrupt status in ISR
 *
 * This function should be called at the beginning of the ISR.
 * For RT-Thread UART driver, this is a no-op as the driver
 * handles interrupt acknowledgment internally.
 *
 * Returns: 1 on success
 */
int uart_irq_update(struct rt_device *uart)
{
    (void)uart;
    /* RT-Thread UART driver handles ACK internally */
    return 1;
}

/*
 * uart_irq_is_pending - Check if UART interrupt is pending
 *
 * Returns: 1 if RX or TX interrupt is pending and enabled
 *          0 if no interrupt is pending
 */
int uart_irq_is_pending(struct rt_device *uart)
{
    (void)uart;

    /* Check if either RX or TX interrupt is ready */
    return uart_irq_rx_ready(uart) || uart_irq_tx_ready(uart);
}

int uart_irq_callback_user_data_set(const struct rt_device *uart, uart_irq_callback_user_data_t cb,
        void *user_data)
{
    uart_irq_callback = cb;
    uart_irq_callback_user_data = user_data;
    return 0;
}

/* RX work handler - called from dedicated workqueue */
static void bt_uart_rx_work_handler(struct rt_work *work, void *work_data)
{
    (void)work;
    (void)work_data;

    /* Call the registered callback to process RX data */
    if (irq_rx_enable && uart_irq_callback) {
        uart_irq_callback(uart, uart_irq_callback_user_data);
    }
}

/* TX work handler - called from dedicated workqueue */
static void bt_uart_tx_work_handler(struct rt_work *work, void *work_data)
{
    (void)work;
    (void)work_data;

    /* Call the registered callback to process TX data */
    if (irq_tx_enable && uart_irq_callback) {
        uart_irq_callback(uart, uart_irq_callback_user_data);
    }
}

/* RX indicate callback - called from UART ISR context */
static rt_err_t uart_rx_ind(rt_device_t dev, rt_size_t size)
{
    uint8_t temp_buf[256];
    rt_size_t to_read, dlen;
    static uint32_t rx_overflow_count = 0;
    static uint32_t rx_total_count = 0;

    if (!rx_ringbuf || !bt_uart_workq) {
        return RT_EOK;
    }

    rx_total_count++;

    /* Check available space in ringbuffer */
    rt_size_t space = rt_ringbuffer_space_len(rx_ringbuf);
    rt_size_t data_len = rt_ringbuffer_data_len(rx_ringbuf);

    /* Read data from UART device */
    to_read = size > sizeof(temp_buf) ? sizeof(temp_buf) : size;
    to_read = to_read > space ? space : to_read;

    if (to_read == 0) {
        rx_overflow_count++;
        if (rx_overflow_count % 100 == 1) {
            printf("[BT_UART] RX ringbuffer full! total=%lu, overflow=%lu, data=%d/%d\n",
                   rx_total_count, rx_overflow_count, (int)data_len, BT_UART_RX_FIFO_SIZE);
        }
        return RT_EOK;
    }

    dlen = rt_device_read(uart, 0, temp_buf, to_read);
    if (dlen > 0) {
        rt_size_t put_len = rt_ringbuffer_put(rx_ringbuf, temp_buf, dlen);
        if (put_len < dlen) {
            rx_overflow_count++;
            printf("[BT_UART] RX ringbuffer overflow! put %d, dropped %d, total=%lu\n",
                   (int)put_len, (int)(dlen - put_len), rx_overflow_count);
        }

        /* Submit RX work to dedicated workqueue (do not process in ISR) */
        if (irq_rx_enable) {
            rt_err_t ret = rt_workqueue_submit_work(bt_uart_workq, &rx_work, 0);
            if (ret == -RT_EBUSY) {
                /* Work already in queue, will be processed */
            }
        }
    }

    return RT_EOK;
}

int bt_uart_baudrate_update(uint32_t baudrate)
{
    config.baud_rate = baudrate;
    if (rt_device_control(uart, RT_SERIAL_SET_BAUDRATE, &baudrate)) {
        printf("[BT_UART] update baudrate failed.\n");
        return -1;
    }

    return 0;
}

int bt_uart_flow_control_update(uint32_t flow_control)
{
    if (flow_control) {
        config.flowcontrol = RT_SERIAL_FLOWCONTROL_CTSRTS;
        config.function = RT_SERIAL_RS232_AUTO_FLOW_CTRL;

        if (rt_device_control(uart, RT_DEVICE_CTRL_CONFIG, &config)) {
            printf("[BT_UART] update flow control failed.\n");
            return -1;
        }
    }

    return 0;
}

__WEAK int bt_ctlr_init(void)
{
    return 0;
}

int bt_uart_init(void)
{
    /* Create dedicated workqueue for Bluetooth UART processing */
    bt_uart_workq = rt_workqueue_create("bt_uart_wq",
                                        BT_UART_WORKQ_STACK_SIZE,
                                        BT_UART_WORKQ_PRIORITY);
    if (!bt_uart_workq) {
        printf("[BT_UART] create workqueue failed!\n");
        return -1;
    }

    /* Initialize RX and TX work items */
    rt_work_init(&rx_work, bt_uart_rx_work_handler, NULL);
    rt_work_init(&tx_work, bt_uart_tx_work_handler, NULL);

    /* Create RX ringbuffer */
    rx_ringbuf = rt_ringbuffer_create(BT_UART_RX_FIFO_SIZE);
    if (!rx_ringbuf) {
        printf("[BT_UART] create RX ringbuffer failed!\n");
        rt_workqueue_destroy(bt_uart_workq);
        bt_uart_workq = NULL;
        return -1;
    }

    /* Create TX ringbuffer */
    tx_ringbuf = rt_ringbuffer_create(BT_UART_TX_FIFO_SIZE);
    if (!tx_ringbuf) {
        printf("[BT_UART] create TX ringbuffer failed!\n");
        rt_workqueue_destroy(bt_uart_workq);
        bt_uart_workq = NULL;
        rt_ringbuffer_destroy(rx_ringbuf);
        rx_ringbuf = NULL;
        return -1;
    }

    uart = rt_device_find(LPKG_BT_UART);
    if (!uart) {
        printf("[BT_UART] find %s failed!\n", LPKG_BT_UART);
        rt_workqueue_destroy(bt_uart_workq);
        bt_uart_workq = NULL;
        rt_ringbuffer_destroy(rx_ringbuf);
        rt_ringbuffer_destroy(tx_ringbuf);
        rx_ringbuf = NULL;
        tx_ringbuf = NULL;
        return -1;
    }

    if (rt_device_control(uart, RT_SERIAL_GET_CONFIG, &config) != RT_EOK) {
        printf("[BT_UART] get config failed!\n");
        rt_workqueue_destroy(bt_uart_workq);
        bt_uart_workq = NULL;
        rt_ringbuffer_destroy(rx_ringbuf);
        rt_ringbuffer_destroy(tx_ringbuf);
        rx_ringbuf = NULL;
        tx_ringbuf = NULL;
        return -1;
    }

    config.baud_rate = BAUD_RATE_115200;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
#if defined(CONFIG_BT_H5)
    config.parity    = PARITY_EVEN;
#else
    config.baud_rate = BAUD_RATE_1500000;
    config.parity    = PARITY_NONE;
    config.flowcontrol = RT_SERIAL_FLOWCONTROL_CTSRTS;
    config.function = RT_SERIAL_RS232_AUTO_FLOW_CTRL;
#endif

    if (rt_device_control(uart, RT_DEVICE_CTRL_CONFIG, &config)) {
        printf("[BT_UART] set config failed!\n");
        rt_workqueue_destroy(bt_uart_workq);
        bt_uart_workq = NULL;
        rt_ringbuffer_destroy(rx_ringbuf);
        rt_ringbuffer_destroy(tx_ringbuf);
        rx_ringbuf = NULL;
        tx_ringbuf = NULL;
        return -1;
    }

    /* Set RX indicate callback */
    rt_device_set_rx_indicate(uart, uart_rx_ind);

#ifdef RT_USING_SERIAL_V2
    rt_device_open(uart, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
#else
    rt_device_open(uart, RT_DEVICE_FLAG_INT_RX);
#endif


    bt_ctlr_init();

    printf("[BT_UART] initialized, workq priority %d, RX buf %d bytes, TX buf %d bytes\n",
           BT_UART_WORKQ_PRIORITY, BT_UART_RX_FIFO_SIZE, BT_UART_TX_FIFO_SIZE);

    return 0;
}
INIT_APP_EXPORT(bt_uart_init);
