#include "draw_panel.h"

#define LOG_TAG "Draw_panel"

static rt_thread_t draw_thread = RT_NULL;
static rt_thread_t touch_read_thread = RT_NULL;
static rt_sem_t touch_semaphore;
static rt_device_t touch_device;

static struct rt_touch_data *touch_data;
static struct rt_touch_data *prev_data;

static struct rt_touch_info touch_info;

static struct mpp_fb *fb = RT_NULL;
static struct aicfb_screeninfo screen_info;


static rt_err_t rx_callback(rt_device_t dev, rt_size_t size) {
    rt_sem_release(touch_semaphore);
    rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return 0;
}
void touch_init(void) {
    touch_device = rt_device_find("gt911");
    if (touch_device == RT_NULL) {
        LOG_E("touch device not found!");
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

    touch_semaphore = rt_sem_create("touch_semaphore", 0, RT_IPC_FLAG_FIFO);
    if (touch_semaphore == RT_NULL) {
        LOG_E("create touch semaphore failed!");
        return;
    }

}

void touch_read_point(void *param) {
    touch_init();
    rt_device_control(touch_device, RT_TOUCH_CTRL_ENABLE_INT, &touch_info);
    prev_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * touch_info.point_num);
    touch_data = (struct rt_touch_data *)rt_malloc(sizeof(struct rt_touch_data) * touch_info.point_num);
    while (1) {
        rt_sem_take(touch_semaphore, RT_WAITING_FOREVER);

        if (rt_device_read(touch_device, 0, touch_data, touch_info.point_num) > 0) {
            for (rt_uint8_t i = 0;i < touch_info.point_num;i++) {
                if (touch_data[i].event == RT_TOUCH_EVENT_DOWN ||
                    touch_data[i].event == RT_TOUCH_EVENT_MOVE ||
                    touch_data[i].event == RT_TOUCH_EVENT_UP) {
                    LOG_I("%d %d %d %d", touch_data[i].track_id,
                        touch_data[i].x_coordinate,
                        touch_data[i].y_coordinate,
                        touch_data[i].event);

                }
            }
        }
        rt_memcpy(prev_data, touch_data, sizeof(struct rt_touch_data) * touch_info.point_num);
        // prev_data=touch_data;

        rt_device_control(touch_device, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
    }
}
void open_panel(void) {
    int ret = 0;
    fb = mpp_fb_open();
    if (!fb) {
        LOG_E("mpp_fb_open error!\n");
        return -1;
    }
    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &screen_info);
    if (ret < 0) {
        LOG_E("mpp fb ioctl failed!error:%d\n", ret);
        return -1;
    }
    memset(screen_info.framebuffer, 0, screen_info.height * screen_info.stride);
    LOG_I("Screen width: %d, height: %d\n", screen_info.width, screen_info.height);

}

void panel_draw_lines(void *param) {
    open_panel();
    struct frame_buffer_info fb_info = {
        .frame_buffer_format = screen_info.format,
        .frame_buffer_width = screen_info.width,
        .frame_buffer_height = screen_info.height,
        .frame_buffer = (uint8_t *)screen_info.framebuffer,
    };

    while (1) {
        struct line_dsc line;
        if (touch_data[0].event == RT_TOUCH_EVENT_DOWN ||
            touch_data[0].event == RT_TOUCH_EVENT_MOVE ||
            touch_data[0].event == RT_TOUCH_EVENT_UP) {
            if (prev_data->x_coordinate != 0 || prev_data->y_coordinate != 0) {

                line.x1 = touch_data[0].x_coordinate;
                line.y1 = touch_data[0].y_coordinate;
                line.x2 = touch_data[0].x_coordinate + 1;
                line.y2 = touch_data[0].y_coordinate + 1;
                line.color = 0x00ff00;
                line.width = 8;

                // LOG_W("point1: %d,%d", line.x1, line.y1);
                // LOG_W("point2: %d,%d", line.x2, line.y2);
                // draw_line(&line, &fb_info);

                draw_wide_pixel(touch_data[0].x_coordinate, touch_data[0].y_coordinate, 8, 0x00ff00, &fb_info);
            }
            // prev_data = touch_data;
        } else {
            // prev_data->x_coordinate = 0;
            // prev_data->y_coordinate = 0;
        }

        aicos_dcache_clean_range((unsigned long *)screen_info.framebuffer, screen_info.smem_len);

        if (fb) {
            mpp_fb_close(fb);
        }
    }
}

void draw_start() {

    touch_read_thread = rt_thread_create("read_thread", touch_read_point, RT_NULL, 2 * 1024, 20, 5);
    if (touch_read_thread != RT_NULL) {
        rt_thread_startup(touch_read_thread);
    }
    LOG_W("read thread startup");

    draw_thread = rt_thread_create("draw_thread", panel_draw_lines, RT_NULL, 2 * 1024, 21, 5);
    if (draw_thread != RT_NULL) {
        rt_thread_startup(draw_thread);
    }
    LOG_W("draw thread startup");
}



