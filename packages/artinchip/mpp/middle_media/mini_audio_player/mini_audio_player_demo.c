/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: audio_player_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <shell.h>
#include <artinchip_fb.h>
#include "mini_audio_player.h"
#include "aic_audio_render_manager.h"
#include "aic_parser.h"
#include "mpp_log.h"

#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#define AUDIO_PLAYER_FILE_MAX_NUM 128
#define AUDIO_PLAYER_FILE_PATH_MAX_LEN 256

unsigned char g_file_path[AUDIO_PLAYER_FILE_PATH_MAX_LEN] = {0};
#define MINI_AUDIO_PLAYER_WAVE_BUFF_SIZE (8*1024)

static void audio_play(struct aic_parser *parser, struct aic_audio_render *render)
{
    int ret;
    struct aic_parser_packet packet;
    int wav_buff_size = MINI_AUDIO_PLAYER_WAVE_BUFF_SIZE;
    unsigned char *wav_buff = aicos_malloc(MEM_DEFAULT, wav_buff_size);
    if (wav_buff == NULL) {
        loge("aicos_malloc error\n");
        goto out;
    }

    while (1) {
        memset(&packet, 0, sizeof(struct aic_parser_packet));
        ret = aic_parser_peek(parser, &packet);
        if (ret == PARSER_EOS) {
            break;
        }
        if (packet.size > wav_buff_size) {
            loge("pkt size[%d] larger than wav_buf_size[%d]\n", packet.size, wav_buff_size);
            break;
        }
        packet.data = wav_buff;
        aic_parser_read(parser, &packet);
        aic_audio_render_rend(render, packet.data, packet.size);
        usleep(1000);
    }

out:
    if (wav_buff)
        free(wav_buff);
}

static int set_render_param(struct aic_parser *parser, struct aic_audio_render *render)
{
    int value;
    struct aic_audio_render_attr attr;
    struct aic_parser_av_media_info media_info;
    if (aic_parser_get_media_info(parser, &media_info)) {
        loge("aic_parser_get_media_info fail\n");
        return -1;
    }

    value = AUDIO_RENDER_SCENE_HIGHEST_PRIORITY;
    if (aic_audio_render_control(render,
                                 AUDIO_RENDER_CMD_SET_SCENE_PRIORITY,
                                 &value)) {
        loge("aic_audio_render_control set priority failed\n");
        return -1;
    }

    if (aic_audio_render_control(render,
                                 AUDIO_RENDER_CMD_CLEAR_CACHE,
                                 NULL)) {
        loge("aic_audio_render_control clear cache failed\n");
        return -1;
    }

    attr.bits_per_sample = media_info.audio_stream.bits_per_sample;
    attr.channels = media_info.audio_stream.nb_channel;
    attr.sample_rate = media_info.audio_stream.sample_rate;
    /*Set warnning tone audio params*/
    if (aic_audio_render_control(render,
                                AUDIO_RENDER_CMD_SET_ATTR,
                                &attr)) {
        loge("aic_audio_render_control set new attribute failed\n");
        return -1;
    }

    return 0;
}

static int play_wav(unsigned char *file_path)
{
    s32 ret = 0;
    struct aic_audio_render_attr ao_attr;
    struct aic_audio_render *render = NULL;
    struct audio_render_create_params create_params;
    struct aic_parser *parser = NULL;

    aic_parser_create(file_path, &parser);
    if (parser == NULL) {
        loge("parser create failed");
        goto exit;
    }
    if (aic_parser_init(parser)) {
        loge("aic_parser_init fail\n");
        goto exit;
    }

    create_params.dev_id = 0;
    create_params.scene_type = AUDIO_RENDER_SCENE_WARNING_TONE;
    if (aic_audio_render_create(&render, &create_params)) {
        loge("aic_audio_render_create failed\n");
        goto exit;
    }

    if (aic_audio_render_init(render)) {
        loge("aic_audio_render_init failed\n");
        ret = -1;
        goto exit;
    }

    if (aic_audio_render_control(render,
                                AUDIO_RENDER_CMD_GET_ATTR,
                                &ao_attr)) {
        loge("aic_audio_render_control set new attribute failed\n");
        return -1;
    }

    set_render_param(parser, render);

    audio_play(parser, render);

exit:
    /*Play warnning tone over, change to the last audio attribute*/
    if (aic_audio_render_control(render,
                                AUDIO_RENDER_CMD_SET_ATTR,
                                &ao_attr))  {
        loge("aic_audio_render_control set the last attribute failed\n");
    }

    if (render)
        aic_audio_render_destroy(render);
    if (parser)
        aic_parser_destroy(parser);

    return ret;
}

