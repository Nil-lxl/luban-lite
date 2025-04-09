/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-30     armink       the first version
 * 2018-08-27     Murphy       update log
 */

#include <rtthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <finsh.h>
#include <ymodem.h>
#include <ota.h>
#include <env.h>
#include <absystem_os.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME               "ymodem"
#ifdef OTA_DOWNLOADER_DEBUG
#define DBG_LEVEL                      DBG_LOG
#else
#define DBG_LEVEL                      DBG_INFO
#endif
#define DBG_COLOR
#include <rtdbg.h>

#ifdef LPKG_USING_YMODEM_OTA
static uint8_t enable_output_log = 0;

static enum rym_code ymodem_on_begin(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    char *file_name, *file_size;
    int ret = RT_ERROR;

    /* calculate and store file size */
    file_name = (char *)&buf[0];
    file_size = (char *)&buf[rt_strlen(file_name) + 1];

    rt_kprintf("Ymodem file_name:%s file_size:%d\n", file_name, atol(file_size));

    if (!enable_output_log) {
        ulog_tag_lvl_filter_set("ota", LOG_LVL_ERROR);
        ulog_tag_lvl_filter_set("ota.burn", LOG_LVL_ERROR);
        ulog_tag_lvl_filter_set("absystem", LOG_LVL_ERROR);
    }

    ret = ota_init();
    if (ret != RT_EOK) {
        LOG_E("ota initialization failed.");
        return RYM_CODE_CAN;
    }

    return RYM_CODE_ACK;
}

static enum rym_code ymodem_on_data(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    if(ota_shard_download_fun((char *)buf, len) < 0) {
        rt_kprintf ("ota download failed.\r\n");
        return RYM_CODE_CAN;
    }

    return RYM_CODE_ACK;
}

void ymodem_ota(uint8_t argc, char **argv)
{
    struct rym_ctx rctx;
    const char str_usage[] = "Usage: ymodem_ota -t <device name>.\n";
    int i;
    int ret;
    rt_device_t dev = rt_console_get_device();
    enable_output_log = 0;

    for (i=1; i<argc;)
    {
        /* change default device to transfer */
        if (!strcmp(argv[i], "-t"))
        {
            if (argc <= (i+1))
            {
                rt_kprintf("%s", str_usage);
                return;
            }
            dev = rt_device_find(argv[i+1]);
            if (dev == RT_NULL)
            {
                rt_kprintf("Device (%s) find error!\n", argv[i+1]);
                return;
            }
            i += 2;
        }
        /* NOT supply parameter */
        else
        {
            rt_kprintf("%s", str_usage);
            return;
        }
    }

    if (dev != rt_console_get_device()) {enable_output_log = 1;}
    rt_kprintf("Save firmware on with device \"%s\".\n", dev->parent.name);
    rt_kprintf("Warning: Ymodem has started! This operator will not recovery.\n");
    rt_kprintf("Please select the ota firmware file and use Ymodem to send.\n");

    if (!rym_recv_on_device(&rctx, dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                            ymodem_on_begin, ymodem_on_data, NULL, RT_TICK_PER_SECOND))
    {
        rt_kprintf("Download firmware to flash success.\n");
        rt_kprintf("System now will restart...\r\n");

        /* wait some time for terminal response finish */
        rt_thread_delay(rt_tick_from_millisecond(200));

        ret = aic_upgrade_end();
        if (ret) {
            rt_kprintf("Aic upgrade end.\r\n");
        }

        /* Reset the device, Start new firmware */
        extern void rt_hw_cpu_reset(void);
        rt_hw_cpu_reset();
        /* wait some time for terminal response finish */
        rt_thread_delay(rt_tick_from_millisecond(200));
    }
    else
    {
        /* wait some time for terminal response finish */
        rt_thread_delay(RT_TICK_PER_SECOND);
        rt_kprintf("Update firmware fail.\n");
    }

    return;
}
/**
 * msh />ymodem_ota
*/
MSH_CMD_EXPORT(ymodem_ota, Use Y-MODEM to download the firmware);

#endif /* LPKG_USING_YMODEM_OTA */
