/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "details.h"
#include "menu.h"
#include "production.h"
#include <stdlib.h>
#include <stdio.h>
#include "aic_ui.h"
#include "coffee.h"

#if USE_ARRAY_IMG
LV_IMG_DECLARE(formula_coffeebean);
LV_IMG_DECLARE(formula_coffeecnt);
LV_IMG_DECLARE(formula_temperature);
LV_IMG_DECLARE(formula_milk);
LV_IMG_DECLARE(Cancel);
LV_IMG_DECLARE(Cancel_pressed);
LV_IMG_DECLARE(Ok);
LV_IMG_DECLARE(Ok_pressed);
LV_IMG_DECLARE(Add_normal);
LV_IMG_DECLARE(Add_pressed);
LV_IMG_DECLARE(Reduce_normal);
LV_IMG_DECLARE(Reduce_pressed);
static struct coffeeParam coffeeParamData[] = {
    {PT_coffeeBean,  &formula_coffeebean,    "咖啡豆",      10,     "g",    10,0,30},
    {PT_coffeeCnt,   &formula_coffeecnt,     "咖啡量",      50,     "ml",   50,0,100},
    {PT_temperature, &formula_temperature,   "温    度",        65,     "℃",    65,0,100},
    {PT_milk,        &formula_milk,          "牛    奶",        0,      "ml",   0,0,100},
};
#else
#define formula_coffeebean          LVGL_PATH(coffee_demo/formula_coffeebean.png)
#define formula_coffeecnt           LVGL_PATH(coffee_demo/formula_coffeecnt.png)
#define formula_temperature         LVGL_PATH(coffee_demo/formula_temperature.png)
#define formula_milk                LVGL_PATH(coffee_demo/formula_milk.png)
#define Cancel                      LVGL_PATH(coffee_demo/Cancel.png)
#define Cancel_pressed              LVGL_PATH(coffee_demo/Cancel_pressed.png)
#define Ok                          LVGL_PATH(coffee_demo/Ok.png)
#define Ok_pressed                  LVGL_PATH(coffee_demo/Ok_pressed.png)
#define Add_normal                  LVGL_PATH(coffee_demo/Add_normal.png)
#define Add_pressed                 LVGL_PATH(coffee_demo/Add_pressed.png)
#define Reduce_normal               LVGL_PATH(coffee_demo/Reduce_normal.png)
#define Reduce_pressed              LVGL_PATH(coffee_demo/Setting_pressed.png)
#endif

static struct coffeeParam coffeeParamData[] = {
    {PT_coffeeBean,  formula_coffeebean,     "咖啡豆",      10,     "克",   10,0,30},
    {PT_coffeeCnt,   formula_coffeecnt,  "咖啡量",      50,     "毫升", 50,0,100},
    {PT_temperature, formula_temperature,    "温    度",        65,     "度",   65,0,100},
    {PT_milk,        formula_milk,           "牛    奶",        0,      "毫升", 0,0,100},
};

static lv_obj_t* bigImg;
static lv_obj_t* bigImgLabel;
#if LVGL_VERSION_MAJOR == 8
    static lv_point_t* line_points;
#else
    static lv_point_precise_t* line_points;
#endif

static void subBtnEventCb(lv_event_t * e)
{
    void* data = lv_event_get_user_data(e);
    struct coffeeParam* param = (struct coffeeParam*)data;
    int expectValue = param->value - 1;
    if(expectValue >= param->minValue)
    {
        param->value = expectValue;
        char buf[16] = {0};
        sprintf(buf, "%d", param->value);
        lv_label_set_text(param->valueLabel, buf);
        lv_obj_invalidate(param->valueLabel);
    }
}

static void addBtnEventCb(lv_event_t * e)
{
    void* data = lv_event_get_user_data(e);
    struct coffeeParam* param = (struct coffeeParam*)data;
    int expectValue = param->value + 1;
    if(expectValue <= param->maxValue)
    {
        param->value = expectValue;
        char buf[16] = {0};
        sprintf(buf, "%d", param->value);
        lv_label_set_text(param->valueLabel, buf);
        lv_obj_invalidate(param->valueLabel);
    }
}

