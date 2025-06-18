/*_------------------------------------------------------------------------------*/

/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <aic_core.h>

#include <finsh.h>

#include "artinchip_fb.h"
#include "mpp_fb.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "packet_allocator.h"
#include "usbh_uvc_stream.h"


#define HEIGHT  480
#define WIDTH   640

struct usbh_uvc_player_t {
    /* AIC Disp parse*/
    struct aicfb_video_layer *vlayer;
    struct mpp_fb *mpp_fb;
    struct mpp_decoder *mpp_dec;
    struct usbh_videoframe *cur_frame;
    usb_osal_thread_t record_tid;
    usb_osal_sem_t record_sem;
    int (*video_update_callback)(void *data);
    int buf_index;

    /* USB parse*/
    uint8_t *frame_buffer1;
    uint8_t *frame_buffer2;

    /* stream data */
    int video_fd;
    int frame_cnt;
    int max_frame_cnt;

    int width;
    int height;
    int image_size;

    uint8_t frame;
    uint8_t format_index;
    char format[32];

    bool stream_on;
    bool stream_record;
}g_usbh_uvc_player;

#define UVC_DATA_PATH "/sdcard/uvc_data.raw"
static struct usbh_videoframe frame_pool[2];
struct mpp_frame g_frame[2];
struct mpp_packet packet;
int cur_frame_id = 0;
int last_frame_id = 0;

void usbh_video_fps_init(void);

/* Global macro and variables */

#ifndef LOG_TAG
#define LOG_TAG "de_test"
#endif

#define ERR(fmt, ...) aic_log(AIC_LOG_ERR, "E", fmt, ##__VA_ARGS__)
#define DBG(fmt, ...) aic_log(AIC_LOG_INFO, "I", fmt, ##__VA_ARGS__)

#define ALIGN_8B(x) (((x) + (7)) & ~(7))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_64B(x) (((x) + (63)) & ~(63))
#define ALIGN_128B(x) (((x) + (127)) & ~(127))
#define ALIGN_1024B(x) (((x) + (1023)) & ~(1023))

#define AICFB_VID_BUF_NUM   2

struct video_data_format {
    enum mpp_pixel_format format;
    char f_str[16];
    int plane_num;
    int y_shift;
    int u_shift;
    int v_shift;
};

static struct video_data_format g_vformat[] = {
    {MPP_FMT_YUV420P, "yuv420p", 3, 0, 2, 2},
    {MPP_FMT_YUV422P, "yuv422p", 3, 0, 1, 1},

    {MPP_FMT_NV12, "nv12", 2, 0, 1, 0},
    {MPP_FMT_NV21, "nv21", 2, 0, 1, 0},
    {MPP_FMT_NV16, "nv16", 2, 0, 0, 0},
    {MPP_FMT_NV61, "nv61", 2, 0, 0, 0},

    {MPP_FMT_YUYV, "yuyv", 1, 1, 0, 0},
    {MPP_FMT_YVYU, "yvyu", 1, 1, 0, 0},
    {MPP_FMT_UYVY, "uyvy", 1, 1, 0, 0},
    {MPP_FMT_VYUY, "vyuy", 1, 1, 0, 0},

    {MPP_FMT_YUV400, "yuv400", 1, 0, 0, 0},

    {MPP_FMT_YUV420_128x16_TILE, "yuv420_128x16", 2, 0, 1, 0},
    {MPP_FMT_YUV420_64x32_TILE,  "yuv420_64x32",  2, 0, 1, 0},
    {MPP_FMT_YUV422_128x16_TILE, "yuv422_128x16", 2, 0, 0, 0},
    {MPP_FMT_YUV422_64x32_TILE,  "yuv422_64x32",  2, 0, 0, 0},

    {MPP_FMT_YUV420P, "mjpeg", 3, 0, 2, 2},
    {MPP_FMT_MAX, "", 0, 0, 0, 0}
};

struct video_plane {
    void *buf;
    unsigned long phy_addr;
    int len;
};
struct video_buf {
    struct video_plane y;
    struct video_plane u;
    struct video_plane v;
};
struct aicfb_video_layer {
    int w;
    int h;
    int s;
    struct video_data_format *f;
    struct video_buf vbuf[AICFB_VID_BUF_NUM];
};

