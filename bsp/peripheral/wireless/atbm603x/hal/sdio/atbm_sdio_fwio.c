/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "atbm_sdio.h"
#include "atbm_sdio_hwio.h"
#include "atbm_sdio_fwio.h"
#include "atbm_sdio_bh.h"

#if (PROJ_TYPE == ATHENA_LITE)   
#include "firmware_lite.h" 

#elif (PROJ_TYPE == ATHENA_B)

#if ATBM_TX_SKB_NO_TXCONFIRM
#include "firmware_athenaB_sdio_notxconfirm.h"  
#else //ATBM_TX_SKB_NO_TXCONFIRM
#include "firmware_athenaB_sdio.h"  
#endif //ATBM_TX_SKB_NO_TXCONFIRM 

#elif (PROJ_TYPE == ARES_A || PROJ_TYPE == ARES_B)

#if ATBM_TX_SKB_NO_TXCONFIRM
#include "firmware_ares_sdio_notxconfirm.h" 
#else
#include "firmware_ares_sdio.h"  
#endif

#elif (PROJ_TYPE == HERA)

#if ATBM_TX_SKB_NO_TXCONFIRM
#include "firmware_hera_sdio_notxconfirm.h"
#else
#include "firmware_hera_sdio.h" 
#endif

#elif (PROJ_TYPE == ASMLITE)
#if CONFIG_WIFI_BT_COMB
#include "firmware_asmlite_sdio_combo_notxconfirm.h"
#else
#include "firmware_asmlite_sdio_notxconfirm.h"
#endif

#else
#error error, project type is invalid.

#endif //ATHENA_LITE 

static struct firmware_altobeam fw_altobeam; 

extern int atbm_reg_read_32(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint32 *val);
extern int atbm_reg_write_32(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint32 val);
extern int atbm_reg_read_16(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint16 *val);
extern int atbm_reg_write_16(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_uint16 val);
extern int atbm_ahb_read_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val);
extern int atbm_ahb_write_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 val);

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

int atbm_before_load_firmware(struct atbmwifi_common *hw_priv)
{
	int ret=0;
	int i;
	atbm_uint32 val32;
	atbm_uint16 val16;
	//int major_revision;

	atbm_uint32 config_reg;
	
	ATBM_BUG_ON(!hw_priv);

	/* Read CONFIG Register Value - We will read 32 bits */
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: can't read config register.\n", __FUNCTION__);
		goto out;
	}
	/* Set wakeup bit in device */
	ret = atbm_reg_read_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID, &val16);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: set_wakeup: can't read " \
			"control register.\n", __FUNCTION__);
		goto out;
	}

	ret = atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID,
		val16 | ATBM_HIFREG_CONT_WUP_BIT);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: set_wakeup: can't write " \
			"control register.\n", __FUNCTION__);
		goto out;
	}
#if TEST_DCXO_DPLL_CONFIG
	/*start config dcxo */
	ret=atbm_config_dcxo(hw_priv,dcxo_value,PROJ_TYPE,DCXO_TYPE,DPLL_CLOCK);
	if (ret<0){
		wifi_printk(WIFI_IF, "atbm_config_dcxo error.\n");
	}
	/*start config dpll */
	ret = atbm_config_dpll(hw_priv,dpll_value,PROJ_TYPE,DPLL_CLOCK);
	if (ret<0){
		wifi_printk(WIFI_IF, "atbm_config_dpll error.\n");
	 }
	/*The fifth step store dpll value to smu*/
	atbm_set_config_to_smu(hw_priv,DPLL_CLOCK);
	/*start shut down system*/
	ret =atbm_system_done(hw_priv);
	if (ret<0){
		wifi_printk(WIFI_IF, "atbm_system_done error.\n");
	}
	wifi_printk(WIFI_IF, "atbm_wait_wlan_rdy  Wait for wakeup .\n");
	/* Set wakeup bit in device */
	ret = atbm_reg_read_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID, &val16);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: set_wakeup: can't read " \
			"control register.\n", __FUNCTION__);
		goto out;
	}

	ret = atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID,
		val16 | ATBM_HIFREG_CONT_WUP_BIT);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: set_wakeup: can't write " \
			"control register.\n", __FUNCTION__);
		goto out;
	}
#endif

	/* Wait for wakeup */
	for (i = 0 ; i < 3000 ; i += 1 + i / 2) {
		ret = atbm_reg_read_16(hw_priv,
			ATBM_HIFREG_CONTROL_REG_ID, &val16);
		if (ret < 0) {
			wifi_printk(WIFI_IF,
				"%s: wait_for_wakeup: can't read " \
				"control register.\n", __FUNCTION__);
			goto out;
		}

		if (val16 & ATBM_HIFREG_CONT_RDY_BIT) {
			wifi_printk(WIFI_IF,
				"WLAN device is ready.\n");
			break;
		}
		atbm_mdelay(i);
	}

	if ((val16 & ATBM_HIFREG_CONT_RDY_BIT) == 0) {
		wifi_printk(WIFI_IF,
			"%s: wait_for_wakeup: device is not responding.\n",
			__FUNCTION__);
		ret = -ATBM_ETIMEDOUT;
		goto out;
	}
	atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &config_reg);
	if(config_reg & ATBM_HIFREG_PS_SYNC_SDIO_FLAG)
	{
		config_reg |= ATBM_HIFREG_PS_SYNC_SDIO_CLEAN;
		atbm_reg_write_32(hw_priv,ATBM_HIFREG_CONFIG_REG_ID,config_reg);
	}
	/* set cpu reset ,cpu will stop */
	/* Checking for access mode */
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't read " \
			"config register.\n", __FUNCTION__);
		goto out;
	}
	val32 |= ATBM_HIFREG_CONFIG_CPU_RESET_BIT|ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT;
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}

	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't read " \
			"config register.\n", __FUNCTION__);
		goto out;
	}

	ATBM_WARN_ON_FUNC(!(val32 & ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT));