static void createOperate(struct coffeeParam* data ,int x, int y)
{
    lv_obj_t* img = lv_img_create(detailsScr);
    lv_obj_set_size(img, pos_width_conversion(56), pos_height_conversion(56));
    lv_obj_set_pos(img, pos_width_conversion(x), pos_height_conversion(y));
    lv_img_set_src(img, data->iconSrc);

    static lv_style_t style_def;
    if (style_def.prop_cnt > 1) {
        lv_style_reset(&style_def);
    } else {
        lv_style_init(&style_def);
    }
    lv_style_set_text_color(&style_def, lv_color_white());
    lv_obj_t* label = lv_label_create(detailsScr);
    lv_label_set_text(label, data->title);
    if (disp_size == DISP_SMALL) {
        lv_obj_set_style_text_font(label,&NotoSanCJK_Medium_16,0);
    } else {
        lv_obj_set_style_text_font(label,&NotoSanCJK_Medium_18,0);
    }
    lv_obj_add_style(label, &style_def, 0);
    lv_obj_set_pos(label, pos_width_conversion(x + 74), pos_height_conversion(y + 20));

    lv_obj_t * subBtn = lv_imgbtn_create(detailsScr);
    lv_imgbtn_set_src(subBtn, LV_IMGBTN_STATE_RELEASED, 0, &Reduce_normal, 0);
    lv_imgbtn_set_src(subBtn, LV_IMGBTN_STATE_PRESSED, 0, &Reduce_pressed, 0);
    lv_obj_set_x(subBtn, pos_width_conversion(x + 160));
    lv_obj_set_y(subBtn, pos_height_conversion(y + 14));
    lv_obj_set_size(subBtn, pos_width_conversion(32), pos_height_conversion(32));
    lv_obj_add_event_cb(subBtn, subBtnEventCb, LV_EVENT_CLICKED, (void*)data);

    lv_obj_t * addBtn = lv_imgbtn_create(detailsScr);
    lv_imgbtn_set_src(addBtn, LV_IMGBTN_STATE_RELEASED, 0, &Add_normal, 0);
    lv_imgbtn_set_src(addBtn, LV_IMGBTN_STATE_PRESSED, 0, &Add_pressed, 0);
    lv_obj_set_x(addBtn, pos_width_conversion(x + 378));
    lv_obj_set_y(addBtn, pos_height_conversion(y + 14));
    lv_obj_set_size(addBtn, pos_width_conversion(32), pos_height_conversion(32));
    lv_obj_add_event_cb(addBtn, addBtnEventCb, LV_EVENT_CLICKED, (void*)data);

    int pos = data->type * 4;
    line_points[pos].x = pos_width_conversion(x + 176);
    line_points[pos].y = pos_height_conversion(y + 14);
    line_points[pos+1].x = pos_width_conversion(x + 394);
    line_points[pos+1].y = pos_height_conversion(y + 14);
    line_points[pos+2].x = pos_width_conversion(x + 176);
    line_points[pos+2].y = pos_height_conversion(y + 45);
    line_points[pos+3].x = pos_width_conversion(x + 394);
    line_points[pos+3].y = pos_height_conversion(y + 45);
    static lv_style_t style_line;
    if (style_line.prop_cnt > 1) {
        lv_style_reset(&style_line);
    } else {
        lv_style_init(&style_line);
    }
    lv_style_set_line_width(&style_line, 1);
    lv_style_set_line_color(&style_line, lv_color_hex(0xefbe77));
    lv_obj_t* line1 = lv_line_create(detailsScr);
    lv_obj_add_style(line1, &style_line, 0);
    lv_line_set_points(line1, &line_points[pos], 2);
    lv_obj_t* line2 = lv_line_create(detailsScr);
    lv_obj_add_style(line2, &style_line, 0);
    lv_line_set_points(line2, &line_points[pos+2], 2);

    static lv_style_t style_value;
    if (style_value.prop_cnt > 1) {
        lv_style_reset(&style_value);
    } else {
        lv_style_init(&style_value);
    }
    lv_style_set_text_color(&style_value, lv_color_hex(0xefbf7b));
    lv_obj_t * label_value = lv_label_create(detailsScr);
    char buf[16] = {0};
    sprintf(buf, "%d", data->defaultValue);
    lv_label_set_text(label_value, buf);
    if (disp_size == DISP_SMALL) {
        lv_obj_set_style_text_font(label_value,&NotoSanCJK_Medium_16,0);
    } else if (disp_size == DISP_MEDIUM) {
        lv_obj_set_style_text_font(label_value,&NotoSanCJK_Medium_18, 0);
    } else {
        lv_obj_set_style_text_font(label_value,&NotoSanCJK_Medium_20, 0);
    }
    lv_obj_set_x(label_value, pos_width_conversion(x + 260));
    lv_obj_set_y(label_value, pos_height_conversion(y + 20));
    lv_obj_add_style(label_value, &style_value, 0);
    data->valueLabel = label_value;

    static lv_style_t style_unit;
    if (style_unit.prop_cnt > 1) {
        lv_style_reset(&style_unit);
    } else {
        lv_style_init(&style_unit);
    }
    lv_style_set_text_color(&style_unit, lv_color_hex(0xfffffe));
    lv_obj_t * label_unit = lv_label_create(detailsScr);
    lv_label_set_text(label_unit, data->unit);
    if (disp_size == DISP_SMALL) {
        lv_obj_set_style_text_font(label_unit,&NotoSanCJK_Medium_16,0);
    } else if (disp_size == DISP_MEDIUM) {
        lv_obj_set_style_text_font(label_unit,&NotoSanCJK_Medium_18, 0);
    } else {
        lv_obj_set_style_text_font(label_unit,&NotoSanCJK_Medium_20, 0);
    }
    lv_obj_set_x(label_unit, pos_width_conversion(x + 290));
    lv_obj_set_y(label_unit, pos_height_conversion(y + 20));
    lv_obj_add_style(label_unit, &style_unit, 0);
}

