/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "lvgl.h"
#include "aic_core.h"
#include "mpp_mem.h"
#include "mpp_ge.h"
#include "mpp_decoder.h"
#include "mpp_list.h"
#include "frame_allocator.h"
#include "aic_dec.h"
#include "aic_ui.h"

#define PNG_HEADER_SIZE (8 + 12 + 13) //png signature + IHDR chuck
#define PNGSIG 0x89504e470d0a1a0aull
#define MNGSIG 0x8a4d4e470d0a1a0aull
#define JPEG_SOI 0xFFD8
#define JPEG_SOF 0xFFC0
#define ALIGN_1024B(x) ((x+1023) & (~1023))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))

struct lv_stream_t
{
    lv_fs_file_t f;
    int cur_index;
    const uint8_t *src;
    int is_file;
    int data_size;
};

static struct mpp_list buf_list;

static inline uint64_t stream_to_u64(uint8_t *ptr)
{
    return ((uint64_t)ptr[0] << 56) | ((uint64_t)ptr[1] << 48) |
           ((uint64_t)ptr[2] << 40) | ((uint64_t)ptr[3] << 32) |
           ((uint64_t)ptr[4] << 24) | ((uint64_t)ptr[5] << 16) |
           ((uint64_t)ptr[6] << 8) | ((uint64_t)ptr[7]);
}

static inline unsigned int stream_to_u32(uint8_t *ptr)
{
    return ((unsigned int)ptr[0] << 24) |
           ((unsigned int)ptr[1] << 16) |
           ((unsigned int)ptr[2] << 8) |
           (unsigned int)ptr[3];
}

static inline unsigned short stream_to_u16(uint8_t *ptr)
{
    return  ((unsigned int)ptr[0] << 8) | (unsigned int)ptr[1];
}

static lv_fs_res_t lv_aic_stream_open(struct lv_stream_t *stream, const void *src)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    stream->src = ((lv_img_dsc_t *)src)->data;
    stream->cur_index = 0;
    if (lv_img_src_get_type(src) == LV_IMG_SRC_FILE) {
        stream->is_file = 1;
        res = lv_fs_open(&stream->f, src, LV_FS_MODE_RD);
    } else {
        stream->is_file = 0;
        stream->data_size = ((lv_img_dsc_t *)src)->data_size;
    }

    return res;
}

static lv_fs_res_t lv_aic_stream_close(struct lv_stream_t *stream)
{
   if (stream->is_file)
        lv_fs_close(&stream->f);

    return LV_FS_RES_OK;
}

static lv_fs_res_t lv_aic_stream_seek(struct lv_stream_t *stream, uint32_t pos, lv_fs_whence_t whence)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    if (stream->is_file) {
        res = lv_fs_seek(&stream->f, pos, whence);
    } else {
        // only support SEEK_CUR
        stream->cur_index += pos;
    }

    return res;
}

static lv_fs_res_t lv_aic_stream_read(struct lv_stream_t *stream, void *buf, uint32_t btr, uint32_t *br)
{
    lv_fs_res_t res = LV_FS_RES_OK;

    if (stream->is_file) {
        res = lv_fs_read(&stream->f, buf, btr, br);
    } else {
        if (stream->cur_index + btr <= stream->data_size) {
            memcpy(buf, &stream->src[stream->cur_index], btr);
            stream->cur_index += btr;
            *br = btr;
        } else {
            LV_LOG_WARN("data_size:%d, cur_index:%d", stream->data_size, stream->cur_index);
        }
    }

    return res;
}

static void lv_aic_stream_reset(struct lv_stream_t *stream)
{
    stream->cur_index = 0;
    return;
}

static lv_res_t lv_aic_stream_get_size(struct lv_stream_t *stream, uint32_t *file_size)
{
    lv_res_t res = LV_RES_OK;

    if (stream->is_file) {
        lv_fs_seek(&stream->f, 0, SEEK_END);
        lv_fs_tell(&stream->f, file_size);
        lv_fs_seek(&stream->f, 0, SEEK_SET);
    } else {
        *file_size = stream->data_size;
    }

    return res;
}

