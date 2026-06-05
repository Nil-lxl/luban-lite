/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li siyao <siyao.li@artinchip.com>
 */
#ifndef _AIC_HAL_CAN_H_
#define _AIC_HAL_CAN_H_

#include <aic_core.h>
#include "aic_osal.h"

/* Register address */
#define CANFD_RBUF0_REG                     0x0000
#define CANFD_RBUF1_REG                     0x0004
#define CANFD_TBUF0_REG                     0x0050
#define CANFD_TBUF1_REG                     0x0054
#define CANFD_CFG_REG                       0x00A0
#define CANFD_TCMD_REG                      0x00A1
#define CANFD_TCTRL_REG                     0x00A2
#define CANFD_RCTRL_REG                     0x00A3
#define CANFD_RTIE_REG                      0x00A4
#define CANFD_RTIF_REG                      0x00A5
#define CANFD_ERRINT_REG                    0x00A6
#define CANFD_LIMIT_REG                     0x00A7
#define CANFD_SSEG1_REG                     0x00A8
#define CANFD_SSEG2_REG                     0x00A9
#define CANFD_SSJW_REG                      0x00AA
#define CANFD_SPRESC_REG                    0x00AB
#define CANFD_FSEG1_REG                     0x00AC
#define CANFD_FSEG2_REG                     0x00AD
#define CANFD_FSJW_REG                      0x00AE
#define CANFD_FPRESC_REG                    0x00AF
#define CANFD_EALCAP_REG                    0x00B0
#define CANFD_TDC_REG                       0x00B1
#define CANFD_RECNT_REG                     0x00B2
#define CANFD_TECNT_REG                     0x00B3
#define CANFD_ACFCTRL_REG                   0x00B4
#define CANFD_TIMECFG_REG                   0x00B5
#define CANFD_EN0_REG                       0x00B6
#define CANFD_EN1_REG                       0x00B7
#define CANFD_ACODEx_REG                    0x00B8 // or CANFD_MASKx_REG
#define CANFD_TBSLOT_REG                    0x00BE
#define CANFD_TTCFG_REG                     0x00BF
#define CANFD_DMAC_REG                      0x00D4

/* Status reg */
#define CANFD_CFG_RESET_FLAG                BIT(7)
#define CANFD_CFG_RESET_SHIFT               7
#define CANFD_CFG_LBME_FLAG                 BIT(6)
#define CANFD_CFG_LBMI_FLAG                 BIT(5)
#define CANFD_CFG_TPSS_FLAG                 BIT(4)
#define CANFD_CFG_TSSS_FLAG                 BIT(3)
#define CANFD_CFG_RACTIVE_FLAG              BIT(2)
#define CANFD_CFG_TACTIVE_FLAG              BIT(1)
#define CANFD_CFG_BUSOFF_FLAG               BIT(0)

#define CANFD_TCMD_TBSEL_FLAG               BIT(7)
#define CANFD_TCMD_LOM_FLAG                 BIT(6)
#define CANFD_TCMD_STBY_FLAG                BIT(5)
#define CANFD_TCMD_TPE_FLAG                 BIT(4)
#define CANFD_TCMD_TPA_FLAG                 BIT(3)
#define CANFD_TCMD_TSONE_FLAG               BIT(2)
#define CANFD_TCMD_TSALL_FLAG               BIT(1)
#define CANFD_TCMD_TSA_FLAG                 BIT(0)

/* arb lost reg */
#define CANFD_EALCAP_ALC_MASK               GENMASK(4, 0)

/* Kind of error*/
#define CANFD_EALCAP_KOER_SHIFT             5
#define CANFD_EALCAP_KOER_MASK              GENMASK(7, CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_NO                (0x0 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_BIT               (0x1 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_FORMAT            (0x2 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_STUFF             (0x3 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_ACK               (0x4 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_CRC               (0x5 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_OTHER             (0x6 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_KOER_NO_USE            (0x7 << CANFD_EALCAP_KOER_SHIFT)
#define CANFD_EALCAP_ALC_MASK               GENMASK(4, 0)