static void createOperates()
{
    int count = sizeof(coffeeParamData) / sizeof(struct coffeeParam);
#if LVGL_VERSION_MAJOR == 8
    line_points = (lv_point_t*)lv_mem_alloc(sizeof(lv_point_t) * count * 4);
#else
    line_points = (lv_point_precise_t*)lv_mem_alloc(sizeof(lv_point_precise_t) * count * 4);
#endif
    int i = 0;
    for(; i < count; i++)
    {
        createOperate(&coffeeParamData[i], 300, 112 + i* 66);
    }
}

static void backBtnEventCb(lv_event_t * e)
{
    showMenu();
}

static void createBackBtn()
{
    lv_obj_t * btn = lv_imgbtn_create(detailsScr);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, 0, Cancel, 0);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, 0, Cancel_pressed, 0);
    lv_obj_set_x(btn, pos_width_conversion(298));
    lv_obj_set_y(btn, pos_height_conversion(412));
    lv_obj_set_size(btn, pos_width_conversion(92), pos_height_conversion(32));
    lv_obj_add_event_cb(btn, backBtnEventCb, LV_EVENT_CLICKED, 0);
}

static void okBtnEventCb(lv_event_t * e)
{
    int size = sizeof(coffeeParamData) / sizeof(struct coffeeParam);
    showProduction(NULL, size);
}

static void createOkBtn()
{
    lv_obj_t * btn = lv_imgbtn_create(detailsScr);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, 0, &Ok, 0);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, 0, &Ok_pressed, 0);
    lv_obj_set_x(btn, pos_width_conversion(410));
    lv_obj_set_y(btn, pos_height_conversion(412));
    lv_obj_set_size(btn, pos_width_conversion(92), pos_height_conversion(32));
    lv_obj_add_event_cb(btn, okBtnEventCb, LV_EVENT_CLICKED, 0);
}

static void createBigImage()
{
    bigImg = lv_img_create(detailsScr);
    lv_obj_set_size(bigImg, pos_width_conversion(202), pos_height_conversion(326));
    lv_obj_set_pos(bigImg, pos_width_conversion(56), pos_height_conversion(66));

    static lv_style_t style_def;
    if (style_def.prop_cnt > 1) {
        lv_style_reset(&style_def);
    } else {
        lv_style_init(&style_def);
    }
    lv_style_set_text_color(&style_def, lv_color_white());

    bigImgLabel = lv_label_create(bigImg);
    if (disp_size == DISP_SMALL) {
        lv_obj_set_style_text_font(bigImgLabel,&NotoSanCJK_Medium_16,0);
    } else if (disp_size == DISP_MEDIUM) {
        lv_obj_set_style_text_font(bigImgLabel,&NotoSanCJK_Medium_18, 0);
    } else {
        lv_obj_set_style_text_font(bigImgLabel,&NotoSanCJK_Medium_20, 0);
    }
    lv_obj_add_style(bigImgLabel, &style_def, 0);
    lv_obj_align(bigImgLabel, LV_ALIGN_CENTER, 0, pos_height_conversion(-100));
}

static void paramDataReset()
{
    int count = sizeof(coffeeParamData) / sizeof(struct coffeeParam);
    int i = 0;
    for(; i < count; i++)
    {
        coffeeParamData[i].value = coffeeParamData[i].defaultValue;
        char buf[16] = {0};
        sprintf(buf, "%d", coffeeParamData[i].value);
        lv_label_set_text(coffeeParamData[i].valueLabel, buf);
        lv_obj_invalidate(coffeeParamData[i].valueLabel);
    }
}

static void changeBigImage()
{
    lv_img_set_src(bigImg, categoryData[currentCoffeeType].detailsSrc);
    lv_label_set_text(bigImgLabel, categoryData[currentCoffeeType].title);
    lv_obj_invalidate(bigImg);
    lv_obj_invalidate(bigImgLabel);
}

void detailsInit()
{
    detailsScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(detailsScr, lv_color_black(), 0);
    createTopTitle(detailsScr);
    createBigImage();
    createOperates();
    createBackBtn();
    createOkBtn();
}

void showDetails()
{
    paramDataReset();
    changeBigImage();
    lv_scr_load(detailsScr);
    loadScr = detailsScr;
}

void detailDel()
{
    lv_mem_free(line_points);
}
