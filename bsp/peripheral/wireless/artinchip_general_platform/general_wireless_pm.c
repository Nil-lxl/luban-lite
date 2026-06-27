/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <ruiqi.zheng@artinchip.com>
 */

/*
 * WiFi PM Suspend/Resume Framework
 *
 * Problem:
 *   After deep sleep, the WiFi chip loses power. SDIO card state in
 *   memory is stale, and the WiFi driver's threads/semaphores need to
 *   be cleaned up before re-initialization.
 *
 *   However, PM suspend/resume callbacks run inside rt_pm_enter_critical
 *   with interrupts disabled. wifi_if_sdio_deinit() deletes RT-Thread
 *   objects (threads, semaphores, mutexes), which requires the scheduler
 *   to run. Calling it during PM callbacks corrupts kernel state.
 *
 * Solution:
 *   Split into two phases:
 *
 *   Phase 1 (PM callbacks, interrupts disabled):
 *     Suspend:  free old SDIO card + power off GPIO
 *     Resume:   power on GPIO + submit work to workqueue
 *
 *   Phase 2 (workqueue thread, interrupts enabled):
 *     Deinit old driver state (wifi_if_sdio_deinit)
 *     Clean up WLAN device mode
 *     mmcsd_change -> async card re-detection
 *     Wait for detection to complete, then release sleep lock
 *
 * SDMC Ordering:
 *   SDMC is moved to the PM late list (SET_LATE_DEVICE_PM_OPS) so it
 *   suspends AFTER WiFi (GPIO power-off triggers card detect events
 *   that need SDMC alive) and resumes BEFORE WiFi (controller must be
 *   ready before we power on the chip and trigger mmcsd_change).
 *
 * Card Lifecycle:
 *   sdio_register_card() keeps a static sdio_card with sc.card pointing
 *   to the last registered card. aic_wlan_pm_sdio_free_card() frees the
 *   old card during suspend. On resume, mmcsd_change triggers the
 *   detect thread which calls sdio_init_card() to create a new card
 *   and sdio_register_card() updates sc.card.
 */

#define LOG_TAG "wlan_pm"
#include <aic_core.h>
#include <aic_drv.h>
#include <drivers/pm.h>
#include <drivers/mmcsd_card.h>
#include <drivers/mmcsd_core.h>
#include <drivers/sdio.h>
#include <ipc/workqueue.h>

#include "general_wireless_pm.h"
#include "general_wireless_pin.h"

static struct aic_wlan_chip_pm_ops *g_chip_ops;
static struct rt_workqueue *g_wifi_pm_wq;
static struct rt_work g_resume_work;

/*
 * Deactivate both WLAN STA and AP devices.
 * Called from workqueue thread (interrupts enabled).
 */
int aic_wlan_pm_free_device()
{
    struct rt_wlan_device *device = (struct rt_wlan_device *)rt_device_find(RT_WLAN_DEVICE_AP_NAME);
    if (device == RT_NULL) {
        return -RT_EIO;
    }

    if (rt_wlan_get_mode(RT_WLAN_DEVICE_AP_NAME) != RT_WLAN_NONE) {
        rt_wlan_set_mode(RT_WLAN_DEVICE_AP_NAME, RT_WLAN_NONE);
    }

    if (device->device.flag & RT_DEVICE_FLAG_ACTIVATED) {
        rt_mutex_detach(&device->lock);
        device->device.flag &=~RT_DEVICE_FLAG_ACTIVATED;
    }

    device = (struct rt_wlan_device *)rt_device_find(RT_WLAN_DEVICE_STA_NAME);
    if (device == RT_NULL){
        return -RT_EIO;
    }

    if (rt_wlan_get_mode(RT_WLAN_DEVICE_STA_NAME) != RT_WLAN_NONE) {
        rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_NONE);
    }

    if (device->device.flag & RT_DEVICE_FLAG_ACTIVATED) {
        rt_mutex_detach(&device->lock);
        device->device.flag &=~RT_DEVICE_FLAG_ACTIVATED;
    }
    return 0;
}

/*
 * Free the SDIO card attached to host.
 *
 * Note: This frees host->card and its sdio_function[] array. The static
 * sdio_card in sdio_register_card() still holds sc.card pointing to this
 * freed memory. This is safe because:
 *   - During suspend, no one accesses sdio_cards (interrupts disabled)
 *   - On resume, mmcsd_change triggers sdio_init_card() which creates a
 *     new card and sdio_register_card() overwrites sc.card
 *
 * Called from PM suspend callback (interrupts disabled) or workqueue
 * thread (interrupts enabled). Only does memory operations, no thread
 * or IPC operations, so safe in both contexts.
 */
