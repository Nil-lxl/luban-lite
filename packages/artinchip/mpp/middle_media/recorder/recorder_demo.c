/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: recorder demo
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

#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#define RECORDER_DEMO_DEFAULT_RECORD_TIME 0x7FFFFFFF

/*You can only choose thread trace or cpu trace*/
#define RECORD_THREAD_TRACE_INFO
//#define RECORD_CPU_TRACE_INFO


static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: recoder_demo [options]:\n"
        "\t-i                             video input source\n"
        "\t-t                             recoder time(s)\n"
        "\t-c                             recoder config file\n"
        "\t-h                             help\n\n"
        "Example1: recoder_demo -i dvp -t 60 -c /sdcard/recorder.config\n"
        "Example2: recoder_demo -i file -t 60 -c /sdcard/recorder.config\n");
}

static void print_cmd_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: recoder_demo_cmd [options]:\n"
        "\tstop                             stop recorder\n"
        "\tsnap                             snap one frame\n"
        "\tdebug                            get debug info\n"
        "\t                                 help\n\n");
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

struct recorder_context {
    struct aic_recorder *recorder;
    enum aic_recorder_vin_type  vin_source_type;
    char config_file_path[256];
    char video_in_file_path[256];
    char audio_in_file_path[256];
    char output_file_path[256];
    char capture_file_path[256];
    struct aic_recorder_config config;
    unsigned int record_time;
};


static int g_recorder_flag = 0;
static struct recorder_context *g_recorder_cxt = NULL;

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
        g_recorder_flag = 1;
        break;
    case AIC_RECORDER_EVENT_NO_SPACE:
        break;

    default:
        break;
    }
    return ret;
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
        recorder_cxt->config.file_duration = cjson->valueint * 1000;
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
            g_recorder_flag = 1;
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
        opt = getopt(argc, argv, "i:c:t:h");
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
#if defined(LPKG_USING_CPU_USAGE) && defined(RECORD_CPU_TRACE_INFO)
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

#ifdef RECORD_THREAD_TRACE_INFO
struct thread_trace_info {
    uint32_t enter_run_tick;
    uint32_t total_run_tick;
    char thread_name[8];
};
static uint32_t sys_tick = 0;
static struct  thread_trace_info thread_trace_infos[6];

void print_thread_status()
{
    printf("\n");
    rt_kprintf("thread\t%10s\t%10s\t%10s\t%10s\t%10s\t%10s\t%10s\t%10s\n"
        ,thread_trace_infos[0].thread_name
        ,thread_trace_infos[1].thread_name
        ,thread_trace_infos[2].thread_name
        ,thread_trace_infos[3].thread_name
        ,thread_trace_infos[4].thread_name
        ,thread_trace_infos[5].thread_name
        ,"thread0-5", "Total");
    rt_kprintf("tick\t%10u\t%10u\t%10u\t%10u\t%10u\t%10u\t%10u\t%10u\n"
        ,thread_trace_infos[0].total_run_tick
        ,thread_trace_infos[1].total_run_tick
        ,thread_trace_infos[2].total_run_tick
        ,thread_trace_infos[3].total_run_tick
        ,thread_trace_infos[4].total_run_tick
        ,thread_trace_infos[5].total_run_tick
        ,thread_trace_infos[5].total_run_tick+
        thread_trace_infos[4].total_run_tick+
        thread_trace_infos[3].total_run_tick+
        thread_trace_infos[2].total_run_tick+
        thread_trace_infos[1].total_run_tick+
        thread_trace_infos[0].total_run_tick
        ,rt_tick_get() - sys_tick);
}

// count the cpu usage time of each thread
static void hook_of_scheduler(struct rt_thread *from,struct rt_thread *to) {
    static int show = 0;

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
         print_thread_status();
         for(i=0;i<6;i++) {
             thread_trace_infos[i].total_run_tick = 0;
         }
         show = 0;
         sys_tick = rt_tick_get();
    }
}

#endif

