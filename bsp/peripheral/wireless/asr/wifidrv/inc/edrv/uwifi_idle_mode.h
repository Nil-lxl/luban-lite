#ifndef _UWIFI_IDLE_MODE_H_
#define _UWIFI_IDLE_MODE_H_

//#include "asr_cm4.h"
#include "uwifi_txq.h"
#include "asr_dbg.h"
#include "uwifi_msg_tx.h"
#include "uwifi_tx.h"
#include "uwifi_rx.h"
#include "uwifi_ops_adapter.h"
#include "hostapd.h"
typedef unsigned char   u8;
typedef unsigned short  u16;
#if !defined(AWORKS) && !defined(STM32H7_SDK) && !defined(HX_SDK)
typedef unsigned int    u32;
#endif
#define  CUS_MGMT_FRAME  300

#define NX_NB_TXQ_MAX_IDLE_MODE NX_TXQ_CNT

uint8_t* uwifi_init_mgmtframe_header(struct ieee80211_hdr *frame, uint8_t *addr1,
                                   uint8_t *addr2, uint8_t *addr3, uint16_t subtype);

void uwifi_tx_dev_custom_mgmtframe(uint8_t *pframe, uint32_t len);

void asr_txq_vif_init_idle_mode(struct asr_hw *asr_hw, uint8_t status);
void asr_txq_vif_deinit_idle_mode(struct asr_hw *asr_hw, struct asr_vif *asr_vif);
void asr_txq_vif_start_idle_mode(struct asr_vif *asr_vif, uint16_t reason, struct asr_hw *asr_hw);
void asr_txq_vif_stop_idle_mode(struct asr_vif *asr_vif, uint16_t reason, struct asr_hw *asr_hw);

struct asr_txq *asr_txq_vif_get_idle_mode(struct asr_hw *asr_hw, uint8_t ac, int *idx);

#endif