static int get_jpeg_format(uint8_t *buf, enum mpp_pixel_format *pix_fmt)
{
    int i;
    uint32_t h_count_flag;
    uint32_t v_count_flag;
    uint8_t h_count[3] = { 0 };
    uint8_t v_count[3] = { 0 };
    uint8_t nb_components = *buf++;

    for (i = 0; i < nb_components; i++) {
        uint8_t h_v_cnt;

        /* skip component id */
        buf++;
        h_v_cnt = *buf++;
        h_count[i] = h_v_cnt >> 4;
        v_count[i] = h_v_cnt & 0xf;

        /*skip quant_index*/
        buf++;
    }

    h_count_flag =  h_count[2] | (h_count[1] << 4) | (h_count[0] << 8);
    v_count_flag =  v_count[2] | (v_count[1] << 4) | (v_count[0] << 8);

    if (h_count_flag == 0x211 && v_count_flag == 0x211) {
        *pix_fmt = MPP_FMT_YUV420P;
    } else if (h_count_flag == 0x211 && v_count_flag == 0x111) {
        *pix_fmt = MPP_FMT_YUV422P;
    } else if (h_count_flag == 0x111 && v_count_flag == 0x111) {
        *pix_fmt = MPP_FMT_YUV444P;
    } else if (h_count_flag == 0x111 && v_count_flag == 0x222) {
        *pix_fmt = MPP_FMT_YUV444P;
    } else if (h_count[1] == 0 && v_count[1] == 0 && h_count[2] == 0 &&
               v_count[2] == 0) {
        *pix_fmt = MPP_FMT_YUV400;
    } else {
        LV_LOG_ERROR("Not support format! h_count: %d %d %d, v_count: %d %d %d",
            h_count[0], h_count[1], h_count[2],
            v_count[0], v_count[1], v_count[2]);
        return -1;
    }

#ifndef AIC_VE_DRV_V10
#if LV_COLOR_DEPTH  == 16
    *pix_fmt = MPP_FMT_RGB_565;
#else
    *pix_fmt = MPP_FMT_RGB_888;
#endif
#endif
    return 0;
}

static lv_res_t jpeg_get_img_size(struct lv_stream_t *stream, int *w, int *h, enum mpp_pixel_format *pix_fmt)
{
    uint32_t read_num;
    uint8_t buf[128];
    lv_fs_res_t fs_res;
    lv_res_t res = LV_RES_OK;

    // read JPEG SOI
    fs_res = lv_aic_stream_read(stream, buf, 2, &read_num);
    if (fs_res != LV_FS_RES_OK || read_num != 2) {
        res = LV_RES_INV;
        goto read_err;
    }

    // check SOI
    if (buf[0] != 0xff || buf[1] != 0xd8) {
        res = LV_RES_INV;
        goto read_err;
    }

    /* check SOI */
    if (stream_to_u16(buf) != JPEG_SOI) {
        res = LV_RES_INV;
        goto read_err;
    }

    /* find SOF */
    while (1) {
        int size;
        fs_res = lv_aic_stream_read(stream, buf, 4, &read_num);
        if (fs_res != LV_FS_RES_OK || read_num != 4) {
            res = LV_RES_INV;
            goto read_err;
        }

        if (stream_to_u16(buf) == JPEG_SOF) {
            fs_res = lv_aic_stream_read(stream, buf, 15, &read_num);
            if (fs_res != LV_FS_RES_OK) {
                res = LV_RES_INV;
                goto read_err;
            }

            *h = stream_to_u16(buf + 1);
            *w = stream_to_u16(buf + 3);

            get_jpeg_format(buf + 5, pix_fmt);
            break;
        } else {
            size = stream_to_u16(buf + 2);
             fs_res = lv_aic_stream_seek(stream, size - 2, SEEK_CUR);
            if (fs_res != LV_FS_RES_OK) {
                res = LV_RES_INV;
                goto read_err;
            }
        }
    }
read_err:
    return res;
}

