/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lvgl.h"
#include "image_ui.h"
#include "aic_ui.h"
#include "aic_core.h"
#include "canvas_image.h"

#ifndef CANVAS_IMAGE_EXAMPLE
#define MAX_IMAGE_SIZE 1024 * 128

static lv_img_dsc_t memory_image = {
  .header.cf = LV_IMG_CF_RAW,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 0,
  .header.h = 0,
  .data_size = 0,
  .data = NULL,
};

static lv_obj_t *mem_img_cook = NULL;
static lv_obj_t *file_img_cook = NULL;
static uint8_t *image_data = NULL;

lv_fs_res_t get_image_size(lv_fs_file_t *file, uint32_t *file_size)
{
    lv_fs_seek(file, 0, SEEK_END);
    lv_fs_tell(file, file_size);
    lv_fs_seek(file, 0, SEEK_SET);
    return LV_FS_RES_OK;
}

static void change_mem_image_callback(lv_timer_t *tmr)
{
    (void)tmr;
    lv_fs_file_t file;
    static int id = 1;
    lv_fs_res_t res = LV_FS_RES_OK;
    uint32_t file_size = 0;

    // open image file
    if (id == 0) {
        res = lv_fs_open(&file, LVGL_PATH(cook_0.jpg), LV_FS_MODE_RD);
    } else {
        res = lv_fs_open(&file, LVGL_PATH(cook_1.png), LV_FS_MODE_RD);
    }

    if (res == LV_FS_RES_OK) {
        get_image_size(&file, &file_size);
        if (file_size > 0 && file_size <= MAX_IMAGE_SIZE) {
            uint32_t read_num = 0;

            // read png/jpg stream from file
            lv_fs_read(&file, image_data, file_size, &read_num);
            if (read_num == file_size) {
                memory_image.data = image_data;
                memory_image.data_size = file_size;
                // must clean image cache, when change image source
                lv_img_cache_invalidate_src(lv_img_get_src(mem_img_cook));
                // set image source
                lv_img_set_src(mem_img_cook, &memory_image);
            } else {
                LV_LOG_ERROR("read filed failed");
            }
        }
        // close image file
        lv_fs_close(&file);
    }

    // swith image
    id = (id == 0) ? 1 : 0;
    return;
}

static void change_file_image_callback(lv_timer_t *tmr)
{
    (void)tmr;
    static int id = 1;

    lv_img_cache_invalidate_src(lv_img_get_src(file_img_cook));
    if (id == 0)
        lv_img_set_src(file_img_cook, LVGL_PATH(cook_2.jpg));
    else
        lv_img_set_src(file_img_cook,LVGL_PATH(cook_3.jpg));

    // swith image
    id = (id == 0) ? 1 : 0;
    return;
}

#endif

void image_ui_init(void)
{
#ifdef CANVAS_IMAGE_EXAMPLE
    extern void lv_canvas_image_text(void);
    lv_canvas_image_text();
#else
    image_data = (uint8_t *)aicos_malloc(MEM_CMA, MAX_IMAGE_SIZE);
    if (!image_data) {
        LV_LOG_ERROR("malloc image_data failed");
        return;
    }
    file_img_cook = lv_img_create(lv_scr_act());
    lv_obj_align(file_img_cook, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_img_opa(file_img_cook, LV_OPA_50, LV_PART_MAIN);

    mem_img_cook = lv_img_create(lv_scr_act());
    lv_obj_center(mem_img_cook);

    lv_timer_create(change_mem_image_callback, 1000, 0);
    lv_timer_create(change_file_image_callback, 1000, 0);
#endif
}

void ui_init(void)
{
    image_ui_init();
}
