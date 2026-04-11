/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef  ATBMWIFI__HWIO_SDIO_H_INCLUDED
#define  ATBMWIFI__HWIO_SDIO_H_INCLUDED
#include "atbm_sbus.h"
 /* DPLL initial values */
#define DPLL_INIT_VAL_9000		(0x00000191)
#define DPLL_INIT_VAL_CW1200		(0x0EC4F121)
#define ALTOBEAM_WIFI_HDR_FLAG  (0x34353677)

#if (PROJ_TYPE>=ARES_A)
#define DOWNLOAD_ITCM_ADDR		(0x00010000)
#else
#define DOWNLOAD_ITCM_ADDR		(0x00000000)
#endif 

#if CONFIG_WIFI_BT_COMB
#define DOWNLOAD_BLE_SRAM_ADDR		(0x09018000)
#define BLE_SRAM_CODE_SIZE			0x10000
#else//#ifdef CONFIG_WIFI_BT_COMB
#define DOWNLOAD_BLE_SRAM_ADDR		(0x0900A000)
#define BLE_SRAM_CODE_SIZE			0x16000
#endif  //#ifdef CONFIG_WIFI_BT_COMB

#define DOWNLOAD_ITCM_SIZE		(160*1024)
#define DOWNLOAD_DTCM_ADDR		(0x00800000)
#define DOWNLOAD_DTCM_SIZE		(48*1024)
#define DOWNLOAD_BLOCK_SIZE		(256)
#define SYS_BASE_ADDR_SILICON		(0)
#define PAC_BASE_ADDRESS_SILICON	(SYS_BASE_ADDR_SILICON + 0x09000000)
#define PAC_SHARED_MEMORY_SILICON	(PAC_BASE_ADDRESS_SILICON)
#define CW12000_APB(addr)		(PAC_SHARED_MEMORY_SILICON + (addr)) 
 /* ***************************************************************
 *Device register definitions
 *************************************************************** */
 /* WBF - SPI Register Addresses */
#define ATBM_HIFREG_ADDR_ID_BASE		(0x0000)
	 /* 16/32 bits */
#define ATBM_HIFREG_CONFIG_REG_ID		(0x0000)
	 /* 16/32 bits */
#define ATBM_HIFREG_CONTROL_REG_ID		(0x0001)
	 /* 16 bits, Q mode W/R */
#define ATBM_HIFREG_IN_OUT_QUEUE_REG_ID	(0x0002)
	 /* 32 bits, AHB bus R/W */
#define ATBM_HIFREG_AHB_DPORT_REG_ID	(0x0003)
	 /* 16/32 bits */
#define ATBM_HIFREG_SRAM_BASE_ADDR_REG_ID   (0x0004)
	 /* 32 bits, APB bus R/W */
#define ATBM_HIFREG_SRAM_DPORT_REG_ID	(0x0005)
	 /* 32 bits, t_settle/general */
#define ATBM_HIFREG_TSET_GEN_R_W_REG_ID	(0x0006)
	 /* 16 bits, Q mode read, no length */
#define ATBM_HIFREG_FRAME_OUT_REG_ID	(0x0007)
#define ATBM_HIFREG_ADDR_ID_MAX		(ATBM_HIFREG_FRAME_OUT_REG_ID)
	 
	 /* WBF - Control register bit set */
	 /* next o/p length, bit 11 to 0 */
#define ATBM_HIFREG_CONT_NEXT_LEN_MASK	(0xCFFF)
#define ATBM_HIFREG_CONT_NEXT_LEN_LSB_MASK	(0x0FFF)
#define ATBM_HIFREG_CONT_NEXT_LEN_MSB_MASK	(0xC000)
#define ATBM_HIFREG_CONT_WUP_BIT		(BIT(12))
#define ATBM_HIFREG_CONT_RDY_BIT		(BIT(13))
#define ATBM_HIFREG_CONT_IRQ_ENABLE		(BIT(14))
#define ATBM_HIFREG_CONT_RDY_ENABLE		(BIT(15))
#define ATBM_HIFREG_CONT_IRQ_RDY_ENABLE	(BIT(14)|BIT(15)) 
#define ATBM_HIFREG_PS_SYNC_SDIO_FLAG	(BIT(23))
#define ATBM_HIFREG_PS_SYNC_SDIO_CLEAN	(BIT(24))
/* SPI Config register bit set */
#define ATBM_HIFREG_CONFIG_FRAME_BIT	(BIT(2))
#define ATBM_HIFREG_CONFIG_WORD_MODE_BITS	(BIT(3)|BIT(4))
#define ATBM_HIFREG_CONFIG_WORD_MODE_1	(BIT(3))
#define ATBM_HIFREG_CONFIG_WORD_MODE_2	(BIT(4))
#define ATBM_HIFREG_CONFIG_ERROR_0_BIT	(BIT(5))
#define ATBM_HIFREG_CONFIG_ERROR_1_BIT	(BIT(6))
#define ATBM_HIFREG_CONFIG_ERROR_2_BIT	(BIT(7))
/* TBD: Sure??? */
#define ATBM_HIFREG_CONFIG_CSN_FRAME_BIT	(BIT(7))
#define ATBM_HIFREG_CONFIG_ERROR_3_BIT	(BIT(8))
#define ATBM_HIFREG_CONFIG_ERROR_4_BIT	(BIT(9))
/* QueueM */
#define ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT	(BIT(10))
/* AHB bus */
#define ATBM_HIFREG_CONFIG_AHB_PFETCH_BIT	(BIT(11))
#define ATBM_HIFREG_CONFIG_CPU_CLK_DIS_BIT	(BIT(12))
/* APB bus */
#define ATBM_HIFREG_CONFIG_PFETCH_BIT	(BIT(13))
/* cpu reset */
#define ATBM_HIFREG_CONFIG_CPU_RESET_BIT	(BIT(14))
#define ATBM_HIFREG_CONFIG_CLEAR_INT_BIT	(BIT(15))
/* For CW1200 the IRQ Enable and Ready Bits are in CONFIG register */
#define ATBM_HIFREG_CONF_IRQ_RDY_ENABLE	(BIT(16)|BIT(17))
#define ATBM_HIFREG_CONF_RDY_IRQ_ENABLE	(BIT(17))
#define ATBM_HIFREG_CONF_DATA_IRQ_ENABLE	(BIT(16))	 
#define ATBM_HIFREG_CONFIG_CPU_RESET_BIT_2	(BIT(22))
 int atbm_data_read(struct atbmwifi_common *hw_priv,
			  atbm_void *buf, atbm_uint32 buf_len);
 int atbm_data_write(struct atbmwifi_common *hw_priv, const atbm_void *buf,
			 atbm_size_t buf_len);
 int atbm_reg_read_dpll(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
			 atbm_void *buf, atbm_uint32 buf_len);
 int atbm_reg_write_dpll(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
			  const atbm_void *buf, atbm_uint32 buf_len);
 int atbm_reg_read(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
			 atbm_void *buf, atbm_uint32 buf_len);
 int atbm_reg_write(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
			  const atbm_void *buf, atbm_uint32 buf_len);
 
