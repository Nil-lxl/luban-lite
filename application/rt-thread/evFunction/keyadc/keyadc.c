#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "hal_psadc.h"
#include "hal_syscfg.h"

#include "keyadc.h"

#define LOG_TAG "KEYADC"

#ifdef AIC_USING_HOT68_DEMO_A01_V0
#define KEYADC_TEST_CHANNLE     5
#else 
#define KEYADC_TEST_CHANNLE     7
#endif 

#define KEYADC_TEST_SCALE       200

static rt_adc_device_t gpai_device;
static rt_thread_t keyadc_thread;

#ifdef AIC_USING_D213ECV_EzUIX1_DEMO_V1
static int keyadc_voltage[] = { 400, 1100, 2000, 2680 };
// static int keyadc_voltage[] = { 460, 1160, 2090, 2740 };
#elif defined AIC_USING_H215_DEMO_A02_V0
static int keyadc_voltage[] = { 340, 700, 1240, 1560 };
#elif defined AIC_USING_HOT68_DEMO_A01_V0
static int keyadc_voltage[] = { 540, 1030, 1810, 2280 };
#elif defined AIC_USING_HT68_DEMO_A02_V1
static int keyadc_voltage[] = { 540, 1030, 1810, 2280 };
#endif

static int keyadc_flag[] = { 1, 2, 3, 4 };


void keyadc_device_enable(int channel) {
    gpai_device = (rt_adc_device_t)rt_device_find("gpai");
    if (gpai_device == RT_NULL) {
        LOG_E("GPAI设备未找到!\n");
    }
    rt_adc_enable(gpai_device, channel);
}

void keyadc_device_disable(int channel) {
    rt_adc_disable(gpai_device, channel);
}

key_flag_t keyadc_get_flag(int channel, int scale) {
    int adc_value;

    while (1) {
        adc_value = rt_adc_read(gpai_device, channel);
        for (int i = 0;i < sizeof(keyadc_voltage) / sizeof(keyadc_voltage[0]);i++) {
            // rt_kprintf("adc_value:%d\n", adc_value);
            if ((keyadc_voltage[i] - scale <= adc_value) && (adc_value <= keyadc_voltage[i] + scale)) {
                rt_thread_mdelay(300);
                return keyadc_flag[i];
            }
        }
    }

}

void keyadc_scan(int channel, int scale) {
    int key_flag;
    while (1) {
        key_flag = keyadc_get_flag(channel, scale);
        switch (key_flag) {
            case KEY_UP:
                rt_kprintf("KEY UP");
                break;
            case KEY_DOWN:
                rt_kprintf("KEY DOWN");
                break;
            case KEY_LEFT:
                rt_kprintf("KEY LEFT");
                break;
            case KEY_RIGHT:
                rt_kprintf("KEY RIGHT");
                break;
            default:
                break;
        }
    }
}

void keyadc_test_thread_entry(void *param) {
    int channel = KEYADC_TEST_CHANNLE;
    int scale = KEYADC_TEST_SCALE;
    keyadc_device_enable(channel);
    keyadc_scan(channel, scale);

}

void keyadc_test(void) {
    if (keyadc_thread) {
        LOG_W("keyadc_thread is running");
    } else {
        keyadc_thread = rt_thread_create("keyadc_thread", keyadc_test_thread_entry, NULL, 4 * 1024, 20, 10);
        rt_thread_startup(keyadc_thread);
    }
}

void keyadc_test_off(void) {
    if (keyadc_thread) {
        rt_thread_delete(keyadc_thread);
        keyadc_thread = RT_NULL;
    } else {
        LOG_W("keyadc_thread is not running");
    }
}


MSH_CMD_EXPORT(keyadc_test, keyadc_test_cmd);
MSH_CMD_EXPORT(keyadc_test_off, keyadc_test_off_cmd);


#if 0   //use psadc

#define PSADC_DEFAULT_VOLTAGE   3
#define PSADC_VOLTAGE_ACCURACY  10000       //精度

static rt_adc_device_t psadc_device;

int psdac_get_adc(float voltage, int sample_num) {

    u32 adc_values[AIC_PSADC_CH_NUM];
    int ref_voltage = 0;        //参考电压
    int channel_count = 0;      //使用的adc通道数量
    int count = 0;
    int sample_num = 10;        //默认采样次数
    u64 start_us, end_us;       //采样开始与结束时间

    psadc_device = (rt_adc_device_t)rt_device_find("psadc");
    if (psadc_device == RT_NULL) {
        LOG_E("PSADC 设备未找到!\n");
        return -RT_ERROR;
    }

    ref_voltage = hal_syscfg_read_ldo_cfg();

    if (!ref_voltage) {
        LOG_E("通过eFuse读取参考电压失败\n");
        ref_voltage = (int)(voltage)*PSADC_VOLTAGE_ACCURACY;
    }
    LOG_I("参考电压为 %d.%02d V", ref_voltage / PSADC_VOLTAGE_ACCURACY, ref_voltage % PSADC_VOLTAGE_ACCURACY);

    rt_adc_enable(psadc_device, 0);

    channel_count = rt_adc_control(psadc_device, RT_ADC_CMD_GET_CHAN_COUNT, NULL);
    LOG_I("PSADC采样次数: %d, 通道数量: %d", sample_num, channel_count);
    while (count < sample_num) {
        count++;
        start_us = aic_get_time_us();
        rt_adc_control(psadc_device, RT_ADC_CMD_GET_VALUES_POLL, (void *)adc_values);
        end_us = aic_get_time_us();
        LOG_I("采样时间: %d us", abs(end_us - start_us));


    }

    rt_adc_disable(psadc_device, 0);
    return -RT_ERROR;

}
#endif 

