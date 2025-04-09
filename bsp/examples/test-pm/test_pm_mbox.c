/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <finsh.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <rtdevice.h>
#include <rtthread.h>

#include "aic_core.h"
#include "aic_log.h"

#include "hal_mailbox.h"
#include "rpmsg_lite.h"
#include "rpmsg_ns.h"
#include "rpmsg_aic.h"

static struct rpmsg_lite_instance *g_pm_rpmsg = NULL;
static struct rpmsg_lite_endpoint *g_pm_ept = NULL;
static char g_name[32] = "";

static int32_t pm_mbox_recv_cb(void *payload, uint32_t payload_len,
                              uint32_t src, void *priv)
{
    int ret = 0;
    struct aic_rpmsg *msg = (struct aic_rpmsg *)payload;
    struct aic_rpmsg tx_msg;

    LOG_D("\n%s: Recv cmd 0x%x, seq %d, len %d from ept %d\n",
           g_name, msg->cmd, msg->seq, msg->len, (u32)src);

    if (msg->cmd == RPMSG_CMD_IS_IDLE)
    {
        rt_memset(&tx_msg, 0, sizeof(struct aic_rpmsg));

        tx_msg.cmd = RPMSG_CMD_ACK;
        ret = rpmsg_lite_send(g_pm_rpmsg, g_pm_ept, src, (char *)&tx_msg,
                              4, 1000);
        if (ret < 0)
            LOG_E("%s: Failed to send msg\n", g_name);
    }
    else if (msg->cmd == RPMSG_CMD_PRE_STANDBY)
    {
        LOG_D("request sleep\n");
        /* request to enter suspend, the system will enter suspend in idle */
        rt_pm_release(0);
    }

    return 0;
}

static void rpmsg_wait_thread(void *parameter)
{
    LOG_I("Wait for the notify from master ...\n");
    rpmsg_lite_wait_for_link_up(g_pm_rpmsg, RL_BLOCK);
    LOG_I("Yeah! The master is online, try announce ...\n");
    rpmsg_ns_announce(g_pm_rpmsg, g_pm_ept, g_name, 0);
}

static int test_pm_mbox(int argc, char **argv)
{
    rt_err_t ret = RT_EOK;
    void *shmem = NULL;
    rt_thread_t thread;

    if (argc != 2)
    {
        rt_kprintf("Usage:\n\t%s LINK_NAME\n", argv[0]);
        return -RT_EINVAL;
    }

    strncpy(g_name, argv[1], 31);

    shmem = (void *)AIC_MAILBOX_SHMEM1_ADDR;
    LOG_I("%s: Init RPMsg-lite as a remote ...\n", g_name);

    //When SP/SC communicate with CSYS, the link_id is 0
    g_pm_rpmsg = rpmsg_lite_remote_init(shmem, 0, 0, g_name);
    if (!g_pm_rpmsg)
    {
        LOG_E("%s: Failed to init RPMsg-lite instance\n", g_name);
        ret = -RT_ERROR;
        goto __exit;
    }

    g_pm_ept = rpmsg_lite_create_ept(g_pm_rpmsg, RL_REMOTE,
                                      pm_mbox_recv_cb, NULL);
    if (g_pm_ept == NULL)
    {
        LOG_E("%s: Failed to init ept\n", g_name);
        ret = -RT_ERROR;
        goto __exit;
    }

    thread = rt_thread_create("rpmsg_wait", rpmsg_wait_thread, RT_NULL,
                                  4096, 10, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        LOG_E("create rpmsg_wait thread failed!\n");
        ret = -RT_ERROR;
    }

__exit:
    return ret;
}
MSH_CMD_EXPORT_ALIAS(test_pm_mbox, pm_mbox, test pm by Mailbox);