static lv_res_t jpeg_decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    lv_fs_res_t res;
    int width;
    int height;
    enum mpp_pixel_format fomat;
    struct lv_stream_t stream;

    res = lv_aic_stream_open(&stream, src);
    if (res != LV_FS_RES_OK)
        return LV_RES_INV;

    res = jpeg_get_img_size(&stream, &width, &height, &fomat);
    if (res != LV_RES_OK) {
        lv_aic_stream_close(&stream);
        return LV_RES_INV;
    }

#if defined(MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE)
    int shift = jpeg_size_limit(width, height);
    header->w = width >> shift;
    header->h = height >> shift;
#else
    header->w = width;
    header->h = height;
#endif

    header->cf = LV_IMG_CF_TRUE_COLOR;

    LV_LOG_INFO("w:%d, h:%d, cf:%d\n", header->w, header->h, header->cf);
	lv_aic_stream_close(&stream);

    return LV_RES_OK;
}

static lv_res_t png_get_img_size(struct lv_stream_t *stream, int *w, int *h, enum mpp_pixel_format *fomat)
{
    uint32_t read_num;
    unsigned char buf[64];
    int color_type;
    uint64_t sig;

    lv_aic_stream_read(stream, buf, 8, &read_num);
    sig = stream_to_u64(buf);
    if (sig != PNGSIG && sig != MNGSIG) {
        return LV_RES_INV;
    }

    lv_aic_stream_read(stream, buf, PNG_HEADER_SIZE - 8, &read_num);

    *w = stream_to_u32(buf + 8);
    *h = stream_to_u32(buf + 8 + 4);

    color_type = buf[8 + 8 + 1];
    if (color_type == 2)
        *fomat = MPP_FMT_RGB_888;
    else
        *fomat = MPP_FMT_ARGB_8888;

    return LV_RES_OK;
}

static lv_res_t fake_decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    int width;
    int height;
    int blend;
    unsigned int color;

    FAKE_IMAGE_PARSE((char *)src, &width, &height, &blend, &color);
    header->w = width;
    header->h = height;
    header->cf = LV_IMG_CF_TRUE_COLOR;

    return LV_RES_OK;
}

static lv_res_t png_decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    lv_fs_res_t res;
    uint32_t read_num;
    uint8_t buf[64];
    struct lv_stream_t stream;

    res = lv_aic_stream_open(&stream, src);
    if (res != LV_FS_RES_OK) {
        return LV_RES_INV;
    }

    // read png sig + IHDR chuck
    res = lv_aic_stream_read(&stream, buf, PNG_HEADER_SIZE, &read_num);
    if (res != LV_FS_RES_OK || read_num != PNG_HEADER_SIZE) {
        LV_LOG_ERROR("lv_aic_stream_read failed");
        lv_aic_stream_close(&stream);
        return LV_RES_INV;
    }

    /* check signature */
    uint64_t sig = stream_to_u64(buf);
    if (sig != PNGSIG && sig != MNGSIG) {
        LV_LOG_WARN("Invalid PNG signature 0x%08llx.", (unsigned long long)sig);
        lv_aic_stream_close(&stream);
        return LV_RES_INV;
    }

    header->w = stream_to_u32(buf + 8 + 8);
    header->h = stream_to_u32(buf + 8 + 8 + 4);
    header->cf = LV_IMG_CF_RAW;

    LV_LOG_INFO("header->w:%d, header->h:%d\n", header->w, header->h);
    lv_aic_stream_close(&stream);

    return LV_RES_OK;
}

