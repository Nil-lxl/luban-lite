#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "aic_ui.h"
#include "lv_port_disp.h"
#ifdef LPKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif

#include "lvgl.h"
#include "app_weather.h"

static lv_obj_t *weather_ui;

lv_obj_t *app_weather_init(void) {

    weather_ui = lv_obj_create(NULL);

    lv_obj_set_style_bg_img_src(weather_ui, LVGL_IMAGE_PATH(main_bg.jpg), 0);

    return weather_ui;

}
