/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: gif demo
*        lvgl has gif control,so if you use lvgl,please using it.
*/

#include <rthw.h>
#include <rtthread.h>
#include <shell.h>
#include <getopt.h>
#include <artinchip_fb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "gifdecoder.h"
#include "aic_core.h"
#include "mpp_fb.h"
#include "mpp_types.h"

#define align_cache_line(addr, len, a, l) \
unsigned long a = (unsigned long)addr; \
unsigned long l = (unsigned long)len; \
if (addr % CACHE_LINE_SIZE) { \
    a -= (addr %  CACHE_LINE_SIZE); \
    l += (addr %  CACHE_LINE_SIZE); \
} \
if (l % CACHE_LINE_SIZE) { \
    l += CACHE_LINE_SIZE - (l % CACHE_LINE_SIZE); \
}

struct gif_ctx {
    gd_gif *gif;
    struct mpp_fb *fb;
    int loop_count;
    int screen_width;
    int screen_height;
    unsigned char is_file;
    int frame_time;
    char file_path[256];
    int data_size;
    unsigned char *data;
};

static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: gif_test [options]:\n"
        "\t-i                             input gif file name\n"
        "\t-l                             loop time,default 1\n"
        "\t-f                             1-file,0-load file to mem ,default 1\n"
        "\t-t                             show time per frame,unit(ms),default 50ms\n"
        "\t-h                             help\n"
        "Example1: gif_test -i /mnt/video/test.giff -l 10 -f 0 -t 30(file can not too large) \n"
        "Example2: gif_test -i /mnt/video/test.giff -l 10 -f 1 -t 30 \n");
}

