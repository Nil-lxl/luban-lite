/**
 ****************************************************************************************
 * @file dbg.h
 *
 * @brief Declaration of the Debug module environment.
 *
 * Copyright (C) ASR 2011-2016
 *
 ****************************************************************************************
 */
#ifndef _DBG_H_
#define _DBG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 #include "asr_config.h"
//#include "reg_mac_pl.h"
//#include "dbg_profiling.h"
//#include "la.h"
//#include "la_mem.h"
#include "phy.h"
//#include "trace.h"
#if 1//NX_PRINT temp debug
#include <stdio.h>
#include "asr_rtos.h"
#endif
#include "asr_types.h"
/**
 ****************************************************************************************
 * @defgroup DEBUG DEBUG
 * @ingroup MACSW
 * @brief Description of the Debug module.
 * @{
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup DBG_PRINT DBG_PRINT
 * @ingroup DEBUG
 * @brief Print Debug feature
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/**
 * @name Debug Print definitions
 * @{
 ****************************************************************************************
 */

uint32_t dbg_snprintf(char *buffer, uint32_t size, const char *fmt, ...);

/**
 * @addtogroup DBG_PROF
 * @{
 * @name Profiling Macro definitions
 * @{
 ****************************************************************************************
 */
/// Profiling macro
#ifdef TX_IPC_IRQ_DEF
#define PROF_TX_IPC_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_IPC_IRQ))
#define PROF_TX_IPC_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_IPC_IRQ))
#else
#define PROF_TX_IPC_IRQ_SET()
#define PROF_TX_IPC_IRQ_CLR()
#endif

#ifdef TX_IPC_EVT_DEF
#define PROF_TX_IPC_EVT_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_IPC_EVT))
#define PROF_TX_IPC_EVT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_IPC_EVT))
#else
#define PROF_TX_IPC_EVT_SET()
#define PROF_TX_IPC_EVT_CLR()
#endif

#ifdef TX_DMA_IRQ_DEF
#define PROF_TX_DMA_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_DMA_IRQ))
#define PROF_TX_DMA_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_DMA_IRQ))
#else
#define PROF_TX_DMA_IRQ_SET()
#define PROF_TX_DMA_IRQ_CLR()
#endif

#ifdef TX_NEW_TAIL_DEF
#define PROF_TX_NEW_TAIL_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_NEW_TAIL))
#define PROF_TX_NEW_TAIL_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_NEW_TAIL))
#else
#define PROF_TX_NEW_TAIL_SET()
#define PROF_TX_NEW_TAIL_CLR()
#endif

#ifdef AGG_FIRST_MPDU_DWNLD_DEF
#define PROF_AGG_FIRST_MPDU_DWNLD_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_FIRST_MPDU_DWNLD))
#define PROF_AGG_FIRST_MPDU_DWNLD_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_FIRST_MPDU_DWNLD))
#else
#define PROF_AGG_FIRST_MPDU_DWNLD_SET()
#define PROF_AGG_FIRST_MPDU_DWNLD_CLR()
#endif

#ifdef AGG_START_AMPDU_DEF
#define PROF_AGG_START_AMPDU_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_START_AMPDU))
#define PROF_AGG_START_AMPDU_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_START_AMPDU))
#else
#define PROF_AGG_START_AMPDU_SET()
#define PROF_AGG_START_AMPDU_CLR()
#endif

#ifdef AGG_ADD_MPDU_DEF
#define PROF_AGG_ADD_MPDU_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_ADD_MPDU))
#define PROF_AGG_ADD_MPDU_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_ADD_MPDU))
#else
#define PROF_AGG_ADD_MPDU_SET()
#define PROF_AGG_ADD_MPDU_CLR()
#endif

#ifdef AGG_FINISH_AMPDU_DEF
#define PROF_AGG_FINISH_AMPDU_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_FINISH_AMPDU))
#define PROF_AGG_FINISH_AMPDU_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_FINISH_AMPDU))
#else
#define PROF_AGG_FINISH_AMPDU_SET()
#define PROF_AGG_FINISH_AMPDU_CLR()
#endif

#ifdef AGG_SMPDU_DONETX_DEF
#define PROF_AGG_SMPDU_DONETX_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_SMPDU_DONETX))
#define PROF_AGG_SMPDU_DONETX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_SMPDU_DONETX))
#else
#define PROF_AGG_SMPDU_DONETX_SET()
#define PROF_AGG_SMPDU_DONETX_CLR()
#endif

#ifdef AGG_BAR_DONETX_DEF
#define PROF_AGG_BAR_DONETX_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_BAR_DONETX))
#define PROF_AGG_BAR_DONETX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_BAR_DONETX))
#else
#define PROF_AGG_BAR_DONETX_SET()
#define PROF_AGG_BAR_DONETX_CLR()
#endif

#ifdef AGG_BA_RXED_DEF
#define PROF_AGG_BA_RXED_SET()    nxmac_sw_set_profiling_set(CO_BIT(AGG_BA_RXED))
#define PROF_AGG_BA_RXED_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AGG_BA_RXED))
#else
#define PROF_AGG_BA_RXED_SET()
#define PROF_AGG_BA_RXED_CLR()
#endif

#ifdef CHAN_CTXT_CDE_EVT_PROF_DEF
#define PROF_CHAN_CTXT_CDE_EVT_SET()    nxmac_sw_set_profiling_set(CO_BIT(CHAN_CTXT_CDE_EVT))
#define PROF_CHAN_CTXT_CDE_EVT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CHAN_CTXT_CDE_EVT))
#else
#define PROF_CHAN_CTXT_CDE_EVT_SET()
#define PROF_CHAN_CTXT_CDE_EVT_CLR()
#endif

#ifdef CHAN_CTXT_TBTT_SWITCH_PROF_DEF
#define PROF_CHAN_CTXT_TBTT_SWITCH_SET()    nxmac_sw_set_profiling_set(CO_BIT(CHAN_CTXT_TBTT_SWITCH))
#define PROF_CHAN_CTXT_TBTT_SWITCH_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CHAN_CTXT_TBTT_SWITCH))
#else
#define PROF_CHAN_CTXT_TBTT_SWITCH_SET()
#define PROF_CHAN_CTXT_TBTT_SWITCH_CLR()
#endif

#ifdef CHAN_CTXT_TX_DISCARD_PROF_DEF
#define PROF_CHAN_CTXT_TX_DISCARD_SET()    nxmac_sw_set_profiling_set(CO_BIT(CHAN_CTXT_TX_DISCARD))
#define PROF_CHAN_CTXT_TX_DISCARD_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CHAN_CTXT_TX_DISCARD))
#else
#define PROF_CHAN_CTXT_TX_DISCARD_SET()
#define PROF_CHAN_CTXT_TX_DISCARD_CLR()
#endif

/// Profiling macro
/// @param idx Channel Context index to be output
#ifdef CHAN_CTXT_IDX_PROF_DEF
#define PROF_CHAN_CTXT_IDX_SET(idx)                                                      \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x7 << CHAN_CTXT_IDX_PROF);             \
    nxmac_sw_profiling_set(prof | ((idx & 0x7) << CHAN_CTXT_IDX_PROF));                  \
}
#else
#define PROF_CHAN_CTXT_IDX_SET(idx)
#endif

