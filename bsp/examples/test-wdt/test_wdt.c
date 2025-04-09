/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <aic_core.h>
#include <drivers/watchdog.h>
#include <aic_drv_wdt.h>
#include <hal_wdt.h>
#include <getopt.h>

#define AIC_WDT_DEV_NAME           "wdt"

irqreturn_t aic_wdt_irq(int irq, void *arg)
{
    rt_kprintf("Watchdog chan0 IRQ happened\n");

    return IRQ_HANDLED;
}

void wdt_feed_thread_entry(void *parameter)
{
    rt_device_t wdt_dev = RT_NULL;
    do {
        wdt_dev = rt_device_find(AIC_WDT_DEV_NAME);
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
        rt_thread_mdelay(200);
    } while (wdt_dev);
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
    int wreg_switch,timeout = 0;
    rt_device_t wdt_dev = RT_NULL;
    rt_thread_t wdt_thread = RT_NULL;

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    wdt_dev =  rt_device_find(AIC_WDT_DEV_NAME);
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
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_CLR_THD, &timeout);
                rt_kprintf("set clear threshold:%d\n", timeout);
                break;
            case 's':
                timeout = strtoul(optarg, NULL, 10);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
                rt_kprintf("set timeout:%d\n", timeout);
                break;
            case 'g':
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &timeout);
                rt_kprintf("timeout:%d\n", timeout);
                break;
            case 'p':
                timeout = strtoul(optarg, NULL, 10);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_IRQ_TIMEOUT, &timeout);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_IRQ_ENABLE, &aic_wdt_irq);
                rt_kprintf("set pretimeout:%d\n", timeout);
                break;
            case 'k':
                wdt_thread = rt_thread_create("wdt_feed_thread", wdt_feed_thread_entry,
                                            RT_NULL, 1024, 10, 10);
                if (wdt_thread != RT_NULL) {
                    rt_thread_startup(wdt_thread);
                    rt_kprintf("keep feeding the dog!\n");
                } else {
                    rt_kprintf("wdt thread create fail!\n");
                }
                break;
            case 'r':
            #ifdef AIC_WDT_DRV_V11
                status = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_GET_RST_EN, RT_NULL);
                if (status)
                    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_RST_SYS, RT_NULL);
                else
                    rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_RST_CPU, RT_NULL);
                break;
            #endif
            case 'w':
                wreg_switch = atoi(optarg);
                rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_EN_REG, &wreg_switch);

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

    return 0;
}
MSH_CMD_EXPORT_ALIAS(test_wdt, test_wdt, Reboot the system);
