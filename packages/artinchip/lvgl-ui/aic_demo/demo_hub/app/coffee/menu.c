/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "global.h"
#include "menu.h"
#include "details.h"
#include "aic_ui.h"
#include "coffee.h"

LV_IMG_DECLARE(Left);
LV_IMG_DECLARE(Next);
LV_IMG_DECLARE(Setting);
LV_IMG_DECLARE(Left_pressed);
LV_IMG_DECLARE(Next_pressed);
LV_IMG_DECLARE(Setting_pressed);

LV_IMG_DECLARE(produceBG);
LV_IMG_DECLARE(produceBG_ON);
LV_IMG_DECLARE(Americano);
LV_IMG_DECLARE(DoubleEspresso);
LV_IMG_DECLARE(Cappuccino);
LV_IMG_DECLARE(Hotwater);
LV_IMG_DECLARE(CafeLatte);
LV_IMG_DECLARE(Espresso);
LV_IMG_DECLARE(Mocha);
LV_IMG_DECLARE(Americano_big);
LV_IMG_DECLARE(DoubleEspresso_big);
LV_IMG_DECLARE(Cappuccino_big);
LV_IMG_DECLARE(Hotwater_big);
LV_IMG_DECLARE(CafeLatte_big);
LV_IMG_DECLARE(Espresso_big);
LV_IMG_DECLARE(Mocha_big);
LV_IMG_DECLARE(Americano_in_production);
LV_IMG_DECLARE(DoubleEspresso_in_production);
LV_IMG_DECLARE(Cappuccino_in_production);
LV_IMG_DECLARE(Hotwater_in_production);
LV_IMG_DECLARE(CafeLatte_in_production);
LV_IMG_DECLARE(Espresso_in_production);
LV_IMG_DECLARE(Mocha_in_production);

#if USE_ARRAY_IMG

LV_IMG_DECLARE(Left);
LV_IMG_DECLARE(Next);
LV_IMG_DECLARE(Setting);
LV_IMG_DECLARE(Left_pressed);
LV_IMG_DECLARE(Next_pressed);
LV_IMG_DECLARE(Setting_pressed);


LV_IMG_DECLARE(produceBG);
LV_IMG_DECLARE(produceBG_ON);
LV_IMG_DECLARE(Americano);
LV_IMG_DECLARE(DoubleEspresso);
LV_IMG_DECLARE(Cappuccino);
LV_IMG_DECLARE(Hotwater);
LV_IMG_DECLARE(CafeLatte);
LV_IMG_DECLARE(Espresso);
LV_IMG_DECLARE(Mocha);
LV_IMG_DECLARE(Americano_big);
LV_IMG_DECLARE(DoubleEspresso_big);
LV_IMG_DECLARE(Cappuccino_big);
LV_IMG_DECLARE(Hotwater_big);
LV_IMG_DECLARE(CafeLatte_big);
LV_IMG_DECLARE(Espresso_big);
LV_IMG_DECLARE(Mocha_big);
LV_IMG_DECLARE(Americano_in_production);
LV_IMG_DECLARE(DoubleEspresso_in_production);
LV_IMG_DECLARE(Cappuccino_in_production);
LV_IMG_DECLARE(Hotwater_in_production);
LV_IMG_DECLARE(CafeLatte_in_production);
LV_IMG_DECLARE(Espresso_in_production);
LV_IMG_DECLARE(Mocha_in_production);