static lv_res_t aic_decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    char* ptr = NULL;
    lv_res_t res = LV_RES_INV;

    if (lv_img_src_get_type(src) == LV_IMG_SRC_FILE) {
        ptr = strrchr(src, '.');
        if (!strcmp(ptr, ".png")) {
            return png_decoder_info(decoder, src, header);
        } else if ((!strcmp(ptr, ".jpg")) || (!strcmp(ptr, ".jpeg"))) {
            return jpeg_decoder_info(decoder, src, header);
        } else if (!strcmp(ptr, ".fake")) {
            return fake_decoder_info(decoder, src, header);
        } else {
            return LV_RES_INV;
        }
    } else if (lv_img_src_get_type(src) == LV_IMG_SRC_VARIABLE) {
        lv_img_cf_t cf = ((lv_img_dsc_t *)src)->header.cf;
        if (cf == LV_IMG_CF_RAW || cf == LV_IMG_CF_RAW_ALPHA) {
            res = jpeg_decoder_info(decoder, src, header);
            if (res != LV_RES_OK)
                res = png_decoder_info(decoder, src, header);
        } else if (cf == LV_IMG_CF_RESERVED_16) {
            struct mpp_buf *buf = (struct mpp_buf *)((lv_img_dsc_t *)src)->data;
            if (buf->crop_en) {
                header->w = buf->crop.width;
                header->h = buf->crop.height;
            } else {
                header->w = buf->size.width;
                header->h = buf->size.height;
            }

            if (buf->format == MPP_FMT_ARGB_8888) {
                header->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
                res = LV_RES_OK;
            } else if (buf->format == MPP_FMT_RGB_565) {
                header->cf = LV_IMG_CF_TRUE_COLOR;
                res = LV_RES_OK;
            }
            LV_LOG_INFO("w:%d, h:%d, cf:%d", header->w, header->h, header->cf);
        }
    }

    return res;
}

struct ext_frame_allocator {
    struct frame_allocator base;
    struct mpp_frame* frame;
};

static int alloc_frame_buffer(struct frame_allocator *p, struct mpp_frame* frame,
                              int width, int height, enum mpp_pixel_format format)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)p;

    memcpy(frame, impl->frame, sizeof(struct mpp_frame));
    return 0;
}

static int free_frame_buffer(struct frame_allocator *p, struct mpp_frame *frame)
{
    return 0;
}

static int close_allocator(struct frame_allocator *p)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)p;

    free(impl);

    return 0;
}

static struct alloc_ops def_ops = {
    .alloc_frame_buffer = alloc_frame_buffer,
    .free_frame_buffer = free_frame_buffer,
    .close_allocator = close_allocator,
};

static struct frame_allocator* open_allocator(struct mpp_frame* frame)
{
    struct ext_frame_allocator* impl = (struct ext_frame_allocator*)malloc(sizeof(struct ext_frame_allocator));

    if(impl == NULL) {
        return NULL;
    }

    memset(impl, 0, sizeof(struct ext_frame_allocator));

    impl->frame = frame;
    impl->base.ops = &def_ops;
    return &impl->base;
}

static void frame_buf_free(struct mpp_frame *alloc_frame)
{
    int i;
    struct framebuf_head *node = NULL, *head = NULL;

    for (i = 0; i < 3; i++) {
        if (alloc_frame->buf.phy_addr[i]) {
            mpp_list_for_each_entry_safe(head, node, &buf_list, list) {
                if (head->buf_info.align_addr == alloc_frame->buf.phy_addr[i]) {
                    mpp_list_del(&head->list);
                    aicos_free(MEM_CMA, (void*)(unsigned long)head->buf_info.addr);
                    alloc_frame->buf.phy_addr[i] = 0;
                    aicos_free(MEM_DEFAULT, head);
                    break;
                }
            }
        }
    }
}

