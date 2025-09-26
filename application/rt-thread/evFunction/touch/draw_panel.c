#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "mpp_fb.h"

#include "draw_panel.h"


#define LOG_TAG "Draw_Panel"

#ifdef APP_USE_DRAW_LINE_TEST
#define TOUCH_IC_NAME APP_TOUCH_DEVICE

#define UI_BG_COLOR         0x18a000a8
#define UI_FG_CTRL          0x18a000b0
#define DE_CONFIG_UPDATE    0x18a00008

#define BG_BLUE_MASK        GENMASK(7, 0)
#define BG_GREEN_MASK       GENMASK(15, 8)
#define BG_RED_MASK         GENMASK(23, 16)

#define PIXELS_BLUE(x)      (((x) & 0xff) << 0)
#define PIXELS_GREEN(x)     (((x) & 0xff) << 8)
#define PIXELS_RED(x)       (((x) & 0xff) << 16)

static rt_thread_t draw_thread = RT_NULL;
static rt_thread_t touch_read_thread = RT_NULL;
static rt_sem_t touch_semaphore;
static rt_device_t touch_device;

static struct rt_touch_info touch_info;
static struct rt_touch_data *touch_data;
static struct rt_touch_data *prev_data;

static struct mpp_fb *fb = RT_NULL;
static struct aicfb_screeninfo screen_info;


static rt_err_t rx_callback(rt_device_t dev, rt_size_t size) {
    rt_sem_release(touch_semaphore);
    rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return 0;
}

void open_panel(void) {
    int ret = 0;
    fb = mpp_fb_open();
    if (!fb) {
        LOG_E("mpp_fb open error!\n");
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
static void bg_update_bits(unsigned int mask, unsigned int value) {
    void *base = (void *)UI_BG_COLOR;
    void *update = (void *)DE_CONFIG_UPDATE;
    unsigned int tmp, orig;

    orig = readl(base);

    tmp = orig & ~mask;
    tmp |= value & mask;

    writel(tmp, base);
    writel(1, update);
}
static void fg_color_disable(void) {
    void *base = (void *)UI_FG_CTRL;
    void *update = (void *)DE_CONFIG_UPDATE;

    writel(0, base);
    writel(1, update);
}
void set_bg_color(void) {
    struct aicfb_layer_data layer = { 0 };
    uint8_t red, green, blue;

    bg_update_bits(BG_RED_MASK, PIXELS_RED(255));
    bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(0));
    bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(0));
    fg_color_disable();
    rt_thread_mdelay(500);

    bg_update_bits(BG_RED_MASK, PIXELS_RED(0));
    bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(255));
    bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(0));
    rt_thread_mdelay(500);

    bg_update_bits(BG_RED_MASK, PIXELS_RED(0));
    bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(0));
    bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(255));
    rt_thread_mdelay(500);

    bg_update_bits(BG_RED_MASK, PIXELS_RED(255));
    bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(255));
    bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(0));
    rt_thread_mdelay(500);

    bg_update_bits(BG_RED_MASK, PIXELS_RED(204));
    bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(108));
    bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(231));
    rt_thread_mdelay(500);

    bg_update_bits(BG_RED_MASK, PIXELS_RED(255));
    bg_update_bits(BG_GREEN_MASK, PIXELS_GREEN(255));
    bg_update_bits(BG_BLUE_MASK, PIXELS_BLUE(255));
    rt_thread_mdelay(500);

    /* enable fg color */
    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &layer);

    layer.enable = 1;
    mpp_fb_ioctl(fb, AICFB_UPDATE_LAYER_CONFIG, &layer);

}
void touch_init(void) {
    touch_device = rt_device_find(APP_TOUCH_DEVICE);
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
                rt_thread_delay(1);

            }
            /* 获取上一次的触摸数据 */
            rt_memcpy(prev_data, touch_data, sizeof(struct rt_touch_data) * touch_info.point_num);

        }
        rt_device_control(touch_device, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
    }
}


void panel_draw_lines(void *param) {
    rt_thread_mdelay(100);

    struct frame_buffer_info fb_info = {
        .frame_buffer_format = screen_info.format,
        .frame_buffer_width = screen_info.width,
        .frame_buffer_height = screen_info.height,
        .frame_buffer = (uint8_t *)screen_info.framebuffer,
    };

    #if 1   //show display border
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
        border[i].color = 0xffffff;
        border[i].width = 5;
        draw_line(&border[i], &fb_info);
    }
    #endif

    while (1) {
        struct line_dsc line[touch_info.point_num];

        for (int i = 0;i < touch_info.point_num;i++) {
            if (touch_data[i].event == RT_TOUCH_EVENT_DOWN ||
                touch_data[i].event == RT_TOUCH_EVENT_MOVE ||
                touch_data[i].event == RT_TOUCH_EVENT_UP) {
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

                    // LOG_W("point1: %d,%d", line[i].x1, line[i].y1);
                    // LOG_W("point2: %d,%d", line[i].x2, line[i].y2);
                    draw_line(&line[i], &fb_info);
                }
                aicos_dcache_clean_range((unsigned long *)screen_info.framebuffer, screen_info.smem_len);
            }
        }
        aicos_dcache_clean_range((unsigned long *)screen_info.framebuffer, screen_info.smem_len);

        if (fb) {
            mpp_fb_close(fb);
        }
    }
}

void panel_draw_start() {
    open_panel();

    set_bg_color();

    touch_read_thread = rt_thread_create("touch_read", touch_read_point, RT_NULL, 2 * 1024, 20, 5);
    if (touch_read_thread != RT_NULL) {
        rt_thread_startup(touch_read_thread);
    }

    draw_thread = rt_thread_create("draw_line", panel_draw_lines, RT_NULL, 2 * 1024, 21, 5);
    if (draw_thread != RT_NULL) {
        rt_thread_startup(draw_thread);
    }
}

#endif


