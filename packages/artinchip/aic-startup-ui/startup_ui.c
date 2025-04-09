/*
 * Copyright (c) 2020-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ArtInChip
 */
#include <rtthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <inttypes.h>
#include "aic_osal.h"
#include "mpp_fb.h"
#include "mpp_mem.h"
#include "frame_allocator.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_ge.h"
#include "startup_ui.h"
#include "rtconfig.h"
#include "stddef.h"

#define SCREEN_MAX_WIDTH  1024
#define SCREEN_MAX_HEIGHT 600

#define IMAGE_MAX_WIDTH   500
#define IMAGE_MAX_HEIGHT  160

#define WIDTH_SCALE(x)     ((x) * IMAGE_MAX_WIDTH / SCREEN_MAX_WIDTH)
#define HEIGHT_SCALE(y)    ((y) * IMAGE_MAX_HEIGHT / SCREEN_MAX_HEIGHT)

#define PATH_MAX 1024
#define MALLOC_FAILED   -1
#define FB_OPEN_FAILED  -2
#define MPP_OPS_FAILED  -3
#define GET_SCREEN_ERR  -4

#ifdef LPKG_CHERRYUSB_DEVICE_DISPLAY_TEMPLATE
#define AIC_STARTUP_UI_SHOW_LVGL_LOGO

#ifdef AIC_LVGL_USB_OSD_DEMO
#error "Not support turn on LVGL USB OSD DEMO and Startup UI at the same time"
#endif
#endif

#ifdef AIC_STARTUP_UI_SHOW_LVGL_LOGO
#define IMAGE_EXTENSION ".png"
#define IMAGE_CATALOG   "/data/lvgl_data"
#define IMAGE_PATH_ROOT "/data/lvgl_data/logo.png"
#else
#define IMAGE_EXTENSION ".jpg"
#define IMAGE_CATALOG   "/data"
#define IMAGE_PATH_ROOT "/data/image"
#endif

#if defined (AIC_CHIP_D13X) || defined (AIC_CHIP_D12X)
#define FORMAT_RGB
#endif

static struct startup_ui_fb startup_screen_info = {0};
static char image_path[PATH_MAX];

static int ge_bitblt(struct ge_bitblt *blt)
{
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

#ifdef AIC_PAN_DISPLAY
static u32 fb_copy_index = 0;
static u32 startup_fb_buf_index = 0;
#endif

static struct aicfb_screeninfo *get_screen_info(void)
{
    struct mpp_fb *fb = NULL;
    int ret;

    if (startup_screen_info.startup_fb_data.width)
        return &startup_screen_info.startup_fb_data;

    fb = mpp_fb_open();
    if (!fb)
        return NULL;

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &startup_screen_info.startup_fb_data);
    if (ret) {
        loge("get screen info failed\n");
        return NULL;
    }

    mpp_fb_close(fb);
    return &startup_screen_info.startup_fb_data;
}

static void render_frame(struct mpp_fb* fb, struct mpp_frame* frame,
                         u32 offset_x, u32 offset_y, u32 width, u32 height,
                         u32 layer_id, bool is_first_img)
{
    struct ge_bitblt blt;
    struct aicfb_screeninfo *info = NULL;
    struct aicfb_layer_data layer = {0};
#ifdef AIC_PAN_DISPLAY
    u32 disp_buf_addr;
    u32 fb0_buf_addr;
#endif
    u32 dst_buf_addr = 0;

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
            dst_buf_addr = is_first_img ? fb0_buf_addr : fb0_buf_addr + info->smem_len;
            startup_fb_buf_index = 1;
        } else {
            dst_buf_addr = fb0_buf_addr;
            startup_fb_buf_index = 0;
        }
#else
        dst_buf_addr = layer.buf.phy_addr[0];
#endif
    }
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

    blt.dst_buf.crop.width = blt.src_buf.crop.width * info->width/SCREEN_MAX_WIDTH;
    blt.dst_buf.crop.height = blt.src_buf.crop.height * info->height/SCREEN_MAX_HEIGHT;

    logi("phy_addr: %x, stride: %d", blt.src_buf.phy_addr[0], blt.src_buf.stride[0]);
    logi("width: %d, height: %d, format: %d", blt.src_buf.size.width, blt.src_buf.size.height, blt.src_buf.format);

    ge_bitblt(&blt);

