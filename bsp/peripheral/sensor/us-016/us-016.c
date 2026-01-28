#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include "rtthread.h"
#include "rtdevice.h"

#include "hal_adcim.h"
#include "aic_core.h"
#include "aic_log.h"
#include "hal_gpai.h"
#include "aic_hal_gpio.h"

#define ADC_DEVICE_NAME     "gpai"
// adc通道
#define ADC_CHANNEL         1
// 电压基准
#define VREF_ADC_HSPI       3.3

static rt_adc_device_t gpai_device;


/**
 * 初始化KEYADC设备
 */
int us016_init(void) {
    gpai_device = (rt_adc_device_t)rt_device_find("gpai");
    if (gpai_device == RT_NULL) {
        LOG_E("GPAI设备未找到!\n");
    }
    rt_adc_enable(gpai_device, ADC_CHANNEL);
}

int US016_DeInit(void) {
    int ret = rt_adc_disable(gpai_device, ADC_CHANNEL);
    if (ret != RT_EOK) {
        LOG_E("Failed to [rt_adc_disable] !!!");

        return -RT_ERROR;
    }

    return RT_EOK;
}

/**********************************************************
 * 函 数 名 称：US016_Get_Value
 * 函 数 功 能：adc获得的电压
 * 传 入 参 数：无
 * 函 数 返 回：读取的电压
 * 作       者：LC
 * 备       注：ADC每个时间
 **********************************************************/
static float US016_Get_Value(void) {
    int value = 0; // 累计读取的数据
    int count = 5; // 采集次数
    int valid_count = 0; // 有效读取次数
    int return_Value = 0; // 分压之后还原的数据
    float voltage_calculation = 0.0; // 电压计算缓存区

    while (count--) {
        uint32_t temp = rt_adc_read(gpai_device, ADC_CHANNEL);
        if ((temp != 0) && (temp < 4096)) // 确保不会把校验数据计算进来
        {
            // rt_kprintf("[%d]adc temp = [%d]\n",valid_count+1,temp);

            value += temp;
            valid_count++;
        }

        aicos_mdelay(5); // 延时5ms
    }

    // 如果没有有效的读取
    if (!valid_count) {
        return -RT_ERROR; // 返回一个错误代码
    }

    return_Value = value / valid_count; // 计算平均值

    voltage_calculation = (VREF_ADC_HSPI / 4095.0) * return_Value; // 换算成电压

    // 返回电压值
    // 因为电压分压为了二分之一所以需要还原
    return voltage_calculation * 2;
}



/**********************************************************
 * 函 数 名 称：US016_Get_distance
 * 函 数 功 能：读取测距距离
 * 传 入 参 数：无
 * 函 数 返 回：浮点型的测距的距离
 * 作       者：LCKFB
 * 备       注：

量程为3米时距离公式为：L = (A*3072/4096)*（Vref/Vcc）
量程为1米时距离公式为：L = (A*1024/4096)*（Vref/Vcc）
Vref 为 ADC 的参考电压，Vcc 为 US-016 的电源电压

量程为3米时距离公式为：L = 3072 *（Vadc/Vcc）
量程为1米时距离公式为：L = 1024 *（Vadc/Vcc）
Vadc 为 ADC 读到的电压，Vcc 为 US-016 的电源电压
**********************************************************/
float US016_Get_distance(void) {
    float distance = 0;

    float d = US016_Get_Value();
    if (d < 0) {
        return -RT_ERROR;
    }
    // rt_kprintf("US016_Get_Value = 【%d.%02d】\n",(d*100)/100, ((int)(d*100))%100);

#if !RANGE
    distance = 3072 * (d / 3.3);
#else
    distance = 1024 * (d / 3.3);
#endif

    return distance;
}


static rt_thread_t us016_thread = RT_NULL; // 线程控制块

// 线程入口函数
static void us016_thread_entry(void *param) {
    int while_count = 1;

    while (while_count++) {
        float dis = US016_Get_distance();
        if (dis < 0) {
            LOG_E("failed to US016_Get_distance!!");
            continue;
        }

        uint32_t value = dis * 100;

        rt_kprintf("\n");

        rt_kprintf("Read Distance = %d.%02dcm\n", (value / 100) / 10, (value % 100) / 10);   // 距离

        rt_kprintf("\n");

        if (while_count >= 100) {
            while_count = 1;

            rt_kprintf("\nType [test_exit_us016_sensor] command to exit \n");
            rt_kprintf("Note: Pressing [TAB] as you type will autocomplete the command\n");

            rt_thread_mdelay(2000);
        }

        rt_thread_mdelay(1000);
    }

}

static void test_us016(int argc, char **argv) {
    int ret = us016_init();
    if (ret != RT_EOK) {
        LOG_E("Failed to [us016_init] !!!");
        return;
    }
    us016_thread = rt_thread_create("us016_thread", us016_thread_entry, RT_NULL, 1024, 25, 5);
    if (us016_thread != RT_NULL)
        rt_thread_startup(us016_thread);
}
// 导出函数为命令
MSH_CMD_EXPORT(test_us016, us016 sensor test);


/* 退出函数 */
void test_exit_us016(void) {
    int ret = rt_thread_delete(us016_thread);
    if (ret != RT_EOK) {
        LOG_E("failed to test_exit_us016_sensor !!");
        return;
    }
    ret = US016_DeInit();
    if (ret != RT_EOK) {
        LOG_E("Failed to [US016_DeInit] !!!");
        return;
    } else {
        rt_kprintf("\nUS016_DeInit successful!!!\n");
    }
}
// 导出函数为命令
MSH_CMD_EXPORT(test_exit_us016, exit us016 sensor test);