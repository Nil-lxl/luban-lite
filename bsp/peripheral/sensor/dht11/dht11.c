#include "rtthread.h"
#include "rtdevice.h"
#include <stdio.h>
#include <getopt.h>
#include <sys/time.h>
#include "aic_core.h"
#include "aic_hal_gpio.h"

#define GPIO_DHT11  rt_pin_get("PD.0")

#define uchar   unsigned char
#define uint8   unsigned char
#define uint16  unsigned short

static float temperature = 0;
static float humidity = 0;

static void delay_ms(uint16 ms) {
    rt_thread_mdelay(ms);
}

static void delay_us(uint16 us) {
    aicos_udelay(us);
}


//温湿度定义
static uchar ucharFLAG, uchartemp;
static float Humi, Temp;
static uchar ucharT_data_H, ucharT_data_L, ucharRH_data_H, ucharRH_data_L, ucharcheckdata;
static uchar ucharT_data_H_temp, ucharT_data_L_temp, ucharRH_data_H_temp, ucharRH_data_L_temp, ucharcheckdata_temp;
static uchar ucharcomdata;

static uchar Humi_small;
static uchar Temp_small;

static void InputInitial(void)//设置为输入
{
    rt_pin_mode(GPIO_DHT11, PIN_MODE_INPUT);
}

static void OutputHigh(void)//输出1
{
    rt_pin_mode(GPIO_DHT11, PIN_MODE_OUTPUT);
    rt_pin_write(GPIO_DHT11, PIN_HIGH);
}

static void OutputLow(void)//输出0
{
    rt_pin_mode(GPIO_DHT11, PIN_MODE_OUTPUT);
    rt_pin_write(GPIO_DHT11, PIN_LOW);
}

static uint8 getData()//读取状态
{
    rt_pin_mode(GPIO_DHT11, PIN_MODE_INPUT);

    return rt_pin_read(GPIO_DHT11);
}

//读取一个字节数据
static void COM(void) {
    uchar i;
    for (i = 0;i < 8;i++) {
        ucharFLAG = 2;
        //等待IO口变低，变低后，通过延时去判断是0还是1
        while ((getData() == 0) && ucharFLAG++) delay_us(10);

        delay_us(35);//延时35us
        uchartemp = 0;

        //如果这个位是1，35us后，还是1，否则为0
        if (getData() == 1)
            uchartemp = 1;

        ucharFLAG = 2;

        //等待IO口变高，变高后，表示可以读取下一位
        while ((getData() == 1) && ucharFLAG++)
            delay_us(10);

        if (ucharFLAG == 1)
            break;

        ucharcomdata <<= 1;
        ucharcomdata |= uchartemp;
    }
}


int DHT11_Read(float *read_buff)   //温湿传感启动
{

    OutputLow();
    delay_ms(19);  //>18MS

    OutputHigh();

    InputInitial(); //输入

    delay_us(30);

    if (!getData())//表示传感器拉低总线
    {
        ucharFLAG = 2;
        //等待总线被传感器拉高
        while ((!getData()) && ucharFLAG++)
            delay_us(10);
        //等待总线被传感器拉低
        while ((getData()) && ucharFLAG++)
            delay_us(10);

        COM();//读取第1字节，
        ucharRH_data_H_temp = ucharcomdata;
        COM();//读取第2字节，
        ucharRH_data_L_temp = ucharcomdata;
        COM();//读取第3字节，
        ucharT_data_H_temp = ucharcomdata;
        COM();//读取第4字节，
        ucharT_data_L_temp = ucharcomdata;
        COM();//读取第5字节，
        ucharcheckdata_temp = ucharcomdata;
        OutputHigh();
        //判断校验和是否一致
        uchartemp = (ucharT_data_H_temp + ucharT_data_L_temp + ucharRH_data_H_temp + ucharRH_data_L_temp);
        if (uchartemp == ucharcheckdata_temp) {
            //校验和一致，
            ucharRH_data_H = ucharRH_data_H_temp; // 湿度高8
            ucharRH_data_L = ucharRH_data_L_temp; // 湿度低8
            ucharT_data_H = ucharT_data_H_temp;   // 温度高8
            ucharT_data_L = ucharT_data_L_temp;   // 温度低8
            ucharcheckdata = ucharcheckdata_temp;

           //保存温度和湿度
            Humi = ucharRH_data_H;
            Humi_small = ucharRH_data_L * 0.1;
            Humi = Humi + Humi_small;

            Temp = ucharT_data_H;
            Temp_small = ucharT_data_L * 0.1;
            Temp = Temp + Temp_small;

            read_buff[0] = Temp; // 温度
            read_buff[1] = Humi; // 湿度

            return RT_EOK;
        } else // 校验失败
        {
            Humi = 100;
            Temp = 100;

            return RT_ERROR;
        }
    } else //没用成功读取
    {
        Humi = 0,
            Temp = 0;

        return RT_ERROR;
    }
}

#define THREAD_PRIORITY         25      // 线程优先级
#define THREAD_STACK_SIZE       4096    // 线程大小
#define THREAD_TIMESLICE        10      // 时间片


static rt_thread_t dht11_thread = RT_NULL; // 线程控制块

// DHT11读取次数
// 默认读取50次
static int read_num = 50;

// 线程入口函数
static void dht11_thread_entry(void *param) {
    float read_buff[2] = { 0 };

    while (read_num--) {
        rt_memset(read_buff, 0, sizeof(read_buff));

        int ret = DHT11_Read(read_buff);
        if (ret == RT_EOK) {
            rt_kprintf("\n===========[%d]=============\n", read_num + 1);
            rt_kprintf("DHT11 temperature:  %d.%d\n",
                (int)read_buff[0], (((uint32_t)(read_buff[0] * 100)) % 100));
            rt_kprintf("DHT11    humidity:  %d.%d\n",
                (int)read_buff[1], (((uint32_t)(read_buff[1] * 100)) % 100));
        }

        rt_thread_mdelay(1000);
    }

    rt_kprintf("\nRead DHT11 End!!\n");
}


static void test_dht11(int argc, char **argv) {
    /* 创建线程，名称是 dht11_thread，入口是 dht11_thread_entry */
    dht11_thread = rt_thread_create("dht11_thread",
        dht11_thread_entry, RT_NULL,
        THREAD_STACK_SIZE,
        THREAD_PRIORITY, THREAD_TIMESLICE);

/* 如果获得线程控制块，启动这个线程 */
    if (dht11_thread != RT_NULL)
        rt_thread_startup(dht11_thread);
}

// 导出函数为命令
MSH_CMD_EXPORT(test_dht11, dht11 test);