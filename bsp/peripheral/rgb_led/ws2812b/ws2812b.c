#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include "hal_adcim.h"

#include "rtthread.h"
#include "rtdevice.h"

#include "aic_core.h"
#include "aic_log.h"
#include "hal_gpai.h"
#include "aic_hal_gpio.h"

#include "ws2812b.h"

static struct rt_spi_device *ws2812_device = RT_NULL;

static uint8_t   ledsArray[WS2812_MAX * 3];      // 定义颜色数据存储数组
static uint32_t  ledsCount = WS2812_NUMBER;   // 定义实际彩灯默认个数
static uint32_t  ledsBytes = WS2812_NUMBER * 3; // 定义实际彩灯颜色数据个数

/**
 * @brief 将WS2812的SPI设备挂载到SPI总线上
 * @return RT_EOK成功  -RT_ERROR失败
 */
static int ws2812_spi_attach(void) {
    struct rt_spi_device *spi_device = RT_NULL; // spi设备结构体

    // 为spi结构体申请一片空间
    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (RT_NULL == spi_device) {
        LOG_E("Failed to malloc the spi device.");
        return -RT_ENOMEM;
    }
    LOG_D("malloc the spi succeed.\n");

    // 挂载设备到总线
    int ret = rt_spi_bus_attach_device(spi_device, WS2812_DEVICE_NAME, SPI_BUS_NAME, RT_NULL);
    if (ret != RT_EOK) {
        LOG_E("Failed to rt_spi_bus_attach_device.");
        return ret;
    }
    LOG_D("rt_spi_bus_attach_device succeed.\n");

    return RT_EOK;
}

/**
 * @brief 初始化WS2812的SPI设备
 * @return RT_EOK成功  -RT_ERROR失败
 */
static int WS2812_Spi_Init(void) {
    struct rt_device *dev;
    int ret = 0;

    ws2812_device = (struct rt_spi_device *)rt_device_find(WS2812_DEVICE_NAME);
    if (!ws2812_device) {
        LOG_E("Failed to get device in name %s\n", WS2812_DEVICE_NAME);
        return -RT_ERROR;
    }
    LOG_I("find spi device :%s\n", WS2812_DEVICE_NAME);

    dev = (struct rt_device *)ws2812_device;
    if (dev->type != RT_Device_Class_SPIDevice) {
        ws2812_device = RT_NULL;
        LOG_E("%s is not SPI device.\n", WS2812_DEVICE_NAME);
        return -RT_ERROR;
    }

    struct rt_spi_configuration spi_cfg;
    rt_memset(&spi_cfg, 0, sizeof(spi_cfg));

    spi_cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_3 | RT_SPI_MSB;
    spi_cfg.max_hz = 8 * 1000 * 1000;   // 8Mhz
    spi_cfg.data_width = 8;

    ret = rt_spi_configure(ws2812_device, &spi_cfg);
    if (ret < 0) {
        LOG_E("spi configure failure.");
        return -RT_ERROR;;
    }

    return RT_EOK;
}

/**
 * @brief 向WS2812写入len长度的字节
 * @param send_buff 数据地址
 * @param len 字节长度
 * @return RT_EOK成功  -RT_ERROR失败
 */
static int ws2812_WriteData(uint8_t *send_buff, uint32_t len) {
    int i, j;
    uint8_t buff[512] = { 0 };  // 发送缓存区
    struct rt_spi_message msg;

    rt_memset(&msg, 0, sizeof(msg));

    msg.send_buf = buff;		    /* 发送缓冲区指针 */
    msg.recv_buf = RT_NULL; 		/* 接收缓冲区指针 */
    /* 这里加了一个字节的数据发送，是因为发送开头不准，先发测试的数，之后再发数据！ */
    msg.length = len * 8 + 1;		/* 发送 / 接收 数据字节数 */
    msg.next = RT_NULL;			    /* 指向继续发送的下一条消息的指针 */
    msg.cs_take = 1;				/* 片选选中 */
    msg.cs_release = 1;			    /* 释放片选 */

    /* 测试数据 */
    buff[0] = WS2812_BLACK;

    // 转换send_buff中的每个bit并存入buff
    for (i = 0, j = 1; i < len; i++) {
        for (int bit = 0; bit < 8; bit++) {
            if (send_buff[i] & (0x80 >> bit)) // 当前位为1
            {
                buff[j++] = WS2812_1_CODE; // 存储1码
            } else // 当前位为0
            {
                buff[j++] = WS2812_0_CODE; // 存储0码
            }
        }
    }
    rt_spi_take_bus((struct rt_spi_device *)ws2812_device);

    if (rt_spi_transfer_message(ws2812_device, &msg) == RT_NULL) {
        LOG_D("rt_spi_transfer_message successful\n");
    } else {
        LOG_E("rt_spi_transfer_message failed!!");
        return -RT_ERROR;
    }

    rt_spi_release_bus((struct rt_spi_device *)ws2812_device);

    return RT_EOK;
}

