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

#define UNICAST_SERVER_PORT   8080
#define MUTICAST_SERVER_PORT  8081
#define BOARDCAST_SERVER_PORT 8082

#define UDP_RECV_BUF_SIZE 4096

void udp_test_unicast_thread(void *para)
{
    int ret_val;
    int socket_fd;
    struct sockaddr_in srv_sockaddr;
    struct sockaddr_in cln_sockaddr = { 0 };
    char *udp_recv_buf;
    size_t length = UDP_RECV_BUF_SIZE;
    socklen_t fromlen = sizeof(struct sockaddr_in);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        printf("socket alloc error!\n");
        return;
    }

    udp_recv_buf = malloc(UDP_RECV_BUF_SIZE);
    if (!udp_recv_buf) {
        closesocket(socket_fd);
        printf("unicast buff malloc error\n");
        return;
    }

    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_len = sizeof(struct sockaddr_in);
    srv_sockaddr.sin_port = htons(UNICAST_SERVER_PORT);
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret_val = bind(socket_fd, (void *)&srv_sockaddr, sizeof(struct sockaddr_in));
    if (ret_val) {
        printf("boardcast bind error, maybe has be testing?\n");
        goto exit;
    }

    printf("UDP unicast test start, port : 0x%02x\n", UNICAST_SERVER_PORT);

    while (1) {
        length = recvfrom(socket_fd, udp_recv_buf, UDP_RECV_BUF_SIZE, 0, (void *)&cln_sockaddr,
                          &fromlen);
        if (length > 0) {
            printf("UDP recv data! From:\n");
            printf("IP: %s\nPort:  %d\n", inet_ntoa(cln_sockaddr.sin_addr),
                   ntohs(cln_sockaddr.sin_port));
            ret_val = sendto(socket_fd, udp_recv_buf, length, 0, (void *)&cln_sockaddr, fromlen);
            if (ret_val < 0) {
                printf("UDP send error!\n");
                goto exit;
            }
        } else {
            printf("recv error\n");
            goto exit;
        }
    }

exit:
    closesocket(socket_fd);
    free(udp_recv_buf);
}

void udp_test_muticast_thread(void *para)
{
    static const char muticase_ip[8][16] = {
        "239.26.1.2", "239.26.1.3", "239.26.1.4", "239.26.1.5",
        "239.26.1.6", "239.26.1.7", "239.26.1.8", "239.26.1.9",
    };
    static int ip_index = 0;

    int ret_val;
    int socket_fd;
    struct sockaddr_in srv_sockaddr;
    struct sockaddr_in cln_sockaddr = { 0 };

    size_t length = UDP_RECV_BUF_SIZE;
    socklen_t fromlen = sizeof(struct sockaddr_in);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        printf("muticast socket alloc error!\n");
        return;
    }

    char *udp_recv_buf;
    udp_recv_buf = malloc(UDP_RECV_BUF_SIZE);
    if (!udp_recv_buf) {
        closesocket(socket_fd);
        printf("unicast buff malloc error\n");
        return;
    }

    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(muticase_ip[0]);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

checkout_memship:
    ret_val = setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq));
    if (ret_val != ERR_OK) {
        printf("Join muticast group: %s error!\n", inet_ntoa(mreq.imr_multiaddr.s_addr));
        goto exit;
    }
    printf("Join muticast group: %s sucess!\n", inet_ntoa(mreq.imr_multiaddr.s_addr));

    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_len = sizeof(struct sockaddr_in);
    srv_sockaddr.sin_port = htons(MUTICAST_SERVER_PORT);
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret_val = bind(socket_fd, (void *)&srv_sockaddr, sizeof(struct sockaddr_in));
    if (ret_val) {
        printf("boardcast bind error, maybe has be testing?\n");
        goto exit;
    }

    printf("UDP muticast test start, port : 0x%02x\n", MUTICAST_SERVER_PORT);
    while (1) {
        length = recvfrom(socket_fd, udp_recv_buf, UDP_RECV_BUF_SIZE, 0, (void *)&cln_sockaddr,
                          &fromlen);
        if (length > 0) {
            printf("UDP recv data! From:\n");
            printf("IP: %s\nPort:  %d\n", inet_ntoa(cln_sockaddr.sin_addr),
                   ntohs(cln_sockaddr.sin_port));
            ret_val = sendto(socket_fd, udp_recv_buf, length, 0, (void *)&cln_sockaddr, fromlen);
            if (ret_val < 0) {
                printf("muticast UDP send error!\n");
                goto exit;
            }

            ret_val = setsockopt(socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
                                 sizeof(struct ip_mreq));
            if (ret_val != ERR_OK) {
                printf("Drop muticast group: %s error!\n", muticase_ip[ip_index]);

                goto exit;
            }
            printf("Drop muticast group: %s sucess!\n", muticase_ip[ip_index]);

            ip_index++;
            ip_index %= 7;
            mreq.imr_multiaddr.s_addr = inet_addr(muticase_ip[ip_index]);

            goto checkout_memship;
        } else {
            printf("muticast recv error\n");
            goto exit;
        }
    }