void aic_wlan_pm_sdio_free_card(struct rt_mmcsd_host *host)
{
    int i;

    if (!host || !host->card) {
        return;
     }
    mmcsd_host_lock(host);

    for (i = host->card->sdio_function_num; i >= 0; i--) {
        if (host->card->sdio_function[i]) {
            if (host->card->sdio_function[i]->curr) {
                rt_free(host->card->sdio_function[i]->curr);
                host->card->sdio_function[i]->curr = NULL;
            }

            sdio_free_cis(host->card->sdio_function[i]);
            rt_free(host->card->sdio_function[i]);
            host->card->sdio_function[i] = NULL;
        }
    }

    rt_free(host->card);
    host->card = NULL;

    mmcsd_host_unlock(host);
}

/*
 * Phase 2 of resume: runs in workqueue thread context.
 *
 * Interrupts are enabled and the scheduler is running, so
 * wifi_if_sdio_deinit() can safely delete threads and IPC objects.
 *
 * Order matches aic8800_test:
 *   deinit -> free WLAN mode -> free old card -> mmcsd_change
 *
 * A 300ms delay after mmcsd_change gives the detect thread time to
 * complete card re-detection before releasing the PM sleep lock.
 * Without this, the system may re-enter sleep before the card is
 * detected, causing queued mmcsd_change messages to be misinterpreted
 * as "card removed" by the detect thread.
 */
static void _wlan_pm_resume_deferred(struct rt_work *work, void *work_data)
{
    struct rt_mmcsd_host *host = (struct rt_mmcsd_host *)work_data;

    if (g_chip_ops->resume)
        g_chip_ops->resume(PM_SLEEP_MODE_DEEP);

    aic_wlan_pm_free_device();

    aic_wlan_pm_sdio_free_card(host);

    mmcsd_change(host);

    /*
     * Wait for detect thread to process mmcsd_change before
     * allowing sleep again. Without this, the system may re-enter
     * sleep before card detection completes, causing queued
     * mmcsd_change messages to be misinterpreted as "card removed".
     */
    aicos_msleep(300);

    rt_pm_module_release(PM_POWER_ID, PM_SLEEP_MODE_NONE);
}

/*
 * Phase 1 suspend: only power off the WiFi chip.
 *
 * Called from PM suspend callback with interrupts disabled.
 * All software cleanup (driver deinit, WLAN mode reset) is deferred
 * to the workqueue callback that runs after resume.
 *
 * The card is freed here (before SDMC suspends) so that when SDMC
 * resumes and hardware card detect fires, host->card is NULL and
 * the detect thread correctly enters the "card insertion" path.
 */
static int _wlan_pm_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    if (mode == PM_SLEEP_MODE_IDLE)
        return RT_EOK;

    aic_platform_wlan_power_off();

    return RT_EOK;
}

/*
 * Phase 1 resume: only power on the WiFi chip and schedule Phase 2.
 *
 * Called from PM resume callback with interrupts disabled.
 * - Powers on WiFi GPIO
 * - Requests PM sleep lock to prevent immediate re-sleep
 * - Submits work to g_wifi_pm_wq for Phase 2 cleanup
 *
 * The workqueue thread runs later with interrupts enabled and
 * completes: deinit -> free card -> mmcsd_change -> release lock.
 */
static void _wlan_pm_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct rt_mmcsd_host *host;

    if (mode == PM_SLEEP_MODE_IDLE)
        return;

    host = aic_sdmc_get_rthost(AIC_WIFI_SDMC_ID);
    if (!host) {
        return;
    }

    aic_platform_wlan_power_on();
    aicos_msleep(10);

    /*
     * Block the idle thread from re-entering sleep before the
     * workqueue completes deinit + mmcsd_change + card detection.
     */
    rt_pm_module_request(PM_POWER_ID, PM_SLEEP_MODE_NONE);

    g_resume_work.work_data = (void *)host;
    rt_workqueue_submit_work(g_wifi_pm_wq, &g_resume_work, 0);
}

static struct rt_device_pm_ops _wlan_pm_ops =
{
    SET_DEVICE_PM_OPS(_wlan_pm_suspend, _wlan_pm_resume)
    NULL,
};

int aic_wlan_pm_register(struct rt_device *device,
                         struct aic_wlan_chip_pm_ops *ops)
{
    if (!device || !ops)
        return -RT_EINVAL;

    g_chip_ops = ops;
    rt_pm_device_register(device, &_wlan_pm_ops);

    /*
     * Create a dedicated workqueue for deferred resume cleanup.
     * The workqueue thread runs at priority 20 with interrupts
     * enabled, so wifi_if_sdio_deinit() can safely delete threads,
     * semaphores, and mutexes. A hard timer cannot be used because
     * rt_timer_init creates a hard timer whose callback runs in
     * ISR context where thread/IPC operations are illegal.
     */
    g_wifi_pm_wq = rt_workqueue_create("wifi_pm", 2048, 20);
    if (g_wifi_pm_wq) {
        rt_work_init(&g_resume_work, _wlan_pm_resume_deferred, NULL);
    }

    return RT_EOK;
}

void aic_wlan_pm_unregister(struct rt_device *device)
{
    if (device)
        rt_pm_device_unregister(device);
    g_chip_ops = NULL;
}
