#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "rtdef.h"

#include "drv_cir.h"

#define LOG_TAG "CIR"

static rt_thread_t cir_thread;
static rt_device_t cir_dev;
static rt_sem_t cir_rx_sem;

static rt_err_t cir_rx_cb(rt_device_t dev, rt_size_t size) {
    rt_sem_release(cir_rx_sem);
    return RT_EOK;
}

static void cir_thread_entry(void *param) {
    uint32_t read_buf = 0;
    rt_size_t size;

    while (1) {
        rt_sem_take(cir_rx_sem, RT_WAITING_FOREVER);
        size = rt_device_read(cir_dev, 0, &read_buf, sizeof(read_buf));
        if (size) {
            switch (read_buf) {
                case CIR_CMD_SHUTDOWN:
                    rt_hw_cpu_reset();
                    break;
                case CIR_CMD_MENU:
                    LOG_I("MENU");
                    break;
                case CIR_CMD_BACK:
                    LOG_I("BACK");
                    break;
                case CIR_CMD_CLEAR:
                    LOG_I("CLEAR");
                    break;
                case CIR_CMD_TEST:
                    LOG_I("TEST");
                    break;
                case CIR_CMD_PLUS:
                    LOG_I("+");
                    break;
                case CIR_CMD_MINUS:
                    LOG_I("-");
                    break;
                case CIR_CMD_PREV:
                    LOG_I("PREV");
                    break;
                case CIR_CMD_NEXT:
                    LOG_I("NEXT");
                    break;
                case CIR_CMD_PLAY:
                    LOG_I("PLAY");
                    break;
                    
            }
            // rt_kprintf("cir received data: 0x%x", read_buf);
        }
        rt_thread_mdelay(100);
    }
}
void test_cir(void) {
    int ret = 0;
    cir_config_t cir_config = {
        .protocol = CIR_PROTOCOL_NEC,
        .tx_duty = 33,
        .rx_level = 1,
    };

    cir_dev = rt_device_find("cir");
    if (cir_dev) {
        LOG_I("Find device CIR");
    } else {
        LOG_E("CIR Device Not Found");
    }

    ret = rt_device_open(cir_dev, RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR);
    if (ret == RT_EOK) {
        LOG_I("open CIR Device success");
    } else {
        LOG_E("open CIR Device failed");
    }


    ret = rt_device_control(cir_dev, IOC_CIR_CONFIGURE, (void *)&cir_config);
    if (ret == RT_EOK) {
        LOG_I("CIR Device configure failed");
    } else {
        LOG_E("CIR Device configure success");
    }

    cir_rx_sem = rt_sem_create("cir_sem", 0, RT_IPC_FLAG_PRIO);
    if (cir_rx_sem == RT_NULL) {
        LOG_E("uart rx sem create failed!");
    }

    rt_device_set_rx_indicate(cir_dev, cir_rx_cb);
    LOG_I("CIR Device Initialized");

    cir_thread = rt_thread_create("cir_thread", cir_thread_entry, RT_NULL, 2 * 1024, 18, 10);
    if (cir_thread != RT_NULL) {
        rt_thread_startup(cir_thread);
    } else {
        LOG_E("CIR Thread Create Failed");
    }
}

MSH_CMD_EXPORT(test_cir, cir test start);