static struct category _category[] = {
    {CT_Americano,      "美式咖啡",     &Americano,         &Americano_big,         &Americano_in_production},
    {CT_DoubleEspresso, "经典咖啡", &DoubleEspresso,    &DoubleEspresso_big,    &DoubleEspresso_in_production},
    {CT_Cappuccino,     "卡布奇诺",     &Cappuccino,        &Cappuccino_big,        &Cappuccino_in_production},
    {CT_Hotwater,       "热水",         &Hotwater,          &Hotwater_big,          &Hotwater_in_production},
    {CT_CafeLatte,      "拿铁咖啡",     &CafeLatte,         &CafeLatte_big,         &CafeLatte_in_production},
    {CT_Espresso,       "意式咖啡", &Espresso,          &Espresso_big,          &Espresso_in_production},
    {CT_Mocha,          "摩卡",         &Mocha,             &Mocha_big,             &Mocha_in_production},
    /* To add a new menu, you can include it in the array by adding its type, name, and three image sources */
    /* The following is an example of a test addition */
    {CT_Americano,      "美式咖啡2",    &Americano,         &Americano_big,         &Americano_in_production},
    {CT_DoubleEspresso, "经典咖啡2",    &DoubleEspresso,    &DoubleEspresso_big,    &DoubleEspresso_in_production},
    {CT_Cappuccino,     "卡布奇诺2",    &Cappuccino,        &Cappuccino_big,        &Cappuccino_in_production},
    {CT_Hotwater,       "热水2",    &Hotwater,          &Hotwater_big,          &Hotwater_in_production},
    {CT_CafeLatte,      "拿铁咖啡2",    &CafeLatte,         &CafeLatte_big,         &CafeLatte_in_production},
    {CT_Espresso,       "意式咖啡2",    &Espresso,          &Espresso_big,          &Espresso_in_production},
    {CT_Mocha,          "摩卡2",    &Mocha,             &Mocha_big,             &Mocha_in_production},
    {CT_Americano,      "美式咖啡3",    &Americano,         &Americano_big,         &Americano_in_production},
    {CT_DoubleEspresso, "经典咖啡3",    &DoubleEspresso,    &DoubleEspresso_big,    &DoubleEspresso_in_production},
    {CT_Cappuccino,     "卡布奇诺3",    &Cappuccino,        &Cappuccino_big,        &Cappuccino_in_production},

};
#else
#define Left                        LVGL_PATH(coffee_demo/Left.png)
#define Next                        LVGL_PATH(coffee_demo/Next.png)
#define Setting                     LVGL_PATH(coffee_demo/Setting.png)
#define Left_pressed                LVGL_PATH(coffee_demo/Left_pressed.png)
#define Next_pressed                LVGL_PATH(coffee_demo/Next_pressed.png)
#define Setting_pressed             LVGL_PATH(coffee_demo/Setting_pressed.png)

#define produceBG                   LVGL_PATH(coffee_demo/produceBG.png)
#define produceBG_ON                LVGL_PATH(coffee_demo/produceBG_ON.png)
#define Americano                   LVGL_PATH(coffee_demo/Americano.png)
#define DoubleEspresso              LVGL_PATH(coffee_demo/DoubleEspresso.png)
#define Cappuccino                  LVGL_PATH(coffee_demo/Cappuccino.png)
#define Hotwater                    LVGL_PATH(coffee_demo/Hotwater.png)
#define CafeLatte                   LVGL_PATH(coffee_demo/CafeLatte.png)
#define Espresso                    LVGL_PATH(coffee_demo/Espresso.png)
#define Mocha                       LVGL_PATH(coffee_demo/Mocha.png)
#define Americano_big               LVGL_PATH(coffee_demo/Americano_big.png)
#define DoubleEspresso_big          LVGL_PATH(coffee_demo/DoubleEspresso_big.png)
#define Cappuccino_big              LVGL_PATH(coffee_demo/Cappuccino_big.png)
#define Hotwater_big                LVGL_PATH(coffee_demo/Hotwater_big.png)
#define CafeLatte_big               LVGL_PATH(coffee_demo/CafeLatte_big.png)
#define Espresso_big                LVGL_PATH(coffee_demo/Espresso_big.png)
#define Mocha_big                   LVGL_PATH(coffee_demo/Mocha_big.png)
#define Americano_in_production     LVGL_PATH(coffee_demo/Americano_in_production.png)
#define DoubleEspresso_in_production                   LVGL_PATH(coffee_demo/DoubleEspresso_in_production.png)
#define Cappuccino_in_production              LVGL_PATH(coffee_demo/Cappuccino_in_production.png)
#define Hotwater_in_production                  LVGL_PATH(coffee_demo/Hotwater_in_production.png)
#define CafeLatte_in_production                   LVGL_PATH(coffee_demo/CafeLatte_in_production.png)
#define Espresso_in_production                LVGL_PATH(coffee_demo/Espresso_in_production.png)
#define Mocha_in_production                   LVGL_PATH(coffee_demo/Mocha_in_production.png)

