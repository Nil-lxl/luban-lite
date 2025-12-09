/*
 * Copyright (C) 2022-2024 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"
#include "rtdef.h"

#include "lvgl.h"
#include "test_ui.h"
#include "lv_aic_player.h"

#define LOG_TAG "LV_TEST"

#define TEST_DEMO_USE_DEFAULT_CONTROL   0       //默认自动切换
#define TEST_DEMO_USE_KEYADC_CONTROL    1       //使用外部按键切换
#define TEST_DEMO_USE_CIR_CONTROL       0       //使用红外遥控切换

#ifdef AIC_PANEL_CUSTOM_RESOLUTION
#define LCD_HOR_RES PANEL_HACTIVE
#define LCD_VER_RES PANEL_VACTIVE
#else
#define LCD_HOR_RES PANEL_HACTIVE_RES
#define LCD_VER_RES PANEL_VACTIVE_RES
#endif

#ifdef AIC_USING_HOT68_DEMO_A01_V0
#define KEYADC_CHANNEL     5 
#define BACKLIGHT_PWM_CHANNEL   2
#else 
#define KEYADC_CHANNEL     7       
#define BACKLIGHT_PWM_CHANNEL   3         
#endif                                          

#define KEYADC_SCALE            200             //电压变化范围(单位:mv)

static rt_thread_t cir_thread;
static rt_device_t cir_dev;
static rt_sem_t cir_sem;

static struct rt_device_pwm *pwm_dev;
#ifdef AIC_PWM_BACKLIGHT
static int brightness_level = AIC_PWM_BRIGHTNESS_LEVEL;
#endif

static lv_obj_t *scr;
static lv_obj_t *gray_block;
static lv_obj_t *container;
static lv_timer_t *timer;

static lv_obj_t *img1;
static lv_obj_t *img2;
static lv_obj_t *img3;
static lv_obj_t *player;

#if TEST_DEMO_USE_DEFAULT_CONTROL
static int count = 0;
#else 
static int count = -1;
#endif

rt_err_t cir_rx_cb(rt_device_t dev, rt_size_t size) {
    rt_sem_release(cir_sem);
    return RT_EOK;
}

static void lv_set_bg_color(int color_hex) {
    lv_obj_set_style_bg_color(scr, lv_color_hex(color_hex), 0);
}

void lv_obj_hide(lv_obj_t *obj) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void lv_obj_show(lv_obj_t *obj) {
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void timer_cb(lv_timer_t *timer) {
    switch (count) {
        case 0:
            lv_obj_hide(img1);
            // lv_obj_show(container);
            lv_obj_hide(container);
            lv_set_bg_color(LV_COLOR_RED);
            break;
        case 1:
            lv_set_bg_color(LV_COLOR_GREEN);
            break;
        case 2:
            lv_set_bg_color(LV_COLOR_BLUE);
            break;
        case 3:
            lv_obj_hide(gray_block);
            lv_set_bg_color(LV_COLOR_WHITE);
            break;
        case 4:
            lv_obj_show(gray_block);
            lv_obj_hide(img1);
            break;
        case 5:
            lv_obj_show(img1);
            lv_obj_hide(gray_block);
            lv_obj_hide(img2);
            break;
        case 6:
            // lv_obj_show(img2);
            lv_obj_hide(img1);
            lv_obj_show(container);
            lv_set_bg_color(LV_COLOR_BLACK);
            break;
        case 7:
            lv_obj_hide(img2);
            lv_obj_show(container);
            lv_set_bg_color(LV_COLOR_BLACK);
            break;
        case 8:
            lv_obj_hide(img3);
            break;
        case 9:
            // lv_obj_show(player);
            // lv_aic_player_set_auto_restart(player, true);
            // lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
            break;
        default:
            break;
    }
#define UI_MAX_COUNT    6
#if TEST_DEMO_USE_DEFAULT_CONTROL
    if (count == 9) {                      //在第x个画面停止
        lv_timer_pause(timer);
    }
    count = (count + 1) % 7;    //在第x个画面结束一轮循环
#endif
}

extern void test_control(void);

static lv_obj_t *gray_bar[16];
const uint32_t gray_color[16] = { 0x000000,0x101010,0x202020,0x303030,0x404040,0x505050,0x606060,0x707070,0x808080,0x909090,0xA0A0A0,0xB0B0B0,0xC0C0C0,0xD0D0D0,0xE0E0E0,0xF0F0F0 };

void test_ui_init() {
    aicos_msleep(1500);     //等待sdcard挂载成功

#if 1
    scr = lv_scr_act();
    lv_obj_set_style_bg_opa(scr, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

    player = lv_aic_player_create(scr);  //视频播放器
    // lv_aic_player_set_src(player, SD_VIDEO_PATH(cartoon.mp4));
    lv_obj_center(player);
    lv_obj_hide(player);

    container = lv_obj_create(scr);         //白色边框
    lv_obj_center(container);
    lv_obj_set_size(container, LCD_HOR_RES, LCD_VER_RES);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_border_color(container, lv_color_white(), 0);

    gray_block = lv_obj_create(scr);        //灰阶
    lv_obj_set_size(gray_block, LCD_HOR_RES, LCD_VER_RES);
    lv_obj_set_style_radius(gray_block, 0, 0);
    lv_obj_set_style_border_width(gray_block, 0, 0);
    lv_obj_set_style_pad_all(gray_block, 0, 0);
    for (int i = 0;i < 16;i++) {
        gray_bar[i] = lv_obj_create(gray_block);
        if (LCD_HOR_RES > LCD_VER_RES) {    //横屏
            lv_obj_set_size(gray_bar[i], LCD_HOR_RES / 16, lv_pct(100));
            lv_obj_set_pos(gray_bar[i], (LCD_HOR_RES / 16) * i, 0);
        } else {                            //竖屏
            lv_obj_set_size(gray_bar[i], lv_pct(100), LCD_VER_RES / 16);
            lv_obj_set_pos(gray_bar[i], 0, (LCD_VER_RES / 16) * i);
        }
        lv_obj_set_style_pad_all(gray_bar[i], 0, 0);
        lv_obj_set_style_border_width(gray_bar[i], 0, 0);
        lv_obj_set_style_radius(gray_bar[i], 0, 0);
        lv_obj_set_style_bg_color(gray_bar[i], lv_color_hex(gray_color[i]), 0);
    }
    lv_obj_hide(gray_block);

    img1 = lv_img_create(scr);
    lv_img_set_src(img1, LVGL_IMAGE_PATH(fruit640x480.jpg));
    img2 = lv_img_create(scr);
    lv_img_set_src(img2, LVGL_IMAGE_PATH(fruit640x480.jpg));
    img3 = lv_img_create(scr);
    lv_img_set_src(img3, LVGL_IMAGE_PATH(img400x1280_3.jpg));
    lv_obj_add_flag(img1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img3, LV_OBJ_FLAG_HIDDEN);

#if TEST_DEMO_USE_DEFAULT_CONTROL
    timer = lv_timer_create(timer_cb, 500, NULL);
#else 
    timer = lv_timer_create(timer_cb, 300, NULL);
#endif

#endif
}

void ui_init(void) {

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    if (pwm_dev == NULL) {
        LOG_E("PWM Device Not Found");
        // return -RT_ERROR;
    }

    test_ui_init();

#if TEST_DEMO_USE_CIR_CONTROL
    cir_thread_begin();
#elif TEST_DEMO_USE_KEYADC_CONTROL
    keyadc_thread_begin();
#endif
}

#if TEST_DEMO_USE_CIR_CONTROL
#include "drv_cir.h"

void cir_thread_entry(void *param) {
    uint32_t read_buf = 0;
    rt_size_t size;
    while (1) {
        rt_sem_take(cir_sem, RT_WAITING_FOREVER);
        size = rt_device_read(cir_dev, 0, &read_buf, sizeof(read_buf));
        if (size) {
            switch (read_buf) {
                case CIR_CMD_SHUTDOWN:
                    rt_hw_cpu_reset();
                    break;
                case CIR_CMD_MENU:
                    LOG_I("MENU");
                    break;
                case CIR_CMD_BACK:
                    LOG_I("BACK");
                    break;
                case CIR_CMD_CLEAR:
                    LOG_I("CLEAR");
                    lv_obj_del(player);
                    lv_timer_resume(timer);
                    break;
                case CIR_CMD_TEST:
                    LOG_I("TEST");
                    break;
                case CIR_CMD_PLUS:
                    brightness_level += 10;
                    if (brightness_level > 100) {
                        brightness_level = 100;
                    }
                    rt_pwm_set(pwm_dev, BACKLIGHT_PWM_CHANNEL, 1000000, 10000 * brightness_level);
                    rt_pwm_enable(pwm_dev, BACKLIGHT_PWM_CHANNEL);
                    LOG_I("+");
                    break;
                case CIR_CMD_MINUS:
                    brightness_level -= 10;
                    if (brightness_level < 10) {
                        brightness_level = 10;
                    }
                    rt_pwm_set(pwm_dev, BACKLIGHT_PWM_CHANNEL, 1000000, 10000 * brightness_level);
                    rt_pwm_enable(pwm_dev, BACKLIGHT_PWM_CHANNEL);
                    LOG_I("-");
                    break;
                case CIR_CMD_PREV:
                    LOG_I("PREV");
                    break;
                case CIR_CMD_NEXT:
                    LOG_I("NEXT");
                    break;
                case CIR_CMD_PLAY:
                    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
                    LOG_I("PLAY");
                    break;
                case CIR_CMD_ZERO:
                    lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_PAUSE, NULL);
                    break;
                case CIR_CMD_ONE:
                    break;

            }
            // rt_kprintf("cir received data: 0x%x", read_buf);
        }
        rt_thread_mdelay(200);
    }
}


void cir_thread_begin(void) {
    int ret = 0;
    cir_config_t cir_config = {
        .protocol = CIR_PROTOCOL_NEC,
        .tx_duty = 33,
        .rx_level = 1,
    };

    cir_dev = rt_device_find("cir");
    if (!cir_dev) {
        LOG_E("CIR Device Not Found");
        return;
    }

    ret = rt_device_open(cir_dev, RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR);
    if (ret != RT_EOK) {
        LOG_E("Open CIR Device Failed");
    }

    ret = rt_device_control(cir_dev, IOC_CIR_CONFIGURE, (void *)&cir_config);
    if (ret != RT_EOK) {
        LOG_E("CIR Device Configure Failed");
    }

    cir_sem = rt_sem_create("cir_sem", 0, RT_IPC_FLAG_PRIO);
    if (cir_sem == RT_NULL) {
        LOG_E("CIR Sem Create Failed!");
    }

    ret = rt_device_set_rx_indicate(cir_dev, cir_rx_cb);
    if (ret != RT_EOK) {
        LOG_E("CIR Device Set Rx Callback Failed!");
    }

    cir_thread = rt_thread_create("cir_thread", cir_thread_entry, RT_NULL, 4 * 1024, 18, 10);
    if (cir_thread != RT_NULL) {
        rt_thread_startup(cir_thread);
    } else {
        LOG_E("CIR Thread Create Failed");
    }
}
#endif

#if TEST_DEMO_USE_KEYADC_CONTROL
#include "keyadc.h"
void keyadc_thread_entry(void *param) {
    int key_flag;
    int channel = KEYADC_CHANNEL;
    int scale = KEYADC_SCALE;
    keyadc_device_enable(channel);
    while (1) {
        key_flag = keyadc_get_flag(channel, scale);
        switch (key_flag) {
            case KEY_UP:
                brightness_level += 10;
                if (brightness_level > 100) {
                    brightness_level = 100;
                }
                rt_pwm_set(pwm_dev, BACKLIGHT_PWM_CHANNEL, 40000, 400 * brightness_level);
                rt_pwm_enable(pwm_dev, BACKLIGHT_PWM_CHANNEL);
                rt_kprintf("KEY UP");
                break;
            case KEY_DOWN:
                brightness_level -= 10;
                if (brightness_level < 10) {
                    brightness_level = 10;
                }
                rt_pwm_set(pwm_dev, BACKLIGHT_PWM_CHANNEL, 40000, 400 * brightness_level);
                rt_pwm_enable(pwm_dev, BACKLIGHT_PWM_CHANNEL);
                rt_kprintf("KEY DOWN");
                break;
            case KEY_LEFT:
                count--;
                if (count < 0) {
                    count = UI_MAX_COUNT;
                }
                rt_kprintf("KEY LEFT");
                break;
            case KEY_RIGHT:
                count++;
                if (count > UI_MAX_COUNT) {
                    count = 0;
                }
                rt_kprintf("KEY RIGHT");
                break;
            default:
                break;
        }
    }

}
void keyadc_thread_begin(void) {
    rt_thread_t keyadc_thread = rt_thread_create("keyadc_thread", keyadc_thread_entry, RT_NULL, 4 * 1024, 22, 10);
    if (keyadc_thread != RT_NULL) {
        rt_thread_startup(keyadc_thread);
    }

}
#endif 