/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#define DBG_TAG "mtop"

#include <finsh.h>
#include <getopt.h>

#include "aic_core.h"
#include "aic_hal_mtop.h"
#include "aic_drv_mtop.h"

extern struct mtop_dev aic_mtop;

static aicos_sem_t g_mtop_update = NULL;
static rt_device_t g_mtop_dev = RT_NULL;
static u32 g_mtop_delay = 1;

static void usage(char * program)
{
    printf("\n");
    printf("Usage: %s [-n iter] [-d delay] [-h]\n", program);
    printf("   -n NUM   Number of updates before this program exiting.\n");
    printf("   -d NUM   Seconds to wait between update.\n");
    printf("   -h Show this help.\n");
    printf("\n");
}

rt_err_t mtop_update_data_done(rt_device_t dev, rt_size_t size)
{
    RT_UNUSED(size);

    if (g_mtop_update)
        aicos_sem_give(g_mtop_update);
    return RT_EOK;
}

static void mtop_update(void)
{
    struct port_bandwidth *bw = NULL;
    int i, j, index, pos;

    /* Clear screen */
    printf("\033[2J\033[H\t\t\n");

    for (i = 0; i < hal_mtop_get_group_num(); i++) {
        printf("\t%s Bandwidth usage (MB/s):\n", grp_name[i]);
        printf("\t=================================\n");
        printf("\t\t\tRead%8sWrite\n", " ");
        for (j = 0; j < hal_mtop_get_port_num(); j++) {
            pos = group_id[i] * 8 + j;

            if ((1 << pos) & PORT_BITMAP) {
                index = i * hal_mtop_get_port_num() + j;
                bw = &aic_mtop.mtop_handle.port_bw[index];
                printf("\t%2d %-8s %4d.%03d %8d.%03d\n", j + 1, prt_name[j],
                    bw->rcnt / (g_mtop_delay * 1000000),
                    bw->rcnt % 1000000 / (g_mtop_delay * 1000),
                    bw->wcnt / (g_mtop_delay * 1000000),
                    bw->wcnt % 1000000 / (g_mtop_delay * 1000));
            }
        }
        printf("\n");
    }
}

static void test_mtop_close(void)
{
    if (g_mtop_dev) {
        rt_device_close(g_mtop_dev);
        g_mtop_dev = RT_NULL;
    }
}

static void test_mtop_thread(void *arg)
{
    s32 loops = (s32)(ptr_t)arg;

    mtop_update();
    while (loops == -1 || loops > 0) {
        aicos_sem_take(g_mtop_update, AICOS_WAIT_FOREVER);
        if (loops > 0)
            loops--;
        mtop_update();
    }

    printf("mtop update done\n\n");
    test_mtop_close();
}

int test_mtop(int argc, char **argv)
{
    static aicos_thread_t thid = NULL;
    rt_err_t ret = RT_EOK;
    s32 loops = -1;
    int opt;

    optind = 0;
    while ((opt = getopt(argc, argv, "n:d:h")) != -1) {
        switch (opt) {
        case 'n':
            if (!optarg) {
                usage(argv[0]);
                return -1;
            }
            loops = strtoul(optarg, NULL, 10);
            break;
        case 'd':
            if (!optarg) {
                usage(argv[0]);
                return -1;
            }
            g_mtop_delay = strtoul(optarg, NULL, 10);
            break;
        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }

    if (thid) {
        /* Stop the mtop thread if it is running */
        if (g_mtop_dev) {
            aicos_thread_delete(thid);
            test_mtop_close();
            printf("Stop the mtop thread\n");
            return 0;
        }
        /* The previous thread is finished, so reset the thread ID */
        thid = NULL;
    }

    if (!g_mtop_dev)
        g_mtop_dev = rt_device_find("mtop");
    if (!g_mtop_dev) {
        LOG_E("mtop device not found\n");
        return -1;
    }

    ret = rt_device_init(g_mtop_dev);
    if (ret) {
        LOG_E("device init error\n");
        return -1;
    }

    ret = rt_device_open(g_mtop_dev, 0);
    if (ret) {
        LOG_E("device open error\n");
        return -1;
    }

    ret = rt_device_set_rx_indicate(g_mtop_dev, mtop_update_data_done);
    if (ret) {
        LOG_E("device set callback error\n");
        return -1;
    }

    ret = rt_device_control(g_mtop_dev, MTOP_SET_PERIOD_MODE, &g_mtop_delay);
    if (ret) {
        LOG_E("set period error\n");
        return -1;
    }

    ret = rt_device_control(g_mtop_dev, MTOP_ENABLE, 0);
    if (ret) {
        LOG_E("mtop enable error\n");
        return -1;
    }

    if (g_mtop_update == NULL)
        g_mtop_update = aicos_sem_create(0);

    if (thid == NULL) {
        thid = aicos_thread_create("mtop", 8096, 25, test_mtop_thread, (void *)(ptr_t)loops);
        if (thid == NULL) {
            LOG_E("Failed to create thread\n");
            return -1;
        }
    }
    return 0;
}
MSH_CMD_EXPORT_ALIAS(test_mtop, mtop, test mtop function);
