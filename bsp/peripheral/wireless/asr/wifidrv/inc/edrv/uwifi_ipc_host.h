/**
 ******************************************************************************
 *
 * @file uwifi_ipc_host.h
 *
 * @brief IPC module.
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */
#ifndef _UWIFI_IPC_HOST_H_
#define _UWIFI_IPC_HOST_H_

/*
 * INCLUDE FILES
 ******************************************************************************
 */
#include <stdint.h>
#include "uwifi_ipc_shared.h"
/*
#define BITS_ER_LONG    32
#define BIT_MASK(nr)    (1 << ((nr)%BITS_PER_LONG))
#define BIT_WORD(nr)    ((nr)/BITS_PER_LONG)

void set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

    asr_rtos_enter_critical();
    *p |= mask;
    asr_rtos_exit_critical();
}

void clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

    asr_rtos_enter_critical();
    *p &= ~mask;
    asr_rtos_exit_critical();
}


void int test_and_set_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long old;
    unsigned long flags;

    asr_rtos_enter_critical();
    old = *p;
    *p = old | mask;
    asr_rtos_exit_critical();
    return (old & mask) != 0;
}

void int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
    unsigned long old;
    unsigned long flags;

    asr_rtos_enter_critical();
    old = *p;
    *p = (old & ~mask);
    asr_rtos_exit_critical();

    return (old & mask) != 0;
}
*/
/*
 * ENUMERATION
 ******************************************************************************
 */

enum ipc_host_desc_status
{
    /// Descriptor is IDLE
    IPC_HOST_DESC_IDLE      = 0,
    /// Data can be forwarded
    IPC_HOST_DESC_FORWARD,
    /// Data has to be kept in UMAC memory
    IPC_HOST_DESC_KEEP,
    /// Delete stored packet
    IPC_HOST_DESC_DELETE,
    /// Update Frame Length status
    IPC_HOST_DESC_LEN_UPDATE,
};

/**
 ******************************************************************************
 * @brief This structure is used to initialize the MAC SW
 *
 * The WLAN device driver provides functions call-back with this structure
 ******************************************************************************
 */
