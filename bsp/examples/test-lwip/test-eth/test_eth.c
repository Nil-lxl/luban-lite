/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: lv.wu@artinchip.com
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <rtthread.h>
#include "sys/socket.h"
#include "aic_log.h"

#define SERVER_PORT    8080

#define UDP_BUF_SIZE    128

static char udp_recv_buf[UDP_BUF_SIZE];
static char udp_send_buf[UDP_BUF_SIZE];

static const char sopts[] = "d:n:ch";
static const struct option lopts[] = {
    {"delay",       optional_argument,  NULL, 'd'},
    {"number",      optional_argument,  NULL, 'n'},
    {"circle",      no_argument,        NULL, 'c'},
    {"help",        no_argument,        NULL, 'h'},
    {0, 0, 0, 0}
};

static void test_eth_loopback_usage(char *program)
{
    printf("Compile time: %s %s\n", __DATE__, __TIME__);
    printf("Usage: %s [options]\n", program);
    printf("\t -d, --delay\t\tDelay time\n");
    printf("\t -n, --number\t\tNumber of tests\n");
    printf("\t -c, --circle\t\tcircle test\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example:\n");
    printf("         %s -d 300 -n 10 \n", program);
}

static int udp_sock_set(int socket)
{
    int optval = 1;
    int ret_val;
    struct timeval time;

    ret_val = setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    if (ret_val != ERR_OK) {
        pr_err("Socket broadcast function set error!\n");
    }

    time.tv_sec = 1;
    time.tv_usec = 0;
    ret_val = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(struct timeval));
    if (ret_val != ERR_OK) {
        pr_err("Socket send timeout error!\n");
    }

    ret_val = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(struct timeval));
    if (ret_val != ERR_OK) {
        pr_err("Socket recv timeout error!\n");
    }

    return ret_val;
}

int cmd_test_eth(int argc, char **argv)
{
    int ret_val;
    int socket;
    struct sockaddr_in srv_sockaddr;
    struct sockaddr_in cln_sockaddr = {0};
    socklen_t fromlen = sizeof(struct sockaddr_in);
    int i, c;
    int max_count = 4, count = 0;
    int delay_time = 1000;
    int circle = 0;

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 'd':
            delay_time = atoi(optarg);
            if (delay_time <= 0) {
                test_eth_loopback_usage(argv[0]);
                return 0;
            }
            break;
        case 'n':
            max_count = atoi(optarg);
            if (max_count <= 0) {
                test_eth_loopback_usage(argv[0]);
                return 0;
            }
            break;
        case 'c':
            circle = 1;
            break;
        case 'h':
        default:
            test_eth_loopback_usage(argv[0]);
            return 0;
        }
    }

    socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket < 0) {
        pr_err("socket alloc error!\n");
        return -1;
    }

    ret_val = udp_sock_set(socket);
    if (ret_val < 0) {
        pr_err("socket set error\n");
        goto error;
    }

    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_len = sizeof(struct sockaddr_in);
    srv_sockaddr.sin_port = htons(SERVER_PORT);
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret_val = bind(socket, (void *)&srv_sockaddr, sizeof(struct sockaddr_in));
    if (ret_val != ERR_OK) {
        pr_err("Bind port: %d error!\n", SERVER_PORT);
        goto error;
    }

    int ipaddr;
    inet_aton("255.255.255.255", &ipaddr);
    cln_sockaddr.sin_family = AF_INET;
    cln_sockaddr.sin_addr.s_addr = ipaddr;
    cln_sockaddr.sin_len = sizeof(struct sockaddr_in);
    cln_sockaddr.sin_port = htons(SERVER_PORT);

    for (i = 0; i < UDP_BUF_SIZE; i++)
        udp_send_buf[i] = i;

    while(1) {
        ret_val = sendto(socket, udp_send_buf, UDP_BUF_SIZE, 0, (void *)&cln_sockaddr, sizeof(struct sockaddr_in));
        if (ret_val < 0) {
            pr_err("UDP send error!\n");
            goto error;
        }

        memset(udp_recv_buf, 0, sizeof(udp_recv_buf));
        ret_val = recvfrom(socket, udp_recv_buf, sizeof(udp_recv_buf), 0, (void *)&srv_sockaddr, &fromlen);
        if (ret_val <= 0) {
            pr_err("UDP recv error!\n");
            goto error;
        }

        if (memcmp(udp_recv_buf, udp_send_buf, sizeof(udp_recv_buf)) != 0) {
            pr_err("UDP recv wrong masg\n");
            goto error;
        }

        pr_info("Ethernet circle test round %d ok\n", count+1);
        if (++count == max_count && circle == 0) {
            pr_info("Ethernet circle test OK, totol %d\n", max_count);
            break;
        }

        rt_thread_mdelay(delay_time);
    };

    closesocket(socket);

    if (max_count == count)
        return 0;
    return -1;

error:
    closesocket(socket);
    return -1;
}

MSH_CMD_EXPORT_ALIAS(cmd_test_eth, test_eth, network loopback testing);