static struct aicfb_video_layer g_vlayer = {0};
static struct aicfb_layer_data g_layer = {0};
static struct mpp_fb *g_mpp_fb = NULL;

static volatile uintptr_t g_uvc_fps = 0;
static usb_osal_thread_t video_fps_tid;

#define VIDEO_DEBUG

static struct usbh_uvc_player_t *get_uvc_player(void)
{
    return &g_usbh_uvc_player;
}

ATTR_FAST_RAM_SECTION void usbh_video_transfer_abort_callback(void)
{
    g_uvc_fps = 0;
}

ATTR_FAST_RAM_SECTION void usbh_video_fps_record(void)
{
    static uint32_t time_last;
    static uint16_t fps_cnt;

    fps_cnt++;
    if (fps_cnt >= 10) {
        uint32_t time = rt_tick_get();
        g_uvc_fps = (1000 * 10) / (uint32_t)(time - time_last);
        time_last = time;
        fps_cnt = 0;
    }
}

static void usbh_vide_fps_thread(void *argument)
{
    while (1) {
        usb_osal_msleep(5000);
        USB_LOG_INFO("fps:%u\n", (unsigned int)g_uvc_fps);
        USB_LOG_INFO("vc:%u\n", (unsigned int)video_complete_count);
    }
}

void usbh_video_fps_init(void)
{
    video_fps_tid = usb_osal_thread_create("usbh_video", 1024 * 4, 10, usbh_vide_fps_thread, NULL);
}

void usbh_video_fps_deinit(void)
{
    if (video_fps_tid)
        usb_osal_thread_delete(video_fps_tid);
}

/* Functions */

static inline bool format_invalid(enum mpp_pixel_format format)
{
    if (format == MPP_FMT_MAX)
        return true;

    return false;
}

static inline bool is_packed_format(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUYV:
    case MPP_FMT_YVYU:
    case MPP_FMT_UYVY:
    case MPP_FMT_VYUY:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_tile_format(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420_128x16_TILE:
    case MPP_FMT_YUV420_64x32_TILE:
    case MPP_FMT_YUV422_128x16_TILE:
    case MPP_FMT_YUV422_64x32_TILE:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_plane_format(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420P:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV400:
    case MPP_FMT_NV12:
    case MPP_FMT_NV21:
    case MPP_FMT_NV16:
    case MPP_FMT_NV61:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_2plane(enum mpp_pixel_format format)
{
	switch (format) {
	case MPP_FMT_NV12:
	case MPP_FMT_NV21:
	case MPP_FMT_NV16:
	case MPP_FMT_NV61:
		return true;
	default:
		break;
	}
	return false;
}

static inline bool is_tile_16_align(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420_128x16_TILE:
    case MPP_FMT_YUV422_128x16_TILE:
        return true;
    default:
        break;
    }
    return false;
}

static inline bool is_tile_32_align(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_YUV420_64x32_TILE:
    case MPP_FMT_YUV422_64x32_TILE:
        return true;
    default:
        break;
    }
    return false;
}

static int aicfb_open(void)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    if (g_mpp_fb)
        return 0;

    g_mpp_fb = mpp_fb_open();
    if (!g_mpp_fb) {
        ERR("open mpp fb failed\n");
        return -1;
    }
    uvc_player->mpp_fb = g_mpp_fb;
    mpp_fb_ioctl(g_mpp_fb, AICFB_POWERON, 0);

    return 0;
}

static int vidbuf_request_one(struct video_plane *plane, int len)
{
    plane->len = len;

    plane->buf = usb_osal_malloc_align(MEM_CMA, len + 1023, 1024);
    if (!plane->buf) {
        ERR("memory alloc failed, need %d bytes", len);
        return -1;
    }

    plane->phy_addr = ALIGN_1024B((unsigned long)plane->buf);
    USB_LOG_DBG("Alloc vidbuf 0x%lx phy_addr 0x%lx len %d\n", (long)plane->buf, (long)plane->phy_addr, len);

    return 0;
}

