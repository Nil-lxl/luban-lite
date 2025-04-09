/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Author: <jun.ma@artinchip.com>
 *  Desc: player demo
 */

#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>
#include <inttypes.h>
#include <getopt.h>
#define LOG_DEBUG
#include "mpp_dec_type.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_fb.h"
#include "aic_message.h"
#include "aic_player.h"
#include "player_dec_share_test.h"
#include "player_audio_render_share_test.h"

#include <rthw.h>
#include <rtthread.h>
#include <shell.h>
#include <artinchip_fb.h>

#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#define PLAYER_DEMO_FILE_MAX_NUM 128
#define PLAYER_DEMO_FILE_PATH_MAX_LEN 256
#define SUPPORT_FILE_SUFFIX_TYPE_NUM 8

struct file_list {
    char *file_path[PLAYER_DEMO_FILE_MAX_NUM];
    int file_num;
};

struct video_player_ctx {
    struct aic_player *player;
    struct file_list  files;
    int volume;
    int loop_time;
    int seek_loop;
    int rotation;
    int alpha_value;
    int player_end;
    int player_stop;
    struct av_media_info media_info;
};

char *file_suffix[SUPPORT_FILE_SUFFIX_TYPE_NUM] = {".h264", ".264", ".mp4", ".mp3",
                                                   ".avi", ".mkv", ".ts", ".flv"};
struct video_player_ctx  *g_video_player_ctx = NULL;

static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: player_demo [options]:\n"
        "\t-i                             input stream file name\n"
        "\t-t                             directory of test files\n"
        "\t-l                             loop time\n"
        "\t-r                             rotation\n"
        "\t-h                             help\n\n"
        "Example1(test single file for 1 time): player_demo -i /mnt/video/test.mp4 \n"
        "Example2(test single file for 3 times): player_demo -i /mnt/video/test.mp4 -l 3 \n"
        "Example3(test dir for 1 time ) : player_demo -t /mnt/video \n"
        "Example4(test dir for 3 times ): player_demo -t /mnt/video -l 3 \n"
        "---------------------------------------------------------------------------------------\n"
        "-------------------------------control key while playing-------------------------------\n"
        "---------------------------------------------------------------------------------------\n"
        "('d'): play next \n"
        "('u'): play previous \n"
        "('p'): pause/play \n"
        "('+'): volum+5 \n"
        "('-'): volum-5 \n"
        "('f'): forward seek +8s \n"
        "('b'): back seek -8s \n"
        "('z'): seek to begin pos \n"
        "('m':  enter/eixt mute \n"
        "('w':  play audio warning tone \n"
        "('e'): eixt app \n");
}

static int read_file(char* path, struct file_list *files)
{
    int file_path_len;
    file_path_len = strlen(path);
    logd("file_path_len:%d\n",file_path_len);
    if (file_path_len > PLAYER_DEMO_FILE_PATH_MAX_LEN-1) {
        loge("file_path_len too long \n");
        return -1;
    }
    files->file_path[0] = (char *)aicos_malloc(MEM_DEFAULT,file_path_len+1);
    files->file_path[0][file_path_len] = '\0';
    strcpy(files->file_path[0], path);
    files->file_num = 1;
    logd("file path: %s\n", files->file_path[0]);
    return 0;
}

