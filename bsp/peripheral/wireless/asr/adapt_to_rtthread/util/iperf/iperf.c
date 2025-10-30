#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "lwip/netif.h"
#include "lwip/sockets.h"

#include "asr_rtos_api.h"
#include "asr_dbg.h"
#include "iperf.h"
#include "iperf_debug.h"
#include "tasks_info.h"


#define inet_pton(af,src,dst) \
    (((af) == AF_INET) ? ip4addr_aton((src),(ip4_addr_t*)(dst)) : 0)
#define inet_ntop(af,src,dst,size) \
    (((af) == AF_INET) ? ip4addr_ntoa_r((const ip4_addr_t*)(src),(dst),(size)) : NULL)
#define inet_aton(x, y)    inet_pton(AF_INET, x, y)

#define malloc(x)    asr_rtos_malloc(x)
#define free(x)        asr_rtos_free(x)


//#define    INADDR_NONE        ((uint32_t) 0xffffffff)

#define iperf_thread_exit(thread)     asr_rtos_delete_thread(thread)
#define iperf_errno                   asr_rtos_GetError()  /* errno */
#define iperf_msleep(msec)            asr_rtos_delay_milliseconds(msec)

#define IPERF_PORT                    5001

#define IPERF_BUF_SIZE                 (1550)
#define IPERF_UDP_SEND_DATA_LEN        (1470)    // UDP: 1470 + 8  + 20 = 1498
#define IPERF_TCP_SEND_DATA_LEN        (1460)    // TCP: 1460 + 20 + 20 = 1500

iperf_arg *g_iperf_arg_handle[IPERF_ARG_HANDLE_MAX] = {NULL};

uint8_t g_iperf_task_not_running[IPERF_ARG_HANDLE_MAX] = {0};

// in 1ms
#define IPERF_TIME_PER_SEC           get_rt_tick_per_second()   // 1000
#define IPERF_TIME()               asr_rtos_get_ticks()
#define IPERF_SEC_2_INTERVAL(sec)  ((sec) * IPERF_TIME_PER_SEC)

#define IPERF_SELECT_TIMEOUT        100

static void iperf_speed_log(iperf_arg *arg, uint64_t bytes, uint32_t time,
                            int8_t is_end)
{
    uint64_t speed;
    uint32_t integer_part, decimal_part;
    char str[16];

    if (arg->flags & IPERF_FLAG_FORMAT) {
        /* KBytes/sec */
        speed = bytes * IPERF_TIME_PER_SEC * 100 / 1024 / time;
    } else {
        /* Mbits/sec */
        speed = bytes * 8 * IPERF_TIME_PER_SEC * 100 / 1000 / 1000 / time;
    }

    integer_part = speed / 100;
    decimal_part = speed % 100;

    if (integer_part >= 100) {
        snprintf(str, sizeof(str), "%u", integer_part);
    } else if (integer_part >= 10) {
        snprintf(str, sizeof(str), "%u.%u", integer_part, decimal_part / 10);
    } else {
        snprintf(str, sizeof(str), "%u.%02u", integer_part, decimal_part);
    }

    IPERF_LOG(1, "[%d] %s%s %s\n", arg->handle, is_end ?  "TEST END: " : "",
              str, (arg->flags & IPERF_FLAG_FORMAT) ? "KB/s" : "Mb/s");
}


static __inline void iperf_udp_seq_set(uint32_t *p, uint32_t seq)
{
    *p = lwip_htonl(seq);
}

