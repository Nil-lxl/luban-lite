/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <fcntl.h>
#include <pthread.h>
#include <rthw.h>
#include <rtthread.h>
#include <shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#define LOG_DEBUG
#include "aic_recorder.h"
#include "cJSON.h"
#include "mpp_list.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "drv_dvp.h"
#include "mpp_vin.h"
#ifdef AIC_USING_CAMERA
#include "drv_camera.h"
#endif
#include "aic_render.h"
#include "mpp_fb.h"
#include "artinchip_fb.h"
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

//#define RECORDER_DVP_CROP_MODE
#define RECORDER_VID_BUF_NUM        3
#define RECORDER_VID_BUF_PLANE_NUM  2
#define RECORDER_DEMO_DEFAULT_RECORD_TIME 0x7FFFFFFF


struct recorder_dvp_data {
    int w;
    int h;
    int frame_size;
    int frame_cnt;
    int fresh_frame;
    int rotation;
    struct mpp_rect dst_pos;        // position in DE video player

    struct mpp_video_fmt src_fmt;   // format of DVP input, i.e. camera output
    struct dvp_out_fmt   dst_fmt;   // format of DVP output
    uint32_t num_buffers;
    struct vin_video_buf binfo;
    bool dvp_init;
};

struct recorder_render_data {
    struct aic_video_render *render;
    int layer_id;
    int dev_id;
    struct mpp_rect dis_rect;
    bool render_init;
};

struct recorder_context {
    struct aic_recorder *recorder;
    enum aic_recorder_vin_type  vin_source_type;
    union {
        struct recorder_dvp_data dvp_data;
    };

    char config_file_path[256];
    char video_in_file_path[256];
    char audio_in_file_path[256];
    char output_file_path[256];
    char capture_file_path[256];
    struct aic_recorder_config config;
    unsigned int record_time;
    struct recorder_render_data render_data;
    bool render_flag;
    bool record_flag;
    bool recorder_stop;
    unsigned int snapshot_count;
    unsigned int record_count;
    int last_index;
    long long first_frame_time;
    struct mpp_frame frame[RECORDER_VID_BUF_NUM];
    struct aicfb_screeninfo fb_info;
};


static struct recorder_context *g_recorder_cxt = NULL;

static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Usage: recoder_demo [options]:\n"
        "\t-i                             video input source\n"
        "\t-t                             recoder time(s)\n"
        "\t-c                             recoder config file\n"
        "\t-d                             display to screen\n"
        "\t-r                             record data\n"
        "\t-h                             help\n\n"
        "Example1: recoder_demo -i dvp -t 60 -c /sdcard/recorder.config\n"
        "Example2: recoder_demo -i file -t 60 -c /sdcard/recorder.config\n");
}

static void print_cmd_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Usage: recoder_demo_cmd [options]:\n"
        "\t-e                             stop recorder\n"
        "\t-s                             snap one frame\n"
        "\t-d                             get debug info\n"
        "\t-h                             help\n\n");
}

char *read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL) {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0) {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0) {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char *)malloc((size_t)length + sizeof(""));
    if (content == NULL) {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length) {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if (file != NULL) {
        fclose(file);
    }

    return content;
}

static cJSON *parse_file(const char *filename)
{
    cJSON *parsed = NULL;
    char *content = read_file(filename);

    parsed = cJSON_Parse(content);

    if (content != NULL) {
        free(content);
    }

    return parsed;
}


static int dvp_queue_buf(int index);

static s32 event_handle(void *app_data, s32 event, s32 data1, s32 data2)
{
    int ret = 0;
    struct recorder_context *recorder_cxt = (struct recorder_context *)app_data;
    char file_path[512] = {0};
    time_t timep;
    struct tm *p = NULL;

    switch (event) {
    case AIC_RECORDER_EVENT_NEED_NEXT_FILE:
        // set recorder file name
        time(&timep);
        p = localtime(&timep);
        snprintf(file_path, sizeof(file_path), "%s%04d%02d%02d_%02d%02d%02d.mp4",
                 recorder_cxt->output_file_path,
                 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
                 p->tm_hour, 1 + p->tm_min, p->tm_sec);
        aic_recorder_set_output_file_path(recorder_cxt->recorder, file_path);
        printf("set recorder file:%s\n", file_path);
        break;
    case AIC_RECORDER_EVENT_COMPLETE:
        recorder_cxt->recorder_stop = true;
        break;
    case AIC_RECORDER_EVENT_NO_SPACE:
        break;

    default:
        break;
    }
    return ret;
}

