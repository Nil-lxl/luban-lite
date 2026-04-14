#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <rtthread.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "aic_log.h"
#include "hal_rtp.h"

#include "artinchip_fb.h"
#include "mpp_fb.h"
#include "touch.h"
#include "boot_param.h"

#include <unistd.h>

#ifdef AIC_USING_RTP
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
#define AIC_CONFIG_PATH                 "/rodata/config"
#define AIC_POINTERCAL_PATH             "/rodata/config/rtp_pointercal"

static rt_device_t rtp_device = RT_NULL;
static rt_thread_t rtp_thread = RT_NULL;
static rt_sem_t rtp_sem = RT_NULL;

static struct aicfb_screeninfo screen_info;
static struct mpp_fb *fb = RT_NULL;

static int g_xres;
static int g_yres;
static int g_opened = 0;
static int g_draw_count = 0;
static int g_last_up_flag = 1;


static calibration g_cal = {
        .x = { 0 },
        .y = { 0 },
};

static rt_err_t rtp_rx_callback(rt_device_t dev, rt_size_t size) {
    rt_sem_release(rtp_sem);
    return 0;
}

static int rtp_get_fb_info(void) {
    int ret = 0;

    fb = mpp_fb_open();
    if (!fb) {
        pr_err("mpp_fb_open error!!!!\n");
        return -1;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &screen_info);
    if (ret < 0) {
        pr_err("ioctl() failed! errno: -%d\n", -ret);
        return -1;
    }

    pr_info("Screen width: %d, height: %d\n", screen_info.width, screen_info.height);

    g_xres = screen_info.width;
    g_yres = screen_info.height;

    return ret;
}

/* Draw a grid, and each cell size: 200*200 */
static void rtp_draw_grid(void) {
    u32 i, j;
    u8 *fb = screen_info.framebuffer;
    u8 rate = screen_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;
    int stride = screen_info.stride;

    memset(fb, 0, screen_info.smem_len);
    for (i = 1; i * 200 < screen_info.height; i++) {
        memset(fb + stride * (200 * i - 1), 0x30, stride);
    }

    for (i = 0; i < screen_info.height; i++) {
        for (j = 1; j * 200 < screen_info.width; j++) {
            memset(fb + stride * i + 200 * rate * j - rate, 0x30, rate);
        }
    }

    aicos_dcache_clean_invalid_range(screen_info.framebuffer, screen_info.smem_len);
}

static void rtp_edge_area_protect(int *x, int *y) {
    if (*x < 0)
        *x = 0;
    if (*y < 0)
        *y = 0;
    if (*x >= screen_info.width)
        *x = screen_info.width - 1;
    if (*y >= screen_info.height)
        *y = screen_info.height - 1;
}

static void test_draw_a_point(u32 cnt, struct rt_touch_data *data, calibration *cal) {
    u32 pos = 0;
    u8 *buf = NULL;
    int panel_x = 0;
    int panel_y = 0;
    int a[7] = { 0 };
    u8 rate = screen_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;

    panel_x = AIC_RTP_MAX_VAL - data->x_coordinate;
    panel_y = AIC_RTP_MAX_VAL - data->y_coordinate;
    panel_x = (panel_x * screen_info.width) / AIC_RTP_MAX_VAL;
    panel_y = (panel_y * screen_info.height) / AIC_RTP_MAX_VAL;

    if (cal->a[6]) {
        memcpy(a, cal->a, sizeof(a));
        panel_x = (panel_x * a[1] + panel_y * a[2] + a[0]) / a[6];
        panel_y = (panel_x * a[4] + panel_y * a[5] + a[3]) / a[6];
    }

    rtp_edge_area_protect(&panel_x, &panel_y);
    rt_kprintf("%d: X %d/%d, Y %d/%d \n", cnt, panel_x, data->x_coordinate, panel_y, data->y_coordinate);

    pos = panel_y * screen_info.stride + panel_x * rate;
    if (pos < screen_info.smem_len) {
        buf = screen_info.framebuffer + pos;
        memset(buf, 0xFF, 4);
        buf -= pos % CACHE_LINE_SIZE;
        aicos_dcache_clean_invalid_range(buf, CACHE_LINE_SIZE);
        return;
    }
    pr_err("Invalid position: %d\n", pos);
}

