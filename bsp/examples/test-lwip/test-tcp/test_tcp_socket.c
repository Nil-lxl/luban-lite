/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu@artinchip.com
 */
#include "rtconfig.h"

#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#endif

#include <stdio.h>
#include "sys/socket.h"
#include "aic_osal.h"

#define SERVER_PORT    8080

#define SOCK_RCV_BUF_SZ     4096
static u8_t sock_rcv_buf[SOCK_RCV_BUF_SZ] = "Aicinchip!\n";

static int socket_startup()
{
    int ret_val = -1;
    int listen_fd = -1;
    struct sockaddr_in srv_sockaddr;

    memset(&srv_sockaddr, 0, sizeof(struct sockaddr_in));

    ret_val = socket(AF_INET, SOCK_STREAM, 0);
    if (ret_val < 0)
    {
        printf("Socket alloc error\n");
        return -1;
    }
    listen_fd = ret_val;

    int  attr_bind = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &attr_bind, sizeof(attr_bind));

    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_len = sizeof(struct sockaddr_in);
    srv_sockaddr.sin_port = htons(SERVER_PORT);
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret_val = bind(listen_fd, (void *)&srv_sockaddr, sizeof(struct sockaddr_in));
    if (ret_val < 0)
    {
        closesocket(listen_fd);
        printf("!!!!!!!Bind error\n");
        return -1;
    }

    ret_val = listen(listen_fd, 3);
    if (ret_val < 0)
    {
        closesocket(listen_fd);
        printf("!!!!!!!Listen error\n");
        return -1;
    }

    return listen_fd;
}

static int set_tcp_keepalive(int clint_fd)
{
    int keepalive = 1;
    setsockopt(clint_fd,SOL_SOCKET, SO_KEEPALIVE,(void*)&keepalive, sizeof(keepalive));

    int keepalive_idle = 30;
    setsockopt(clint_fd,IPPROTO_TCP, TCP_KEEPIDLE,(void*)&keepalive_idle,sizeof(keepalive_idle));

    int keepalive_intvl = 1;
    setsockopt(clint_fd,IPPROTO_TCP, TCP_KEEPINTVL,(void*)&keepalive_intvl,sizeof(keepalive_intvl));

    int keepalive_count = 3;
    setsockopt(clint_fd,IPPROTO_TCP, TCP_KEEPCNT,(void*)&keepalive_count,sizeof(keepalive_count));

    return 0;
}

void tcp_server_echo_back_thread(void *para)
{
    int i;
    int ret_val = 0;
    int listen_fd, max_fd, clint_fd;
    struct sockaddr_in cln_sockaddr;
    int length;

    socklen_t cln_length;
    fd_set read_set,all_set;

restart:
    FD_ZERO(&all_set);
    memset(&cln_sockaddr, 0, sizeof(struct sockaddr_in));

    ret_val = socket_startup();
    if (ret_val < 0)
        return;

    listen_fd = ret_val;
    max_fd = listen_fd;
    FD_SET(listen_fd, &all_set);

    printf("Tcp Server echo back task start, port : 0x%02x\n", SERVER_PORT);
    while (1)
    {
        read_set = all_set;

        ret_val = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if (ret_val > 0)
        {
            if (FD_ISSET(listen_fd, &read_set))
            {
                clint_fd = accept(listen_fd, (void *)&cln_sockaddr, &cln_length);
                if (clint_fd < 0)
                {
                    printf("Accept error\n");
                    continue;
                }

                FD_SET(clint_fd, &all_set);

                set_tcp_keepalive(clint_fd);

                if (clint_fd > max_fd)
                {
                    max_fd = clint_fd;
                }

                if (ret_val == 1)
                {
                    continue;
                }
            }

            for (i = listen_fd + 1; i < FD_SETSIZE; i++)
            {
                if (FD_ISSET(i, &read_set))
                {
                    ret_val = recv(i, sock_rcv_buf, SOCK_RCV_BUF_SZ, 0);
                    if (ret_val < 0)
                    {
                        if(errno == EWOULDBLOCK || errno == EAGAIN)
                            continue;
                        printf("socket recv ERROR!!!\r\n");
                        closesocket(i);
                        FD_CLR(i, &all_set);
                        continue;
                    } else if (ret_val == 0) {
                        printf("remote has been disconnected\r\n");
                        closesocket(i);
                        FD_CLR(i, &all_set);
                        continue;
                    }

                    length = ret_val;
                    ret_val = send(i, sock_rcv_buf, length, 0);
                    if (ret_val <= 0)
                    {
                        printf("socket send ERROR!!!\r\n");
                        closesocket(i);
                        FD_CLR(i, &all_set);
                        continue;
                    }
                }
            }
        }
        else
        {
            for (i = listen_fd; i < FD_SETSIZE; i++)
            {
                if (FD_ISSET(i, &all_set))
                    closesocket(i);
            }

            printf("\n#######Restart######\n");
            goto restart;
        }
    }
}

int cmd_test_tcp(int argc, char *argv[])
{
    static int init_flag = 0;

    if (init_flag) {
        printf("Failed: Only start once\n");
        return -1;
    }
    init_flag++;

    aicos_thread_create("tcp server echo back", 4096, 22, tcp_server_echo_back_thread, NULL);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_tcp, test_tcp, test tcp server ehco);
