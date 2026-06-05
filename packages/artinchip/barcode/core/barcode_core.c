/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo <guojun.dong@artinchip.com>
 */

#include <string.h>
#include <sys/time.h>
#include "aic_core.h"
#include "aic_log.h"
#include "aic_osal.h"

#include "core/barcode_core.h"
#include "config/barcode_config.h"

#ifdef LPKG_CHERRYUSB_DEVICE_HID_KEYBOARD_TEMPLATE
extern int usbd_keyboard_putnchar(const char *str, int n);
#endif

static void decode_thread(void *arg)
{
    barcode_system_t *sys = (barcode_system_t *)arg;
#ifdef BARCODE_ENABLE_DECODER
    unsigned char result[BARCODE_MAX_BUF_LEN];
#endif
    struct timespec begin, end;

    while (sys->running) {
        // Wait for data ready event
        if (barcode_decoder_wait_data(&sys->decoder, AICOS_WAIT_FOREVER) != 0) {
            continue;
        }

        // Check running flag after wakeup to allow immediate exit on stop
        if (!sys->running) {
            break;
        }

        // Decode image
        gettimespec(&begin);
#ifdef BARCODE_ENABLE_DECODER
        if (barcode_decoder_process(&sys->decoder, sys->frame_buffer,
                                     sys->camera.width, sys->camera.height) == 0) {

            // Get result
            if (barcode_decoder_get_result(&sys->decoder, result, BARCODE_MAX_BUF_LEN)) {
                gettimespec(&end);

#ifdef AIC_BARCODE_BEEP
                rt_pwm_enable(sys->pwm_dev, BARCODE_PWM_CHANNEL);
#endif

#ifdef LPKG_CHERRYUSB_DEVICE_HID_KEYBOARD_TEMPLATE
                usbd_keyboard_putnchar((char *)result, sys->decoder.result_len);
                usbd_keyboard_putnchar("\r\n", 2);
#endif

#ifdef BARCODE_ENABLE_UART
                // Send via UART with correct length (BUG FIX)
                if (sys->decoder.result_len > 0) {
                    int sent_bytes = uart_comm_send(&sys->uart, result, sys->decoder.result_len);
                    if (sent_bytes < 0) {
                        pr_err("UART: send failed\n");
                    } else if (sent_bytes != sys->decoder.result_len) {
                        pr_warn("UART: partial send %d/%d bytes\n", sent_bytes, sys->decoder.result_len);
                    }
                }
#endif

#ifdef BARCODE_DISPLAY_RESULT_TEXT
                barcode_display_show_text(&sys->display, (const char *)result,
                                         BARCODE_TEXT_START_X, BARCODE_TEXT_START_Y);
#endif
            }
        }
#else  // BARCODE_ENABLE_DECODER
        // For g72x or chips without decoder, just simulate a delay or skip
        gettimespec(&end);
#endif  // BARCODE_ENABLE_DECODER

#ifdef AIC_BARCODE_BEEP
        rt_pwm_disable(sys->pwm_dev, BARCODE_PWM_CHANNEL);
#endif
    }

    pr_info("Decode thread exit\n");
}

static void shot_thread(void *arg)
{
    barcode_system_t *sys = (barcode_system_t *)arg;
    int index = 0;

    // Start camera capture
    if (camera_capture_start(&sys->camera) < 0) {
        pr_err("Camera: start failed\n");
        sys->running = false;
        // Wake up decode thread to prevent blocking forever
        barcode_decoder_signal_ready(&sys->decoder);
        return;
    }

    while (sys->running) {
        // Dequeue buffer
        if (camera_capture_dequeue(&sys->camera, &index) < 0) {
            // Wake up decode thread before exit
            barcode_decoder_signal_ready(&sys->decoder);
            break;
        }
        // pr_info("Shot thread: dequeued buffer %d\n", index);
        // Copy frame if decoder is ready
        if (!sys->decoder.ready) {
            if (camera_capture_copy_frame(&sys->camera, index,
                                          sys->frame_buffer,
                                          sys->camera.frame_size) == 0) {
                // Signal decoder
                barcode_decoder_signal_ready(&sys->decoder);
            }
        }

        // Update display
#ifdef BARCODE_ENABLE_DISPLAY
        {
            unsigned long phy_y = sys->camera.buf_info.planes[index * BARCODE_VIDEO_BUF_PLANE_NUM].buf;
            unsigned long phy_uv = sys->camera.buf_info.planes[index * BARCODE_VIDEO_BUF_PLANE_NUM + 1].buf;
            barcode_display_update_video(&sys->display,
                                         sys->camera.width,
                                         sys->camera.height,
                                         phy_y,
                                         phy_uv);
        }
#endif

        // Re-queue buffer
        camera_capture_queue(&sys->camera, index);
    }

    // Ensure decode thread can exit before stopping camera
    barcode_decoder_signal_ready(&sys->decoder);

    sys->running = false;
    camera_capture_stop(&sys->camera);

    pr_info("Shot thread exit\n");
}