static int set_ui_layer_alpha(struct mpp_fb* fb, int val)
{
    int ret = 0;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = val;
    ret = mpp_fb_ioctl(fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    if (ret < 0)
        printf("ioctl() failed! errno: %d\n", ret);

    return ret;
}

static void render_frame(struct gif_ctx *ctx , struct mpp_frame* frame)
{
    struct aicfb_layer_data layer = {0};
    // be careful,show on ui layer, DE framebuffer must be same frame->buf.format
    //layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.layer_id = AICFB_LAYER_TYPE_VIDEO;
    if (layer.layer_id == AICFB_LAYER_TYPE_VIDEO) {
        set_ui_layer_alpha(ctx->fb,0);
    }
    layer.rect_id = 0;
    layer.enable = 1;
    layer.buf.phy_addr[0] = frame->buf.phy_addr[0];
    layer.buf.stride[0] = frame->buf.stride[0];
    layer.buf.size.width = frame->buf.size.width;
    layer.buf.size.height = frame->buf.size.height;
    layer.buf.crop_en = 0;
    layer.buf.format = frame->buf.format;
    layer.buf.buf_type = MPP_PHY_ADDR;
    // set disp_rect, align center
    layer.pos.x = (ctx->screen_width - frame->buf.size.width)/2;
    layer.pos.y = (ctx->screen_height - frame->buf.size.height)/2;
    layer.scale_size.width = frame->buf.size.width;
    layer.scale_size.height = frame->buf.size.height;

    int ret = mpp_fb_ioctl(ctx->fb, AICFB_UPDATE_LAYER_CONFIG, &layer);
    if (ret < 0) {
        printf("update_layer_config error, %d\n", ret);
    }
    aicos_msleep(ctx->frame_time);

}

static void gif_thread(void *arg)
{
    struct gif_ctx *ctx = (struct gif_ctx *)arg;
    int has_next = 0;
    struct mpp_frame frame;
    if (!ctx) {
        printf("para error\n");
        return;
    }

    if (ctx->is_file) {
        ctx->gif = gifdec_open_gif_file(ctx->file_path);
    } else {
        ctx->gif = gifdec_open_gif_data(ctx->data);
    }

    if (!ctx->gif) {
         printf("gd_open_gif_file %s error\n",ctx->file_path);
         goto _EXIT_;
    }

    frame.buf.buf_type =MPP_PHY_ADDR;
    if (GIF_COLOR_FORMAT == MPP_FMT_ARGB_8888) {
        frame.buf.format = MPP_FMT_ARGB_8888;
        frame.buf.stride[0] = (ctx->gif->width * 4 + 7)&(~7);//8byte align
    } else if(GIF_COLOR_FORMAT == MPP_FMT_RGB_565) {
        frame.buf.format = MPP_FMT_RGB_565;
        frame.buf.stride[0] = (ctx->gif->width * 2 + 7)&(~7);;//8byte align
    } else {
        printf("unsupport GIF_COLOR_FORMAT :%d\n",GIF_COLOR_FORMAT);
        goto _EXIT_;
    }
    frame.buf.size.width = ctx->gif->width;
    frame.buf.size.height = ctx->gif->height;
    //ctx->gif->canvas store decode data,ctx->gif->canvas was malloced by gif lib.
    frame.buf.phy_addr[0] = (unsigned long)ctx->gif->canvas;

    while(1) {
        has_next = 0;
        has_next = gifdec_get_frame(ctx->gif);
        if(has_next == 0) {
            /*It was the last repeat*/
            if(ctx->loop_count == 1) {
                break;
            } else {
                if(ctx->loop_count > 1) {
                    ctx->loop_count--;
                }
                gifdec_rewind(ctx->gif);
            }
        }
        gifdec_render_frame(ctx->gif, ctx->gif->canvas);
        // clean cache
        align_cache_line((unsigned long)ctx->gif->canvas, (int64_t)(frame.buf.stride[0]*frame.buf.size.height), a, l);
        aicos_dcache_clean_range((unsigned long *)a, (int64_t)l);
        render_frame(ctx,&frame);
    }

_EXIT_:
    if (ctx) {
        if (ctx->gif) {
            gifdec_close_gif(ctx->gif);
            ctx->gif = NULL;
        }
        if (ctx->fb) {
            mpp_fb_close(ctx->fb);
            ctx->fb = NULL;
        }
         aicos_free(MEM_DEFAULT,ctx);
         ctx = NULL;
    }
}

static void gif_test(int argc, char **argv)
{
    int opt;
    aicos_thread_t thid = NULL;
    struct gif_ctx *ctx = NULL;
    struct aicfb_screeninfo screen_info;

    ctx = aicos_malloc(MEM_DEFAULT,sizeof(struct gif_ctx));
    if(NULL == ctx) {
        printf("aicos_malloc error\n");
        return;
    }
    memset(ctx,0x00,sizeof(struct gif_ctx));
    ctx->fb = mpp_fb_open();
    if (!ctx->fb) {
        printf("mpp_fb_open error!!!!\n");
        goto _EXIT_;
    }
    if (mpp_fb_ioctl(ctx->fb, AICFB_GET_SCREENINFO, &screen_info) < 0) {
        printf("fb ioctl() AICFB_GET_SCREEN_SIZE failed!");
            goto _EXIT_;
    }
    ctx->screen_width = screen_info.width;
    ctx->screen_height = screen_info.height;
    // default value
    ctx->is_file = 1;
    ctx->loop_count = 1;
    ctx->frame_time = 50;

   optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:l:f:t:h");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            if (strlen(optarg) > sizeof(ctx->file_path) -1) {
                printf("file path too long\n");
                return;
            }
            strcpy(ctx->file_path, optarg);
            break;
        case 'l':
            ctx->loop_count = atoi(optarg);
            break;
        case 'f':
            ctx->is_file = atoi(optarg);
            break;
        case 't':
            ctx->frame_time = atoi(optarg);
            break;
        case 'h':
            print_help(argv[0]);
        default:
            goto _EXIT_;
            break;
        }
    }
    if (strlen(ctx->file_path) == 0) {
        print_help(argv[0]);
        goto _EXIT_;
    }

    // load file to mem
    if (ctx->is_file != 1) {
        struct stat st;
        int fd = -1;
        int r_len = 0;
        int count = 0;
        int pos = 0;

        stat(ctx->file_path, &st);
        ctx->data_size = st.st_size;
        ctx->data = aicos_malloc(MEM_DEFAULT,ctx->data_size);
        if (!ctx->data) {
            printf("aicos_malloc %d error\n",ctx->data_size);
            goto _EXIT_;
        }
        fd = open(ctx->file_path,O_RDONLY);
        if (fd < 0) {
            printf("open %s error\n",ctx->file_path);
            goto _EXIT_;
        }
        count = ctx->data_size;
        while(count > 0) {
            r_len = read(fd,ctx->data+pos,ctx->data_size);
            if (r_len < 0) {
                printf("read %s error\n",ctx->file_path);
                close(fd);
                goto _EXIT_;
            } else if (r_len == 0) {
                break;
            }
            count -= r_len;
            pos += r_len;
        }
        close(fd);
    }

    thid = aicos_thread_create("gif_test", 8096, 0, gif_thread, ctx);
    if (thid == NULL) {
        printf("failed to create thread\n");
        goto _EXIT_;
    }
    return;

_EXIT_:
    if (ctx) {
        if(ctx->data) {
            aicos_free(MEM_DEFAULT,ctx->data);
            ctx->data = NULL;
        }
        if (ctx->fb) {
            mpp_fb_close(ctx->fb);
            ctx->fb = NULL;
        }
        aicos_free(MEM_DEFAULT,ctx);
        ctx = NULL;
    }
}

MSH_CMD_EXPORT_ALIAS(gif_test, gif_test, gif test);