static int vidbuf_request(struct aicfb_video_layer *vlayer)
{
    int i, j;
    int y_frame = vlayer->w * vlayer->h;

    /* Prepare two group buffer for video player,
       and each group has three planes: y, u, v. */
    for (i = 0; i < AICFB_VID_BUF_NUM; i++) {
        struct video_plane *p = (struct video_plane *)&vlayer->vbuf[i];
        int *shift = &vlayer->f->y_shift;

        if (is_packed_format(vlayer->f->format)) {
            vidbuf_request_one(p, y_frame << shift[0]);
            frame_pool[i].frame_buf = (uint8_t *)p->phy_addr;
            frame_pool[i].frame_bufsize = p->len;
        }

        if (is_plane_format(vlayer->f->format)) {
            vidbuf_request_one(p, y_frame * 3 / 2);
            frame_pool[i].frame_buf = (uint8_t *)p->phy_addr;
            frame_pool[i].frame_bufsize = p->len;
            for (j = 0, p++; j < vlayer->f->plane_num - 1; j++, p++) {
                p->phy_addr = ((uintptr_t)frame_pool[i].frame_buf + (y_frame >> shift[j]));
            }
        }

        USB_LOG_DBG("frame_pool[%d]:%#lx size :%d\n", i,
            (long)frame_pool[i].frame_buf, frame_pool[i].frame_bufsize);
    }
    return 0;
}

void vidbuf_release(struct aicfb_video_layer *vlayer)
{
    int i;
    struct video_plane *p = NULL;

    for (i = 0; i < AICFB_VID_BUF_NUM; i++) {
        p = (struct video_plane *)&vlayer->vbuf[i];
        if (p->buf == NULL)
            continue;
        USB_LOG_DBG("free p 0x%lx\n", (long)p);
        usb_osal_free_align(MEM_CMA, p->buf);
        USB_LOG_DBG("free buf 0x%lx\n", (long)p->buf);
    }
}

static int set_ui_layer_alpha(int val)
{
    int ret = 0;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.mode     = AICFB_GLOBAL_ALPHA_MODE;
    alpha.enable   = 1;
    alpha.value    = val;
    ret = mpp_fb_ioctl(g_mpp_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        ERR("ioctl update alpha config failed!\n");

    return ret;
}

static int video_layer_set_yuv(struct aicfb_video_layer *vlayer, int index)
{
    struct video_buf *vbuf = &vlayer->vbuf[index];

    memset(&g_layer, 0, sizeof(g_layer));
    g_layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    g_layer.enable = 1;
    g_layer.pos.x = 0;
    g_layer.pos.y = 0;
    g_layer.scale_size.width = vlayer->w;
    g_layer.scale_size.height = vlayer->h;
    g_layer.buf.size.width = vlayer->w;
    g_layer.buf.size.height = vlayer->h;
    g_layer.buf.format = vlayer->f->format;
    g_layer.buf.buf_type = MPP_PHY_ADDR;
    g_layer.buf.phy_addr[0] = vbuf->y.phy_addr;
    g_layer.buf.phy_addr[1] = vbuf->u.phy_addr;
    g_layer.buf.phy_addr[2] = vbuf->v.phy_addr;

    if (is_packed_format(vlayer->f->format))
        g_layer.buf.stride[0] = vlayer->w << 1;

    if (is_tile_format(vlayer->f->format)) {
        g_layer.buf.stride[0] = vlayer->s;
        g_layer.buf.stride[1] = vlayer->s;
    }

    if (is_plane_format(vlayer->f->format)) {
        g_layer.buf.stride[0] = vlayer->w;
		g_layer.buf.stride[1] = is_2plane(vlayer->f->format) ?
					vlayer->w : vlayer->w >> 1;
        g_layer.buf.stride[2] = vlayer->w >> 1;
    }

    if (mpp_fb_ioctl(g_mpp_fb, AICFB_UPDATE_LAYER_CONFIG, &g_layer) < 0) {
        ERR("ioctl update layer config failed!\n");
        return -1;
    }

    return 0;
}

void video_layer_disable(void)
{
    struct aicfb_layer_data layer = {0};

    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    layer.enable = 0;

    if (mpp_fb_ioctl(g_mpp_fb, AICFB_UPDATE_LAYER_CONFIG, &layer) < 0)
        ERR("ioctl update layer config failed!\n");
}

static void vidbuf_cpu_begin(struct video_buf *vbuf)
{

}

static void vidbuf_cpu_end(struct video_buf *vbuf)
{
    int i;
    struct video_plane *p = (struct video_plane *)vbuf;

    for (i = 0; i < g_vlayer.f->plane_num; i++, p++)
        aicos_dcache_clean_invalid_range((unsigned long *)p->phy_addr, p->len);
}

static int format_parse(char *str)
{
    int i;

    for (i = 0; g_vformat[i].format != MPP_FMT_MAX; i++) {
        if (strncmp(g_vformat[i].f_str, str, strlen(str)) == 0)
            return i;
    }

    ERR("Invalid format: %s\n", str);
    return -1;
}

static int vidbuf_write(struct usbh_videoframe *frame)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    int fd = uvc_player->video_fd;

    if (uvc_player->frame_cnt > uvc_player->max_frame_cnt)
        return 0;

    write(fd, (void *)frame->frame_buf, frame->frame_bufsize);
    fsync(fd);

    uvc_player->frame_cnt++;

    if (uvc_player->frame_cnt == uvc_player->max_frame_cnt)
        close(fd);


    return 0;
}

