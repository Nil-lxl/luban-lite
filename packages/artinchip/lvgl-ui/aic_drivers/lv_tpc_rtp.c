/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtconfig.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef KERNEL_RTTHREAD
#ifdef AIC_USING_RTP
#include <rtdevice.h>
#include <rtthread.h>
#include "lv_tpc_run.h"
#include "aic_osal.h"
#include "aic_core.h"
#include "hal_rtp.h"
#include "mpp_fb.h"
#include "../components/drivers/include/drivers/touch.h"

#define AIC_POINT_NUM                   5
#define AIC_CROSS_LENGTH                50
#define AIC_CROSS_WIDTH                 25
#define AIC_CROSS_HEIGHT                25
#define AIC_BITS_TO_BYTE_RATE           8
#define AIC_CALI_ACCURACY               65536.0
#define AIC_DRAW_POINT_NUM              1000
#define AIC_CALI_MIN_INTERVAL           150
#define AIC_CALI_POINT_NUM              7
#define AIC_CONFIG_FOLDER_PERMISSION    0755
#define AIC_CONFIG_PATH                 "/data/config"
#define AIC_POINTERCAL_PATH             "/data/config/rtp_pointercal"


#define THREAD_PRIORITY   25
#define THREAD_STACK_SIZE 4096
#define THREAD_TIMESLICE  5

static rt_sem_t g_rtp_sem;
static int g_fb_width = 0;
static int g_fb_height = 0;
static struct mpp_fb *g_fb;
static struct aicfb_screeninfo g_fb_info;
static int g_xres;
static int g_yres;
static int g_last_up_flag = 1;

static calibration g_cal = {
    .x = { 0 },
    .y = { 0 },
};

void lv_rtp_calibrate(rt_device_t rtp_dev, int fb_width, int fb_height);
void lv_convert_adc_to_coord(rt_device_t rtp_dev, struct rt_touch_data *data);

static void rtp_check_event_type(int event_type, int pressure)
{
    static int up_flag = 0;

    switch(event_type) {
    case RT_TOUCH_EVENT_DOWN:
        up_flag = 0;
        break;
    case RT_TOUCH_EVENT_UP:
        up_flag = 1;
        break;
    default:
        break;
    }

    if (g_last_up_flag && !pressure)
        rt_kprintf("Press: too light\n");
    else
        g_last_up_flag = up_flag;
    return;
}

static int rtp_get_fb_info(void)
{
    int ret = 0;

    g_fb = mpp_fb_open();
    if (!g_fb) {
        pr_err("mpp_fb_open error!!!!\n");
        return -1;
    }

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0) {
        pr_err("ioctl() failed! errno: -%d\n", -ret);
        return -1;
    }

    pr_info("Screen width: %d, height: %d\n", g_fb_info.width,
            g_fb_info.height);

    g_xres = g_fb_info.width;
    g_yres = g_fb_info.height;

    return ret;
}

/* Draw a cross, and each line size: 50 */
static void rtp_draw_cross(int index, char *name, int y, int x)
{
    u32 i;
    u8 *fb = g_fb_info.framebuffer;
    u8 rate = g_fb_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;
    int length = AIC_CROSS_LENGTH;

    memset(fb, 0, g_fb_info.smem_len);
    memset(fb + g_fb_info.stride * (y + length / 2) + rate * x, 0xFF, rate * length);

    for (i = 0; i < length; i++)
         memset(fb + g_fb_info.stride * (y + i) + rate * (x + length / 2) , 0xFF, rate);

    g_cal.xfb[index] = x + length / 2;
    g_cal.yfb[index] = y + length / 2;

    aicos_dcache_clean_invalid_range(g_fb_info.framebuffer,
                                     g_fb_info.smem_len);

#ifndef AIC_DISP_COLOR_BLOCK
        /* enable display power after flush first frame */
        static bool first_frame = true;

        if (first_frame) {
            mpp_fb_ioctl(g_fb, AICFB_POWERON, 0);
            first_frame = false;
        }
#endif

    return;
}

/* Calculate the average value of multiple points triggered by one click as
 * the calibration point. Among them, the calibration point is the touch
 * screen coordinate system */