struct ipc_host_cb_tag
{
    /// WLAN driver call-back function: send_data_cfm
    int (*send_data_cfm)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_data_ind
    uint8_t (*recv_data_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_radar_ind
    uint8_t (*recv_radar_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_msg_ind
    uint8_t (*recv_msg_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_msgack_ind
    uint8_t (*recv_msgack_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_dbg_ind
    uint8_t (*recv_dbg_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: prim_tbtt_ind
    void (*prim_tbtt_ind)(void *pthis);

    /// WLAN driver call-back function: sec_tbtt_ind
    void (*sec_tbtt_ind)(void *pthis);

};

/*
 * Struct used to store information about host buffers (DMA Address and local pointer)
 */
struct ipc_hostbuf
{
    void    *hostid;     ///< ptr to hostbuf client (ipc_host client) structure
    uint32_t dma_addr;   ///< ptr to real hostbuf dma address
};

/// Definition of the IPC Host environment structure.
struct ipc_host_env_tag
{
    /// Structure containing the callback pointers
    struct ipc_host_cb_tag cb;

    /// Pointer to the shared environment
    struct ipc_shared_env_tag *shared;

    // Array used to store the descriptor addresses
    //struct ipc_hostbuf ipc_host_rxdesc_array[IPC_RXDESC_CNT];
    // Index of the host RX descriptor array (ipc_shared environment)
    uint8_t ipc_host_rxdesc_idx;
    /// Store the number of RX Descriptors
    uint8_t rxdesc_nb;

    /// Fields for Data Rx handling
    // Index used for ipc_host_rxbuf_array to point to current buffer
    uint8_t ipc_host_rxbuf_idx;
    // Store the number of Rx Data buffers
    uint32_t rx_bufnb;
    // Store the size of the Rx Data buffers
    uint32_t rx_bufsz;

    // Store the number of Rx Msg buffers
    uint32_t rx_msgbufnb;
    // Store the size of the Rx Msg buffers
    uint32_t rx_msgbufsz;

    // Index used that points to the first free TX desc
    uint32_t txdesc_free_idx[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    // Index used that points to the first used TX desc
    uint32_t txdesc_used_idx[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    // Array storing the currently pushed host ids for the BK queue
    void *tx_host_id0[CONFIG_USER_MAX][NX_TXDESC_CNT0];
    // Array storing the currently pushed host ids for the BE queue
    void *tx_host_id1[CONFIG_USER_MAX][NX_TXDESC_CNT1];
    // Array storing the currently pushed host ids for the VI queue
    void *tx_host_id2[CONFIG_USER_MAX][NX_TXDESC_CNT2];
    // Array storing the currently pushed host ids for the VO queue
    void *tx_host_id3[CONFIG_USER_MAX][NX_TXDESC_CNT3];
    // Array storing the currently pushed host ids for the BCN queue
    void *tx_host_id4[1][NX_TXDESC_CNT4];
    // Pointer to the different host ids arrays, per IPC queue
    void **tx_host_id[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    // Pointer to the different TX descriptor arrays, per IPC queue
    volatile struct txdesc_host *txdesc[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];

    /// Fields for Emb->App MSGs handling
    // Global array used to store the hostid and hostbuf addresses for msg/ind
    struct ipc_hostbuf ipc_host_msgbuf_array[IPC_MSGE2A_BUF_CNT];
    // Index of the MSG E2A buffers array to point to current buffer
    uint8_t ipc_host_msge2a_idx;
    // Store the number of E2A MSG buffers
    uint32_t ipc_e2amsg_bufnb;
    // Store the size of the E2A MSG buffers
    uint32_t ipc_e2amsg_bufsz;

    /// E2A ACKs of A2E MSGs
    uint8_t msga2e_cnt;
    void *msga2e_hostid;

    #ifdef CFG_AMSDU_TEST
    // Store the number of Rx Data buffers
    uint32_t rx_bufnb_split;
    // Store the size of the Rx Data buffers
    uint32_t rx_bufsz_split;
    #endif

    #ifdef SDIO_DEAGGR
    // Store the number of Rx Data buffers
    uint32_t rx_bufnb_sdio_deagg;
    // Store the size of the Rx Data buffers
    uint32_t rx_bufsz_sdio_deagg;
    #endif

    /// Pointer to the attached object (used in callbacks and register accesses)
    void *pthis;
};

extern const int nx_txdesc_cnt[];
extern const int nx_txuser_cnt[];

/**
 ******************************************************************************
 * @brief Initialize the IPC running on the Application CPU.
 *
 * This function:
 *   - initializes the IPC software environments
 *   - enables the interrupts in the IPC block
 *
 * @param[in]   env   Pointer to the IPC host environment
 *
 * @warning Since this function resets the IPC Shared memory, it must be called
 * before the LMAC FW is launched because LMAC sets some init values in IPC
 * Shared memory at boot.
 *
 ******************************************************************************
 */
void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb,
                  void *pthis);

/**
 ******************************************************************************
 * @brief Retrieve a new free Tx descriptor (host side).
 *
 * This function returns a pointer to the next Tx descriptor available from the
 * queue queue_idx to the host driver. The driver will have to fill it with the
 * appropriate endianness and to send it to the
 * emb side with ipc_host_txdesc_push().
 *
 * This function should only be called once until ipc_host_txdesc_push() is called.
 *
 * This function will return NULL if the queue is full.
 *
 * @param[in]   env   Pointer to the IPC host environment
 * @param[in]   queue_idx   Queue index. The index can be inferred from the
 *                          user priority of the incoming packet.
 * @param[in]   user_pos    User position. If MU-MIMO is not used, this value
 *                          shall be 0.
 * @return                  Pointer to the next Tx descriptor free. This can
 *                          point to the host memory or to shared memory,
 *                          depending on IPC implementation.
 *
 ******************************************************************************
 */
volatile struct txdesc_host *ipc_host_txdesc_get(struct ipc_host_env_tag *env,
                                                 const int queue_idx,
                                                 const int user_pos);

/**
 ******************************************************************************
 * @brief Push a filled Tx descriptor (host side).
 *
 * This function sets the next Tx descriptor available by the host side:
 * - as used for the host side
 * - as available for the emb side.
 * The Tx descriptor must be correctly filled before calling this function.
 *
 * This function may trigger an IRQ to the emb CPU depending on the interrupt
 * mitigation policy and on the push count.
 *
 * @param[in]   env   Pointer to the IPC host environment
 * @param[in]   queue_idx   Queue index. Same value than ipc_host_txdesc_get()
 * @param[in]   user_pos    User position. If MU-MIMO is not used, this value
 *                          shall be 0.
 * @param[in]   host_id     Parameter indicated by the IPC at TX confirmation,
 *                          that allows the driver finding the buffer
 *
 ******************************************************************************
 */
void ipc_host_txdesc_push(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void *host_id);

/**
 ******************************************************************************
 * @brief Check if there are TX frames pending in the TX queues.
 *
 * @param[in]   env   Pointer to the IPC host environment
 *
 * @return true if there are frames pending, false otherwise.
 *
 ******************************************************************************
 */
bool ipc_host_tx_frames_pending(struct ipc_host_env_tag *env);

/**
 ******************************************************************************
 * @brief Get and flush a packet from the IPC queue passed as parameter.
 *
 * @param[in]   env        Pointer to the IPC host environment
 * @param[in]   queue_idx  Index of the queue to flush
 * @param[in]   user_pos   User position to flush
 *
 * @return The flushed hostid if there is one, 0 otherwise.
 *
 ******************************************************************************
 */
void *ipc_host_tx_flush(struct ipc_host_env_tag *env, const int queue_idx,
                        const int user_pos);

/** @addtogroup IPC_RX
 *  @{
 */
void ipc_host_patt_addr_push(struct ipc_host_env_tag *env, uint32_t addr);

/**
 ******************************************************************************
 * @brief Push a pre-allocated buffer descriptor for Rx packet (host side)
 *
 * This function should be called by the host IRQ handler to supply the
 * embedded side with new empty buffer.
 *
 * @param[in]   env         Pointer to the IPC host environment
 * @param[in]   hostid      Packet ID used by the host (skbuff pointer on Linux)
 * @param[in]   hostbuf     Pointer to the start of the buffer payload in the
 *                          host memory (this may be inferred from the skbuff?)
 *                          The length of this buffer should be predefined
 *                          between host and emb statically (constant needed?).
 *
 ******************************************************************************
 */
int ipc_host_rxbuf_push(struct ipc_host_env_tag *env,
                        uint32_t hostid,
                        uint32_t hostbuf);

/**
 ******************************************************************************
 * @brief Push a pre-allocated Descriptor
 *
 * This function should be called by the host IRQ handler to supply the
 * embedded side with new empty buffer.
 *
 * @param[in]   env         Pointer to the IPC host environment
 * @param[in]   hostid      Address of packet for host
 * @param[in]   hostbuf     Pointer to the start of the buffer payload in the
 *                          host memory. The length of this buffer should be
 *                          predefined between host and emb statically.
 *
 ******************************************************************************
 */
int ipc_host_rxdesc_push(struct ipc_host_env_tag *env, void *hostid,
                         uint32_t hostbuf);

/**
 ******************************************************************************
 * @brief Push a pre-allocated buffer descriptor for IPC MSGs (host side)
 *
 * This function is called at Init time to initialize all Emb2App messages
 * buffers. Then each time embedded send a IPC message, this function is used
 * to push back the same buffer once it has been handled.
 *
 * @param[in]   env         Pointer to the IPC host environment
 * @param[in]   hostid      Address of buffer for host
 * @param[in]   hostbuf     Address of buffer for embedded
 *                          The length of this buffer should be predefined
 *                          between host and emb statically.
 *
 ******************************************************************************
 */
int ipc_host_msgbuf_push(struct ipc_host_env_tag *env, void *hostid,
                         uint32_t hostbuf);

int asr_process_int_status(struct ipc_host_env_tag *env);

#endif // _UWIFI_IPC_HOST_H_