static void rtp_check_event_type(int event_type, int pressure) {
    int up_flag = 0;

    switch (event_type) {
        case RT_TOUCH_EVENT_DOWN:
            up_flag = 0;
            rt_kprintf("Event type : down, ");
            break;
        case RT_TOUCH_EVENT_UP:
            up_flag = 1;
            rt_kprintf("Event type : up, ");
            break;
        default:
            break;
    }

    if (g_last_up_flag && !pressure)
        rt_kprintf("Press: too light\n");
    else
        rt_kprintf("Press : %d\n", pressure);

    g_last_up_flag = up_flag;

    return;
}
static void rtp_entry(void *parameter) {
    int cnt = 0;
    struct rt_touch_data *data;
    int max = (int)(long)parameter;
    data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data));

    char cal_buf[sizeof(float) * AIC_CALI_POINT_NUM];
    int cali_cnt;

    int fd = open(AIC_POINTERCAL_PATH, O_RDONLY);
    if (fd >= 0) {
        read(fd, cal_buf, AIC_CALI_POINT_NUM * sizeof(float));
        for (cali_cnt = 0; cali_cnt < AIC_CALI_POINT_NUM; cali_cnt++) {
            g_cal.a[cali_cnt] = *(int *)(cal_buf + cali_cnt * sizeof(float));
        }
        close(fd);
    }

    rt_kprintf("Try to read %d points from RTP ...\n", max);
    rt_device_control(rtp_device, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

    while (cnt < max) {
        if (rt_sem_take(rtp_sem, RT_WAITING_FOREVER) != RT_EOK)
            break;

        if (rt_device_read(rtp_device, 0, data, g_draw_count) != g_draw_count)
            continue;

        rtp_check_event_type(data->event, data->pressure);
        if (data->event != RT_TOUCH_EVENT_DOWN)
            continue;
        rt_device_control(rtp_device, RT_TOUCH_CTRL_SET_X_TO_Y, (void *)data);
        test_draw_a_point(cnt, data, &g_cal);
        cnt++;
    };

    rt_free(data);
    g_last_up_flag = 1;
    rt_device_control(rtp_device, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return;
}

static int rtp_save_cali_param(calibration *cal) {
    int fd;
    int cali_cnt;
    char cal_buf[sizeof(float) * AIC_CALI_POINT_NUM];

    if (open(AIC_CONFIG_PATH, O_RDONLY) < 0) {
        mkdir(AIC_CONFIG_PATH, AIC_CONFIG_FOLDER_PERMISSION);
    }
    fd = open(AIC_POINTERCAL_PATH, O_WRONLY | O_CREAT);

    if (fd > 0) {
        for (cali_cnt = 0; cali_cnt < AIC_CALI_POINT_NUM; cali_cnt++) {
            memcpy(cal_buf + cali_cnt * sizeof(float), &cal->a[cali_cnt],
                sizeof(float));
        }
        write(fd, cal_buf, AIC_CALI_POINT_NUM * sizeof(float));
        close(fd);
    } else {
        rt_kprintf("open file failed!\n");
    }
    return 0;
}

static int rtp_perform_calibration(calibration *cal) {
    int j;
    float n, x, y, x2, y2, xy, z, zx, zy;
    float det, a, b, c, e, f, i;
    float scaling = AIC_CALI_ACCURACY;
    int temp[7] = { 0 };

    memcpy(cal->a, temp, sizeof(temp));
    /* Get sums for matrix */
    n = x = y = x2 = y2 = xy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        n += 1.0;
        x += (float)cal->x[j];
        y += (float)cal->y[j];
        x2 += (float)(cal->x[j] * cal->x[j]);
        y2 += (float)(cal->y[j] * cal->y[j]);
        xy += (float)(cal->x[j] * cal->y[j]);
    }

    /* Get determinant of matrix -- check if determinant is too small */
    det = n * (x2 * y2 - xy * xy) + x * (xy * y - x * y2) + y * (x * xy - y * x2);
    if (det < 0.1 && det > -0.1) {
        printf("ts_calibrate: determinant is too small -- %f\n", det);
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
        z += (float)cal->xfb[j];
        zx += (float)(cal->xfb[j] * cal->x[j]);
        zy += (float)(cal->xfb[j] * cal->y[j]);
    }

    /* Now multiply out to get the calibration for framebuffer x coord */
    cal->a[0] = (int)((a * z + b * zx + c * zy) * (scaling));
    cal->a[1] = (int)((b * z + e * zx + f * zy) * (scaling));
    cal->a[2] = (int)((c * z + f * zx + i * zy) * (scaling));

    /* Get sums for y calibration */
    z = zx = zy = 0;
    for (j = 0; j < AIC_POINT_NUM; j++) {
        z += (float)cal->yfb[j];
        zx += (float)(cal->yfb[j] * cal->x[j]);
        zy += (float)(cal->yfb[j] * cal->y[j]);
    }

    /* Now multiply out to get the calibration for framebuffer y coord */
    cal->a[3] = (int)((a * z + b * zx + c * zy) * (scaling));
    cal->a[4] = (int)((b * z + e * zx + f * zy) * (scaling));
    cal->a[5] = (int)((c * z + f * zx + i * zy) * (scaling));

    /* If we got here, we're OK, so assign scaling to a[6] and return */
    cal->a[6] = (int)scaling;

    rtp_save_cali_param(cal);

    return 1;
}

/* Calculate the average value of multiple points triggered by one click as
 * the calibration point. Among them, the calibration point is the touch
 * screen coordinate system */
