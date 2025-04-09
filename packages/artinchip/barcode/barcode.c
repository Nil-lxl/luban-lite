/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <posix/string.h>
#include <drivers/pin.h>

#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"
#include "aic_drv_gpio.h"
#include "include/yydecoder.h"

#include "mpp_vin.h"
#include "drv_camera.h"
#include "artinchip_fb.h"
#include "mpp_fb.h"
#include "mpp_ge.h"
#include "drv_efuse.h"

#define BARCODE_DEMO_SUCCESS 1
#define BUFFER_SIZE 180 * 1024
#define VID_BUF_NUM             3
#define VID_BUF_PLANE_NUM       2
#define VID_SCALE_OFFSET        0
#define DATA_PREPARE_EVENT    1

#define ENABLE_DISPLAY
#define DVP_DISPLAY_ROTATION

#ifdef ENABLE_DISPLAY
#include "include/video_font_data.h"
#ifdef DVP_DISPLAY_ROTATION
static struct mpp_ge *g_ge_dev = NULL;
int g_rotation = MPP_ROTATION_270;
unsigned char *g_ge_out_buffer = NULL;
#endif
static struct mpp_fb *g_fb = NULL;
static struct aicfb_screeninfo g_fb_info = {0};
#endif

#define ALIGN_DOWM(x, align)    ((x) & ~(align - 1))
#define BEEP_PIN_NAME           "PA.4"

struct aic_dvp_data {
    int w;
    int h;
    int frame_size;
    int frame_cnt;
    int dst_fmt;  // output format
    struct mpp_video_fmt src_fmt;
    uint32_t num_buffers;
    struct vin_video_buf binfo;
};

#define MAX_BUF_LEN 256
struct aic_decode_data {
    bool dready;
    int w;
    int h;
    unsigned char *in_buffer;
    unsigned char out_buffer[MAX_BUF_LEN];
    aicos_mutex_t lock;
    aicos_event_t data_ready_event;
};

static struct aic_dvp_data g_vdata = {0};
static struct aic_decode_data g_ddata = {0};
static bool g_running = true;
unsigned char g_last_barcode[MAX_BUF_LEN] = {0};
static struct rt_device_pwm *pwm_dev = RT_NULL;

static int dvp_cfg(int width, int height, int format)
{
    int ret = 0;
    struct dvp_out_fmt f = {0};

    f.width = g_vdata.src_fmt.width;
    f.height = g_vdata.src_fmt.height;
    f.pixelformat = format;
    f.num_planes = VID_BUF_PLANE_NUM;

    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &f);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int dvp_subdev_set_fmt(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &g_vdata.src_fmt);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int sensor_get_fmt(void)
{
    int ret = 0;
    struct mpp_video_fmt f = {0};

    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &f);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        // return -1;
    }

    g_vdata.src_fmt = f;
    g_vdata.w = g_vdata.src_fmt.width;
    g_vdata.h = g_vdata.src_fmt.height;
    pr_info("Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x\n",
            f.width, f.height, f.code, f.bus_type, f.colorspace);

    if (f.bus_type == MEDIA_BUS_RAW8_MONO) {
        pr_info("Forbid the output format to YUV400\n");
        g_vdata.dst_fmt = MPP_FMT_YUV400;
    }

    return 0;
}

static int dvp_request_buf(struct vin_video_buf *vbuf)
{
    int i;

    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)vbuf) < 0) {
        pr_err("ioctl() failed!\n");
        return -1;
    }

    pr_info("Buf   Plane[0]     size   Plane[1]     size\n");
    for (i = 0; i < vbuf->num_buffers; i++) {
        pr_info("%3d 0x%x %8d 0x%x %8d\n", i,
            vbuf->planes[i * vbuf->num_planes].buf,
            vbuf->planes[i * vbuf->num_planes].len,
            vbuf->planes[i * vbuf->num_planes + 1].buf,
            vbuf->planes[i * vbuf->num_planes + 1].len);
    }

    return 0;
}



static void dvp_release_buf(int num)
{
#if 0
    int i;
    struct video_buf_info *binfo = NULL;

    for (i = 0; i < num; i++) {
        binfo = &g_vdata.binfo[i];
        if (binfo->vaddr) {
            munmap(binfo->vaddr, binfo->len);
            binfo->vaddr = NULL;
        }
    }
#endif
}

