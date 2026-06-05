#include "rtthread.h"
#include "rtdevice.h"
#include <stdio.h>
#include <getopt.h>
#include <sys/time.h>
#include "aic_core.h"
#include "aic_hal_gpio.h"
#include "rtdbg.h"

#define LOG_TAG "BMP180"
#define I2C_DEV_NAME "i2c2"
#define BMP180_SLAVE_ADDR 0x77


static struct rt_i2c_client bmp180_client;

typedef struct bmp180_cali_param {
    short AC1;
    short AC2;
    short AC3;
    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;
    short B1;
    short B2;
    short MB;
    short MC;
    short MD;
}cali_param_t;

static cali_param_t CaliParam = { 0 };
static long B5 = 0;

static rt_err_t bmp180_write_regs(struct rt_i2c_client *client, rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs;

    msgs.addr = client->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = data;
    msgs.len = len;

    if (rt_i2c_transfer(client->bus, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        return -RT_ERROR;
    }
}

static rt_err_t bmp180_read_regs(struct rt_i2c_client *client, rt_uint8_t *reg, rt_uint8_t *data, rt_uint8_t len) {
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = client->client_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = reg;
    msgs[0].len = 1;

    msgs[1].addr = client->client_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;

    if (rt_i2c_transfer(client->bus, msgs, 2) == 2) {
        LOG_D("I2c read reg success\n");
        return RT_EOK;
    } else {
        LOG_E("I2C read reg error\n");
        return -RT_ERROR;
    }
}

static cali_param_t bmp180_readCaliParams(void) {
    cali_param_t param;

    rt_uint8_t read_reg[1] = { 0xAA };
    rt_uint8_t read_buf[22] = { 0 };
    bmp180_read_regs(&bmp180_client, read_reg, read_buf, 22);

    param.AC1 = (read_buf[0] << 8) + read_buf[1];
    param.AC2 = (read_buf[2] << 8) + read_buf[3];
    param.AC3 = (read_buf[4] << 8) + read_buf[5];
    param.AC4 = (read_buf[6] << 8) + read_buf[7];
    param.AC5 = (read_buf[8] << 8) + read_buf[9];
    param.AC6 = (read_buf[10] << 8) + read_buf[11];
    param.B1 = (read_buf[12] << 8) + read_buf[13];
    param.B2 = (read_buf[14] << 8) + read_buf[15];
    param.MB = (read_buf[16] << 8) + read_buf[17];
    param.MC = (read_buf[18] << 8) + read_buf[19];
    param.MD = (read_buf[20] << 8) + read_buf[21];

    LOG_I("BMP180 Calibration Parameters:\n");
    LOG_I("  AC1  = %d", param.AC1);
    LOG_I("  AC2  = %d", param.AC2);
    LOG_I("  AC3  = %d", param.AC3);
    LOG_I("  AC4  = %d", param.AC4);
    LOG_I("  AC5  = %d", param.AC5);
    LOG_I("  AC6  = %d", param.AC6);
    LOG_I("  B1   = %d", param.B1);
    LOG_I("  B2   = %d", param.B2);
    LOG_I("  MB   = %d", param.MB);
    LOG_I("  MC   = %d", param.MC);
    LOG_I("  MD   = %d", param.MD);

    // for (int i = 0;i < 22;i++) {
    //     LOG_I("%x  ", read_buf[i]);
    // }

    return param;
}

/**
 * @brief 读取BMP180原始温度值
 * @param 无
 * @return UP-原始温度值
 */
static long bmp180_readRawTemperature(void) {
    uint8_t msb, lsb;
    long UT;

    rt_uint8_t write_buf[2] = { 0xF4,0x2E };
    bmp180_write_regs(&bmp180_client, write_buf, 2);
    aic_mdelay(5);

    rt_uint8_t read_reg[1] = { 0xF6 };
    rt_uint8_t read_buf[2] = { 0 };
    bmp180_read_regs(&bmp180_client, read_reg, read_buf, 2);

    msb = read_buf[0];
    lsb = read_buf[1];
    UT = (msb << 8) + lsb;

    LOG_I("MSB:%x LSB:%x UT = %d", msb, lsb, UT);

    return UT;
}
/**
 * @brief 读取BMP180原始气压值
 * @param oss 精度
 * @return UP-原始气压值
 */
static long bmp180_readRawPressure(uint8_t oss) {
    uint32_t msb, lsb, xlsb;
    long UP;

    uint8_t delay_ms;

    switch (oss) {
    case 0: delay_ms = 6;break;
    case 1: delay_ms = 10;break;
    case 2: delay_ms = 20;break;
    case 3: delay_ms = 30;break;
    default: delay_ms = 6;break;
    }

    rt_uint8_t write_buf[2];
    write_buf[0] = 0xF4;
    write_buf[1] = 0x34 + (oss << 6);
    bmp180_write_regs(&bmp180_client, write_buf, 2);

    aic_mdelay(delay_ms);

    rt_uint8_t read_reg[1] = { 0xF6 };
    rt_uint8_t read_buf[3] = { 0 };
    bmp180_read_regs(&bmp180_client, read_reg, read_buf, 3);

    msb = read_buf[0];
    lsb = read_buf[1];
    xlsb = read_buf[2];
    UP = ((msb << 16) + (lsb << 8) + xlsb) >> (8 - oss);

    LOG_I("MSB:%x LSB:%x xlsb:%x UP = %x", msb, lsb, xlsb, UP);

    return UP;
}

/**
 * @brief 获取计算后的真实温度值
 * @param NULL
 * @return 温度值temp，浮点型
 */
static float bmp180_getTrueTemp(void) {
    long UT;
    long X1, X2;
    float Temperature;

    UT = bmp180_readRawTemperature();

    X1 = (UT - CaliParam.AC6) * CaliParam.AC5 / 32768.0;
    X2 = CaliParam.MC * 2048.0 / (X1 + CaliParam.MD);
    B5 = X1 + X2;

    LOG_I("X1 = %d", X1);
    LOG_I("X2 = %d", X2);
    LOG_I("B5 = %d", B5);

    Temperature = (B5 + 8) / 16.0;

    LOG_I("Temperature:%d", (long)(Temperature * 100));

    return Temperature;
}

/**
 * @brief 获取计算后的真实气压值
 * @param NULL
 * @return 气压值Pressure，浮点型
 */
static float bmp180_getTruePressure(uint8_t oss) {
    long UP;
    long B3, B6, X1, X2, X3, p;
    unsigned long B4, B7;
    float Pressure;

    oss = 3;

    UP = bmp180_readRawPressure(oss);

    B6 = B5 - 4000;
    LOG_I("B6 = %d", B6);

    X1 = (CaliParam.B2 * (B6 * B6 >> 12)) >> 11;

    X2 = CaliParam.AC2 * B6 >> 11;

    X3 = X1 + X2;

    B3 = (((CaliParam.AC1 * 4 + X3) << oss) + 2) / 4;

    LOG_I("X1 = %d", X1);
    LOG_I("X2 = %d", X2);
    LOG_I("X3 = %d", X3);
    LOG_I("B3 = %d", B3);

    X1 = CaliParam.AC3 * B6 >> 13;

    X2 = (CaliParam.B1 * (B6 * B6 >> 12)) >> 16;

    X3 = (X1 + X2 + 2) >> 2;

    B4 = CaliParam.AC4 * (X3 + 32768) >> 15;

    B7 = (UP - B3) * (50000 >> oss);

    if (B7 < 0x80000000) {
        p = (B7 * 2) / B4;
    } else {
        p = (B7 / B4) * 2;
    }

    X1 = (p >> 8) * (p >> 8);

    LOG_I("X1 = %d", X1);
    LOG_I("X2 = %d", X2);
    LOG_I("X3 = %d", X3);
    LOG_I("B4 = %d", B4);
    LOG_I("B7 = %d", B7);
    LOG_I("p = %d", p);
    LOG_I("X1 = %d", X1);

    X1 = (X1 * 3038) >> 16;

    X2 = (-7357 * p) >> 16;

    p = p + ((X1 + X2 + 3791) >> 4);
    LOG_I("X1 = %d", X1);
    LOG_I("X2 = %d", X2);

    LOG_I("Pressure:%d", p);

    Pressure = p / 1.0f;

    return Pressure;

}

static rt_err_t bmp180_Init() {
    int ret = 0;

    /* 查找I2C总线设备，获取I2C总线设备句柄 */
    bmp180_client.bus = (struct rt_i2c_bus_device *)rt_device_find(I2C_DEV_NAME);
    if (bmp180_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", I2C_DEV_NAME);
        return -RT_ERROR;
    } else {
        LOG_I("Find device: %s\n", I2C_DEV_NAME);
    }

    aic_mdelay(50); // 延时一小会儿

    bmp180_client.client_addr = BMP180_SLAVE_ADDR;

    CaliParam = bmp180_readCaliParams();

    return RT_EOK;
}

INIT_DEVICE_EXPORT(bmp180_Init);

MSH_CMD_EXPORT_ALIAS(bmp180_getTrueTemp, get_temp, get bmp180 temp);
MSH_CMD_EXPORT_ALIAS(bmp180_getTruePressure, get_press, get bmp180 pressure);
