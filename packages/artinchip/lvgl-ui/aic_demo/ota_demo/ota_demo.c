/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan.liang@artinchip.com>
 */

#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "lv_os.h"

#ifdef LPKG_USING_OTA_DOWNLOADER

#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>
#include <dfs_fs.h>
#include "ota_demo.h"
#include <ota.h>
#include <absystem_os.h>
#include <env.h>

#define OTA_BUFFER_SIZE     256
#define OTA_THREAD_STACK    4096
#define OTA_THREAD_PRIO     LV_THREAD_PRIO_HIGH
#define UI_TIMER_PERIOD_MS  100

typedef enum {
    OTA_ERR_NONE = 0,
    OTA_ERR_FILE_OPEN,
    OTA_ERR_INIT_FAILED,
    OTA_ERR_DOWNLOAD,
    OTA_ERR_UPGRADE_END,
} ota_error_t;

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_RUNNING,
    OTA_STATE_SUCCESS,
    OTA_STATE_ERROR,
} ota_state_t;

/* Shared state - volatile for thread safety */
static struct {
    volatile int progress;
    volatile ota_state_t state;
    volatile ota_error_t error_code;

    lv_thread_t ota_thread;
    lv_timer_t *ui_timer;
    lv_timer_t *reboot_timer;

    lv_obj_t *scr;
    lv_obj_t *progress_bar;
    lv_obj_t *progress_label;
    lv_obj_t *status_label;
    lv_obj_t *btn_start;
    lv_obj_t *btn_label;

    char ota_file_path[64];
} g_ota_ctx;

static char ota_buffer[OTA_BUFFER_SIZE];

static void ota_ui_timer_cb(lv_timer_t *timer);
static void ota_reboot_timer_cb(lv_timer_t *timer);
static void ota_download_thread(void *arg);
static void btn_start_cb(lv_event_t *e);
static void scr_unload_cb(lv_event_t *e);
static int ota_do_upgrade(void);

static const char *get_error_msg(ota_error_t err)
{
    switch (err) {
    case OTA_ERR_NONE:
        return "Ready";
    case OTA_ERR_FILE_OPEN:
        return "Error: Failed to open OTA file";
    case OTA_ERR_INIT_FAILED:
        return "Error: OTA initialization failed";
    case OTA_ERR_DOWNLOAD:
        return "Error: Download failed";
    case OTA_ERR_UPGRADE_END:
        return "Error: Upgrade end failed";
    default:
        return "Unknown error";
    }
}

/* LVGL thread context - safe to call LVGL APIs */
static void ota_ui_timer_cb(lv_timer_t *timer)
{
    int progress = g_ota_ctx.progress;
    ota_state_t state = g_ota_ctx.state;

    if (g_ota_ctx.progress_bar) {
        lv_bar_set_value(g_ota_ctx.progress_bar, progress, LV_ANIM_OFF);
    }

    if (g_ota_ctx.progress_label) {
        lv_label_set_text_fmt(g_ota_ctx.progress_label, "%d%%", progress);
    }

    if (g_ota_ctx.status_label) {
        switch (state) {
        case OTA_STATE_IDLE:
            lv_label_set_text(g_ota_ctx.status_label, "Ready to upgrade");
            if (g_ota_ctx.btn_start) {
                lv_obj_clear_state(g_ota_ctx.btn_start, LV_STATE_DISABLED);
            }
            break;

        case OTA_STATE_RUNNING:
            lv_label_set_text(g_ota_ctx.status_label, "Upgrading...");
            if (g_ota_ctx.btn_start) {
                lv_obj_add_state(g_ota_ctx.btn_start, LV_STATE_DISABLED);
            }
            break;

        case OTA_STATE_SUCCESS:
            lv_label_set_text(g_ota_ctx.status_label, "Upgrade successful! Rebooting...");
            if (g_ota_ctx.btn_start) {
                lv_obj_add_state(g_ota_ctx.btn_start, LV_STATE_DISABLED);
            }
            if (g_ota_ctx.ui_timer) {
                lv_timer_del(g_ota_ctx.ui_timer);
                g_ota_ctx.ui_timer = NULL;
            }
            g_ota_ctx.reboot_timer = lv_timer_create(ota_reboot_timer_cb, 3000, NULL);
            g_ota_ctx.state = OTA_STATE_IDLE;
            break;

        case OTA_STATE_ERROR:
            lv_label_set_text(g_ota_ctx.status_label, get_error_msg(g_ota_ctx.error_code));
            if (g_ota_ctx.btn_start) {
                lv_obj_clear_state(g_ota_ctx.btn_start, LV_STATE_DISABLED);
                if (g_ota_ctx.btn_label) {
                    lv_label_set_text(g_ota_ctx.btn_label, "Retry");
                }
            }
            g_ota_ctx.state = OTA_STATE_IDLE;
            break;
        }
    }
}

