#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "rtdef.h"
#include "aic_hal_can.h"

#define LOG_TAG "CAN"
#define CAN_DEV_NAME "can0"

static rt_sem_t can_rx_sem;
static rt_device_t can_dev;
static rt_thread_t can_rx_thread;
static rt_thread_t can_tx_thread;

static rt_err_t can_rx_cb(rt_device_t dev, rt_size_t size) {
    rt_sem_release(can_rx_sem);
    return RT_EOK;
}
static void can_rx_entry(void *param) {
    rt_size_t size;
    struct rt_can_msg msg = { 0 };
    // rt_can_msg_t msg = { 0 };
    while (1) {
        rt_sem_take(can_rx_sem, RT_WAITING_FOREVER);

        msg.hdr = -1;
        size = rt_device_read(can_dev, 0, &msg, sizeof(msg));
        if (size) {
            rt_kprintf("ID: %X\n", msg.id);
        }
        if (msg.len) {
            rt_kprintf("DATA: ");
            for (int i = 0;i < msg.len;i++) {
                rt_kprintf("%02X ", msg.data[i]);
            }
        }
        rt_kprintf("\n");
        // rt_device_close(can_dev);
    }
}

void can_start(void) {
    rt_err_t ret = 0;
    can_rx_sem = rt_sem_create("can_rx_sem", 0, RT_IPC_FLAG_FIFO);

    can_dev = rt_device_find(CAN_DEV_NAME);
    if (can_dev == RT_NULL) {
        LOG_E("CAN Device %S Not Found!", CAN_DEV_NAME);
        return;
    }

    ret = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        LOG_E("CAN Device Open Failed!");
        return;
    }

    ret = rt_device_control(can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN1MBaud);
    if (ret != RT_EOK) {
        LOG_E("CAN Device Set Baudrate Failed!");
        return;
    }

    rt_device_control(can_dev, RT_DEVICE_CTRL_SET_INT, NULL);
    rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)CAN_SELFTEST_MODE);

    rt_device_set_rx_indicate(can_dev, can_rx_cb);

    can_rx_thread = rt_thread_create("can_rx", can_rx_entry, NULL, 2 * 1024, 20, 10);
    if (can_rx_thread != RT_NULL) {
        rt_thread_startup(can_rx_thread);
    }
}
