/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date              Notes
 * 2025-05-23        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <math.h>
#define DBG_TAG "sc7a20h"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SC7A20H_SLAVE_ADDR      0x19

static struct rt_i2c_client g_sc7a20h_client;
static struct rt_device g_sc7a20h_dev = {0};

static rt_err_t sc7a20h_write_reg(struct rt_i2c_client *dev, rt_uint8_t reg, rt_uint8_t val)
{
    if (rt_i2c_write_reg(dev->bus, SC7A20H_SLAVE_ADDR, reg, &val, 1) != 1) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, val);
        return -1;
    }

    return 0;
}

static rt_err_t sc7a20h_read_reg(struct rt_i2c_client *dev, rt_uint8_t reg, rt_uint8_t *val, rt_uint8_t len)
{
    if (rt_i2c_read_reg(dev->bus, SC7A20H_SLAVE_ADDR, reg, val, len) != len) {
        LOG_E("%s: error: reg = 0x%x, val = 0x%x", __func__, reg, *val);
        return -1;
    }

    return 0;
}

static int sc7a20h_check(void)
{
    rt_uint8_t am_val = 0;
    rt_uint8_t version_val = 0;

    sc7a20h_read_reg(&g_sc7a20h_client, 0x0f, &am_val, 1);
    sc7a20h_read_reg(&g_sc7a20h_client, 0x70, &version_val, 1);

    if ((am_val == 0x11) && (version_val == 0x28))
        return 0x1;
    else
        return 0x0;
}

static uint8_t sc7a20h_power_down(void)
{
    rt_uint8_t read_buf = 0xff;
    /* Power down */
    sc7a20h_write_reg(&g_sc7a20h_client, 0x20, 0x00);
    /* Close aoil function */
    sc7a20h_read_reg(&g_sc7a20h_client, 0x20, &read_buf, 1);
    if (read_buf == 0x00)
        return 1;
    else
        return 0;
}

static uint8_t sc7a20h_soft_reset(void)
{
    rt_uint8_t read_buf = 0xff;
    /* Flag */
    sc7a20h_write_reg(&g_sc7a20h_client, 0x23, 0x80);
    rt_thread_mdelay(100);
    /* Boot */
    sc7a20h_write_reg(&g_sc7a20h_client, 0x24, 0x80);
    rt_thread_mdelay(200);
    /* Soft reset */
    sc7a20h_write_reg(&g_sc7a20h_client, 0x68, 0xa5);
    /* Close aoil function */
    sc7a20h_read_reg(&g_sc7a20h_client, 0x23, &read_buf, 1);
    if (read_buf == 0x00)
        return 1;
    else
        return 0;
}

static void sc7a20h_rawdata_read(int16_t *x_data, int16_t *y_data, int16_t *z_data)
{
    rt_uint8_t raw_data[6] = {0};

    sc7a20h_read_reg(&g_sc7a20h_client, 0xa8, raw_data, sizeof(raw_data));
    *x_data = (int16_t)((raw_data[1] << 8) + raw_data[0]) >> 4;
    *y_data = (int16_t)((raw_data[3] << 8) + raw_data[2]) >> 4;
    *z_data = (int16_t)((raw_data[5] << 8) + raw_data[4]) >> 4;
}

static rt_err_t sc7a20h_init(rt_device_t dev)
{
    rt_uint8_t check_flag = 0;

    check_flag = sc7a20h_check();

    sc7a20h_write_reg(&g_sc7a20h_client, 0x20, 0x27);
    sc7a20h_write_reg(&g_sc7a20h_client, 0x23, 0x00);

    if (check_flag == 1)
        check_flag = sc7a20h_power_down();

    if (check_flag == 1)
        check_flag = sc7a20h_soft_reset();

    if (check_flag == 1) {
        sc7a20h_write_reg(&g_sc7a20h_client, 0x1f, 0x01);
        sc7a20h_write_reg(&g_sc7a20h_client, 0x23, 0x80);
        sc7a20h_write_reg(&g_sc7a20h_client, 0x2e, 0x00);
#ifdef AIC_GYRO_HPF_ENABLE
        sc7a20h_write_reg(&g_sc7a20h_client, 0x21, 0x68);
#else
        sc7a20h_write_reg(&g_sc7a20h_client, 0x21, 0x00);
#endif
#ifdef AIC_GYRO_FIFO_ENABLE
        sc7a20h_write_reg(&g_sc7a20h_client, 0x24, 0xc0);
#else
        sc7a20h_write_reg(&g_sc7a20h_client, 0x24, 0x80);
        sc7a20h_write_reg(&g_sc7a20h_client, 0x2e, 0x9f);
        rt_thread_mdelay(1);
#endif
#ifdef AIC_GYRO_INT_ENABLE
        sc7a20h_write_reg(&g_sc7a20h_client, 0x25, 0x02);
#else
        sc7a20h_write_reg(&g_sc7a20h_client, 0x25, 0x00);
#endif
        rt_thread_mdelay(1);
        sc7a20h_write_reg(&g_sc7a20h_client, 0x20, 0x37);
        rt_thread_mdelay(10);
        sc7a20h_write_reg(&g_sc7a20h_client, 0x22, 0x00);
        sc7a20h_write_reg(&g_sc7a20h_client, 0x57, 0x00);
    }

    return RT_EOK;
}

