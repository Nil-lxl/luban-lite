/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  ArtInChip
 */

#include "cookbook_comp.h"
#include "string.h"

#define EMPHASIZE_COLOR    lv_color_hex(0xffc53d)
#define SECONDARY_COLOR    lv_color_hex(0x272729)
#define PRIMARY_COLOR      lv_color_hex(0x1A1D26)

static lv_coord_t main_row_dsc[MAX_MAIN_INGREDIENTS];
static lv_coord_t main_col_dsc[MAX_MAIN_INGREDIENTS];
static lv_coord_t supporting_row_dsc[MAX_MAIN_INGREDIENTS];
static lv_coord_t supporting_col_dsc[MAX_MAIN_INGREDIENTS];

static void cookbook_head_create(lv_obj_t *obj, cookbook_data_t *data);
static void cookbook_introduce_create(lv_obj_t *obj, cookbook_data_t *data);
static void cookbook_ingredients_create(lv_obj_t *obj, cookbook_data_t *data);
static void cookbook_cooking_steps_create(lv_obj_t *obj, cookbook_data_t *data);

static void cookbook_blank_line_create(lv_obj_t *obj, lv_coord_t height);
static void cookbook_ingredients_table_create(lv_obj_t *obj, char ingredients[][INGREDIENTS_TEXT_LEN],
                                              char ingredients_weight[][INGREDIENTS_TEXT_LEN], int main_ingredients);