static s32 giveback_buf(void *app_data, s32 event, void *buffer)
{
    if (!app_data || !buffer) {
        loge("app_data %p or buffer %p is null", app_data, buffer);
        return -1;
    }

    switch (event) {
        case AIC_RECORDER_EVENT_GIVEBACK_FRAME:
            dvp_queue_buf(((struct mpp_frame*)buffer)->id);
            break;

        case AIC_RECORDER_EVENT_GIVEBACK_PACKET:
            //Reserved for user send packet to recorder
            break;

        default:
            break;
    }

    return 0;
}

int parse_config_file(char *config_file, struct recorder_context *recorder_cxt)
{
    int ret = 0;
    cJSON *cjson = NULL;
    cJSON *root = NULL;
    if (!config_file || !recorder_cxt) {
        ret = -1;
        goto _EXIT;
    }
    root = parse_file(config_file);
    if (!root) {
        loge("parse_file error %s!!!", config_file);
        ret = -1;
        goto _EXIT;
    }

    cjson = cJSON_GetObjectItem(root, "video_in_file");
    if (!cjson) {
        loge("no video_in_file error");
        ret = -1;
        goto _EXIT;
    }
    strcpy(recorder_cxt->video_in_file_path, cjson->valuestring);

    cjson = cJSON_GetObjectItem(root, "audio_in_file");
    if (!cjson) {
        strcpy(recorder_cxt->audio_in_file_path, cjson->valuestring);
    }

    cjson = cJSON_GetObjectItem(root, "output_file");
    if (!cjson) {
        loge("no output_file error");
        ret = -1;
        goto _EXIT;
    }
    strcpy(recorder_cxt->output_file_path, cjson->valuestring);

    cjson = cJSON_GetObjectItem(root, "file_duration");
    if (cjson) {
        recorder_cxt->config.file_duration = cjson->valueint;
    }

    cjson = cJSON_GetObjectItem(root, "file_num");
    if (cjson) {
        recorder_cxt->config.file_num = cjson->valueint;
    }

    cjson = cJSON_GetObjectItem(root, "qfactor");
    if (cjson) {
        recorder_cxt->config.qfactor = cjson->valueint;
    }

    cjson = cJSON_GetObjectItem(root, "video");
    if (cjson) {
        int enable = cJSON_GetObjectItem(cjson, "enable")->valueint;
        if (enable == 1) {
            recorder_cxt->config.has_video = 1;
        }
        recorder_cxt->config.video_config.codec_type =
            cJSON_GetObjectItem(cjson, "codec_type")->valueint;

        printf("codec_type:0x%x\n", recorder_cxt->config.video_config.codec_type);
        if (recorder_cxt->config.video_config.codec_type != MPP_CODEC_VIDEO_DECODER_MJPEG) {
            ret = -1;
            loge("only support  MPP_CODEC_VIDEO_DECODER_MJPEG");
            recorder_cxt->recorder_stop = true;
            goto _EXIT;
        }
        recorder_cxt->config.video_config.out_width =
            cJSON_GetObjectItem(cjson, "out_width")->valueint;
        recorder_cxt->config.video_config.out_height =
            cJSON_GetObjectItem(cjson, "out_height")->valueint;
        recorder_cxt->config.video_config.out_frame_rate =
            cJSON_GetObjectItem(cjson, "out_framerate")->valueint;
        recorder_cxt->config.video_config.out_bit_rate =
            cJSON_GetObjectItem(cjson, "out_bitrate")->valueint;
        recorder_cxt->config.video_config.in_width =
            cJSON_GetObjectItem(cjson, "in_width")->valueint;
        recorder_cxt->config.video_config.in_height =
            cJSON_GetObjectItem(cjson, "in_height")->valueint;
        recorder_cxt->config.video_config.in_pix_fomat =
            cJSON_GetObjectItem(cjson, "in_pix_format")->valueint;
    }

    cjson = cJSON_GetObjectItem(root, "audio");
    if (cjson) {
        int enable = cJSON_GetObjectItem(cjson, "enable")->valueint;
        if (enable == 1) {
            recorder_cxt->config.has_audio = 1;
        }
        recorder_cxt->config.audio_config.codec_type =
            cJSON_GetObjectItem(cjson, "codec_type")->valueint;
        recorder_cxt->config.audio_config.out_bitrate =
            cJSON_GetObjectItem(cjson, "out_bitrate")->valueint;
        recorder_cxt->config.audio_config.out_samplerate =
            cJSON_GetObjectItem(cjson, "out_samplerate")->valueint;
        recorder_cxt->config.audio_config.out_channels =
            cJSON_GetObjectItem(cjson, "out_channels")->valueint;
        recorder_cxt->config.audio_config.out_bits_per_sample =
            cJSON_GetObjectItem(cjson, "out_bits_per_sample")->valueint;
        recorder_cxt->config.audio_config.in_samplerate =
            cJSON_GetObjectItem(cjson, "in_samplerate")->valueint;
        recorder_cxt->config.audio_config.in_channels =
            cJSON_GetObjectItem(cjson, "in_channels")->valueint;
        recorder_cxt->config.audio_config.in_bits_per_sample =
            cJSON_GetObjectItem(cjson, "in_bits_per_sample")->valueint;
    }
_EXIT:
    if (root) {
        cJSON_Delete(root);
    }
    return ret;
}


