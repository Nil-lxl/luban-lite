#ifndef _AT_HANDLE_H
#define _AT_HANDLE_H

#include "tasks_info.h"

//#define AT_USER_DEBUG

#ifdef ALIOS_SUPPORT
#define LWIP
#endif

#define UART_RXBUF_LEN            128
#define MIN_USEFUL_DEC            32
#define MAX_USEFUL_DEC            127
#define UART_CMD_NB               5

#define WIFI_RFTEST

//#define     UWIFI_AT_TASK_NAME                "AT_task"
//#define     UWIFI_AT_TASK_PRIORITY            (244)
//#define     UWIFI_AT_TASK_STACK_SIZE          (2048 - 256)

//#define        UART_TASK_NAME                "UART_task"
//#define     UART_TASK_PRIORITY            (19)
//#define     UART_TASK_STACK_SIZE        (256*6)

typedef struct uart_at_msg {
    uint32_t str;
} uart_at_msg_t;
#ifdef CFG_SMART_CONFIG_NETWORK
//Multicast Control field
/*
    'ASR'+4bit 0000 -- reserved
    'ASR'+4bit 0001 -- start indication
    'ASR'+4bit xxxx -- reserved
    'ASR'+4bit 1111 -- stop indication
*/
#define MC_CNTRL_TICK           0x41535200
#define MC_CNTRL_START                0x41535210
#define MC_CNTRL_END                  0x415352f0
#define MC_CNTRL_RESERVED_1           0x41535220
#define MC_CNTRL_RESERVED_2           0x41535230
#define MC_CNTRL_RESERVED_3           0x41535240
#define MC_CNTRL_RESERVED_4           0x41535250
#define MC_CNTRL_RESERVED_5           0x41535260
#define MC_CNTRL_RESERVED_6           0x41535270
#define MC_CNTRL_RESERVED_7           0x41535280
#define MC_CNTRL_RESERVED_8           0x41535290
#define MC_CNTRL_RESERVED_9           0x415352a0
#define MC_CNTRL_RESERVED_10          0x415352b0
#define MC_CNTRL_RESERVED_11          0x415352c0
#define MC_CNTRL_RESERVED_12          0x415352d0
#define MC_CNTRL_RESERVED_13          0x415352e0

#define MC_CNTRL                      0x41530000
//Multicast Len field
#define MC_LN                         0x4c4e0000
//Multicast Index field
#define MC_IX                         0x49580000
//Multicast CheckSum filed
#define MC_CS                         0x43530000
//Multicast IP field
#define MC_IP                         0x49500000
typedef enum dynamic_encode_method {
    INFO_ENCODE_4 = 0,//'0' ~ '0'
    INFO_ENCODE_5_1,//'a' ~ 'z'
    INFO_ENCODE_5_2,//'A' ~ 'Z'
    INFO_ENCODE_6,//'0' ~ '0', 'a' ~ 'z', 'A' ~ 'Z'
    INFO_ENCODE_7,// all visiable characters of ascii
} dynamic_encode_method_t;

#endif
#ifdef CFG_DEVICE_CONFIG_NETWORK
typedef struct device_config_network_parameter {
    char ssid[32];
    char password[64];
    uint32_t checksum;
    //uint16_t chan_no;
} device_config_network_para_t;

#define ASR_DEVICE_CONFIG_NETWORK_SSID     "softap_asr"
#define ASR_DEVICE_CONFIG_NETWORK_PASSWORD "12345678"
#define ASR_DEVICE_CONFIG_NETWORK_CHAN     11
#define ASR_DEVICE_CONFIG_NETWORK_DEFAULT_PORT 12345
#endif
void uart_at_msg_send(char *cmd_str);


int convert_str_to_int(char *str);

int asr_at_wifi_ping_stop(int argc, char **argv);


#endif //_AT_HANDLE_H
