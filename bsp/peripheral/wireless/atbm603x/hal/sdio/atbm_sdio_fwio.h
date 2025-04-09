/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#ifndef ATBM_SDIO_FWIO_H
#define ATBM_SDIO_FWIO_H

#define ALTOBEAM_WIFI_HDR_FLAG_V2  (0x34353678)

struct firmware_headr {
	atbm_uint32 flags; /*0x34353677*/
	atbm_uint32 version;
	atbm_uint32 iccm_len;
	atbm_uint32 dccm_len;
	atbm_uint32 sram_len;
	atbm_uint32 sram_addr;
	atbm_uint32 reserve[1];
	atbm_uint16 reserve2;
	atbm_uint16 checksum;
};

struct firmware_altobeam {
	struct firmware_headr hdr;
	atbm_uint8 *fw_iccm;
	atbm_uint8 *fw_dccm;
	atbm_uint8 *fw_sram;
};
#define FIRMWARE_DEFAULT_PATH "fw.bin"
int atbm_init_firmware(atbm_void) ;
int atbm_load_firmware(struct atbmwifi_common *hw_priv);
atbm_void atbm_release_firmware(atbm_void) ;
atbm_void atbm_firmware_init_check(struct atbmwifi_common *hw_priv);

#endif //ATBM_USB_FWIO_H