/* Recv buf0 reg */
#define CANFD_RBUF0_ESI_SHIFT               31
#define CANFD_RBUF0_ID_STAND_MASK           GENMASK(10, 0)
#define CANFD_RBUF0_ID_EXT_MASK             GENMASK(28, 0)

/* Recv buf1 reg */
#define CANFD_RBUF1_CYCLE_TIME              GENMASK(31, 16)
#define CANFD_RBUF1_KOER_MASK               GENMASK(15, 13)
#define CANFD_RBUF1_TX_SHIFT                12
#define CANFD_RBUF1_IDE_FLAG                BIT(7)
#define CANFD_RBUF1_RTR_FLAG                BIT(6)
#define CANFD_RBUF1_IDE_SHIFT               7
#define CANFD_RBUF1_RTR_SHIFT               6
#define CANFD_RBUF1_FDF_SHIFT               5
#define CANFD_RBUF1_BRS_SHIFT               4
#define CANFD_RBUF1_DLC_MASK                GENMASK(3, 0)

/* Send buf01 reg */
#define CANFD_TBUF0_TTSEN_FLAG              BIT(31)
#define CANFD_TBUF0_ID_STAND_MASK           GENMASK(28, 0)
#define CANFD_TBUF0_ID_EXT_MASK             GENMASK(10, 0)

/* Send buf1 reg */
#define CANFD_TBUF1_IDE_FLAG                BIT(7)
#define CANFD_TBUF1_RTR_FLAG                BIT(6)
#define CANFD_TBUF1_FDF_FLAG                BIT(5)
#define CANFD_TBUF1_BRS_FLAG                BIT(4)
#define CANFD_TBUF1_IDE_SHIFT               7
#define CANFD_TBUF1_RTR_SHIFT               6
#define CANFD_TBUF1_FDF_SHIFT               5
#define CANFD_TBUF1_BRS_SHIFT               4

#define CANFD_TCTRL_FD_ISO_FLAG             BIT(7)
#define CANFD_TCTRL_TSNEXT_FLAG             BIT(6)
#define CANFD_TCTRL_TSMODE_FLAG             BIT(5)
#define CANFD_TCTRL_TTTBM_FLAG              BIT(4)
#define CANFD_TCTRL_TSSTAT_MASK             GENMASK(1, 0)

#define CANFD_RCTRL_SACK_FLAG               BIT(7)
#define CANFD_RCTRL_SACK_SHIFT              7
#define CANFD_RCTRL_ROM_SHIFT               6
#define CANFD_RCTRL_ROV_SHIFT               5
#define CANFD_RCTRL_RREL_FLAG               BIT(4)
#define CANFD_RCTRL_RBALL_SHIFT             3
#define CANFD_RCTRL_RSTAT_MASK              GENMASK(1, 0)

#define CANFD_RBUF_DATA(n)                  ((n & 0x1) << 12)

#define CANFD_RTIF_RIF_FLAG                 BIT(7)
#define CANFD_RTIF_ROIF_FLAG                BIT(6)
#define CANFD_RTIF_RFIF_FLAG                BIT(5)
#define CANFD_RTIF_RAFIF_FLAG               BIT(4)
#define CANFD_RTIF_TPIF_FLAG                BIT(3)
#define CANFD_RTIF_TSIF_FLAG                BIT(2)
#define CANFD_RTIF_EIF_FLAG                 BIT(1)
#define CANFD_RTIF_AIF_FLAG                 BIT(0)

#define CANFD_RTIE_ALL_MASK                 GENMASK(7, 0)

