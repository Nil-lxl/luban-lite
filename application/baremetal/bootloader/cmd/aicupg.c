/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <boot_param.h>
#include <config_parse.h>
#include <aicupg.h>
#include <fatfs.h>
#include <mmc.h>
#include <hal_syscfg.h>
#include <upg_uart.h>
#include <hal_rtc.h>
#include <wdt.h>
#include <progress_bar.h>
#ifdef LPKG_CHERRYUSB_HOST
#include <usbh_core.h>
#endif

#define WAIT_UPG_MODE_TMO_US 2000000
#define AICUPG_HELP                                                      \
    "ArtInChip upgrading command:\n"                                     \
    "aicupg [devtype] [interface]\n"                                     \
    "  - devtype: should be usb, uart, mmc, fat, brom\n"                 \
    "  - interface: specify the controller id\n"                         \
    "e.g.\n"                                                             \
    "  aicupg usb 0\n"                                                   \
    "  aicupg mmc 1\n"                                                   \
    "when devtype is fat: \n"                                            \
    "aicupg [devtype] [blkdev] [interface]\n"                            \
    "- blkdev: should be udisk,mmc \n"                                   \
    "e.g.: \n"                                                           \
    "  aicupg fat udisk 0\n"                                             \
    "  aicupg fat mmc 1\n"                                               \
    "when devtype is brom, device will reset to Boot ROM upgrade mode\n" \
    "  aicupg brom\n"                                                    \
    "  aicupg\n"

static void aicupg_help(void)
{
    puts(AICUPG_HELP);
}

#define AICUPG_ARGS_MAX 4
extern void stdio_unset_uart(int id);

__USED static int ctrlc (void)
{
    switch (getchar()) {
        case 0x03:              /* ^C - Control C */
            return 1;
        default:
            break;
    }

    return 0;
}

static void reboot_device(void)
{
#ifdef AIC_WDT_DRV
    wdt_init();
    printf("Going to reboot ...\n");
    wdt_expire_now();
    while(1)
        continue;
#endif
}

#if defined(AIC_BOOTLOADER_FATFS_SUPPORT)
static int image_header_check(struct image_header_pack *header)
{
    /*check header*/
    if ((strcmp(header->hdr.magic, "AIC.FW") != 0)) {
        pr_err("Error:image check failed, maybe not have a image in media!\n");
        return -1;
    }

    return 0;
}
#endif

#define CHECK_MODE_WAITING 0
#define CHECK_MODE_OK    1
#define CHECK_MODE_TIMEOUT 2

int check_upg_mode(u64 start_tm, long tmo)
{
    u64 cur_tm, tm;
    int mode;

    cur_tm = aic_get_time_us();
    tm = (cur_tm - start_tm);

    mode = aicupg_get_upg_mode();
    if (mode == UPG_MODE_BURN_USER_ID)
        return CHECK_MODE_OK;
    if (mode == UPG_MODE_BURN_IMG_FORCE)
        return CHECK_MODE_OK;

    if (tm < tmo)
        return CHECK_MODE_WAITING;

    return CHECK_MODE_TIMEOUT;
}