static int parse_options(struct recorder_context *recoder_ctx, int cnt, char **options)
{
    int argc = cnt;
    char **argv = options;
    struct recorder_context *ctx = recoder_ctx;
    int opt;

    if (!ctx || argc == 0 || !argv) {
        loge("para error !!!");
        return -1;
    }
    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:c:t:r:dh");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            if (strcmp(optarg, "dvp") == 0) {
                ctx->vin_source_type = AIC_RECORDER_VIN_DVP;
            } else {
                ctx->vin_source_type = AIC_RECORDER_VIN_FILE;
            }
            break;

        case 'c':
            strcpy(ctx->config_file_path, optarg);
            break;

        case 't':
            ctx->record_time = atoi(optarg);
            break;

        case 'r':
            ctx->record_flag = atoi(optarg);
            break;

        case 'd':
            ctx->render_flag = true;
            break;

        case 'h':
        default:
            print_help(argv[0]);
            return -1;
        }
    }

    return 0;
}

static void show_cpu_usage()
{
#if defined(LPKG_USING_CPU_USAGE)
    static int index = 0;
    char data_str[64];
    float value = 0.0;

    if (index++ % 500 == 0) {
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

static int recorder_render_init(struct recorder_render_data *render_data)
{
    struct aicfb_alpha_config alpha = {0};
    rt_device_t render_dev = NULL;
    int ret = 0;

    if (!render_data) {
        loge("render_data is null");
        return -1;
    }

    // set fb power on
    render_dev = rt_device_find("aicfb");
    if (!render_dev) {
        loge("rt_device_find aicfb failed!");
        return -1;
    }
    rt_device_control(render_dev, AICFB_POWERON, 0);

    //set ui layer alpha
    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    alpha.enable = 1;
    alpha.mode = 1;
    alpha.value = 0;
    rt_device_control(render_dev, AICFB_UPDATE_ALPHA_CONFIG, &alpha);

    // create video render instance
    ret = aic_video_render_create(&render_data->render);
    if (ret) {
        loge("create render failed %d.", ret);
        return ret;
    }
    render_data->layer_id = AICFB_LAYER_TYPE_VIDEO;
    render_data->dev_id = 0;
    ret = aic_video_render_init(render_data->render,
                                render_data->layer_id,
                                render_data->dev_id);
    if (ret) {
        loge("init render failed %d.", ret);
        aic_video_render_destroy(render_data->render);
        return ret;
    }

    render_data->render_init = true;
    return 0;
}

static void recorder_render_deinit(struct recorder_render_data *render_data)
{
    if (!render_data) {
        loge("render not init");
        return;
    }

    if (render_data->render && render_data->render_init) {
        aic_video_render_set_on_off(render_data->render, 0);
        aic_video_render_destroy(render_data->render);
        render_data->render = NULL;
    }
    render_data->render_init = false;
}

#ifdef RECORDER_DVP_CROP_MODE
static int recorder_fb_get_info(struct aicfb_screeninfo *fb_info)
{
    struct mpp_fb *fb = NULL;
    int ret = 0;

    fb = mpp_fb_open();
    if (!fb) {
        loge("Failed to open FB\n");
        return -1;
    }

    ret = mpp_fb_ioctl(fb, AICFB_GET_SCREENINFO, fb_info);
    if (ret < 0)
        loge("Failed to get screen info! errno: -%d\n", -ret);

    logi("Screen width: %d, height %d\n", fb_info->width, fb_info->height);

    mpp_fb_close(fb);

    return 0;
}
#endif

static int recorder_dvp_init(struct recorder_dvp_data *dvp_data,
                             struct aicfb_screeninfo *fb_info)
{
    struct mpp_video_fmt *src;
    struct dvp_out_fmt *dst;
    int ret = -1;
    int i = 0;

    if (!dvp_data || !fb_info) {
        loge("dvp_data or fb is null");
        return -1;
    }

    src = &dvp_data->src_fmt;
    dst = &dvp_data->dst_fmt;

#ifdef AIC_USING_CAMERA
    /*Initial vin*/
    if (mpp_vin_init(CAMERA_DEV_NAME))
        return -1;
#endif

    /*get sensor fmt*/
    ret = mpp_dvp_ioctl(DVP_IN_G_FMT, src);
    if (ret < 0) {
        loge("ioctl(DVP_IN_G_FMT) failed! err -%d\n", -ret);
        goto _exit;
    }

    ret = mpp_dvp_ioctl(DVP_IN_S_FMT, src);
    if (ret < 0) {
        loge("ioctl(DVP_IN_S_FMT) failed! err -%d\n", -ret);
        goto _exit;
    }

    /*dvp config*/
#ifdef RECORDER_DVP_CROP_MODE
    ret = recorder_fb_get_info(fb_info);
    if (ret < 0) {
        loge("ioctl(DVP_IN_S_FMT) failed! err -%d\n", -ret);
        goto _exit;
    }
    if (src->width > fb_info->width) {
        dst->width = fb_info->width;
        dst->crop_x = (src->width - fb_info->width) / 2;
    } else {
        dst->width = src->width;
    }

    if (src->height > fb_info->height) {
        dst->height = fb_info->height;
        dst->crop_y = (src->height - fb_info->height) / 2;
    } else {
        dst->height = src->height;
    }
#else
    dvp_data->w = src->width;
    dvp_data->h = src->height;
    dst->width = src->width;
    dst->height = src->height;
#endif
    dst->pixelformat = MPP_FMT_NV12;
    dst->num_planes = RECORDER_VID_BUF_PLANE_NUM;
    dst->frame_offset = 0;
    ret = mpp_dvp_ioctl(DVP_OUT_S_FMT, dst);
    if (ret < 0) {
        loge("ioctl(DVP_OUT_S_FMT) failed! err -%d\n", -ret);
        goto _exit;
    }

    /*request dvp buffer*/
    if (mpp_dvp_ioctl(DVP_REQ_BUF, (void *)&dvp_data->binfo) < 0) {
        loge("ioctl(DVP_REQ_BUF) failed!\n");
        goto _exit;
    }
    for (i = 0; i < RECORDER_VID_BUF_NUM; i++) {
        if (dvp_queue_buf(i) < 0) {
            loge("dvp_queue_buf %d failed!\n", i);
            goto _exit;
        }
    }

    /*start dvp*/
    ret = mpp_dvp_ioctl(DVP_STREAM_ON, NULL);
    if (ret < 0) {
        loge("ioctl(DVP_STREAM_ON) failed! err -%d\n", -ret);
        goto _exit;
    }

    logi("dvp:src: w(%d), h(%d); dst(%d): w(%d), h(%d)\n",
        src->width, src->height, dst->pixelformat, dst->width, dst->height);

    dvp_data->dvp_init = true;

    return 0;

_exit:
    mpp_vin_deinit();

    dvp_data->dvp_init = false;
    return -1;
}

static void recorder_dvp_deinit(struct recorder_dvp_data *dvp_data)
{
    int ret = 0;
    if (!dvp_data || !dvp_data->dvp_init)
        return;

    ret = mpp_dvp_ioctl(DVP_STREAM_OFF, NULL);
    if (ret < 0) {
        loge("ioctl(DVP_STREAM_OFF) failed! err -%d\n", -ret);
        return;
    }

    mpp_vin_deinit();

    dvp_data->dvp_init = false;
}

static int dvp_queue_buf(int index)
{
    if (mpp_dvp_ioctl(DVP_Q_BUF, (void *)(ptr_t)index) < 0) {
        loge("Q failed! Maybe buf state is invalid.\n");
        return -1;
    }

    return 0;
}

static int dvp_dequeue_buf(int *index)
{
    int ret = 0;

    if (index == NULL)
        return -1;

    ret = mpp_dvp_ioctl(DVP_DQ_BUF, (void *)index);
    if (ret < 0) {
        logd("ioctl(DVP_DQ_BUF) failed! err -%d\n", -ret);
        return -1;
    }
    return 0;
}

static int recorder_render_frame(struct recorder_render_data *render_data,
                                 struct mpp_frame *frame)
{
    if (!render_data || !render_data->render_init || !render_data->render)
        return -1;

    if (aic_video_render_rend(render_data->render, frame)) {
        loge("render failed.");
        return -1;
    }
    return 0;
}


static void do_recorder(struct recorder_context *recorder_cxt)
{
    struct vin_video_buf *binfo = NULL;
    struct mpp_frame *frame = NULL;
    struct dvp_out_fmt *dst = NULL;
    int index = -1;
    int ret = 0;
    int i = 0;

    if (!recorder_cxt)
        return;

#ifndef AIC_MPP_RECORDER_USING_EXTRA_FRAME
    return;
#endif

    // dequeue a frame
    ret = dvp_dequeue_buf(&index);
    if (ret != 0)
        return;

    if (recorder_cxt->record_count == 0) {
        recorder_cxt->first_frame_time = aic_dvp_get_timestamp(index);
    }

    // set frame info
    binfo = &recorder_cxt->dvp_data.binfo;
    dst = &recorder_cxt->dvp_data.dst_fmt;
    frame = &recorder_cxt->frame[index];
    frame->id = index;
    frame->pts = aic_dvp_get_timestamp(index) - recorder_cxt->first_frame_time; //ms
    frame->buf.buf_type = MPP_PHY_ADDR;
    frame->buf.size.width = dst->width;
    frame->buf.size.height = dst->height;
    frame->buf.stride[0] = dst->width;
    frame->buf.stride[1] = dst->width;
    frame->buf.stride[2] = 0;
    frame->buf.format = dst->pixelformat;
    for (i = 0; i < RECORDER_VID_BUF_PLANE_NUM; i++) {
        frame->buf.phy_addr[i] =
            binfo->planes[index * RECORDER_VID_BUF_PLANE_NUM + i].buf;
    }

    // render a frame
    recorder_render_frame(&recorder_cxt->render_data, frame);

    if (recorder_cxt->record_flag) {
        // send to recorder
        ret = aic_recorder_send_frame(recorder_cxt->recorder, frame);
        if (ret != 0) {
            dvp_queue_buf(index);
            return;
        }
    } else {
        dvp_queue_buf(index);
    }
    recorder_cxt->record_count++;
}

static void *recorder_thread(void *arg)
{
    struct recorder_context *recorder_cxt = (struct recorder_context *)arg;
    struct timespec start_time = { 0 }, cur_time = { 0 };

    clock_gettime(CLOCK_REALTIME, &start_time);

    recorder_cxt->record_count = 0;
    while (!recorder_cxt->recorder_stop) {
        clock_gettime(CLOCK_REALTIME, &cur_time);
        if (recorder_cxt->record_time <= (cur_time.tv_sec - start_time.tv_sec)) {
            recorder_cxt->recorder_stop = true;
            printf("recorder end time coming, stop recorder.\n");
            break;
        }

        do_recorder(recorder_cxt);

        show_cpu_usage();
        usleep(10 * 1000);
    }

    if (recorder_cxt->vin_source_type == AIC_RECORDER_VIN_DVP)
        recorder_dvp_deinit(&recorder_cxt->dvp_data);
    if (recorder_cxt->render_flag)
        recorder_render_deinit(&recorder_cxt->render_data);

    if (recorder_cxt && recorder_cxt->recorder) {
        aic_recorder_stop(recorder_cxt->recorder);
        aic_recorder_destroy(recorder_cxt->recorder);
        recorder_cxt->recorder = NULL;
    }

    if (recorder_cxt) {
        free(recorder_cxt);
        recorder_cxt = NULL;
    }

    printf("recorder_thread exit\n");

    return NULL;
}


int recorder_demo_test(int argc, char *argv[])
{
    int ret = 0;
    pthread_attr_t attr;
    pthread_t thread_id;
    struct recorder_context *recorder_cxt = NULL;

    recorder_cxt = malloc(sizeof(struct recorder_context));
    if (!recorder_cxt) {
        loge("malloc error");
        return -1;
    }
    memset(recorder_cxt, 0x00, sizeof(struct recorder_context));
    recorder_cxt->record_flag = true;
    recorder_cxt->record_time = RECORDER_DEMO_DEFAULT_RECORD_TIME;
    if (parse_options(recorder_cxt, argc, argv)) {
        goto _EXIT;
    }
    g_recorder_cxt = recorder_cxt;
    if (parse_config_file(recorder_cxt->config_file_path, recorder_cxt)) {
        loge("parse_config_file %s error", recorder_cxt->config_file_path);
        goto _EXIT;
    }

    //create recorder module
    recorder_cxt->recorder = aic_recorder_create();
    if (!recorder_cxt->recorder) {
        loge("aic_recorder_create error");
        goto _EXIT;
    }

    //set recorder event callback
    if (aic_recorder_set_event_callback(recorder_cxt->recorder,
                                        recorder_cxt, event_handle)) {
        loge("aic_recorder_set_event_callback error");
        goto _EXIT;
    }

    //set extra frame or packet giveback callback
    if (aic_recorder_set_giveback_buf_callback(recorder_cxt->recorder,
                                               giveback_buf)) {
        loge("aic_recorder_set_giveback_buf_callback error");
        goto _EXIT;
    }

    //initialize recorder module
    recorder_cxt->config.video_config.vin_type = recorder_cxt->vin_source_type;
    if (aic_recorder_init(recorder_cxt->recorder, &recorder_cxt->config)) {
        loge("aic_recorder_init error");
        goto _EXIT;
    }

    //display frame to video layer
    if (recorder_cxt->render_flag) {
        if (recorder_render_init(&recorder_cxt->render_data))
            goto _EXIT;
    }

    //initialize video input module
    if (recorder_cxt->vin_source_type == AIC_RECORDER_VIN_DVP) {
        if (recorder_dvp_init(&recorder_cxt->dvp_data, &recorder_cxt->fb_info))
            goto _EXIT;
    } else if (recorder_cxt->vin_source_type == AIC_RECORDER_VIN_FILE) {
        aic_recorder_set_input_file_path(recorder_cxt->recorder,
            recorder_cxt->video_in_file_path, NULL);
    }

    //start recorder
    if (aic_recorder_start(recorder_cxt->recorder)) {
        loge("aic_recorder_start error");
        goto _EXIT;
    }


    //create recorder thread
    pthread_attr_init(&attr);
    attr.stacksize = 8 * 1024;
    attr.schedparam.sched_priority = 30;
    ret = pthread_create(&thread_id, &attr, recorder_thread, recorder_cxt);
    if (ret) {
        loge("create recorder_thread failed\n");
    }

    return ret;

_EXIT:
    if (recorder_cxt->vin_source_type == AIC_RECORDER_VIN_DVP)
        recorder_dvp_deinit(&recorder_cxt->dvp_data);
    if (recorder_cxt->render_flag)
        recorder_render_deinit(&recorder_cxt->render_data);

    if (recorder_cxt && recorder_cxt->recorder) {
        aic_recorder_stop(recorder_cxt->recorder);
        aic_recorder_destroy(recorder_cxt->recorder);
        recorder_cxt->recorder = NULL;
    }

    if (recorder_cxt) {
        free(recorder_cxt);
        recorder_cxt = NULL;
    }

    return ret;
}

MSH_CMD_EXPORT_ALIAS(recorder_demo_test, recorder_demo, recorder demo);

static void recorder_snapshot(struct recorder_context *recorder_cxt)
{
    struct aic_record_snapshot_info snap_info;

    if (!recorder_cxt || !recorder_cxt->recorder)
        return;

    snprintf(recorder_cxt->capture_file_path, sizeof(recorder_cxt->capture_file_path),
            "/sdcard/capture-%d.jpg", recorder_cxt->snapshot_count++);
    snap_info.file_path = (s8 *)recorder_cxt->capture_file_path;

    aic_recorder_snapshot(recorder_cxt->recorder, &snap_info);
}

static int recorder_cmd(int argc, char**argv)
{
    struct recorder_context *recorder_cxt = g_recorder_cxt;
    int opt;

    if (argc == 0 || !argv) {
        loge("para error !!!");
        return -1;
    }
    if (!recorder_cxt) {
        loge("recorder is not start running !!!");
        return -1;
    }

    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "esdh");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 's':
            recorder_snapshot(recorder_cxt);
            break;

        case 'e':
            recorder_cxt->recorder_stop = true;
            break;

        case 'd':
            aic_recorder_print_debug_info(recorder_cxt->recorder);
            break;

        case 'h':
            print_cmd_help(argv[0]);
            break;

        default:
            break;
        }
    }

    return 0;
}

MSH_CMD_EXPORT_ALIAS(recorder_cmd, recorder_cmd, recorder demo cmd);