static void rtp_get_valid_point(rt_device_t rtp_dev, int index, struct rt_touch_data *data)
{
    int x=0, y=0;
    int cnt = 0;
    u32 tp_x = 0, tp_y = 0;
    int sum_x = 0;
    int sum_y = 0;
    int press_flag = 0;

    rt_device_control(rtp_dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
    do {
        if (rt_sem_take(g_rtp_sem, RT_WAITING_FOREVER) != RT_EOK)
            break;

        if (rt_device_read(rtp_dev, 0, data, 1) != 1)
            continue;

        rtp_check_event_type(data->event, data->pressure);

        if (data->event == RT_TOUCH_EVENT_UP) {
            if (press_flag)
                break;
            continue;
        }

        if (data->x_coordinate > 0 || data->y_coordinate > 0) {
            press_flag = 1;
            rt_device_control(rtp_dev, RT_TOUCH_CTRL_SET_X_TO_Y,
                              (void *)data);
            x = data->x_coordinate;
            y = data->y_coordinate;
            sum_x += x;
            sum_y += y;
            cnt++;
        }

    } while (1);

    x = sum_x /cnt;
    y = sum_y /cnt;

    /* ADC value converted to touch panel's coordinate value */
    tp_x = AIC_RTP_MAX_VAL - x;
    tp_y = AIC_RTP_MAX_VAL - y;
    tp_x = (tp_x * g_fb_info.width) / AIC_RTP_MAX_VAL;
    tp_y = (tp_y * g_fb_info.height) / AIC_RTP_MAX_VAL;
    g_cal.x[index] = tp_x;
    g_cal.y[index] = tp_y;

    g_last_up_flag = 1;
    rt_device_control(rtp_dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return;
}

static void lv_rtp_read_calibrate_pare(rt_device_t rtp_dev)
{
    char cal_buf[sizeof(float) * AIC_CALI_POINT_NUM];
    int cali_cnt;
    int fd = open(AIC_POINTERCAL_PATH, O_RDONLY);
    if (fd >= 0) {
        read(fd, cal_buf, AIC_CALI_POINT_NUM * sizeof(float));
        for (cali_cnt = 0; cali_cnt < AIC_CALI_POINT_NUM; cali_cnt++) {
            g_cal.a[cali_cnt] = *(int *)(cal_buf + cali_cnt * sizeof(float));
        }
        close(fd);
    } else {
        rt_kprintf("the calibrate file is not exit, please open the marco AIC_USING_FS_IMAGE_1\n");
    }

}

static int rtp_save_cali_param()
{
    int fd;
    int cali_cnt;
    char cal_buf[sizeof(float) * AIC_CALI_POINT_NUM];

    if (open(AIC_CONFIG_PATH, O_RDONLY) < 0)
        mkdir(AIC_CONFIG_PATH, AIC_CONFIG_FOLDER_PERMISSION);
    fd = open(AIC_POINTERCAL_PATH, O_WRONLY | O_CREAT);

    if (fd > 0) {
        for (cali_cnt = 0; cali_cnt < AIC_CALI_POINT_NUM; cali_cnt++) {
            memcpy(cal_buf + cali_cnt * sizeof(float), &g_cal.a[cali_cnt],
                   sizeof(float));
        }
        write(fd, cal_buf, AIC_CALI_POINT_NUM * sizeof(float));
        close(fd);
    } else {
        rt_kprintf("open file failed!, "
                   "please open the the macro AIC_USING_FS_IMAGE_1, "
                   "the calibrate file save in data region\n");
    }
    return 0;
}

static int rtp_perform_calibration()
{
    int j;
    float n, x, y, x2, y2, xy, z, zx, zy;
    float det, a, b, c, e, f, i;
    float scaling = AIC_CALI_ACCURACY;

    /* Get sums for matrix */
    n = x = y = x2 = y2 = xy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        n += 1.0;
        x += (float)g_cal.x[j];
        y += (float)g_cal.y[j];
        x2 += (float)(g_cal.x[j] * g_cal.x[j]);
        y2 += (float)(g_cal.y[j] * g_cal.y[j]);
        xy += (float)(g_cal.x[j] * g_cal.y[j]);
    }

    /* Get determinant of matrix -- check if determinant is too small */
    det = n * (x2 * y2 - xy * xy) + x * (xy * y - x * y2) + y * (x * xy - y * x2);
    if (det < 0.1 && det > -0.1) {
        rt_kprintf("ts_calibrate: determinant is too small -- %f\n", det);
        return 0;
    }

    /* Get elements of inverse matrix */
    a = (x2 * y2 - xy * xy) / det;
    b = (xy * y - x * y2) / det;
    c = (x * xy - y * x2) / det;
    e = (n * y2 - y * y) / det;
    f = (x * y - n * xy) / det;
    i = (n * x2 - x * x) / det;

    /* Get sums for x calibration */
    z = zx = zy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        z += (float)g_cal.xfb[j];
        zx += (float)(g_cal.xfb[j] * g_cal.x[j]);
        zy += (float)(g_cal.xfb[j] * g_cal.y[j]);
    }

    /* Now multiply out to get the calibration for framebuffer x coord */
    g_cal.a[0] = (int)((a * z + b * zx + c * zy) * (scaling));
    g_cal.a[1] = (int)((b * z + e * zx + f * zy) * (scaling));
    g_cal.a[2] = (int)((c * z + f * zx + i * zy) * (scaling));

    /* Get sums for y calibration */
    z = zx = zy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        z += (float)g_cal.yfb[j];
        zx += (float)(g_cal.yfb[j] * g_cal.x[j]);
        zy += (float)(g_cal.yfb[j] * g_cal.y[j]);
    }

    /* Now multiply out to get the calibration for framebuffer y coord */
    g_cal.a[3] = (int)((a * z + b * zx + c * zy) * (scaling));
    g_cal.a[4] = (int)((b * z + e * zx + f * zy) * (scaling));
    g_cal.a[5] = (int)((c * z + f * zx + i * zy) * (scaling));

    /* If we got here, we're OK, so assign scaling to a[6] and return */
    g_cal.a[6] = (int)scaling;

    rtp_save_cali_param(&g_cal);

    return 1;
}

