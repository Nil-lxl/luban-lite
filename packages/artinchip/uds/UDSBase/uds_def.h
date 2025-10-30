/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _AIC_UDS_DEFINE_H_
#define _AIC_UDS_DEFINE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <rtthread.h>
#include <rtdevice.h>
#define CAN_TX_DEV_NAME      "can0"
#define CAN_RX_DEV_NAME      "can1"
#define CAN_FRAME_SIZE       8     // CAN Frame size
#define UDS_REQUEST_ID       0x7E0 // REQUEST ID（Tester→ECU）
#define UDS_RESPONSE_ID      0x7E8 // RESPONSE ID（ECU→Tester）
#define UDS_FUNCTION_ID      0x7DF // Function ID
#define UDS_RX_MAX           1024  // The length of RX FIFO, max value is 4095
#define UDS_TX_MAX           1024  // The length of TX FIFO, max value is 4095
#define UDS_DATA_PADDING_VAL 0x00
typedef enum {
    /* General rejection codes */
    NRC_GENERAL_REJECT = 0x10,                   //General rejection (not supported by protocol)
    NRC_SERVICE_NOT_SUPPORTED = 0x11,            // Service not supported by ECU
    NRC_SUBFUNCTION_NOT_SUPPORTED = 0x12,        // Sub-function not supported
    NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT = 0x13, // Invalid message length or format

    /* Condition not correct codes */
    NRC_CONDITIONS_NOT_CORRECT = 0x22, // Preconditions not met
    NRC_REQUEST_SEQUENCE_ERROR = 0x24, // Invalid request sequence

    /* Request out of range codes */
    NRC_REQUEST_OUT_OF_RANGE = 0x31, // Parameter out of range or unsupported data ID

    /* Security related codes */
    NRC_SECURITY_ACCESS_DENIED = 0x33,          // Security access denied
    NRC_INVALID_KEY = 0x35,                     // Invalid security key
    NRC_EXCEEDED_NUMBER_OF_ATTEMPTS = 0x36,     // Maximum number of attempts exceeded
    NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37, // Required time delay not expired

    /* Upload/download codes */
    NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70, // Upload/download not accepted
    NRC_TRANSFER_DATA_SUSPENDED = 0x71,      // Data transfer suspended
    NRC_GENERAL_PROGRAMMING_FAILURE = 0x72,  // General programming failure
    NRC_WRONG_BLOCK_SEQUENCE_COUNTER = 0x73, // Invalid block sequence counter

    /* Busy response codes */
    NRC_SERVICE_BUSY = 0x78, // Service request received, response pending

    /* Session related codes */
    // Sub-function not supported in current session
    NRC_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7E,
    // Service not supported in current session
    NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7F,

    /* Vehicle condition codes */
    NRC_VOLTAGE_TOO_HIGH = 0x92, // Voltage above threshold
    NRC_VOLTAGE_TOO_LOW = 0x93   // Voltage below threshold
} uds_negative_response_code_t;

/* The highest bit (bit 7) of the second byte indicates the positive response suppression bit.
 * When this bit is set to 1, it indicates that a positive response reply is not required,
 * and the server only needs to process the service request.
 */
#define UDS_GET_SUB_FUNCTION_SUPPRESS_POSRSP(byte) ((byte >> 7u) & 0x01u)
/* Get Sub-function Number -- The lower 7 bits of the second byte represent the sub-function number,
 * with a range of 0 to 0x7F.'
 */
#define UDS_GET_SUB_FUNCTION(byte) (byte & 0x7fu)
// positive response, server ID should +0x40
#define POSITIVE_RSP                    0x40
#define USD_GET_POSITIVE_RSP(server_id) (POSITIVE_RSP + server_id)
// negative response
#define NEGATIVE_RSP 0x7F
/* Security Access Timeout Period, unit: ms.
 * If the number of successful security access seed matches reaches two, start this timer.
 * If a request seed service is received within the TIMEOUT_FSA period,
 * a response with NRC 37 shall be sent.
 */
#define TIMEOUT_FSA 10000

/*
 * S3server timer timeout period. In non-default session mode, if no message is received within
 * TIMEOUT_S3server period, the system will automatically revert to default session.
 */
#define TIMEOUT_S3server 5000
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif
typedef unsigned char bool_t;