static void uvc_record_thread(void *arg)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    while (1) {
        if (usb_osal_sem_take(uvc_player->record_sem, AICOS_WAIT_FOREVER))
            break;

        vidbuf_write(uvc_player->cur_frame);
    }
}

static void uvc_record_start(int max_frame_cnt)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    if (uvc_player->max_frame_cnt != 0 &&
        uvc_player->frame_cnt < uvc_player->max_frame_cnt)
        return;

    uvc_player->stream_record = true;
    uvc_player->frame_cnt = 0;
    uvc_player->max_frame_cnt = max_frame_cnt;
    USB_LOG_INFO("UVC start recoring.(max frame cnt: %d)\n", uvc_player->max_frame_cnt);

    uvc_player->video_fd = open(UVC_DATA_PATH, O_RDWR);
    if (uvc_player->video_fd < 0) {
        creat(UVC_DATA_PATH, O_RDWR);
        uvc_player->video_fd = open(UVC_DATA_PATH, O_RDWR);
    }

    if (uvc_player->stream_record == true && uvc_player->record_sem == NULL
        && uvc_player->record_tid == NULL) {
        uvc_player->record_sem = usb_osal_sem_create(0);
        uvc_player->record_tid = usb_osal_thread_create("uvc_record", 1024 * 6, 18, uvc_record_thread, NULL);
    }
}

static void uvc_player_info(struct usbh_uvc_player_t *uvc_player)
{
    int info_width = 40;

    for (int i = 0; i < info_width / 2; i++) {
        printf("-");
    }

    printf(" UVC player parsemeter ");

    for (int i = 0; i < info_width / 2; i++) {
        printf("-");
    }

    printf("\n");
    printf("|     UVC player status -> %s \n", uvc_player->stream_on ? "on" : "off");
    printf("|     frame buffer1 : %#lx\n", (long)uvc_player->frame_buffer1);
    printf("|     frame buffer2 : %#lx\n", (long)uvc_player->frame_buffer2);
    printf("|     Format\t |\tWidth\t|\tHeight\t|\tFrame\t\n");
    printf("\t%s\t", uvc_player->format);
    printf("\t %d\t", uvc_player->width);
    printf("\t %d\t", uvc_player->height);
    printf("\t %u\t", (unsigned int)g_uvc_fps);
    printf("\n");
    for (int i = 0; i < (info_width + 23); i++) {
        printf("-");
    }
    printf("\n");
}

static void usb_video_soft_reset(void)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    memset(uvc_player, 0, sizeof(struct usbh_uvc_player_t));
    memset(&g_vlayer, 0, sizeof(struct aicfb_video_layer));

    uvc_player->vlayer = &g_vlayer;
    uvc_player->mpp_fb = g_mpp_fb;

    //set default parse
    uvc_player->height = HEIGHT;
    uvc_player->width = WIDTH;
    uvc_player->frame = 15;
    strcpy(uvc_player->format, "yuyv");
    uvc_player->format_index = USBH_VIDEO_FORMAT_UNCOMPRESSED_YUY2;
}

static int usb_video_src_release(void)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    switch (UVC_GET_FORMAT(uvc_player->format_index)) {
        case USBH_VIDEO_FORMAT_UNCOMPRESSED:
            video_layer_disable();
            if (uvc_player->vlayer)
                vidbuf_release(uvc_player->vlayer);
            break;
        case USBH_VIDEO_FORMAT_MJPEG:
            for (int i = 0; i < 2; i++) {
                usb_osal_free_align(MEM_CMA, frame_pool[i].frame_buf);
            }
            break;
        default:
            break;
    }

    if (uvc_player->record_tid)
        usb_osal_thread_delete(uvc_player->record_tid);

    if (uvc_player->record_sem)
        usb_osal_sem_delete(uvc_player->record_sem);

    usb_video_soft_reset();

    usbh_video_fps_deinit();

    return 0;
}