static int iperf_sock_create(int sock_type, short local_port, int do_bind, iperf_arg *idata)
{
    int local_sock;
    struct sockaddr_in local_addr;
    int ret, tmp;

    local_sock = lwip_socket(AF_INET, sock_type, 0);
    if (local_sock < 0) {
        IPERF_ERR("socket() return %d\n", local_sock);
        return local_sock;
    }

    if (!do_bind)
        return local_sock;

    tmp = 1;
    ret = lwip_setsockopt(local_sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
    if (ret != 0) {
        IPERF_ERR("setsockopt(SO_REUSEADDR) failed, err %d\n", iperf_errno);
        lwip_close(local_sock);
        return -1;
    }

    if (sock_type == SOCK_STREAM) {
        tmp = 1;
        // ret = setsockopt(local_sock, SOL_SOCKET, SO_KEEPALIVE, &tmp, sizeof(int));
        // if (ret != 0) {
        //     IPERF_ERR("setsockopt(SO_KEEPALIVE) failed, err %d\n", iperf_errno);
        //     close(local_sock);
        //     return -1;
        // }
    }

    /* bind socket to port */
    memset(&local_addr, 0, sizeof(struct sockaddr_in));
    if (inet_aton(idata->host_ip, &local_addr.sin_addr.s_addr) == 0) {
        local_addr.sin_addr.s_addr = lwip_htonl(INADDR_ANY);
    }
    local_addr.sin_port = lwip_htons(local_port);
    local_addr.sin_family = AF_INET;
    ret = lwip_bind(local_sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        IPERF_ERR("Failed to bind socket %d, err %d\n", local_sock, iperf_errno);
        lwip_close(local_sock);
        return -1;
    }
    return local_sock;
}

static int iperf_set_sock_opt(int sock, iperf_arg *idata)
{
    int ret;

#if IPERF_OPT_TOS
    /* set IP TOS (type-of-service) field */
    if (idata->tos > 0) {
        int tos = idata->tos;
         ret = lwip_setsockopt(sock, IPPROTO_IP, IP_TOS, (void *)&tos, sizeof(tos));
        if (ret != 0) {
            IPERF_ERR("setsockopt(IP_TOS) failed, err %d\n", iperf_errno);
            return -1;
        }
    }
#endif
    return 0;
}

static uint8_t *iperf_buf_new(uint32_t size)
{
    uint32_t i;
    uint8_t *data_buf = malloc(size);
    if (data_buf == NULL) {
        IPERF_ERR("malloc() failed!\n");
    } else {
        for (i = 0; i < size; ++i) {
            data_buf[i] = '0' + i % 10;
        }
    }
    return data_buf;
}

#define iperf_loop_init()                                                     \
    data_total_cnt = 0;                                                        \
    data_cnt = 0;                                                            \
    beg_tm = IPERF_TIME();                                                    \
    end_tm = beg_tm + IPERF_SEC_2_INTERVAL(((iperf_arg *)arg)->interval);    \
    run_beg_tm = beg_tm;                                                    \
    run_end_tm = run_beg_tm + run_time;

#define iperf_calc_speed()                                                    \
    data_total_cnt += data_len;                                             \
    cur_tm = IPERF_TIME();                                                    \
    if (cur_tm > end_tm) {                                                    \
        iperf_speed_log((iperf_arg *)arg, data_cnt, cur_tm - beg_tm, 0);                 \
        data_cnt = 0;                                                        \
        beg_tm = IPERF_TIME();                                               \
        end_tm = beg_tm + IPERF_SEC_2_INTERVAL(((iperf_arg *)arg)->interval);\
    }                                                                        \
    if (idata->mode_time) {                                                    \
        if (run_time && cur_tm > run_end_tm) {                                \
            idata->flags |= IPERF_FLAG_STOP;                                \
        }                                                                    \
    } else {                                                                \
        if (idata->amount > data_len) {                                        \
            idata->amount -= data_len;                                        \
        } else {                                                            \
            idata->amount = 0;                                                \
            idata->flags |= IPERF_FLAG_STOP;                                \
        }                                                                    \
    }                                                                        \
    if (idata->flags & IPERF_FLAG_STOP) {                                    \
        iperf_speed_log((iperf_arg *)arg, data_total_cnt, cur_tm - run_beg_tm, 1);       \
        break;                                                                \
    }

#define iperf_calc_speed_fin()                                              \
    cur_tm = IPERF_TIME();                                                  \
    data_total_cnt += data_len;                                             \
    iperf_speed_log((iperf_arg *)arg, data_total_cnt, cur_tm - run_beg_tm, 1);           \

/* -------------------------------------------------------------------
 * Send a datagram on the socket. The datagram's contents should signify
 * a FIN to the application. Keep re-transmitting until an
 * acknowledgement datagram is received.
 * ------------------------------------------------------------------- */

void write_UDP_FIN(int local_sock,
                        struct sockaddr *remote_addr,
                        uint8_t *data_buf)
{
    int rc;
    fd_set readSet;
    struct timeval timeout;
    struct UDP_datagram* mBuf_UDP = (struct UDP_datagram*) data_buf;

    int count = 0;
    int packetid;
    while (count < 10) {
         count++;

         // write data
         lwip_sendto(local_sock, data_buf, IPERF_UDP_SEND_DATA_LEN, 0,
                          remote_addr, sizeof(*remote_addr));
        // decrement the packet count
        packetid = lwip_ntohl(mBuf_UDP->id);
        packetid--;
        mBuf_UDP->id = lwip_htonl(packetid);

        // wait until the socket is readable, or our timeout expires
        FD_ZERO(&readSet);
        FD_SET(local_sock, &readSet);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 250000; // quarter second, 250 ms

        rc = lwip_select(local_sock + 1, &readSet, NULL, NULL, &timeout);
        if (rc == 0) {
         // select timed out
         continue;
        } else {
            // socket ready to read
            rc = lwip_recvfrom(local_sock, data_buf, IPERF_BUF_SIZE, 0, NULL, 0);
            if (rc < 0)
                break;
            return;
        }
    }

    IPERF_WARN("did not receive ack of last datagram after %d tries.\n", count);
}

/* -------------------------------------------------------------------
 * Send an AckFIN (a datagram acknowledging a FIN) on the socket,
 * then select on the socket for some time. If additional datagrams
 * come in, probably our AckFIN was lost and they are re-transmitted
 * termination datagrams, so re-transmit our AckFIN.
 * ------------------------------------------------------------------- */

void write_UDP_AckFIN(int local_sock,
                            struct sockaddr *remote_addr,
                            uint8_t *data_buf) {
    int rc;
    fd_set readSet;
    FD_ZERO(&readSet);
    struct timeval timeout;
    int count = 0;

    while ( count < 10 ) {
        count++;

        // write data
        lwip_sendto(local_sock, data_buf, IPERF_UDP_SEND_DATA_LEN, 0,
                          remote_addr, sizeof(*remote_addr));

        // wait until the socket is readable, or our timeout expires
        FD_SET(local_sock, &readSet);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        rc = lwip_select(local_sock + 1, &readSet, NULL, NULL, &timeout);
        if ( rc == 0 ) {
            // select timed out
            return;
        } else {
            // socket ready to read
            rc = lwip_recvfrom(local_sock, data_buf, IPERF_BUF_SIZE, 0, NULL, 0);
            if ( rc <= 0 ) {
                // Connection closed or errored
                // Stop using it.
                return;
            }
        }
    }

    IPERF_WARN("ack of last datagram failed after %d tries.\n", count);
}

void iperf_udp_send_task(asr_thread_arg_t arg)
{
    int local_sock = -1;
    struct sockaddr_in remote_addr;
    iperf_arg *idata = (iperf_arg *)arg;
    uint32_t run_time = IPERF_SEC_2_INTERVAL(idata->run_time);
    uint32_t port = idata->port;
    uint8_t *data_buf = NULL;
    int32_t data_len;
    int packetID = 0;
    struct UDP_datagram* mBuf_UDP;
    int handle = 0;

    local_sock = iperf_sock_create(SOCK_DGRAM, 0, 1, idata);
    if (local_sock < 0) {
        IPERF_ERR("socket() return %d\n", local_sock);
        goto socket_error;
    }
    iperf_set_sock_opt(local_sock, idata);

    data_buf = iperf_buf_new(IPERF_BUF_SIZE);
    if (data_buf == NULL) {
        IPERF_ERR("malloc() failed!\n");
        goto socket_error;
    }

    memset(&remote_addr, 0, sizeof(struct sockaddr_in));
    inet_aton(idata->remote_ip, &remote_addr.sin_addr);
    remote_addr.sin_port = lwip_htons(port);
    remote_addr.sin_family = AF_INET;

    IPERF_DBG("iperf: UDP send to %s:%d\n", idata->remote_ip, port);

    uint64_t run_end_tm = 0, run_beg_tm = 0, beg_tm = 0, end_tm = 0, cur_tm = 0;
    uint64_t data_total_cnt = 0;
    uint32_t data_cnt = 0;
#if IPERF_OPT_BANDWIDTH
    uint32_t send_tm, run_tm; /* relative to run_beg_tm */
#endif

    mBuf_UDP = (struct UDP_datagram*) data_buf;
    mBuf_UDP->id = lwip_htonl(packetID);
    iperf_loop_init();

    while (!(idata->flags & IPERF_FLAG_STOP)) {
#if IPERF_OPT_BANDWIDTH
        if (idata->bandwidth != 0) {
            run_tm = IPERF_TIME() - run_beg_tm;
            //send_tm = data_total_cnt * 8 / (idata->bandwidth / 1000);
            send_tm = data_total_cnt * 8 / (idata->bandwidth / IPERF_TIME_PER_SEC);
            if (send_tm > run_tm) {
                iperf_msleep(send_tm - run_tm);
            }
        }
#endif
        data_len = lwip_sendto(local_sock, data_buf, IPERF_UDP_SEND_DATA_LEN, 0,
                          (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        if (data_len > 0) {
            data_cnt += data_len;
            packetID++;
            mBuf_UDP->id = lwip_htonl(packetID);
        } else {
            data_len = 0;
        }
        iperf_calc_speed();
    }

    mBuf_UDP->id = lwip_htonl(-packetID);
    write_UDP_FIN(local_sock, (struct sockaddr *)&remote_addr, data_buf);

socket_error:
    if (data_buf)
        free(data_buf);
    if (local_sock >= 0)
        lwip_close(local_sock);

    asr_thread_hand_t thread = idata->iperf_thread;
    handle = idata->handle;
    iperf_handle_free(idata->handle);
    if ((handle >= 0) && (handle < IPERF_ARG_HANDLE_MAX))
    {
        g_iperf_task_not_running[handle] = 1;
    }
    IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
    iperf_thread_exit(&thread);
}

void iperf_udp_recv_task(asr_thread_arg_t arg)
{
    int local_sock = -1;
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    iperf_arg *idata = (iperf_arg *)arg;
    uint32_t run_time = IPERF_SEC_2_INTERVAL(idata->run_time);
    uint32_t port = idata->port;
    uint8_t *data_buf = NULL;
    int32_t data_len;

    int32_t   packet_count = 0;
    int32_t   cnt_error = 0;

    int packetID = 0;
    struct UDP_datagram* mBuf_UDP;
    uint8_t wait = 1;
    int handle = 0;
    #if 0
    int timeout = IPERF_SELECT_TIMEOUT; //ms
    #else
    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = 100000,
    };
    #endif

    if (port == 0) {
        port = IPERF_PORT;
    }
    local_sock = iperf_sock_create(SOCK_DGRAM, port, 1, idata);
    if (local_sock < 0) {
        IPERF_ERR("socket() return %d\n", local_sock);
        goto socket_error;
    }
    iperf_set_sock_opt(local_sock, idata);

    data_buf = iperf_buf_new(IPERF_BUF_SIZE);
    if (data_buf == NULL) {
        IPERF_ERR("malloc() failed!\n");
        goto socket_error;
    }

    IPERF_DBG("iperf: UDP recv at port %d\n", port);

    uint64_t run_end_tm = 0, run_beg_tm = 0, beg_tm = 0, end_tm = 0, cur_tm = 0;
    uint64_t data_total_cnt = 0;
    uint32_t data_cnt = 0;
#if IPERF_OPT_NUM
    idata->mode_time = 1;
#endif

    int error_socket = 0;
    if ((error_socket = lwip_setsockopt(local_sock, SOL_SOCKET, SO_RCVTIMEO,
                    &timeout, sizeof(timeout))) < 0) {
        IPERF_ERR("set socket option err %d\n", error_socket);
        goto socket_error;
    }

    mBuf_UDP = (struct UDP_datagram*)data_buf;
    iperf_loop_init();

    while (!(idata->flags & IPERF_FLAG_STOP)) {
        data_len = lwip_recvfrom(local_sock, data_buf, IPERF_BUF_SIZE, 0,
                            (struct sockaddr *)&remote_addr, &addr_len);
        if (data_len > 0) {
            if (wait) {
                wait = 0;
                iperf_loop_init(); // reinitialization time
                IPERF_DBG("iperf udp_recv_task reinit time and reclocking\n");
            }
            data_cnt += data_len;
        } else {
            data_len = 0;
            continue;
        }

        packetID = lwip_ntohl(mBuf_UDP->id);
        if (packetID < 0) {
            iperf_calc_speed_fin();
            IPERF_LOG(1, "Lost/Total Datagrams: %d/%d\n", cnt_error, packet_count);
            IPERF_DBG("iperf udp_recv_task receive a FIN datagram\n");
            write_UDP_AckFIN(local_sock, (struct sockaddr *)&remote_addr,
                                data_buf);
            break;
        }

        iperf_calc_speed();

        if (packetID >= packet_count + 1) {
            /* likely wrong packet, ignore */
            if (data_len < IPERF_UDP_SEND_DATA_LEN) {
                continue;
            }
            /* Forward, but is there a gap in sequence numbers? */
            if (packetID > packet_count + 1) {
            /* There's a gap so count that as a loss. */
                cnt_error += (packetID - 1) - packet_count;
            }
            /* Update the highest sequence number seen so far. */
            packet_count = packetID;
        } else {
            if (cnt_error > 0)
                cnt_error--;
        }
    }

socket_error:
    if (data_buf)
        free(data_buf);
    if (local_sock >= 0)
        lwip_close(local_sock);

    asr_thread_hand_t thread = idata->iperf_thread;
    handle = idata->handle;
    iperf_handle_free(idata->handle);
    if ((handle >= 0) && (handle < IPERF_ARG_HANDLE_MAX))
    {
        g_iperf_task_not_running[handle] = 1;
    }
    IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
    iperf_thread_exit(&thread);
}

void iperf_tcp_send_task(asr_thread_arg_t arg)
{
    int local_sock = -1;
    struct sockaddr_in remote_addr;
    iperf_arg *idata = (iperf_arg *)arg;
    uint32_t run_time = IPERF_SEC_2_INTERVAL(idata->run_time);
    uint32_t port = idata->port;
    uint8_t *data_buf = NULL;
    int32_t data_len;
    int ret;
    int handle = 0;

    fd_set fdw;
    struct timeval tv;

    struct timeval send_timeout = {2, 0};

//    asr_rtos_delay_milliseconds(1); // enable task create function return
    local_sock = iperf_sock_create(SOCK_STREAM, 0, 1, idata);
    if (local_sock < 0) {
        IPERF_ERR("socket() return %d\n", local_sock);
        goto socket_error;
    }
    iperf_set_sock_opt(local_sock, idata);

    setsockopt(local_sock, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    data_buf = iperf_buf_new(IPERF_BUF_SIZE);
    if (data_buf == NULL) {
        IPERF_ERR("malloc() failed!\n");
        goto socket_error;
    }

    //uint32_t i = 1;
    // ret = ioctlsocket(local_sock, FIONBIO, &i);
    // if (ret < 0) {
    //     IPERF_ERR("ioctlsocket() return %d\n", ret);
    //     goto socket_error;
    // }

    memset(&remote_addr, 0, sizeof(struct sockaddr_in));
    inet_aton(idata->remote_ip, &remote_addr.sin_addr);
    remote_addr.sin_port = lwip_htons(port);
    remote_addr.sin_family = AF_INET;

    ret = lwip_connect(local_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (ret != 0) {
        /*if (errno != EINPROGRESS) {
            IPERF_ERR("connect() return %d\n", ret);
            goto socket_error;
        } else {*/
            while (!(idata->flags & IPERF_FLAG_STOP)) {

                FD_ZERO(&fdw);
                FD_SET(local_sock, &fdw);
                tv.tv_sec = 0;
                tv.tv_usec = 100000; //100ms

                ret = lwip_select(local_sock + 1, NULL, &fdw, NULL, &tv);
                if (ret > 0) {
                    if (FD_ISSET(local_sock, &fdw)) {
                        break;
                    }
                } else if (ret < 0) {
                    IPERF_ERR("socket select err! errno:%d\n", iperf_errno);
                    goto socket_error;
                } else if (ret == 0) {
                    continue;
                }
            }
            if (idata->flags & IPERF_FLAG_STOP)
                goto socket_error;
        //}
    }

    IPERF_DBG("iperf: TCP send to %s:%d\n", idata->remote_ip, port);

    uint64_t run_end_tm = 0, run_beg_tm = 0, beg_tm = 0, end_tm = 0, cur_tm = 0;
    uint64_t data_total_cnt = 0;
    uint32_t data_cnt = 0;
    uint32_t select_ret0_cnt = 0;

    iperf_loop_init();

    while (!(idata->flags & IPERF_FLAG_STOP)) {
        FD_ZERO(&fdw);
        FD_SET(local_sock, &fdw);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; //100ms

        ret = lwip_select(local_sock + 1, NULL, &fdw, NULL, &tv);
        if (ret > 0) {
            select_ret0_cnt = 0;
            if (FD_ISSET(local_sock, &fdw)) {
                data_len = lwip_send(local_sock, data_buf, IPERF_TCP_SEND_DATA_LEN, 0);
                if (data_len > 0) {
                    data_cnt += data_len;
                } else {
                    IPERF_WARN("send return %d, err %d\n", data_len, iperf_errno);
                    break;
                }
                iperf_calc_speed();
            }
        } else if (ret < 0) {
            IPERF_ERR("socket select err! errno:%d\n", iperf_errno);
            goto socket_error;
        } else if (ret == 0) {
            data_len = 0;
            iperf_calc_speed();
            select_ret0_cnt++;
        if (select_ret0_cnt > 100) {
                IPERF_DBG("%s() tcp select ret is [%d] \n", __func__, ret);
                select_ret0_cnt = 0;
                iperf_msleep(500);
        }
            continue;
        }
    }

socket_error:
    if (data_buf)
        free(data_buf);
    if (local_sock >= 0)
        lwip_close(local_sock);

    asr_thread_hand_t thread = idata->iperf_thread;
    handle = idata->handle;
    iperf_handle_free(idata->handle);
    if ((handle >= 0) && (handle < IPERF_ARG_HANDLE_MAX))
    {
        g_iperf_task_not_running[handle] = 1;
    }
    IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
    iperf_thread_exit(&thread);
}

void iperf_tcp_recv_task(asr_thread_arg_t arg)
{
    int local_sock = -1;
    int remote_sock = -1;
    struct sockaddr_in remote_addr;
    iperf_arg *idata = (iperf_arg *)arg;
    uint32_t port = idata->port;
    uint32_t run_time = IPERF_SEC_2_INTERVAL(idata->run_time);
    uint8_t *data_buf = NULL;
    int32_t data_len;
    int ret;
    int handle = 0;

    #if 0
    int timeout = IPERF_SELECT_TIMEOUT; //ms
    #else
    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = 100000,
    };
    #endif

    local_sock = iperf_sock_create(SOCK_STREAM, port, 1, idata);
    if (local_sock < 0) {
        IPERF_ERR("socket() return %d\n", local_sock);
        goto socket_error;
    }
    iperf_set_sock_opt(local_sock, idata);

    data_buf = iperf_buf_new(IPERF_BUF_SIZE);
    if (data_buf == NULL) {
        IPERF_ERR("malloc() failed!\n");
        goto socket_error;
    }

    IPERF_DBG("iperf: TCP listen at port %d\n", port);

    ret = lwip_listen(local_sock, 5);
    if (ret < 0) {
        IPERF_ERR("Failed to listen socket %d, err %d\n", local_sock, iperf_errno);
        goto socket_error;
    }

    ret = sizeof(struct sockaddr_in);

    /* Set the recv timeout to set the accept timeout */
    if (lwip_setsockopt(local_sock, SOL_SOCKET, SO_RCVTIMEO,
                    &timeout, sizeof(timeout)) < 0) {
        IPERF_ERR("set socket option err %d\n", iperf_errno);
        goto socket_error;
    }

    while (!(idata->flags & IPERF_FLAG_STOP)) {
        remote_sock = lwip_accept(local_sock, (struct sockaddr *)&remote_addr,
                            (socklen_t *)&ret);
        if (remote_sock >= 0) {
            iperf_set_sock_opt(remote_sock, idata);
            break;
        }
    }

    if (!(idata->flags & IPERF_FLAG_STOP)) {
        char addr_str[32];
        inet_ntop(AF_INET, &remote_addr.sin_addr,
              addr_str, sizeof(addr_str));
        IPERF_DBG("iperf: client from %s:%d\n", addr_str,
                    lwip_ntohs(remote_addr.sin_port));
    }

    uint64_t run_end_tm = 0, run_beg_tm = 0, beg_tm = 0, end_tm = 0, cur_tm = 0;
    uint64_t data_total_cnt = 0;
    uint32_t data_cnt = 0;
#if IPERF_OPT_NUM
    idata->mode_time = 1;
#endif

    iperf_loop_init();

    while (!(idata->flags & IPERF_FLAG_STOP)) {
        data_len = lwip_recv(remote_sock, data_buf, IPERF_BUF_SIZE, 0);
        if (data_len > 0) {
            data_cnt += data_len;
        } else {
            iperf_calc_speed_fin();
            IPERF_WARN("recv return %d, err %d\n", data_len, iperf_errno);
            break;
        }
        iperf_calc_speed();
    }

socket_error:
    if (remote_sock)
        lwip_close(remote_sock);
    if (data_buf)
        free(data_buf);
    if (local_sock >= 0)
        lwip_close(local_sock);

    asr_thread_hand_t thread = idata->iperf_thread;
    handle = idata->handle;
    iperf_handle_free(idata->handle);
    if ((handle >= 0) && (handle < IPERF_ARG_HANDLE_MAX))
    {
        g_iperf_task_not_running[handle] = 1;
    }
    IPERF_DBG("%s() [%d] exit!\n", __func__, idata->handle);
    iperf_thread_exit(&thread);
}

static const asr_thread_function_t iperf_thread_entry[IPERF_MODE_NUM] = {
    iperf_udp_send_task,
    iperf_udp_recv_task,
    iperf_tcp_send_task,
    iperf_tcp_recv_task,
}; /* index by enum IPERF_MODE */

int iperf_handle_start(void *nif, int handle)
{
    asr_task_config_t cfg;

    if (handle < 0 || handle >= IPERF_ARG_HANDLE_MAX)
        return -1;

    if (asr_rtos_is_thread_valid(&(g_iperf_arg_handle[handle]->iperf_thread))) {
        IPERF_ERR("iperf task is running\n");
        return -1;
    }

    if (kNoErr != asr_rtos_task_cfg_get(ASR_TASK_CONFIG_IPERF, &cfg)) {
        IPERF_ERR("get iperf task information fail\n");
        return -1;
    }

    g_iperf_arg_handle[handle]->flags &= ~IPERF_FLAG_STOP; /* clean stop flag */
    g_iperf_task_not_running[handle] = 0;
    // priority need note.
    if (asr_rtos_create_thread(&(g_iperf_arg_handle[handle]->iperf_thread),
                            cfg.task_priority,
                            IPERF_TASK_NAME,
                            iperf_thread_entry[g_iperf_arg_handle[handle]->mode],
                            cfg.stack_size,
                            (void *)g_iperf_arg_handle[handle]
                            ) != 0) {
            IPERF_ERR("iperf task create failed\n");
            return -1;
    }
    return 0;
}

int iperf_handle_stop(int handle)
{
    int i = 0;
    asr_thread_hand_t iperf_thread;

    if (handle < 0)
        return -1;

    if ((handle < IPERF_ARG_HANDLE_MAX) && (g_iperf_arg_handle[handle] != NULL)) {
        if (asr_rtos_is_thread_valid(&(g_iperf_arg_handle[handle]->iperf_thread))) {
            iperf_thread = g_iperf_arg_handle[handle]->iperf_thread;
            g_iperf_arg_handle[handle]->flags |= IPERF_FLAG_STOP;
            while (g_iperf_task_not_running[handle] != 1)
            {
                asr_rtos_delay_milliseconds(110);
            }
            //iperf_thread_exit(&iperf_thread);
        } else {
            IPERF_ERR("iperf task %d not exist.\n", handle);
            return -1;
        }
    } else if (handle >= IPERF_ARG_HANDLE_MAX) {
        for (i = 0; i < IPERF_ARG_HANDLE_MAX; i++) {
            if (g_iperf_arg_handle[i] != NULL) {
                iperf_handle_stop(i);
            }
        }
    }
    return 0;
}

int iperf_start(void *nif, int handle)
{
    return iperf_handle_start(nif, handle);
}

int iperf_stop(char* arg)
{
    int handle = 0;
    if (strcmp("a", arg) == 0)
        return iperf_handle_stop(IPERF_ARG_HANDLE_MAX);
    handle = atoi(arg);
    if (handle < 0 || handle >= IPERF_ARG_HANDLE_MAX ||
        (handle == 0 && strcmp("0", arg))) {
        IPERF_DBG("handle '%s' input err\n", arg);
        return -1;
    }
    return iperf_handle_stop(handle);
}

int iperf_handle_new(iperf_arg* arg)
{
    int i = 0, pos = 0, find = 0;
    if (arg == NULL)
        return -1;

    for (i = 0; i < IPERF_ARG_HANDLE_MAX; i++) {
        if (g_iperf_arg_handle[i] == NULL) {
            if (!find) {
                find = 1;
                pos = i;
                if ((arg->mode & IPERF_MODE_TCP_SEND) ||
                    (arg->mode & IPERF_MODE_UDP_SEND))
                    break;
            }
        } else {
            if ((arg->mode & IPERF_MODE_TCP_RECV) ||
                (arg->mode & IPERF_MODE_UDP_RECV)) {
                if (strcmp(g_iperf_arg_handle[i]->host_ip, arg->host_ip) == 0)
                {
                    if(g_iperf_arg_handle[i]->port == arg->port) {
                        IPERF_DBG("the port %d has been used\n", arg->port);
                        return -1;
                    }
                }
            }
        }
    }

    if (find) {
        g_iperf_arg_handle[pos] = malloc(sizeof(iperf_arg));
        if (g_iperf_arg_handle[pos] == NULL) {
            IPERF_ERR("iperf malloc faild\n");
            return -1;
        }
        memcpy(g_iperf_arg_handle[pos], arg, sizeof(iperf_arg));
        g_iperf_arg_handle[pos]->handle = pos;
        return pos;
    }

    IPERF_DBG("cannot new more handle, buf is full\n");
    return -1;
}

int iperf_handle_free(int handle)
{
    if (handle >= IPERF_ARG_HANDLE_MAX)
        return -1;

    if (g_iperf_arg_handle[handle] != NULL) {
        free(g_iperf_arg_handle[handle]);
        g_iperf_arg_handle[handle] = NULL;
    }
    return 0;
}

static const char *iperf_mode_str[IPERF_MODE_NUM] = {
    "udp-send",
    "udp-recv",
    "tcp-send",
    "tcp-recv",
};

int iperf_show_list(void) {
    int i = 0;
    int run_num = 0;

    for (i = 0; i < IPERF_ARG_HANDLE_MAX; i++) {
        if (g_iperf_arg_handle[i] != NULL) {
            run_num++;

            IPERF_LOG(1, "\nhandle    = %d\n", i);
            IPERF_LOG(1, "mode      = %s\n",
                        iperf_mode_str[g_iperf_arg_handle[i]->mode]);
            IPERF_LOG(1, "host ip   = %s\n", g_iperf_arg_handle[i]->host_ip);
            IPERF_LOG(1, "remote ip = %s\n", g_iperf_arg_handle[i]->remote_ip);
            IPERF_LOG(1, "port      = %u\n", g_iperf_arg_handle[i]->port);
            IPERF_LOG(1, "run time  = %u\n", g_iperf_arg_handle[i]->run_time);
            IPERF_LOG(1, "interval  = %u\n\n", g_iperf_arg_handle[i]->interval);
        }
    }
    if (!run_num)
        return -1;
    return 0;
}

int iperf_parse_argv(int argc, char *argv[])
{
    iperf_arg iperf_arg_t;
    uint32_t port;
    int opt = 0;
    char *short_opts = "LusQ:c:f:p:t:i:b:n:S:B:";
    memset(&iperf_arg_t, 0, sizeof(iperf_arg_t));
#if IPERF_OPT_BANDWIDTH
    iperf_arg_t.bandwidth = 1000 * 1000; /* default to 1Mbits/sec */
#endif
#if IPERF_OPT_NUM
    iperf_arg_t.mode_time = 1;
#endif

    optind = 0; /* reset the index */
    //opterr = 0; /* close the "invalid option" warning */
    while ((opt = getopt(argc, argv, short_opts)) != -1) {
        //IPERF_DBG("opt=%c\n", opt);
        switch (opt) {
            case 'f':
                if (strcmp("m", optarg) == 0 )
                    iperf_arg_t.flags &= ~IPERF_FLAG_FORMAT;
                else if (strcmp("K", optarg) == 0)
                    iperf_arg_t.flags |= IPERF_FLAG_FORMAT;
                else {
                    IPERF_ERR("invalid format arg '%s'\n", optarg);
                    return -1;
                }
                break;
            case 'p':
                port = (uint32_t)atoi(optarg);
                if (port > 65535 || port == 0) {
                    IPERF_ERR("invalid port arg '%s'\n", optarg);
                    return -1;
                } else {
                    iperf_arg_t.port = port;
                    iperf_arg_t.flags |= IPERF_FLAG_PORT;
                }
                break;
            case 'u':
                iperf_arg_t.flags |= IPERF_FLAG_UDP;
                break;
            case 's':
                iperf_arg_t.flags |= IPERF_FLAG_SERVER;
                break;
            case 'c':
            //    if (inet_addr(optarg) == INADDR_NONE) {
            //        IPERF_ERR("invalid ip arg '%s'\n", optarg);
            //        return -1;
            //    } else
                {
                    snprintf(iperf_arg_t.remote_ip, IPERF_ADDR_STRLEN_MAX, "%s", optarg);
                    iperf_arg_t.flags |= IPERF_FLAG_CLINET;
                }
                break;
            case 'i':
                iperf_arg_t.interval = (uint32_t)atoi(optarg);
                break;
            case 't':
                iperf_arg_t.run_time = (uint32_t)atoi(optarg);
                break;
            case 'Q':
                iperf_stop(optarg);
                return -2;
                break;
            case 'L':
                if(iperf_show_list())
                    IPERF_DBG("no task running\n");
                return -2;
                break;
#if IPERF_OPT_BANDWIDTH
            case 'b': {
                uint32_t value;
                char suffix = '\0';
                sscanf(optarg, "%u%c", &value, &suffix);
                if (suffix == 'm' || suffix == 'M') {
                    value *= 1000 * 1000;
                } else if (suffix == 'k' || suffix == 'K') {
                    value *= 1000;
                }
                iperf_arg_t.bandwidth = value;
                break;
            }
#endif
#if IPERF_OPT_NUM
            case 'n': {
                uint32_t value;
                char suffix = '\0';
                sscanf(optarg, "%u%c", &value, &suffix);
                iperf_arg_t.mode_time = 0;
                iperf_arg_t.amount = value;
                if (suffix == 'm' || suffix == 'M') {
                    iperf_arg_t.amount *= 1024 * 1024;
                } else if (suffix == 'k' || suffix == 'K') {
                    iperf_arg_t.amount *= 1024;
                }
                break;
            }
#endif
#if IPERF_OPT_TOS
            case 'S':
                iperf_arg_t.tos = (uint16_t)strtol(optarg, NULL, 0);
                break;
#endif
            case 'B':
            //    if (inet_addr(optarg) == INADDR_NONE) {
            //        IPERF_ERR("invalid ip arg '%s'\n", optarg);
            //        return -1;
            //    } else
                {
                    snprintf(iperf_arg_t.host_ip, IPERF_ADDR_STRLEN_MAX, "%s", optarg);
                }
                break;
            default :
                return -1;
                break;
        }
    }

    /* set the default interval time 1 second */
    if (!iperf_arg_t.interval)
        iperf_arg_t.interval = 1;

    /* Cannot be client and server simultaneously */
    uint32_t flag = IPERF_FLAG_SERVER | IPERF_FLAG_CLINET;
    if ((iperf_arg_t.flags & flag) == 0 || (~iperf_arg_t.flags & flag) == 0) {
        IPERF_ERR("invalid iperf cmd\n");
        return -1;
    }

    if (iperf_arg_t.port == 0)
        iperf_arg_t.port = IPERF_PORT;

    if ((iperf_arg_t.flags & IPERF_FLAG_UDP) == IPERF_FLAG_UDP) {
        if ((iperf_arg_t.flags & IPERF_FLAG_SERVER) == IPERF_FLAG_SERVER)
            iperf_arg_t.mode = IPERF_MODE_UDP_RECV;
        else
            iperf_arg_t.mode = IPERF_MODE_UDP_SEND;
    } else {
        if ((iperf_arg_t.flags & IPERF_FLAG_SERVER) == IPERF_FLAG_SERVER)
            iperf_arg_t.mode = IPERF_MODE_TCP_RECV;
        else
            iperf_arg_t.mode = IPERF_MODE_TCP_SEND;
    }

    int handle = iperf_handle_new(&iperf_arg_t);
    return handle;
}

void asr_wifi_iperf(int argc, char **argv)
{
    char iperf_usage[] = "usage:\n\t"
                         "iperf [-B host][-f speed format] [-p port] [-u UDP] [-s server] [-c client] [-i interval]"
                         " [-t run time] [-Q stop] [-L show iperf param list] [-b bandwidth] [-n count]\n\t"
                         "e.g:\n\t"
                         "iperf -u -s -p 6000\n\t"
                         "iperf -Q 0\n";

    if (argc == 1 || (strcmp(argv[1], "-h") == 0))
    {
        printf("%s\n", iperf_usage);
		return ;
    }

   int iperf_handle = -1;

   iperf_handle = iperf_parse_argv(argc,argv);

   if (iperf_handle >= 0)
       iperf_start(NULL,iperf_handle);


}
