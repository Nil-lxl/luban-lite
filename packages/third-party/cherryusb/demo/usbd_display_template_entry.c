/*
 * Copyright (c) 2022-2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "usbd_core.h"
#include "usbd_hid.h"
#include "aic_core.h"
#include "aic_drv.h"
#include "mpp_fb.h"
#include "mpp_mem.h"
#include "frame_allocator.h"
#include "mpp_decoder.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_ge.h"

#define PIXEL_ENCODE_RGB565     0x00
#define PIXEL_ENCODE_ARGB8888   0x01
#define PIXEL_ENCODE_JPEG       0x10
#define PIXEL_ENCODE_H264       0x11
#define PIXEL_ENCODE_AUTO       0xFF

#if defined(LPKG_CHERRYUSB_USB_TRANSFER_AUTO)
uint8_t usbdisp_encode_format = PIXEL_ENCODE_AUTO;
#elif defined(LPKG_CHERRYUSB_USB_TRANSFER_JPEG)
uint8_t usbdisp_encode_format = PIXEL_ENCODE_JPEG;
#elif defined(LPKG_CHERRYUSB_USB_TRANSFER_H264)
uint8_t usbdisp_encode_format = PIXEL_ENCODE_H264;
#elif defined(LPKG_CHERRYUSB_USB_TRANSFER_RGB565)
uint8_t usbdisp_encode_format = PIXEL_ENCODE_RGB565;
#elif defined(LPKG_CHERRYUSB_USB_TRANSFER_ARGB8888)
uint8_t usbdisp_encode_format = PIXEL_ENCODE_ARGB8888;
#endif

#ifdef LPKG_CHERRYUSB_DEVICE_DISPLAY_FPS
uint8_t fps_print = 1;
#else
uint8_t fps_print = 0;
#endif

#ifdef AIC_FB_ROTATE_EN
uint32_t usb_disp_rotate = AIC_FB_ROTATE_DEGREE;
#else
uint32_t usb_disp_rotate = 0;
#endif

#ifdef AIC_USB_DISP_DEF_DIS
uint8_t usb_display_en = 0;
#else
uint8_t usb_display_en = 1;
#endif

#ifdef AIC_USB_DISP_INIT_DELAY_MS
uint32_t usb_display_init_delay_ms = AIC_USB_DISP_INIT_DELAY_MS;
#else
uint32_t usb_display_init_delay_ms = 0;
#endif

#ifndef AIC_LVGL_USB_OSD_DEMO
uint8_t usb_disp_free_framebuffer = 1;
#else
uint8_t usb_disp_free_framebuffer = 0;
#endif

#ifdef LPKG_CHERRYUSB_DEVICE_AUDIO
uint8_t usb_large_transfer = 0;
#else
uint8_t usb_large_transfer = 1;
#endif

static void usage_fps(char * program)
{
    printf("\n");
    printf("%s - Turn fps print on/off.\n\n", program);
    printf("Usage: %s <on|off>\n", program);
    printf("For example:\n");
    printf("\t%s on\n", program);
    printf("\t%s off\n", program);
    printf("\n");
}

static int test_usb_fps(int argc, char *argv[])
{
    if (argc != 2)
    {
        usage_fps(argv[0]);
        return -RT_EINVAL;
    }

    if (strcmp(argv[1], "on") == 0) {
        fps_print = 1;
    } else if (strcmp(argv[1], "off") == 0) {
        fps_print = 0;
    } else {
        usage_fps(argv[0]);
        return -RT_EINVAL;
    }

    return 0;
}

MSH_CMD_EXPORT_ALIAS(test_usb_fps, usb_fps, usb display fps);

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
void usbd_comp_disp_event_handler(uint8_t event)
#else
void usbd_event_handler(uint8_t event)
#endif
{
    extern void usbd_display_event_handler(uint8_t event);
    usbd_display_event_handler(event);
}

void usb_display_init(void)
{
#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    extern int usbd_comp_disp_init(uint8_t *ep_table, void *data);
    extern uint8_t graphic_vendor_descriptor[];
    usbd_comp_func_register(graphic_vendor_descriptor,
                            usbd_comp_disp_event_handler,
                            usbd_comp_disp_init, "disp");
#else
    extern void usb_nocomp_display_init(void);
    usb_nocomp_display_init();
#endif
}

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdevice.h>

void usbd_disp_comp_setup(void *parameter)
{
#if defined(LPKG_USING_COMP_MSC) && \
    defined(LPKG_CHERRYUSB_DEVICE_MSC)
    extern int msc_usbd_forbid(void);
    msc_usbd_forbid();
#endif
#if defined(LPKG_USING_COMP_TOUCH) && \
    defined(LPKG_CHERRYUSB_DEVICE_HID_TOUCH_TEMPLATE)
    extern int usbd_hid_touch_init(void);
    usbd_hid_touch_init();
#endif
#if defined(LPKG_USING_COMP_UAC) && \
    defined(LPKG_CHERRYUSB_DEVICE_AUDIO_SPEAKER_V2_TEMPLATE)
    extern int usbd_audio_v2_init(void);
    usbd_audio_v2_init();
#endif
}

rt_thread_t usbd_comp_disp_tid = NULL;
int comp_dev_ctl_table(void)
{
#if defined(LPKG_CHERRYUSB_AIC_DISP_DR)
    if (usbd_comp_disp_tid == NULL) {
        usbd_comp_disp_tid = rt_thread_create("usbd_disp_comp_setup",
                                    usbd_disp_comp_setup, RT_NULL,
                                    2048, RT_THREAD_PRIORITY_MAX - 2, 20);
        if (usbd_comp_disp_tid != RT_NULL)
            rt_thread_startup(usbd_comp_disp_tid);
        else
            printf("create usbd_disp_comp_setup thread err!\n");
    }
#endif

    return 0;
}

extern int usbd_usb_display_init(void);

#if !defined(LPKG_CHERRYUSB_DYNAMIC_REGISTRATION_MODE) || \
    defined(LPKG_CHERRYUSB_AIC_DISP_DR)
INIT_COMPONENT_EXPORT(usbd_usb_display_init);

#ifdef AIC_USB_DISP_SW_GPIO_EN
void aic_panel_backlight_enable(void)
{
#ifdef AIC_PANEL_ENABLE_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_PANEL_ENABLE_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_direction_output(g, p);

#ifndef AIC_PANEL_ENABLE_GPIO_LOW
    hal_gpio_set_output(g, p);
#else
    hal_gpio_clr_output(g, p);
#endif
#endif /* AIC_PANEL_ENABLE_GPIO */

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    /* pwm frequency: 1KHz = 1000000ns */
    rt_pwm_set(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL,
            1000000, 10000 * AIC_PWM_BRIGHTNESS_LEVEL);
    rt_pwm_enable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

