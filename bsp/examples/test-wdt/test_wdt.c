/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <aic_core.h>
#include <drivers/watchdog.h>
#include <aic_drv_wdt.h>
#include <hal_wdt.h>
#include <getopt.h>

#define AIC_WDT_DEV_NAME            "wdt"
#define KEEP_FEED_TIMES             5

static aicos_sem_t g_wdt_sem = NULL;
static u64 g_wdt_start_ms = 0;

int aic_wdt_irq_cb(void)
{
    printf("Watchdog IRQ pretimeout happened after %ld ms\n",
           (long)(aic_get_time_ms() - g_wdt_start_ms));
    if (g_wdt_sem)
        aicos_sem_give(g_wdt_sem);
    return 0;
}

void wdt_feed_thread_entry(void *parameter)
{
    int timeout = *((int *)parameter) * 1000;
    rt_device_t wdt_dev = RT_NULL;
    int i;

    wdt_dev = rt_device_find(AIC_WDT_DEV_NAME);
    if (!wdt_dev) {
        pr_err("Failed to open %s\n", AIC_WDT_DEV_NAME);
        return;
    }

    if (timeout <= 0)
        timeout = 1000;

    for (i = 0; i < KEEP_FEED_TIMES; i++) {
        aicos_msleep(timeout);
        printf("[%lld] %d. Feed watchdog\n", aic_get_time_ms(), i);
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
    }

    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_STOP, NULL);
}

static void usage(char * program)
{
    printf("\n");
    printf("Usage: %s [-s timeout] [-p pretimeout] [-c clear threshold] [-g] [-k] [-h]\n",\
        program);
    printf("\t -s, --set-timeout\tSet a timeout, in second\n");
    printf("\t -p, --set-pretimeout\tSet a pretimeout, in second\n");
    printf("\t -c, --set-clear threshold\tSet clear threshold,in second(0~3)\n");
    printf("\t -g, --get-timeout\tGet the current timeout, in second\n");
    printf("\t -k, --keepalive\tKeepalive the watchdog\n");
    #ifdef AIC_WDT_DRV_V11
    printf("\t -r, --change reset object \t change reset cpu or system\n");
    #endif
    printf("\t -w, --write reg enable/disable\tEnable(1)/Disable(0) write protect reg function\n");
    printf("\t -h, --help \n");
    printf("\n");
}

int test_wdt(int argc, char **argv)
{
    int opt, ret = 0;
    __unused int status;
    int wreg_switch, timeout = 0, pretimeout = 0;
    rt_device_t wdt_dev = RT_NULL;
    rt_thread_t wdt_thread = RT_NULL;
    bool keepalive = false;

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    wdt_dev = rt_device_find(AIC_WDT_DEV_NAME);
    if (!wdt_dev) {
        rt_kprintf("Failed to open %s device\n", AIC_WDT_DEV_NAME);
        return -1;
    }
    ret = rt_device_init(wdt_dev);
    if (ret < 0) {
        rt_kprintf("Failed to init %s device\n", AIC_WDT_DEV_NAME);
        return -1;
    }

    optind = 0;
    while ((opt = getopt(argc, argv, "s:p:c:w:gkrh")) != -1) {
        switch (opt) {
            case 'c':
                timeout = strtoul(optarg, NULL, 10);
                ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_CLR_THD, &timeout);
                rt_kprintf("set clear threshold:%d\n", timeout);
                break;
            case 's':
                timeout = strtoul(optarg, NULL, 10);
                ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
                ret |= rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
                rt_kprintf("set timeout:%d\n", timeout);
                break;
            case 'g':
                ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &timeout);
                rt_kprintf("timeout:%d\n", timeout);
                return timeout;
            case 'p':
                pretimeout = strtoul(optarg, NULL, 10);
                g_wdt_sem = aicos_sem_create(0);
                if (!g_wdt_sem) {
                    pr_err("Failed to create sem\n");
                    return -1;
                }
                ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_IRQ_TIMEOUT, &pretimeout);
                ret |= rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_ENABLE, &aic_wdt_irq_cb);
                if (ret)
                    return ret;
                rt_kprintf("set pretimeout:%d\n", pretimeout);
                g_wdt_start_ms = aic_get_time_ms();
                break;
            case 'k':
                keepalive = true;
                break;
            case 'r':
            #ifdef AIC_WDT_DRV_V11
                status = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_RST_EN, RT_NULL);
                if (status)
                    ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_RST_SYS, RT_NULL);
                else
                    ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_RST_CPU, RT_NULL);
                break;
            #endif
            case 'w':
                wreg_switch = atoi(optarg);
                ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_EN_REG, &wreg_switch);

                if (wreg_switch == 1)
                    rt_kprintf("enable write protect reg function\n");
                else if (wreg_switch == 0)
                    rt_kprintf("disable write protect reg function\n");
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    if (keepalive) {
        if (timeout >= 2)
            timeout -= 1;
        else
            timeout = 1;
        wdt_thread = rt_thread_create("wdt_feed_thread", wdt_feed_thread_entry,
                                      &timeout, 2048, 10, 10);
        if (wdt_thread != RT_NULL) {
            rt_thread_startup(wdt_thread);
            timeout = (timeout + 1) * KEEP_FEED_TIMES;
            printf("Keep feeding the watchdog %d times.\n", KEEP_FEED_TIMES);
            printf("Waiting for %d sec ...\n", timeout);
            aicos_msleep(timeout * 1000);
        } else {
            rt_kprintf("wdt thread create fail!\n");
            ret = -1;
        }
    }

    if (g_wdt_sem) {
        rt_kprintf("Wait for the pretimeout IRQ ...\n");
        ret = aicos_sem_take(g_wdt_sem, (pretimeout + 2) * 1000);
        if (ret < 0)
            pr_err("Timeout\n");
        else
            ret = (aic_get_time_ms() - g_wdt_start_ms + 300) / 1000;

        aicos_sem_delete(g_wdt_sem);
        if (rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_DISABLE, RT_NULL))
            return -1;
        if (rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_STOP, RT_NULL))
            return -1;
        g_wdt_sem = NULL;
    }

    return ret;
}
MSH_CMD_EXPORT_ALIAS(test_wdt, test_wdt, Reboot the system);