static int read_dir(char* path, struct file_list *files)
{
    char* ptr = NULL;
    int file_path_len = 0;
    struct dirent* dir_file;
    int i = 0;
    DIR* dir = opendir(path);
    if (dir == NULL) {
        loge("read dir failed");
        return -1;
    }

    while((dir_file = readdir(dir))) {
        if (strcmp(dir_file->d_name, ".") == 0 || strcmp(dir_file->d_name, "..") == 0)
            continue;

        ptr = strrchr(dir_file->d_name, '.');
        if (ptr == NULL)
            continue;
        for (i = 0; i < SUPPORT_FILE_SUFFIX_TYPE_NUM; i++) {
            if (strcmp(ptr, file_suffix[i]) == 0) {
                break;
            }
        }
        if (i == SUPPORT_FILE_SUFFIX_TYPE_NUM)
            continue;

        logd("name: %s", dir_file->d_name);

        file_path_len = 0;
        file_path_len += strlen(path);
        file_path_len += 1; // '/'
        file_path_len += strlen(dir_file->d_name);
        logd("file_path_len:%d\n",file_path_len);
        if (file_path_len > PLAYER_DEMO_FILE_PATH_MAX_LEN-1) {
            loge("%s too long \n",dir_file->d_name);
            continue;
        }
        files->file_path[files->file_num] = (char *)mpp_alloc(file_path_len+1);
        files->file_path[files->file_num][file_path_len] = '\0';
        strcpy(files->file_path[files->file_num], path);
        strcat(files->file_path[files->file_num], "/");
        strcat(files->file_path[files->file_num], dir_file->d_name);
        logd("i: %d, filename: %s", files->file_num, files->file_path[files->file_num]);
        files->file_num ++;
        if (files->file_num >= PLAYER_DEMO_FILE_MAX_NUM)
            break;
    }
    closedir(dir);
    return 0;
}

void player_demo_stop(void)
{
    if (g_video_player_ctx)
        g_video_player_ctx->player_stop = 1;
}

s32 event_handle(void* app_data,s32 event,s32 data1,s32 data2)
{
    int ret = 0;
    struct video_player_ctx *ctx = (struct video_player_ctx *)app_data;
    switch(event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            ctx->player_end = 1;
            logd("g_player_end\n");
            break;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        default:
            break;
    }
    return ret;
}

static int set_volume(struct video_player_ctx *player_ctx,int volume)
{
    struct video_player_ctx *ctx = player_ctx;
    if (volume < 0) {
        volume = 0;
    } else if (volume < 101) {

    } else {
        volume = 100;
    }
    logd("volume:%d\n",volume);
    return aic_player_set_volum(ctx->player,volume);
}

static int do_seek(struct video_player_ctx *player_ctx,int forward)
{
    s64 pos;
    struct video_player_ctx *ctx = player_ctx;
    pos = aic_player_get_play_time(ctx->player);
    if (pos == -1) {
        loge("aic_player_get_play_time error!!!!\n");
        return -1;
    }
    if (forward == 1) {
        pos += 8*1000*1000;//+8s
    } else {
        pos -= 8*1000*1000;//-8s
    }

    if (pos < 0) {
        pos = 0;
    } else if (pos < ctx->media_info.duration) {

    } else {
        pos = ctx->media_info.duration;
    }

    if (aic_player_seek(ctx->player,pos) != 0) {
        loge("aic_player_seek error!!!!\n");
        return -1;
    }
    logd("aic_player_seek ok\n");
    return 0;
}

static int do_rotation_common(struct video_player_ctx *player_ctx, u32 rotation)
{
    struct video_player_ctx *ctx = player_ctx;
    if (rotation > MPP_ROTATION_270) {
        loge("Invalid rotation: %d\n", rotation);
        rotation = MPP_ROTATION_0;
    }

    logd("Rotation %d\n", rotation * 90);
    aic_player_set_rotation(ctx->player, rotation);
    return 0;
}

static int do_rotation(struct video_player_ctx *player_ctx)
{
    static int index = 0;
    struct video_player_ctx *ctx = player_ctx;

    do_rotation_common(ctx, index % 4);
    index++;
    return 0;
}
#ifndef PLAYER_DEMO_USE_VE_FILL_FB
static int cal_disp_size(struct aic_video_stream *media, struct mpp_rect *disp)
{
    struct mpp_fb *fb = mpp_fb_open();
    struct aicfb_screeninfo screen = {0};
    float screen_ratio, media_ratio;

    if (!fb)
        return -1;

    if (mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, &screen)) {
        loge("get screen info failed\n");
        goto out;
    }

#if 0
    /* No scale when the resolution of media is smaller than screen */
    if (media->width <= screen.width && media->height <= screen.height) {
        disp->x = (screen.width - media->width) / 2;
        disp->y = (screen.height - media->height) / 2;
        disp->width = media->width;
        disp->height = media->height;
        goto out;
    }