int atbm_indirect_read(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_void *buf,
		  atbm_uint32 buf_len, atbm_uint32 prefetch, atbm_uint16 port_addr);
int atbm_apb_write(struct atbmwifi_common *hw_priv, atbm_uint32 addr, const atbm_void *buf,
		  atbm_uint32 buf_len);
int atbm_ahb_write(struct atbmwifi_common *priv, atbm_uint32 addr, const atbm_void *buf,
				  atbm_uint32 buf_len);
int atbm_direct_read_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val);
int atbm_direct_write_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 val);
int __atbm_irq_enable(struct atbmwifi_common *priv, int enable);

#define WRITE_32K_ADDR_MSK	(0xfffff000)
#define WRITE_32K_ADDR	(0x16101000)

extern struct sbus_ops atbm_sdio_sbus_ops;
#define ATBM_SDIO_CCCR_IENx		0x04
#define ATBM_SDIO_CCCR_IOEx  0x02
#define ATBM_SDIO_CCCR_IORx  0x03
#define ATBM_SDIO_CCCR_ABORT		0x06	/* function abort/card reset */
#define ATBM_SDIO_CCCR_IF		0x07	/* bus interface controls */
#define ATBM_SDIO_CCCR_SPEED		0x13

#define  ATBM_SDIO_SPEED_SHS		0x01	/* Supports High-Speed mode */

#define  ATBM_SDIO_SPEED_BSS_SHIFT	1
#define  ATBM_SDIO_SPEED_SDR25	(1<<ATBM_SDIO_SPEED_BSS_SHIFT)
#define  ATBM_SDIO_SPEED_EHS		ATBM_SDIO_SPEED_SDR25	/* Enable High-Speed */
#define  ATBM_SDIO_BUS_WIDTH_4BIT	0x02
#define ATBM_SDIO_FBR_BLKSIZE   0x10

#define ATBM_SDIO_CCCR_POWER		0x12

#define  SDIO_POWER_SMPC	0x01	/* Supports Master Power Control */
#define  SDIO_POWER_EMPC	0x02	/* Enable Master Power Control */


#define ATBM_SDIO_CCCR_CAPS 0x08

#define  SDIO_CCCR_CAP_SDC	0x01	/* can do CMD52 while data transfer */
#define  SDIO_CCCR_CAP_SMB	0x02	/* can do multi-block xfers (CMD53) */
#define  SDIO_CCCR_CAP_SRW	0x04	/* supports read-wait protocol */
#define  SDIO_CCCR_CAP_SBS	0x08	/* supports suspend/resume */
#define  SDIO_CCCR_CAP_S4MI	0x10	/* interrupt during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_E4MI	0x20	/* enable ints during 4-bit CMD53 */
#define  SDIO_CCCR_CAP_LSC	0x40	/* low speed card */
#define  SDIO_CCCR_CAP_4BLS	0x80	/* 4 bit low speed card */

	
#define ATBM_SDIO_CCCR_CCCR  0x00
#define  SDIO_CCCR_REV_1_00	0	/* CCCR/FBR Version 1.00 */
#define  SDIO_CCCR_REV_1_10	1	/* CCCR/FBR Version 1.10 */
#define  SDIO_CCCR_REV_1_20	2	/* CCCR/FBR Version 1.20 */
	
#define  SDIO_SDIO_REV_1_00	0	/* SDIO Spec Version 1.00 */
#define  SDIO_SDIO_REV_1_10	1	/* SDIO Spec Version 1.10 */
#define  SDIO_SDIO_REV_1_20	2	/* SDIO Spec Version 1.20 */
#define  SDIO_SDIO_REV_2_00	3	/* SDIO Spec Version 2.00 */

atbm_void atbm_sdio_module_init(atbm_void);
int atbm_sdio_set_block_size(struct sbus_priv *self, atbm_uint32 size);
int atbm_sdio_suspend(struct atbmwifi_common *hw_priv);
int atbm_sdio_resume(struct atbmwifi_common *hw_priv);
atbm_void atbm_sdio_module_exit(atbm_void);

#endif /*  ATBMWIFI__HWIO_SDIO_H_INCLUDED */