static void ota_reboot_timer_cb(lv_timer_t *timer)
{
    extern void rt_hw_cpu_reset(void);

    LV_LOG_INFO("Rebooting system...");
    rt_hw_cpu_reset();
}

/* Runs in separate thread - only writes to volatile shared vars */
static void ota_download_thread(void *arg)
{
    int ret;

    (void)arg;

    LV_LOG_INFO("OTA download thread started");

    ret = ota_do_upgrade();

    if (ret == 0) {
        g_ota_ctx.state = OTA_STATE_SUCCESS;
        g_ota_ctx.progress = 100;
        LV_LOG_INFO("OTA upgrade completed successfully");
    } else {
        g_ota_ctx.state = OTA_STATE_ERROR;
        LV_LOG_ERROR("OTA upgrade failed with error: %d", g_ota_ctx.error_code);
    }
}

static int ota_do_upgrade(void)
{
    FILE *file = NULL;
    int size;
    int ret = -1;
    long total_size = 0;
    long downloaded = 0;

    file = fopen(g_ota_ctx.ota_file_path, "rb");
    if (file == NULL) {
        LV_LOG_ERROR("Failed to open OTA file: %s", g_ota_ctx.ota_file_path);
        g_ota_ctx.error_code = OTA_ERR_FILE_OPEN;
        return -1;
    }

    fseek(file, 0, SEEK_END);
    total_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (total_size <= 0) {
        LV_LOG_ERROR("Invalid OTA file size: %ld", total_size);
        g_ota_ctx.error_code = OTA_ERR_FILE_OPEN;
        goto __exit;
    }

    LV_LOG_INFO("OTA file size: %ld bytes", total_size);

    ret = ota_init();
    if (ret != RT_EOK) {
        LV_LOG_ERROR("OTA initialization failed");
        g_ota_ctx.error_code = OTA_ERR_INIT_FAILED;
        goto __exit;
    }

    while (!feof(file)) {
        size = fread(ota_buffer, 1, OTA_BUFFER_SIZE, file);

        if (size > 0) {
            ret = ota_shard_download_fun(ota_buffer, size);
            if (ret < 0) {
                LV_LOG_ERROR("OTA download failed");
                g_ota_ctx.error_code = OTA_ERR_DOWNLOAD;
                goto __exit_deinit;
            }

            downloaded += size;
            g_ota_ctx.progress = (int)(downloaded * 100 / total_size);
        }
    }

    g_ota_ctx.progress = 100;

    ret = aic_upgrade_end();
    if (ret) {
        LV_LOG_ERROR("OTA upgrade end failed");
        g_ota_ctx.error_code = OTA_ERR_UPGRADE_END;
        goto __exit_deinit;
    }

    ret = 0;

__exit_deinit:
    ota_deinit();

__exit:
    if (file) {
        fclose(file);
    }

    return ret;
}

static void btn_start_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        if (g_ota_ctx.state != OTA_STATE_IDLE) {
            LV_LOG_WARN("OTA already in progress");
            return;
        }

        g_ota_ctx.progress = 0;
        g_ota_ctx.error_code = OTA_ERR_NONE;
        g_ota_ctx.state = OTA_STATE_RUNNING;

        lv_result_t result = lv_thread_init(
            &g_ota_ctx.ota_thread,
            OTA_THREAD_PRIO,
            ota_download_thread,
            OTA_THREAD_STACK,
            NULL
        );

        if (result != LV_RESULT_OK) {
            LV_LOG_ERROR("Failed to create OTA thread");
            g_ota_ctx.state = OTA_STATE_ERROR;
            g_ota_ctx.error_code = OTA_ERR_INIT_FAILED;
        } else {
            LV_LOG_INFO("OTA thread created successfully");
        }
    }
}