static void rtp_get_valid_point(calibration *cal, int index, struct rt_touch_data *data) {
    int x = 0, y = 0;
    int cnt = 0;
    u32 tp_x = 0, tp_y = 0;
    int sum_x = 0;
    int sum_y = 0;
    int pressed_flag = 0;

    rtp_sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_FIFO);
    rt_device_control(rtp_device, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

    do {
        if (rt_sem_take(rtp_sem, RT_WAITING_FOREVER) != RT_EOK)
            break;

        if (rt_device_read(rtp_device, 0, data, 1) != 1)
            continue;

        rtp_check_event_type(data->event, data->pressure);

        if (data->event == RT_TOUCH_EVENT_UP) {
            if (pressed_flag)
                break;

            continue;
        }

        if (data->x_coordinate > 0 || data->y_coordinate > 0) {
            pressed_flag = 1;
            rt_device_control(rtp_device, RT_TOUCH_CTRL_SET_X_TO_Y, (void *)data);
            x = data->x_coordinate;
            y = data->y_coordinate;
            sum_x += x;
            sum_y += y;
            cnt++;
        }

    } while (1);

    x = sum_x / cnt;
    y = sum_y / cnt;

    /* ADC value converted to touch panel's coordinate value */
    tp_x = AIC_RTP_MAX_VAL - x;
    tp_y = AIC_RTP_MAX_VAL - y;
    tp_x = (tp_x * screen_info.width) / AIC_RTP_MAX_VAL;
    tp_y = (tp_y * screen_info.height) / AIC_RTP_MAX_VAL;
    cal->x[index] = tp_x;
    cal->y[index] = tp_y;

    rt_kprintf("Calibration: X = %d, Y = %d\n", tp_x, tp_y);
    g_last_up_flag = 1;
    rt_device_control(rtp_device, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);

    return;
}
/* Draw a cross, and each line size: 50 */
static void rtp_draw_cross(calibration *cal, int index, char *name, int y,
    int x) {
    u32 i;
    u8 *fb = screen_info.framebuffer;
    u8 rate = screen_info.bits_per_pixel / AIC_BITS_TO_BYTE_RATE;
    int length = AIC_CROSS_LENGTH;

    memset(fb, 0, screen_info.smem_len);
    memset(fb + screen_info.stride * (y + length / 2) + rate * x, 0xFF, rate * length);

    for (i = 0; i < length; i++)
        memset(fb + screen_info.stride * (y + i) + rate * (x + length / 2), 0xFF, rate);

    cal->xfb[index] = x + length / 2;
    cal->yfb[index] = y + length / 2;
    rt_kprintf("%s : X = %4d Y = %4d\n", name, cal->xfb[index],
        cal->yfb[index]);

    aicos_dcache_clean_invalid_range(screen_info.framebuffer,
        screen_info.smem_len);

    return;
}
static void rtp_calibrate(calibration *cal) {
    int length = AIC_CROSS_LENGTH;
    int width = AIC_CROSS_WIDTH;
    int height = AIC_CROSS_HEIGHT;

    struct rt_touch_data *data = RT_NULL;
    data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data));
    memset(data, 0, sizeof(&data));
    memset(cal, 0, sizeof(&cal));

    rtp_draw_cross(cal, 0, "Top left", height, width);
    rtp_get_valid_point(cal, 0, data);

    rtp_draw_cross(cal, 1, "Top right", height, g_xres - width - length);
    rtp_get_valid_point(cal, 1, data);

    rtp_draw_cross(cal, 2, "Bot right", g_yres - height - length,
        g_xres - width - length);
    rtp_get_valid_point(cal, 2, data);

    rtp_draw_cross(cal, 3, "Bot left", g_yres - height - length, width);
    rtp_get_valid_point(cal, 3, data);

    rtp_draw_cross(cal, 4, "Center", (g_yres - length) / 2,
        (g_xres - length) / 2);
    rtp_get_valid_point(cal, 4, data);

    memset(screen_info.framebuffer, 0, screen_info.smem_len);
    rtp_perform_calibration(cal);
    rt_free(data);

    return;
}
static void rtp_draw(long cnt) {
    rtp_sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_PRIO);
    if (rtp_sem == RT_NULL) {
        rt_kprintf("create dynamic semaphore failed.\n");
        return;
    }

    // rtp_draw_grid();
    rtp_thread = rt_thread_create("thread", rtp_entry, (void *)cnt, 2048, 25, 5);
    if (rtp_thread != RT_NULL)
        rt_thread_startup(rtp_thread);

    if (fb)
        mpp_fb_close(fb);

    return;
}

void rtp_draw_panel(void) {
    int ret;
    static int draw_point_num = AIC_DRAW_POINT_NUM;

    rtp_get_fb_info();
    rtp_device = rt_device_find(AIC_RTP_NAME);
    if (rtp_device == RT_NULL) {
        rt_kprintf("Failed to find %s device\n", AIC_RTP_NAME);
        return;
    }

    if (!g_opened) {
        ret = rt_device_open(rtp_device, RT_DEVICE_FLAG_INT_RX);
        if (ret != RT_EOK) {
            rt_kprintf("Failed to open %s device\n", AIC_RTP_NAME);
            return;
        }
        g_opened = 1;
        rt_kprintf("g_opened %s device\n", AIC_RTP_NAME);
    }
    rt_device_set_rx_indicate(rtp_device, rtp_rx_callback);
    rtp_calibrate(&g_cal);
    rtp_draw(draw_point_num);


}

#endif