static rt_err_t rtp_rx_callback(rt_device_t rtp_dev, rt_size_t size)
{
    rt_sem_release(g_rtp_sem);
    return 0;
}

void lv_rtp_calibrate(rt_device_t rtp_dev, int fb_width, int fb_height)
{
    int length = AIC_CROSS_LENGTH;
    int width = AIC_CROSS_WIDTH;
    int height = AIC_CROSS_HEIGHT;
    struct stat rtp_config;
    struct rt_touch_data *data;
    int result = stat(AIC_POINTERCAL_PATH, &rtp_config);
    g_fb_width = fb_width;
    g_fb_height = fb_height;

    if (result == -1) {
        data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data));

        rt_device_set_rx_indicate(rtp_dev, rtp_rx_callback);
        g_rtp_sem = rt_sem_create("rtp_cali_sem", 0, RT_IPC_FLAG_FIFO);

        rtp_get_fb_info();

        memset(data, 0, sizeof(*data));
        memset(&g_cal, 0, sizeof(g_cal));

        rtp_draw_cross(0, "Top left", height, width);
        rtp_get_valid_point(rtp_dev, 0, data);

        rtp_draw_cross(1, "Top right", height, g_xres - width - length);
        rtp_get_valid_point(rtp_dev, 1, data);

        rtp_draw_cross(2, "Bot right", g_yres - height - length,
                       g_xres - width - length);
        rtp_get_valid_point(rtp_dev, 2, data);

        rtp_draw_cross(3, "Bot left", g_yres - height - length, width);
        rtp_get_valid_point(rtp_dev, 3, data);

        rtp_draw_cross(4, "Center", (g_yres - length) / 2,
                       (g_xres - length) / 2);
        rtp_get_valid_point(rtp_dev, 4, data);

        memset(g_fb_info.framebuffer, 0, g_fb_info.smem_len);

        rtp_perform_calibration();

        rt_sem_delete(g_rtp_sem);
        mpp_fb_close(g_fb);
    } else {
        lv_rtp_read_calibrate_pare(rtp_dev);
    }
    rt_device_control(rtp_dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
}

void lv_convert_adc_to_coord(rt_device_t rtp_dev, struct rt_touch_data *data)
{
    int panel_x = 0;
    int panel_y = 0;
    int a[7] = {0};

    rt_device_control(rtp_dev, RT_TOUCH_CTRL_SET_X_TO_Y, (void *)data);

    panel_x = AIC_RTP_MAX_VAL - data->x_coordinate;
    panel_y = AIC_RTP_MAX_VAL - data->y_coordinate;
    panel_x = (panel_x * g_fb_width) / AIC_RTP_MAX_VAL;
    panel_y = (panel_y * g_fb_height) / AIC_RTP_MAX_VAL;

    if (g_cal.a[6]) {
        memcpy(a, g_cal.a, sizeof(a));
        panel_x = (panel_x * a[1] + panel_y * a[2] + a[0]) / a[6];
        panel_y = (panel_x * a[4] + panel_y * a[5] + a[3]) / a[6];
    }

    data->x_coordinate = panel_x;
    data->y_coordinate = panel_y;
}
#endif
#endif