static int parse_options(struct usbh_uvc_player_t *uvc_player, int cnt, char**options)
{
    int argc = cnt;
    char **argv = options;
    int opt;

    if (!uvc_player || argc == 0 || !argv) {
        USB_LOG_ERR("para error !!!");
        return -1;
    }

    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "w:h:f:r:isl");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'h':
            uvc_player->height = strtoul(optarg, NULL, 10);
            break;
        case 'w':
            uvc_player->width = strtoul(optarg, NULL, 10);
            break;
        case 'f':
            if (strcmp(optarg, "nv12") == 0) {
                uvc_player->format_index = USBH_VIDEO_FORMAT_UNCOMPRESSED_NV12;
                strcpy(uvc_player->format, "nv12");
            }
            if (strcmp(optarg, "yuy2") == 0 || strcmp(optarg, "yuyv") == 0 ) {
                uvc_player->format_index = USBH_VIDEO_FORMAT_UNCOMPRESSED_YUY2;
                strcpy(uvc_player->format, "yuyv");
            }
            if (strcmp(optarg, "mjpeg") == 0) {
                uvc_player->format_index = USBH_VIDEO_FORMAT_MJPEG;
                strcpy(uvc_player->format, "mjpeg");
            }
            break;
        case 's':
            if (uvc_player->stream_on == false)
                return -1;
            uvc_player->stream_on = false;
            usbh_video_stream_stop();
            rt_thread_mdelay(50);
            usbh_video_stream_deinit();
            rt_thread_mdelay(50);
            usb_video_src_release();
            return -1;
        case 'r':
            uvc_record_start(strtoul(optarg, NULL, 10));
            break;
        case 'l':
            usbh_video_format_list();
            return -1;
        case 'i':
            uvc_player_info(uvc_player);
            return -1;
        default:
            return -1;
        }
    }

    return 0;
}

static int usbh_disp_parse_set(struct usbh_uvc_player_t *uvc_player)
{
    int index;

    g_vlayer.w = uvc_player->width;
    g_vlayer.h = uvc_player->height;
    index = format_parse(uvc_player->format);
    if (index < 0)
        return 0;

    g_vlayer.f = &g_vformat[index];

    return 0;
}

static void uvc_yuv_init(void)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    vidbuf_request(uvc_player->vlayer);
    video_layer_set_yuv(uvc_player->vlayer, uvc_player->buf_index);
}

static int uvc_yuv_update_callback(void *data)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    struct video_buf *vbuf;

    vbuf = &g_vlayer.vbuf[uvc_player->buf_index];

    vidbuf_cpu_begin(vbuf);

    vidbuf_cpu_end(vbuf);

    video_layer_set_yuv(&g_vlayer, uvc_player->buf_index);

    uvc_player->buf_index = !uvc_player->buf_index;

    return 0;
}

static int pkt_allocator_init(struct packet_allocator *p)
{
    return 0;
}

static int pkt_allocator_deinit(struct packet_allocator *p)
{
    return 0;
}

static int pkt_alloc(struct packet_allocator *p,struct mpp_packet *packet)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    packet->data = uvc_player->cur_frame->frame_buf;
    packet->size = uvc_player->cur_frame->frame_bufsize;
    packet->flag = PACKET_FLAG_EOS;
    return 0;
}

static int pkt_free(struct packet_allocator *p,struct mpp_packet *packet)
{
    return 0;
}

static struct pkt_alloc_ops pkt_ops = {
    .allocator_init = pkt_allocator_init,       // allocator_init
    .allocator_deinit = pkt_allocator_deinit,   // allocator_deinit
    .alloc = pkt_alloc,                         // alloc
    .free = pkt_free,                           // free
};

static struct packet_allocator pkt_allocator = {
    .ops = &pkt_ops,
};

static int aic_uvc_player_update_callback(void *data)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    uvc_player->cur_frame = (struct usbh_videoframe *)data;
    uvc_player->video_update_callback(data);

    if (uvc_player->stream_record == true || uvc_player->record_sem)
        usb_osal_sem_give(uvc_player->record_sem);

    return 0;
}

