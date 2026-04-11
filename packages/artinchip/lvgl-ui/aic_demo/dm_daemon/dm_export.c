/*
 * Copyright (C) 2022-2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Geo Dong <guojun.dong@artinchip.com>
 */

#include <rtthread.h>
#include <lvgl.h>
#include <rtm.h>
#include <stdint.h>
#include "aic_common.h"

// LVGL API
RTM_EXPORT(lv_obj_remove_flag);
RTM_EXPORT(lv_obj_add_event_cb);
RTM_EXPORT(lv_obj_align);
RTM_EXPORT(lv_screen_active);
RTM_EXPORT(lv_button_create);
RTM_EXPORT(lv_display_get_screen_active);
RTM_EXPORT(lv_display_get_default);
RTM_EXPORT(lv_event_get_code);
RTM_EXPORT(lv_log_register_print_cb);
RTM_EXPORT(lv_tick_set_cb);
RTM_EXPORT(lv_init);
RTM_EXPORT(lv_timer_handler);
RTM_EXPORT(lv_label_create);
RTM_EXPORT(lv_label_set_text);
RTM_EXPORT(lv_obj_center);
RTM_EXPORT(lv_obj_set_height);
RTM_EXPORT(lv_obj_add_flag);

// ArtInChip API
extern int lvgl_thread_init(void);
RTM_EXPORT(lvgl_thread_init);
extern void lv_port_disp_init(void);
RTM_EXPORT(lv_port_disp_init);
extern void lv_port_indev_init(void);
RTM_EXPORT(lv_port_indev_init);
extern void lv_user_gui_init(void);
RTM_EXPORT(lv_user_gui_init);
extern void lv_wait_fs_mounted(void);
RTM_EXPORT(lv_wait_fs_mounted);
extern void aic_mdelay(u32 ms);
RTM_EXPORT(aic_mdelay);

// RT-Thread API
RTM_EXPORT(rt_tick_get_millisecond);
extern void ulog_output(rt_uint32_t level, const char *tag, rt_bool_t newline, const char *format, ...);
RTM_EXPORT(ulog_output);
extern struct dfs_filesystem *dfs_filesystem_lookup(const char *path);
RTM_EXPORT(dfs_filesystem_lookup);

