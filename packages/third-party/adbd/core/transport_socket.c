/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-11     heyuanjie87  the first version
 */

#include <rtthread.h>
#include <string.h>
#include "transport.h"

#ifdef LPKG_ADB_TR_TCPIP_ENABLE
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <dfs_select.h>
#include <dfs_poll.h>

//#define DBG_ENABLE
#define DBG_SECTION_NAME "ADB iosk"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#if defined(LPKG_ADB_USING_SSDP)
#include "lssdp_service.h"
#endif

#ifndef LPKG_ADB_TR_STACK_SIZE
#define LPKG_ADB_TR_STACK_SIZE     1280
#endif

static int is_running = 0;

static int sk_read(int fd, void *buf, int size)
{
    return recv(fd, buf, size, 0);
}

static int sk_write(int fd, void *buf, int size)
{
    return send(fd, buf, size, 0);
}

static int sk_poll(int fd, int evt, int ms)
{
    struct pollfd pfd = {0};
    int ret;

    pfd.fd = fd;
    if (evt & TRE_READ)
        pfd.events |= POLLIN;
    if (evt & TRE_WRITE)
        pfd.events |= POLLOUT;

    ret = poll(&pfd, 1, ms);
    if (ret == 0)
        return 0;

    ret = 0;
    if (pfd.revents & POLLIN)
        ret |= TRE_READ;
    if (pfd.revents & POLLOUT)
        ret |= TRE_WRITE;
    if (pfd.revents & (POLLERR | POLLHUP))
        ret |= TRE_ERROR;

    return ret;
}

static void sk_close(int fd)
{
    closesocket(fd);
}

static const struct adb_tr_ops _ops =
{
    sk_read,
    sk_write,
    sk_poll,
    sk_close,
};

#if defined(LPKG_ADB_USING_SSDP)
int adb_ssdp_service_register(void)
{
    struct lssdp_service adb_service =
    {
        .name                        = "adb",
        .info.search_target          = "urn:rt-thread:service:ssdp",
        .info.unique_service_name    = "adb file sync service",
        .info.sm_id                  = "80000001",
        .info.device_type            = "RT-Thread adb device",
        .info.suffix                 = ":5555",
    };

    if (lssdp_service_add(&adb_service) != 0)
    {
        LOG_E("service %s add failed!", adb_service.name);
    }

    return 0;
}

static int adb_ssdp_service_unregister(void)
{
    struct lssdp_service adb_service = {
        .name = "adb",
    };

    return lssdp_service_del(&adb_service);
}
#endif

static void tcp_server(void *arg)
{
    int ret;
    int sock, connected;
    struct sockaddr_in server_addr, client_addr;

    struct timeval timeout;
    fd_set readset;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int port;
#if defined(LPKG_ADB_USING_SSDP)
    int ssdp_reg = 0;
#endif

    is_running = 1;
    port = (int)arg;

#ifdef RT_USING_NETDEV
    extern struct netdev *netdev_default;
    {
        int try = 100;
        while (try--)
        {
            if (netdev_default != RT_NULL)
            {
                break;
            }
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        }
        if (netdev_default == RT_NULL)
        {
            LOG_E("No default network card was found");
            return;
        }
    }
#endif

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        LOG_E("Create socket error");
        goto __exit;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0x0, sizeof(server_addr.sin_zero));

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        LOG_E("Unable to bind");
        goto __exit;
    }

    if (listen(sock, 1) == -1)
    {
        LOG_E("Listen error");
        goto __exit;
    }

    while (is_running)
    {
        FD_ZERO(&readset);
        FD_SET(sock, &readset);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

#if defined(LPKG_ADB_USING_SSDP)
        if (adb_transport_isexist())
        {
            if (ssdp_reg)
            {
                adb_ssdp_service_unregister();
                ssdp_reg = 0;
            }
        }
        else if (!ssdp_reg)
        {
            adb_ssdp_service_register();
            ssdp_reg = 1;
        }
#endif

        /* Wait for read or write */
        if (select(sock + 1, &readset, RT_NULL, RT_NULL, &timeout) <= 0)
            continue;

        /* ?????????socket???,??????????? */
        connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
        /* ?????????socket */
        if (connected < 0)
        {
            LOG_E("accept connection failed! errno = %d", errno);
            continue;
        }
        if (adb_transport_isexist())
        {
            closesocket(connected);
            continue;
        }

        LOG_D("accept connection");

        adb_transport_unregister(0);
        ret = adb_transport_register(TR_TCPIP, connected, &_ops);
        if (ret != 0)
        {
            closesocket(connected);
            LOG_E("register transport tcpip fail");
        }
    }

__exit:
    if (sock >= 0)
    {
        closesocket(sock);
    }
    is_running = 0;
}

int adb_tcpip(int port)
{
    rt_thread_t tid;
    int ret = 0;

    if (is_running)
    {
        LOG_E("adbd tcpip is exist");
    }

    tid = rt_thread_create("adbd-sk",
                           tcp_server,
                           (void *)port,
                           LPKG_ADB_TR_STACK_SIZE,
                           22,
                           20);
    if (tid)
    {
        ret = rt_thread_startup(tid);
    }

    return ret;
}

int adb_socket_init(void)
{
    int ret;

    ret = adb_tcpip(5555);

    return ret;
}
INIT_APP_EXPORT(adb_socket_init);
#endif