#ifdef CHAN_CTXT_WAIT_END_PROF_DEF
#define PROF_CHAN_CTXT_WAIT_END_SET()    nxmac_sw_set_profiling_set(CO_BIT(CHAN_CTXT_WAIT_END))
#define PROF_CHAN_CTXT_WAIT_END_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CHAN_CTXT_WAIT_END))
#else
#define PROF_CHAN_CTXT_WAIT_END_SET()
#define PROF_CHAN_CTXT_WAIT_END_CLR()
#endif

#ifdef CHAN_CTXT_SWITCH_PROF_DEF
#define PROF_CHAN_CTXT_SWITCH_SET()    nxmac_sw_set_profiling_set(CO_BIT(CHAN_CTXT_SWITCH))
#define PROF_CHAN_CTXT_SWITCH_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CHAN_CTXT_SWITCH))
#else
#define PROF_CHAN_CTXT_SWITCH_SET()
#define PROF_CHAN_CTXT_SWITCH_CLR()
#endif

#ifdef CHAN_CTXT_TBTT_PRES_PROF_DEF
#define PROF_CHAN_CTXT_TBTT_PRES_SET()    nxmac_sw_set_profiling_set(CO_BIT(CHAN_CTXT_TBTT_PRES))
#define PROF_CHAN_CTXT_TBTT_PRES_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CHAN_CTXT_TBTT_PRES))
#else
#define PROF_CHAN_CTXT_TBTT_PRES_SET()
#define PROF_CHAN_CTXT_TBTT_PRES_CLR()
#endif

#ifdef P2P_NOA_ABS_DEF
#define PROF_P2P_NOA_ABS_SET(inst)    nxmac_sw_set_profiling_set(CO_BIT(P2P_NOA_0_ABS + inst))
#define PROF_P2P_NOA_ABS_CLR(inst)    nxmac_sw_clear_profiling_clear(CO_BIT(P2P_NOA_0_ABS + inst))
#else
#define PROF_P2P_NOA_ABS_SET(inst)
#define PROF_P2P_NOA_ABS_CLR(inst)
#endif
#ifdef P2P_CTW_DEF
#define PROF_P2P_CTW_SET()          nxmac_sw_set_profiling_set(CO_BIT(P2P_CTW))
#define PROF_P2P_CTW_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(P2P_CTW))
#else
#define PROF_P2P_CTW_SET()
#define PROF_P2P_CTW_CLR()
#endif
#ifdef P2P_WAIT_BCN_DEF
#define PROF_P2P_WAIT_BCN_SET()     nxmac_sw_set_profiling_set(CO_BIT(P2P_WAIT_BCN))
#define PROF_P2P_WAIT_BCN_CLR()     nxmac_sw_clear_profiling_clear(CO_BIT(P2P_WAIT_BCN))
#else
#define PROF_P2P_WAIT_BCN_SET()
#define PROF_P2P_WAIT_BCN_CLR()
#endif
#ifdef P2P_ABSENCE_DEF
#define PROF_P2P_ABSENCE_SET()     nxmac_sw_set_profiling_set(CO_BIT(P2P_ABSENCE))
#define PROF_P2P_ABSENCE_CLR()     nxmac_sw_clear_profiling_clear(CO_BIT(P2P_ABSENCE))
#else
#define PROF_P2P_ABSENCE_SET()
#define PROF_P2P_ABSENCE_CLR()
#endif
#ifdef P2P_PS_PAUSED_DEF
#define PROF_P2P_PS_PAUSED_SET()     nxmac_sw_set_profiling_set(CO_BIT(P2P_PS_PAUSED))
#define PROF_P2P_PS_PAUSED_CLR()     nxmac_sw_clear_profiling_clear(CO_BIT(P2P_PS_PAUSED))
#else
#define PROF_P2P_PS_PAUSED_SET()
#define PROF_P2P_PS_PAUSED_CLR()
#endif

#ifdef AGG_BA_REL_DEF
#define PROF_AGG_BA_REL_SET()       nxmac_sw_set_profiling_set(CO_BIT(AGG_BA_REL))
#define PROF_AGG_BA_REL_CLR()       nxmac_sw_clear_profiling_clear(CO_BIT(AGG_BA_REL))
#else
#define PROF_AGG_BA_REL_SET()
#define PROF_AGG_BA_REL_CLR()
#endif

#ifdef MU_SEC_USER_IRQ_DEF
#define PROF_MU_SEC_USER_IRQ_SET()       nxmac_sw_set_profiling_set(CO_BIT(MU_SEC_USER_IRQ))
#define PROF_MU_SEC_USER_IRQ_CLR()       nxmac_sw_clear_profiling_clear(CO_BIT(MU_SEC_USER_IRQ))
#else
#define PROF_MU_SEC_USER_IRQ_SET()
#define PROF_MU_SEC_USER_IRQ_CLR()
#endif
#ifdef MU_PPDU_START_DEF
#define PROF_MU_PPDU_START_SET()       nxmac_sw_set_profiling_set(CO_BIT(MU_PPDU_START))
#define PROF_MU_PPDU_START_CLR()       nxmac_sw_clear_profiling_clear(CO_BIT(MU_PPDU_START))
#else
#define PROF_MU_PPDU_START_SET()
#define PROF_MU_PPDU_START_CLR()
#endif
#ifdef MU_MPDU_ADD_DEF
#define PROF_MU_MPDU_ADD_SET()       nxmac_sw_set_profiling_set(CO_BIT(MU_MPDU_ADD))
#define PROF_MU_MPDU_ADD_CLR()       nxmac_sw_clear_profiling_clear(CO_BIT(MU_MPDU_ADD))
#else
#define PROF_MU_MPDU_ADD_SET()
#define PROF_MU_MPDU_ADD_CLR()
#endif
#ifdef MU_PPDU_CLOSE_DEF
#define PROF_MU_PPDU_CLOSE_SET()       nxmac_sw_set_profiling_set(CO_BIT(MU_PPDU_CLOSE))
#define PROF_MU_PPDU_CLOSE_CLR()       nxmac_sw_clear_profiling_clear(CO_BIT(MU_PPDU_CLOSE))
#else
#define PROF_MU_PPDU_CLOSE_SET()
#define PROF_MU_PPDU_CLOSE_CLR()
#endif
/// Profiling macro
/// @param pos MU user position to be output
#ifdef MU_USER_POS_DEF
#define PROF_MU_USER_POS_SET(pos)                                                        \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x3 << MU_USER_POS);                    \
    nxmac_sw_profiling_set(prof | (((pos) & 0x3) << MU_USER_POS));                       \
}
#else
#define PROF_MU_USER_POS_SET(pos)
#endif
/// Profiling macro
/// @param idx MU user index to be output
#ifdef MU_USER_POS_IRQ_DEF
#define PROF_MU_USER_POS_IRQ_SET(idx)                                                    \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x3 << MU_USER_POS_IRQ);                \
    nxmac_sw_profiling_set(prof | (((idx) & 0x3) << MU_USER_POS_IRQ));                   \
}
#else
#define PROF_MU_USER_POS_IRQ_SET(idx)
#endif

/// Profiling macro
/// @param ac Access Category to be output
#ifdef TX_AC_BG_DEF
#define PROF_TX_AC_BG_SET(ac)                                                            \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x3 << TX_AC_BG);                       \
    nxmac_sw_profiling_set(prof | (((ac) & 0x3) << TX_AC_BG));                           \
}
#else
#define PROF_TX_AC_BG_SET(ac)
#endif

