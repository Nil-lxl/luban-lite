#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "aic_hal.h"

#include "frame_allocator.h"
#include "mpp_fb.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_ge.h"

#include "lcd_test.h"
#include "touch_test.h"

struct aicfb_screeninfo screen_info;

static int get_file_size(int fd, char *path) {
    struct stat st;
    stat(path, &st);

    LOG_D("mode: %"PRIu32", size: %ld", st.st_mode, st.st_size);

    return st.st_size;
}


static int ge_bitblt(struct ge_bitblt *blt) {
    int ret = 0;
    struct mpp_ge *ge = mpp_ge_open();

    if (!ge) {
        loge("open ge device error\n");
    }

    ret = mpp_ge_bitblt(ge, blt);
    if (ret < 0) {
        loge("bitblt task failed\n");
        return ret;
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        loge("emit task failed\n");
        return ret;
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        loge("ge sync fail\n");
        return ret;
    }

    mpp_ge_close(ge);

    return 0;
}

static u32 *g_vlayer_addr = NULL;
#ifdef AIC_PAN_DISPLAY
static u32 g_fb_buf_index = 0;
#endif

static struct aicfb_screeninfo *get_screen_info(void) {
    struct mpp_fb *fb = NULL;
    int ret;

    if (screen_info.width)
        return &screen_info;

    fb = mpp_fb_open();
    if (!fb)
        return NULL;

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &screen_info);
    if (ret) {
        loge("get screen info failed\n");
        return NULL;
    }

    mpp_fb_close(fb);
    return &screen_info;
}

static void ui_layer_buf_sync(void) {
#ifdef AIC_PAN_DISPLAY
    struct ge_bitblt blt = { 0 };
    struct aicfb_screeninfo *info = NULL;
    uint8_t *src_buf = NULL, *dst_buf = NULL;

    info = get_screen_info();
    if (info == NULL)
        return;

    if (g_fb_buf_index) {
        src_buf = info->framebuffer + info->smem_len;
        dst_buf = info->framebuffer;
    } else {
        src_buf = info->framebuffer;
        dst_buf = info->framebuffer + info->smem_len;
    }

    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (u32)(long)src_buf;
    blt.src_buf.format = info->format;
    blt.src_buf.stride[0] = info->stride;
    blt.src_buf.size.width = info->width;
    blt.src_buf.size.height = info->height;
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = (u32)(long)dst_buf;
    blt.dst_buf.format = info->format;
    blt.dst_buf.stride[0] = info->stride;
    blt.dst_buf.size.width = info->width;
    blt.dst_buf.size.height = info->height;
    ge_bitblt(&blt);
#endif
}

static void render_frame(struct mpp_fb *fb, struct mpp_frame *frame,
    u32 offset_x, u32 offset_y, u32 width, u32 height,
    u32 layer_id) {
    struct ge_bitblt blt;
    struct aicfb_screeninfo *info = NULL;
    struct aicfb_layer_data layer = { 0 };
#ifdef AIC_PAN_DISPLAY
    u32 disp_buf_addr;
    u32 fb0_buf_addr;
#endif
    u32 dst_buf_addr;

    info = get_screen_info();
    if (info == NULL)
        return;

    if (layer_id == AICFB_LAYER_TYPE_UI) {
        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        if (mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &layer) < 0) {
            loge("get ui layer config failed\n");
            return;
        }

#ifdef AIC_PAN_DISPLAY
        disp_buf_addr = (u32)layer.buf.phy_addr[0];
        fb0_buf_addr = (u32)(unsigned long)info->framebuffer;
        /* Switch the double-buffer */
        if (disp_buf_addr == fb0_buf_addr) {
            dst_buf_addr = fb0_buf_addr + info->smem_len;
            g_fb_buf_index = 1;
        } else {
            dst_buf_addr = fb0_buf_addr;
            g_fb_buf_index = 0;
        }
#else
        dst_buf_addr = layer.buf.phy_addr[0];
#endif
    } else {
        layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
        if (mpp_fb_ioctl(fb, AICFB_GET_LAYER_CONFIG, &layer) < 0) {
            loge("get video layer config failed\n");
            return;
        }
        dst_buf_addr = layer.buf.phy_addr[0];
    }
    logi("Render to buf 0x%08x\n", dst_buf_addr);

    memset(&blt, 0, sizeof(struct ge_bitblt));
    memcpy(&blt.src_buf, &frame->buf, sizeof(struct mpp_buf));

    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = dst_buf_addr;
    blt.dst_buf.format = info->format;
    blt.dst_buf.stride[0] = info->stride;
    blt.dst_buf.size.width = info->width;
    blt.dst_buf.size.height = info->height;
    blt.dst_buf.crop_en = 1;

    blt.dst_buf.crop.x = offset_x;
    blt.dst_buf.crop.y = offset_y;
    blt.dst_buf.crop.width = blt.src_buf.crop.width;
    blt.dst_buf.crop.height = blt.src_buf.crop.height;

    logi("phy_addr: %x, stride: %d", blt.src_buf.phy_addr[0], blt.src_buf.stride[0]);
    logi("width: %d, height: %d, format: %d", blt.src_buf.size.width, blt.src_buf.size.height, blt.src_buf.format);

    ge_bitblt(&blt);