static int do_uart_protocol_upg(int intf, char *mode)
{
    int ret = 0;

#if defined(AICUPG_UART_ENABLE)
    struct upg_init init;
    int need_ckmode = 0;
    u64 start_tm;

    init.mode_bits = INIT_MODE(UPG_MODE_FULL_DISK_UPGRADE);
    if (mode) {
        if (!strcmp(mode, "userid")) {
            need_ckmode = 1;
            init.mode_bits = INIT_MODE(UPG_MODE_BURN_USER_ID);
#if defined(AICUPG_FORCE_UPGRADE_SUPPORT)
            /* Enter burn USERID mode also support force burn image */
            init.mode_bits |= INIT_MODE(UPG_MODE_BURN_IMG_FORCE);
#endif
        } else if (!strcmp(mode, "force")) {
            need_ckmode = 1;
            init.mode_bits = INIT_MODE(UPG_MODE_BURN_IMG_FORCE);
        }
    }
    aicupg_initialize(&init);

    stdio_unset_uart(intf);
    aic_upg_uart_init(intf);
    start_tm = aic_get_time_us();

    while (1) {
        if (ctrlc())
            break;
        aic_upg_uart_loop();
        if (need_ckmode) {
            /* Need to check the correct upg mode for Burn UserID
             * and Force Image upgrading
             */
            /* Wait 2s for UserID burn */
            int rst = check_upg_mode(start_tm, 10 * WAIT_UPG_MODE_TMO_US);
            if (rst == CHECK_MODE_TIMEOUT) {
                /* Host tool not set the mode in WAIT_UPG_MODE_TMO_US
                 * exit upg loop and boot kernel
                 */
                ret = -1;
                break;
            } else if (rst == CHECK_MODE_OK) {
                /* Update the start time.
                 * Host tool may change the mode to exit loop
                 */
                start_tm = aic_get_time_us();
            }
        }
    }
#endif

    return ret;
}

extern int usb_init(void);
static int do_usb_protocol_upg(int intf, char *mode)
{
    int ret = 0;

#if defined(AICUPG_USB_ENABLE)
    struct upg_init init;
    int need_ckmode = 0;
    u64 start_tm;

#ifndef AIC_SYSCFG_DRV_V12
    hal_syscfg_usb_phy0_sw_host(0);
#endif
    init.mode_bits = INIT_MODE(UPG_MODE_FULL_DISK_UPGRADE);
    if (mode) {
        if (!strcmp(mode, "userid")) {
            need_ckmode = 1;
            init.mode_bits = INIT_MODE(UPG_MODE_BURN_USER_ID);
#if defined(AICUPG_FORCE_UPGRADE_SUPPORT)
            /* Enter burn USERID mode also support force burn image */
            init.mode_bits |= INIT_MODE(UPG_MODE_BURN_IMG_FORCE);
#endif
        } else if (!strcmp(mode, "force")) {
            need_ckmode = 1;
            init.mode_bits = INIT_MODE(UPG_MODE_BURN_IMG_FORCE);
        }
    }
    aicupg_initialize(&init);
    usb_init();
    start_tm = aic_get_time_us();
    while (1) {
        if (ctrlc())
            break;
        if (need_ckmode) {
            /* Need to check the correct upg mode for Burn UserID
             * and Force Image upgrading
             */
            int rst = check_upg_mode(start_tm, WAIT_UPG_MODE_TMO_US);
            if (rst == CHECK_MODE_TIMEOUT) {
                /* Host tool not set the mode in WAIT_UPG_MODE_TMO_US
                 * exit upg loop and boot kernel
                 */
                ret = -1;
                break;
            } else if (rst == CHECK_MODE_OK) {
                /* Update the start time.
                 * Host tool may change the mode to exit loop
                 */
                start_tm = aic_get_time_us();
            }
        }
    }
#endif

    return ret;
}

extern int hid_init(void);
static int do_hid_protocol_upg(int intf, char *mode)
{
    int ret = 0;

#if defined(AICUPG_HID_ENABLE)
    struct upg_init init;
    int need_ckmode = 0;
    u64 start_tm;

#ifndef AIC_SYSCFG_DRV_V12
    hal_syscfg_usb_phy0_sw_host(0);
#endif
    init.mode_bits = INIT_MODE(UPG_MODE_FULL_DISK_UPGRADE);
    if (mode) {
        if (!strcmp(mode, "userid")) {
            need_ckmode = 1;
            init.mode_bits = INIT_MODE(UPG_MODE_BURN_USER_ID);
#if defined(AICUPG_FORCE_UPGRADE_SUPPORT)
            /* Enter burn USERID mode also support force burn image */
            init.mode_bits |= INIT_MODE(UPG_MODE_BURN_IMG_FORCE);
#endif
        } else if (!strcmp(mode, "force")) {
            need_ckmode = 1;
            init.mode_bits = INIT_MODE(UPG_MODE_BURN_IMG_FORCE);
        }
    }
    aicupg_initialize(&init);
    hid_init();
    start_tm = aic_get_time_us();
    while (1) {
        if (ctrlc())
            break;
        if (need_ckmode) {
            /* Need to check the correct upg mode for Burn UserID
             * and Force Image upgrading
             */
            int rst = check_upg_mode(start_tm, WAIT_UPG_MODE_TMO_US);
            if (rst == CHECK_MODE_TIMEOUT) {
                /* Host tool not set the mode in WAIT_UPG_MODE_TMO_US
                 * exit upg loop and boot kernel
                 */
                ret = -1;
                break;
            } else if (rst == CHECK_MODE_OK) {
                /* Update the start time.
                 * Host tool may change the mode to exit loop
                 */
                start_tm = aic_get_time_us();
            }
        }
    }
#endif

    return ret;
}