#define CANFD_ERRINT_EWARN_FLAG             BIT(7)
#define CANFD_ERRINT_EPASS_FLAG             BIT(6)
#define CANFD_ERRINT_EPIE_FLAG              BIT(5)
#define CANFD_ERRINT_EPIF_FLAG              BIT(4)
#define CANFD_ERRINT_ALIE_FLAG              BIT(3)
#define CANFD_ERRINT_ALIF_FLAG              BIT(2)
#define CANFD_ERRINT_BEIE_FLAG              BIT(1)
#define CANFD_ERRINT_BEIF_FLAG              BIT(0)
#define CANFD_ERRINT_ALL_IRQ_FLAG           (CANFD_ERRINT_EPIF_FLAG | CANFD_ERRINT_ALIF_FLAG | CANFD_ERRINT_BEIF_FLAG)
#define CANFD_ERRINT_ALL_IE                 (CANFD_ERRINT_EPIE_FLAG | CANFD_ERRINT_ALIE_FLAG | CANFD_ERRINT_BEIE_FLAG)

#define CANFD_LIMIT_AFWL_MASK               GENMASK(7, 4)
#define CANFD_LIMIT_EWL_MASK                GENMASK(3, 0)

#define CANFD_TTCFG_WTIE_FLAG               BIT(7)
#define CANFD_TTCFG_WTIF_FLAG               BIT(6)
#define CANFD_TTCFG_TEIF_FLAG               BIT(5)
#define CANFD_TTCFG_TTIE_FLAG               BIT(4)
#define CANFD_TTCFG_TTIF_FLAG               BIT(3)
#define CANFD_TTCFG_T_PRESC_MASK            GENMASK(2, 1)
#define CANFD_TTCFG_TTEN_FLAG               BIT(0)
#define CANFD_TTCFG_ALL_IRQ_FLAG            (CANFD_TTCFG_WTIF_FLAG | CANFD_TTCFG_TEIF_FLAG | CANFD_TTCFG_TTIF_FLAG)

#define CANFD_SCR_THR_SHIFT                 24
#define CANFD_SCR_EN_SHIFT                  16
#define CANFD_SCR_EN_FLAG                   BIT(0)

#define CANFD_DST_EN_SHIFT                  8
#define CANFD_DMAC_DST_EN_FLAG              BIT(0)
#define CANFD_DMAC_TS_AUTO_FLAG             BIT(1)

#define CANFD_ACFCTRL_SELMASK_FLAG          BIT(5)
#define CANFD_ACFCTRL_ADRMASK               GENMASK(3, 0)
#define CANFD_ACFCTRL_ADR(x)                (x % 16)
#define CANFD_ACF_NUM                       16

#define CANFD_SEG_1_SHIFT                   0
#define CANFD_SEG_2_SHIFT                   8
#define CANFD_SJW_SHIFT                     16
#define CANFD_PRESC_SHIFT                   24

#define CANFD_EFF_MASK                      GENMASK(28, 0)
#define CANFD_SFF_MASK                      GENMASK(10, 0)

#define CANFD_MAX_TIMEOUT                   100000 /* ms */
#define CANFD_CAN_FRAME_MAX_BYTES           8

typedef enum {
    CANFD_STATE_ACTIVE = 0,         /* RX/TX error count < 96 */
    CANFD_STATE_WARNING,            /* RX/TX error count < 128 */
    CANFD_STATE_PASSIVE,            /* RX/TX error count < 256 */
    CANFD_STATE_BUS_OFF,                  /* RX/TX error count >= 256 */
    CANFD_STATE_STOPPED,
    CANFD_STATE_SLEEPING,
    CANFD_STATE_MAX,
} canfd_state_t;

typedef struct canfd_bittiming_const canfd_bittiming_const;
struct canfd_bittiming_const{
    u32 tseg1_min;
    u32 tseg1_max;
    u32 tseg2_min;
    u32 tseg2_max;
    u32 sjw_max;
    u32 brp_min;
    u32 brp_max;
    u32 brp_inc;
};

typedef enum {
    CANFD_NO_RESET = 0,
    CANFD_ACT_RESET = 1
} canfd_reset_stat_t;

typedef enum {
    CANFD_TXMODE_FULL     = 0, // FULL-TTCAN mode
    CANFD_TXMODE_STB_FIFO = 1,
    CANFD_TXMODE_STB_PRIO = 2,
    CANFD_TXMODE_PTB = 3 // ptb mode which supports to send out of turn
} canfd_tx_mode_t;

