#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"

#include "frame_allocator.h"
#include "mpp_fb.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_ge.h"

#include "lcd_test.h"

static struct aicfb_screeninfo g_screen_info = { 0 };

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

    if (g_screen_info.width)
        return &g_screen_info;

    fb = mpp_fb_open();
    if (!fb)
        return NULL;

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &g_screen_info);
    if (ret) {
        loge("get screen info failed\n");
        return NULL;
    }

    mpp_fb_close(fb);
    return &g_screen_info;
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

    struct mpp_fb *fb = NULL;
    fb = mpp_fb_open();

    int input_fd = open(file_path, O_RDONLY);
    int file_len = get_file_size(input_fd, file_path);

    /* 1. 创建mpp decoder */
    struct mpp_decoder *decoder = mpp_decoder_create(type);

    struct decode_config config;
    config.bitstream_buffer_size = (file_len + 1023) & (~1023);
    config.extra_frame_num = 0;
    config.packet_count = 1;
    config.pix_fmt = MPP_FMT_ARGB_8888;

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
        goto _exit;
    }

    /* 7. 获取解码后视频帧数据 */
    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(decoder, &frame);

    /* 8. 显示该视频帧 */
    render_frame(fb, &frame, 0, 0, 0, 0, AICFB_LAYER_TYPE_UI);

    /* 9. 归还该视频帧 */
    mpp_decoder_put_frame(decoder, &frame);

_exit:

    /* 10. 销毁 mpp_decoder */
    mpp_decoder_destory(decoder);

    if (fb)
        mpp_fb_close(fb);

    if (input_fd > 0)
        close(input_fd);

}

static lcd_fill_color(int start_color, int end_color) {

    struct mpp_ge *ge = NULL;
    struct mpp_fb *fb = NULL;
    struct ge_fillrect fill;
    int width, height;
    unsigned int framebuf = 0;

    fb = mpp_fb_open();

    int ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &g_screen_info);
    if (ret) {
        LOG_E("get screen info failed\n");
        goto _exit;
    }

    framebuf = (unsigned long)g_screen_info.framebuffer;
    width = g_screen_info.width;
    height = g_screen_info.height;
    LOG_D("FB: 0x%08x, width: %d, height: %d\n", framebuf, width, height);

    ge = mpp_ge_open();
    if (!ge) {
        LOG_E("GE open fail\n");
        goto _exit;
    }

#define APP_FB_NUM 2

    int index = 0;

    memset(&fill, 0, sizeof(struct ge_fillrect));

    fill.type = GE_H_LINEAR_GRADIENT;
    fill.start_color = start_color;
    fill.end_color = end_color;
    fill.dst_buf.buf_type = MPP_PHY_ADDR;

    if (!index) {
        fill.dst_buf.phy_addr[0] = framebuf;
    } else {
        if (APP_FB_NUM > 1) {
            fill.dst_buf.phy_addr[0] = framebuf + g_screen_info.smem_len;
        } else {
            fill.dst_buf.phy_addr[0] = framebuf;
        }
    }

    fill.dst_buf.stride[0] = g_screen_info.stride;
    fill.dst_buf.size.width = g_screen_info.width;
    fill.dst_buf.size.height = g_screen_info.height;
    fill.dst_buf.format = g_screen_info.format;
    fill.ctrl.flags = 0;
    fill.ctrl.alpha_en = 0;

    fill.dst_buf.crop_en = 0;
    fill.dst_buf.crop.x = 0;
    fill.dst_buf.crop.y = 0;
    fill.dst_buf.crop.width = width;
    fill.dst_buf.crop.height = height;

    ret = mpp_ge_fillrect(ge, &fill);
    if (ret < 0) {
        LOG_E("ge fillrect fail\n");
    }

    ret = mpp_ge_emit(ge);
    if (ret < 0) {
        LOG_E("ge emit fail\n");
    }

    ret = mpp_ge_sync(ge);
    if (ret < 0) {
        LOG_E("ge sync fail\n");
        goto _exit;
    }

    if (APP_FB_NUM > 1) {
        ret = mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &index);
        if (ret == 0) {
            ret = mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, &index);
            if (ret < 0)
                LOG_E("wait for sync error\n");
        } else {
            LOG_E("pan display fail\n");
        }
    }

    index = !index;

    LOG_D("ge fill test success\n");

_exit:
    mpp_ge_close(ge);
    mpp_fb_close(fb);
    return;
}

static rt_thread_t lcd_test_thread = RT_NULL;
#define LCD_TEST_DELAY_MS 1000

static void lcd_delay(void) {
    aic_mdelay(LCD_TEST_DELAY_MS);
}

/* LCD测试线程入口 */
static void lcd_test_entry(void *param) {

    while (1) {
        lcd_decode_img_path("rodata/lcd_test/image/fruit640x480.jpg");
        lcd_delay();
        lcd_decode_img_path("rodata/lcd_test/image/11.png");
        lcd_delay();
        lcd_fill_color(0x0000ff, 0xffffff);
        lcd_delay();
        lcd_fill_color(0xff0000, 0xffffff);
        lcd_delay();
    }

}

/* 启动LCD测试线程,主函数调用 */
void lcd_test_start(void) {
    lcd_test_thread = rt_thread_create("display_test", lcd_test_entry, NULL, 8 * 1024, 25, 5);
    if (lcd_test_thread != RT_NULL) {
        rt_thread_startup(lcd_test_thread);
    }
}








