#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mpp_fb.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "aic_ui.h"
#include "app_ui.h"

#ifdef AIC_WIRELESS_LAN
#include "wifi_interactive_mode.h"
#endif

static lv_obj_t *wifi_ui;

lv_obj_t *app_wifi_init() {
    wifi_ui = lv_obj_create(NULL);
    // lv_obj_set_style_bg_color(wifi_ui, lv_color_hex(0xffff00), 0);

    return wifi_ui;
}