lv_obj_t *cookbook_comp_create(lv_obj_t *parent)
{
    lv_obj_t *cookbook = lv_obj_create(parent);
    lv_obj_remove_style_all(cookbook);
    lv_obj_set_size(cookbook, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_img_src(cookbook, LVGL_PATH(steam_box/steam_box_bg.png), 0);
    lv_obj_set_style_bg_img_opa(cookbook, LV_OPA_100, 0);
    lv_obj_set_flex_flow(cookbook, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cookbook, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_flex_grow(cookbook, 0, 0);
    lv_obj_add_flag(cookbook, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(cookbook, LV_DIR_VER);

    /*Create a style for the scrollbars*/
    static lv_style_t style_rollbars;
    if (style_rollbars.prop_cnt > 1) {
        lv_style_reset(&style_rollbars);
    } else {
        lv_style_init(&style_rollbars);
    }

    lv_style_set_height(&style_rollbars, LV_VER_RES * 0.2);
    lv_style_set_width(&style_rollbars, LV_HOR_RES * 0.01);
    lv_style_set_pad_right(&style_rollbars, LV_HOR_RES * 0.01);  /*Space from the parallel side*/
    lv_style_set_pad_top(&style_rollbars, LV_HOR_RES * 0.01);    /*Space from the perpendicular side*/

    lv_style_set_radius(&style_rollbars, 4);
    lv_style_set_bg_opa(&style_rollbars, LV_OPA_100);
    lv_style_set_bg_color(&style_rollbars, EMPHASIZE_COLOR);

    lv_obj_add_style(cookbook, &style_rollbars, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    lv_obj_add_style(cookbook, &style_rollbars, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    for (int i = 0; i < MAX_MAIN_INGREDIENTS; i++) {
        main_col_dsc[i] = LV_GRID_TEMPLATE_LAST;
        main_row_dsc[i] = LV_GRID_TEMPLATE_LAST;
        supporting_col_dsc[i] = LV_GRID_TEMPLATE_LAST;
        supporting_row_dsc[i] = LV_GRID_TEMPLATE_LAST;
    }
    return cookbook;
}

void cookbook_comp_set_data(lv_obj_t *obj, cookbook_data_t *data)
{
    if (data == NULL)
        return;

    cookbook_head_create(obj, data);
    cookbook_introduce_create(obj, data);
    cookbook_ingredients_create(obj, data);
    cookbook_cooking_steps_create(obj, data);
}

static void cookbook_head_create(lv_obj_t *obj, cookbook_data_t *data)
{
    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);

    lv_obj_t *cook_head = NULL;
    if (strstr(data->cookbook_title, LVGL_DIR) == NULL) {
        cook_head = lv_label_create(obj);
        lv_label_set_text(cook_head, (const char*)data->cookbook_title);
        lv_label_set_recolor(cook_head, 1);
        lv_obj_set_style_text_color(cook_head, lv_color_hex(0xffc53d), 0);
    } else {
        cook_head = lv_img_create(obj);
        lv_img_set_src(cook_head, (const char*)data->cookbook_title);
    }
    cookbook_blank_line_create(obj, LV_VER_RES * 0.1);

    lv_obj_t *food_img = lv_img_create(obj);
    lv_img_set_src(food_img, (const char*)data->food_path);
    lv_obj_center(food_img);
    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);

    lv_obj_t *additional_info = lv_label_create(obj);
    lv_label_set_long_mode(additional_info, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(additional_info, (const char*)data->additional_info);
    lv_label_set_recolor(additional_info, 1);
    lv_obj_set_style_text_color(additional_info, lv_color_hex(0xf0f0f0), 0);
    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);
}

static void cookbook_introduce_create(lv_obj_t *obj, cookbook_data_t *data)
{
    lv_obj_t *introduce_name_container = lv_obj_create(obj);
    lv_obj_remove_style_all(introduce_name_container);
    lv_obj_set_size(introduce_name_container, lv_pct(95), lv_pct(10));
    lv_obj_t *food_name = NULL;
    if (strstr(data->cookbook_title, LVGL_DIR) == NULL) {
        food_name = lv_label_create(introduce_name_container);
        lv_label_set_text(food_name, (const char*)data->food_name);
        lv_label_set_recolor(food_name, 1);
        lv_obj_set_style_text_color(food_name, lv_color_hex(0xffc53d), 0);
    } else {
        food_name = lv_img_create(introduce_name_container);
        lv_img_set_src(food_name, (const char*)data->food_name);
    }
    lv_obj_set_align(food_name, LV_ALIGN_LEFT_MID);
    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);

    lv_obj_t *additional_info = lv_label_create(obj);
    lv_label_set_long_mode(additional_info, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(additional_info, (const char *)data->food_introduction);
    lv_label_set_recolor(additional_info, 1);
    lv_obj_set_style_text_color(additional_info, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_size(additional_info, lv_pct(95), LV_SIZE_CONTENT);
    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);
}

static void cookbook_ingredients_create(lv_obj_t *obj, cookbook_data_t *data)
{
    lv_obj_t *ingredients_title_container = lv_obj_create(obj);
    lv_obj_remove_style_all(ingredients_title_container);
    lv_obj_set_size(ingredients_title_container, lv_pct(95), lv_pct(10));
    lv_obj_t *ingredients_title = lv_img_create(ingredients_title_container);
    lv_img_set_src(ingredients_title, LVGL_PATH(steam_box/cookbook/main_supporting_img.png));
    lv_obj_set_align(ingredients_title, LV_ALIGN_LEFT_MID);

    lv_obj_t *main_label_container = lv_obj_create(obj);
    lv_obj_remove_style_all(main_label_container);
    lv_obj_set_size(main_label_container, lv_pct(95), lv_pct(10));
    lv_obj_t *main_ingredients = lv_label_create(main_label_container);
    lv_obj_set_align(main_ingredients, LV_ALIGN_LEFT_MID);
    lv_label_set_text(main_ingredients, "Main Ingredients");
    lv_label_set_recolor(main_ingredients, 1);
    lv_obj_set_style_text_color(main_ingredients, lv_color_hex(0xf0f0f0), 0);

    cookbook_ingredients_table_create(obj, data->main_ingredients,
                                      data->main_ingredients_weight, 1);

    lv_obj_t *supporting_label_container = lv_obj_create(obj);
    lv_obj_remove_style_all(supporting_label_container);
    lv_obj_set_size(supporting_label_container, lv_pct(95), lv_pct(10));
    lv_obj_t *supporting_ingredients = lv_label_create(supporting_label_container);
    lv_label_set_text(supporting_ingredients, "Supporting Ingredients");
    lv_label_set_recolor(supporting_ingredients, 1);
    lv_obj_set_style_text_color(supporting_ingredients, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_align(ingredients_title, LV_ALIGN_LEFT_MID);
    cookbook_ingredients_table_create(obj, data->supporting_ingredients,

                                      data->supporting_ingredients_weight, 0);

    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);
}

static void cookbook_cooking_steps_create(lv_obj_t *obj, cookbook_data_t *data)
{
    lv_obj_t *cook_steps_title_container = lv_obj_create(obj);
    lv_obj_remove_style_all(cook_steps_title_container);
    lv_obj_set_size(cook_steps_title_container, lv_pct(95), lv_pct(10));
    lv_obj_t *cook_steps_title = lv_img_create(cook_steps_title_container);
    lv_img_set_src(cook_steps_title, LVGL_PATH(steam_box/cookbook/cooking_steps.png));
    lv_obj_set_align(cook_steps_title, LV_ALIGN_LEFT_MID);

    lv_obj_t *cooking_steps = lv_label_create(obj);
    lv_label_set_long_mode(cooking_steps, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(cooking_steps, (const char*)data->cooking_steps);
    lv_label_set_recolor(cooking_steps, 1);
    lv_obj_set_style_text_color(cooking_steps, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_size(cooking_steps, lv_pct(95), LV_SIZE_CONTENT);

    cookbook_blank_line_create(obj, LV_VER_RES * 0.05);

    lv_obj_t *note = lv_label_create(obj);
    lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(note, (const char*)data->note);
    lv_label_set_recolor(note, 1);
    lv_obj_set_style_text_color(note, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_size(note, lv_pct(95), LV_SIZE_CONTENT);
}

static void cookbook_blank_line_create(lv_obj_t *obj, lv_coord_t height)
{
    lv_obj_t *blank = lv_obj_create(obj);
    lv_obj_remove_style_all(blank);
    lv_obj_set_size(blank, lv_pct(100), height);
}

static int cookbook_get_table_row(char ingredients[][INGREDIENTS_TEXT_LEN], int *row)
{
    int ingredient_num = 0;
    while(strlen(ingredients[ingredient_num]) != 0) {
        ingredient_num++;
    }

    *row = ingredient_num / 2;
    if (ingredient_num % 2)
        (*row)++;

    return ingredient_num;
}

static void _cookbook_ingredients_table_create(lv_obj_t *obj, char ingredients[][INGREDIENTS_TEXT_LEN],
                                               char ingredients_weight[][INGREDIENTS_TEXT_LEN],
                                               lv_coord_t *col_dsc, lv_coord_t *row_dsc)
{
    int i = 0;
    int ingredient_num = 0, row = 0;
    const int row_height = LV_VER_RES * 0.16;
    const int col_width = LV_HOR_RES * 0.5;

    ingredient_num = cookbook_get_table_row(ingredients, &row);

    col_dsc[0] = LV_GRID_FR(1); /* obj width */
    col_dsc[1] = LV_GRID_FR(1); /* obj width */

    for (i = 0; i <= row; i++) {
        row_dsc[i] = row_height;
    }

    lv_obj_t *table = lv_obj_create(obj);
    lv_obj_remove_style_all(table);
    lv_obj_set_size(table, lv_pct(95), (row_height * row));
    lv_obj_set_style_grid_row_dsc_array(table, row_dsc, 0);
    lv_obj_set_style_grid_column_dsc_array(table, col_dsc, 0);
    lv_obj_set_layout(table, LV_LAYOUT_GRID);

    for (i = 0; i < ingredient_num; i++) {
        int now_row = i / 2;
        int now_col = i % 2;

        lv_obj_t *container = lv_obj_create(table);
        lv_obj_remove_style_all(container);
        lv_obj_set_style_radius(container, 8, 0);
        lv_obj_set_style_bg_color(container, SECONDARY_COLOR, 0);
        lv_obj_set_style_bg_opa(container, LV_OPA_100, 0);
        lv_obj_set_size(container, col_width * 0.9, row_height * 0.85);
        lv_obj_set_grid_cell(container, LV_GRID_ALIGN_START, now_col, 1,
                             LV_GRID_ALIGN_CENTER, now_row, 1);

        lv_obj_t *label = lv_label_create(container);
        lv_label_set_text(label, (const char*)ingredients[i]);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_recolor(label, 1);
        lv_obj_set_style_text_color(label, lv_color_hex(0xf0f0f0), 0);
        lv_obj_set_pos(label, LV_HOR_RES * 0.01, 0);
        lv_obj_set_align(label, LV_ALIGN_LEFT_MID);

        lv_obj_t *label_weight = lv_label_create(container);
        lv_label_set_text(label_weight, (const char*)ingredients_weight[i]);
        lv_obj_set_pos(label_weight, -LV_HOR_RES * 0.01 ,0);
        lv_label_set_long_mode(label_weight, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_align(label_weight, LV_ALIGN_RIGHT_MID);
        lv_label_set_recolor(label_weight, 1);
        lv_obj_set_style_text_color(label_weight, lv_color_hex(0xf0f0f0), 0);
    }
}

static void cookbook_ingredients_table_create(lv_obj_t *obj, char ingredients[][INGREDIENTS_TEXT_LEN],
                                              char ingredients_weight[][INGREDIENTS_TEXT_LEN], int main_ingredients)
{
    if (main_ingredients) {
        return _cookbook_ingredients_table_create(obj, ingredients,ingredients_weight,
                                                  main_col_dsc, main_row_dsc);
    }

    return _cookbook_ingredients_table_create(obj, ingredients,ingredients_weight,
                                              supporting_col_dsc, supporting_row_dsc);
}