static rt_err_t sc7a20h_open(rt_device_t dev, rt_uint16_t flag)
{
    if (dev)
        sc7a20h_init(dev);

    return RT_EOK;
}

static rt_err_t sc7a20h_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t sc7a20h_read(rt_device_t dev, rt_off_t pos, void *buf, rt_size_t size)
{
    if ((buf == RT_NULL) || size < sizeof(int16_t) * 3) {
        rt_kprintf("buffer should use int16_t and buf szie should more than 3!!\n");
        return 0;
    }

    int16_t *read_buf = (int16_t *)buf;

    sc7a20h_rawdata_read(&read_buf[0], &read_buf[1], &read_buf[2]);

    return sizeof(int16_t) * 3;
}

static rt_err_t sc7a20h_control(rt_device_t dev, int cmd, void *args)
{
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops sc7a20h_ops =
{
    .init = sc7a20h_init,
    .open = sc7a20h_open,
    .close = sc7a20h_close,
    .read = sc7a20h_read,
    .control = sc7a20h_control,
};
#endif

int rt_hw_sc7a20h_init(void)
{
#ifdef RT_USING_DEVICE_OPS
    g_sc7a20h_dev.ops = &sc7a20h_ops;
#else
    g_sc7a20h_dev.init = sc7a20h_init;
    g_sc7a20h_dev.open = sc7a20h_open;
    g_sc7a20h_dev.close = sc7a20h_close;
    g_sc7a20h_dev.read = sc7a20h_read;
    g_sc7a20h_dev.control = sc7a20h_control;
#endif
    g_sc7a20h_client.bus = (struct rt_i2c_bus_device *)rt_device_find(AIC_GYRO_I2C_CHAN_NAME);
    if (g_sc7a20h_client.bus == RT_NULL) {
        LOG_E("Can't find %s device", AIC_GYRO_I2C_CHAN_NAME);
        return -RT_ERROR;
    }

    if (rt_device_open((rt_device_t)g_sc7a20h_client.bus, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        LOG_E("open %s device failed", AIC_GYRO_I2C_CHAN_NAME);
        return -RT_ERROR;
    }

    g_sc7a20h_client.client_addr = SC7A20H_SLAVE_ADDR;

    rt_device_register(&g_sc7a20h_dev, AIC_GYRO_NAME, RT_DEVICE_FLAG_STANDALONE);
    LOG_I("sc7a20h init\n");

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_sc7a20h_init);

/* TEST SAMPLE */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "aic_common.h"

#define TEST_SC7A20HTHREAD_STACK_SIZE   1024 * 4
#define TEST_SC7A20H_THREAD_PRIORITY    25
#define M_PI_F                          3.1415926535f
#define DEBOUNCE_MS                     300
#define ROTATE_0                        0
#define ROTATE_90                       90
#define ROTATE_180                      180
#define ROTATE_270                      270

extern int usb_display_set_rotate(unsigned int rotate_angle);
static int g_cur_screen_rotation = ROTATE_0;
static int g_stability_rotations[AIC_GYRO_STABILITY_CHECK_COUNT] = {0};
static int g_stability_index = 0;
static uint64_t g_last_rotation_time = 0;

void print_fixed_point_float(const char *prefix, float value) {
    int integer_part = (int)value;
    int decimal_part = (int)(value * 100.0f);

    if (decimal_part < 0)
        decimal_part = -decimal_part;
    else
        decimal_part %= 100;

    rt_kprintf("%s%d.%02d\n", prefix, integer_part, decimal_part);
}

float rad_to_deg(float rad) {
    return rad * 180.0f / M_PI_F;
}

int is_device_tilted(int16_t x_acc, int16_t y_acc, int16_t z_acc) {
    float norm = sqrtf(x_acc * x_acc + y_acc * y_acc + z_acc * z_acc);

    if (norm == 0.0f || norm < 100.0f) {
        rt_kprintf("Invalid acceleration data\n");
        return 0;
    }

    float ax = x_acc / norm;
    float ay = y_acc / norm;
    float az = z_acc / norm;

    // Calculate roll and pitch angles (with sign)
    float roll_rad = atan2f(ay, az);
    float pitch_rad = atan2f(-ax, sqrtf(ay * ay + az * az));

    float roll_deg = rad_to_deg(roll_rad);
    float pitch_deg = rad_to_deg(pitch_rad);

    // Compute total tilt angle
    float tilt_angle = sqrtf(roll_deg * roll_deg + pitch_deg * pitch_deg);
    // Check if tilt exceeds threshold
    int tilted = (tilt_angle > AIC_GYRO_TILT_THRESHOLD_DEG);

#ifdef AIC_GYRO_DEBUG_ENABLE
    print_fixed_point_float("Roll: ", roll_deg);
    print_fixed_point_float("Pitch: ", pitch_deg);
    print_fixed_point_float("Tilt: ", tilt_angle);
    rt_kprintf(tilted ? "→ Tilted\n" : "→ Flat\n");
#endif
    return tilted;
}

int get_rotation_from_acceleration(int16_t x_acc, int16_t y_acc, int16_t z_acc) {
    if (!is_device_tilted(x_acc, y_acc, z_acc))
        return g_cur_screen_rotation; // Don't rotate if device is flat

    if (x_acc > 0 && x_acc < 200 && y_acc > 0)
        return ROTATE_0;
    else if (x_acc > 200 && y_acc > 0)
        return ROTATE_270;
    else if (x_acc > 0 && x_acc < 200 && y_acc < 0)
        return ROTATE_180;
    else if (x_acc < -200 && y_acc < 0)
        return ROTATE_90;
    else
        return g_cur_screen_rotation;
}

void check_screen_rotation(int16_t x_acc, int16_t y_acc, int16_t z_acc) {
    uint64_t now = aic_get_time_ms();
    int consistent = 1;

    // Step 1: Get new rotation from acceleration
    int new_rotation = get_rotation_from_acceleration(x_acc, y_acc, z_acc);

    // Step 2: Stability check
    g_stability_rotations[g_stability_index++] = new_rotation;
    if (g_stability_index >= AIC_GYRO_STABILITY_CHECK_COUNT)
        g_stability_index = 0;

    for (int i = 0; i < AIC_GYRO_STABILITY_CHECK_COUNT; i++) {
        if (g_stability_rotations[i] != new_rotation) {
            consistent = 0;
            break;
        }
    }

    // Step 3: Update screen rotation if stable and different from current
    if (consistent && new_rotation != g_cur_screen_rotation) {
        if (now - g_last_rotation_time > DEBOUNCE_MS) {
            g_cur_screen_rotation = new_rotation;
            usb_display_set_rotate(g_cur_screen_rotation);
#ifdef AIC_GYRO_DEBUG_ENABLE
            rt_kprintf("----------Rotation updated to %d°------------\n", g_cur_screen_rotation);
#endif
            g_last_rotation_time = now;
        } else {
#ifdef AIC_GYRO_DEBUG_ENABLE
            rt_kprintf("Rotation too frequent → Debounce active\n");
#endif
        }
    }
}

// Public interface: pass raw acceleration data
static void sc7a20h_rawangle_show(int16_t *acc_data) {
    int16_t x, y, z;

    x = acc_data[0];
    y = acc_data[1];
    z = acc_data[2];

#ifdef AIC_GYRO_X_INVERT
    x = -x;
#endif
#ifdef AIC_GYRO_Y_INVERT
    y = -y;
#endif
#ifdef AIC_GYRO_Z_INVERT
    z = -z;
#endif
#ifdef AIC_GYRO_DEBUG_ENABLE
    rt_kprintf("x:%d y:%d z:%d\n", x, y, z);
#endif
    check_screen_rotation(x, y, z);
    rt_thread_mdelay(10);
}

static void test_sc7a20h_thread_entry(void *parameter)
{
    rt_device_t dev = RT_NULL;
    /* Buffer size must more than 3 */
    int16_t buffer[3] = {0};

    dev = rt_device_find(AIC_GYRO_NAME);
    if (!dev) {
        LOG_E("Device %s not found!", AIC_GYRO_NAME);
        return;
    }

    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDONLY) != RT_EOK) {
        LOG_E("Failed to open device %s!", AIC_GYRO_NAME);
        return;
    }

    LOG_I("Start reading from device %s every 20ms...", AIC_GYRO_NAME);

    while (1) {
        rt_device_read(dev, 0, buffer, sizeof(buffer));
        sc7a20h_rawangle_show(buffer);
        rt_thread_mdelay(20);
    }

    rt_device_close(dev);
}


static void test_sc7a20h(int argc, char *argv[])
{
    rt_thread_t tid = rt_thread_create("test_sc7a20h", test_sc7a20h_thread_entry, RT_NULL,
                                        TEST_SC7A20HTHREAD_STACK_SIZE, TEST_SC7A20H_THREAD_PRIORITY, 10);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
        LOG_I("Test thread 'test_sc7a20h' started.");
    } else {
        LOG_E("Failed to create test thread!");
    }

    return;
}
MSH_CMD_EXPORT(test_sc7a20h, test sc7a20h sample);