struct audio_file_list {
    char *file_path[AUDIO_PLAYER_FILE_MAX_NUM];
    int file_num;
};

static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Compile time: %s\n", __TIME__);
    printf("Usage: player_demo [options]:\n"
        "\t-i                             input stream file name\n"
        "\t-t                             directory of test files\n"
        "\t-l                             loop time\n"
        "\t-f                             set loop flag\n"
        "\t-w                             set wav file\n"
        "\t-h                             help\n\n"
        "---------------------------------------------------------------------------------------\n"
        "-------------------------------control key while playing-------------------------------\n"
        "---------------------------------------------------------------------------------------\n"
        "('s'): stop current \n"
        "('p'): pause \n"
        "('r'): resume \n"
        "('+'): volum+5 \n"
        "('-'): volum-5 \n"
        "('e'): eixt app \n");
}

static int read_dir(char* path, struct audio_file_list *files)
{
    char* ptr = NULL;
    int file_path_len = 0;
    struct dirent* dir_file;
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("read dir failed");
        return -1;
    }

    while((dir_file = readdir(dir))) {
        if (strcmp(dir_file->d_name, ".") == 0 || strcmp(dir_file->d_name, "..") == 0)
            continue;

        ptr = strrchr(dir_file->d_name, '.');
        if (ptr == NULL)
            continue;

        if (strcmp(ptr, ".mp3") && strcmp(ptr, ".wav") &&
            strcmp(ptr, ".flac") && strcmp(ptr, ".wma"))
            continue;

        printf("name: %s\n", dir_file->d_name);

        file_path_len = 0;
        file_path_len += strlen(path);
        file_path_len += 1; // '/'
        file_path_len += strlen(dir_file->d_name);
        printf("file_path_len:%d\n",file_path_len);
        if (file_path_len > AUDIO_PLAYER_FILE_PATH_MAX_LEN-1) {
            printf("%s too long \n",dir_file->d_name);
            continue;
        }
        files->file_path[files->file_num] = (char *)aicos_malloc(MEM_DEFAULT,file_path_len+1);
        files->file_path[files->file_num][file_path_len] = '\0';
        strcpy(files->file_path[files->file_num], path);
        strcat(files->file_path[files->file_num], "/");
        strcat(files->file_path[files->file_num], dir_file->d_name);
        printf("i: %d, filename: %s\n", files->file_num, files->file_path[files->file_num]);
        files->file_num ++;
        if (files->file_num >= AUDIO_PLAYER_FILE_MAX_NUM)
            break;
    }
    closedir(dir);
    return 0;
}

static int read_file(char* path, struct audio_file_list *files)
{
    int file_path_len;
    file_path_len = strlen(path);
    printf("file_path_len:%d\n",file_path_len);
    if (file_path_len > AUDIO_PLAYER_FILE_PATH_MAX_LEN-1) {
        printf("file_path_len too long \n");
        return -1;
    }
    files->file_path[0] = (char *)aicos_malloc(MEM_DEFAULT,file_path_len+1);
    files->file_path[0][file_path_len] = '\0';
    strcpy(files->file_path[0], path);
    files->file_num = 1;
    printf("file path: %s\n", files->file_path[0]);
    return 0;
}

