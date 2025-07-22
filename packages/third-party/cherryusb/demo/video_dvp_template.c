/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiayu.ou@artinchip.com
 */

#include "usbd_core.h"
#include "usbd_video.h"
#include "cherryusb_nv12.h"
#include "usb_osal.h"

#ifdef AIC_USING_DVP
#include "drv_dvp.h"
#endif
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif
#ifdef AIC_MPP_VIN
#include "mpp_vin.h"
#endif

/* Video Gloabl marco */
#define VID_BUF_NUM             10
#define VID_BUF_PLANE_NUM       2
#define SENSOR_FORMAT           MPP_FMT_NV12

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t packet_buffer[460 * 1024];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t buf[460 * 1024];

/* USB Config Gloabl marco */
#define VIDEO_IN_EP  0x81
#define VIDEO_INT_EP 0x83

#define DESC_bLength                0  /** Length offset */
#define DESC_bDescriptorType        1  /** Descriptor type offset */
#define DESC_DESC_bDescriptorType   2  /** DescriptorType offset */
#define DESC_WIDTH_OFFSET           5  /** witdh offset in video frame */
#define DESC_HEIGH_OFFSET           7  /** heigh offset in video frame */
#define DESC_MINBIT_OFFSET          9  /** min_bit_rate offset in video frame */
#define DESC_MAXBIT_OFFSET          13 /** max_bit_rate offset in video frame */
#define DESC_FRAMESIZE_OFFSET       17 /** max_frame_size offset in video frame */

#ifdef CONFIG_USB_HS
/* select the matching payload size based on the frame size */
// #define MAX_PAYLOAD_SIZE  1024 // for high speed with one transcations every one micro frame
// #define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 1)) | (0x00 << 11))

#define MAX_PAYLOAD_SIZE  2048 // for high speed with two transcations every one micro frame
#define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 2)) | (0x01 << 11))

// #define MAX_PAYLOAD_SIZE  3072 // for high speed with three transcations every one micro frame
// #define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 3)) | (0x02 << 11))

#else
#define MAX_PAYLOAD_SIZE  1020
#define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 1)) | (0x00 << 11))
#endif

#define WIDTH  (unsigned int)(640)
#define HEIGHT (unsigned int)(480)

#define CAM_FPS        (30)
#define INTERVAL       (unsigned long)(10000000 / CAM_FPS)
#define MIN_BIT_RATE   (unsigned long)(WIDTH * HEIGHT * 16 * CAM_FPS) //16 bit
#define MAX_BIT_RATE   (unsigned long)(WIDTH * HEIGHT * 16 * CAM_FPS)
#define MAX_FRAME_SIZE (unsigned long)(WIDTH * HEIGHT * 2)

#define VS_HEADER_SIZ (unsigned int)(VIDEO_SIZEOF_VS_INPUT_HEADER_DESC(1,1) + VIDEO_SIZEOF_VS_FORMAT_UNCOMPRESSED_DESC + VIDEO_SIZEOF_VS_FRAME_UNCOMPRESSED_DESC(1))

#define USB_VIDEO_DESC_SIZ (unsigned long)(9 +                            \
                                           VIDEO_VC_NOEP_DESCRIPTOR_LEN + \
                                           9 +                            \
                                           VS_HEADER_SIZ +                \
                                           6 +                            \
                                           9 +                            \
                                           7)

#define USBD_VID           0x33c3
#define USBD_PID           0x1001
#define USBD_MAX_POWER     500
#define USBD_LANGID_STRING 1033

