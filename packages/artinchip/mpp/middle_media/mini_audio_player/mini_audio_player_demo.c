/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
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

#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#define AUDIO_PLAYER_DEMO_FILE_MAX_NUM 128
#define AUDIO_PLAYER_DEMO_FILE_PATH_MAX_LEN 256

struct audio_file_list {
    char *file_path[AUDIO_PLAYER_DEMO_FILE_MAX_NUM];
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
        if (file_path_len > AUDIO_PLAYER_DEMO_FILE_PATH_MAX_LEN-1) {
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
        if (files->file_num >= AUDIO_PLAYER_DEMO_FILE_MAX_NUM)
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
    if (file_path_len > AUDIO_PLAYER_DEMO_FILE_PATH_MAX_LEN-1) {
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
    rt_device_t uart_dev = RT_NULL;
    struct mini_audio_player *player = NULL;
    struct audio_file_list  files = {0};

    uart_dev = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    if (uart_dev == NULL) {
        printf("Failed to open %s\n", RT_CONSOLE_DEVICE_NAME);
        return;
    }

    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:t:l:h");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'i':
            read_file(optarg,&files);
            break;
        case 'l':
            loop_time = atoi(optarg);
            break;
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

    for(i = 0;i < loop_time; i++) {
        for(j = 0; j < files.file_num; j++) {
            mini_audio_player_play(player,files.file_path[j]);
            printf("loop:%d,index:%d,path:%s\n",i,j,files.file_path[j]);
            while(1) {
                if (rt_device_read(uart_dev, -1, &ch, 1) == 1) {
                    if (ch == 's') {// stop cur
                        mini_audio_player_stop(player);
                        printf("force stop\n");
                        break;
                    } else if (ch == '+') {// volume++
                        mini_audio_player_set_volume(player,volume(&vol,ch));
                    } else if (ch == '-') {// volume--
                        mini_audio_player_set_volume(player,volume(&vol,ch));
                    } else if (ch == 'p') {// pause
                        mini_audio_player_pause(player);
                        printf("enter pause\n");
                    } else if (ch == 'r') {// resume
                        mini_audio_player_resume(player);
                        printf("resume \n");
                    } else if (ch == 'e') {// exit app
                        printf("exit app \n");
                        goto _exit;
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
            aicos_free(MEM_DEFAULT,files.file_path[i]);
        }
    }
}

MSH_CMD_EXPORT_ALIAS(audio_player_demo, audio_player_demo, audio player demo);

