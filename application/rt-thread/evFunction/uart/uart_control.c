#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "rtdef.h"

#include "uart_control.h"

#define LOG_TAG "Uart"
#define UART_DEVICE_NAME APP_USE_UART_DEVICE

static rt_thread_t uart_thread;
static rt_device_t uart_device;

static struct rt_sem_t *uart_rx_sem;


int uart_config(rt_device_t dev) {
    rt_serial_t *uart = container_of(dev, rt_serial_t, parent);
    uart->config.baud_rate = BAUD_RATE_115200;
    uart->config.data_bits = DATA_BITS_8;
    uart->config.stop_bits = STOP_BITS_1;
    uart->config.parity = PARITY_NONE;

    if (rt_device_control(dev, RT_DEVICE_CTRL_CONFIG, &uart->config) != RT_EOK) {
        LOG_E("uart set config failed!");
        return -1;
    }

    return 0;
}
rt_err_t uart_rx_cb(rt_device_t dev, rt_size_t size) {
    if (size > 0) {
        rt_sem_release(uart_rx_sem);
    }
    return RT_EOK;
}

void uart_thread_entry(void *param) {
    const char *msg = "uart thread start\n";
    int buf_size=1024;

    void *read_buf = rt_malloc(buf_size);

    rt_device_write(uart_device, 0, msg, strlen(msg));

    while (1) {
        memset(read_buf, 0, buf_size);

        rt_sem_take(uart_rx_sem, RT_WAITING_FOREVER);
        int ret = rt_device_read(uart_device, -1, read_buf, buf_size);

        rt_thread_mdelay(10);

        if (ret > 0) {
            // rt_device_write(uart_device, 0, read_buf, strlen(read_buf));
            LOG_I("receive data:%s ,read size:%d", read_buf, ret);
        }

    }
    LOG_I("Uart thread exit");
    rt_sem_detach(uart_rx_sem);
    rt_thread_delete(uart_thread);
}

void uart_startup(void) {
    int ret = 0;
    /* create semaphore */
    uart_rx_sem = rt_sem_create("uart_rx_sem", 0, RT_IPC_FLAG_FIFO);
    if (uart_rx_sem == RT_NULL) {
        LOG_E("uart rx sem create failed!");
        abort();
    }
    
    /* find uart device */
    uart_device = rt_device_find(UART_DEVICE_NAME);
    if (!uart_device) {
        LOG_E("%s device not found",UART_DEVICE_NAME);
        abort();
    }
    /* set uart device configuration */
    uart_config(uart_device);

    /* open uart device */
    ret = rt_device_open(uart_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        LOG_E("uart open failed");
        abort();
    }
    
    /* set uart rx indicate callback */
    rt_device_set_rx_indicate(uart_device, uart_rx_cb);

    uart_thread = rt_thread_create("app_uart", uart_thread_entry, RT_NULL, 4 * 1024, 20, 10);
    if (uart_thread != RT_NULL) {
        rt_thread_startup(uart_thread);
    }

}