/**
 ****************************************************************************************
 *
 * @file asr_at_api.h
 *
 * @brief AT API.
 *
 * Copyright (C) ASR
 *
 ****************************************************************************************
 */
#ifndef _ASR_AT_API_H_
#define _ASR_AT_API_H_

//#include "asr_uart.h"
typedef struct _cmd_entry {
    char *command;
    int (*function)(int, char **);
    char *help;
} cmd_entry;

typedef struct {
    char *command;  /*at cmd string*/
    int (*function)(int argc, char **argv); /*at cmd proccess function*/
} asr_at_cmd_entry;

typedef struct
{
    uint8_t   uart_echo;       /* echo uart input info log */
    uint8_t   max_txpwr;       /* max tx power for both sta and softap */
    uint8_t   flag_sap;        /* flag of user set softap ip config */
    uint8_t   flag_sta;        /* flag of user set sta ip config */
    uint8_t   dis_dhcp;        /* disable dhcp func, use static ip */
    uint8_t   at_scan;         /* scan flag which indicate call by at task */
    uint8_t   sta_connected;   /* indicate status of station is connected */
    uint8_t   sap_opend;       /* indicate status of softap is open done */
    //ip_addr_t at_ping;         /* save ping ip addr for at cmd */
    char      staip[16];       /* Local IP address on the target wlan interface for station mode, ASCII */
    char      stagw[16];       /* Router IP address on the target wlan interface for station mode, ASCII */
    char      stamask[16];     /* Netmask on the target wlan interface for station mode, ASCII */
    char      sapip[16];       /* Local IP address on the target wlan interface for softap mode, ASCII */
    char      sapgw[16];       /* Router IP address on the target wlan interface for softap mode, ASCII */
    char      sapmask[16];     /* Netmask on the target wlan interface for softap mode, ASCII */
    char      start_ip[16];    /* start ip addr of dhcp pool in softap mode */
    char      end_ip[16];      /* end ip addr of dhcp pool in softap mode */
}_at_user_info;

#if 0
typedef enum
{
    CONFIG_OK,          /* indicate at cmd set success and response OK */
    PARAM_RANGE,        /* indicate some at cmd param is out of range */
    PARAM_MISS,         /* indicate at cmd param is less than needed count */
    CONFIG_FAIL,        /* indicate at cmd set failed, or execute fail */
    CONN_TIMEOUT,       /* indicate connect timeout in station mode */
    CONN_EAPOL_FAIL,    /* indicate 4-way handshake failed in station mode */
    CONN_DHCP_FAIL,     /* indicate got ip by dhcp failed in station mode */
    WAIT_PEER_RSP,
    RSP_NULL=0xFF
}asr_at_rsp_status_t;
#endif

/** @brief  register user at cmd.
 *
 * @param cmd_entry    : user at cmd array pointer
 * @param cmd_num      : user at cmd number
 */
int asr_at_cmd_register(asr_at_cmd_entry *cmd_entry, int cmd_num);

/** @brief  at init functin, user should call it before use at cmd
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_at_init(void);

/** @brief  at deinit functin, user should call it when donot use at any more, to free resources
 *  @return    0       : on success.
 *  @return    other   : error occurred
 */
int asr_at_deinit(int argc, char **argv);

/** @brief  at command callback function, used to register to uart.
 */
void at_handle_uartirq(char ch);

/** @brief  uart handle for receiving at command.
 */
//extern asr_uart_dev_t asr_at_uart;

#endif  //_ASR_AT_API_H_