static int dvp_queue_buf(int index)
{
    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        pr_err("ioctl() failed!\n");
        return -1;
    }

    return 0;
}

static int dvp_dequeue_buf(int *index)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int dvp_start(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

static int dvp_stop(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        pr_err("ioctl() failed! err -%d\n", -ret);
        return -1;
    }

    return 0;
}

extern int usbd_keyboard_putnchar(const char *str, int n);

#ifdef ENABLE_DISPLAY

#ifdef DVP_DISPLAY_ROTATION
static void video_layer_set()
{
    struct aicfb_layer_data layer = {0};
    int ret = 0;
    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    layer.buf.buf_type = MPP_PHY_ADDR;
    layer.buf.size.width = g_fb_info.width;
    layer.buf.size.height = g_fb_info.height;
    layer.buf.format = g_fb_info.format;
    layer.buf.stride[0] = g_fb_info.stride;
    layer.buf.phy_addr[0] = (u32)(long)g_ge_out_buffer;

    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0) {
        pr_err("update_layer_config error, %d", ret);
    }

    mpp_fb_ioctl(g_fb, AICFB_WAIT_FOR_VSYNC, &layer);
}

int do_rotate(struct aic_dvp_data *vdata, int index)
{
    struct ge_bitblt blt = {0};
    struct mpp_buf  *src = &blt.src_buf;
    struct mpp_buf  *dst = &blt.dst_buf;
    struct vin_video_buf *binfo = &vdata->binfo;
    int ret = 0;

    // g73 only support YUV400, we only need Y component
    src->format = MPP_FMT_YUV400;
    src->buf_type = MPP_PHY_ADDR;
    src->phy_addr[0] = binfo->planes[index * VID_BUF_PLANE_NUM].buf;
    src->phy_addr[1] = binfo->planes[index * VID_BUF_PLANE_NUM + 1].buf;
    src->stride[0] = vdata->w;
    src->stride[1] = vdata->w;
    src->size.width = vdata->w;
    src->size.height = vdata->h;

    dst->format = g_fb_info.format;
    dst->buf_type = MPP_PHY_ADDR;
    dst->phy_addr[0] = (u32)(long)g_ge_out_buffer;
    dst->stride[0] = g_fb_info.stride;
    dst->size.width = g_fb_info.width;
    dst->size.height = g_fb_info.height;

    blt.ctrl.flags = g_rotation;

    ret =  mpp_ge_bitblt(g_ge_dev, &blt);
    if (ret < 0) {
        pr_err("GE bitblt failed, ret: %d\n", ret);
        return -1;
    }

    ret = mpp_ge_emit(g_ge_dev);
    if (ret < 0) {
        pr_err("GE emit failed\n");
        return -1;
    }

    ret = mpp_ge_sync(g_ge_dev);
    if (ret < 0) {
        pr_err("GE sync failed\n");
        return -1;
    }
    return 0;
}

#else

static int barcode_showvideo_tolcd(struct aic_dvp_data *vdata, int index)
{
    int i;
    struct aicfb_layer_data layer = {0};
    struct vin_video_buf *binfo = &vdata->binfo;

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 1;

    layer.scale_size.width = g_fb_info.width - VID_SCALE_OFFSET * 2;
    layer.scale_size.height = g_fb_info.height - VID_SCALE_OFFSET * 2;
    layer.pos.x = VID_SCALE_OFFSET;
    layer.pos.y = VID_SCALE_OFFSET;

    layer.buf.size.width = vdata->w;
    layer.buf.size.height = vdata->h;
    layer.buf.format = MPP_FMT_NV12;

    layer.buf.buf_type = MPP_PHY_ADDR;

    for (i = 0; i < VID_BUF_PLANE_NUM; i++) {
        layer.buf.stride[i] = layer.buf.size.width;
        layer.buf.phy_addr[i] = binfo->planes[index * VID_BUF_PLANE_NUM + i].buf;
    }

    if (mpp_fb_ioctl(g_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0) {
        pr_err("ioctl() failed!\n");
        return -1;
    }
    return 0;
}
#endif

static void aicfb_lcd_putc(unsigned int x, unsigned int y, char ch)
{
    unsigned long dcache_size, dcache_start;
    int pbytes = g_fb_info.bits_per_pixel / 8;
    int i, row;
    void *line;

    line = (unsigned char *)(g_fb_info.framebuffer + y * g_fb_info.stride + x * pbytes);
    dcache_start = ALIGN_DOWM((unsigned long)line, ARCH_DMA_MINALIGN);

    for (row = 0; row < VIDEO_FONT_HEIGHT; row++) {
        unsigned int idx = (ch - 32) * VIDEO_FONT_HEIGHT + row;
        uint32_t bits = video_fontdata[idx];

        uint16_t *dst = line;

        for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
            *dst++ = (bits & 0x80000000) ? 0xFFFF : 0x0000;
             bits <<= 1;
        }

        line += g_fb_info.stride;
    }

    dcache_size = ALIGN_UP((unsigned long)line - dcache_start,
            ARCH_DMA_MINALIGN);
    aicos_dcache_clean_range((unsigned long *)dcache_start, dcache_size);
}