/// Profiling macro
/// @param ac Access Category to be output
#ifdef TX_AC_IRQ_DEF
#define PROF_TX_AC_IRQ_SET(ac)                                                           \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x3 << TX_AC_IRQ);                      \
    nxmac_sw_profiling_set(prof | (((ac) & 0x3) << TX_AC_IRQ));                          \
}
#else
#define PROF_TX_AC_IRQ_SET(ac)
#endif

#ifdef TX_MAC_IRQ_DEF
#define PROF_TX_MAC_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_MAC_IRQ))
#define PROF_TX_MAC_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_MAC_IRQ))
#else
#define PROF_TX_MAC_IRQ_SET()
#define PROF_TX_MAC_IRQ_CLR()
#endif

#ifdef TX_BUF_FREE_DEF
#define PROF_TX_BUF_FREE_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_BUF_FREE))
#define PROF_TX_BUF_FREE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_BUF_FREE))
#else
#define PROF_TX_BUF_FREE_SET()
#define PROF_TX_BUF_FREE_CLR()
#endif

#ifdef TX_BUF_ALLOC_DEF
#define PROF_TX_BUF_ALLOC_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_BUF_ALLOC))
#define PROF_TX_BUF_ALLOC_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_BUF_ALLOC))
#else
#define PROF_TX_BUF_ALLOC_SET()
#define PROF_TX_BUF_ALLOC_CLR()
#endif

#ifdef TX_PAYL_HDL_DEF
#define PROF_TX_PAYL_HDL_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_PAYL_HDL))
#define PROF_TX_PAYL_HDL_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_PAYL_HDL))
#else
#define PROF_TX_PAYL_HDL_SET()
#define PROF_TX_PAYL_HDL_CLR()
#endif

#ifdef TX_CFM_EVT_DEF
#define PROF_TX_CFM_EVT_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_CFM_EVT))
#define PROF_TX_CFM_EVT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_CFM_EVT))
#else
#define PROF_TX_CFM_EVT_SET()
#define PROF_TX_CFM_EVT_CLR()
#endif

#ifdef TX_CFM_DMA_IRQ_DEF
#define PROF_TX_CFM_DMA_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(TX_CFM_DMA_IRQ))
#define PROF_TX_CFM_DMA_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TX_CFM_DMA_IRQ))
#else
#define PROF_TX_CFM_DMA_IRQ_SET()
#define PROF_TX_CFM_DMA_IRQ_CLR()
#endif

#ifdef MM_HW_IDLE_DEF
#define PROF_MM_HW_IDLE_SET()    nxmac_sw_set_profiling_set(CO_BIT(MM_HW_IDLE))
#define PROF_MM_HW_IDLE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MM_HW_IDLE))
#else
#define PROF_MM_HW_IDLE_SET()
#define PROF_MM_HW_IDLE_CLR()
#endif

#ifdef MM_SET_CHANNEL_DEF
#define PROF_MM_SET_CHANNEL_SET()    nxmac_sw_set_profiling_set(CO_BIT(MM_SET_CHANNEL))
#define PROF_MM_SET_CHANNEL_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MM_SET_CHANNEL))
#else
#define PROF_MM_SET_CHANNEL_SET()
#define PROF_MM_SET_CHANNEL_CLR()
#endif

#ifdef TX_FRAME_PUSH_DEF
#define PROF_TX_FRAME_PUSH_SET()     nxmac_sw_set_profiling_set(CO_BIT(TX_FRAME_PUSH))
#define PROF_TX_FRAME_PUSH_CLR()     nxmac_sw_clear_profiling_clear(CO_BIT(TX_FRAME_PUSH))
#else
#define PROF_TX_FRAME_PUSH_SET()
#define PROF_TX_FRAME_PUSH_CLR()
#endif

#ifdef TX_FRAME_CFM_DEF
#define PROF_TX_FRAME_CFM_SET()      nxmac_sw_set_profiling_set(CO_BIT(TX_FRAME_CFM))
#define PROF_TX_FRAME_CFM_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(TX_FRAME_CFM))
#else
#define PROF_TX_FRAME_CFM_SET()
#define PROF_TX_FRAME_CFM_CLR()
#endif

#ifdef MSG_BASIC_PROF_DEF
#define PROF_MSG_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(MSG_IRQ))
#define PROF_MSG_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MSG_IRQ))
#define PROF_MSG_FWD_SET()    nxmac_sw_set_profiling_set(CO_BIT(MSG_FWD))
#define PROF_MSG_FWD_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MSG_FWD))
#define PROF_MSG_IPC_IND_SET()    nxmac_sw_set_profiling_set(CO_BIT(MSG_IPC_IND))
#define PROF_MSG_IPC_IND_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MSG_IPC_IND))
#else
#define PROF_MSG_IRQ_SET()
#define PROF_MSG_IRQ_CLR()
#define PROF_MSG_FWD_SET()
#define PROF_MSG_FWD_CLR()
#define PROF_MSG_IPC_IND_SET()
#define PROF_MSG_IPC_IND_CLR()
#endif

#ifdef RX_MAC_IRQ_DEF
#define PROF_RX_MAC_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_MAC_IRQ))
#define PROF_RX_MAC_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_MAC_IRQ))
#else
#define PROF_RX_MAC_IRQ_SET()
#define PROF_RX_MAC_IRQ_CLR()
#endif

#ifdef RX_DMA_IRQ_DEF
#define PROF_RX_DMA_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_DMA_IRQ))
#define PROF_RX_DMA_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_DMA_IRQ))
#else
#define PROF_RX_DMA_IRQ_SET()
#define PROF_RX_DMA_IRQ_CLR()
#endif

#ifdef RX_DMA_EVT_DEF
#define PROF_RX_DMA_EVT_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_DMA_EVT))
#define PROF_RX_DMA_EVT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_DMA_EVT))
#else
#define PROF_RX_DMA_EVT_SET()
#define PROF_RX_DMA_EVT_CLR()
#endif

#ifdef RX_IPC_IND_DEF
#define PROF_RX_IPC_IND_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_IPC_IND))
#define PROF_RX_IPC_IND_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_IPC_IND))
#else
#define PROF_RX_IPC_IND_SET()
#define PROF_RX_IPC_IND_CLR()
#endif

#ifdef RX_MPDU_XFER_DEF
#define RX_MPDU_XFER_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_MPDU_XFER))
#define RX_MPDU_XFER_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_MPDU_XFER))
#else
#define RX_MPDU_XFER_SET()
#define RX_MPDU_XFER_CLR()
#endif

#ifdef RX_MPDU_FREE_DEF
#define RX_MPDU_FREE_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_MPDU_FREE))
#define RX_MPDU_FREE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_MPDU_FREE))
#else
#define RX_MPDU_FREE_SET()
#define RX_MPDU_FREE_CLR()
#endif

#ifdef RX_CNTRL_EVT_DEF
#define RX_CNTRL_EVT_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_CNTRL_EVT))
#define RX_CNTRL_EVT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_CNTRL_EVT))
#else
#define RX_CNTRL_EVT_SET()
#define RX_CNTRL_EVT_CLR()
#endif

#ifdef RX_HOSTBUF_IDX_DEF
#define RX_HOSTBUF_IDX_SET(val)    nxmac_sw_set_profiling_set(val<<(RX_HOSTBUF_IDX))
#define RX_HOSTBUF_IDX_CLR()       nxmac_sw_clear_profiling_clear(0x0F<<(RX_HOSTBUF_IDX))
#else
#define RX_HOSTBUF_IDX_SET(val)
#define RX_HOSTBUF_IDX_CLR()
#endif