static int frame_buf_alloc(struct mpp_frame *alloc_frame, int size[])
{
    int i;

    for (i = 0; i < 3; i++) {
        if (size[i]) {
            struct framebuf_head *head;

            head =(struct framebuf_head *)aicos_malloc(MEM_DEFAULT, sizeof(*head));
            if (!head) {
                goto out;
            }
            memset(head, 0, sizeof(struct framebuf_head));
            head->buf_info.addr = (unsigned long)aicos_malloc_try_cma(size[i] + 1023);
            head->buf_info.align_addr = ALIGN_1024B(head->buf_info.addr);

            if (!head->buf_info.addr) {
                aicos_free(MEM_DEFAULT, head);
                goto out;
            } else {
                head->buf_info.align_addr = ALIGN_1024B(head->buf_info.addr);
                head->buf_info.size = size[i];
                alloc_frame->buf.phy_addr[i] = head->buf_info.align_addr;
                aicos_dcache_clean_invalid_range((unsigned long *)((unsigned long)head->buf_info.align_addr), size[i]);
                mpp_list_add_tail(&head->list, &buf_list);
            }
        }
    }

    return 0;
out:
    frame_buf_free(alloc_frame);
    return -1;
}

static void set_frame_buf_size(struct mpp_frame *frame, int *buf_size, int size_shift)
{
    int height_align;
    int width = frame->buf.size.width;
    int height = frame->buf.size.height;

    if (size_shift > 0) {
        height_align = ALIGN_16B(ALIGN_16B(height) >> size_shift);
        frame->buf.size.width =  width >> size_shift;;
        frame->buf.size.height = height >> size_shift;
    } else {
        height_align = ALIGN_16B(height);
    }

    switch (frame->buf.format) {
    case MPP_FMT_YUV420P:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift);
        else
            frame->buf.stride[0] =  ALIGN_16B(width);

        frame->buf.stride[1] =  frame->buf.stride[0] >> 1;
        frame->buf.stride[2] =  frame->buf.stride[0] >> 1;
        buf_size[0] = frame->buf.stride[0] * height_align;
        buf_size[1] = frame->buf.stride[1] * (height_align >> 1);
        buf_size[2] = frame->buf.stride[2] * (height_align >> 1);
        break;
    case MPP_FMT_YUV422P:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift);
        else
            frame->buf.stride[0] =  ALIGN_16B(width);

        frame->buf.stride[1] =  frame->buf.stride[0] >> 1;
        frame->buf.stride[2] =  frame->buf.stride[0] >> 1;
        buf_size[0] = frame->buf.stride[0] * height_align;
        buf_size[1] = frame->buf.stride[1] * height_align;
        buf_size[2] = frame->buf.stride[2] * height_align;
        break;
    case MPP_FMT_YUV444P:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift);
        else
            frame->buf.stride[0] =  ALIGN_16B(width);

        frame->buf.stride[1] =  frame->buf.stride[0];
        frame->buf.stride[2] =  frame->buf.stride[0];
        buf_size[0] = frame->buf.stride[0] * height_align;
        buf_size[1] = frame->buf.stride[1] * height_align;
        buf_size[2] = frame->buf.stride[2] * height_align;
        break;
    case MPP_FMT_RGB_565:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift) * 2;
        else
            frame->buf.stride[0] =  ALIGN_16B(width) * 2;

        buf_size[0] = frame->buf.stride[0] * height_align;
    case MPP_FMT_RGB_888:
        if (size_shift > 0)
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift) * 3;
        else
            frame->buf.stride[0] =  ALIGN_16B(width) * 3;

        buf_size[0] = frame->buf.stride[0] * height_align;
        break;
    case MPP_FMT_ARGB_8888:
        if (size_shift > 0) {
            frame->buf.stride[0] =  ALIGN_16B(ALIGN_16B(width) >> size_shift) * 4;
            buf_size[0] = frame->buf.stride[0] * height_align;
        } else {
            frame->buf.stride[0] =  ALIGN_16B(width * 4);
            buf_size[0] = ALIGN_UP(frame->buf.stride[0] * height, CACHE_LINE_SIZE);
        }
        break;
    default:
        LV_LOG_ERROR("unsupported format:%d", frame->buf.format);
        break;
    }
}

