/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <board.h>
#include <hal_syscfg.h>
#include <aic_core.h>
#include <aic_drv_bare.h>
#ifdef AIC_DISPLAY_DRV
#include <artinchip_fb.h>
#endif
#ifdef AIC_VE_DRV
#include "aic_hal_ve.h"
#endif
#include "aic_reboot_reason.h"
#include <aic_log.h>
#ifdef AIC_DMA_DRV
#include "drv_dma.h"
#endif

#ifdef LPKG_CHERRYUSB_HOST
#include <usbh_core.h>
#include <usbh_hub.h>
#include <usb_hc.h>
#endif

#ifdef AIC_GPIO_DRV
#include "aic_drv_gpio.h"
#endif

#ifdef AIC_ADCIM_DRV
#include "hal_adcim.h"
#endif

#ifdef LPKG_USING_DFS
#include <dfs.h>
#include <dfs_fs.h>
#ifdef LPKG_USING_DFS_ELMFAT
#include <dfs_elm.h>
#endif
#ifdef LPKG_USING_DFS_ROMFS
#include <dfs_romfs.h>
#endif
#endif

#ifdef AIC_SDMC_DRV
#include "mmc.h"
#endif

#ifdef AIC_LVGL_DEMO
#include "lvgl.h"

extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void lv_user_gui_init(void);
#endif

void show_version(void);

void show_banner(void)
{
    printf("%s\n", BANNER);
    show_version();
}

#ifdef AIC_CONSOLE_BARE_DRV
static void console_loop_thread(void *arg)
{
    /* Console shell loop */
    console_init();
    console_loop();

    /* FreeRTOS Task exit */
    aicos_thread_delete(NULL);
}
#endif

static int board_init(void)
{
    int cons_uart;
#ifdef AIC_CONSOLE_BARE_DRV
    aicos_thread_t tshell = NULL;
#endif

#ifdef AIC_SYSCFG_DRV
    hal_syscfg_probe();
#endif
    aicos_local_irq_enable();

    cons_uart = AIC_BAREMETAL_CONSOLE_UART;
    uart_init(cons_uart);
#ifdef AIC_PRINTF_BARE_DRV
    stdio_set_uart(cons_uart);
#endif

    show_banner();

#ifdef AIC_CONSOLE_BARE_DRV
    tshell = aicos_thread_create("shell", 4096, configMAX_PRIORITIES-20, console_loop_thread, NULL);
    if (tshell == NULL) {
        pr_err("Failed to create shell thread\n");
        return -1;
    }
#endif

    return 0;
}

#if LV_USE_LOG
#if defined(LVGL_V_8)
static void lv_rt_log(const char *buf)
{
    printf("%s\n", buf);
}
#else
static void lv_rt_log(lv_log_level_t level, const char *buf)
{
    (void)level;
    printf("%s\n", buf);
}

static uint32_t lv_tick_get_ms(void)
{
    return (uint32_t)aic_get_time_ms();
}

#endif
#endif /* LV_USE_LOG */

extern void lwip_test_example_main_loop(void * data);
int main(void)
{
    board_init();

#ifdef AIC_DMA_DRV
    drv_dma_init();
#endif

#ifdef TLSF_MEM_HEAP
    aic_tlsf_heap_test();
#endif

#ifdef LPKG_USING_DFS
    dfs_init();
#ifdef LPKG_USING_DFS_ROMFS
    dfs_romfs_init();
#endif
#ifdef LPKG_USING_DFS_ELMFAT
    elm_init();
#endif
#endif

#ifdef AIC_GPIO_DRV
    drv_pin_init();
#endif

#ifdef LPKG_USING_DFS_ROMFS
    if (dfs_mount(NULL, "/", "rom", 0, &romfs_root) < 0)
        pr_err("Failed to mount romfs\n");
#endif

#ifdef AIC_SDMC_DRV
    mmc_init(1);
#ifdef AIC_SD_USING_HOTPLUG
    sdcard_hotplug_init();
#endif
#endif

#if defined(LPKG_USING_DFS_ELMFAT) && defined(AIC_SDMC_DRV)
    if (dfs_mount("sd1", "/sdcard", "elm", 0, DEVICE_TYPE_SDMC_DISK) < 0)
        pr_err("Failed to mount sdmc with FatFS\n");
#endif

#if defined(AIC_SPINAND_DRV) || defined(AIC_SPINOR_DRV)
    mtd_probe();
#endif

#if defined(LPKG_USING_DFS_ELMFAT) && defined(AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_0)
#if defined(AIC_SPINAND_DRV)
    if (dfs_mount("rodata", "/rodata", "elm", 0, DEVICE_TYPE_SPINAND_DISK) < 0)
        pr_err("Failed to mount spinand with FatFS\n");
#endif
#if defined(AIC_SPINOR_DRV)
    if (dfs_mount("rodata", "/rodata", "elm", 0, DEVICE_TYPE_SPINOR_DISK) < 0)
        pr_err("Failed to mount spinor with FatFS\n");
#endif
#endif

#ifdef AIC_DISPLAY_DRV
    aicfb_probe();
#endif
#ifdef AIC_GE_DRV
    hal_ge_init();
#endif
#ifdef AIC_VE_DRV
    hal_ve_probe();
#endif
#ifdef AIC_WRI_DRV
    aic_get_reboot_reason();
    aic_clr_reboot_reason();
    aic_show_startup_time();
#endif

#ifdef LPKG_CHERRYUSB_DEVICE
#ifdef LPKG_CHERRYUSB_DEVICE_MSC_TEMPLATE
    extern void msc_ram_init(void);
    msc_ram_init();
#endif
#endif

#ifdef LPKG_CHERRYUSB_HOST
    usbh_init();
    while(1)
    {
        usbh_hub_poll();
    }
#endif

#ifdef LPKG_LWIP_EXAMPLES
    /* LwIP test loop */
    lwip_test_example_main_loop(NULL);
#endif

#ifdef AIC_LVGL_DEMO
#if LV_USE_LOG
    lv_log_register_print_cb(lv_rt_log);
#endif /* LV_USE_LOG */
    lv_init();
    lv_port_disp_init();
    lv_user_gui_init();

#if defined(LVGL_V_9)
    lv_tick_set_cb(&lv_tick_get_ms);
#endif
    while(1)
    {
        lv_task_handler();
    }
#endif /* AIC_LVGL_DEMO */

#ifdef AIC_VE_TEST
    extern int  do_pic_dec_test(int argc, char **argv);
    /* Main loop */
    aicos_mdelay(2000);
    while (1) {
        do_pic_dec_test(0,NULL);
    }
#endif

#ifdef AIC_SD_USING_HOTPLUG
    while (1) {
        sdcard_hotplug_act();
    }
#endif

#ifdef AIC_ADCIM_DRV
    hal_adcim_probe();
#endif

    /* FreeRTOS Task exit */
    aicos_thread_delete(NULL);
    return 0;
}