static int aic_uvc_player_deinit_callback(void *data)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();

    if (uvc_player->stream_on == false)
        return -1;

    uvc_player->stream_on = false;
    usbh_video_stream_stop();
    rt_thread_mdelay(50);
    usbh_video_stream_deinit();
    rt_thread_mdelay(50);
    usb_video_src_release();

    return 0;
}

static int usb_disp_ve_update_buf_size(struct mpp_frame *frame)
{
    int w = frame->buf.size.width;
    int h = frame->buf.size.height;

    if ((w > h && frame->buf.crop.width < frame->buf.crop.height) ||
        (w < h && frame->buf.crop.width > frame->buf.crop.height)) {
        frame->buf.size.width = h;
        frame->buf.size.height = w;
    }
    return 0;
}

static int usb_disp_decoder_init(void)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    struct decode_config config;
    uint32_t usb_ve_rotate = 0;

    if (uvc_player->format_index & USBH_VIDEO_FORMAT_MJPEG)
        uvc_player->mpp_dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_MJPEG);
    else if (uvc_player->format_index & USBH_VIDEO_FORMAT_H264)
        uvc_player->mpp_dec = mpp_decoder_create(MPP_CODEC_VIDEO_DECODER_H264);
    else
        uvc_player->mpp_dec = NULL;
    if (!uvc_player->mpp_dec) {
        USB_LOG_ERR("mpp_dec_create failed.\n");
        return -1;
    }

    mpp_decoder_control(uvc_player->mpp_dec, MPP_DEC_INIT_CMD_SET_EXT_PACKET_ALLOCATOR, &pkt_allocator);
    config.bitstream_buffer_size = ALIGN_UP(uvc_player->image_size, 1024);
    if (uvc_player->format_index & USBH_VIDEO_FORMAT_MJPEG)
        config.extra_frame_num = 1;
    else
        config.extra_frame_num = 0;
    config.packet_count = 1;
    config.pix_fmt = MPP_FMT_YUV420P;
    mpp_decoder_init(uvc_player->mpp_dec, &config);

    if (uvc_player->format_index & USBH_VIDEO_FORMAT_MJPEG) {
        mpp_decoder_control(uvc_player->mpp_dec, MPP_DEC_INIT_CMD_SET_ROT_FLIP_FLAG, &usb_ve_rotate);
    }

    return 0;
}

static int usb_disp_render_frame(int cur_frame_id, int last_frame_id)
{
    int ret = 0;

    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    struct mpp_frame *pframe = &g_frame[cur_frame_id];
    struct mpp_frame *last_pframe = &g_frame[last_frame_id];

    ret = mpp_decoder_get_frame(uvc_player->mpp_dec, pframe);
    if (ret) {
        USB_LOG_ERR("mpp_decoder_get_frame error = %d.\n", ret);
        return ret;
    } else {
        USB_LOG_DBG("mpp_decoder_get_frame succeed.\n");
    }

    if (uvc_player->format_index != USBH_VIDEO_FORMAT_MJPEG) {
        mpp_decoder_put_frame(uvc_player->mpp_dec, pframe);
        memset(pframe, 0, sizeof(struct mpp_frame));
    } else {
        //Update ve output buffer size
        usb_disp_ve_update_buf_size(pframe);
        memcpy(&g_layer.buf, &pframe->buf, sizeof(struct mpp_buf));
    }

    mpp_fb_ioctl(uvc_player->mpp_fb, AICFB_UPDATE_LAYER_CONFIG, &g_layer);

    if (last_pframe->buf.phy_addr[0]) {
        mpp_decoder_put_frame(uvc_player->mpp_dec, last_pframe);
        memset(last_pframe, 0, sizeof(struct mpp_frame));
    }

    return 0;
}

static void uvc_decode_init(void)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    uint32_t max_frame_size = ALIGN_UP((uvc_player->width * uvc_player->height * 3) / 2, 16) + 16;

    memset(g_frame, 0, sizeof(g_frame));

    for (int i = 0; i < 2; i++) {
        if ((uvc_player->format_index == USBH_VIDEO_FORMAT_H264))
            frame_pool[i].frame_buf = usb_osal_malloc_align(MEM_CMA, max_frame_size, 1024);
        else
            frame_pool[i].frame_buf = usb_osal_malloc_align(MEM_CMA, max_frame_size, CACHE_LINE_SIZE);
        frame_pool[i].frame_bufsize = max_frame_size;
    }

    if (video_layer_set_yuv(uvc_player->vlayer, 0))
        return;

    uvc_player->image_size = max_frame_size;
    if (usb_disp_decoder_init())
        return;
}