#ifdef HW_TBTT_EVT_DEF
#define PROF_HW_TBTT_EVT_SET()    nxmac_sw_set_profiling_set(CO_BIT(HW_TBTT_EVT))
#define PROF_HW_TBTT_EVT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(HW_TBTT_EVT))
#else
#define PROF_HW_TBTT_EVT_SET()
#define PROF_HW_TBTT_EVT_CLR()
#endif

#ifdef BCN_PRIM_TBTT_IRQ_DEF
#define PROF_BCN_PRIM_TBTT_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(BCN_PRIM_TBTT_IRQ))
#define PROF_BCN_PRIM_TBTT_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BCN_PRIM_TBTT_IRQ))
#else
#define PROF_BCN_PRIM_TBTT_IRQ_SET()
#define PROF_BCN_PRIM_TBTT_IRQ_CLR()
#endif

#ifdef BCN_SEC_TBTT_IRQ_DEF
#define PROF_BCN_SEC_TBTT_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(BCN_SEC_TBTT_IRQ))
#define PROF_BCN_SEC_TBTT_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BCN_SEC_TBTT_IRQ))
#else
#define PROF_BCN_SEC_TBTT_IRQ_SET()
#define PROF_BCN_SEC_TBTT_IRQ_CLR()
#endif

#ifdef AP_TBTT_DEF
#define PROF_AP_TBTT_SET()    nxmac_sw_set_profiling_set(CO_BIT(AP_TBTT))
#define PROF_AP_TBTT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(AP_TBTT))
#else
#define PROF_AP_TBTT_SET()
#define PROF_AP_TBTT_CLR()
#endif

#ifdef STA_TBTT_DEF
#define PROF_STA_TBTT_SET()    nxmac_sw_set_profiling_set(CO_BIT(STA_TBTT))
#define PROF_STA_TBTT_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(STA_TBTT))
#else
#define PROF_STA_TBTT_SET()
#define PROF_STA_TBTT_CLR()
#endif

/// Profiling macro
/// @param idx TBTT index to be output
#ifdef TBTT_IDX_DEF
#define PROF_TBTT_IDX_SET(idx)                                                           \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x7 << TBTT_IDX_PROF);                  \
    nxmac_sw_profiling_set(prof | ((idx & 0x7) << TBTT_IDX_PROF));                       \
}
#else
#define PROF_TBTT_IDX_SET(idx)
#endif

#ifdef PS_SLEEP_DEF
#define PROF_PS_SLEEP_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_SLEEP))
#define PROF_PS_SLEEP_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_SLEEP))
#else
#define PROF_PS_SLEEP_SET()
#define PROF_PS_SLEEP_CLR()
#endif

#ifdef PS_PAUSE_DEF
#define PROF_PS_PAUSE_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_PAUSE))
#define PROF_PS_PAUSE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_PAUSE))
#else
#define PROF_PS_PAUSE_SET()
#define PROF_PS_PAUSE_CLR()
#endif

#ifdef PS_DPSM_UPDATE_DEF
#define PROF_PS_DPSM_UPDATE_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_DPSM_UPDATE))
#define PROF_PS_DPSM_UPDATE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_DPSM_UPDATE))
#else
#define PROF_PS_DPSM_UPDATE_SET()
#define PROF_PS_DPSM_UPDATE_CLR()
#endif

#ifdef PS_CHECK_RX_DEF
#define PROF_PS_CHECK_RX_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_CHECK_RX))
#define PROF_PS_CHECK_RX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_CHECK_RX))
#else
#define PROF_PS_CHECK_RX_SET()
#define PROF_PS_CHECK_RX_CLR()
#endif

#ifdef PS_CHECK_TX_DEF
#define PROF_PS_CHECK_TX_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_CHECK_TX))
#define PROF_PS_CHECK_TX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_CHECK_TX))
#else
#define PROF_PS_CHECK_TX_SET()
#define PROF_PS_CHECK_TX_CLR()
#endif

#ifdef PS_CHECK_BCN_DEF
#define PROF_PS_CHECK_BCN_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_CHECK_BCN))
#define PROF_PS_CHECK_BCN_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_CHECK_BCN))
#else
#define PROF_PS_CHECK_BCN_SET()
#define PROF_PS_CHECK_BCN_CLR()
#endif

#ifdef TD_CHECK_RX_DEF
#define PROF_TD_CHECK_RX_SET()    nxmac_sw_set_profiling_set(CO_BIT(TD_CHECK_RX))
#define PROF_TD_CHECK_RX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TD_CHECK_RX))
#else
#define PROF_TD_CHECK_RX_SET()
#define PROF_TD_CHECK_RX_CLR()
#endif

#ifdef TD_CHECK_TX_DEF
#define PROF_TD_CHECK_TX_SET()    nxmac_sw_set_profiling_set(CO_BIT(TD_CHECK_TX))
#define PROF_TD_CHECK_TX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TD_CHECK_TX))
#else
#define PROF_TD_CHECK_TX_SET()
#define PROF_TD_CHECK_TX_CLR()
#endif

#ifdef TD_CHECK_RX_PS_DEF
#define PROF_TD_CHECK_RX_PS_SET()    nxmac_sw_set_profiling_set(CO_BIT(TD_CHECK_RX_PS))
#define PROF_TD_CHECK_RX_PS_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TD_CHECK_RX_PS))
#else
#define PROF_TD_CHECK_RX_PS_SET()
#define PROF_TD_CHECK_RX_PS_CLR()
#endif

#ifdef TD_CHECK_TX_PS_DEF
#define PROF_TD_CHECK_TX_PS_SET()    nxmac_sw_set_profiling_set(CO_BIT(TD_CHECK_TX_PS))
#define PROF_TD_CHECK_TX_PS_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TD_CHECK_TX_PS))
#else
#define PROF_TD_CHECK_TX_PS_SET()
#define PROF_TD_CHECK_TX_PS_CLR()
#endif

#ifdef TD_TIMER_END_DEF
#define PROF_TD_TIMER_END_SET()    nxmac_sw_set_profiling_set(CO_BIT(TD_TIMER_END))
#define PROF_TD_TIMER_END_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(TD_TIMER_END))
#else
#define PROF_TD_TIMER_END_SET()
#define PROF_TD_TIMER_END_CLR()
#endif

#ifdef BFR_MU_CALIB_DEF
#define PROF_BFR_MU_CALIB_SET()    nxmac_sw_set_profiling_set(CO_BIT(BFR_MU_CALIB))
#define PROF_BFR_MU_CALIB_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BFR_MU_CALIB))
#else //(BFR_SU_CALIB_DEF)
#define PROF_BFR_MU_CALIB_SET()
#define PROF_BFR_MU_CALIB_CLR()
#endif //(BFR_SU_CALIB_DEF)

#ifdef BFR_SU_CALIB_DEF
#define PROF_BFR_SU_CALIB_SET()    nxmac_sw_set_profiling_set(CO_BIT(BFR_SU_CALIB))
#define PROF_BFR_SU_CALIB_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BFR_SU_CALIB))
#else //(BFR_SU_CALIB_DEF)
#define PROF_BFR_SU_CALIB_SET()
#define PROF_BFR_SU_CALIB_CLR()
#endif //(BFR_SU_CALIB_DEF)