const uint8_t video_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xef, 0x02, 0x01, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_VIDEO_DESC_SIZ, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    VIDEO_VC_NOEP_DESCRIPTOR_INIT(0x00, VIDEO_INT_EP, 0x0100, VIDEO_VC_TERMINAL_LEN, 48000000, 0x02),
    VIDEO_VS_DESCRIPTOR_INIT(0x01, 0x00, 0x00),
    VIDEO_VS_INPUT_HEADER_DESCRIPTOR_INIT(0x01, VS_HEADER_SIZ, VIDEO_IN_EP, 0x00),
    VIDEO_VS_FORMAT_UNCOMPRESSED_DESCRIPTOR_INIT(0x01, 0x01, VIDEO_GUID_NV12, VIDEO_PIXEL_NV12),
    VIDEO_VS_FRAME_UNCOMPRESSED_DESCRIPTOR_INIT(0x01, WIDTH, HEIGHT, MIN_BIT_RATE, MAX_BIT_RATE, MAX_FRAME_SIZE, DBVAL(INTERVAL), 0x01, DBVAL(INTERVAL)),
    VIDEO_VS_COLOR_MATCHING_DESCRIPTOR_INIT(),
    VIDEO_VS_DESCRIPTOR_INIT(0x01, 0x01, 0x01),
    /* 1.2.2.2 Standard VideoStream Isochronous Video Data Endpoint Descriptor */
    USB_ENDPOINT_DESCRIPTOR_INIT(VIDEO_IN_EP, 0x05, VIDEO_PACKET_SIZE, 0x01),

    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'U', 0x00,                  /* wcChar10 */
    'V', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '0', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
    ///////////////////////////////////////
    /// other speed descriptor
    ///////////////////////////////////////
    0x09,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED,
    WBVAL(USB_VIDEO_DESC_SIZ),
    0x01,
    0x01,
    0x00,
    USB_CONFIG_BUS_POWERED,
    USB_CONFIG_POWER_MA(USBD_MAX_POWER),
#endif
    0x00
};

/* Sensor Global variables */
struct uvc_video {
    int w;
    int h;
    int dst_fmt; /* output format */
    struct mpp_video_fmt src_fmt;
    struct vin_video_buf binfo;
    usb_osal_sem_t stream_sem;
    usb_osal_sem_t tx_sem;
};

static struct uvc_video g_uvc_video = {0};
static usb_osal_thread_t video_thread = NULL;

/* USB Global variables */
static volatile bool g_running = false;
static volatile bool iso_tx_busy = false;

void usbd_video_iso_callback(uint8_t ep, uint32_t nbytes)
{
    USB_LOG_DBG("actual in len:%d\r\n", nbytes);
    iso_tx_busy = false;
    usb_osal_sem_give(g_uvc_video.tx_sem);
}

static struct usbd_endpoint video_in_ep = {
    .ep_cb = usbd_video_iso_callback,
    .ep_addr = VIDEO_IN_EP
};


/* Sensor video function */
static int sensor_get_fmt(void)
{
    int ret = 0;
    uint32_t frame_size;
    struct mpp_video_fmt f = {0};

    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, &f);
    if (ret < 0) {
        USB_LOG_ERR("Failed to get sensor format! err %d\n", ret);
        return -1;
    }

    g_uvc_video.src_fmt = f;
    g_uvc_video.w = g_uvc_video.src_fmt.width;
    g_uvc_video.h = g_uvc_video.src_fmt.height;
    USB_LOG_INFO("Camera Sensor format: w %d h %d, code 0x%x, bus 0x%x, colorspace 0x%x\n",
                 f.width, f.height, f.code, f.bus_type, f.colorspace);

    if (f.bus_type == MEDIA_BUS_RAW8_MONO) {
        USB_LOG_INFO("Set the output format to YUV400\n");
        g_uvc_video.dst_fmt = MPP_FMT_YUV400;
        frame_size = g_uvc_video.w * g_uvc_video.h;
        memset(buf + frame_size, 128, frame_size / 2);
    }
    return 0;
}

static int sensor_set_infmt(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, &g_uvc_video.src_fmt);
    if (ret < 0) {
        USB_LOG_ERR("Failed to set DVP in-format! err %d\n", ret);
        return -1;
    }

    return 0;
}

static int sensor_set_outfmt(int width, int height, int format)
{
    int ret = 0;
    struct dvp_out_fmt f = {0};

    f.width = width;
    f.height = height;
    f.pixelformat = format;
    f.num_planes = VID_BUF_PLANE_NUM;

    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, &f);
    if (ret < 0) {
        USB_LOG_ERR("Failed to set DVP out-format! err -%d\n", -ret);
        return -1;
    }
    return 0;
}