int barcode_system_init(barcode_system_t *sys)
{
    int ret;

    if (!sys) {
        return -1;
    }

    memset(sys, 0, sizeof(barcode_system_t));
    sys->running = true;

    // Initialize LED
#ifdef AIC_BARCODE_DEMO_LED
    ret = barcode_led_init(&sys->led);
    if (ret != 0) {
        pr_warn("LED: init failed, continuing without LED\n");
    } else {
        barcode_led_off(&sys->led);
    }
#endif

    // Initialize PWM for beep
#ifdef AIC_BARCODE_BEEP
    sys->pwm_dev = (struct rt_device_pwm *)rt_device_find("pwm");
    if (sys->pwm_dev == NULL) {
        rt_kprintf("PWM: can't find pwm device!\n");
        return -1;
    }
    rt_pwm_set(sys->pwm_dev, BARCODE_PWM_CHANNEL,
               BARCODE_PWM_PERIOD, BARCODE_PWM_DUTY_CYCLE);
#endif

#ifdef BARCODE_ENABLE_UART
    // Initialize UART
    ret = uart_comm_init(&sys->uart, BARCODE_UART_PORT);
    if (ret != RT_EOK) {
        pr_err("UART: init failed\n");
        goto error_out;
    }
#endif

    // Initialize display
#ifdef BARCODE_ENABLE_DISPLAY
    ret = barcode_display_init(&sys->display);
    if (ret != 0) {
        pr_err("Display: init failed\n");
        goto error_out;
    }

    ret = barcode_display_set_ui_alpha(&sys->display, BARCODE_UI_LAYER_ALPHA);
    if (ret != 0) {
        pr_err("Display: set UI alpha failed\n");
        goto error_out;
    }
#endif

    // Initialize camera
    ret = camera_capture_init(&sys->camera, CAMERA_DEV_NAME);
    if (ret != 0) {
        pr_err("Camera: init failed\n");
        goto error_out;
    }

    // Allocate frame buffer
    sys->frame_buffer = aicos_malloc(MEM_CMA, sys->camera.width * sys->camera.height);
    if (!sys->frame_buffer) {
        pr_err("Core: allocate frame buffer failed\n");
        goto error_out;
    }

    // Initialize decoder
    ret = barcode_decoder_init(&sys->decoder);
    if (ret != 0) {
        pr_err("Decoder: init failed\n");
        goto error_out;
    }

    pr_info("Barcode system initialized successfully\n");

    return 0;

error_out:
    barcode_system_deinit(sys);
    return -1;
}

void barcode_system_start(barcode_system_t *sys)
{
    if (!sys) {
        return;
    }

    // Create shot thread
    sys->shot_thread = aicos_thread_create("shot_thread",
                                           BARCODE_SHOT_THREAD_STACK,
                                           BARCODE_SHOT_THREAD_PRIO,
                                           shot_thread, sys);
    if (sys->shot_thread == NULL) {
        pr_err("Core: Failed to create shot thread\n");
        return;
    }

    // Create decode thread
    sys->decode_thread = aicos_thread_create("decode_thread",
                                             BARCODE_DECODE_THREAD_STACK,
                                             BARCODE_DECODE_THREAD_PRIO,
                                             decode_thread, sys);
    if (sys->decode_thread == NULL) {
        pr_err("Core: Failed to create decode thread\n");
        return;
    }

    pr_info("Barcode system started\n");
}

void barcode_system_stop(barcode_system_t *sys)
{
    if (!sys) {
        return;
    }

    sys->running = false;

#ifdef BARCODE_ENABLE_DECODER
    barcode_decoder_signal_ready(&sys->decoder);
#endif

    pr_info("Barcode system stopped\n");
}

void barcode_system_deinit(barcode_system_t *sys)
{
    if (!sys) {
        return;
    }

    // Stop system if running
    if (sys->running) {
        barcode_system_stop(sys);
    }

    // Delete threads to prevent resource leak
    if (sys->shot_thread) {
        aicos_thread_delete(sys->shot_thread);
        sys->shot_thread = NULL;
    }
    if (sys->decode_thread) {
        aicos_thread_delete(sys->decode_thread);
        sys->decode_thread = NULL;
    }

    // Free frame buffer
    if (sys->frame_buffer) {
        aicos_free(MEM_CMA, sys->frame_buffer);
        sys->frame_buffer = NULL;
    }

    // Deinit subsystems
    barcode_decoder_deinit(&sys->decoder);
    camera_capture_deinit(&sys->camera);
#ifdef BARCODE_ENABLE_DISPLAY
    barcode_display_deinit(&sys->display);
#endif
#ifdef BARCODE_ENABLE_UART
    uart_comm_deinit(&sys->uart);
#endif
#ifdef AIC_BARCODE_DEMO_LED
    barcode_led_deinit(&sys->led);
#endif

    memset(sys, 0, sizeof(barcode_system_t));
    pr_info("Barcode system deinitialized\n");
}