#ifdef AIC_PAN_DISPLAY
    if(!is_first_img) {
        mpp_fb_ioctl(fb, AICFB_PAN_DISPLAY, &startup_fb_buf_index);
        mpp_fb_ioctl(fb, AICFB_WAIT_FOR_VSYNC, NULL);
    }
#endif
}

#ifdef AIC_PAN_DISPLAY
static void framebuffer_copy(int buffer_index)
{
    struct ge_bitblt blt;
    struct aicfb_screeninfo *info = NULL;

    info = get_screen_info();
    if (info == NULL)
        return;

    u32 fb0_buf_addr0 = (u32)(unsigned long)info->framebuffer;
    u32 fb0_buf_addr1 = fb0_buf_addr0 + info->smem_len;

    memset(&blt, 0, sizeof(struct ge_bitblt));

    if (buffer_index == 0) {
        blt.src_buf.phy_addr[0] = fb0_buf_addr0;
    } else {
        blt.src_buf.phy_addr[0] = fb0_buf_addr1;
    }
    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.format = info->format;
    blt.src_buf.stride[0] = info->stride;
    blt.src_buf.size.width = info->width;
    blt.src_buf.size.height = info->height;
    blt.src_buf.crop_en = 0;

    if (buffer_index == 0) {
        blt.dst_buf.phy_addr[0] = fb0_buf_addr1;
    } else {
        blt.dst_buf.phy_addr[0] = fb0_buf_addr0;
    }
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.format = info->format;
    blt.dst_buf.stride[0] = info->stride;
    blt.dst_buf.size.width = info->width;
    blt.dst_buf.size.height = info->height;
    blt.dst_buf.crop_en = 0;

    logi("phy_addr: %x, stride: %d", blt.src_buf.phy_addr[0], blt.src_buf.stride[0]);
    logi("width: %d, height: %d, format: %d", blt.src_buf.size.width, blt.src_buf.size.height, blt.src_buf.format);

    ge_bitblt(&blt);
}
#endif

static int decode_pic_from_path(const char *path, u32 offset_x, u32 offset_y,
                         u32 width, u32 height, u32 layer_id, bool is_first_img)
{
    struct mpp_fb *fb = mpp_fb_open();
    int ret = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        loge("Failed to open file '%s'\n", path);
        return 0;
    }

    fseek(fp, 0L, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    uint8_t *pic = malloc(fsize);
    if (!pic) {
        loge("Failed to allocate memory for picture data,size is :%ld\n",fsize);
        fclose(fp);
        return 0;
    }

    size_t read_len = fread(pic, 1, fsize, fp);
    fclose(fp);
    if (read_len != fsize) {
        loge("Failed to read entire file\n");
        free(pic);
        return 0;
    }

    struct mpp_decoder* dec = NULL;
    struct decode_config config;
    config.bitstream_buffer_size = (fsize + 1023) & (~1023);
    config.extra_frame_num = 0;
    config.packet_count = 1;

#ifdef FORMAT_RGB
    config.pix_fmt = MPP_FMT_RGB_888;
#else
    config.pix_fmt = MPP_FMT_NV12;
#endif

    if (pic[0] == 0xff && pic[1] == 0xd8) {
        dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_MJPEG);
    } else if (pic[1] == 'P' || pic[2] == 'N' || pic[3] == 'G') {
        if (config.pix_fmt == MPP_FMT_RGB_565 || config.pix_fmt == MPP_FMT_BGR_565) {
            loge("PNG decode does nor support RGB565\n");
            free(pic);
            return -1;
        }
        dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_PNG);
    } else {
        loge("Unsupported or invalid image format\n");
        free(pic);
        return 0;
    }

    if (!dec)
        goto out;


    mpp_decoder_init(dec, &config);

    struct mpp_packet packet;
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, fsize);

    memcpy(packet.data, pic, fsize);
    packet.size = fsize;
    packet.flag = PACKET_FLAG_EOS;

    mpp_decoder_put_packet(dec, &packet);

    ret = mpp_decoder_decode(dec);
    if(ret < 0) {
        loge("decode error");
        ret = 0;
        goto out;
    }

    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(dec, &frame);

    render_frame(fb, &frame, offset_x, offset_y, width, height, layer_id, is_first_img);

    mpp_decoder_put_frame(dec, &frame);

