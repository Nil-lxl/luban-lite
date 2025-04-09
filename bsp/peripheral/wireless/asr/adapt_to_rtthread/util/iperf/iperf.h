
#ifndef _IPERF_H_
#define _IPERF_H_

#if 1 //PRJCONF_NET_EN

#include "asr_rtos_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPERF_OPT_BANDWIDTH        1    /* -b, bandwidth to send at in bits/sec */
#define IPERF_OPT_NUM            1    /* -n, number of bytes to transmit (instead of -t) */
#define IPERF_OPT_TOS            1    /* -S, the type-of-service for outgoing packets */

#define MAX_INTERVAL 60
#define IPERF_ARG_HANDLE_MAX    4

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN         16
#endif

#if (!defined(__CONFIG_LWIP_V1) && LWIP_IPV6)
#define IPERF_ADDR_STRLEN_MAX   INET6_ADDRSTRLEN
#else
#define IPERF_ADDR_STRLEN_MAX   INET_ADDRSTRLEN
#endif

enum IPERF_MODE {
    IPERF_MODE_UDP_SEND = 0,
    IPERF_MODE_UDP_RECV,
    IPERF_MODE_TCP_SEND,
    IPERF_MODE_TCP_RECV,
    IPERF_MODE_NUM,
};

enum IPERF_FLAGS {
    IPERF_FLAG_PORT    = 0x00000001,
    IPERF_FLAG_IP      = 0x00000002,
    IPERF_FLAG_CLINET  = 0x00000004,
    IPERF_FLAG_SERVER  = 0x00000008,
    IPERF_FLAG_UDP     = 0x00000010,
    IPERF_FLAG_FORMAT  = 0x00000020,
    IPERF_FLAG_STOP    = 0x00000040,
};

typedef struct {
    enum IPERF_MODE    mode;
    char        remote_ip[IPERF_ADDR_STRLEN_MAX];
    uint32_t    port;
    uint32_t    run_time; // in seconds, 0 means forever
    uint32_t    interval; // in seconds, 0 means 1 second(default)
#if IPERF_OPT_TOS
    uint8_t        tos; // type-of-service
#endif
#if IPERF_OPT_NUM
    uint8_t        mode_time; // 1: time limited, 0: byte limited
    uint64_t    amount; // in bytes (K == 1024, M == 1024 * 1024)
#endif
#if IPERF_OPT_BANDWIDTH
    uint32_t    bandwidth; // in bits/sec (k == 1000, m == 1000 * 1000)
#endif
    uint32_t    flags;
    asr_thread_hand_t iperf_thread;
    int         handle;
    //uint32_t    send_fail_count;
    char        host_ip[IPERF_ADDR_STRLEN_MAX];
}iperf_arg;

typedef struct UDP_datagram {
    signed int id ;
} UDP_datagram;


int iperf_handle_new(iperf_arg* arg);
int iperf_start(void *nif, int handle);
int iperf_stop(char* arg);
int iperf_parse_argv(int argc, char *argv[]);
int iperf_handle_free(int handle);
int iperf_handle_start(void * nif, int handle);

#ifdef __cplusplus
}
#endif

#endif /* PRJCONF_NET_EN */
#endif /* _IPERF_H_ */