typedef enum {
    CANFD_TB_TYPE_PTB = 0,
    CANFD_TB_TYPE_STB = 1
} canfd_tbsel_t;

typedef enum {
    CANFD_TXTYPE_TSONE = 0, // tsone mode which sends one frame at a time
    CANFD_TXTYPE_TSALL = 1, // tsall mode which sends all frame at a time
    CANFD_TXTYPE_TTCAN = 2, // time-trigger CAN
    CANFD_TXTYPE_NONE = 3
} canfd_tx_type_t;

typedef enum {
    CANFD_RUNMODE_NORMAL   = 0,
    CANFD_RUNMODE_EXTERNAL = 1,  // external loopback mode
    CANFD_RUNMODE_INTERNAL = 2,  // internal loopback mode
    CANFD_RUNMODE_STANDBY = 3,
    CANFD_RUNMODE_LISTEN = 4,    // listen only mode
    CANFD_RUNMODE_SINGLE = 5     // TPSS and TSSS,single shot
} canfd_run_mode_t;

typedef enum {
    CANFD_SET_RUNMODE   = 0,
    CANFD_SET_TXMODE = 1,
    CANFD_SET_TXTYPE = 2
} canfd_set_mode_t;

struct aic_canfd_mode_info
{
    canfd_run_mode_t run_mode;
    canfd_tx_type_t tx_type;
    canfd_tx_mode_t tx_mode;
};

typedef enum {
    CANFD_BAUD_FD   = 0, //For CANFD
    CANFD_BAUD = 1 //For CAN2.0
} canfd_baud_type_t;

struct aic_canfd_baud_info
{
    unsigned long baudrate;
    u32 duty;
    u32 prop_seg;
    u32 phase_seg1;
    u32 phase_seg2;
    u32 brp;
    u32 sjw;
    u32 tq;
};

struct aic_canfd_allbaud_info
{
    canfd_baud_type_t baud_type;
    struct aic_canfd_baud_info slow_baud;
    struct aic_canfd_baud_info fast_baud;
};

typedef enum
{
    CANFD_FILTER_CODE = 0,
    CANFD_FILTER_MASK = 1
} CANFD_FILTER_SEL;

struct canfd_filter_item
{
    u32 id;
    u32 ide;
    u32 rtr;
    u32 mode;
    u32 mask;
};

typedef struct canfd_filter_config {
    int filter_chan;
    u8 is_eff;
    struct canfd_filter_item *items;
} canfd_filter_config_t;

typedef enum {
    CAN_TYPE       = 0,
    CANFD_TYPE     = 1,
} can_type_t;

enum {
    CAN_FRAME_TYPE_DATA      = 0,
    CAN_FRAME_TYPE_REMOTE    = 1,
};

enum aic_canfd_obtain_data_mode {
    CANFD_OBTAIN_DATA_BY_CPU = 0,
    CANFD_OBTAIN_DATA_BY_DMA = 1
};

typedef void (*dma_callback)(void *dma_param);

struct canfd_dma_transfer_info
{
    u32 chan_id;
    struct aic_dma_chan *dma_chan;

    void *buf;
    int buf_size;
    void *callback_param;
    dma_callback callback;
};

typedef struct {
    u8 code;
    char *msg;
} canfd_bus_err_msg_t;

typedef struct canfd_status {
    canfd_state_t current_state;
    u32    recverrcnt; // receive error count
    u32    snderrcnt; // send error count
    u32    rxovercnt;
    u32    arblostcnt;
    u32    biterrcnt;    // KOER count
    u32    formaterrcnt;
    u32    stufferrcnt;
    u32    ackerrcnt;
    u32    crcerrcnt;
    u32    othererrcnt;   // KOER count
    u32    recvpkgcnt;
    u32    sndpkgcnt;
    u32    rxerrcnt;
    u32    txerrcnt;
} canfd_status_t;