#ifdef BFR_RX_BFR_DEF
#define PROF_BFR_RX_BFR_SET()      nxmac_sw_set_profiling_set(CO_BIT(BFR_RX_BFR))
#define PROF_BFR_RX_BFR_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(BFR_RX_BFR))
#else //(BFR_RX_BFR_DEF)
#define PROF_BFR_RX_BFR_SET()
#define PROF_BFR_RX_BFR_CLR()
#endif //(BFR_RX_BFR_DEF)

#ifdef BFR_UPLOAD_DEF
#define PROF_BFR_UPLOAD_SET()      nxmac_sw_set_profiling_set(CO_BIT(BFR_UPLOAD))
#define PROF_BFR_UPLOAD_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(BFR_UPLOAD))
#else //(BFR_UPLOAD_DEF)
#define PROF_BFR_UPLOAD_SET()
#define PROF_BFR_UPLOAD_CLR()
#endif //(BFR_UPLOAD_DEF)

#ifdef BFR_DOWNLOAD_DEF
#define PROF_BFR_DOWNLOAD_SET()    nxmac_sw_set_profiling_set(CO_BIT(BFR_DOWNLOAD))
#define PROF_BFR_DOWNLOAD_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BFR_DOWNLOAD))
#else //(BFR_DOWNLOAD_DEF)
#define PROF_BFR_DOWNLOAD_SET()
#define PROF_BFR_DOWNLOAD_CLR()
#endif //(BFR_DOWNLOAD_DEF)

#ifdef BFR_TX_IND_DEF
#define PROF_BFR_TX_IND_SET()      nxmac_sw_set_profiling_set(CO_BIT(BFR_TX_IND))
#define PROF_BFR_TX_IND_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(BFR_TX_IND))
#else //(BFR_TX_IND_DEF)
#define PROF_BFR_TX_IND_SET()
#define PROF_BFR_TX_IND_CLR()
#endif //(BFR_TX_IND_DEF)

#ifdef BFR_TX_CFM_DEF
#define PROF_BFR_TX_CFM_SET()      nxmac_sw_set_profiling_set(CO_BIT(BFR_TX_CFM))
#define PROF_BFR_TX_CFM_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(BFR_TX_CFM))
#else //(BFR_TX_CFM_DEF)
#define PROF_BFR_TX_CFM_SET()
#define PROF_BFR_TX_CFM_CLR()
#endif //(BFR_TX_CFM_DEF)

/// Profiling macro
/// @param idx SMM to be output
#ifdef BFR_SMM_IDX_DEF
#define PROF_SMM_IDX_SET(idx)                                                            \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0xF << BFR_SMM_IDX_0);                  \
    nxmac_sw_profiling_set(prof | ((idx & 0xF) << BFR_SMM_IDX_0));                       \
}
#else //(BFR_SMM_IDX_DEF)
#define PROF_SMM_IDX_SET(idx)
#endif //(BFR_SMM_IDX_DEF)

#ifdef RX_DUPLI_CHECK_DEF
#define PROF_RX_DUPLI_CHECK_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_DUPLI_CHECK))
#define PROF_RX_DUPLI_CHECK_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_DUPLI_CHECK))
#else
#define PROF_RX_DUPLI_CHECK_SET()
#define PROF_RX_DUPLI_CHECK_CLR()
#endif

#ifdef RX_DUPLI_NSTA_CHECK_DEF
#define PROF_RX_DUPLI_NSTA_CHECK_SET()    nxmac_sw_set_profiling_set(CO_BIT(RX_DUPLI_NSTA_CHECK))
#define PROF_RX_DUPLI_NSTA_CHECK_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RX_DUPLI_NSTA_CHECK))
#else
#define PROF_RX_DUPLI_NSTA_CHECK_SET()
#define PROF_RX_DUPLI_NSTA_CHECK_CLR()
#endif

#ifdef IPCDESC_PREPARE_DEF
#define PROF_IPCDESC_PREPARE_SET()    nxmac_sw_set_profiling_set(CO_BIT(IPCDESC_PREPARE))
#define PROF_IPCDESC_PREPARE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(IPCDESC_PREPARE))
#else
#define PROF_IPCDESC_PREPARE_SET()
#define PROF_IPCDESC_PREPARE_CLR()
#endif

#ifdef IPCDESC_TRANSFER_DEF
#define PROF_IPCDESC_TRANSFER_SET()    nxmac_sw_set_profiling_set(CO_BIT(IPCDESC_TRANSFER))
#define PROF_IPCDESC_TRANSFER_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(IPCDESC_TRANSFER))
#else
#define PROF_IPCDESC_TRANSFER_SET()
#define PROF_IPCDESC_TRANSFER_CLR()
#endif

#ifdef DEFRAG_CHECK_DEF
#define PROF_DEFRAG_CHECK_SET()    nxmac_sw_set_profiling_set(CO_BIT(DEFRAG_CHECK))
#define PROF_DEFRAG_CHECK_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(DEFRAG_CHECK))
#else
#define PROF_DEFRAG_CHECK_SET()
#define PROF_DEFRAG_CHECK_CLR()
#endif

#ifdef DEFRAG_TRANSFER_DEF
#define PROF_DEFRAG_TRANSFER_SET()    nxmac_sw_set_profiling_set(CO_BIT(DEFRAG_TRANSFER))
#define PROF_DEFRAG_TRANSFER_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(DEFRAG_TRANSFER))
#else
#define PROF_DEFRAG_TRANSFER_SET()
#define PROF_DEFRAG_TRANSFER_CLR()
#endif

#ifdef DEFRAG_UPD_LENGTH_DEF
#define PROF_DEFRAG_UPD_LENGTH_SET()    nxmac_sw_set_profiling_set(CO_BIT(DEFRAG_UPD_LENGTH))
#define PROF_DEFRAG_UPD_LENGTH_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(DEFRAG_UPD_LENGTH))
#else
#define PROF_DEFRAG_UPD_LENGTH_SET()
#define PROF_DEFRAG_UPD_LENGTH_CLR()
#endif

#ifdef REORD_CHECK_DEF
#define PROF_REORD_CHECK_SET()    nxmac_sw_set_profiling_set(CO_BIT(REORD_CHECK))
#define PROF_REORD_CHECK_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(REORD_CHECK))
#else
#define PROF_REORD_CHECK_SET()
#define PROF_REORD_CHECK_CLR()
#endif

#ifdef REORD_BAR_CHECK_DEF
#define PROF_REORD_BAR_CHECK_SET()    nxmac_sw_set_profiling_set(CO_BIT(REORD_BAR_CHECK))
#define PROF_REORD_BAR_CHECK_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(REORD_BAR_CHECK))
#else
#define PROF_REORD_BAR_CHECK_SET()
#define PROF_REORD_BAR_CHECK_CLR()
#endif

#ifdef REORD_FLUSH_DEF
#define PROF_REORD_FLUSH_SET()    nxmac_sw_set_profiling_set(CO_BIT(REORD_FLUSH))
#define PROF_REORD_FLUSH_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(REORD_FLUSH))
#else
#define PROF_REORD_FLUSH_SET()
#define PROF_REORD_FLUSH_CLR()
#endif

#ifdef REORD_FWD_DEF
#define PROF_REORD_FWD_SET()    nxmac_sw_set_profiling_set(CO_BIT(REORD_FWD))
#define PROF_REORD_FWD_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(REORD_FWD))
#else
#define PROF_REORD_FWD_SET()
#define PROF_REORD_FWD_CLR()
#endif