static lv_res_t aic_decoder_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    lv_res_t res = LV_RES_OK;
    uint32_t file_len = 0;
    struct lv_stream_t stream;
    struct mpp_packet packet;
    int width = 0;
    int height = 0;
    enum mpp_codec_type type = MPP_CODEC_VIDEO_DECODER_PNG;
    char* ptr = NULL;
    struct decode_config config = { 0 };
    int buf_size[3] = { 0 };
    struct mpp_decoder *dec = NULL;
    struct frame_allocator *allocator = NULL;
    struct mpp_frame *alloc_frame = NULL;
    lv_img_cf_t cf = ((lv_img_dsc_t *)dsc->src)->header.cf;

    // means mpp buffer from user
    if (cf == LV_IMG_CF_RESERVED_16) {
        dsc->img_data = (unsigned char *)((lv_img_dsc_t *)dsc->src)->data;
        dsc->header.cf = LV_IMG_CF_RESERVED_16;
        return LV_RES_OK;
    }

    if (lv_aic_stream_open(&stream, dsc->src) != LV_FS_RES_OK)
        return LV_RES_INV;

    if (lv_img_src_get_type(dsc->src) == LV_IMG_SRC_FILE) {
        ptr = strrchr(dsc->src, '.');
        if ((!strcmp(ptr, ".jpg")) || (!strcmp(ptr, ".jpeg")))
            type = MPP_CODEC_VIDEO_DECODER_MJPEG;

        if (type == MPP_CODEC_VIDEO_DECODER_PNG) {
            png_get_img_size(&stream, &width, &height, &config.pix_fmt);
            if (config.pix_fmt == MPP_FMT_ARGB_8888)
                dsc->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
            else
                dsc->header.cf = LV_IMG_CF_TRUE_COLOR;
        } else {
            dsc->header.cf = LV_IMG_CF_TRUE_COLOR;
            jpeg_get_img_size(&stream, &width, &height, &config.pix_fmt);
        }
    } else if (lv_img_src_get_type(dsc->src) == LV_IMG_SRC_VARIABLE) {
        lv_img_cf_t cf = ((lv_img_dsc_t *)dsc->src)->header.cf;
        if (cf == LV_IMG_CF_RAW || cf == LV_IMG_CF_RAW_ALPHA) {
            // check jpg format
            res = jpeg_get_img_size(&stream, &width, &height, &config.pix_fmt);
            if (res != LV_RES_OK) {
                lv_aic_stream_reset(&stream);
                res = png_get_img_size(&stream, &width, &height, &config.pix_fmt);
                if (res != LV_RES_OK) {
                    res = LV_RES_INV;
                    goto out;
                }

                if (config.pix_fmt == MPP_FMT_ARGB_8888)
                    dsc->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
                else
                    dsc->header.cf = LV_IMG_CF_TRUE_COLOR;

                 res = LV_RES_OK;
            } else {
                type = MPP_CODEC_VIDEO_DECODER_MJPEG;
                res = LV_RES_OK;
            }
            lv_aic_stream_reset(&stream);
        }
    }

    lv_aic_stream_get_size(&stream, &file_len);
    dec = mpp_decoder_create(type);
    if (!dec) {
        res = LV_RES_INV;
        goto out;
    }

    config.bitstream_buffer_size = (file_len + 1023) & (~1023);
    config.extra_frame_num = 0;
    config.packet_count = 1;

    alloc_frame = (struct mpp_frame *)malloc(sizeof(struct mpp_frame));
    if (!alloc_frame) {
        res = LV_RES_INV;
        goto out;
    }

    memset(alloc_frame, 0, sizeof(struct mpp_frame));
    alloc_frame->id = 0;
    alloc_frame->buf.size.width = width;
    alloc_frame->buf.size.height = height;
    alloc_frame->buf.format = config.pix_fmt;
    alloc_frame->buf.buf_type = MPP_PHY_ADDR;

    int size_shift = 0;