static void scr_unload_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        LV_LOG_INFO("OTA demo screen unloading, cleaning up...");

        if (g_ota_ctx.ui_timer) {
            lv_timer_del(g_ota_ctx.ui_timer);
            g_ota_ctx.ui_timer = NULL;
        }

        if (g_ota_ctx.reboot_timer) {
            lv_timer_del(g_ota_ctx.reboot_timer);
            g_ota_ctx.reboot_timer = NULL;
        }

        if (g_ota_ctx.state == OTA_STATE_RUNNING) {
            lv_thread_delete(&g_ota_ctx.ota_thread);
        }

        memset(&g_ota_ctx, 0, sizeof(g_ota_ctx));
    }
}

void ui_init(void)
{
    memset(&g_ota_ctx, 0, sizeof(g_ota_ctx));
    strncpy(g_ota_ctx.ota_file_path, "/sdcard/ota.cpio", sizeof(g_ota_ctx.ota_file_path) - 1);

    g_ota_ctx.scr = lv_obj_create(NULL);
    lv_obj_clear_flag(g_ota_ctx.scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(g_ota_ctx.scr, lv_color_hex(0x1A1D26), 0);
    lv_obj_set_style_bg_opa(g_ota_ctx.scr, LV_OPA_100, 0);

    lv_obj_add_event_cb(g_ota_ctx.scr, scr_unload_cb, LV_EVENT_SCREEN_UNLOAD_START, NULL);

    lv_obj_t *title = lv_label_create(g_ota_ctx.scr);
    lv_label_set_text(title, "OTA Upgrade");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    g_ota_ctx.status_label = lv_label_create(g_ota_ctx.scr);
    lv_label_set_text(g_ota_ctx.status_label, "Ready to upgrade");
    lv_obj_set_style_text_color(g_ota_ctx.status_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(g_ota_ctx.status_label, LV_ALIGN_TOP_MID, 0, 80);

    g_ota_ctx.progress_bar = lv_bar_create(g_ota_ctx.scr);
    lv_obj_set_size(g_ota_ctx.progress_bar, 400, 30);
    lv_obj_align(g_ota_ctx.progress_bar, LV_ALIGN_CENTER, 0, -20);
    lv_bar_set_range(g_ota_ctx.progress_bar, 0, 100);
    lv_bar_set_value(g_ota_ctx.progress_bar, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(g_ota_ctx.progress_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_ota_ctx.progress_bar, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_ota_ctx.progress_bar, lv_color_hex(0x00A2E9), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_ota_ctx.progress_bar, LV_OPA_100, LV_PART_INDICATOR);

    g_ota_ctx.progress_label = lv_label_create(g_ota_ctx.scr);
    lv_label_set_text(g_ota_ctx.progress_label, "0%");
    lv_obj_set_style_text_color(g_ota_ctx.progress_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_ota_ctx.progress_label, LV_ALIGN_CENTER, 0, 20);

    g_ota_ctx.btn_start = lv_btn_create(g_ota_ctx.scr);
    lv_obj_set_size(g_ota_ctx.btn_start, 150, 50);
    lv_obj_align(g_ota_ctx.btn_start, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_add_event_cb(g_ota_ctx.btn_start, btn_start_cb, LV_EVENT_CLICKED, NULL);

    g_ota_ctx.btn_label = lv_label_create(g_ota_ctx.btn_start);
    lv_label_set_text(g_ota_ctx.btn_label, "Start OTA");
    lv_obj_set_style_text_font(g_ota_ctx.btn_label, &lv_font_montserrat_16, 0);
    lv_obj_center(g_ota_ctx.btn_label);

    g_ota_ctx.ui_timer = lv_timer_create(ota_ui_timer_cb, UI_TIMER_PERIOD_MS, NULL);

    LV_LOG_INFO("OTA demo UI initialized");

    lv_scr_load(g_ota_ctx.scr);
}

#else /* LPKG_USING_OTA_DOWNLOADER */

void ui_init(void)
{
    lv_obj_t *warn = lv_label_create(lv_scr_act());
    lv_label_set_text(warn, "OTA demo requires LPKG_USING_OTA_DOWNLOADER");
    lv_obj_set_style_text_color(warn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(warn, LV_ALIGN_CENTER, 0, 30);
    LV_LOG_WARN("OTA demo requires LPKG_USING_OTA_DOWNLOADER");
}

#endif /* LPKG_USING_OTA_DOWNLOADER */