#ifdef MAC2ETH_UPDATE_DEF
#define PROF_MAC2ETH_UPDATE_SET()    nxmac_sw_set_profiling_set(CO_BIT(MAC2ETH_UPDATE))
#define PROF_MAC2ETH_UPDATE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MAC2ETH_UPDATE))
#else
#define PROF_MAC2ETH_UPDATE_SET()
#define PROF_MAC2ETH_UPDATE_CLR()
#endif

#ifdef PS_PEER_STATE_DEF
#define PROF_PS_PEER_STATE_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_PEER_STATE))
#define PROF_PS_PEER_STATE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_PEER_STATE))
#else
#define PROF_PS_PEER_STATE_SET()
#define PROF_PS_PEER_STATE_CLR()
#endif

#ifdef PS_BCMC_STATE_DEF
#define PROF_PS_BCMC_STATE_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_BCMC_STATE))
#define PROF_PS_BCMC_STATE_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_BCMC_STATE))
#else
#define PROF_PS_BCMC_STATE_SET()
#define PROF_PS_BCMC_STATE_CLR()
#endif

/// Profiling macro
/// @param val PS state to be output
#ifdef PS_STATE_VAL_DEF
#define PROF_PS_STATE_VAL_SET(val)                                                       \
{                                                                                        \
    uint32_t prof = nxmac_sw_profiling_get() & ~(0x3 << PS_STATE_VAL);                   \
    nxmac_sw_profiling_set(prof | ((val & 0x3) << PS_STATE_VAL));                        \
}
#else
#define PROF_PS_STATE_VAL_SET(val)
#endif

#ifdef PS_PSPOLL_RX_DEF
#define PROF_PS_PSPOLL_RX_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_PSPOLL_RX))
#define PROF_PS_PSPOLL_RX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_PSPOLL_RX))
#else
#define PROF_PS_PSPOLL_RX_SET()
#define PROF_PS_PSPOLL_RX_CLR()
#endif

#ifdef PS_TRAFFIC_REQ_DEF
#define PROF_PS_TRAFFIC_REQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(PS_TRAFFIC_REQ))
#define PROF_PS_TRAFFIC_REQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(PS_TRAFFIC_REQ))
#else
#define PROF_PS_TRAFFIC_REQ_SET()
#define PROF_PS_TRAFFIC_REQ_CLR()
#endif

#ifdef BW_DROP_IRQ_DEF
#define PROF_BW_DROP_IRQ_SET()    nxmac_sw_set_profiling_set(CO_BIT(BW_DROP_IRQ))
#define PROF_BW_DROP_IRQ_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BW_DROP_IRQ))
#else
#define PROF_BW_DROP_IRQ_SET()
#define PROF_BW_DROP_IRQ_CLR()
#endif

#ifdef BW_DROP_STEP_DEF
#define PROF_BW_DROP_STEP_SET()    nxmac_sw_set_profiling_set(CO_BIT(BW_DROP_STEP))
#define PROF_BW_DROP_STEP_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(BW_DROP_STEP))
#else
#define PROF_BW_DROP_STEP_SET()
#define PROF_BW_DROP_STEP_CLR()
#endif

#ifdef RC_UPD_COUNTERS_DEF
#define PROF_RC_UPD_COUNTERS_SET()    nxmac_sw_set_profiling_set(CO_BIT(RC_UPD_COUNTERS))
#define PROF_RC_UPD_COUNTERS_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RC_UPD_COUNTERS))
#else
#define PROF_RC_UPD_COUNTERS_SET()
#define PROF_RC_UPD_COUNTERS_CLR()
#endif

#ifdef RC_STATS_CALC_DEF
#define PROF_RC_STATS_CALC_SET()    nxmac_sw_set_profiling_set(CO_BIT(RC_STATS_CALC))
#define PROF_RC_STATS_CALC_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RC_STATS_CALC))
#else
#define PROF_RC_STATS_CALC_SET()
#define PROF_RC_STATS_CALC_CLR()
#endif

#ifdef RC_UPD_RETRY_CHAIN_DEF
#define PROF_RC_UPD_RETRY_CHAIN_SET()    nxmac_sw_set_profiling_set(CO_BIT(RC_UPD_RETRY_CHAIN))
#define PROF_RC_UPD_RETRY_CHAIN_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RC_UPD_RETRY_CHAIN))
#else
#define PROF_RC_UPD_RETRY_CHAIN_SET()
#define PROF_RC_UPD_RETRY_CHAIN_CLR()
#endif

#ifdef RC_LOOKAROUND_TX_DEF
#define PROF_RC_LOOKAROUND_TX_SET()    nxmac_sw_set_profiling_set(CO_BIT(RC_LOOKAROUND_TX))
#define PROF_RC_LOOKAROUND_TX_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RC_LOOKAROUND_TX))
#else
#define PROF_RC_LOOKAROUND_TX_SET()
#define PROF_RC_LOOKAROUND_TX_CLR()
#endif

#ifdef RC_LOOKAROUND_RV_DEF
#define PROF_RC_LOOKAROUND_RV_SET()    nxmac_sw_set_profiling_set(CO_BIT(RC_LOOKAROUND_RV))
#define PROF_RC_LOOKAROUND_RV_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(RC_LOOKAROUND_RV))
#else
#define PROF_RC_LOOKAROUND_RV_SET()
#define PROF_RC_LOOKAROUND_RV_CLR()
#endif

#ifdef RC_SW_RETRY_DEF
#define PROF_RC_SW_RETRY_SET()         nxmac_sw_set_profiling_set(CO_BIT(RC_SW_RETRY))
#define PROF_RC_SW_RETRY_CLR()         nxmac_sw_clear_profiling_clear(CO_BIT(RC_SW_RETRY))
#else
#define PROF_RC_SW_RETRY_SET()
#define PROF_RC_SW_RETRY_CLR()
#endif

#ifdef FT_OVER_DS_REQ_DEF
#define PROF_FT_OVER_DS_REQ_SET()      nxmac_sw_set_profiling_set(CO_BIT(FT_OVER_DS_REQ))
#define PROF_FT_OVER_DS_REQ_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(FT_OVER_DS_REQ))
#else
#define PROF_FT_OVER_DS_REQ_SET()
#define PROF_FT_OVER_DS_REQ_CLR()
#endif

#ifdef FT_REASSOC_DEF
#define PROF_FT_REASSOC_SET()          nxmac_sw_set_profiling_set(CO_BIT(FT_REASSOC))
#define PROF_FT_REASSOC_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(FT_REASSOC))
#else
#define PROF_FT_REASSOC_SET()
#define PROF_FT_REASSOC_CLR()
#endif

#ifdef MESH_PS_ENABLE_DEF
#define PROF_MESH_PS_ENABLE_SET()      nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_ENABLE))
#define PROF_MESH_PS_ENABLE_CLR()      nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_ENABLE))
#else
#define PROF_MESH_PS_ENABLE_SET()
#define PROF_MESH_PS_ENABLE_CLR()
#endif //(MESH_PS_ENABLE_DEF)

#ifdef MESH_PS_SP_OWNER_DEF
#define PROF_MESH_PS_SP_OWNER_SET()    nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_SP_OWNER))
#define PROF_MESH_PS_SP_OWNER_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_SP_OWNER))
#else
#define PROF_MESH_PS_SP_OWNER_SET()
#define PROF_MESH_PS_SP_OWNER_CLR()
#endif //(MESH_PS_SP_OWNER_DEF)