static struct category _category[] = {
    {CT_Americano,      "美式咖啡",     Americano,          Americano_big,      Americano_in_production},
    {CT_DoubleEspresso, "经典咖啡",     DoubleEspresso,     DoubleEspresso_big, DoubleEspresso_in_production},
    {CT_Cappuccino,     "卡布奇诺",     Cappuccino,         Cappuccino_big,     Cappuccino_in_production},
    {CT_Hotwater,       "热水",         Hotwater,           Hotwater_big,           Hotwater_in_production},
    {CT_CafeLatte,      "拿铁咖啡",     CafeLatte,          CafeLatte_big,          CafeLatte_in_production},
    {CT_Espresso,       "意式咖啡",     Espresso,           Espresso_big,           Espresso_in_production},
    {CT_Mocha,          "摩卡",         Mocha,              Mocha_big,              Mocha_in_production},
    /* To add a new menu, you can include it in the array by adding its type, name, and three image sources */
    /* The following is an example of a test addition */
    {CT_Americano,      "美式咖啡2",    Americano,      Americano_big,      Americano_in_production},
    {CT_DoubleEspresso, "经典咖啡2",    DoubleEspresso, DoubleEspresso_big, DoubleEspresso_in_production},
    {CT_Cappuccino,     "卡布奇诺2",    Cappuccino,     Cappuccino_big,     Cappuccino_in_production},
    {CT_Hotwater,       "热水2",    Hotwater,           Hotwater_big,       Hotwater_in_production},
    {CT_CafeLatte,      "拿铁咖啡2",    CafeLatte,      CafeLatte_big,      CafeLatte_in_production},
    {CT_Espresso,       "意式咖啡2",    Espresso,       Espresso_big,       Espresso_in_production},
    {CT_Mocha,          "摩卡2",    Mocha,              Mocha_big,          Mocha_in_production},
    {CT_Americano,      "美式咖啡3",    Americano,      Americano_big,      Americano_in_production},
    {CT_DoubleEspresso, "经典咖啡3",    DoubleEspresso, DoubleEspresso_big, DoubleEspresso_in_production},
    {CT_Cappuccino,     "卡布奇诺3",    Cappuccino,     Cappuccino_big,     Cappuccino_in_production},
};
#endif

static int currentPage = 0;
static int endPage = -1;
#if LVGL_VERSION_MAJOR == 8
    static lv_point_t* line_points[10];
#else
    static lv_point_precise_t* line_points[10];
#endif

static void menuBtnEventCb(lv_event_t * e)
{
    void* data = lv_event_get_user_data(e);
    currentCoffeeType = (enum coffeeType)data;
    showDetails();
}

static void createMenuBtn(lv_obj_t* scr, enum coffeeType type, int x, int y)
{
    lv_obj_t * btn = lv_imgbtn_create(scr);
    lv_obj_t* img = lv_img_create(btn);
    lv_img_set_src(img, categoryData[type].menuSrc);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, 0, &produceBG, 0);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, 0, &produceBG_ON, 0);
    lv_obj_set_x(btn, x);
    lv_obj_set_y(btn, y);
    lv_obj_set_size(btn, pos_width_conversion(168), pos_height_conversion(168));
    lv_obj_add_event_cb(btn, menuBtnEventCb, LV_EVENT_CLICKED, (void*)type);

    static lv_style_t style_def;
    if (style_def.prop_cnt > 1) {
        lv_style_reset(&style_def);
    } else {
        lv_style_init(&style_def);
    }
    lv_style_set_text_color(&style_def, lv_color_white());

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, categoryData[type].title);
    lv_obj_set_style_text_font(label,&NotoSanCJK_Medium_16,0);
    lv_obj_add_style(label, &style_def, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, pos_height_conversion(-58));
}

static void pageBtnEventCb(lv_event_t * e)
{
    void* data = lv_event_get_user_data(e);
    uintptr_t value = (uintptr_t)(data);
    if(value == -1)
    {
        -- currentPage;
        if(currentPage < 0)
            currentPage = endPage;
    }
    else
    {
        ++ currentPage;
        if(currentPage > endPage)
            currentPage = 0;
    }
    showMenu();
}

static void createPageBtn(lv_obj_t* scr)
{
    lv_obj_t * leftBtn = lv_imgbtn_create(scr);
    lv_imgbtn_set_src(leftBtn, LV_IMGBTN_STATE_RELEASED, 0, &Left, 0);
    lv_imgbtn_set_src(leftBtn, LV_IMGBTN_STATE_PRESSED, 0, &Left_pressed, 0);
    lv_obj_set_size(leftBtn, pos_width_conversion(34), pos_height_conversion(34));
    lv_obj_set_x(leftBtn, pos_width_conversion(12));
    lv_obj_set_y(leftBtn, pos_height_conversion(227));
    lv_obj_add_event_cb(leftBtn, pageBtnEventCb, LV_EVENT_CLICKED, (void*)-1);

    lv_obj_t * rightBtn = lv_imgbtn_create(scr);
    lv_imgbtn_set_src(rightBtn, LV_IMGBTN_STATE_RELEASED, 0, &Next, 0);
    lv_imgbtn_set_src(rightBtn, LV_IMGBTN_STATE_PRESSED, 0, &Next_pressed, 0);
    lv_obj_set_size(rightBtn, pos_width_conversion(34), pos_height_conversion(34));
    lv_obj_set_x(rightBtn, pos_width_conversion(754));
    lv_obj_set_y(rightBtn, pos_height_conversion(227));
    lv_obj_add_event_cb(rightBtn, pageBtnEventCb, LV_EVENT_CLICKED, (void*)1);
}

