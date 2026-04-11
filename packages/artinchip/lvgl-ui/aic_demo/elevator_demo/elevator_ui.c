/*
 * Copyright (C) 2023-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Keliang Liu <keliang.liu@artinchip.com>
 *          Huahui Mai <huahui.mai@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "aic_ui.h"
#include "aic_player.h"
#include "elevator_ui.h"
#include "lv_port_disp.h"
#include "mpp_fb.h"

#include "elevator_uart.h"

FAKE_IMAGE_DECLARE(ELEVATOR_BG_WHITE);

LV_FONT_DECLARE(ui_font_h1);
LV_FONT_DECLARE(ui_font_regular);

#define AIC_ELEVATOR_INSIDE 1

#ifdef LV_ELEVATOR_UART_COMMAND
extern rt_device_t elevator_uart;
#endif

struct elevator_player_context{
    int file_cnt;
    int file_index;
    int player_state;
    struct aic_player *player;
    int sync_flag;
    struct av_media_info media_info;
    int demuxer_detected_flag;
    int player_end;
    struct mpp_size screen_size;
    struct mpp_rect disp_rect;
};

lv_obj_t *outside_up_down;
lv_obj_t *outside_level_1;
lv_obj_t *outside_level_2;

static int main_level;
static bool main_is_up;
static bool last_is_up;

#if AIC_ELEVATOR_INSIDE
lv_obj_t *title_date;
lv_obj_t *inside_up_down;
lv_obj_t *inside_level_1;
lv_obj_t *inside_level_2;

lv_obj_t *inside_up_down;
lv_obj_t *inside_level_1;
lv_obj_t *inside_level_2;
lv_obj_t *inside_repari;
char time_str[64];

#define LVGL_PLAYER_STATE_PLAY      1
#define LVGL_PLAYER_STATE_PAUSE     2
#define LVGL_PLAYER_STATE_STOP      3
struct elevator_player_context g_elevator_player_ctx;

static int aic_elevator_lvgl_play(struct elevator_player_context *ctx)
{
    int ret = 0;

    aic_player_set_uri(ctx->player, "/rodata/lvgl_data/elevator.mp4");
    ctx->sync_flag = AIC_PLAYER_PREPARE_SYNC;
    if (ctx->sync_flag == AIC_PLAYER_PREPARE_ASYNC)
        ret = aic_player_prepare_async(ctx->player);
    else
        ret = aic_player_prepare_sync(ctx->player);

    if (ret)
        return -1;

    if (ctx->sync_flag == AIC_PLAYER_PREPARE_SYNC) {
        ret = aic_player_start(ctx->player);
        if(ret != 0)
            return -1;

        aic_player_get_screen_size(ctx->player, &ctx->screen_size);
        printf("screen_width:%d,screen_height:%d\n",ctx->screen_size.width,ctx->screen_size.height);
        ctx->disp_rect.x = 222;
        ctx->disp_rect.y = 80;
        ctx->disp_rect.width = 800;
        ctx->disp_rect.height = 460;

        ret = aic_player_set_disp_rect(ctx->player, &ctx->disp_rect);
        if(ret != 0) {
            printf("aic_player_set_disp_rect error\n");
            return -1;
        }

        ret =  aic_player_get_media_info(ctx->player,&ctx->media_info);
        if (ret != 0)
            return -1;

        ret = aic_player_play(ctx->player);
        if (ret != 0)
            return -1;

        ctx->player_end  = 0;
    }
    return 0;
}

s32 aic_player_event_handle(void* app_data,s32 event,s32 data1,s32 data2)
{
    int ret = 0;
    struct elevator_player_context *ctx = (struct elevator_player_context *)app_data;

    switch (event) {
        case AIC_PLAYER_EVENT_PLAY_END:
            ctx->player_end = 1;
        case AIC_PLAYER_EVENT_PLAY_TIME:
            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == ctx->sync_flag)
                ctx->demuxer_detected_flag = 1;

            break;
        case AIC_PLAYER_EVENT_DEMUXER_FORMAT_NOT_DETECTED:
            if (AIC_PLAYER_PREPARE_ASYNC == ctx->sync_flag)
                ctx->player_end = 1;

            break;
        default:
            break;
    }
    return ret;
}

static void create_player(lv_obj_t * parent)
{
    lv_memset(&g_elevator_player_ctx, 0, sizeof(struct elevator_player_context));
    g_elevator_player_ctx.player = aic_player_create(NULL);
    if (g_elevator_player_ctx.player == NULL) {
        printf("aic_player_create fail!!!!\n");
        return;
    }

    g_elevator_player_ctx.file_cnt =  1;
    g_elevator_player_ctx.file_index = 0;
    g_elevator_player_ctx.player_state = LVGL_PLAYER_STATE_STOP;
    aic_player_set_event_callback(g_elevator_player_ctx.player, &g_elevator_player_ctx, aic_player_event_handle);

    if (aic_elevator_lvgl_play(&g_elevator_player_ctx) != 0) {
        g_elevator_player_ctx.player_state = LVGL_PLAYER_STATE_STOP;
        g_elevator_player_ctx.player_end = 1;
        return;
    } else {
        g_elevator_player_ctx.player_end  = 0;
    }
}

static char *get_time_string()
{
    static const char *week_day[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static time_t unix_time;
    static struct tm *time_info;

    unix_time = time(NULL);
    time_info = localtime(&unix_time);

    int year = time_info->tm_year+1900;
    int month = time_info->tm_mon + 1;
    int day = time_info->tm_mday;
    int weekday = time_info->tm_wday;
    int hour = time_info->tm_hour;
    int minutes = time_info->tm_min;
    int second = time_info->tm_sec;

    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %s %02d:%02d:%02d", year, month, day, week_day[weekday], hour, minutes, second);
    return time_str;
}

static void create_data_time(lv_obj_t *parament_obj)
{
    title_date = lv_label_create(parament_obj);
    lv_obj_align(title_date, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(title_date, get_time_string());
    lv_obj_center(title_date);
    lv_obj_set_style_text_color(title_date, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(title_date, 255, 0);
    lv_obj_set_style_text_font(title_date, &ui_font_h1, 0);
}

#ifndef LV_ELEVATOR_UART_COMMAND
static void update_data_time(lv_obj_t *inside_level_1)
{
    lv_label_set_text(title_date, get_time_string());
}

static void timer_inside_callback(lv_timer_t *tmr)
{
    char image_str[64];

    if (main_is_up)
        main_level ++;
    else
        main_level --;

    if (main_level > ELEVATOR_MAX_LEVEL) {
        main_is_up = false;
        main_level = ELEVATOR_MAX_LEVEL;
    }
    if (main_level < 1) {
        main_is_up = true;
        main_level = 1;
    }

    if (main_is_up != last_is_up) {
        if(main_is_up)
            lv_img_set_src(inside_up_down, LVGL_PATH(inside/arrow_up_Image.png));
        else
            lv_img_set_src(inside_up_down, LVGL_PATH(inside/arrow_down_Image.png));

        last_is_up = main_is_up;
    }

    if (main_level == 12) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_12A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 13) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_12B_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 14) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_15A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 15) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_15B_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 24) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_23A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 34) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_33A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level > 0 && main_level < 10) {
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level);
        lv_img_set_src(inside_level_1, image_str);
        lv_obj_set_pos(inside_level_1, 50 + 30, 80 + 50 + 121);
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level > 9 && main_level < 20) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_1_Image.png));
        lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level % 10);
        lv_obj_clear_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(inside_level_2, image_str);
    } else if (main_level > 19 && main_level < 30) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_2_Image.png));
        lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level % 20);
        lv_obj_clear_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(inside_level_2, image_str);
    } else if (main_level > 29 && main_level < 40) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_3_Image.png));
        lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level % 30);
        lv_obj_clear_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(inside_level_2, image_str);
    }

    if(main_level % 2 == 0)
        lv_img_set_src(inside_repari, LVGL_PATH(inside/repair_Image.png));
    else
        lv_img_set_src(inside_repari, LVGL_PATH(inside/siji_Image.png));

    update_data_time(title_date);
    return;
}
#endif

#ifdef LV_ELEVATOR_UART_COMMAND
int inside_up_down_ioctl(void *data, unsigned int data_len)
{
    char *str_rx = data;
    int enable;

    enable = atoi(str_rx);

    if (enable)
        lv_img_set_src(inside_up_down, LVGL_PATH(inside/arrow_down_Image.png));
    else
        lv_img_set_src(inside_up_down, LVGL_PATH(inside/arrow_up_Image.png));

    return 0;
}

int inside_level_ioctl(void *data, unsigned int data_len)
{
    char *str_rx = data;
    int main_level = atoi(str_rx);
    char send_srt[16] = { 0 };
    char image_str[64];

    lv_snprintf(send_srt, sizeof(send_srt), "level: %d", main_level);
    rt_device_write(elevator_uart, 0, send_srt, strlen(send_srt));

    if (main_level == 12) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_12A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 13) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_12B_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 14) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_15A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 15) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_15B_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 24) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_23A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 34) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_3_33A_Image.png));
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level > 0 && main_level < 10) {
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level);
        lv_img_set_src(inside_level_1, image_str);
        lv_obj_set_pos(inside_level_1, 50 + 30, 80 + 50 + 121);
        lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if(main_level > 9 && main_level < 20) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_1_Image.png));
        lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level % 10);
        lv_obj_clear_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(inside_level_2, image_str);
    } else if (main_level > 19 && main_level < 30) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_2_Image.png));
        lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
        snprintf(image_str,sizeof(image_str),  LVGL_PATH(inside/floor_l_%d_Image.png), main_level % 20);
        lv_obj_clear_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(inside_level_2, image_str);
    } else if (main_level > 29 && main_level < 40) {
        lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_3_Image.png));
        lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
        snprintf(image_str, sizeof(image_str), LVGL_PATH(inside/floor_l_%d_Image.png), main_level % 30);
        lv_obj_clear_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(inside_level_2, image_str);
    }

    return 0;
}

int replayer_ioctl(void *data, unsigned int data_len)
{
#if AIC_ELEVATOR_INSIDE
    char *str[] = { "replayer", "wait player" };

    if(g_elevator_player_ctx.player_end == 1)
    {
        aic_player_stop(g_elevator_player_ctx.player);
        aic_elevator_lvgl_play(&g_elevator_player_ctx);

        rt_device_write(elevator_uart, 0, str[0], strlen(str[0]));
    }
    else
    {
        rt_device_write(elevator_uart, 0, str[1], strlen(str[1]));
    }
#endif
    return 0;
}
#endif

void aic_elevator_create_main(lv_obj_t *tab)
{
    FAKE_IMAGE_INIT(ELEVATOR_BG_WHITE, 1024, 600, 0, 0x00000000);
    lv_obj_t * img_bg;
    img_bg = lv_img_create(tab);

    lv_img_set_src(img_bg, FAKE_IMAGE_NAME(ELEVATOR_BG_WHITE));
    lv_obj_set_pos(img_bg, 0, 0);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    //220 x 80
    lv_obj_t * image_otistitle = lv_img_create(tab);
    lv_img_set_src(image_otistitle, LVGL_PATH(inside/otistitle.png));
    lv_obj_set_pos(image_otistitle, 0, 0);

    //860 x 80
    lv_obj_t * image_datetime = lv_img_create(tab);
    lv_img_set_src(image_datetime, LVGL_PATH(inside/datetime.png));
    lv_obj_set_pos(image_datetime, 222, 0);

    create_data_time(image_datetime);

    //200 x 520
    lv_obj_t * image_panel = lv_img_create(tab);
    lv_img_set_src(image_panel, LVGL_PATH(inside/panel.png));
    lv_obj_set_pos(image_panel, 0 , 82);

    //800 x 60
    lv_obj_t * image_bottom = lv_img_create(tab);
    lv_img_set_src(image_bottom, LVGL_PATH(inside/bottom.png));
    lv_obj_set_pos(image_bottom, 222,  540);

    lv_obj_t *title_wel= lv_label_create(image_bottom);
    lv_obj_align(title_wel, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(title_wel, "Welcom to OTIS electric");
    lv_obj_center(title_wel);
    lv_obj_set_style_text_color(title_wel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(title_wel, 255, 0);
    lv_obj_set_style_text_font(title_wel, &ui_font_regular, 0);

    //117 x 121
    inside_up_down = lv_img_create(tab);
    lv_img_set_src(inside_up_down, LVGL_PATH(inside/arrow_up_Image.png));
    lv_obj_set_pos(inside_up_down, 50, 80 + 30);

    //60 x 95
    inside_level_1 = lv_img_create(tab);
    lv_img_set_src(inside_level_1, LVGL_PATH(inside/floor_l_1_Image.png));
    lv_obj_set_pos(inside_level_1, 50, 80 + 50 + 121);
    inside_level_2 = lv_img_create(tab);
    lv_img_set_src(inside_level_2, LVGL_PATH(inside/floor_l_1_Image.png));
    lv_obj_set_pos(inside_level_2, 120, 80 + 50 + 121);
    lv_obj_add_flag(inside_level_2, LV_OBJ_FLAG_HIDDEN);

    //116 x 32
    inside_repari = lv_img_create(tab);
    lv_img_set_src(inside_repari, LVGL_PATH(inside/repair_Image.png));
    lv_obj_set_pos(inside_repari, 50, 80 + 80 + 121 + 95 + 80);

#ifndef LV_ELEVATOR_UART_COMMAND
    lv_timer_create(timer_inside_callback, 500, 0);
#endif
}
#endif

static void timer_outside_callback(lv_timer_t *tmr)
{
    int x_margin = 20;
    int y_margin = 20;
    char image_str[64];

    if(main_is_up)
        main_level ++;
    else
        main_level --;

    if (main_level > ELEVATOR_MAX_LEVEL) {
        main_is_up = false;
        main_level = ELEVATOR_MAX_LEVEL;
    }
    if (main_level < 1) {
        main_is_up = true;
        main_level = 1;
    }

    if (main_is_up != last_is_up) {
        if (main_is_up)
            lv_img_set_src(outside_up_down, LVGL_PATH(outside/arrow_up_Image.png));
        else
            lv_img_set_src(outside_up_down, LVGL_PATH(outside/arrow_down_Image.png));

        last_is_up = main_is_up;
    }

    if (main_level == 12) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_3_12A_Image.png));
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 13) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_3_12B_Image.png));
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 14) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_3_15A_Image.png));
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 15) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_3_15B_Image.png));
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 24) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_3_23A_Image.png));
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if (main_level == 34) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_3_33A_Image.png));
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if(main_level > 0 && main_level < 10) {
        snprintf(image_str, sizeof(image_str), LVGL_PATH(outside/floor_l_%d_Image.png), main_level);
        lv_img_set_src(outside_level_1, image_str);
        lv_obj_set_pos(outside_level_1, 1024 / 2 + x_margin, 600 / 2 - 200 - (228 - 200) /2 + y_margin);
        lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
    } else if(main_level > 9 && main_level < 20) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_l_1_Image.png));
        snprintf(image_str, sizeof(image_str), LVGL_PATH(outside/floor_l_%d_Image.png), main_level % 10);
        lv_obj_set_pos(outside_level_1, 1024 / 2 - x_margin, 600 / 2 - 200 - (228 - 200) /2 + y_margin);
        lv_obj_clear_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(outside_level_2, image_str);
    } else if(main_level > 19 && main_level < 30) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_l_2_Image.png));
        snprintf(image_str, sizeof(image_str), LVGL_PATH(outside/floor_l_%d_Image.png), main_level % 20);
        lv_obj_set_pos(outside_level_1, 1024 / 2 - x_margin, 600 / 2 - 200 - (228 - 200) /2 + y_margin);
        lv_obj_clear_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(outside_level_2, image_str);
    } else if(main_level > 29 && main_level < 40) {
        lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_l_3_Image.png));
        snprintf(image_str, sizeof(image_str), LVGL_PATH(outside/floor_l_%d_Image.png),main_level % 30);
        lv_obj_set_pos(outside_level_1, 1024 / 2 - x_margin, 600 / 2 - 200 - (228 - 200) /2 + y_margin);
        lv_obj_clear_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(outside_level_2, image_str);
    }

    return;
}

void aic_elevator_create_outside(lv_obj_t *tab)
{
    FAKE_IMAGE_INIT(ELEVATOR_BG_WHITE, 1024, 600, 0, 0xFFFFFFFF);
    int x_margin = 20;
    int y_margin = 20;
    lv_obj_t * img_bg;
    img_bg = lv_img_create(tab);

    lv_img_set_src(img_bg, FAKE_IMAGE_NAME(ELEVATOR_BG_WHITE));
    lv_obj_set_pos(img_bg, 0, 0);

    //220 x 228
    outside_up_down = lv_img_create(tab);
    lv_img_set_src(outside_up_down, LVGL_PATH(outside/arrow_up_Image.png));
    //lv_obj_align(outside_up_down, LV_ALIGN_CENTER, -70, -30);
    lv_obj_set_pos(outside_up_down, 1024 / 2 - 220 - x_margin, 600 / 2 - 228 + y_margin);

    //126 x 200
    outside_level_1 = lv_img_create(tab);
    lv_img_set_src(outside_level_1, LVGL_PATH(outside/floor_l_1_Image.png));
    //lv_obj_align(outside_level_1, LV_ALIGN_CENTER, 10, -30);
    lv_obj_set_pos(outside_level_1, 1024 / 2 - x_margin, 600 / 2 - 200 - (228 - 200) /2 + y_margin);

    //126 x 200
    outside_level_2 = lv_img_create(tab);
    lv_img_set_src(outside_level_2, LVGL_PATH(outside/floor_l_7_Image.png));
    //lv_obj_align(outside_level_2, LV_ALIGN_CENTER, 10 + 60, -30);
    lv_obj_set_pos(outside_level_2, 1024 / 2  + 126 - x_margin , 600 / 2 - 200 - (228 - 200) /2 + y_margin);
    lv_obj_add_flag(outside_level_2, LV_OBJ_FLAG_HIDDEN);

    //118 x 143
    lv_obj_t *overload_image = lv_img_create(tab);
    lv_img_set_src(overload_image, LVGL_PATH(outside/overload_Image.png));
    lv_obj_align(overload_image, LV_ALIGN_CENTER, - (10 + 118), 100);

    //118 x 143
    lv_obj_t *nosmoke_image = lv_img_create(tab);
    lv_img_set_src(nosmoke_image, LVGL_PATH(outside/nosmoke_Image.png));
    lv_obj_align(nosmoke_image, LV_ALIGN_CENTER, 0, 100);

    //118 x 143
    lv_obj_t *fire_img = lv_img_create(tab);
    lv_img_set_src(fire_img, LVGL_PATH(outside/fire_Image.png));
    lv_obj_align(fire_img, LV_ALIGN_CENTER, 10 + 118, 100);

    lv_timer_create(timer_outside_callback, 500, 0);
}

void elevator_ui_init()
{
    main_level = 1;
    main_is_up = true; //0: down, 1: up
    last_is_up = main_is_up;

#if AIC_ELEVATOR_INSIDE
    aic_elevator_create_main(lv_scr_act());
    create_player(lv_scr_act());
#ifdef LV_ELEVATOR_UART_COMMAND
    evevator_uart_main();
#endif
#else
    aic_elevator_create_outside(lv_scr_act());
#endif
}

void ui_init(void)
{
    elevator_ui_init();
}