typedef enum {
    UDS_STATE_IDLE,
    UDS_STATE_ENTER_EXT_SESSION,
    UDS_STATE_READ_ECU_INFO,
    UDS_STATE_ENTER_PROG_SESSION,
    UDS_STATE_SAFE_ACCESS,
    UDS_STATE_SAFE_ACCESS_SEND_VERIFY,
    UDS_STATE_ERASE_CODE,
    UDS_STATE_REQUEST_UPLOAD,
    UDS_STATE_SEND_UPLOAD,
    UDS_STATE_REQUEST_UPLOAD_EXIT,
    UDS_STATE_APP_CHECK,
    UDS_STATE_CODE_CHECK,
    UDS_STATE_ECU_RESET,
    UDS_STATE_ENTER_DEFAULT_SESSION,
    UDS_STATE_WAIT_DOWNLOAD_RESPONSE,
    UDS_STATE_WAIT_TRANSFER_RESPONSE,
    UDS_STATE_ERROR,
    UDS_STATE_REQUEST_UPLOAD_WAIT,
} uds_state_t;

extern uds_state_t g_current_state;
extern uint32_t g_max_block_size;
extern uint32_t g_bytes_per_block;
extern struct rt_semaphore g_flow_control_sem;
extern uint32_t req_total_len;
extern rt_device_t g_can_tx_dev;
extern rt_mq_t can_tx_mq;
extern rt_sem_t can_tx_sem;
extern FILE *transfer_file;
extern rt_mutex_t g_uds_state_mutex;
extern char uds_output_file_path[256];

typedef enum {
    UDS_TIMER_FSA = 0,  // Security Access timer
    UDS_TIMER_S3server, // S3server timer
    UDS_TIMER_CNT       // Number of application layer timers
} uds_timer_t;
typedef enum {
    UDS_SESSION_STD = 1, // default session
    UDS_SESSION_PROG,    // program session
    UDS_SESSION_EXT      // extended session
} uds_session_t;
typedef enum {
    UDS_SA_NON = 0, // Security Access Level: NONE
    UDS_SA_LV1,     // Security Access Level: 1 level
    UDS_SA_LV2,     // Security Access Level: 2 level
} uds_sa_lv;
typedef struct {
    uint8_t uds_sid;                                // server ID
    void (*uds_service)(const uint8_t *, uint16_t); // service processing function
    bool_t (*check_len)(const uint8_t *, uint16_t); // check the length is valid or not
    bool_t std_spt;                                 // whether default session is supported
    bool_t prog_spt;                                // whether program session is supported
    bool_t ext_spt;                                 // whether extended session is supported
    bool_t fun_spt;                                 // whether addressing function is supported
    bool_t ssp_spt;                                 // whether positive response supression
    uds_sa_lv uds_sa;                               // Security Access Level
} uds_service_t;
typedef enum _N_TATYPE_T_ {
    N_TATYPE_NONE = 0,  // none
    N_TATYPE_PHYSICAL,  // physical addressing
    N_TATYPE_FUNCTIONAL // function addressing
} n_tatype_t;
typedef enum _N_RESULT_ {
    N_OK = 0,
    N_TIMEOUT_Bs, // TIMER_N_BS timer timeout
    N_TIMEOUT_Cr, // TIMER_N_CR timer timeout
    N_WRONG_SN,   // Incorrect sequence number in received consecutive frame
    N_INVALID_FS, // Invalid flow status in received flow control frame
    /* Frame type not expected.
     * For example, receiving a first frame while expecting a consecutive frame
     */
    N_UNEXP_PDU,
    N_BUFFER_OVFLW, // Overflow status received in flow control frame
} n_result_t;

/*
 * Interface functions registered by the upper layer to the TP layer.
 * After the TP layer completes data processing,
 * it delivers the data back to the upper layer for further processing through
 * these interface functions.
 */
