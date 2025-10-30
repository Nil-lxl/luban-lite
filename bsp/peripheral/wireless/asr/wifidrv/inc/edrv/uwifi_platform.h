/**
 ******************************************************************************
 *
 * @file uwifi_platorm.h
 *
 * Copyright (C) ASR
 *
 ******************************************************************************
 */

#ifndef _UWIFI_PLAT_H_
#define _UWIFI_PLAT_H_
#include "wifi_types.h"
#include "uwifi_sdio.h"
#define ASR_CONFIG_FW_NAME             "asr_settings.ini"  //MAC ADDR
#define ASR_PHY_CONFIG_TRD_NAME        "asr_trident.ini"  // null
#define ASR_PHY_CONFIG_KARST_NAME      "asr_karst.ini"    //0100 0000
#define ASR_AGC_FW_NAME                "agcram.bin"
#define ASR_LDPC_RAM_NAME              "ldpcram.bin"
#define ASR_MAC_FW_NAME                "fmacfw.bin"
#define ASR_FCU_FW_NAME                "fcuram.bin"

#define MSGL2U_VALID_PATTERN    0xADDEDE2A

//mdm
#define MDM_PHY_CONFIG_TRIDENT     0
#define MDM_PHY_CONFIG_ELMA        1
#define MDM_PHY_CONFIG_KARST       2

/// NSS field mask
#define MDM_NSS_MASK        ((uint32_t)0x00000F00)
/// NSS field LSB position
#define MDM_NSS_LSB         8

/// RFMODE field mask
#define MDM_RFMODE_MASK     ((uint32_t)0x000F0000)
/// RFMODE field LSB position
#define MDM_RFMODE_LSB      16

/// CHBW field mask
#define MDM_CHBW_MASK       ((uint32_t)0x03000000)
/// CHBW field LSB position
#define MDM_CHBW_LSB        24

/// AGC load version field mask
#define RIU_AGC_LOAD_MASK          ((uint32_t)0x00C00000)
/// AGC load version field LSB position
#define RIU_AGC_LOAD_LSB           22

#define __MDM_PHYCFG_FROM_VERS(v)  (((v) & MDM_RFMODE_MASK) >> MDM_RFMODE_LSB)

#define __RIU_AGCLOAD_FROM_VERS(v)  (((v) & RIU_AGC_LOAD_MASK) >> RIU_AGC_LOAD_LSB)

/* Modem Config */
#define MDM_MEMCLKCTRL0_ADDR          0x60C00848
/* AGC (trident) */
#define AGC_ASRAGCCNTL_ADDR          0x60C02060
/* LDPC RAM*/
#define PHY_LDPC_RAM_ADDR             0x60C09000
/* AGC RAM */
#define PHY_AGC_UCODE_ADDR            0x60C0A000
/* RIU */
#define RIU_AGCMEMBISTSTAT_ADDR       0x60C0B238
#define RIU_AGCMEMSIGNATURESTAT_ADDR  0x60C0B23C

/* */
#define FPGAB_MPIF_SEL_ADDR           0x60C10030

#define  BOOTROM_ENABLE               BIT(4)
#define  FPGA_B_RESET                 BIT(1)
#define  SOFT_RESET                   BIT(0)
//
#define TX_AGG_NUM           6


#define TX_AGGR_TIMEOUT_MS   4

#ifdef THREADX_STM32
#define RX_TRIGGER_TIMEOUT_MS     10
#else
#define RX_TRIGGER_TIMEOUT_MS     3
#endif

#define SDIO_DEBUG_TIMEOUT_MS   25000

#ifdef CFG_STA_AP_COEX
#define TX_AGG_BUF_UNIT_CNT  (50) // (5 * TX_AGG_NUM)  //  (16*3)  //240
#else
#define TX_AGG_BUF_UNIT_CNT  (5 * TX_AGG_NUM)     //  (16*3)  //240
#endif

#define TX_AGG_BUF_UNIT_SIZE ASR_ALIGN_BLKSZ_HI(1632)
#define TX_AGG_BUF_SIZE (TX_AGG_BUF_UNIT_CNT*TX_AGG_BUF_UNIT_SIZE)


struct asr_phy_conf_file {
    struct phy_trd_cfg_tag trd;
    struct phy_karst_cfg_tag karst;
};

#include <stdint.h>
#include "uwifi_defs.h"
#include "tasks_info.h"

struct asr_hw* uwifi_get_asr_hw(void);
int asr_sdio_init_config(struct asr_hw *asr_hw);
int asr_platform_on(struct asr_hw *asr_hw);


int asr_download_fw(struct asr_hw *asr_hw);

//int asr_platform_on(struct asr_hw *asr_hw);
void asr_platform_off(struct asr_hw *asr_hw);
void asr_ipc_tx_drain(struct asr_hw *asr_hw);
int asr_rxbuff_alloc(struct asr_hw *asr_hw, uint32_t len, struct sk_buff **skb);
int asr_handle_dynparams(struct asr_hw *asr_hw, struct wiphy *wiphy);
OSStatus init_sdio_task(struct asr_hw *asr_hw);
OSStatus init_rx_to_os(struct asr_hw *asr_hw);
OSStatus deinit_sdio_task(void);
OSStatus deinit_rx_to_os(void);



//#define SHARE_RAM_BASE_ADDR  0x68000000 //need to modify

#endif /* _UWIFI_PLAT_H_ */
