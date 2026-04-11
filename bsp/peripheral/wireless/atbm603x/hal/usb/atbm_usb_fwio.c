/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

 
#include "atbm_hal.h"  
#if (PROJ_TYPE == ATHENA_LITE)   
#include "firmware_lite.h" 

#elif (PROJ_TYPE == ATHENA_B)

#if ATBM_TX_SKB_NO_TXCONFIRM
#include "firmware_athenaB_usb_notxconfirm.h"  
#else //ATBM_TX_SKB_NO_TXCONFIRM
#include "firmware_athenaB_usb.h"  
#endif //ATBM_TX_SKB_NO_TXCONFIRM 

#elif ((PROJ_TYPE == ARES_A) || (PROJ_TYPE == ARES_B) )

#if ATBM_TX_SKB_NO_TXCONFIRM
#if USE_COMPRESSED_FIRMWARE
#include "compress.h"
#include "firmware_ares_usb_notxconfirm_compress.h"
#else
#include "firmware_ares_usb_notxconfirm.h" 
#endif
#else
#include "firmware_ares_usb.h" 
#endif
#elif (PROJ_TYPE == ASMLITE)
#if CONFIG_WIFI_BT_COMB
#include "firmware_asmlite_usb_combo_notxconfirm.h"
#else
#include "firmware_asmlite_usb_notxconfirm.h"
#endif
#elif (PROJ_TYPE == MERCURIUS)
#include "firmware_mercurius_usb_notxconfirm.h"
#else
#error error, project type is invalid.
#endif //ATHENA_LITE 
static struct firmware_altobeam fw_altobeam; 
extern int atbm_direct_read_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val);
atbm_void atbm_release_firmware(atbm_void)    
{
	wifi_printk(WIFI_ALWAYS,"atbm_release_firmware\n");
}
int atbm_init_firmware(atbm_void)   
{
	wifi_printk(WIFI_ALWAYS,"atbm_init_firmware\n");
	atbm_memset(&fw_altobeam,0,sizeof(struct firmware_altobeam));
	return 0; 
}



static int atbm_load_firmware_generic(struct atbmwifi_common *priv, atbm_uint8 *data,atbm_uint32 size,atbm_uint32 addr)
{
	int ret=0;
	atbm_uint32 put = 0;
	atbm_uint8 *buf = ATBM_NULL;

	buf = atbm_kmalloc(DOWNLOAD_BLOCK_SIZE*2,GFP_KERNEL);
	if (!buf) {
		wifi_printk(WIFI_DBG_ERROR,
			"%s: can't allocate bootloader buffer.\n", __FUNCTION__);
		ret = -ATBM_ENOMEM;
		goto error;
	}

	//if(priv->sbus_ops->bootloader_debug_config)
	//	priv->sbus_ops->bootloader_debug_config(priv->sbus_priv,0);

	/*  downloading loop */
	wifi_printk(WIFI_ALWAYS,"%s: addr %x: len %x\n",__FUNCTION__,addr,size);
	for (put = 0; put < size ;put += DOWNLOAD_BLOCK_SIZE) {
		atbm_uint32 tx_size;

		/* calculate the block size */
		tx_size  = atbm_min((size - put),(atbm_uint32)DOWNLOAD_BLOCK_SIZE);

		atbm_memcpy(buf, &data[put], tx_size);

		/* send the block to sram */
		ret = atbm_fw_write(priv,put+addr,buf, tx_size);
		if (ret < 0) {
			wifi_printk(WIFI_DBG_ERROR,
				"%s: can't write block at line %d.\n",
				__FUNCTION__, __LINE__);
			goto error;
		}
	} /* End of bootloader download loop */
error:
	atbm_kfree(buf);
	return ret;
}

char * atbm_HwGetChipFw(struct atbmwifi_common *priv)
{
	atbm_uint32 chipver = 0;
	char * strHwChipFw;


	atbm_direct_read_reg_32(priv,0x0acc017c,&chipver);
	chipver&=0x3f;
	switch(chipver)
	{
		case 0x0:	
			strHwChipFw = ("ApolloC0.bin");		
			break;
		case 0x1:	
			strHwChipFw = ("ApolloC0_TC.bin");	
			break;
		case 0x3:	
			strHwChipFw = ("ApolloC1_TC.bin");	
			break;
		case 0xc:	
			strHwChipFw = ("ApolloD.bin");		
			break;
		case 0xd:	
			strHwChipFw = ("ApolloD_TC.bin");	
			break;
		case 0x10:	
			strHwChipFw = ("ApolloE.bin");		
			break;
		case 0x20:	
			strHwChipFw = ("AthenaA.bin");		
			break;
		case 0x14:	
			strHwChipFw = ("ApolloF.bin");		
			break;
		case 0x15:	
			strHwChipFw = ("ApolloF_TC.bin");	
			break;
		case 0x24:	
			strHwChipFw = ("AthenaB.bin");		
			break;
		case 0x25:	
			strHwChipFw = ("AthenaBX.bin");		
			break;
		case 0x18:	
			strHwChipFw = ("Apollo_FM.bin");		
			break;
		default:
			strHwChipFw = FIRMWARE_DEFAULT_PATH;		
		break;
	} 

	wifi_printk(WIFI_ALWAYS,"%s, chipver=0x%x, use fw [%s]\n",__FUNCTION__, chipver,strHwChipFw );

	return strHwChipFw;  
}