static void settingBtnEventCb(lv_event_t * e)
{

}

void createSettingBtn(lv_obj_t* scr)
{
    lv_obj_t * btn = lv_imgbtn_create(scr);
    lv_obj_t* img = lv_img_create(btn);
    lv_img_set_src(img, &Setting);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, 0, &Setting, 0);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, 0, &Setting_pressed, 0);
    lv_obj_set_size(btn, pos_width_conversion(34), pos_height_conversion(34));
    lv_obj_set_x(btn, pos_width_conversion(754));
    lv_obj_set_y(btn, pos_height_conversion(10));
    lv_obj_add_event_cb(btn, settingBtnEventCb, LV_EVENT_CLICKED, NULL);
}

void createPageLed(lv_obj_t* scr, int size, int pos)
{
    static lv_style_t style_led;
    if (style_led.prop_cnt > 1) {
        lv_style_reset(&style_led);
    } else {
        lv_style_init(&style_led);
    }
    lv_style_set_border_width(&style_led, 0);
    lv_style_set_outline_width(&style_led, 0);
    lv_style_set_shadow_width(&style_led, 0);

    static lv_style_t style_line;
    if (style_line.prop_cnt > 1) {
        lv_style_reset(&style_line);
    } else {
        lv_style_init(&style_line);
    }
    lv_style_set_line_width(&style_line, 6);
    lv_style_set_line_color(&style_line, lv_color_hex(0xefbf7b));
    lv_style_set_line_rounded(&style_line, true);

    int x = (800 - (18 + (size - 1)*12)) / 2;
    int y = 444;
    int i = 0;
    for(i = 0; i < size; i ++)
    {
        if(i == pos)
        {
#if LVGL_VERSION_MAJOR == 8
            line_points[pos] = lv_mem_alloc(sizeof(lv_point_t)*2);
#else
            line_points[pos] = lv_mem_alloc(sizeof(lv_point_precise_t)*2);
#endif
            line_points[pos][0].x = pos_width_conversion(x);
            line_points[pos][0].y = pos_height_conversion(y + 3);
            line_points[pos][1].x = pos_width_conversion(x + 18);
            line_points[pos][1].y = pos_height_conversion(y + 3);
            lv_obj_t* line = lv_line_create(scr);
            lv_obj_add_style(line, &style_line, 0);
            lv_line_set_points(line, line_points[pos], 2);
            x += (18 + 6);
        }
        else
        {
            lv_obj_t * led  = lv_led_create(scr);
            lv_obj_set_x(led, pos_width_conversion(x));
            lv_obj_set_y(led, pos_height_conversion(y));
            lv_obj_set_size(led, pos_width_conversion(6), pos_height_conversion(6));
            lv_led_set_color(led, lv_color_hex(0x2e2e2e));
            lv_obj_add_style(led, &style_led, 0);
            lv_led_on(led);
            x += (6 + 6);
        }
    }
}

void menuInit()
{
    categoryData = _category;
    int count = sizeof(_category) / sizeof(struct category);
    int pageTotal = count / 7 + (count % 7 == 0 ? 0 : 1);
    int i = 0;
    int page = -1;

    currentPage = 0;
    endPage = -1;
    for(; i < count; i ++)
    {
        int pos = i % 7;
        if(pos == 0)
        {
            page ++;
            menuScr[page] = lv_obj_create(NULL);
            lv_obj_set_style_bg_color(menuScr[page] , lv_color_black(), 0);
            // lv_obj_set_style_bg_color(menuScr[page] , lv_color_hex(0xefbf7b), 0);
            createTopTitle(menuScr[page]);
            createSettingBtn(menuScr[page]);

            if(pageTotal > 1)
            {
                createPageBtn(menuScr[page]);
                createPageLed(menuScr[page], pageTotal, page);
            }
        }

        if(pos < 4)
            createMenuBtn(menuScr[page], i, pos_width_conversion(56 + pos * (168 + 5)), pos_height_conversion(70));
        else
            createMenuBtn(menuScr[page], i, pos_width_conversion(143 + (pos - 4) * (168 + 5)), pos_height_conversion(254));
    }
    endPage = page;
}

void showMenu()
{
    lv_scr_load(menuScr[currentPage]);
    loadScr = menuScr[currentPage];
}

lv_obj_t *getNowMenuScr(void)
{
    loadScr = menuScr[currentPage];
    return menuScr[currentPage];
}

void menuDel()
{
    int i = 0;

    for (i = 0; i < 10; i++) {
        if (line_points[i])
            lv_mem_free(line_points[i]);
    }
}