static int sensor_request_buf(struct vin_video_buf *vbuf)
{
    int i = 0;

    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)vbuf) < 0) {
        USB_LOG_ERR("Failed to request buf!\n");
        return -1;
    }

    USB_LOG_INFO("Buf   Plane[0]     size   Plane[1]     size\n");
    for (i = 0; i < vbuf->num_buffers; i++) {
        USB_LOG_INFO("%3d 0x%x %8d 0x%x %8d\n", i,
            vbuf->planes[i * vbuf->num_planes].buf,
            vbuf->planes[i * vbuf->num_planes].len,
            vbuf->planes[i * vbuf->num_planes + 1].buf,
            vbuf->planes[i * vbuf->num_planes + 1].len);
    }

    if (vbuf->num_buffers < 3) {
        pr_err("The number of video buf must >= 3!\n");
        return -1;
    }

    return 0;
}

static int sensor_queue_buf(int index)
{
    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        USB_LOG_ERR("Q failed! Maybe buf state is invalid.\n");
        return -1;
    }

    return 0;
}

static int sensor_dequeue_buf(int *index)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        USB_LOG_ERR("DQ failed! Maybe cannot receive data from Camera. err %d\n", ret);
        return -1;
    }

    return 0;
}

static void sensor_release_buf(int num)
{
    return;
}

static int sensor_start(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        USB_LOG_ERR("Failed to start streaming! err %d\n", ret);
        return -1;
    } else {
        USB_LOG_DBG("Start sensor\n");
    }

    return 0;
}

static int sensor_stop(void)
{
    int ret = 0;

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        USB_LOG_ERR("Failed to start streaming! err -%d\n", -ret);
        return -1;
    }

    return 0;
}


static void usb_set_fmt(int width, int height, int dst_fmt)
{
    uint8_t *p = NULL;
    uint8_t w[2] = {0}, h[2] = {0};
    uint8_t min[4] = {0}, max[4] = {0}, frame[4] = {0};
    unsigned long min_bit_rate, max_bit_rate, frame_size, i;

    min_bit_rate = width * height * 16 * CAM_FPS;
    max_bit_rate = min_bit_rate;
    frame_size = width * height * 2;

    for (i = 0; i < 4; i++) {
        min[i] = (min_bit_rate >> (8 * i)) & 0xff;
        max[i] = (max_bit_rate >> (8 * i)) & 0xff;
        frame[i] = (frame_size >> (8 * i)) & 0xff;
    }
    w[0] = width & 0xff;
    w[1] = (width >> 8) & 0xff;
    h[0] = height & 0xff;
    h[1] = (height >> 8) & 0xff;

    USB_LOG_DBG("usb set fmt:w:%d h:%d\n", width, height);

    p = (uint8_t *)video_descriptor;
    while (p[DESC_bLength] != 0U) {
        if (p[DESC_bDescriptorType] == VIDEO_CS_INTERFACE_DESCRIPTOR_TYPE && \
            p[DESC_DESC_bDescriptorType] == VIDEO_VS_FRAME_UNCOMPRESSED_DESCRIPTOR_SUBTYPE && \
            p[DESC_bLength] >= 0x1a) {
                memcpy(&p[DESC_WIDTH_OFFSET], &w[0], 2);
                memcpy(&p[DESC_HEIGH_OFFSET], &h[0], 2);
                memcpy(&p[DESC_MINBIT_OFFSET], &min[0], 4);
                memcpy(&p[DESC_MAXBIT_OFFSET], &max[0], 4);
                memcpy(&p[DESC_FRAMESIZE_OFFSET], &frame[0], 4);
                break;
        }
        p += p[DESC_bLength];
    }
}

