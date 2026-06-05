/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */
#include "aic_core.h"
#include "aic_log.h"
#include "drv_efuse.h"
#include "core/barcode_core.h"
#include "led/barcode_led.h"

// Global barcode system instance
static barcode_system_t g_barcode_sys;

#ifdef BARCODE_LED_ENABLE
/**
 * @brief Turn on barcode LED command
 */
static void cmd_barcode_led_on(int argc, char **argv)
{
    barcode_led_on(&g_barcode_sys.led);
}
MSH_CMD_EXPORT_ALIAS(cmd_barcode_led_on, barcode_led_on, Barcode LED on);

/**
 * @brief Turn off barcode LED command
 */
static void cmd_barcode_led_off(int argc, char **argv)
{
    barcode_led_off(&g_barcode_sys.led);
}
MSH_CMD_EXPORT_ALIAS(cmd_barcode_led_off, barcode_led_off, Barcode LED off);
#endif

/**
 * @brief Barcode demo command handler
 * @param argc Argument count
 * @param argv Argument vector
 */
static void barcode_demo(void)
{
    int ret;

    ret = barcode_system_init(&g_barcode_sys);
    if (ret != 0) {
        pr_err("Barcode: system init failed with %d\n", ret);
        return;
    }

    barcode_system_start(&g_barcode_sys);
}

/**
 * @brief Auto-start barcode demo on system initialization
 * @return 0 on success
 */
static int barcode_demo_auto_start(void)
{
    barcode_demo();
    return 0;
}
INIT_LATE_APP_EXPORT(barcode_demo_auto_start);

#ifdef RT_USING_FINSH
/**
 * @brief Print chip ID information
 * @param argc Argument count
 * @param argv Argument vector
 */
static void cmd_printf_chipid(int argc, char **argv)
{
#ifdef AIC_USING_SID
#define CHIPID_BUF_SIZE 64
    u32 chipid[4] = {0};
    drv_efuse_read_chip_id(chipid);
    pr_info("chipid:[%.4x][%.4x][%.4x][%.4x]\n",chipid[0],chipid[1],chipid[2],chipid[3]);

    u8 reserved[CHIPID_BUF_SIZE] = {0};
    drv_efuse_read_reserved_1(reserved);
    pr_info("reserved 1 :[");
    for(size_t i = 0; i < CHIPID_BUF_SIZE; i++) {
        pr_info("0x%02x ", reserved[i]);
    }
    pr_info("]\n");

    memset(reserved, 0, sizeof(reserved));
    drv_efuse_read_reserved_2(reserved);
    pr_info("reserved 2 :[");
    for(size_t i = 0; i < CHIPID_BUF_SIZE; i++) {
        pr_info("0x%02x ", reserved[i]);
    }
    pr_info("]\n");
#endif
}
MSH_CMD_EXPORT_ALIAS(cmd_printf_chipid, printf_chipid, Printf chipid);
#endif
