/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */

#include "rtthread.h"
#include "lvgl.h"
#include "aic_ui.h"
#include "string.h"

#include "./view/simple_screen.h"
#include "./module/ui_msg_queen.h"
#include "./module/simple_serial.h"

#define LOAD_SCREEN_0_CMD "load simple_screen_0"
#define LOAD_SCREEN_1_CMD "load simple_screen_1"
#define SCREEN_0_PREFIX "simple_screen_0"
#define SCREEN_1_PREFIX "simple_screen_1"

static void serial_recv_entry(void *parameter);
static int serial_recv_thread_init(void);
static void process_ui_msg_cb(lv_timer_t *timer);
static int process_ui_msg_timer_init(void);

static lv_obj_t *g_simple_screen0;
static lv_obj_t *g_simple_screen1;

char g_current_screen[128];

void ui_init(void)
{
    int ret = -1;

    g_simple_screen0 = simple_screen_create();
    g_simple_screen1 = simple_screen_create();

    lv_screen_load_anim(g_simple_screen0, LV_SCR_LOAD_ANIM_FADE_ON, 2000, 0, 0);

    ret = ui_msg_queen_init();
    if (ret < 0)
        return;

    ret = simple_serial_init("uart4", 115200, 8, 1, 0);
    if (ret < 0)
        return;

    ret = serial_recv_thread_init();
    if (ret < 0)
        return;

    process_ui_msg_timer_init();
}

char *get_current_active_screen_str(void)
{
    return g_current_screen;
}

void set_current_active_screen_str(const char *name)
{
    memset(g_current_screen, 0, sizeof(g_current_screen));
    strncpy(g_current_screen, name, sizeof(g_current_screen) - 1);
}

static void serial_recv_entry(void *parameter)
{
    ui_message_t ui_msg = {0};
    int len = 0;

    while (1) {
        memset(&ui_msg, 0, sizeof(ui_message_t));

        len = simple_serial_recv((unsigned char *)ui_msg.msg, SIMPLE_MSQ_SIZE);
        if (len <= 0)
            continue;

        ui_msg.msg_len = len;
        ui_msg_queen_send(&ui_msg);
    }
}

static int serial_recv_thread_init(void)
{
    rt_thread_t thread = rt_thread_create("ui_msg_send", serial_recv_entry, RT_NULL, 1024*4, 20, 10);
    if (thread == RT_NULL) {
        printf("create ui msg thread failed!\n");
        return -1;
    }

    rt_thread_startup(thread);
    return 0;
}

static void process_ui_msg_cb(lv_timer_t *timer)
{
    ui_message_t msg;

    if (ui_msg_queen_recv(&msg) == -1)
        return;

    /* handle msg */
    if (strncmp(msg.msg, LOAD_SCREEN_0_CMD, strlen(LOAD_SCREEN_0_CMD)) == 0) {
        lv_screen_load_anim(g_simple_screen0, LV_SCR_LOAD_ANIM_MOVE_LEFT, 2000, 0, 0);
    } else if (strncmp(msg.msg, LOAD_SCREEN_1_CMD, strlen(LOAD_SCREEN_1_CMD)) == 0) {
        lv_screen_load_anim(g_simple_screen1, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 2000, 0, 0);
    }

    if (strncmp(msg.msg, SCREEN_0_PREFIX, strlen(SCREEN_0_PREFIX)) == 0) {
        lv_obj_t *label = simple_screen_get_label(g_simple_screen0);
        lv_label_set_text_fmt(label, "msg0:%s", msg.msg);
    } else if (strncmp(msg.msg, SCREEN_1_PREFIX, strlen(SCREEN_1_PREFIX)) == 0) {
        lv_obj_t *label = simple_screen_get_label(g_simple_screen1);
        lv_label_set_text_fmt(label, "msg1:%s", msg.msg);
    }
}

static int process_ui_msg_timer_init(void)
{
    if (lv_timer_create(process_ui_msg_cb, 5, NULL) == NULL) {
        printf("create recv_ui_msg_timer failed\n");
        return -1;
    }

    return 0;
}
