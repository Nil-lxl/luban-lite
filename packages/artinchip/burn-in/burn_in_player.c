/*
* Copyright (C) 2020-2023 ArtInChip Technology Co. Ltd
*
*  author: <jun.ma@artinchip.com>
*  Desc: burn_in_player
*/

#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <rthw.h>
#include <rtthread.h>
#include <shell.h>
#include "msh.h"
#include "burn_in.h"

#define BURN_PLAYER_FILE_MAX_NUM 128
#define BURN_PLAYER_FILE_PATH_MAX_LEN 256

#define BURN_PLAYER_RODATA_DIR   "/rodata/aic_test/video"
#define BURN_PLAYER_DATA_DIR     "/data/aic_test/video"
#define BURN_PLAYER_SDCARD_DIR   "/sdcard/aic_test/video"

#ifdef LPKG_BURN_IN_PLAYER_LOG_TO_SERIAL
    #define BURN_IN_PLAYER_LOG_TO_SERIAL 1
#else
    #define BURN_IN_PLAYER_LOG_TO_SERIAL 0
#endif


struct file_list {
    char *file_path[BURN_PLAYER_FILE_MAX_NUM];
    int file_num;
};

struct burn_in_player_ctx {
    char *dir;
    struct file_list  files;
    FILE *log_file;
};

static int8_t dir_is_exist(const char * dir)
{
    DIR* d = opendir(dir);
    if (d) {
        closedir(d);
        return 1;
    }
    return 0;
}

static int8_t resource_is_exist(struct burn_in_player_ctx *ctx)
{
    if(dir_is_exist(BURN_PLAYER_RODATA_DIR)) {
        ctx->dir = BURN_PLAYER_RODATA_DIR;
        return 1;
    } else if (dir_is_exist(BURN_PLAYER_DATA_DIR)) {
        ctx->dir = BURN_PLAYER_DATA_DIR;
        return 1;
    } else if (dir_is_exist(BURN_PLAYER_SDCARD_DIR)) {
        ctx->dir = BURN_PLAYER_SDCARD_DIR;
        return 1;
    }
    return 0;
}

static int8_t create_log_file(struct burn_in_player_ctx *ctx)
{
    char log_file[128] = {0};
    time_t now;
    struct tm *local_time;
    now = time(RT_NULL);
    local_time = localtime(&now);
    snprintf(log_file,sizeof(log_file),"%s/%04d-%02d-%02d-%02d-%02d-%02d.log"
                    ,ctx->dir
                    ,local_time->tm_year+1900, local_time->tm_mon+1, local_time->tm_mday
                    ,local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    ctx->log_file = fopen(log_file, "wb");
    if (!ctx->log_file) {
        BURN_PRINT_ERR("fopen %s fail\n",log_file);
        return -1;
    }
    return 0;
}

static void output_log(struct burn_in_player_ctx *ctx,char *log,int res)
{
    if (BURN_IN_PLAYER_LOG_TO_SERIAL) {
        BURN_PRINT_DGB("play %s %s\n",log,((res==0)?"ok":"fail"));
    } else {
        fprintf(ctx->log_file,"play %s %s\n",log,((res==0)?"ok":"fail"));
    }
}


static int read_dir(char* path, struct file_list *files)
{
    char* ptr = NULL;
    int file_path_len = 0;
    struct dirent* dir_file;
    DIR* dir = opendir(path);
    if (dir == NULL) {
        BURN_PRINT_ERR("read dir failed \n");
        return -1;
    }

    while((dir_file = readdir(dir))) {
        if (strcmp(dir_file->d_name, ".") == 0 || strcmp(dir_file->d_name, "..") == 0)
            continue;

        ptr = strrchr(dir_file->d_name, '.');
        if (ptr == NULL)
            continue;

        if (strcmp(ptr, ".h264") && strcmp(ptr, ".264") && strcmp(ptr, ".mp4") && strcmp(ptr, ".mp3") && strcmp(ptr, ".avi"))
            continue;

        file_path_len = 0;
        file_path_len += strlen(path);
        file_path_len += 1; // '/'
        file_path_len += strlen(dir_file->d_name);
        if (file_path_len > BURN_PLAYER_FILE_PATH_MAX_LEN-1) {
            BURN_PRINT_ERR("%s too long \n",dir_file->d_name);
            continue;
        }
        files->file_path[files->file_num] = (char *)malloc(file_path_len+1);
        files->file_path[files->file_num][file_path_len] = '\0';
        strcpy(files->file_path[files->file_num], path);
        strcat(files->file_path[files->file_num], "/");
        strcat(files->file_path[files->file_num], dir_file->d_name);
        BURN_PRINT_DGB("i: %d, filename: %s", files->file_num, files->file_path[files->file_num]);
        files->file_num ++;
        if (files->file_num >= BURN_PLAYER_FILE_MAX_NUM)
            break;
    }
    closedir(dir);
    return 0;
}

int burn_in_player_test(void *arg)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    char cmd[64] = {0};
    struct burn_in_player_ctx *ctx = malloc(sizeof(struct burn_in_player_ctx));

    BURN_PRINT_DGB("!!!!!enter player test!!!!!");

    if (!ctx) {
        BURN_PRINT_ERR("malloc error\n");
        return -1;
    }

    memset(ctx,0x00,sizeof(struct burn_in_player_ctx));

    if (!resource_is_exist(ctx)) {
        BURN_PRINT_ERR("!!!!resource is not exist!!!!!\n");
        ret = -1;
        goto _exit;
    }

    if (!BURN_IN_PLAYER_LOG_TO_SERIAL) {
        if (create_log_file(ctx)) {
            BURN_PRINT_ERR("create_log_file error\n");
            ret = -1;
            goto _exit;
        }
    }

    read_dir(ctx->dir,&ctx->files);

    if (ctx->files.file_num == 0) {
        BURN_PRINT_ERR("no test video in %s\n",ctx->dir);
        ret = -1;
        goto _exit;
    }

    for(i = 0;i < LPKG_BURN_IN_PLAYER_LOOP; i++) {
        for(j = 0;j < ctx->files.file_num;j++) {
            memset(cmd,0x00,sizeof(cmd));
            snprintf(cmd,sizeof(cmd),"player_demo -i %s",ctx->files.file_path[j]);
            ret = msh_exec(cmd, strlen(cmd));
            output_log(ctx,ctx->files.file_path[j],ret);
        }
    }

_exit:
    if (ctx->log_file) {
        fclose(ctx->log_file);
    }
    for(i = 0; i <ctx->files.file_num ;i++) {
        if (ctx->files.file_path[i]) {
            free(ctx->files.file_path[i]);
            ctx->files.file_path[i] = NULL;
        }
    }
    free(ctx);
    ctx = NULL;
    BURN_PRINT_DGB("!!!!!exit player test!!!!!");
    return ret;
}
