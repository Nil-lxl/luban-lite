/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

struct rt_device_pwm *pwm_dev = RT_NULL;
rt_device_t qep_dev = RT_NULL;
rt_uint32_t pwm_ch;
rt_thread_t tid;
rt_uint32_t pulse_count;

/* callback function */
static rt_err_t qep_cb(rt_device_t dev, void *buffer)
{
    rt_pwm_disable(pwm_dev, pwm_ch);

    rt_device_close(qep_dev);

#ifdef ULOG_USING_ISR_LOG
    rt_uint32_t *temp = (rt_uint32_t *)buffer;
    rt_kprintf("qep %d callback\n", temp[0]);
#endif

    return RT_EOK;
}

static void qep_thread(void *parameter)
{
    rt_uint32_t read_data = 0;

    while (1) {
        rt_device_read(qep_dev, 0, &read_data, 0);

        if (rt_get_errno() < 0)
            break;

        rt_kprintf("this is return pluse cnt:%d/%d\n", read_data, pulse_count);

        rt_thread_mdelay(200);
    }

    rt_thread_delete(tid);
}

int test_qep(int argc, char **argv)
{
    char device_name[8] = {"qep"};
    rt_uint32_t period_ns, duty_ns;
    int ret;

    if (argc < 6) {
        rt_kprintf("usage:test_qep <qep_channel> <pwm_channel> <period_ns> <duty_ns> <pulse_count>\n");
        rt_kprintf("example: test_qep 1 3 10000 5000 10\n");
        return -RT_ERROR;
    }

    strcat(device_name, argv[1]);

    /* qep configuration */
    qep_dev =  rt_device_find(device_name);
    if (qep_dev == NULL) {
        rt_kprintf("can't find %s device!\n", device_name);
        return -RT_ERROR;
    }

    //set the qep callback function
    rt_device_set_tx_complete(qep_dev, qep_cb);

    ret = rt_device_open(qep_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to open %s device!\n", device_name);
        return ret;
    }

    tid = rt_thread_create("test-qep", qep_thread, RT_NULL,
                           1024, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    else
        rt_kprintf("create test-qep thread err!\n");

    //set the pulse count
    pulse_count = atoi(argv[5]);
    ret = rt_device_control(qep_dev, PULSE_ENCODER_CMD_SET_COUNT, &pulse_count);
    if (ret != RT_EOK) {
        rt_kprintf("Failed to set the pulse count of %s device!\n", device_name);
        return ret;
    }

    rt_thread_mdelay(10);

    /* pwm configuration */
    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");

    if (pwm_dev == NULL) {
        rt_kprintf("can't find pwm device!\n");
        return -RT_ERROR;
    }

    pwm_ch = atoi(argv[2]);
    period_ns = atoi(argv[3]);
    duty_ns = atoi(argv[4]);

    rt_pwm_set(pwm_dev, pwm_ch, period_ns, duty_ns);
    rt_pwm_enable(pwm_dev, pwm_ch);

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(test_qep, test_qep, Test the qep);