#if USE_COMPRESSED_FIRMWARE


static int decopress_callback(CBP *cbp, atbm_uint8 *addr, int offset, int size){
	return atbm_load_firmware_generic(cbp->priv,addr,size,cbp->addr+offset);
}

static int atbm_start_load_firmware(struct atbmwifi_common *priv)
{
	CBP cbp;

	cbp.addr = DOWNLOAD_ITCM_ADDR;
	cbp.priv = priv;
	cbp.cb = decopress_callback;

	fcs_decompress(&fw_code, &cbp);

	cbp.addr = DOWNLOAD_DTCM_ADDR;
	fcs_decompress(&fw_data, &cbp);
}

#else
//#define USED_FW_FILE
static int atbm_start_load_firmware(struct atbmwifi_common *priv)
{
	  
	int ret;
	struct firmware_headr* hdr = (struct firmware_headr*)firmware_headr;

	wifi_printk(WIFI_DBG_ERROR,"%s: used firmware.h=\n", __FUNCTION__);
	fw_altobeam.hdr.iccm_len = sizeof(fw_code);
	fw_altobeam.hdr.dccm_len = sizeof(fw_data);
	
	fw_altobeam.fw_iccm = &fw_code[0];
	fw_altobeam.fw_dccm = &fw_data[0];

#if CONFIG_WIFI_BT_COMB
	fw_altobeam.hdr.sram_len = sizeof(fw_sram);
	if (hdr->flags == ALTOBEAM_WIFI_HDR_FLAG_V2)
		fw_altobeam.hdr.sram_addr = hdr->sram_addr;
	else
		fw_altobeam.hdr.sram_addr = DOWNLOAD_BLE_SRAM_ADDR;
	fw_altobeam.fw_sram = &fw_sram[0];
#endif

	wifi_printk(WIFI_DBG_ERROR,"%s: START DOWNLOAD ICCM=========\n", __FUNCTION__);
	ret = atbm_load_firmware_generic(priv,fw_altobeam.fw_iccm,fw_altobeam.hdr.iccm_len,DOWNLOAD_ITCM_ADDR);
	if(ret<0)
		goto error;
	if(fw_altobeam.hdr.dccm_len > 0x9000)
	fw_altobeam.hdr.dccm_len = 0x9000;

	wifi_printk(WIFI_DBG_ERROR,"%s: START DOWNLOAD DCCM=========\n", __FUNCTION__);
	ret = atbm_load_firmware_generic(priv,fw_altobeam.fw_dccm,fw_altobeam.hdr.dccm_len,DOWNLOAD_DTCM_ADDR);
	if(ret<0)
		goto error;

#if CONFIG_WIFI_BT_COMB
	if(fw_altobeam.hdr.sram_len){
		wifi_printk(WIFI_DBG_ERROR,"START DOWNLOAD BLE SRAM=========\n");
		ret = atbm_load_firmware_generic(priv, fw_altobeam.fw_sram, fw_altobeam.hdr.sram_len, fw_altobeam.hdr.sram_addr);
		if(ret<0)
			goto error;
	}
#endif

	wifi_printk(WIFI_DBG_ERROR, "%s: FIRMWARE DOWNLOAD SUCCESS\n",__FUNCTION__);

error:
	return ret;


}
#endif

int atbm_load_firmware(struct atbmwifi_common *hw_priv)
{
	int ret;

	wifi_printk(WIFI_ALWAYS,"atbm_before_load_firmware++\n");
	ret = atbm_before_load_firmware(hw_priv);
	if(ret <0)
		goto out;
	wifi_printk(WIFI_ALWAYS,"atbm_start_load_firmware++\n");
	ret = atbm_start_load_firmware(hw_priv);
	if(ret <0)
		goto out;
	wifi_printk(WIFI_ALWAYS,"atbm_after_load_firmware++\n");
	ret = atbm_after_load_firmware(hw_priv);
	if(ret <0){
		goto out;
	}
	ret =0;
out:
	return ret;
}