#if defined(MPP_JPEG_DEC_OUT_SIZE_LIMIT_ENABLE)
    if (type == MPP_CODEC_VIDEO_DECODER_MJPEG)
        size_shift = jpeg_size_limit(width, height);
#endif
    set_frame_buf_size(alloc_frame, buf_size, size_shift);

    if (size_shift > 0) {
        struct mpp_scale_ratio scale;
        scale.hor_scale = size_shift;
        scale.ver_scale = size_shift;
        mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_SCALE, &scale);
    }

    if (frame_buf_alloc(alloc_frame, buf_size) < 0) {
        LV_LOG_ERROR("frame_buf_alloc failed");
        res = LV_RES_INV;
        goto out;
    }

    // allocator will be released in decoder
    allocator = open_allocator(alloc_frame);
    if (!allocator) {
        res = LV_RES_INV;
        LV_LOG_ERROR("open_allocator failed");
        goto out;
    }

    mpp_decoder_control(dec, MPP_DEC_INIT_CMD_SET_EXT_FRAME_ALLOCATOR, (void*)allocator);
    mpp_decoder_init(dec, &config);
    memset(&packet, 0, sizeof(struct mpp_packet));
    mpp_decoder_get_packet(dec, &packet, file_len);

    uint32_t read_size = 0;
    lv_aic_stream_read(&stream, packet.data, file_len, &read_size);
    packet.size = file_len;
    packet.flag = PACKET_FLAG_EOS;

    mpp_decoder_put_packet(dec, &packet);
    if (mpp_decoder_decode(dec) < 0) {
        LV_LOG_ERROR("mpp_decoder_decode failed\n");
        res = LV_RES_INV;
        goto out;
    }

    struct mpp_frame frame;
    memset(&frame, 0, sizeof(struct mpp_frame));
    mpp_decoder_get_frame(dec, &frame);
    mpp_decoder_put_frame(dec, &frame);

    // means aic mpp buffer
    dsc->header.cf = LV_IMG_CF_RESERVED_16;
    memcpy(&alloc_frame->buf, &frame, sizeof(struct mpp_frame));
    dsc->img_data = (unsigned char *)alloc_frame;

out:
    if (dec)
        mpp_decoder_destory(dec);

    if (res == LV_RES_INV) {
        if (alloc_frame) {
            frame_buf_free(alloc_frame);
            free(alloc_frame);
        }

        dsc->img_data = NULL;
    }

    lv_aic_stream_close(&stream);
    return res;
}

static void aic_decoder_close(lv_img_decoder_t * decoder, lv_img_decoder_dsc_t * dsc)
{
    if (dsc->img_data) {
        lv_img_cf_t cf = ((lv_img_dsc_t *)dsc->src)->header.cf;
        // means mpp buffer from user
        if (cf != LV_IMG_CF_RESERVED_16) {
            struct mpp_frame *alloc_frame = (struct mpp_frame *)dsc->img_data;
            if (alloc_frame) {
                frame_buf_free(alloc_frame);
                free(alloc_frame);
            }
        }

        dsc->img_data = NULL;
    }

    return;
}

void aic_dec_create()
{
    lv_img_decoder_t *aic_dec = lv_img_decoder_create();

    /* init frame info lists */
    mpp_list_init(&buf_list);
    lv_img_decoder_set_info_cb(aic_dec, aic_decoder_info);
    lv_img_decoder_set_open_cb(aic_dec, aic_decoder_open);
    lv_img_decoder_set_close_cb(aic_dec, aic_decoder_close);
}