#ifdef MESH_PS_SP_RECIP_DEF
#define PROF_MESH_PS_SP_RECIP_SET()    nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_SP_RECIP))
#define PROF_MESH_PS_SP_RECIP_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_SP_RECIP))
#else
#define PROF_MESH_PS_SP_RECIP_SET()
#define PROF_MESH_PS_SP_RECIP_CLR()
#endif //(MESH_PS_SP_RECIP_DEF)

#ifdef MESH_PS_WAIT_BCMC_DEF
#define PROF_MESH_PS_WAIT_BCMC_SET()   nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_WAIT_BCMC))
#define PROF_MESH_PS_WAIT_BCMC_CLR()   nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_WAIT_BCMC))
#else
#define PROF_MESH_PS_WAIT_BCMC_SET()
#define PROF_MESH_PS_WAIT_BCMC_CLR()
#endif //(MESH_PS_WAIT_BCMC_DEF)

#ifdef MESH_PS_WAIT_BCN_DEF
#define PROF_MESH_PS_WAIT_BCN_SET()    nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_WAIT_BCN))
#define PROF_MESH_PS_WAIT_BCN_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_WAIT_BCN))
#else
#define PROF_MESH_PS_WAIT_BCN_SET()
#define PROF_MESH_PS_WAIT_BCN_CLR()
#endif //(MESH_PS_WAIT_BCN_DEF)

#ifdef MESH_PS_LOCAL_MAW_DEF
#define PROF_MESH_PS_LOCAL_MAW_SET()   nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_LOCAL_MAW))
#define PROF_MESH_PS_LOCAL_MAW_CLR()   nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_LOCAL_MAW))
#else
#define PROF_MESH_PS_LOCAL_MAW_SET()
#define PROF_MESH_PS_LOCAL_MAW_CLR()
#endif //(MESH_PS_LOCAL_MAW_DEF)

#ifdef MESH_PS_PEER_MAW_DEF
#define PROF_MESH_PS_PEER_MAW_SET()    nxmac_sw_set_profiling_set(CO_BIT(MESH_PS_PEER_MAW))
#define PROF_MESH_PS_PEER_MAW_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(MESH_PS_PEER_MAW))
#else
#define PROF_MESH_PS_PEER_MAW_SET()
#define PROF_MESH_PS_PEER_MAW_CLR()
#endif //(MESH_PS_PEER_MAW_DEF)

#ifdef CPU_SLEEP_DEF
#define PROF_CPU_SLEEP_SET()    nxmac_sw_set_profiling_set(CO_BIT(CPU_SLEEP))
#define PROF_CPU_SLEEP_CLR()    nxmac_sw_clear_profiling_clear(CO_BIT(CPU_SLEEP))
#else
#define PROF_CPU_SLEEP_SET()
#define PROF_CPU_SLEEP_CLR()
#endif //CPU_SLEEP_DEF

#ifdef TDLS_CHSW_REQ_RX_DEF
#define PROF_TDLS_CHSW_REQ_RX_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_RX_CHSW_REQ))
#define PROF_TDLS_CHSW_REQ_RX_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_RX_CHSW_REQ))
#else
#define PROF_TDLS_CHSW_REQ_RX_SET()
#define PROF_TDLS_CHSW_REQ_RX_CLR()
#endif

#ifdef TDLS_CHSW_RESP_RX_DEF
#define PROF_TDLS_CHSW_RESP_RX_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_RX_CHSW_RESP))
#define PROF_TDLS_CHSW_RESP_RX_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_RX_CHSW_RESP))
#else
#define PROF_TDLS_CHSW_RESP_RX_SET()
#define PROF_TDLS_CHSW_RESP_RX_CLR()
#endif

#ifdef TDLS_CHSW_REQ_TX_DEF
#define PROF_TDLS_CHSW_REQ_TX_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_TX_CHSW_REQ))
#define PROF_TDLS_CHSW_REQ_TX_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_TX_CHSW_REQ))
#else
#define PROF_TDLS_CHSW_REQ_TX_SET()
#define PROF_TDLS_CHSW_REQ_TX_CLR()
#endif

#ifdef TDLS_CHSW_RESP_TX_DEF
#define PROF_TDLS_CHSW_RESP_TX_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_TX_CHSW_RESP))
#define PROF_TDLS_CHSW_RESP_TX_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_TX_CHSW_RESP))
#else
#define PROF_TDLS_CHSW_RESP_TX_SET()
#define PROF_TDLS_CHSW_RESP_TX_CLR()
#endif

#ifdef TDLS_SWITCH_TO_OFFCH_DEF
#define PROF_TDLS_SWITCH_TO_OFFCH_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_SWITCH_TO_OFFCH))
#define PROF_TDLS_SWITCH_TO_OFFCH_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_SWITCH_TO_OFFCH))
#else
#define PROF_TDLS_SWITCH_TO_OFFCH_SET()
#define PROF_TDLS_SWITCH_TO_OFFCH_CLR()
#endif

#ifdef TDLS_SWITCH_TO_BASECH_DEF
#define PROF_TDLS_SWITCH_TO_BASECH_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_SWITCH_TO_BASECH))
#define PROF_TDLS_SWITCH_TO_BASECH_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_SWITCH_TO_BASECH))
#else
#define PROF_TDLS_SWITCH_TO_BASECH_SET()
#define PROF_TDLS_SWITCH_TO_BASECH_CLR()
#endif

#ifdef TDLS_CHSW_TIME_TIMER_DEF
#define PROF_TDLS_CHSW_TIME_TIMER_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_CHSW_TIME_TIMER))
#define PROF_TDLS_CHSW_TIME_TIMER_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_CHSW_TIME_TIMER))
#else
#define PROF_TDLS_CHSW_TIME_TIMER_SET()
#define PROF_TDLS_CHSW_TIME_TIMER_CLR()
#endif

#ifdef TDLS_CHSW_TIMEOUT_TIMER_DEF
#define PROF_TDLS_CHSW_TIMEOUT_TIMER_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_CHSW_TIMEOUT_TIMER))
#define PROF_TDLS_CHSW_TIMEOUT_TIMER_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_CHSW_TIMEOUT_TIMER))
#else
#define PROF_TDLS_CHSW_TIMEOUT_TIMER_SET()
#define PROF_TDLS_CHSW_TIMEOUT_TIMER_CLR()
#endif

#ifdef TDLS_CHSW_REQ_TX_TIMER_DEF
#define PROF_TDLS_CHSW_REQ_TX_TIMER_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_CHSW_REQ_TX_TIMER))
#define PROF_TDLS_CHSW_REQ_TX_TIMER_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_CHSW_REQ_TX_TIMER))
#else
#define PROF_TDLS_CHSW_REQ_TX_TIMER_SET()
#define PROF_TDLS_CHSW_REQ_TX_TIMER_CLR()
#endif

#ifdef TDLS_DELAY_CHSW_DEF
#define PROF_TDLS_DELAY_CHSW_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_DELAY_CHSW))
#define PROF_TDLS_DELAY_CHSW_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_DELAY_CHSW))
#else
#define PROF_TDLS_DELAY_CHSW_SET()
#define PROF_TDLS_DELAY_CHSW_CLR()
#endif

#ifdef TDLS_CHSW_NULL_FRAME_TX_DEF
#define PROF_TDLS_CHSW_NULL_FRAME_TX_SET()          nxmac_sw_set_profiling_set(CO_BIT(TDLS_TX_CHSW_NULL_FRAME))
#define PROF_TDLS_CHSW_NULL_FRAME_TX_CLR()          nxmac_sw_clear_profiling_clear(CO_BIT(TDLS_TX_CHSW_NULL_FRAME))
#else
#define PROF_TDLS_CHSW_NULL_FRAME_TX_SET()
#define PROF_TDLS_CHSW_NULL_FRAME_TX_CLR()
#endif

