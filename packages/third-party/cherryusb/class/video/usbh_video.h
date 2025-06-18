/*
 * Copyright (c) 2022-2025, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_VIDEO_H
#define USBH_VIDEO_H

#include "usb_video.h"

#define USBH_VIDEO_FORMAT_UNCOMPRESSED BIT(0)
#define USBH_VIDEO_FORMAT_MJPEG        BIT(1)
#define USBH_VIDEO_FORMAT_H264         BIT(2)
#define USBH_VIDEO_FORMAT_UNCOMPRESSED_YUY2 ((1 << 4) | USBH_VIDEO_FORMAT_UNCOMPRESSED)
#define USBH_VIDEO_FORMAT_UNCOMPRESSED_NV12 ((1 << 5) | USBH_VIDEO_FORMAT_UNCOMPRESSED)

#define UVC_GET_FORMAT(x)           ((x) & 0xf)
#define UVC_GET_SUBFORMAT(x)        ((x >> 4) & 0xf)
struct usbh_video_resolution {
    uint16_t wWidth;
    uint16_t wHeight;
};

struct usbh_video_format {
    struct usbh_video_resolution frame[36];
    char format_type_name[5];
    uint8_t format_type;
    uint8_t num_of_frames;
};

struct usbh_videoframe {
    uint8_t *frame_buf;
    uint32_t frame_bufsize;
    uint32_t frame_format;
    uint32_t frame_size;
    uint8_t index;
};

struct usbh_videostreaming {
    struct usbh_videoframe *frame;
    uint32_t frame_format;
    uint32_t bufoffset;
    uint16_t width;
    uint16_t height;
};

struct usbh_video {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *isoin;  /* ISO IN endpoint */
    struct usb_endpoint_descriptor *isoout; /* ISO OUT endpoint */

    uint8_t ctrl_intf; /* interface number */
    uint8_t data_intf; /* interface number */
    uint8_t minor;
    struct video_probe_and_commit_controls probe;
    struct video_probe_and_commit_controls commit;
    uint16_t isoin_mps;
    uint16_t isoout_mps;
    bool is_opened;
    uint8_t current_format;
    uint16_t bcdVDC;
    uint8_t num_of_intf_altsettings;
    uint8_t num_of_formats;
    struct usbh_video_format format[3];

    void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int usbh_video_get(struct usbh_video *video_class, uint8_t request, uint8_t intf, uint8_t entity_id, uint8_t cs, uint8_t *buf, uint16_t len);
int usbh_video_set(struct usbh_video *video_class, uint8_t request, uint8_t intf, uint8_t entity_id, uint8_t cs, uint8_t *buf, uint16_t len);

int usbh_video_open(struct usbh_video *video_class,
                    uint8_t format_type,
                    uint16_t wWidth,
                    uint16_t wHeight,
                    uint8_t altsetting);
int usbh_video_close(struct usbh_video *video_class);

void usbh_video_list_info(struct usbh_video *video_class);

void usbh_video_run(struct usbh_video *video_class);
void usbh_video_stop(struct usbh_video *video_class);

#ifdef __cplusplus
}
#endif

#endif /* USBH_VIDEO_H */
