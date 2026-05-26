#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "mpp_fb.h"

#include "touch_test.h"
#include "lcd_test.h"

#define LOG_TAG "Touch_Test"

static rt_sem_t touch_sem;
static rt_device_t touch_device;

static struct rt_touch_info touch_info;
static struct rt_touch_data *touch_data;
static struct rt_touch_data *prev_data;

extern struct aicfb_screeninfo screen_info;

static rt_err_t rx_callback(rt_device_t dev, rt_size_t size) {
    rt_sem_release(touch_sem);
    rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return 0;
}

void TouchScreen_Init(void) {
    touch_device = rt_device_find(AIC_TOUCH_PANEL_NAME);
    if (touch_device != RT_NULL) {
        LOG_I("found touch device %s", AIC_TOUCH_PANEL_NAME);
    } else {
        LOG_E("found touch device %s failed!", AIC_TOUCH_PANEL_NAME);
        return;
    }

    if (rt_device_open(touch_device, RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        LOG_E("open touch device failed!");
        return;
    } else {
        LOG_W("open touch device success!");
        rt_device_control(touch_device, RT_TOUCH_CTRL_GET_INFO, &touch_info);
        LOG_I("type       :%d", touch_info.type);
        LOG_I("point_num  :%d", touch_info.point_num);
        LOG_I("range_x    :%d", touch_info.range_x);
        LOG_I("range_y    :%d", touch_info.range_y);
    }

    rt_device_set_rx_indicate(touch_device, rx_callback);

    touch_sem = rt_sem_create("touch_semaphore", 0, RT_IPC_FLAG_FIFO);
    if (touch_sem != RT_NULL) {
        LOG_D("create touch semaphore success");
    } else {
        LOG_E("create touch semaphore failed!");
        return;
    }

}

void TouchTest_Thread(void *buf_index) {
    static unsigned char *framebuf = RT_NULL;
    TouchScreen_Init();   /* 初始化触摸设备 */
    rt_device_control(touch_device, RT_TOUCH_CTRL_ENABLE_INT, &touch_info); /* 使能触摸设备中断 */

    /* 获取屏幕信息并初始化帧缓冲区信息结构体 */
    struct frame_buffer_info fb_info = { 0 };

    fb_info.frame_buffer_format = screen_info.format;
    fb_info.frame_buffer_width = screen_info.width;
    fb_info.frame_buffer_height = screen_info.height;

    if (buf_index == 0) {
        framebuf = (unsigned long)screen_info.framebuffer + screen_info.smem_len;
        fb_info.frame_buffer = (uint8_t *)(screen_info.framebuffer + screen_info.smem_len);

    } else if (buf_index == 1) {
        framebuf = (unsigned long)screen_info.framebuffer;
        fb_info.frame_buffer = (uint8_t *)screen_info.framebuffer;
    }

    LOG_I("buf_index:%d", buf_index);

    if (framebuf == NULL) {
        pr_err("Invalid framebuffer\n");
        return -1;
    }
    memset(framebuf, 0, screen_info.smem_len);

    int pixel_size = screen_info.bits_per_pixel >> 3;
    int color = 0xFFFFFF;

    /* 初始化屏幕为白色 */
    for (int i = 0;i < screen_info.height;i++) {
        memset(framebuf, color, screen_info.width * pixel_size);
        framebuf += screen_info.stride;
    }
    aicos_dcache_clean_range(framebuf, screen_info.smem_len);

    printf("Framebuf: size %d, width %d, height %d, bits per pixel %d\n", screen_info.smem_len, screen_info.width, screen_info.height, screen_info.bits_per_pixel);

#if 1   /* 显示屏幕左上角方向标识 */
    struct line_dsc mark[1];
    mark[0].x1 = 0;
    mark[0].y1 = 0;
    mark[0].x2 = 50;
    mark[0].y2 = 50;

    mark[0].color = 0xff8800;
    mark[0].width = 50;
    draw_line(&mark[0], &fb_info);
#endif

#if 1   /* 显示屏幕边框 */
    struct line_dsc border[4];

    border[0].x1 = 0;
    border[0].x2 = screen_info.width;
    border[0].y1 = 0;
    border[0].y2 = 0;

    border[1].x1 = 0;
    border[1].x2 = 0;
    border[1].y1 = 0;
    border[1].y2 = screen_info.height;

    border[2].x1 = 0;
    border[2].x2 = screen_info.width;
    border[2].y1 = screen_info.height;
    border[2].y2 = screen_info.height;

    border[3].x1 = screen_info.width;
    border[3].x2 = screen_info.width;
    border[3].y1 = 0;
    border[3].y2 = screen_info.height;

    for (int i = 0;i < 4;i++) {
        border[i].color = 0xff0000;
        border[i].width = 5;
        draw_line(&border[i], &fb_info);
    }
#endif
    // aicos_dcache_clean_range(framebuf, screen_info.smem_len);

    prev_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * touch_info.point_num);
    touch_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * touch_info.point_num);

    if (prev_data && touch_data) {
        /* 初始化上一次的触摸数据为无效坐标和事件 */
        for (int i = 0;i < touch_info.point_num;i++) {
            prev_data[i].x_coordinate = -1;
            prev_data[i].y_coordinate = -1;
            prev_data[i].event = RT_TOUCH_EVENT_NONE;
        }
        if (touch_info.point_num > 10) {
            touch_info.point_num = 10;
        }
        memset(prev_data, 0, sizeof(struct rt_touch_data) * touch_info.point_num);
        memset(touch_data, 0, sizeof(struct rt_touch_data) * touch_info.point_num);
    } else {
        LOG_E("malloc touch data failed!");
        return;
    }

    struct line_dsc line[touch_info.point_num];

    while (1) {
        /* 等待信号量：未接收到新坐标数据则不处理 */
        rt_sem_take(touch_sem, RT_WAITING_FOREVER);

        /* 从触摸设备读取当前坐标数据到touch_data */
        rt_size_t read_size = rt_device_read(touch_device, 0, touch_data, touch_info.point_num);
        if (read_size > 0) {
            for (int i = 0;i < touch_info.point_num;i++) {
                /* 处理触摸事件：按下或移动时绘制线条，抬起时重置坐标和事件 */
                if (touch_data[i].event == RT_TOUCH_EVENT_DOWN || touch_data[i].event == RT_TOUCH_EVENT_MOVE) {
                    /* 只有当上一次的坐标有效时才绘制线条 */
                    if (prev_data[i].x_coordinate != 0 || prev_data[i].y_coordinate != 0) {
                        line[i].x1 = prev_data[i].x_coordinate;
                        line[i].y1 = prev_data[i].y_coordinate;
                        line[i].x2 = touch_data[i].x_coordinate;
                        line[i].y2 = touch_data[i].y_coordinate;
                        line[i].width = 5;

                        line[0].color = 0x00ff00;
                        line[1].color = 0xff00ff;
                        line[2].color = 0x2196f3;
                        line[3].color = 0xf44336;
                        line[4].color = 0xffc107;

                        // LOG_W("%d %d %d %d", prev_data[i].track_id,
                        //     prev_data[i].x_coordinate,
                        //     prev_data[i].y_coordinate,
                        //     prev_data[i].event);
                        // LOG_I("%d %d %d %d", touch_data[i].track_id,
                        //     touch_data[i].x_coordinate,
                        //     touch_data[i].y_coordinate,
                        //     touch_data[i].event);

                        // LOG_W("point1: %d,%d, point2: %d,%d", line[i].x1, line[i].y1, line[i].x2, line[i].y2);
                        draw_line(&line[i], &fb_info);
                    }

                } else if (touch_data[i].event == RT_TOUCH_EVENT_UP) {
                    prev_data[i].x_coordinate = -1;
                    prev_data[i].y_coordinate = -1;
                    touch_data[i].event = RT_TOUCH_EVENT_NONE;
                    // LOG_I("point %d up", touch_data[i].track_id);
                }
            }

            if (buf_index == 0) {
                aicos_dcache_clean_range(framebuf - screen_info.smem_len, screen_info.smem_len);
            } else if (buf_index == 1) {
                aicos_dcache_clean_range((unsigned long *)screen_info.framebuffer, screen_info.smem_len);
            }
            LOG_D("framebuf[%d],addr=%x info.framebuf=%x", buf_index, framebuf, screen_info.framebuffer);
            /* 获取上一次的触摸数据 */
            rt_memcpy(prev_data, touch_data, sizeof(struct rt_touch_data) * touch_info.point_num);
        }
        rt_device_control(touch_device, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);

    }
}

static rt_thread_t touch_test_thread = RT_NULL;

void TouchScreen_Test(int buf_index) {
    touch_test_thread = rt_thread_create("touch_test", TouchTest_Thread, (void *)buf_index, 4096, 20, 10);
    if (touch_test_thread != RT_NULL) {
        rt_thread_startup(touch_test_thread);
    }
}