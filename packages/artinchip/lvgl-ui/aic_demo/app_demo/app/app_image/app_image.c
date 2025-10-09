#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "aic_ui.h"

#include "lvgl.h"
#include "app_image.h"

#define PICTURE_PATH(y) CONN(LVGL_DIR"image/picture/", y)

#define IMAGE_NUM 4

static lv_obj_t *image_ui;
static int image_seq = 1;

static void image_toggle(void) {
    switch (image_seq) {
        case 1:
            lv_obj_set_style_bg_img_src(image_ui, PICTURE_PATH(img1.jpg), 0);
            break;
        case 2:
            lv_obj_set_style_bg_img_src(image_ui, PICTURE_PATH(img2.jpg), 0);
            break;
        case 3:
            lv_obj_set_style_bg_img_src(image_ui, PICTURE_PATH(img3.jpg), 0);
            break;
        case 4:
            lv_obj_set_style_bg_img_src(image_ui, PICTURE_PATH(img4.jpg), 0);
            break;
        default:
            break;
    }
}

static void gesture_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_indev_wait_release(indev);
        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        switch (dir) {
            case LV_DIR_LEFT:
                if (image_seq == 1) {
                    image_seq = IMAGE_NUM;
                } else {
                    image_seq--;
                }
                image_toggle();
                break;
            case LV_DIR_RIGHT:
                if (image_seq == IMAGE_NUM) {
                    image_seq = 1;
                } else {
                    image_seq++;
                }
                image_toggle();
                break;
            default:
                break;
        }
    }

}

lv_obj_t *app_image_init() {
    image_ui = lv_obj_create(NULL);
    lv_obj_set_style_bg_img_src(image_ui, PICTURE_PATH(img1.jpg), 0);

    lv_obj_add_event_cb(image_ui, gesture_cb, LV_EVENT_ALL, NULL);
    return image_ui;
}