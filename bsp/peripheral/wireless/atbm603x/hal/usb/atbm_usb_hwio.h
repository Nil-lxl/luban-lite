
/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBM_USB_HWIO_H
#define ATBM_USB_HWIO_H
#define MEMENDADDR (0x9000000+80*1024-4)
#define MEMVAL	   (0xaaffaaff)
#define ATBM_READ_REG_TEST 			(0)
#define ATBM_DEBUG_BUS_TEST			(0)
#define DOWNLOAD_BLOCK_SIZE		(64)

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

#define DOWNLOAD_ITCM_SIZE		(128*1024)
#define DOWNLOAD_DTCM_ADDR		(0x00800000)
#define DOWNLOAD_DTCM_SIZE		(48*1024)

int atbm_before_load_firmware(struct atbmwifi_common *hw_priv);
int atbm_after_load_firmware(struct atbmwifi_common *hw_priv);
int atbm_fw_write(struct atbmwifi_common *priv, atbm_uint32 addr, const atbm_void *buf,atbm_uint32 buf_len);
atbm_void atbm_firmware_init_check(struct atbmwifi_common *hw_priv);
#endif //ATBM_USB_HWIO_H