static int uvc_decode_update_callback(void *data)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    int ret = 0;
    int buff_len= ALIGN_UP((uvc_player->width * uvc_player->height * 3) / 2, 16) + 16;

    memset(&packet, 0, sizeof(struct mpp_packet));
    ret = mpp_decoder_get_packet(uvc_player->mpp_dec, &packet, buff_len);
    if (ret) {
        USB_LOG_ERR("mpp_decoder_get_packet error = %d.\n", ret);
        return -1;
    } else {
        USB_LOG_DBG("mpp_decoder_get_packet succeed.\n");
    }

    ret = mpp_decoder_put_packet(uvc_player->mpp_dec, &packet);
    if (ret) {
        USB_LOG_ERR("mpp_decoder_put_packet error = %d.\n", ret);
        return -1;
    } else {
        USB_LOG_DBG("mpp_decoder_put_packet succeed.\n");
    }

    ret = mpp_decoder_decode(uvc_player->mpp_dec);
    if (ret == DEC_NO_EMPTY_FRAME) {
        USB_LOG_DBG("mpp_decoder_decode error, no empty frame.\n");
    } else if (ret) {
        USB_LOG_ERR("mpp_decoder_decode error = %d.\n", ret);
        if (uvc_player->stream_record == true || uvc_player->record_sem)
            usb_osal_sem_give(uvc_player->record_sem);
        return -1;
    } else {
        USB_LOG_DBG("mpp_decoder_decode succeed.\n");
    }

    if (cur_frame_id == 0)
        last_frame_id = 1;
    else
        last_frame_id = 0;

    ret = usb_disp_render_frame(cur_frame_id, last_frame_id);
    if (ret) {
        return -1;
    }

    if (cur_frame_id == 0)
        cur_frame_id = 1;
    else
        cur_frame_id = 0;

    return 0;
}

static void test_usbh_video(int argc, char **argv)
{
    struct usbh_uvc_player_t *uvc_player = get_uvc_player();
    int ret = -1;

    if (uvc_player->stream_on == false) {
        usb_video_soft_reset();
    }

    ret = parse_options(uvc_player, argc, argv);
    if (ret < 0)
        return;

    if (uvc_player->stream_on == true) {
        USB_LOG_RAW("UVC player has been started.\n");
        return;
    }

    ret = usbh_disp_parse_set(uvc_player);
    if (ret < 0)
        USB_LOG_ERR("Payer don't support this format.\n");

    if (aicfb_open() < 0)
        USB_LOG_ERR("aic fb open fail.\n");

    set_ui_layer_alpha(0);

    switch (UVC_GET_FORMAT(uvc_player->format_index)) {
        case USBH_VIDEO_FORMAT_UNCOMPRESSED:
            uvc_yuv_init();
            uvc_player->video_update_callback = uvc_yuv_update_callback;
            break;
        case USBH_VIDEO_FORMAT_MJPEG:
            uvc_decode_init();
            uvc_player->video_update_callback = uvc_decode_update_callback;
            break;
        default:
            USB_LOG_ERR("uvc player playback format abnormality.\n");
            return;
    }

    if (uvc_player->video_update_callback == NULL) {
        USB_LOG_ERR("Failed to start uvc player.\n");
        return;
    }

    uvc_player->frame_buffer1 = frame_pool[0].frame_buf;
    uvc_player->frame_buffer2 = frame_pool[1].frame_buf;
    usbh_video_stream_init(5, frame_pool, AICFB_VID_BUF_NUM);

    usbh_video_player_register(aic_uvc_player_update_callback);
    usbh_video_player_deinit_register(aic_uvc_player_deinit_callback);

    usbh_video_fps_init();

    usbh_video_stream_start(uvc_player->width, uvc_player->height,
                                uvc_player->format_index);

    uvc_player->stream_on = true;

}

MSH_CMD_EXPORT_ALIAS(test_usbh_video, test_usbh_video, usb video layer test);
#endif /* RT_USING_FINSH */