/// @}
/// @}

/// Type of errors
enum
{
    /// Recoverable error, not requiring any action from Upper MAC
    DBG_ERROR_RECOVERABLE = 0,
    /// Fatal error, requiring Upper MAC to reset Lower MAC and HW and restart operation
    DBG_ERROR_FATAL
};

/// Maximum length of the SW diag trace
#define DBG_SW_DIAG_MAX_LEN   1024

/// Maximum length of the error trace
#define DBG_ERROR_TRACE_SIZE  256

/// Number of MAC diagnostic port banks
#define DBG_DIAGS_MAC_MAX     48

/// Number of PHY diagnostic port banks
#define DBG_DIAGS_PHY_MAX     32

/// Maximum size of the RX header descriptor information in the debug dump
#define DBG_RHD_MEM_LEN      (5 * 1024)

/// Maximum size of the RX buffer descriptor information in the debug dump
#define DBG_RBD_MEM_LEN      (5 * 1024)

/// Maximum size of the TX header descriptor information in the debug dump
#define DBG_THD_MEM_LEN      (10 * 1024)

/// Debug information forwarded to host when an error occurs
struct dbg_debug_info_tag
{
    /// Type of error (0: recoverable, 1: fatal)
    uint32_t error_type;
    /// Pointer to the first RX Header Descriptor chained to the MAC HW
    uint32_t rhd;
    /// Size of the RX header descriptor buffer
    uint32_t rhd_len;
    /// Pointer to the first RX Buffer Descriptor chained to the MAC HW
    uint32_t rbd;
    /// Size of the RX buffer descriptor buffer
    uint32_t rbd_len;
    /// Pointer to the first TX Header Descriptors chained to the MAC HW
    uint32_t thd[NX_TXQ_CNT];
    /// Size of the TX header descriptor buffer
    uint32_t thd_len[NX_TXQ_CNT];
    /// MAC HW diag configuration
    uint32_t hw_diag;
    /// Error message
    uint32_t error[DBG_ERROR_TRACE_SIZE/4];
    /// SW diag configuration length
    uint32_t sw_diag_len;
    /// SW diag configuration
    uint32_t sw_diag[DBG_SW_DIAG_MAX_LEN/4];
    /// PHY channel information
    struct phy_channel_info chan_info;
    /// Embedded LA configuration
#if NX_DEBUG_DUMP
    struct la_conf_tag la_conf;
#endif
    /// MAC diagnostic port state
    uint16_t diags_mac[DBG_DIAGS_MAC_MAX];
    /// PHY diagnostic port state
    uint16_t diags_phy[DBG_DIAGS_PHY_MAX];
    /// MAC HW RX Header descriptor pointer
    uint32_t rhd_hw_ptr;
    /// MAC HW RX Buffer descriptor pointer
    uint32_t rbd_hw_ptr;
};

/// Full debug dump that is forwarded to host in case of error
struct dbg_debug_dump_tag
{
    /// Debug information
    struct dbg_debug_info_tag dbg_info;

    /// RX header descriptor memory
    uint32_t rhd_mem[DBG_RHD_MEM_LEN/4];

    /// RX buffer descriptor memory
    uint32_t rbd_mem[DBG_RBD_MEM_LEN/4];

    /// TX header descriptor memory
    uint32_t thd_mem[NX_TXQ_CNT][DBG_THD_MEM_LEN/4];
#if NX_DEBUG_DUMP
    /// Logic analyzer memory
    uint32_t la_mem[sizeof(struct la_mem_format) * LA_MEM_LINE_COUNT / 4];
#endif
};


#if NX_SYS_STAT
/// Structure containing the parameters used for the DOZE/CPU statistics
struct dbg_sys_stats_tag
{
    /// Last time CPU sleep was started
    uint32_t cpu_sleep_start;
    /// Time spent in CPU sleep since last reset of the statistics
    uint32_t cpu_sleep_time;
    /// Last time DOZE was started
    uint32_t doze_start;
    /// Time spent in DOZE since last reset of the statistics
    uint32_t doze_time;
    /// Last time the statistics were reset
    uint32_t start_time;
};

/// Maximum system statistics measurement timer (around 4 minutes)
#define DBG_SYS_STAT_TIMEOUT      (4 * 60 * 1000 * TU_DURATION)
#endif



/**
 *****************************************************************************************
 * @brief Declaration of DEBUG environment.
 * Any module or task will retrieve the DEBUG environment on its own, it is
 * not passed anymore as a parameter to the function handlers
 * of the Debug task.  If an other module wants to make use of the
 * environment of debug, it simply needs to include this file and use
 * the extern global variable.
 *****************************************************************************************
 */
struct debug_env_tag
{
    /// Error trace to be reported to the host
    char        error[DBG_ERROR_TRACE_SIZE];
    /// User trace filter (bit set means traces enabled)
    uint32_t    filter_module;
    /// Severity of filter
    uint32_t    filter_severity;
    #if NX_SYS_STAT
    /// System statistics
    struct dbg_sys_stats_tag sys_stats;
    #endif
};

/// DEBUG module environment declaration.
extern struct debug_env_tag dbg_env;

/**
 * @name System Stats Macro definitions
 * @{
 ****************************************************************************************
 */
#if NX_SYS_STAT
#define DBG_CPU_SLEEP_START()                                                  \
{                                                                              \
    dbg_env.sys_stats.cpu_sleep_start = hal_machw_time();                      \
}

#define DBG_CPU_SLEEP_END()                                                    \
{                                                                              \
    uint32_t time = hal_machw_time();                                          \
    dbg_env.sys_stats.cpu_sleep_time += (time - dbg_env.sys_stats.cpu_sleep_start); \
}

#define DBG_DOZE_START()                                                       \
{                                                                              \
    dbg_env.sys_stats.doze_start = hal_machw_time();                           \
}

#define DBG_DOZE_END()                                                         \
{                                                                              \
    uint32_t time = hal_machw_time();                                          \
    dbg_env.sys_stats.doze_time += (time - dbg_env.sys_stats.doze_start);      \
}
#else
#define DBG_CPU_SLEEP_START()
#define DBG_CPU_SLEEP_END()
#define DBG_DOZE_START()
#define DBG_DOZE_END()
#endif

/** @} */

/**
 ****************************************************************************************
 * @brief Initialize the debug module
 ****************************************************************************************
 */
void dbg_init(void);

#if NX_DEBUG_DUMP
/**
 ****************************************************************************************
 * @brief Indicate a fatal error to the driver
 *
 * @param[in] msg    Null-terminated string describing the error
 * @param[in] type   Error type (0: recoverable, 1: fatal)
 ****************************************************************************************
 */
void dbg_error_ind(char *msg, uint32_t type);

/**
 ****************************************************************************************
 * @brief This function saves the error trace in the debug environment
 *
 * @param[in] error Error trace
 *
 ****************************************************************************************
 */
void dbg_error_save(const char *error);
#endif

#if NX_SYS_STAT
/**
 ****************************************************************************************
 * @brief Reset the system statistic measurement
 ****************************************************************************************
 */
void dbg_sys_stat_reset(void);
#endif


/// @} end of group

#endif // _DBG_H_