static int video_usb_set(struct uvc_video *uvc_video, int index)
{
    struct vin_video_buf *binfo = &uvc_video->binfo;
    int image_size = 0, i = 0;
    char *addr = NULL;
    uint32_t out_len, remaining_len;
    uint32_t packets, packet_size;
    /* calculate total image length */
    for (i = 0; i < VID_BUF_PLANE_NUM; i++)
        image_size += binfo->planes[index * VID_BUF_PLANE_NUM + i].len;

    addr = (char *)(uintptr_t)binfo->planes[index * VID_BUF_PLANE_NUM].buf;
    USB_LOG_DBG("i:%d, len:%d, buf:0x%x, addr:%p\n",
                i, binfo->planes[index * VID_BUF_PLANE_NUM].len,
                binfo->planes[index * VID_BUF_PLANE_NUM].buf,
                addr);
    aicos_dcache_invalid_range(addr, image_size);

    if (g_uvc_video.dst_fmt == MPP_FMT_YUV400) {
        image_size += (image_size / 2);
        memcpy(buf, addr, binfo->planes[index * VID_BUF_PLANE_NUM].len);
        addr = (char *)(uintptr_t)buf;
    }

    packets = usbd_video_payload_fill((uint8_t *)addr, image_size, packet_buffer, &out_len);
    remaining_len = out_len;

    USB_LOG_DBG("ep:0x%x, packet:%d image_size:%d, total_size:%ld, packets:%ld\n",
                video_in_ep.ep_addr, MAX_PAYLOAD_SIZE, image_size, out_len, packets);

    for (i = 0; i < packets; i++) {
        iso_tx_busy = true;
        packet_size = MIN(MAX_PAYLOAD_SIZE, remaining_len);
        usbd_ep_start_write(video_in_ep.ep_addr, &packet_buffer[out_len - remaining_len], packet_size);

        USB_LOG_DBG("ep:0x%x, [%d]%#lx (%ld) total_size:%ld, remaining_len:%ld\n",
                        video_in_ep.ep_addr, i, (uint32_t)&packet_buffer[out_len - remaining_len],
                        packet_size, out_len, remaining_len);

        if (usb_osal_sem_take(g_uvc_video.tx_sem, 3000)) {
            USB_LOG_WRN("UVC transmission termination.\n");
        }

        if (g_running == 0) {
            USB_LOG_DBG("video transfer interrupt close\n");
            return -1;
        }
        remaining_len -= packet_size;
    }

    return 0;
}

static void usbd_video_pump(void)
{
    int index= 0, i = 0;

    if (sensor_start() < 0) {
        USB_LOG_ERR("sensor start failed, streaming off\n");
        return;
    }

    g_running = true;

    while (g_running) {
        i++;
        if (sensor_dequeue_buf(&index) < 0) {
            USB_LOG_ERR("dequeu buffer failed\n");
            break;
        }

        /* send data by usb */
        if (video_usb_set(&g_uvc_video, index) < 0) {
            break;
        }

        sensor_queue_buf(index);
    }
}

static void usbd_video_thread(void *arg)
{
    int i = 0, cnt = 0;

    while (1) {
        /* wait set_alt 1 to streaming on */
        usb_osal_sem_take(g_uvc_video.stream_sem, USB_OSAL_WAITING_FOREVER);
        USB_LOG_DBG("UVC streaming on [%d]\n", cnt);

        if (cnt)
            mpp_vin_reinit();

        if (sensor_request_buf(&g_uvc_video.binfo) < 0) {
            USB_LOG_ERR("request buf failed\n");
            return;
        }

        for (i = 0; i < g_uvc_video.binfo.num_buffers; i++) {
            if (sensor_queue_buf(i) < 0) {
                USB_LOG_ERR("queue buf failed\n");
                return;
            }
        }

        usbd_video_pump();

        sensor_stop();
        sensor_release_buf(g_uvc_video.binfo.num_buffers);
        cnt++;
    }
}