#ifdef AIC_PAN_DISPLAY
    if (layer_id == AICFB_LAYER_TYPE_UI) {
        /* display at last */
        mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &g_fb_buf_index);
        mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, 0);
        ui_layer_buf_sync();
    }
#endif
}

#define APP_FB_NUM      2   /* 定义双fb缓冲区 */

static struct mpp_fb *fb = NULL;
static struct mpp_ge *ge = NULL;
static int buf_index = 1;
static unsigned int framebuf[2] = { 0 };

int lcd_ge_begin(void) {
    fb = mpp_fb_open();

    int ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &screen_info);
    if (ret) {
        LOG_E("get screen info failed\n");
        return RT_ERROR;
    }
    framebuf[0] = (unsigned long)screen_info.framebuffer;
    framebuf[1] = framebuf[0] + screen_info.smem_len;

    ge = mpp_ge_open();
    if (!ge) {
        LOG_E("GE open fail\n");
        return RT_ERROR;
    }
    buf_index = !buf_index;
    return 0;
}

static void lcd_ge_flush(void) {
    mpp_ge_emit(ge);
    mpp_ge_sync(ge);
    if (APP_FB_NUM > 1) {
        int ret = mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &buf_index);
        if (ret == 0) {
            mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, &buf_index);
            if (ret < 0)
                LOG_E("wait for sync error\n");
        } else {
            LOG_E("pan display fail\n");
        }
    }
}

static void lcd_decode_img_path(char *file_path) {
    int type;
    char *ptr = strrchr(file_path, '.');

    if (!strcmp(ptr, ".jpg")) {
        type = MPP_CODEC_VIDEO_DECODER_MJPEG;
    }
    if (!strcmp(ptr, ".png")) {
        type = MPP_CODEC_VIDEO_DECODER_PNG;
    }
    if (!strcmp(ptr, ".aicp")) {
        type = MPP_CODEC_VIDEO_DECODER_AICP;
    }

    lcd_ge_begin();

    int input_fd = open(file_path, O_RDONLY);
    int file_len = get_file_size(input_fd, file_path);

    /* 1. 创建mpp decoder */
    struct mpp_decoder *decoder = mpp_decoder_create(type);

    struct decode_config config;
    config.bitstream_buffer_size = (file_len + 1023) & (~1023);
    config.extra_frame_num = 0;
    config.packet_count = 1;
#ifdef AICFB_RGB565
    config.pix_fmt = MPP_FMT_RGB_565;
#else
    config.pix_fmt = MPP_FMT_ARGB_8888;
#endif

#ifdef AIC_VE_DRV_V10
    if (type == MPP_CODEC_VIDEO_DECODER_MJPEG)
        config.pix_fmt = MPP_FMT_NV12;
#endif

    /* 2. 初始化mpp decoder */
    mpp_decoder_init(decoder, &config);

    /* 3. 获取一个空的 packet */
    struct mpp_packet packet;
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(decoder, &packet, file_len);

    /* 4. 把视频码流数据拷贝到 packet */
    int r_len;
    if (input_fd > 0) {
        r_len = read(input_fd, packet.data, file_len);
    } else {
        memcpy(packet.data, ptr, file_len);
        r_len = file_len;
        LOG_W("Read image from 0x%lx, length %d", (long)ptr, file_len);
    }
    packet.size = r_len;
    packet.flag = PACKET_FLAG_EOS;
    LOG_D("read len: %d, file_len: %d\n", r_len, file_len);

    /* 5. 归还 packet */
    mpp_decoder_put_packet(decoder, &packet);

    /* 6. 解码该笔码流数据 */
    int ret = mpp_decoder_decode(decoder);
    if (ret < 0) {
        LOG_E("decode error");
        return;
    }

    /* 7. 获取解码后视频帧数据 */
    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(decoder, &frame);

    /* 8. 显示该视频帧 */
    render_frame(fb, &frame, 0, 0, 0, 0, AICFB_LAYER_TYPE_UI);

    /* 9. 归还该视频帧 */
    mpp_decoder_put_frame(decoder, &frame);

    /* 10. 销毁 mpp_decoder */
    mpp_decoder_destory(decoder);

    /* 释放ge和fb资源,避免内存泄漏导致系统崩溃 */
    mpp_ge_close(ge);
    mpp_fb_close(fb);

    if (input_fd > 0)
        close(input_fd);

}