__USED static void fat_upg_progress(u32 percent)
{
    /* Show to screen */
    aicfb_draw_bar(percent);

    /*
     * User can add more code to show the progress in customize way
     */
    printf("progress: %d%%\n", percent);
}

static int do_sdcard_upg(int intf)
{
    int ret = 0;

    /* Use SDFAT32 instead of it */
    return ret;
}

int boot_cfg_parse_direct_mode_val(char *strofs, char *name, u32 *offset,
                                   char *attr)
{
    int len, i;
    char *p = NULL;

    if (strofs == NULL || offset == NULL)
        return -1;

    len = strlen(strofs);

    for (i = 0; i < len; i++) {
        name[i] = strofs[i];
        if (name[i] == ',') {
            p = &strofs[i + 1];
            name[i] = ' ';
            break;
        }
    }
    while (i) {
        /* strip blank of tail */
        if (name[i] == ' ' || name[i] == '\t')
            name[i] = 0;
        else
            break;
        i--;
    }

    *offset = 0;
    if (p == NULL)
        return 0;

    if (attr == NULL)
        return 0;

    while (*p == ' ' || *p == '\t')
        p++;

    *offset = strtoul(p, NULL, 0);

    while (*p != 0 && *p != ',')
        p++;

    if (*p == ',')
        strcpy(attr, p + 1);

    return 0;
}

#if defined(AIC_BOOTLOADER_FATFS_SUPPORT)
/* Direct mode:
 * # writetype value can be: spi-nor, spi-nand, mmc
 * writetype=spi-nor
 * # writeintf specify the controller id, default is 0
 * writeintf=0
 * writeboot=bootloader.aic
 * write0=data0.bin,0x1000
 * write1=data1.bin,0x2000
 * write2=data2.bin,0x3000
 * # Support format:
 * #   writeX=file,offset,attribute
 * #   writeX=file,offset
 * #   writeX=file
 * # If offset is absent, default value is 0
 * write3=data.fatfs,0x3000,nftl
 */
