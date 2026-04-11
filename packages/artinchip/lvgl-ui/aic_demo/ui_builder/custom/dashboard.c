#include "custom.h"
#include "ui_objects.h"

static int last_high = -1;
static int last_low = -1;
static int end_degree = 250;
static int direct = 0;
static int cur_degree = 0;
static int max_speed = 99;

static lv_timer_t *point_timer = NULL;
static lv_timer_t *speed_timer = NULL;
static lv_timer_t *trip_timer = NULL;

static scr_dashboard_t *scr;
static lv_style_t style_bg;
static lv_style_t style_fp;
static lv_style_t style_roller;

#define LVGL_DASHBOARD_DIR LVGL_DIR"image/dashboard/"

static void point_timer_cb(lv_timer_t *timer);
static void speed_timer_cb(lv_timer_t *timer);
static void trip_timer_cb(lv_timer_t *timer);


void scr_dashboard_custom_load_start() {
    scr = scr_dashboard_get(&ui_manager);

    lv_style_init(&style_bg);
    lv_style_set_arc_rounded(&style_bg, 0);
    lv_style_set_arc_width(&style_bg, 0);
    lv_style_set_arc_opa(&style_bg, LV_OPA_0);

    lv_style_init(&style_fp);

    lv_style_set_arc_color(&style_fp, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_arc_rounded(&style_fp, 0);
    lv_style_set_arc_width(&style_fp, 80);
    lv_style_set_arc_img_src(&style_fp, LVGL_IMAGE_PATH(dashboard/point/normal.png));


    lv_obj_t *arc = scr->arc;
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_value(arc, 0);
    lv_arc_set_end_angle(arc, 0);

    lv_arc_set_angles(arc, 0, end_degree);
    lv_arc_set_bg_angles(arc, 0, end_degree);
    lv_arc_set_bg_angles(arc, 0, max_speed);

    lv_obj_add_style(arc, &style_fp, LV_PART_INDICATOR);
    lv_obj_add_style(arc, &style_bg, LV_PART_MAIN);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

	lv_style_init(&style_roller);
	

    point_timer = lv_timer_create(point_timer_cb, 10, 0);
    speed_timer = lv_timer_create(speed_timer_cb, 60, 0);
    trip_timer = lv_timer_create(trip_timer_cb, 1000 * 5, 0);


}

void scr_dashboard_custom_unload_start() {
    lv_timer_del(point_timer);
    lv_timer_del(speed_timer);
    lv_timer_del(trip_timer);

    cur_degree = 0;
}


static void point_timer_cb(lv_timer_t *timer) {
    lv_obj_t *arc = scr->arc;
    lv_arc_set_end_angle(arc, cur_degree);
    if (direct == 0) {
        cur_degree++;
        if (cur_degree > end_degree) {
            cur_degree = end_degree - 1;
            direct = 1;
        }
    } else {
        cur_degree--;
        if (cur_degree < 0) {
            cur_degree = 1;
            direct = 0;
        }
    }
    return;
}
static void speed_timer_cb(lv_timer_t *timer) {
    char data_str[128];
    int speed_num = ((cur_degree << 8) / end_degree * max_speed) >> 8;

    int cur_low = speed_num % 10;
    int cur_high = speed_num / 10;

    if (cur_low != last_low) {
        ui_snprintf(data_str, "%sspeed_num/s_%d.png", LVGL_DASHBOARD_DIR, speed_num%10);
        lv_img_set_src(scr->speed_low, data_str);
        last_low = cur_low;
    }

    if (cur_high != last_high) {
        ui_snprintf(data_str, "%sspeed_num/s_%d.png", LVGL_DASHBOARD_DIR, speed_num/10);
        lv_img_set_src(scr->speed_high, data_str);
        last_high = cur_high;
    }
}

static void trip_timer_cb(lv_timer_t *timer) {
    char data_str[128];
    int num[4];
    int cur;
    static int trip = 98;

    (void)timer;
    trip++;
    if (trip >= 9999)
        trip = 0;

    num[0] = trip / 1000;
    cur = trip % 1000;
    num[1] = cur / 100;
    cur = cur % 100;
    num[2] = cur / 10;
    num[3] = cur % 10;

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DASHBOARD_DIR, num[0]);
    lv_img_set_src(scr->trip_0, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DASHBOARD_DIR, num[1]);
    lv_img_set_src(scr->trip_1, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DASHBOARD_DIR, num[2]);
    lv_img_set_src(scr->trip_2, data_str);

    ui_snprintf(data_str, "%smileage/%d.png", LVGL_DASHBOARD_DIR, num[3]);
    lv_img_set_src(scr->trip_3, data_str);
}

void scr_dashboard_mode_custom_value_changed() {
    char *mode = lv_malloc(32);
	lv_roller_get_selected_str(scr->mode, mode, 32);
	
	if(strcmp(mode, "normal") == 0){
		lv_img_set_src(scr->bg, LVGL_IMAGE_PATH(dashboard/bg/normal.jpg));
		lv_style_set_arc_img_src(&style_fp, LVGL_IMAGE_PATH(dashboard/point/normal.png));
		lv_obj_set_style_arc_image_src(scr->arc, LVGL_IMAGE_PATH(dashboard/point/normal.png), 0);
	}else if(strcmp(mode, "echo") == 0){
		lv_img_set_src(scr->bg, LVGL_IMAGE_PATH(dashboard/bg/echo.jpg));
		lv_style_set_arc_img_src(&style_fp, LVGL_IMAGE_PATH(dashboard/point/echo.png));
	}else if(strcmp(mode, "rain") == 0){
		lv_img_set_src(scr->bg, LVGL_IMAGE_PATH(dashboard/bg/rain.jpg));
		lv_style_set_arc_img_src(&style_fp, LVGL_IMAGE_PATH(dashboard/point/rain.png));
	}else if(strcmp(mode, "sport") == 0){
		lv_img_set_src(scr->bg, LVGL_IMAGE_PATH(dashboard/bg/sport.jpg));
		lv_style_set_arc_img_src(&style_fp, LVGL_IMAGE_PATH(dashboard/point/sport.png));
	}
   
}