out:
    if (dec)
        mpp_decoder_destory(dec);

    free(pic);

    if (fb)
        mpp_fb_close(fb);

    return ret;
}

static void fb_clean(void* ptr, int value, size_t num)
{
    memset(ptr, value, num);
    aicos_dcache_clean_invalid_range(ptr, num);
}

static int count_images(const char *directoryPath)
{
    DIR *dir;
    struct dirent *ent;
    int imageCount = 0;

    dir = opendir(directoryPath);
    if (!dir) {
        LOG_E("Error opening directory");
        return 0;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, IMAGE_EXTENSION)) {
            imageCount++;
        }
    }

    closedir(dir);
    return imageCount;
}

static void construct_imagepath(char *outputBuffer, int image_number)
{
#ifdef AIC_STARTUP_UI_SHOW_LVGL_LOGO
    (void)image_number;
    snprintf(outputBuffer, PATH_MAX, "%s", IMAGE_PATH_ROOT);
#else
    snprintf(outputBuffer, PATH_MAX, "%s%d%s", IMAGE_PATH_ROOT, image_number, IMAGE_EXTENSION);
#endif
}

int startup_ui_show(void)
{
    int i, ret = 0;
    int screen_pos_x = 0;
    int screen_pos_y = 0;
    int image_count = count_images(IMAGE_CATALOG);
    int width_scale = 0;
    int height_scale = 0;
    struct startup_ui_fb *startup_ui = malloc(sizeof(struct startup_ui_fb));

    if (!startup_ui) {
        LOG_E("malloc failed\n");
        return MALLOC_FAILED;
    }

    startup_ui->startup_fb = mpp_fb_open();

    if (!startup_ui->startup_fb) {
        LOG_E("fb open failed\n");
        free(startup_ui);
        return FB_OPEN_FAILED;
    }

    ret = mpp_fb_ioctl(startup_ui->startup_fb, AICFB_GET_SCREENINFO , &startup_ui->startup_fb_data);
    if (ret) {
        LOG_E("mpp_fb_ioctl ops failed\n");
        free(startup_ui);
        return MPP_OPS_FAILED;
    }

    fb_clean(startup_ui->startup_fb_data.framebuffer, 0, startup_ui->startup_fb_data.height * startup_ui->startup_fb_data.stride);

#ifdef AIC_PAN_DISPLAY
    fb_clean(startup_ui->startup_fb_data.framebuffer + startup_ui->startup_fb_data.smem_len, 0, \
    startup_ui->startup_fb_data.height * startup_ui->startup_fb_data.stride);
#endif

    if ((!startup_ui->startup_fb_data.width) || (!startup_ui->startup_fb_data.height)) {
        LOG_E("Get scrren error\n");
        free(startup_ui);
        return GET_SCREEN_ERR;
    }

    width_scale = WIDTH_SCALE(startup_ui->startup_fb_data.width);
    height_scale = HEIGHT_SCALE(startup_ui->startup_fb_data.height);

    screen_pos_x = (startup_ui->startup_fb_data.width - width_scale) / 2;
    screen_pos_y = (startup_ui->startup_fb_data.height - height_scale) / 2;

    for(i = 0; i < image_count; i++) {
        construct_imagepath(image_path, i);

        if(i == 0) {
            decode_pic_from_path(image_path, screen_pos_x, screen_pos_y, 0, 0, AICFB_LAYER_TYPE_UI, true);
            mpp_fb_ioctl(startup_ui->startup_fb, AICFB_POWERON, 0);
        } else {
            aic_mdelay(80);
            decode_pic_from_path(image_path, screen_pos_x, screen_pos_y, 0, 0, AICFB_LAYER_TYPE_UI, false);
        }
    }
#ifdef AIC_PAN_DISPLAY
    if ((image_count % 2) == 0) {
        /* frame buffer copy */
        framebuffer_copy(!fb_copy_index);
        /* switch to frame buffer 0 */
        mpp_fb_ioctl(startup_ui->startup_fb, AICFB_PAN_DISPLAY, &fb_copy_index);
        mpp_fb_ioctl(startup_ui->startup_fb, AICFB_WAIT_FOR_VSYNC, NULL);
    }
#endif
    free(startup_ui);

    return 0;
}

INIT_LATE_APP_EXPORT(startup_ui_show);
