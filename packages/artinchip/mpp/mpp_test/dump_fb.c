/*
* Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
*
*  SPDX-License-Identifier: Apache-2.0
*
*  author: <qi.xu@artinchip.com>
*  Desc: dump fb to bmp file
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include "aic_osal.h"
#include "mpp_fb.h"
#include "mpp_mem.h"
#include "frame_allocator.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"

static const char sopts[] = "o:dh";
static const struct option lopts[] = {
    {"output",    required_argument, NULL, 'o'},
    {"help",     no_argument,       NULL, 'h'},

    {0, 0, 0, 0}
};

static struct aicfb_screeninfo g_info = {0};

static void print_help(char *program)
{
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: %s [options]", program);
    printf("\t -o, --output: \t\t output stream file name\n");
    printf("\t -h, --help: \t\t print help info\n");
    printf("End:\n");
}

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BitmapFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BitmapInfoHeader;
#pragma pack(pop)

int save_rgb_to_bmp(int fd, const uint8_t* rgb_data) {
    if (!fd || !rgb_data) {
        return -1;
    }
    uint16_t data;
    int width = g_info.width;
    int height = g_info.height;

    const int bytes_per_pixel = 3;
    const int stride = (width * bytes_per_pixel + 3) & ~3;

    BitmapFileHeader file_header = {
        .bfType = 0x4D42, // 'BM'
        .bfSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * height,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader)
    };

    BitmapInfoHeader info_header = {
        .biSize = sizeof(BitmapInfoHeader),
        .biWidth = width,
        .biHeight = height,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = 0,
        .biSizeImage = stride * height,
        .biXPelsPerMeter = 0,
        .biYPelsPerMeter = 0,
        .biClrUsed = 0,
        .biClrImportant = 0
    };

    write(fd, &file_header, sizeof(file_header));
    write(fd, &info_header, sizeof(info_header));

    if (g_info.format == MPP_FMT_BGR_888) {
        write(fd, g_info.framebuffer, g_info.stride * g_info.height);
        return 0;
    }

    uint8_t* row_buffer = (uint8_t*)malloc(stride);
    if (!row_buffer) {
        return -1;
    }
    memset(row_buffer, 0, stride);

    for (int y = 0; y < height; y++) {
        const uint8_t* src_row = rgb_data + (height - 1 - y) * g_info.stride;

        for (int x = 0; x < width; x++) {
            switch (g_info.format) {
            case MPP_FMT_RGB_565:
                data = ((uint16_t)src_row[x * 2 + 1] << 8) | src_row[x * 2];
                row_buffer[x * 3 + 0] = (data  & 0x1f) << 3;
                row_buffer[x * 3 + 1] = ((data >> 5) & 0x3f) << 2;
                row_buffer[x * 3 + 2] = ((data >> 11) & 0x1f) << 3;
                break;

            case MPP_FMT_BGR_888:
                row_buffer[x * 3 + 0] = src_row[x * 3 + 2];
                row_buffer[x * 3 + 1] = src_row[x * 3 + 1];
                row_buffer[x * 3 + 2] = src_row[x * 3 + 0];
                break;

            case MPP_FMT_RGB_888:
                row_buffer[x * 3 + 0] = src_row[x * 3 + 0];
                row_buffer[x * 3 + 1] = src_row[x * 3 + 1];
                row_buffer[x * 3 + 2] = src_row[x * 3 + 2];
                break;

            case MPP_FMT_RGBA_8888:
                row_buffer[x * 3 + 0] = src_row[x * 4 + 1];
                row_buffer[x * 3 + 1] = src_row[x * 4 + 2];
                row_buffer[x * 3 + 2] = src_row[x * 4 + 3];
                break;
            case MPP_FMT_BGRA_8888:
                row_buffer[x * 3 + 0] = src_row[x * 4 + 3];
                row_buffer[x * 3 + 1] = src_row[x * 4 + 2];
                row_buffer[x * 3 + 2] = src_row[x * 4 + 1];
                break;

            case MPP_FMT_ABGR_8888:
                row_buffer[x * 3 + 0] = src_row[x * 4 + 2];
                row_buffer[x * 3 + 1] = src_row[x * 4 + 1];
                row_buffer[x * 3 + 2] = src_row[x * 4 + 0];
                break;

            case MPP_FMT_ARGB_8888:
                row_buffer[x * 3 + 0] = src_row[x * 4 + 0]; // B
                row_buffer[x * 3 + 1] = src_row[x * 4 + 1]; // G
                row_buffer[x * 3 + 2] = src_row[x * 4 + 2]; // R
                break;

            default:
                loge("unsupport format: 0x%x", g_info.format);
                free(row_buffer);
                return -1;
            }
        }

        if (write(fd, row_buffer, stride) != stride) {
            loge("write data error");
            free(row_buffer);
            return -1;
        }
    }

    free(row_buffer);
    return 0;
}

void dump_fb(int argc, char **argv)
{
    int ret = 0;
    int fd = 0;
    struct mpp_fb *fb = NULL;

    if (argc < 2) {
        print_help(argv[0]);
        return;
    }

    optind = 0;
    int c;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'o':
            fd = open(optarg, O_WRONLY | O_CREAT);
            if(fd < 0) {
                loge("open file(%s) failed, %d", optarg, fd);
                return;
            }

            continue;

        case 'h':
        default:
            return print_help(argv[0]);
        }
    }

    if (fd == 0) {
        return print_help(argv[0]);
    }

    fb = mpp_fb_open();
    if (!fb) {
        loge("mpp_fb_open error!!!!\n");
        goto out;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO , &g_info);
    if (ret) {
        loge("get screen info failed\n");
        goto out;
    }
    printf("[%s:%d]format:%d,width:%d,height:%d,stride:%d,smem_len:%d,framebuffer:%p\n"
            ,__FUNCTION__,__LINE__
            ,g_info.format, g_info.width, g_info.height
            ,g_info.stride, g_info.smem_len, g_info.framebuffer);

    aicos_dcache_invalid_range(g_info.framebuffer, g_info.smem_len);

    save_rgb_to_bmp(fd, g_info.framebuffer);

out:
    if (fd > 0) {
        close(fd);
    }
    if (fb) {
        mpp_fb_close(fb);
    }
}

MSH_CMD_EXPORT_ALIAS(dump_fb, dump_fb, dump fb data);