/**
 * @brief 复位ws2812
 * @return RT_EOK成功  -RT_ERROR失败
 */
int ws2812_reset(void) {
    uint8_t buff[300] = { 0x00 };  // 发送缓存区
    struct rt_spi_message msg;

    rt_memset(&msg, 0, sizeof(msg));

    msg.send_buf = buff;		    /* 发送缓冲区指针 */
    msg.recv_buf = RT_NULL; 	    /* 接收缓冲区指针 */

    msg.length = 300;			    /* 发送 / 接收 数据字节数 */
    msg.next = RT_NULL;			    /* 指向继续发送的下一条消息的指针 */
    msg.cs_take = 1;				/* 片选选中 */
    msg.cs_release = 1;			    /* 释放片选 */

    rt_spi_take_bus((struct rt_spi_device *)ws2812_device);
    if (rt_spi_transfer_message(ws2812_device, &msg) == RT_NULL) {
        LOG_D("rt_spi_transfer_message successful\n");
    } else {
        LOG_E("rt_spi_transfer_message failed!!");
        return -RT_ERROR;
    }

    rt_spi_release_bus((struct rt_spi_device *)ws2812_device);

    return RT_EOK;
}
/**
 ** @brief 设置所有彩灯颜色
 ** @param Color 颜色数据
 */
void ws2812_SetColorAll(uint64_t Color) {
    for (uint8_t i = 0; i < ledsCount; i++) {
        ledsArray[i * 3] = 0;
        ledsArray[i * 3 + 1] = 0;
        ledsArray[i * 3 + 2] = 0;

        ledsArray[i * 3] = (Color >> 8) & 0xff;
        ledsArray[i * 3 + 1] = (Color >> 16) & 0xff;
        ledsArray[i * 3 + 2] = (Color >> 0) & 0xff;
    }
}
/**
 * @brief 设置指定彩灯颜色
 * @param Index 控制彩灯的序号
 * @param Color 颜色数据
 * @return RT_EOK成功  -RT_ERROR失败
 */
int ws2812_SetColorIndex(uint8_t Index, uint64_t Color) {
    if (Index >= ledsCount) {
        return -RT_ERROR;    //to avoid overflow
    }

    ledsArray[Index * 3] = 0;
    ledsArray[Index * 3 + 1] = 0;
    ledsArray[Index * 3 + 2] = 0;

    ledsArray[Index * 3] = (Color >> 8) & 0xff;
    ledsArray[Index * 3 + 1] = (Color >> 16) & 0xff;
    ledsArray[Index * 3 + 2] = (Color >> 0) & 0xff;

    // rt_kprintf("ws2812_SetColorIndex successful !\n");

    return RT_EOK;
}

/**
 * @brief 发送彩灯数据
 * @return RT_EOK成功  -RT_ERROR失败
 */
int ws2812_send_array(void) {
    ws2812_reset();
    aicos_mdelay(5);

    int ret = ws2812_WriteData(ledsArray, ledsBytes);
    if (ret != RT_EOK) {
        LOG_E("%s--->WS2812_WriteByte failed !!", __FUNCTION__);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**********************************************************
 * 函 数 名 称：ws2812_init
 * 函 数 功 能：初始化模块
 * 传 入 参 数：无
 * 函 数 返 回：RT_EOK成功  -RT_ERROR失败
**********************************************************/

/**
 * @brief 初始化ws2812
 * @return RT_EOK成功  -RT_ERROR失败
 */
int ws2812_init(void) {
    /* 设备挂载 */
    if (RT_EOK != ws2812_spi_attach()) {
        LOG_E("%s-->ws2812_spi_attach failed !!!", __FUNCTION__);
        return -RT_ERROR;
    }

    /* 设备初始化 */
    if (RT_EOK != WS2812_Spi_Init()) {
        LOG_E("%s-->WS2812_Spi_Init failed !!!", __FUNCTION__);
        return -RT_ERROR;
    }

    LOG_I("ws2812_init successful !!\n");

    return RT_EOK;
}

// 系统加载时初始化设备
INIT_DEVICE_EXPORT(ws2812_init);