static int do_fat_upg_in_direct_mode(char *cfg, u32 clen)
{
    u32 i, ret, maxlen, offset, intf_id, per, cnt;
    char keyname[16], keyval[IMG_NAME_MAX_SIZ], type[16], fname[32], attr[16];

    maxlen = IMG_NAME_MAX_SIZ;
    ret = boot_cfg_get_key_val(cfg, clen, "writetype", 9, keyval, maxlen);
    if (ret == 0) {
        pr_err("Key writetype is not found.\n");
        return -1;
    }

    if (boot_cfg_parse_direct_mode_val(keyval, type, &offset, NULL)) {
        pr_err("parse writetype error.\n");
        return -1;
    }

    intf_id = 0;
    ret = boot_cfg_get_key_val(cfg, clen, "writeintf", 9, keyval, maxlen);
    if (ret > 0) {
        intf_id = strtoul(keyval, NULL, 0);
    }
    printf("Going to program %s on controller %d\n", type, intf_id);

    /* Get the program item count */
    cnt = 0;
    per = 0;
    ret = boot_cfg_get_key_val(cfg, clen, "writeboot", 9, keyval, maxlen);
    if (ret > 0)
        cnt++;
    for (i = 0; i < 32; i++) {
        snprintf(keyname, 16, "write%d", i);
        ret = boot_cfg_get_key_val(cfg, clen, keyname, strlen(keyname), keyval,
                                   maxlen);
        if (ret == 0)
            break;
        cnt++;
    }

    if (cnt == 0)
        goto out;

    fat_upg_progress(0);
    ret = boot_cfg_get_key_val(cfg, clen, "writeboot", 9, keyval, maxlen);
    if (ret > 0) {
        memset(fname, 0, 32);
        boot_cfg_parse_direct_mode_val(keyval, fname, &offset, NULL);
        aicupg_fat_direct_write(type, intf_id, fname, offset, 1, NULL);
        per = 100 / cnt;
        fat_upg_progress(per);
    }

    for (i = 0; i < 32; i++) {
        snprintf(keyname, 16, "write%d", i);
        ret = boot_cfg_get_key_val(cfg, clen, keyname, strlen(keyname), keyval,
                                   maxlen);
        if (ret == 0)
            goto out;
        memset(fname, 0, 32);
        memset(attr, 0, 16);
        boot_cfg_parse_direct_mode_val(keyval, fname, &offset, attr);
        aicupg_fat_direct_write(type, intf_id, fname, offset, 0, attr);
        per += (100 / cnt);
        fat_upg_progress(per);
    }

    fat_upg_progress(100);
out:
    printf("Done\n");
    return 0;
}

static int do_fat_upg_by_single_image(char *image_name, char *protection)
{
    struct image_header_pack *hdrpack;
    int ret;
    ulong actread;

    hdrpack = (void *)aicos_malloc_align(0, sizeof(struct image_header_pack), CACHE_LINE_SIZE);
    if (!hdrpack) {
        pr_err("Error, malloc hdrpack failed.\n");
        return -1;
    }
    memset(hdrpack, 0, sizeof(struct image_header_pack));

    ret = aic_fat_read_file(image_name, (void *)hdrpack, 0,
                            sizeof(struct image_header_pack), &actread);
    if (actread != sizeof(struct image_header_pack) || ret) {
        printf("Error:read file %s failed!\n", image_name);
        ret = -1;
        goto err;
    }

    /*check header*/
    ret = image_header_check(hdrpack);
    if (ret) {
        pr_err("check image header failed!\n");
        ret = -1;
        goto err;
    }

    /*write data to media*/
    ret = aicupg_fat_write(image_name, protection, &hdrpack->hdr);
    if (ret == 0) {
        pr_err("fat write data failed!\n");
        ret = -1;
        goto err;
    }

    ret = 0;
err:
    if (hdrpack)
        aicos_free_align(0, hdrpack);
    return ret;
}
#endif