typedef struct {
    u32    id;      // IDE = 1, 29bit EXID; IDE = 0, 11bit STID
    u32    ttsen;   // Transmit time-stamp enable
    u8     rtr;     // Remote Transmission Request bit
    u8     ide;     // ID Extended flag
    u32    fdf;     // 1:CAN FD      0:CAN
    u32    brs;     // Bit Rate Switch
    u8     dlc;     // Data Length Code reg
    u8     len;     // Data Length
    u8    data[64]; // Data Buffer
} canfd_msg_t;

typedef struct canfd_handle canfd_handle;
struct canfd_handle {
    unsigned long canfd_base;
    u32 irq_num;
    u32 clk_id;
    u32 idx;
    u8 can_type;
    void (*callback)(canfd_handle * phandle, void *arg);
    void *arg;
    u32 s_baud;
    u32 f_baud;
    canfd_msg_t msg;
    canfd_status_t status;
    struct aic_canfd_mode_info mode;
    u32 tbsel;/*tbsel=0 choose PTB,tbsel=1 choose STB*/
    u8 dma_port_id;
    enum aic_canfd_obtain_data_mode obtain_data_mode;
    struct canfd_dma_transfer_info dma_rx_info;
    struct canfd_dma_transfer_info dma_tx_info;
    u8 running;
    canfd_filter_config_t hw_filter;
};

static const unsigned int canfd_data_lengths[16] = {
    0,  1,  2,  3,  4,  5,  6,  7,
    8, 12, 16, 20, 24, 32, 64,  0
};

static inline void hal_canfd_enable_interrupt(canfd_handle *phandle)
{
    writeb(CANFD_RTIE_ALL_MASK, phandle->canfd_base + CANFD_RTIE_REG);
}

static inline void hal_canfd_disable_interrupt(canfd_handle *phandle)
{
    writeb(0, phandle->canfd_base + CANFD_RTIE_REG);
}

#define CANFD_IOCTL_SET_MODE            1
#define CANFD_IOCTL_SET_FILTER          2
#define CANFD_IOCTL_SET_BAUDRATE        4
#define CANFD_IOCTL_GET_BAUDRATE        8
#define CANFD_IOCTL_RELEASE_MODE        0x10

#define CAN_EVENT_RX_IND                0x01    /* Rx indication */
#define CAN_EVENT_TX_DONE               0x02    /* Tx complete   */
#define CAN_EVENT_TX_FAIL               0x03    /* Tx fail   */
#define CAN_EVENT_RXOF_IND              0x06    /* Rx overflow */

u8 hal_canfd_dlc2len(u8 dlc);
u8 hal_canfd_len2dlc(u8 len);
int canfd_tx_active(canfd_handle *phandle);
void hal_canfd_set_reset_mode(canfd_handle *phandle, canfd_reset_stat_t status);
void hal_canfd_enable_int(canfd_handle *phandle, bool enable);
void canfd_reg_enable(unsigned long canfd_base, int offset, u8 bit);
int hal_canfd_init(canfd_handle *phandle);
void hal_canfd_uninit(canfd_handle *phandle);
void hal_canfd_tx_frame(canfd_handle *phandle, canfd_msg_t * msg);
void hal_canfd_rx_frame(u32 reg_base, canfd_msg_t *msg);
#ifdef AIC_CANFD_GET_DATA_BY_DMA
void hal_canfd_config_dma_tx(canfd_handle *phandle);
void hal_canfd_config_dma_rx(canfd_handle *phandle);
void hal_canfd_start_dma(canfd_handle *phandle);
void hal_canfd_stop_dma(canfd_handle *phandle);
void hal_canfd_set_dma_request(canfd_handle *phandle, u32 frame_data_len);
void hal_canfd_dma_rx_frame(u32 reg_base, void *buf, canfd_msg_t *msg);
#endif
int hal_canfd_ioctl(canfd_handle *phandle, int cmd, void *arg);
int hal_canfd_attach_callback(canfd_handle *phandle, void *callback, void *arg);
void hal_canfd_detach_callback(canfd_handle *phandle);
irqreturn_t hal_canfd_isr_handler(int irq_num, void *arg);
#endif