out:
	return ret;

}
static int atbm_load_firmware_generic(struct atbmwifi_common *priv, atbm_uint8 *data,atbm_uint32 size,atbm_uint32 addr)
{
	int ret=0;
	atbm_uint32 put = 0;
	atbm_uint8 *buf = ATBM_NULL;


	buf = (atbm_uint8 *)atbm_kmalloc(DOWNLOAD_BLOCK_SIZE*2,GFP_KERNEL);
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
//atbm_initial_irq
int atbm_after_load_firmware(struct atbmwifi_common *hw_priv)
{
	int ret;
	atbm_uint32 val32;

	//enable gpio irq register,may need move to lmac/apb.c	SMU_Init
	ret=atbm_ahb_read_32(hw_priv,0x161000ac,&val32);
	val32&=0xFFFFF7F8;
	val32|=BIT(12);
	ret=atbm_ahb_write_32(hw_priv,0x161000ac,val32);
	if(ret<0){
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't read " \
			"config register.\n", __FUNCTION__);

	}
	/* Register Interrupt Handler */
	ret = hw_priv->sbus_ops->irq_subscribe(hw_priv->sbus_priv,
		(sbus_irq_handler)atbm_irq_handler, hw_priv);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: can't register IRQ handler.\n", __FUNCTION__);
	}		
#if (PROJ_TYPE>=ARES_A)
	ret=atbm_ahb_read_32(hw_priv,0x1610102c,&val32);
	if(ret<0){
		wifi_printk(WIFI_DBG_ERROR,
			"%s: 0x1610102c: can't read register.\n", __func__);
		goto out;
	}
	val32 &= ~(0xffff0000);
	val32 |= BIT(0) | BIT(1) | (0x1 << 16);
	ret=atbm_ahb_write_32(hw_priv,0x1610102c,val32);
	if(ret<0){
		wifi_printk(WIFI_DBG_ERROR,
			"%s: 0x1610102c: can't write register.\n", __func__);
		goto out;
	}
	while(1)
	{
		ret=atbm_ahb_read_32(hw_priv,0x1610102c,&val32);
		if(ret<0){
			wifi_printk(WIFI_DBG_ERROR,
				"%s: 0x1610102c: can't read register.\n", __func__);
			goto out;
		}
		if (val32 & BIT(16))
			break;
		atbm_mdelay(1);
	}
#endif
	/* If device is CW1200 the IRQ enable/disable bits
	 * are in CONFIG register, clear cpu reset ,cpu will run */
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't read " \
			"config register.\n", __FUNCTION__);
		goto unsubscribe;
	}
	val32 |= ATBM_HIFREG_CONF_IRQ_RDY_ENABLE;
	val32 &= ~ATBM_HIFREG_CONFIG_CPU_RESET_BIT;
	//enable data1 IRQ
	val32 &= ~ATBM_HIFREG_CONFIG_CLEAR_INT_BIT;
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't write " \
			"config register.\n", __FUNCTION__);
		goto unsubscribe;
	}

#if (PROJ_TYPE==ARES_B)
	ret=atbm_ahb_write_32(hw_priv,0x16100074,0x1);
	if(ret<0){
		wifi_printk(WIFI_DBG_ERROR,
			"%s: 0x1610102c: can't write register.\n", __func__);
		goto out;
	}
#endif

	/* Configure device for MESSSAGE MODE */
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: set_mode: can't read config register.\n",
			__FUNCTION__);
		goto unsubscribe;
	}
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,
		val32 & ~ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: set_mode: can't write config register.\n",
			__FUNCTION__);
		goto unsubscribe;
	}

	hw_priv->init_done = 1;
	/* Unless we read the CONFIG Register we are
	 * not able to get an interrupt */
	atbm_mdelay(10);
	atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);

out:
	return ret;
unsubscribe:
	hw_priv->sbus_ops->irq_unsubscribe(hw_priv->sbus_priv);
	return ret;

}
atbm_void atbm_firmware_init_check(struct atbmwifi_common *hw_priv)
{
	atbm_uint16 ctrl_reg;

	ATBM_WARN_ON(atbm_reg_write_16(hw_priv, ATBM_HIFREG_CONTROL_REG_ID,
					ATBM_HIFREG_CONT_WUP_BIT));

	if (atbm_reg_read_16(hw_priv,ATBM_HIFREG_CONTROL_REG_ID, &ctrl_reg))
		ATBM_WARN_ON(atbm_reg_read_16(hw_priv,ATBM_HIFREG_CONTROL_REG_ID,
						&ctrl_reg));

	ATBM_WARN_ON_FUNC(!(ctrl_reg & ATBM_HIFREG_CONT_RDY_BIT));

}

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

