/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "lvgl/lvgl.h"

#ifndef COFFEE_GLOBAL_H_
#define COFFEE_GLOBAL_H_

LV_FONT_DECLARE(NotoSanCJK_Medium_16);
LV_FONT_DECLARE(NotoSanCJK_Medium_18);
LV_FONT_DECLARE(NotoSanCJK_Medium_20);

enum coffeeType{
	CT_Americano = 0,		// Americano coffee
	CT_DoubleEspresso,		// Double Espresso
	CT_Cappuccino,			// Cappuccino
	CT_Hotwater,			// Hot water
	CT_CafeLatte,			// Cafe Latte
	CT_Espresso,			// Espresso
	CT_Mocha,				// Mocha
};

enum paramType{
	PT_coffeeBean = 0,		// Coffee beans
	PT_coffeeCnt,			// Coffee quantity
	PT_temperature,			// Temperature
	PT_milk,				// Milk
};

struct category{
	enum coffeeType type;		// Coffee type
	char title[64];		        // Coffee title
	const void* menuSrc;		// Menu page coffee icon
	const void* detailsSrc;		// Details page coffee icon
	const void* productionSrc;	// Production page coffee icon
};

struct coffeeParam{
	enum paramType type;		// Parameter type
	const void* iconSrc;		// Parameter icon
	char title[32];				// Parameter title
	int defaultValue;			// Default value
	char unit[16];				// Unit symbol
	int value;					// Current value
	int minValue;				// Minimum value
	int maxValue;				// Maximum value
	lv_obj_t * valueLabel;		// Label control to display the current value
};

extern struct category* categoryData;
extern enum coffeeType currentCoffeeType;
extern lv_obj_t* menuScr[10];
extern lv_obj_t* detailsScr;
extern lv_obj_t* productionScr;
extern lv_obj_t* loadScr;

void createTopTitle(lv_obj_t* scr);
void globalSrcDel(void);

#endif /* COFFEE_GLOBAL_H_ */