static int barcode_set_ui_layer_alpha(int val)
{
    int ret = BARCODE_DEMO_SUCCESS;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;
    ret = mpp_fb_ioctl(g_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        pr_err("ioctl() failed! errno: -%d\n", -ret);
    return BARCODE_DEMO_SUCCESS;
}

static int show_barcode_tolcd(int len)
{
    int font_width = 32;
    int i,startx;
    startx = 10;
    int display_len = len > MAX_BUF_LEN ? MAX_BUF_LEN : len;

    //memset(g_fb_info.framebuffer,0, g_fb_info.smem_len);
    //aicos_dcache_clean_range(g_fb_info.framebuffer, g_fb_info.smem_len);

    if (0) {
        for (i = 0; i < display_len; i ++)
            aicfb_lcd_putc(startx + (i * font_width), 60, g_ddata.out_buffer[i]);
    }
    memcpy(g_last_barcode, g_ddata.out_buffer, display_len);

    return BARCODE_DEMO_SUCCESS;
}

static void deinit_screen_fb()
{
    if (g_fb) {
        mpp_fb_close(g_fb);
        g_fb = NULL;
    }

#ifdef DVP_DISPLAY_ROTATION
    if (g_ge_dev) {
        mpp_ge_close(g_ge_dev);
        g_ge_dev = NULL;
    }
    if (g_ge_out_buffer)
        aicos_free(MEM_CMA, g_ge_out_buffer);
#endif
}

static int init_screen_fb()
{
    int ret = 0;
    g_fb = mpp_fb_open();
    if (g_fb < 0) {
        pr_err("mpp_fb_open() failed\n");
        return -1;
    }

    ret = mpp_fb_ioctl(g_fb, AICFB_GET_SCREENINFO, &g_fb_info);
    if (ret < 0) {
        pr_err("ioctl() failed! errno: -%d\n", -ret);
        goto error_out;
    }

    memset(g_fb_info.framebuffer, 0, g_fb_info.smem_len);
    aicos_dcache_clean_range(g_fb_info.framebuffer, g_fb_info.smem_len);

    ret = mpp_fb_ioctl(g_fb, AICFB_POWERON, &g_fb_info);
    if (ret < 0) {
        pr_err("ioctl() failed! errno: -%d\n", -ret);
        goto error_out;
    }

#ifdef DVP_DISPLAY_ROTATION
    if (g_rotation) {
        g_ge_dev = mpp_ge_open();
        if (!g_ge_dev)
            goto error_out;
    }
    g_ge_out_buffer = aicos_malloc_try_cma(g_fb_info.smem_len);
    printf("Rotate %d by GE, g_ge_out_buffer: %p\n", g_rotation * 90, g_ge_out_buffer);
#endif

    pr_info("Screen width: %d, height %d\n", g_fb_info.width, g_fb_info.height);

    return BARCODE_DEMO_SUCCESS;

error_out:
    deinit_screen_fb();
    return -1;
}
#endif

static void barcode_copy_buffer(struct aic_dvp_data *vdata, int index)
{
    struct vin_video_buf *binfo = &vdata->binfo;
    unsigned long buf = binfo->planes[index * VID_BUF_PLANE_NUM].buf;
    int len = binfo->planes[index * VID_BUF_PLANE_NUM].len;

    aicos_memcpy(g_ddata.in_buffer, (void *)buf, len);
}

static void barcode_decode_thread()
{
    int ret;
    int len = 0;
    struct timespec begin, end;
    uint32_t recved;

    g_running = true;

    while (g_running) {
        aicos_event_recv(g_ddata.data_ready_event, DATA_PREPARE_EVENT, &recved, AICOS_WAIT_FOREVER);

        // 1. decode
        gettimespec(&begin);
        ret = Decoding_Image(g_ddata.in_buffer, g_ddata.w, g_ddata.h);

        len =  GetResultLength();
        if (len > 0 && len < MAX_BUF_LEN) {
            rt_pwm_enable(pwm_dev, 1);
            memset(g_ddata.out_buffer, 0, sizeof(g_ddata.out_buffer));
            ret =  GetDecoderResult(g_ddata.out_buffer);

            gettimespec(&end);
            show_timespec_diff("decode", NULL, &begin, &end);


            pr_info("GetDecoderResult %d [%s] \n", len, g_ddata.out_buffer);
#ifdef LPKG_CHERRYUSB_DEVICE_HID_KEYBOARD_TEMPLATE
            usbd_keyboard_putnchar((char *)g_ddata.out_buffer, sizeof(g_ddata.out_buffer));
            usbd_keyboard_putnchar("\r\n", sizeof("\r\n"));
#endif

#ifdef ENABLE_DISPLAY
            if (show_barcode_tolcd(len) == BARCODE_DEMO_SUCCESS) {

            }
#endif
        }

        aicos_mutex_take(g_ddata.lock, AICOS_WAIT_FOREVER);
        g_ddata.dready = false;
        aicos_mutex_give(g_ddata.lock);
        rt_pwm_disable(pwm_dev, 1);
    }
#ifdef ENABLE_DISPLAY
    deinit_screen_fb();
#endif

    if (g_ddata.in_buffer) {
        aicos_free(MEM_CMA, g_ddata.in_buffer);
        g_ddata.in_buffer = NULL;
    }

    ret = 0;
    pr_info("exit with ret = %d \n", ret);
}

static void barcode_shotting_thread()
{
    int index = 0;
    int i;

    if (dvp_request_buf(&g_vdata.binfo) < 0)
        return;

    for (i = 0; i < VID_BUF_NUM; i++) {
        if (dvp_queue_buf(i) < 0)
            return;
    }

    if (dvp_start() < 0)
        return;

    while (true) {
        if (dvp_dequeue_buf(&index) < 0) {
            break;
        }

        if (!g_ddata.dready) {
            barcode_copy_buffer(&g_vdata, index);

            aicos_mutex_take(g_ddata.lock, AICOS_WAIT_FOREVER);
            g_ddata.dready = true;
            aicos_mutex_give(g_ddata.lock);

            aicos_event_send(g_ddata.data_ready_event, DATA_PREPARE_EVENT);
        }
#ifdef ENABLE_DISPLAY
#ifdef DVP_DISPLAY_ROTATION
        if (do_rotate(&g_vdata, index) < 0)
            break;

        video_layer_set();
#else
        barcode_showvideo_tolcd(&g_vdata, index);
#endif
#endif
        dvp_queue_buf(index);
    }

    g_running = false;
    dvp_stop();
    dvp_release_buf(g_vdata.binfo.num_buffers);
    mpp_vin_deinit();

#ifdef DVP_DISPLAY_ROTATION
    if (g_ge_dev) {
        mpp_ge_close(g_ge_dev);
        g_ge_dev = NULL;
    }
    aicos_free(MEM_CMA, g_ge_out_buffer);
#endif
}

static int init_dvp_device()
{
    memset(&g_vdata, 0, sizeof(struct aic_dvp_data));
    g_vdata.dst_fmt = MPP_FMT_NV12;
    g_vdata.frame_cnt = 1;

    if (mpp_vin_init(CAMERA_DEV_NAME))
        return -1;

    if (sensor_get_fmt() < 0)
        goto error_out;

    if (dvp_subdev_set_fmt() < 0)
        goto error_out;

    if (g_vdata.dst_fmt == MPP_FMT_NV16)
        g_vdata.frame_size = g_vdata.w * g_vdata.h * 2;
    else if (g_vdata.dst_fmt == MPP_FMT_NV12)
        g_vdata.frame_size = (g_vdata.w * g_vdata.h * 3) >> 1;
    else if (g_vdata.dst_fmt == MPP_FMT_YUV400)
        g_vdata.frame_size = g_vdata.w * g_vdata.h;

    if (dvp_cfg(g_vdata.w, g_vdata.h, g_vdata.dst_fmt) < 0)
        goto error_out;

    g_vdata.num_buffers = VID_BUF_NUM;

    return BARCODE_DEMO_SUCCESS;

error_out:
    mpp_vin_deinit();

    return -1;
}

static void cmd_barcode_demo(int argc, char **argv)
{
    int ret;
    aicos_thread_t thid = NULL;

    // control beep with pwm
    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    if (pwm_dev == NULL) {
        rt_kprintf("can't find pwm device!\n");
        return;
    }

    rt_pwm_set(pwm_dev, 1, 1000000, 500000);

#ifdef ENABLE_DISPLAY
    ret = init_screen_fb();
    if (ret != BARCODE_DEMO_SUCCESS) {
        pr_info("init_screen_fb failed with %d \n", ret);
        goto error_out;
    }

    ret = barcode_set_ui_layer_alpha(64);
    if (ret != BARCODE_DEMO_SUCCESS) {
        pr_info("set_ui_layer_alpha failed with %d \n", ret);
        goto error_out;
    }

#endif

    ret = init_dvp_device();
    if (ret != BARCODE_DEMO_SUCCESS) {
        pr_info("init_dvp_device failed with %d \n", ret);
        goto error_out;
    }

    g_ddata.w = g_vdata.w;
    g_ddata.h = g_vdata.h;
    g_ddata.dready = false;
    g_running = true;
    g_ddata.in_buffer = aicos_malloc(MEM_CMA, g_ddata.w * g_ddata.w);
    g_ddata.lock = aicos_mutex_create();
    g_ddata.data_ready_event = aicos_event_create();

    ret = Initial_Decoder();
    if (ret != BARCODE_DEMO_SUCCESS) {
        pr_info("Initial_Decoder failed with %d \n", ret);
        goto error_out;
    }

    for (size_t i = 0; i <= 13; i++) {
        Set_Donfig_Decoder(i, 1);
    }

    thid = aicos_thread_create("shoting_thead", 1024 * 8, 0, barcode_shotting_thread, NULL);
    if (thid == NULL) {
        pr_err("Failed to create DVP thread\n");
        goto error_out;
    }

    thid = aicos_thread_create("barcode_thread", 1024 * 32, 0, barcode_decode_thread, NULL);
    if (thid == NULL) {
        pr_err("Failed to create decode thread\n");
        goto error_out;
    }

    return;

error_out:
#ifdef ENABLE_DISPLAY
    deinit_screen_fb();
#endif
    if (g_ddata.in_buffer) {
        aicos_free(MEM_CMA, g_ddata.in_buffer);
        g_ddata.in_buffer = NULL;
    }
    if (g_ddata.lock)
        aicos_mutex_delete(g_ddata.lock);
    if (g_ddata.data_ready_event)
        aicos_event_delete(g_ddata.data_ready_event);
    return;
}

static int barcode_demo()
{
    cmd_barcode_demo(0, NULL);
    return 0;
}
INIT_LATE_APP_EXPORT (barcode_demo);

static void cmd_printf_chipid(int argc, char **argv)
{
#ifdef AIC_USING_SID
    u32 chipid[4];
    drv_efuse_read_chip_id(chipid);
    pr_info("chipid:[%.4x][%.4x][%.4x][%.4x]\n",chipid[0],chipid[1],chipid[2],chipid[3]);

    u8 reserved[64] = {0};
    drv_efuse_read_reserved_1(reserved);
    pr_info("reseved 1 :[");
    for(size_t i = 0; i < 64; i++) {
	printf("0x%02x ", reserved[i]);
    }
    printf("]\n");

    memset(reserved, 0, sizeof(reserved[64]));
    drv_efuse_read_reserved_2(reserved);
    pr_info("reseved 2 :[");
    for(size_t i = 0; i < 64; i++) {
	printf("0x%02x ", reserved[i]);
    }
    printf("]\n");
#endif
}
MSH_CMD_EXPORT_ALIAS(cmd_printf_chipid, printf_chipid, Printf chipid);
