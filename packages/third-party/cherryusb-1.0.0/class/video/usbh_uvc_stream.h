/*
 * Copyright (c) 2022-2025, sakumisu
 *
 */
#ifndef USBH_UVC_STREAM_H
#define USBH_UVC_STREAM_H

#include "usbh_core.h"
#include "usbh_video.h"

#define VIDEO_ISO_INTERVAL (2)
#define VIDEO_ISO_PACKETS  (8 * VIDEO_ISO_INTERVAL)
#define VIDEO_EP_MPS       3072

enum Stage {
    HOST_UVC_TRANSMIT_START = 0, /* Command Block Wrapper */
    HOST_UVC_TRANSMIT_STOP = 1, /* Data Out Phase */
    HOST_UVC_TRANSMIT_FILL = 2,  /* Data In Phase */
    HOST_UVC_TRANSMIT_CHECK = 3, /* Command Status Wrapper */
    HOST_UVC_TRANSMIT_CALLBACK = 4, /* Command Status Wrapper */
    HOST_UVC_TEXT_PRINT = 5,
};

#define UVC_DMA_CHAN_NUM    2
#define CONFIG_UVC_ALIGN_SIZE   8
/* debug */
struct uvc_debug_t {
    /* dma port debug*/
    uint32_t dbg_desc_index;
    uint8_t *dbg_src_addr;
    uint8_t *dbg_dst_addr;
    uint32_t dbg_nbytes;
    uint8_t dbg_urb_index;
    uint8_t dbg_err_flag;
    uint8_t dbg_dma_status;

    /* other */
    char dbg_text[200];
};

struct uvc_port_ops_t {
    void (*video_init)(void);
    void (*video_deinit)(void);
    void (*video_start)(uint8_t urb_index);
    void (*video_stop)(uint8_t urb_index);
    bool (*video_check)(uint8_t urb_index);
    int (*video_lli_fill)(uint8_t urb_index, uint32_t desc_index, uint8_t *src_addr, uint8_t *dst_addr, uint32_t nbytes);
};

struct uvc_port_dma_t {
    struct dma_chan *chan;
    uint32_t desc_index;

    uint8_t *src_addr;
    uint8_t *dest_addr;
    uint32_t data_len;

    uint8_t index_owner;
    bool is_alloced;
    bool is_used;
};

struct uvc_port_cpu_param {
    uint8_t *src_addr;
    uint8_t *dest_addr;
    uint32_t data_len;
    bool is_alloced;
};

struct uvc_port_cpu_t {
    struct uvc_port_cpu_param param[VIDEO_ISO_PACKETS];
    uint32_t desc_index;
    uint8_t index_owner;

    bool is_used;
};

struct uvc_port_priv_t {
    struct uvc_port_dma_t *port_dma[UVC_DMA_CHAN_NUM];
    struct uvc_port_cpu_t *port_cpu[UVC_DMA_CHAN_NUM];
    struct uvc_port_ops_t *uvc_ops;
    struct usbh_video *video_class;
    struct uvc_debug_t debug;

    int (*player_video_update)(void *data);
    int (*player_video_init)(void *data);
    int (*player_video_deinit)(void *data);

    usb_osal_mq_t usbh_uvc_data_mq;
    usb_osal_mq_t usbh_uvc_debug_mq;
    usb_osal_thread_t usbd_uvc_debug_thread;
    uint32_t nbytes;
    uint8_t mode;
    uint8_t cur_dma_index;
    uint8_t cur_cpu_index;
};

// #define ATTR_FAST_RAM_SECTION __attribute__((section(".sram_cma_data")))
#define ATTR_FAST_RAM_SECTION
extern volatile uintptr_t video_complete_count;

/* implemented by user */
void usbh_video_fps_record(void);
void usbh_video_transfer_abort_callback(void);

/* implemented by user */
struct uvc_port_priv_t *get_port_priv(void);
void usbh_video_transmit_init(void);
void usbh_video_transmit_deinit(void);
int usbh_video_transmit_lli_fill(uint8_t urb_index, uint32_t desc_index, uint8_t *src_addr, uint8_t *dst_addr, uint32_t nbytes);
void usbh_video_transmit_start(uint8_t urb_index);
void usbh_video_transmit_stop(uint8_t urb_index, uint8_t err_flag);
bool usbh_video_transmit_isbusy(uint8_t urb_index);
int usbh_video_player_register(int (*player_video_update)(void *data));
int usbh_video_player_init_register(int (*player_video_init)(void *data));
int usbh_video_player_deinit_register(int (*player_video_deinit)(void *data));
void usbh_video_format_list(void);
/* implemented by user */
void usbh_video_frame_callback(struct usbh_videoframe *frame);

int usbh_video_stream_init(uint8_t prio, struct usbh_videoframe *pool, uint8_t pool_num);
int usbh_video_stream_deinit(void);
void usbh_video_stream_start(uint16_t width, uint16_t height, uint8_t format);
void usbh_video_stream_stop(void);

#endif
