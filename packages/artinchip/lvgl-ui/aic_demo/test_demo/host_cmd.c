#include "aic_core.h"
#include "aic_ui.h"

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"

#include "lvgl.h"
#include "test_ui.h"

#define LOG_TAG "HOST_CMD"

static int line_width = 1;

static lv_obj_t *scr;
static uint8_t sum = 0;

static lv_obj_t *black_stripe;
static lv_obj_t *white_stripe;
static int direction = 0;  // 0 horizontal, 1 vertical

static void lv_set_bg_color(lv_obj_t *obj, int color_hex) {
    lv_obj_set_style_bg_color(scr, lv_color_hex(color_hex), 0);
}

void show_color_timer(lv_timer_t *timer);

void test_control(void) {
    scr = lv_scr_act();
    // lv_obj_set_style_bg_color(scr, lv_color_hex(0x00ffff), 0);

    lv_timer_t *timer;
    timer = lv_timer_create(show_color_timer, 20 * 60 * 1000, NULL);
}
void show_color_timer(lv_timer_t *timer) {
    switch (sum) {
        case 0:
            lv_set_bg_color(scr, 0xffff00);
            break;
        case 1:
            lv_set_bg_color(scr, 0x00ffff);
            break;
        case 2:
            lv_set_bg_color(scr, 0x808080);
            break;
        case 3:
            lv_set_bg_color(scr, 0x000000);
            break;
        default:
            break;
    }
    sum = (sum + 1) % 4;
}

void play_images(int argc, char *argv[]) {

    LOG_I("play_images\n");
}

MSH_CMD_EXPORT(play_images, playimages);