typedef void (*ffindication_func)(n_result_t n_result);
typedef void (*indication_func)(uint8_t *msg_buf, uint16_t msg_dlc, n_result_t n_result);
typedef void (*confirm_func)(n_result_t n_result);
typedef struct _NETWORK_USER_DATA_T_ {
    ffindication_func ffindication;
    indication_func indication;
    confirm_func confirm;
} nt_usdata_t;
// 0:physical addressing; 1:function addressing
extern uint8_t g_tatype;
typedef enum {
    /* N_CR timer. The time interval between consecutive frames received by the receiver
     * shall not exceed TIMEOUT_N_CR. unit: ms
     */
    TIMER_N_CR = 0,
    /* N_BS timer. The time between the sender transmitting the first frame
     * and receiving the flow control frame shall not exceed TIMEOUT_N_BS. unit: ms
     */
    TIMER_N_BS,
    /* STmin timer. When sending consecutive farmes, the minimum interval between frames is
     * NT_XMIT_FC_STMIN. unit: ms
     */
    TIMER_STmin,
    TIMER_CNT // Total number of timers
} nt_timer_t;
typedef enum {
    NWL_IDLE = 0, // idle state
    NWL_XMIT,     // transmitting state
    NWL_RECV,     // receive state
    NWL_CNT       // Total number of states
} network_layer_st;
typedef enum {
    PCI_SF = 0, // single frame
    PCI_FF,     // first frame
    PCI_CF,     // consecutive frame
    PCI_FC      // flow control frame
} network_pci_type_t;
typedef enum {
    FS_CTS = 0, // allow continued transmission
    FS_WT,      // waitting
    FS_OVFLW,   // overflow
    FS_RESERVED // invalid
} network_flow_status_t;
/* Padding value. If the transmitted valid data does not fill a complete frame,
 * the remainder shall be padded with this value.
 */
// #define PADDING_VAL                 (0x55)
// Set frame type to single frame
#define NT_SET_PCI_TYPE_SF(low) (0x00 | (low & 0x0f))
// Set frame type to first frame
#define NT_SET_PCI_TYPE_FF(low) (0x10 | (low & 0x0f))
// Set frame type to consecutive frame
#define NT_SET_PCI_TYPE_CF(low) (0x20 | (low & 0x0f))
// Set frame type to flow control frame
#define NT_SET_PCI_TYPE_FC(low) (0x30 | (low & 0x0f))
// Get frame type
#define NT_GET_PCI_TYPE(n_pci) (n_pci >> 4)
// Get consecutive frame sequence number
#define NT_GET_CF_SN(n_pci) (0x0f & n_pci)
// Get flow status
#define NT_GET_FC_FS(n_pci) (0x0f & n_pci)

/*
 * The number of consecutive frames allowed to be sent.
 * If set to 0, it indicates that the sender can continuously transmit consecutive frames
 * without restriction until all data is sent. If not 0, it means that after the sender has
 * transmitted NT_XMIT_FC_BS consecutive frames, it must wait for a flow control frame from
 * the receiver to determine the subsequent transmission behavior.
 */
#define NT_XMIT_FC_BS (0)
// Notify the sender of the minimum time interval between consecutive frames. unit: ms
#define NT_XMIT_FC_STMIN (0x0A)
/* The time interval between consecutive frames received by the receiver shall
 * not exceed TIMEOUT_N_CR. unit: ms
 */
#define TIMEOUT_N_CR (1000)
/* The time between the sender completing the transmission of the first frame and
 * receiving a flow control frame shall not exceed TIMEOUT_N_BS. unit: ms
 */
#define TIMEOUT_N_BS (1000)
void uds_init(void);
void uds_1ms_task(void);
void print_uds_response(uint32_t id, uint8_t *data, uint8_t len);
void uds_recv_frame(uint32_t id, uint8_t *frame_buf, uint8_t frame_dlc);
void uds_send_frame(uint32_t response_id, uint8_t *frame_buf, uint8_t frame_dlc,
                    uint8_t expect_response);
void uds_timer_start(uds_timer_t num);
void uds_negative_rsp(uint8_t sid, uds_negative_response_code_t rsp_nrc);
void uds_positive_rsp(uint8_t *data, uint16_t len);
int uds_timer_chk(uds_timer_t num);
void service_task(void);
int service_init(void);
void network_task(void);
void uds_tp_recv_frame(uint8_t func_addr, uint8_t *frame_buf, uint8_t frame_dlc);
int network_send_udsmsg(uint8_t *msg_buf, uint16_t msg_dlc);
int network_reg(nt_usdata_t *usdata);
void assemble_complete_frame(void);
int send_multipleframe(uint8_t *msg_buf, uint16_t msg_dlc);
int uds_tester_device_tx_init(void);
void nt_timer_start_wv(nt_timer_t num, uint32_t value);
void set_feak_mode(void);

#endif