#endif

    screen_ratio = (float)screen.width / (float)screen.height;
    if (media->width > 0  && media->height > 0)
        media_ratio  = (float)media->width / (float)media->height;
    else
        media_ratio  = screen_ratio;

    if (media_ratio < screen_ratio) {
        disp->y = 0;
        disp->height = screen.height;
        disp->width = (int)((float)screen.height * media_ratio) & 0xFFFE;
        disp->x = (screen.width - disp->width) / 2;
    } else {
        disp->x = 0;
        disp->width = screen.width;
        disp->height = (int)((float)screen.width / media_ratio) & 0xFFFE;
        disp->y = (screen.height - disp->height) / 2;
    }

out:
    logd("Media size %d x %d, Display offset (%d, %d) size %d x %d\n",
           media->width, media->height,
           disp->x, disp->y, disp->width, disp->height);
    mpp_fb_close(fb);
    return 0;
}
#endif

static int start_play(struct video_player_ctx *player_ctx)
{
    int ret = -1;
    static struct av_media_info media_info;
    struct video_player_ctx *ctx = player_ctx;

    ret = aic_player_start(ctx->player);
    if (ret != 0) {
        loge("aic_player_start error!!!!\n");
        return -1;
    }
    logd("[%s:%d]aic_player_start ok\n",__FUNCTION__,__LINE__);

    ret =  aic_player_get_media_info(ctx->player,&media_info);
    if (ret != 0) {
        loge("aic_player_get_media_info error!!!!\n");
        return -1;
    }
    ctx->media_info = media_info;
    logd("aic_player_get_media_info duration:"FMT_x64",file_size:"FMT_x64"\n",media_info.duration,media_info.file_size);

    logd("has_audio:%d, has_video:%d,"
        "width:%d, height:%d,\n"
        "bits_per_sample:%d, nb_channel:%d, sample_rate:%d\n"
        ,media_info.has_audio
        ,media_info.has_video
        ,media_info.video_stream.width
        ,media_info.video_stream.height
        ,media_info.audio_stream.bits_per_sample
        ,media_info.audio_stream.nb_channel
        ,media_info.audio_stream.sample_rate);

#ifndef PLAYER_DEMO_USE_VE_FILL_FB
    struct mpp_size screen_size;
    if (media_info.has_video) {
        ret = aic_player_get_screen_size(ctx->player, &screen_size);
        if (ret != 0) {
            loge("aic_player_get_screen_size error!!!!\n");
            return -1;
        }
        logd("screen_width:%d,screen_height:%d\n",screen_size.width,screen_size.height);
        if (strcmp(PRJ_CHIP,"d12x") != 0) {
            struct mpp_rect disp_rect;

            ret = cal_disp_size(&media_info.video_stream, &disp_rect);
            if (ret < 0) {
                loge("Failed to calculate the size of screen\n");
                return -1;
            }
            ret = aic_player_set_disp_rect(ctx->player, &disp_rect);//attention:disp not exceed screen_size
            if (ret != 0) {
                loge("aic_player_set_disp_rect error\n");
                return -1;
            }
            logd("aic_player_set_disp_rect  ok\n");
        }

        if (ctx->rotation)
            do_rotation_common(ctx, ctx->rotation);
    }
#endif

    if (media_info.has_audio) {
        ret = set_volume(ctx,ctx->volume);
        if (ret != 0) {
            loge("set_volume error!!!!\n");
            return -1;
        }
    }


    ret = aic_player_play(ctx->player);
    if (ret != 0) {
        loge("aic_player_play error!!!!\n");
        return -1;
    }
    logd("[%s:%d]aic_player_play ok\n",__FUNCTION__,__LINE__);
    return 0;
}

static void show_cpu_usage()
{
#ifdef LPKG_USING_CPU_USAGE
    static int index = 0;
    char data_str[64];
    float value = 0.0;

    if (index++ % 30 == 0) {
        value = cpu_load_average();
        #ifdef AIC_PRINT_FLOAT_CUSTOM
            int cpu_i;
            int cpu_frac;
            cpu_i = (int)value;
            cpu_frac = (value - cpu_i) * 100;
            snprintf(data_str, sizeof(data_str), "%d.%02d\n", cpu_i, cpu_frac);
        #else
            snprintf(data_str, sizeof(data_str), "%.2f\n", value);
        #endif
        printf("cpu_loading:%s\n",data_str);
    }
#endif
}