/**
 * @brief 屏幕任意位置填充矩形
 * @param color 填充的矩形颜色
 * @param x 矩形左上角的x坐标
 * @param y 矩形左上角的y坐标
 * @param w 矩形的宽度
 * @param h 矩形的高度
 */
static void lcd_fill_rect(uint32_t color, int x, int y, int w, int h) {
    struct ge_fillrect fill;
    memset(&fill, 0, sizeof(struct ge_fillrect));

    fill.type = GE_NO_GRADIENT;
    fill.start_color = color;
    fill.end_color = 0;
    fill.dst_buf.buf_type = MPP_PHY_ADDR;
    fill.dst_buf.phy_addr[0] = framebuf[buf_index];
    fill.dst_buf.stride[0] = screen_info.stride;

    fill.dst_buf.size.width = screen_info.width;
    fill.dst_buf.size.height = screen_info.height;
    fill.dst_buf.format = screen_info.format;
    fill.ctrl.flags = 0;
    fill.ctrl.alpha_en = 0;

    fill.dst_buf.crop_en = 1;   //不启用裁剪,默认全屏填充
    fill.dst_buf.crop.x = x;    //裁剪区域的x坐标
    fill.dst_buf.crop.y = y;    //裁剪区域的y坐标
    fill.dst_buf.crop.width = w;    //裁剪区域的宽度
    fill.dst_buf.crop.height = h;  //裁剪区域的高度

    mpp_ge_fillrect(ge, &fill);

}

/**
 * @brief 填充整个屏幕
 * @param start_color 起始颜色
 * @param end_color 结束颜色(无渐变为0)
 * @param is_gradient 是否渐变
 */
static void lcd_fill_color(int start_color, int end_color, int is_gradient) {
    if (lcd_ge_begin()) {
        return;
    }
    struct ge_fillrect fill;
    memset(&fill, 0, sizeof(struct ge_fillrect));

    fill.type = is_gradient ? GE_H_LINEAR_GRADIENT : GE_NO_GRADIENT;;
    fill.start_color = start_color;
    fill.end_color = end_color;
    fill.dst_buf.buf_type = MPP_PHY_ADDR;

    fill.dst_buf.phy_addr[0] = framebuf[buf_index];
    fill.dst_buf.stride[0] = screen_info.stride;

    fill.dst_buf.size.width = screen_info.width;
    fill.dst_buf.size.height = screen_info.height;
    fill.dst_buf.format = screen_info.format;

    fill.ctrl.flags = 0;
    fill.ctrl.alpha_en = 0;
    fill.dst_buf.crop_en = 0;

    mpp_ge_fillrect(ge, &fill);
    lcd_ge_flush();

    /* 释放ge和fb资源,避免内存泄漏导致系统崩溃 */
    mpp_ge_close(ge);
    mpp_fb_close(fb);
}

/**
 * @brief 绘制屏幕边框
 * @param bg_color 背景颜色
 * @param border_color 边框颜色
 * @param border_width 边框宽度
 */