static int volume(int* vol,char ch)
{
    int volume = *vol;
    if (ch == '+') {
        volume += 5;
    } else {
        volume -= 5;
    }

    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
         volume = 100;
    }
    *vol = volume;
    printf("volum:%d\n",volume);
    return volume;
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

static void audio_player_demo(int argc, char **argv)
{
    int i,j;
    char ch;
    int vol = 80;
    int opt;
    int loop_time = 1;
    int loop_flag = 0;
    rt_device_t uart_dev = RT_NULL;
    struct mini_audio_player *player = NULL;
    struct audio_file_list  files = {0};
    char state_name[3][8] = {"stop", "playing", "pause"};
    int len = 0;

    uart_dev = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (uart_dev == NULL) {
        printf("Failed to open %s\n", RT_CONSOLE_DEVICE_NAME);
        return;
    }

    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:t:l:w:fh");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            read_file(optarg, &files);
            break;
        case 'l':
            loop_time = atoi(optarg);
            break;
        case 'f':
            loop_flag = 1;
            break;
        case 'w':
            len = strlen(optarg);
            if (len >= AUDIO_PLAYER_FILE_PATH_MAX_LEN) {
                loge("wav file path > 256");
                return;
            }

            strncpy((char*)g_file_path, optarg, AUDIO_PLAYER_FILE_PATH_MAX_LEN);
        case 't':
            read_dir(optarg, &files);
            break;
        case 'h':
            print_help(argv[0]);
        default:
            goto _exit;
            break;
        }
    }

    if (files.file_num == 0) {
        print_help(argv[0]);
        printf("files.file_num ==0 !!!\n");
        goto _exit;
    }

    player = mini_audio_player_create();
    if (player == NULL) {
        printf("mini_audio_player_create fail!!!\n");
        goto _exit;
    }

    mini_audio_player_set_volume(player,vol);

    for(i = 0; i < loop_time; i++) {
        for(j = 0; j < files.file_num; j++) {
            if (loop_flag)
                mini_audio_player_play_loop(player,files.file_path[j]);
            else
                mini_audio_player_play(player,files.file_path[j]);
            printf("loop:%d,index:%d,path:%s\n",i,j,files.file_path[j]);
            while(1) {
                if (rt_device_read(uart_dev, -1, &ch, 1) == 1) {
                    if (ch == 's') {// stop cur
                        time_start(mini_audio_player_stop);
                        mini_audio_player_stop(player);
                        time_end(mini_audio_player_stop);
                        break;
                    } else if (ch == '+') {// volume++
                        mini_audio_player_set_volume(player,volume(&vol,ch));
                    } else if (ch == '-') {// volume--
                        mini_audio_player_set_volume(player,volume(&vol,ch));
                    } else if (ch == 'p') {// pause
                        printf("enter pause\n");
                        mini_audio_player_pause(player);
                    } else if (ch == 'r') {// resume
                        printf("resume \n");
                        mini_audio_player_resume(player);
                    } else if (ch == 'w') {
                        printf("wav play \n");
                        play_wav(g_file_path);
                    } else if (ch == 'e') {// exit app
                        printf("exit app \n");
                        goto _exit;
                    } else if (ch == 'g') {// get_state
                        printf("get status: %s \n", state_name[mini_audio_player_get_state(player)]);
                    }
                }
                if (mini_audio_player_get_state(player) == MINI_AUDIO_PLAYER_STATE_STOPED) {
                    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
                    break;
                }
                usleep(100*1000);
                show_cpu_usage();
            }
        }
    }
_exit:
    if (player) {
        mini_audio_player_destroy(player);
        player = NULL;
    }
    for(i = 0; i <files.file_num ;i++) {
        if (files.file_path[i]) {
            aicos_free(MEM_DEFAULT, files.file_path[i]);
        }
    }
}

MSH_CMD_EXPORT_ALIAS(audio_player_demo, audio_player_demo, audio player demo);