//#define _THREAD_TRACE_INFO_

#ifdef _THREAD_TRACE_INFO_
struct thread_trace_info {
    uint32_t enter_run_tick;
    uint32_t total_run_tick;
    char thread_name[8];
};

static struct  thread_trace_info thread_trace_infos[6];


// count the cpu usage time of each thread
static void hook_of_scheduler(struct rt_thread *from,struct rt_thread *to) {
    static int show = 0;
    static uint32_t sys_tick = 0;
    int i = 0;
    for(i=0;i<6;i++) {
        if (!strcmp(thread_trace_infos[i].thread_name,from->name)) {
            uint32_t run_tick;
            run_tick = rt_tick_get() -  thread_trace_infos[i].enter_run_tick;
            thread_trace_infos[i].total_run_tick += run_tick;
            break;
        }
    }

    for(i=0;i<6;i++) {
        if (!strcmp(thread_trace_infos[i].thread_name,to->name)) {
                thread_trace_infos[i].enter_run_tick = rt_tick_get();
            break;
        }
    }
    show++;
    if (show > 10*1000) {
        rt_kprintf("[%u:%u:%u:%u:%u:%u:%u]:%u\n"
            ,thread_trace_infos[0].total_run_tick
            ,thread_trace_infos[1].total_run_tick
            ,thread_trace_infos[2].total_run_tick
            ,thread_trace_infos[3].total_run_tick
            ,thread_trace_infos[4].total_run_tick
            ,thread_trace_infos[5].total_run_tick
            ,thread_trace_infos[5].total_run_tick+thread_trace_infos[4].total_run_tick+thread_trace_infos[3].total_run_tick+thread_trace_infos[2].total_run_tick+thread_trace_infos[1].total_run_tick+thread_trace_infos[0].total_run_tick
            ,rt_tick_get() - sys_tick);

         for(i=0;i<6;i++) {
             thread_trace_infos[i].total_run_tick = 0;
         }
         show = 0;
         sys_tick = rt_tick_get();
    }
}
#endif

static int parse_options(struct video_player_ctx *player_ctx,int cnt,char**options)
{
    int argc = cnt;
    char **argv = options;
    struct video_player_ctx *ctx = player_ctx;
    int opt;

    if (!ctx || argc == 0 || !argv) {
        loge("para error !!!");
        return -1;
    }
    //set default alpha_value loop_time  volume
    ctx->alpha_value = 0;
    ctx->loop_time = 1;
    ctx->volume = 100;
    ctx->player_end = 0;
    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:t:l:c:W:H:q:a:r:sh");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            read_file(optarg,&ctx->files);
            break;
        case 'l':
            ctx->loop_time = atoi(optarg);
            break;
        case 't':
            read_dir(optarg, &ctx->files);
            break;
        case 'a':
            ctx->alpha_value = atoi(optarg);
            break;
        case 'r':
            ctx->rotation = (atoi(optarg) % 360) / 90;
            break;
        case 's':
            ctx->seek_loop = 1;
            break;
        case 'h':
            print_help(argv[0]);
            return -1;
        default:
            break;
        }
    }
    if (ctx->files.file_num == 0) {
        print_help(argv[0]);
        return -1;
    }
    return 0;
}

static int process_command(struct video_player_ctx *player_ctx,char cmd)
{
    struct video_player_ctx *ctx = player_ctx;
    int ret = 0;
    if (cmd == 0x20) {// pause
        ret = aic_player_pause(ctx->player);
    } else if (cmd == '-') {
        ctx->volume -= 5;
        ret = set_volume(ctx,ctx->volume);
    } else if (cmd == '+') {
        ctx->volume += 5;
        ret = set_volume(ctx,ctx->volume);
    } else if (cmd == 'm') {
        ret = aic_player_set_mute(ctx->player);
    } else if (cmd == 'f') {
        ret = do_seek(ctx,1);//+8s
    } else if (cmd == 'b') {
        ret = do_seek(ctx,0);//-8s
    } else if (cmd == 'z') {
        ret = aic_player_seek(ctx->player,0);
    } else if (cmd == 'r') {
        ret = do_rotation(ctx);
    } else if (cmd == 'w') {
#ifdef AIC_MPP_PLAYER_AUDIO_RENDER_SHARE_TEST
        ret = player_audio_render_share_play();
#endif
    }
    return ret;
}