void aic_panel_backlight_disable(void)
{
#ifdef AIC_PANEL_ENABLE_GPIO
    unsigned int g, p;
    long pin;

    pin = hal_gpio_name2pin(AIC_PANEL_ENABLE_GPIO);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

#ifndef AIC_PANEL_ENABLE_GPIO_LOW
    hal_gpio_clr_output(g, p);
#else
    hal_gpio_set_output(g, p);
#endif
#endif /* AIC_PANEL_ENABLE_GPIO */

#if defined(KERNEL_RTTHREAD) && defined(AIC_PWM_BACKLIGHT_CHANNEL)
    struct rt_device_pwm *pwm_dev;

    pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    rt_pwm_disable(pwm_dev, AIC_PWM_BACKLIGHT_CHANNEL);
#endif
}

void lcd_sw_key_irq_callback(void *args)
{
    rt_base_t pin;
    unsigned int g, p, val;

    pin = rt_pin_get(AIC_USB_DISP_SW_GPIO_NAME);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_get_value(g, p, &val);

#if defined(AIC_USB_DISP_SW_GPIO_BACKLIGHT)
    if (val ^ AIC_USB_DISP_SW_GPIO_NAME_POLARITY)
        aic_panel_backlight_enable();
    else
        aic_panel_backlight_disable();
#elif defined(AIC_USB_DISP_SW_GPIO_USBDISPLAY)
    extern void usb_display_enable(void);
    extern void usb_display_disable(void);
    if (val ^ AIC_USB_DISP_SW_GPIO_NAME_POLARITY)
        usb_display_enable();
    else
        usb_display_disable();
#endif
}

int lcd_sw_key_init(void)
{
    rt_base_t pin;
    unsigned int g, p;

    pin = rt_pin_get(AIC_USB_DISP_SW_GPIO_NAME);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_set_debounce(g, p, 0xFFF);

    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING, lcd_sw_key_irq_callback, RT_NULL);
    rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);

    return 0;
}

INIT_APP_EXPORT(lcd_sw_key_init);
#endif

#endif

#if defined(LPKG_CHERRYUSB_AIC_DISP_DR)
extern int usbd_msc_detection(void);
INIT_APP_EXPORT(usbd_msc_detection);
#endif
#endif