static void lcd_fill_border(int bg_color, int border_color, int border_width) {
    if (lcd_ge_begin()) {
        return;
    }
    int width = screen_info.width;
    int height = screen_info.height;

    /* 设置背景颜色 */
    lcd_fill_rect(bg_color, 0, 0, width, height);
    /* 上边框 */
    lcd_fill_rect(border_color, 0, 0, width, border_width);
    /* 左边框 */
    lcd_fill_rect(border_color, 0, 0, border_width, height);
    /* 右边框 */
    lcd_fill_rect(border_color, width - border_width, 0, border_width, height);
    /* 下边框 */
    lcd_fill_rect(border_color, 0, height - border_width, width, border_width);

    lcd_ge_flush();

    /* 释放ge和fb资源,避免内存泄漏导致系统崩溃 */
    mpp_ge_close(ge);
    mpp_fb_close(fb);
}
const uint32_t gray_color[16] = {
    0x000000, 0x101010, 0x202020, 0x303030,
    0x404040, 0x505050, 0x606060, 0x707070,
    0x808080, 0x909090, 0xA0A0A0, 0xB0B0B0,
    0xC0C0C0, 0xD0D0D0, 0xE0E0E0, 0xF0F0F0
};

/**
 * @brief 显示灰度渐变
 * @param level 灰阶级数，通常为16
 */
static void lcd_fill_gray_level(int level) {
    if (lcd_ge_begin()) {
        return;
    }

    int width = screen_info.width;
    int height = screen_info.height;

    for (int i = 0;i < level;i++) {
        if (width > height) {   //横屏
            lcd_fill_rect(gray_color[i], i * (width / level), 0, width / level, height);
        } else {//竖屏
            lcd_fill_rect(gray_color[i], 0, i * (height / level), width, height / level);
        }
    }
    lcd_ge_flush();

    /* 释放ge和fb资源,避免内存泄漏导致系统崩溃 */
    mpp_ge_close(ge);
    mpp_fb_close(fb);

}

static void fill_border(lcd_test_param_t param) {
    lcd_fill_border(param.border.bg_color, param.border.border_color, param.border.border_width);
}

static void fill_color(lcd_test_param_t param) {
    lcd_fill_color(param.color.start_color, param.color.end_color, param.color.is_gradient);
}

static void decode_img(lcd_test_param_t param) {
    lcd_decode_img_path(param.img_path);
}
static void fill_gray_level(lcd_test_param_t param) {
    lcd_fill_gray_level(param.gray_level_num);
}
static void touchscreen_test(lcd_test_param_t param) {
    lcd_ge_begin();
    TouchScreen_Test(buf_index);
}

/**
 * @brief LCD测试项数组,每个测试项包含名称、测试函数、参数、延时和是否启用
 * @param name 测试项名称
 * @param func 测试函数指针
 * @param param 测试函数参数，使用联合体lcd_test_param_t，根据测试函数需求设置对应字段
 * @param delay_ms 测试项执行后延时时间，单位毫秒
 * @param enabled 是否启用该测试项，true表示启用，false表示跳过
 * @note 参数说明：border(背景色,边框色,边框宽度)，color(起始色,结束色,是否渐变)，
 *       img_path(图片路径)，gray_level_num(灰阶级数)
 */