static int power_on_flag = 0;

static int player_demo_prepare_stop(struct video_player_ctx *ctx, int force_stop,
                                    int loop_time, int file_num)
{
    int enable = 0;
    /*force stop need set render delay disable*/
    if (force_stop && (ctx->loop_time > 1 || ctx->files.file_num > 1)) {
        enable = 0;
        aic_player_control(ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
        return 0;
    }

    /*check the final play file then set render delay disable*/
    if ((ctx->loop_time > 1) && (ctx->files.file_num > 1)) {
        if ((loop_time == ctx->loop_time - 1) && (file_num == ctx->files.file_num -1)) {
            enable = 0;
            aic_player_control(ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
        }
    } else if ((ctx->loop_time > 1) && (loop_time == ctx->loop_time - 1)) {
        enable = 0;
        aic_player_control(ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
    } else if ((ctx->files.file_num > 1) && (file_num == ctx->files.file_num - 1)) {
        enable = 0;
        aic_player_control(ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
    }
    return 0;
}

static int player_demo_test(int argc, char **argv)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int enable = 0;
    char ch;
    rt_device_t render_dev = RT_NULL;
    struct aicfb_alpha_config alpha_bak = {0};
    struct aicfb_alpha_config alpha = {0};
    struct aicfb_layer_data layer = {0};

    rt_device_t uart_dev = RT_NULL;
    struct video_player_ctx *ctx = NULL;

    ctx = mpp_alloc(sizeof(struct video_player_ctx));
    if (!ctx) {
        loge("mpp_alloc fail!!!");
        return -1;
    }
    memset(ctx,0x00,sizeof(struct video_player_ctx));
    g_video_player_ctx = ctx;

    if (parse_options(ctx,argc,argv)) {
        loge("parse_options fail!!!\n");
        ret = -1;
        goto _EXIT_;
    }

    render_dev = rt_device_find("aicfb");
    if (!render_dev) {
        loge("rt_device_find aicfb failed!");
        ret = -1;
        goto _EXIT_;
    }

    uart_dev = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (uart_dev == NULL) {
        loge("Failed to open %s\n", RT_CONSOLE_DEVICE_NAME);
        ret = -1;
        goto _EXIT_;
    }

    if (!power_on_flag) {
        rt_device_control(render_dev,AICFB_POWERON,0);
        power_on_flag = 1;
    }
    //stroe alpha
    alpha_bak.layer_id = AICFB_LAYER_TYPE_UI;
    rt_device_control(render_dev, AICFB_GET_ALPHA_CONFIG, &alpha_bak);
    //set alpha
    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = ctx->alpha_value;
    rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);

    // store ui layer before playing
    if (strcmp(PRJ_CHIP, "d12x") == 0) {
        layer.layer_id = AICFB_LAYER_TYPE_UI;
        if (rt_device_control(render_dev, AICFB_GET_LAYER_CONFIG, &layer) < 0) {
            loge("get ui layer config failed\n");
            ret = -1;
            goto _EXIT_;
        }
    }

#ifdef _THREAD_TRACE_INFO_
    memset(&thread_trace_infos,0x00,sizeof(struct thread_trace_info));
    for (i = 0; i < 6 ;i++) {
        snprintf(thread_trace_infos[i].thread_name,sizeof(thread_trace_infos[i].thread_name),"%s%02d","pth",i);
        logd("%s\n",thread_trace_infos[i].thread_name);
    }
    rt_scheduler_sethook(hook_of_scheduler);
#endif

    ctx->player = aic_player_create(NULL);
    if (ctx->player == NULL) {
        loge("aic_player_create fail!!!\n");
        ret = -1;
        goto _EXIT_;
    }

#ifdef PLAYER_DEMO_USE_VE_FILL_FB
    if (player_vdec_share_frame_init(ctx->player) != 0) {
        loge("player_vdec_share_frame_init fail!!!\n");
        goto _EXIT_;
    }
#endif
    aic_player_set_event_callback(ctx->player,ctx,event_handle);


    if (ctx->loop_time > 1 || ctx->files.file_num > 1) {
        enable = 1;
        aic_player_control(ctx->player, AIC_PLAYER_CMD_SET_VIDEO_RENDER_KEEP_LAST_FRAME, &enable);
    }

    for(i = 0;i < ctx->loop_time; i++) {
        for(j = 0; j < ctx->files.file_num; j++) {
            logd("loop:%d,index:%d,path:%s\n",i,j,ctx->files.file_path[j]);
            ctx->player_end = 0;
            if (aic_player_set_uri(ctx->player,ctx->files.file_path[j])) {
                loge("aic_player_prepare error!!!!\n");
                player_demo_prepare_stop(ctx, 0, i, j);
                aic_player_stop(ctx->player);
                ret = -1;
                continue;
            }

            if (aic_player_prepare_sync(ctx->player)) {
                player_demo_prepare_stop(ctx, 0, i, j);
                loge("aic_player_prepare error!!!!\n");
                aic_player_stop(ctx->player);
                ret = -1;
                continue;
            }

            if (start_play(ctx) != 0) {
                loge("start_play error!!!!\n");
                player_demo_prepare_stop(ctx, 0, i, j);
                aic_player_stop(ctx->player);
                ret = -1;
                continue;
            }

            while(1)
            {
                if (ctx->player_stop == 1) {
                    logd("Stop player!\n");
                    player_demo_prepare_stop(ctx, 0, i, j);
                    aic_player_stop(ctx->player);
                    goto _EXIT_;
                }
                if (ctx->player_end == 1) {
                    logd("play file:%s end!!!!\n",ctx->files.file_path[j]);
                    if(ctx->seek_loop == 1) {
                        aic_player_seek(ctx->player,0);
                        ctx->player_end = 0;
                    } else {
                        player_demo_prepare_stop(ctx, 0, i, j);
                        aic_player_stop(ctx->player);
                        ctx->player_end = 0;
                        break;
                    }
                }
                if (rt_device_read(uart_dev, -1, &ch, 1) == 1) {
                    if (ch == 'u') {// up
                        j -= 2;
                        j = (j < -1)?(-1):(j);
                        aic_player_stop(ctx->player);
                        break;
                    } else if (ch == 'd') {// down
                        player_demo_prepare_stop(ctx, 0, i, j);
                        aic_player_stop(ctx->player);
                        break;
                    } else if (ch == 'e') {// end
                        player_demo_prepare_stop(ctx, 1, i, j);
                        aic_player_stop(ctx->player);
                        goto _EXIT_;
                    }
                    process_command(ctx,ch);
                } else {
                    show_cpu_usage();
                    usleep(100*1000);
                }
            }
        }
    }

_EXIT_:
#ifdef PLAYER_DEMO_USE_VE_FILL_FB
    player_vdec_share_frame_deinit();
#endif
    if (ctx->player) {
        aic_player_destroy(ctx->player);
        ctx->player = NULL;
    }

    for(i = 0; i <ctx->files.file_num ;i++) {
        if (ctx->files.file_path[i]) {
            mpp_free(ctx->files.file_path[i]);
            ctx->files.file_path[i] = NULL;
        }
    }

    if (strcmp(PRJ_CHIP, "d12x") == 0 && render_dev) {
        // restore ui layer after playing
        rt_device_control(render_dev,AICFB_UPDATE_LAYER_CONFIG,&layer);
    }

    if (render_dev) {
        //restore alpha
        rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha_bak);
    }

    mpp_free(ctx);
    g_video_player_ctx = ctx = NULL;
    logd("player_demo exit\n");
    return ret;
}

MSH_CMD_EXPORT_ALIAS(player_demo_test, player_demo, player demo);
