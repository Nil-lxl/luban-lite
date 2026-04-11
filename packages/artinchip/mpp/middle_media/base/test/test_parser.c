/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: parser test
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
#include "mpp_log.h"
#include "mpp_mem.h"
#include "aic_audio_decoder.h"
#include "aic_core.h"
#include "aic_parser.h"

#include <rthw.h>
#include <rtthread.h>
#include <shell.h>

#define PARSER_FILE_MAX_NUM 128
#define PARSER_FILE_PATH_MAX_LEN 128

struct parser_file_list {
    char *file_path[PARSER_FILE_MAX_NUM];
    int file_num;
};

struct parser_context {
    struct aic_parser *parser;
    struct parser_file_list files;
    struct aic_parser_av_media_info media_info;
};


static void print_help(const char* prog)
{
    printf("name: %s\n", prog);
    printf("Usage: parser_test [options]:\n"
        "\t-i\t\tinput stream file name\n"
        "\t-t\t\tdirectory of test files\n");
}

static int read_file(char* path, struct parser_file_list *files)
{
    int file_path_len;
    file_path_len = strlen(path);
    logd("file_path_len:%d\n", file_path_len);
    if (file_path_len > PARSER_FILE_PATH_MAX_LEN-1) {
        loge("file_path_len too long \n");
        return -1;
    }
    files->file_path[0] = (char *)aicos_malloc(MEM_DEFAULT, file_path_len + 1);
    files->file_path[0][file_path_len] = '\0';
    strcpy(files->file_path[0], path);
    files->file_num = 1;
    logd("file path: %s\n", files->file_path[0]);
    return 0;
}

static int read_dir(char* path, struct parser_file_list *files)
{
    char* ptr = NULL;
    int file_path_len = 0;
    struct dirent* dir_file;

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

        logd("name: %s", dir_file->d_name);

        file_path_len = 0;
        file_path_len += strlen(path);
        file_path_len += 1;
        file_path_len += strlen(dir_file->d_name);
        logd("file_path_len:%d\n", file_path_len);
        if (file_path_len > PARSER_FILE_PATH_MAX_LEN - 1) {
            loge("%s too long \n",dir_file->d_name);
            continue;
        }
        files->file_path[files->file_num] = (char *)mpp_alloc(file_path_len + 1);
        if (files->file_path[files->file_num] == NULL)
            continue;

        files->file_path[files->file_num][file_path_len] = '\0';
        snprintf(files->file_path[files->file_num],
             file_path_len + 1, "%s/%s", path, dir_file->d_name);
        logd("i: %d, filename: %s", files->file_num, files->file_path[files->file_num]);
        files->file_num++;
        if (files->file_num >= PARSER_FILE_MAX_NUM)
            break;
    }
    closedir(dir);
    return 0;
}

static int parse_options(struct parser_context *ctx, int cnt, char **options)
{
    int argc = cnt;
    char **argv = options;
    int opt;

    if (!ctx || argc == 0 || !argv) {
        loge("para error !!!");
        return -1;
    }
    optind = 0;
    while (1) {
        opt = getopt(argc, argv, "i:t:h");
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'i':
                read_file(optarg, &ctx->files);
                break;
            case 't':
                read_dir(optarg, &ctx->files);
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

char *vdec_type_str(enum mpp_codec_type codec_type)
{
    switch (codec_type) {
        case MPP_CODEC_VIDEO_DECODER_H264:
            return "H264";
        case MPP_CODEC_VIDEO_DECODER_MJPEG:
            return "MJPEG";
        default:
            return "Unknown";
    }
}

char *adec_type_str(enum aic_audio_codec_type codec_type)
{
    switch (codec_type) {
        case MPP_CODEC_AUDIO_DECODER_MP3:
            return "MP3";
        case MPP_CODEC_AUDIO_DECODER_AAC:
            return "AAC";
        case MPP_CODEC_AUDIO_DECODER_FLAC:
            return "FLAC";
        case MPP_CODEC_AUDIO_DECODER_WMA:
            return "WMA";
        case MPP_CODEC_AUDIO_DECODER_APE:
            return "APE";
        default:
            return "Unknown";
    }
}

static void parser_info_print(char *file, struct aic_parser_av_media_info *media_info)
{
    if (!file || !media_info)
        return;

    printf("%s\n", file);
    if (media_info->has_video) {
        printf("\t\tVideo info:\n");
        printf("\t\t\tdec_type:\t\t%s\n",
            vdec_type_str(media_info->video_stream.codec_type));
        printf("\t\t\twidth x height:\t\t%d x %d\n", media_info->video_stream.width,
                media_info->video_stream.height);
    }

    if (media_info->has_audio) {
        printf("\t\tAudio info:\n");
        printf("\t\t\tdec_type:\t\t%s\n",
            adec_type_str(media_info->audio_stream.codec_type));

        printf("\t\t\tbit_width:\t\t%d\n", media_info->audio_stream.bits_per_sample);
        printf("\t\t\tnb_channel:\t\t%d\n", media_info->audio_stream.nb_channel);
        printf("\t\t\tsample_rate:\t\t%d\n", media_info->audio_stream.sample_rate);
    }
    printf("\n");
}

static int parser_test(int argc, char **argv)
{
    int i = 0;
    struct parser_context *ctx = NULL;
    struct aic_parser_av_media_info *media_info;

    ctx = mpp_alloc(sizeof(struct parser_context));
    if (!ctx) {
        loge("mpp_alloc fail!");
        return -1;
    }
    memset(ctx, 0x0, sizeof(struct parser_context));

    if (parse_options(ctx, argc, argv)) {
        goto exit;
    }

    media_info = &ctx->media_info;
    for(i = 0; i < ctx->files.file_num; i++) {
        aic_parser_create((unsigned char*)ctx->files.file_path[i], &ctx->parser);
        if (ctx->parser == NULL) {
            loge("aic_parser_create %s fail\n", ctx->files.file_path[i]);
            continue;
        }
        if (aic_parser_init(ctx->parser)) {
            loge("aic_parser_init fail\n");
            goto next_file;
        }
        memset(media_info, 0, sizeof(struct aic_parser_av_media_info));
        if (aic_parser_get_media_info(ctx->parser, media_info)) {
            loge("aic_parser_get_media_info fail\n");
            goto next_file;
        }

        parser_info_print(ctx->files.file_path[i], media_info);

next_file:
        aic_parser_destroy(ctx->parser);
        ctx->parser = NULL;
    }

exit:
    for(i = 0; i <ctx->files.file_num ;i++) {
        if (ctx->files.file_path[i]) {
            mpp_free(ctx->files.file_path[i]);
            ctx->files.file_path[i] = NULL;
        }
    }
    mpp_free(ctx);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(parser_test, parser_test, parser test);