static const lcd_test_item_t test_items[] = {
    // {"边框",       fill_border,         .param.border = { BLACK, WHITE, 2 },      1000, true},
    {"红色",       fill_color,          .param.color = { RED, 0, 0 },             1000, true},
    {"绿色",       fill_color,          .param.color = { GREEN, 0, 0 },           1000, true},
    {"蓝色",       fill_color,          .param.color = { BLUE, 0, 0 },            1000, true},
    // {"黄色",       fill_color,          .param.color = { YELLOW, 0, 0 },          1000, true},
    // {"白色",       fill_color,          .param.color = { WHITE, 0, 0 },           1000, true},
    // {"黑色",       fill_color,          .param.color = { BLACK, 0, 0 },           1000, true},
    {"灰阶",       fill_gray_level,     .param.gray_level_num = 16,               1000, true},
    {"图片1",      decode_img,          .param.img_path = "rodata/lcd_test/image/1024x600.jpg", 1000, true},
    // {"图片2",      decode_img,          .param.img_path = "rodata/lcd_test/image/1024x600-1.jpg", 2000, true},
    // {"图片3",      decode_img,          .param.img_path = "rodata/lcd_test/image/1024x600-2.jpg", 2000, true},
    // {"图片4",      decode_img,          .param.img_path = "rodata/lcd_test/image/1024x600-3.jpg", 2000, true},
    // {"图片5",      decode_img,          .param.img_path = "rodata/lcd_test/image/1024x600-4.jpg", 2000, true},
    // {"图片6",      decode_img,          .param.img_path = "rodata/lcd_test/image/480x800-5.jpg", 2000, true},
    // {"图片7",      decode_img,          .param.img_path = "rodata/lcd_test/image/480x800-6.jpg", 60000, true},
    // {"图片8",      decode_img,          .param.img_path = "rodata/lcd_test/image/720x720-7.png", 3000, true},
    // {"图片9",      decode_img,          .param.img_path = "rodata/lcd_test/image/800x1280-8.jpg", 3000, true},
    // {"图片10",     decode_img,          .param.img_path = "rodata/lcd_test/image/800x1280-9.jpg", 3000, true},
    // {"图片10",     decode_img,          .param.img_path = "rodata/lcd_test/image/800x1280-10.jpg", 3000, true},
    // {"图片11",     decode_img,          .param.img_path = "rodata/lcd_test/image/800x1280-11.jpg", 3000, true},

    {"触摸测试",      touchscreen_test,    .param = {0},                             10000, true},

};

static rt_thread_t lcd_test_thread = RT_NULL;

/* LCD测试线程入口 */
static void lcd_test_entry(void *param) {

    while (1) {
        for (int i = 0;i < sizeof(test_items) / sizeof(test_items[0]);i++) {
            if (test_items[i].enabled) {
                if (test_items[i].func == touchscreen_test) {
                    test_items[i].func(test_items[i].param);
                    // 触摸测试项执行一次后退出循环，避免重复创建触摸测试线程
                    LOG_I("Touchscreen test executed, exiting test loop");
                    return;
                }
                test_items[i].func(test_items[i].param);
                aic_mdelay(test_items[i].delay_ms);
            }

        }
    }

}

static unsigned int init_pin_status;
static int thread_suspended = 0;
struct rt_device_pwm *pwm_dev;
int brightness = 20;

/* 启动LCD测试线程,主函数调用 */
void lcd_test_start(void) {
    lcd_test_thread = rt_thread_create("display_test", lcd_test_entry, NULL, 8 * 1024, 25, 5);
    if (lcd_test_thread != RT_NULL) {
        rt_thread_startup(lcd_test_thread);
    }

    // pause_key_config();
    // pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
}

static void pwm_config(void) {

}

static void pause_key_config(void) {
    // 配置按键输入中断 使用PA.0作为按键输入
    u32 pin;
    unsigned int g, p;
    pin = rt_pin_get("PA.0");
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);
    // 获取按钮未被按下时引脚电平的初始状态
    hal_gpio_get_value(g, p, &init_pin_status);
    rt_kprintf("Current pin status: %d\n", init_pin_status);
    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING, pause_key_input_irq_handler, &pin);
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);
}

/**
 * @brief 按键输入中断处理函数
 * @param args 中断处理函数参数，包含按键引脚信息
 */
static void pause_key_input_irq_handler(void *args) {
    unsigned int value;
    unsigned int g, p;
    u32 pin = *((u32 *)(args));

    // rt_kprintf("Key Pressed, pause or resume the LCD test thread\n");

    if (!thread_suspended) {
        //暂停LCD测试线程
        rt_thread_suspend(lcd_test_thread);
        thread_suspended = 1;
        rt_kprintf("LCD test thread suspended\n");
    } else {
        //恢复LCD测试线程
        rt_thread_resume(lcd_test_thread);
        thread_suspended = 0;
        rt_kprintf("LCD test thread resumed\n");
    }
    // rt_pwm_set(pwm_dev, 1, 10000, 100 * brightness);
    // brightness += 10;

}