exit:
    closesocket(socket_fd);
    free(udp_recv_buf);
}

void udp_test_boardcast_thread(void *para)
{
    int ret_val;
    int socket_fd;
    struct sockaddr_in srv_sockaddr;
    struct sockaddr_in cln_sockaddr = { 0 };

    size_t length = UDP_RECV_BUF_SIZE;
    socklen_t fromlen = sizeof(struct sockaddr_in);

    int optval = 1;
    int ipaddr;
    struct sockaddr_in boardcast_sockaddr = { 0 };

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        printf("socket alloc error!\n");
        return;
    }

    ret_val = setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    if (ret_val != ERR_OK) {
        closesocket(socket_fd);
        printf("Socket broadcast function set error!\n");
        return;
    }

    char *udp_recv_buf;
    udp_recv_buf = malloc(UDP_RECV_BUF_SIZE);
    if (!udp_recv_buf) {
        closesocket(socket_fd);
        printf("unicast buff malloc error\n");
        return;
    }

    inet_aton("255.255.255.255", &ipaddr);
    boardcast_sockaddr.sin_family = AF_INET;
    boardcast_sockaddr.sin_addr.s_addr = ipaddr;
    boardcast_sockaddr.sin_len = sizeof(struct sockaddr_in);
    boardcast_sockaddr.sin_port = htons(8080);

    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_len = sizeof(struct sockaddr_in);
    srv_sockaddr.sin_port = htons(BOARDCAST_SERVER_PORT);
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret_val = bind(socket_fd, (void *)&srv_sockaddr, sizeof(struct sockaddr_in));
    if (ret_val) {
        printf("boardcast bind error, maybe has be testing?\n");
        goto exit;
    }

    printf("UDP boardcast test start,  src port : 0x%02x\n remote port : 8080",
           BOARDCAST_SERVER_PORT);

    while (1) {
        length = recvfrom(socket_fd, udp_recv_buf, UDP_RECV_BUF_SIZE, 0, (void *)&cln_sockaddr,
                          &fromlen);
        if (length > 0) {
            printf("UDP recv data! From:\n");
            printf("IP: %s\nPort:  %d\n", inet_ntoa(cln_sockaddr.sin_addr),
                   ntohs(cln_sockaddr.sin_port));
        } else {
            printf("recv error\n");
            goto exit;
        }

        ret_val = sendto(socket_fd, udp_recv_buf, length, 0, (void *)&boardcast_sockaddr,
                         sizeof(struct sockaddr_in));
        if (ret_val < 0) {

            printf("boardcast send error\n");
            goto exit;
        }
    }

exit:
    closesocket(socket_fd);
    free(udp_recv_buf);
}

int cmd_test_udp(int argc, char *argv[])
{
    static int init_flag = 0;

    if (init_flag) {
        printf("Failed: Only start once\n");
        return -1;
    }
    init_flag++;

    aicos_thread_create("udp unicast echo back", 4096, 22, udp_test_unicast_thread, NULL);
    aicos_thread_create("udp boardcast echo back", 4096, 22, udp_test_boardcast_thread, NULL);
    aicos_thread_create("udp muticast echo back", 4096, 22, udp_test_muticast_thread, NULL);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_udp, test_udp, udp unicast / muticast / boardcast ehco back);
