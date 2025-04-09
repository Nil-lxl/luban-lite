/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include <stdio.h>
#include "production.h"
#include <unistd.h>
#include "details.h"
#include "coffee.h"

static lv_obj_t* bigImg;
static lv_obj_t* bigImgLabel;
static lv_obj_t *bar;
static lv_obj_t *barLable;

static void createBigImage()
{
    bigImg = lv_img_create(productionScr);
    lv_obj_set_size(bigImg, pos_width_conversion(158), pos_height_conversion(262));
    lv_obj_set_pos(bigImg, pos_width_conversion(321), pos_height_conversion(80));

    static lv_style_t style_def;
    if (style_def.prop_cnt > 1) {
        lv_style_reset(&style_def);
    } else {
        lv_style_init(&style_def);
    }
    lv_style_set_text_color(&style_def, lv_color_white());

    bigImgLabel = lv_label_create(bigImg);
    lv_obj_set_style_text_font(bigImgLabel,&NotoSanCJK_Medium_18,0);
    lv_obj_add_style(bigImgLabel, &style_def, 0);
    lv_obj_align(bigImgLabel, LV_ALIGN_CENTER, 0, pos_height_conversion(-80));
}

static void createBar()
{
    static lv_style_t style_bg;
    if (style_bg.prop_cnt > 1) {
        lv_style_reset(&style_bg);
    } else {
        lv_style_init(&style_bg);
    }
    lv_style_set_bg_opa(&style_bg, LV_OPA_TRANSP);
    lv_style_set_border_color(&style_bg, lv_color_hex(0xeebf7b));
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 4);
    lv_style_set_radius(&style_bg, 11);
    lv_style_set_anim_time(&style_bg, 1000);

    static lv_style_t style_indic;
    if (style_indic.prop_cnt > 1) {
        lv_style_reset(&style_indic);
    } else {
        lv_style_init(&style_indic);
    }
    lv_style_set_radius(&style_indic, 9);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_color_hex(0xeebf7b));
    lv_style_set_bg_grad_color(&style_indic, lv_color_hex(0xfdb802));
    lv_style_set_bg_grad_dir(&style_indic, LV_GRAD_DIR_HOR);

    bar = lv_bar_create(productionScr);
    lv_obj_remove_style_all(bar);
    lv_obj_add_style(bar, &style_bg, 0);
    lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(bar, pos_width_conversion(386), pos_height_conversion(26));
    // lv_obj_align(bar, LV_ALIGN_CENTER, 0, 120);
    lv_obj_set_pos(bar, pos_width_conversion(207), pos_height_conversion(320));
    lv_bar_set_range(bar, 0, 100);

    static lv_style_t style_def;
    if (style_def.prop_cnt > 1) {
        lv_style_reset(&style_def);
    } else {
        lv_style_init(&style_def);
    }
    lv_style_set_text_color(&style_def, lv_color_white());

    barLable = lv_label_create(productionScr);
    lv_obj_set_style_text_font(barLable,&NotoSanCJK_Medium_18,0);
    lv_obj_add_style(barLable, &style_def, 0);
    // lv_obj_align(barLable, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_pos(barLable, pos_width_conversion(380), pos_height_conversion(280));
}

static void changeBigImage()
{
    lv_img_set_src(bigImg, categoryData[currentCoffeeType].productionSrc);
    lv_label_set_text(bigImgLabel, categoryData[currentCoffeeType].title);
    lv_obj_invalidate(bigImg);
    lv_obj_invalidate(bigImgLabel);
}

static void barEventCb(void * bar, int32_t value)
{
    char buf[16] = {0};
    long int l_value = value;
    lv_bar_set_value(bar, value, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%ld%%", l_value);
    lv_label_set_text(barLable, buf);
    lv_obj_invalidate(barLable);

    if (value == 100)
    {
        showDetails();
    }
}

static void setProductionParam(struct coffeeParam* param[], int paramSize)
{
    lv_anim_t test;
    lv_anim_init(&test);
    lv_anim_set_exec_cb(&test, barEventCb);
    lv_anim_set_time(&test, 3000);
    lv_anim_set_var(&test, bar);
    lv_anim_set_values(&test, 0, 100);
    lv_anim_start(&test);
}

void productionInit()
{
    productionScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(productionScr, lv_color_black(), 0);
    createTopTitle(productionScr);
    createBigImage();
    createBar();
}

void showProduction(struct coffeeParam* param[], int paramSize)
{
    changeBigImage();
    loadScr = productionScr;
    lv_scr_load(productionScr);
    setProductionParam(param, paramSize);
}