static void *test_recorder_thread(void *arg)
{
    struct recorder_context *recorder_cxt = (struct recorder_context *)arg;
    struct timespec start_time = { 0 }, cur_time = { 0 };

    clock_gettime(CLOCK_REALTIME, &start_time);

    while (!g_recorder_flag) {
        clock_gettime(CLOCK_REALTIME, &cur_time);
        if (recorder_cxt->record_time <= (cur_time.tv_sec - start_time.tv_sec)) {
            g_recorder_flag = 1;
            break;
        }
        show_cpu_usage();
        usleep(100 * 1000);
    }

    if (recorder_cxt && recorder_cxt->recorder) {
        aic_recorder_stop(recorder_cxt->recorder);
        aic_recorder_destroy(recorder_cxt->recorder);
        recorder_cxt->recorder = NULL;
    }

    if (recorder_cxt) {
        free(recorder_cxt);
        recorder_cxt = NULL;
    }

    printf("test_recorder_thread exit\n");

    return NULL;
}


int recorder_demo_test(int argc, char *argv[])
{
    int ret = 0;
    pthread_attr_t attr;
    pthread_t thread_id;
    struct recorder_context *recorder_cxt = NULL;
    g_recorder_flag = 0;

    recorder_cxt = malloc(sizeof(struct recorder_context));
    if (!recorder_cxt) {
        loge("malloc error");
        return -1;
    }
    memset(recorder_cxt, 0x00, sizeof(struct recorder_context));
    recorder_cxt->record_time = RECORDER_DEMO_DEFAULT_RECORD_TIME;
    if (parse_options(recorder_cxt, argc, argv)) {
        goto _EXIT;
    }
    g_recorder_cxt = recorder_cxt;
    if (parse_config_file(recorder_cxt->config_file_path, recorder_cxt)) {
        loge("parse_config_file %s error", recorder_cxt->config_file_path);
        goto _EXIT;
    }

#ifdef RECORD_THREAD_TRACE_INFO
    memset(&thread_trace_infos, 0x00, sizeof(struct thread_trace_info));
    for (int i = 0; i < 6; i++) {
        snprintf(thread_trace_infos[i].thread_name, sizeof(thread_trace_infos[i].thread_name),
                 "%s%02d", "pth", i);
        printf("%s\n", thread_trace_infos[i].thread_name);
    }
    rt_scheduler_sethook(hook_of_scheduler);
#endif

    recorder_cxt->recorder = aic_recorder_create();
    if (!recorder_cxt->recorder) {
        loge("aic_recorder_create error");
        goto _EXIT;
    }

    if (aic_recorder_set_event_callback(recorder_cxt->recorder,
                                        recorder_cxt, event_handle)) {
        loge("aic_recorder_set_event_callback error");
        goto _EXIT;
    }

    if (aic_recorder_init(recorder_cxt->recorder, &recorder_cxt->config)) {
        loge("aic_recorder_init error");
        goto _EXIT;
    }

    aic_recorder_set_vin_type(recorder_cxt->recorder, recorder_cxt->vin_source_type);
    aic_recorder_set_input_file_path(recorder_cxt->recorder, recorder_cxt->video_in_file_path, NULL);

    if (aic_recorder_start(recorder_cxt->recorder)) {
        loge("aic_recorder_start error");
        goto _EXIT;
    }
    pthread_attr_init(&attr);
    attr.stacksize = 2 * 1024;
    attr.schedparam.sched_priority = 30;

    ret = pthread_create(&thread_id, &attr, test_recorder_thread, recorder_cxt);
    if (ret) {
        loge("create test_recorder_thread failed\n");
    }

    return ret;

_EXIT:

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

int recorder_demo_cmd(int argc, char *argv[])
{
    int ret = 0;
    static int capture_count = 0;

    struct recorder_context *recorder_cxt = g_recorder_cxt;
    if (argc < 1) {
        print_cmd_help(argv[0]);
        return -1;
    }
    if (strcmp(argv[1], "stop") == 0) {
        g_recorder_flag = 1;
    } else if (strcmp(argv[1], "snap") == 0) {
        struct aic_record_snapshot_info snap_info;
        snprintf(recorder_cxt->capture_file_path, sizeof(recorder_cxt->capture_file_path),
                "/sdcard/capture-%d.jpg", capture_count++);
        snap_info.file_path = (s8 *)recorder_cxt->capture_file_path;
        aic_recorder_snapshot(recorder_cxt->recorder, &snap_info);
    } else if (strcmp(argv[1], "debug") == 0) {
        aic_recorder_print_debug_info(recorder_cxt->recorder);
    } else {
        print_cmd_help(argv[0]);
    }

    return ret;
}
MSH_CMD_EXPORT_ALIAS(recorder_demo_cmd, recorder_demo_cmd, recorder demo cmd);