static int do_fat_upg(int intf, char *const blktype)
{

    int ret = 0;
#if defined(AIC_BOOTLOADER_FATFS_SUPPORT)
    char image_name[IMG_NAME_MAX_SIZ] = {0};
    char protection[PROTECTION_PARTITION_LEN] = {0};
    ulong actread, offset, bootlen;
    char *file_buf;

    if (!strcmp(blktype, "udisk")) {
        /*usb init*/
#if defined(AICUPG_UDISK_ENABLE)
        if (usbh_init() < 0) {
            pr_err("usbh init failed!\n");
            ret = -1;
            return ret;
        }

        ret = aic_fat_set_blk_dev(intf, BLK_DEV_TYPE_MSC);
        if (ret) {
            pr_err("set blk dev failed.\n");
            return ret;
        }

        usbh_hub_poll();
#else
        pr_err("udisk upgrade disabled.\n");
#endif
    } else if (!strcmp(blktype, "mmc")) {
#if defined(AICUPG_SDCARD_ENABLE)
        ret = mmc_init(intf);
        if (ret) {
            printf("sdmc %d init failed.\n", intf);
            return ret;
        }

        ret = aic_fat_set_blk_dev(intf, BLK_DEV_TYPE_MMC);
        if (ret) {
            pr_err("set blk dev failed.\n");
            return ret;
        }
#else
        pr_err("sdcard upgrade disabled.\n");
#endif
    } else {
        return ret;
    }

    file_buf = (char *)aicos_malloc_align(0, 2048, CACHE_LINE_SIZE);
    if (!file_buf) {
        pr_err("Error, malloc buf failed.\n");
        ret = -1;
        goto err;
    }
    memset((void *)file_buf, 0, 2048);

    ret = aic_fat_read_file("bootcfg.txt", (void *)file_buf, 0, 2048, &actread);
    if (actread == 0 || ret) {
        printf("Error:read file bootcfg.txt failed!\n");
        goto err;
    }

    ret = boot_cfg_get_boot0(file_buf, actread, image_name, IMG_NAME_MAX_SIZ,
                             &offset, &bootlen);

    aicupg_fat_set_process_cb(fat_upg_progress);

    if (offset > 0) {
        /* Perform upgrading from image file */
        ret = boot_cfg_get_protection(file_buf, actread, protection,
                                      PROTECTION_PARTITION_LEN);
        if (ret)
            pr_warn("No protected partition.\n");
        else
            pr_info("Protected=%s\n", protection);
        ret = do_fat_upg_by_single_image(image_name, protection);
    } else {
        ret = do_fat_upg_in_direct_mode(file_buf, actread);
    }

    if (!ret) {
        printf("\nPlug-out SDCard/UDISK to reboot device.\n");
        printf(" CTRL+C exit to command line.\n");

        /* Reboot when SDCard/UDISK plug-out */
        while (1) {
            /* If failed to read data, reboot */
            ret = aic_fat_read_file("bootcfg.txt", (void *)file_buf, 0, 2048,
                                    &actread);
            if (actread == 0 || ret) {
                aicos_mdelay(1000);
                reboot_device();
            }

            if (ctrlc()) {
                /* Exit to cmd line */
                ret = 0;
                break;
            }
            aicos_mdelay(100);
        }
    }

err:
    if (file_buf)
        aicos_free_align(0, file_buf);
#endif
    return ret;
}

static void do_brom_upg(void)
{
#ifdef AIC_WRI_DRV
    aic_set_reboot_reason(REBOOT_REASON_UPGRADE);
#endif

    reboot_device();
}

static int do_aicupg(int argc, char *argv[])
{
    char *devtype = NULL, *mode;
    int intf, ret = 0;

    mode = NULL;
    if ((argc == 1) || ((argc == 2) && (!strcmp(argv[1], "brom")))) {
        do_brom_upg();
        return 0;
    }

    if ((argc < 3) || (argc > AICUPG_ARGS_MAX))
        goto help;
    devtype = argv[1]; /* mmc  usb fat */
    if (argc >= 4 && argv[3])
        intf = strtol(argv[3], NULL, 0);
    else
        intf = strtol(argv[2], NULL, 0);

    if (devtype == NULL)
        goto help;
    if (!strcmp(devtype, "uart")) {
        if (argc >= 4)
            mode = argv[3];
        ret = do_uart_protocol_upg(intf, mode);
    }
    if (!strcmp(devtype, "usb")) {
        if (argc >= 4)
            mode = argv[3];
        ret = do_usb_protocol_upg(intf, mode);
    }
    if (!strcmp(devtype, "hid")) {
        if (argc >= 4)
            mode = argv[3];
        ret = do_hid_protocol_upg(intf, mode);
    }
    if (!strcmp(devtype, "mmc"))
        ret = do_sdcard_upg(intf);
    if (!strcmp(devtype, "fat"))
        ret = do_fat_upg(intf, argv[2]);

    return ret;

help:
    aicupg_help();
    return ret;
}

CONSOLE_CMD(aicupg, do_aicupg, "Upgrading mode command.");