static int camera_init(void)
{
    memset(&g_uvc_video, 0, sizeof(struct uvc_video));
    g_uvc_video.dst_fmt = SENSOR_FORMAT;

    USB_LOG_INFO("DVP out format: NV12\n");

    /* init mpp_vin to read camera information
     * and use if for usb enumeration.
     */
    if (mpp_vin_init(CAMERA_DEV_NAME)) {
        USB_LOG_ERR("mpp vin init failed\n");
        return -1;
    }

    if (sensor_get_fmt()) {
        USB_LOG_ERR("get sensor fmt failed\n");
        return -1;
    }

    if (sensor_set_infmt()) {
        USB_LOG_ERR("set sensor infmt failed\n");
        return -1;
    }

    if (sensor_set_outfmt(g_uvc_video.w, g_uvc_video.h, g_uvc_video.dst_fmt)) {
        USB_LOG_ERR("set sensor outfmt failed\n");
        return -1;
    }

    usb_set_fmt(g_uvc_video.w, g_uvc_video.h, g_uvc_video.dst_fmt);

    /* stop dvp to reduce power */
    sensor_stop();

    g_uvc_video.stream_sem = usb_osal_sem_create(0);
    if (!g_uvc_video.stream_sem) {
        USB_LOG_ERR("stream_sem create failed\n");
        return -1;
    }

    g_uvc_video.tx_sem = usb_osal_sem_create(0);
    if (!g_uvc_video.tx_sem) {
        USB_LOG_ERR("create dynamic semaphore failed.\n");
        return -1;
    }

    video_thread = usb_osal_thread_create("usbd_video_thread", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbd_video_thread, NULL);
    if (video_thread == NULL) {
        USB_LOG_ERR("video_thread create failed\n");
        return -1;
    }

    USB_LOG_INFO("UVC init success!\n");

    return 0;
}

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
void usbd_comp_video_event_handler(uint8_t event)
#else
void usbd_event_handler(uint8_t event)
#endif
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            g_running = 0;
            iso_tx_busy = false;
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_video_open(uint8_t intf)
{
    USB_LOG_RAW("OPEN\r\n");
    iso_tx_busy = false;
    usb_osal_sem_give(g_uvc_video.stream_sem);
}

void usbd_video_close(uint8_t intf)
{
    USB_LOG_RAW("CLOSE\r\n");
    if (iso_tx_busy == true)
        usb_osal_sem_give(g_uvc_video.tx_sem);
    iso_tx_busy = false;
    g_running = false;
}

struct usbd_interface intf0;
struct usbd_interface intf1;

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
int usbd_comp_video_init(uint8_t *ep_table, void *data)
{
    uint32_t max_frame_size = 0;

    video_in_ep.ep_addr = ep_table[0];

    camera_init();

    max_frame_size = g_uvc_video.w * g_uvc_video.h * 2;

    usbd_add_interface(usbd_video_init_intf(&intf0, INTERVAL, max_frame_size, MAX_PAYLOAD_SIZE));
    usbd_add_interface(usbd_video_init_intf(&intf1, INTERVAL, max_frame_size, MAX_PAYLOAD_SIZE));
    usbd_add_endpoint(&video_in_ep);
    return 0;
}
#endif

int video_deinit(void)
{
#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_deinitialize();
#else
    usbd_comp_func_release(video_descriptor, "video");
#endif

    iso_tx_busy = false;
    g_running = false;

    if (video_thread) {
        usb_osal_thread_delete(video_thread);
        video_thread = NULL;
    }

    if (g_uvc_video.stream_sem) {
        usb_osal_sem_delete(g_uvc_video.stream_sem);
        g_uvc_video.stream_sem = NULL;
    }

    if (g_uvc_video.tx_sem) {
        usb_osal_sem_delete(g_uvc_video.tx_sem);
        g_uvc_video.tx_sem = NULL;
    }

    mpp_vin_deinit();

    return 0;
}

int video_init(void)
{
#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    uint32_t max_frame_size = 0;

    camera_init();

    max_frame_size = g_uvc_video.w * g_uvc_video.h * 2;

    usbd_desc_register(video_descriptor);
    usbd_add_interface(usbd_video_init_intf(&intf0, INTERVAL, max_frame_size, MAX_PAYLOAD_SIZE));
    usbd_add_interface(usbd_video_init_intf(&intf1, INTERVAL, max_frame_size, MAX_PAYLOAD_SIZE));
    usbd_add_endpoint(&video_in_ep);

    usbd_initialize();
#else
    usbd_comp_func_register(video_descriptor,
                            usbd_comp_video_event_handler,
                            usbd_comp_video_init, "video");
#endif
    return 0;
}
USB_INIT_APP_EXPORT(video_init);
