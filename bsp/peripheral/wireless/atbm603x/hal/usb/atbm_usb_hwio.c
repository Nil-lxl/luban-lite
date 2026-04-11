/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#include "atbm_hal.h"


int atbm_ep0_read(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
				atbm_void *buf, atbm_uint32 buf_len)
{

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_read_sync(hw_priv->sbus_priv,
						  addr,
						  buf, buf_len); 
}

int atbm_ep0_write(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
				const atbm_void *buf, atbm_uint32 buf_len)
{
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_write_sync(hw_priv->sbus_priv,
						addr,buf, buf_len);
}

int atbm_fw_write(struct atbmwifi_common *priv, atbm_uint32 addr, const atbm_void *buf,
                        atbm_uint32 buf_len)
{
	return atbm_ep0_write(priv,  addr, buf, buf_len);
}

int atbm_direct_read_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val)
{
    int ret;

	ret= atbm_ep0_read(hw_priv, addr, val, sizeof(int));

	if (ret <= 0) {
		*val = 0xff;
		wifi_printk(WIFI_IF,
			"%s:  can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}

out:
	return ret;
}
int atbm_direct_write_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 val)
{
	int ret = atbm_ep0_write(hw_priv, addr, &val, sizeof(val));

	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s:  can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}

out:
	return ret;
}

int atbm_before_load_firmware(struct atbmwifi_common *hw_priv)
{
#if (PROJ_TYPE == ATHENA_LITE)

#elif (PROJ_TYPE == ATHENA_B)

#elif (PROJ_TYPE >= ARES_A)
	atbm_uint32 val;
	atbm_direct_write_reg_32(hw_priv,0x16100008,0xffffffff);
	atbm_direct_write_reg_32(hw_priv,0x0b000130,0xf);
	atbm_direct_write_reg_32(hw_priv,0x0b000134,0xf);

	atbm_direct_read_reg_32(hw_priv,0x16101000, &val);	
	val |= BIT(8);
	atbm_direct_write_reg_32(hw_priv,0x16101000, val);


	atbm_direct_read_reg_32(hw_priv,0x1610007c, &val);	
	val |= BIT(1);
	atbm_direct_write_reg_32(hw_priv,0x1610007c, val);
#endif
	return 0;
}
int atbm_after_load_firmware(struct atbmwifi_common *hw_priv)
{
	atbm_uint32 regdata;
#if (PROJ_TYPE == ATHENA_LITE)
   atbm_direct_write_reg_32(hw_priv,0x16100008,0);
   atbm_direct_write_reg_32(hw_priv,0x0b000130,0);
   atbm_direct_write_reg_32(hw_priv,0x0b000134,0);
   
  // regdata=0x100;
   /*reset cpu*/
   atbm_direct_write_reg_32(hw_priv,0x16101000,0x100);
   atbm_direct_write_reg_32(hw_priv,0x1610102c,0x3);
   /*release cpu*/
   atbm_direct_write_reg_32(hw_priv,0x16101000,0x0);
   hw_priv->sbus_ops->lmac_start(hw_priv->sbus_priv);
   iot_printf("lmac_start ATHENA_LITE\n");
#elif (PROJ_TYPE == ATHENA_B)
   atbm_direct_read_reg_32(hw_priv,0x1610007c,&regdata);
   regdata |= BIT(1);
   atbm_direct_write_reg_32(hw_priv,0x1610007c,regdata);
   atbm_direct_read_reg_32(hw_priv,0x16101000,&regdata);
   regdata |= BIT(8);
   atbm_direct_write_reg_32(hw_priv,0x16101000,regdata);
   regdata &= ~BIT(8);
   atbm_direct_write_reg_32(hw_priv,0x16101000,regdata);
   hw_priv->sbus_ops->lmac_start(hw_priv->sbus_priv);
   iot_printf("lmac_start ATHENA_B\n");
#elif (PROJ_TYPE >= ARES_A)
	atbm_direct_write_reg_32(hw_priv,0x16100008, 0);
	atbm_direct_write_reg_32(hw_priv,0x0b000130, 0);
	atbm_direct_write_reg_32(hw_priv,0x0b000134, 0);
	
	atbm_direct_read_reg_32(hw_priv,0x1610102c, &regdata);
	wifi_printk(WIFI_ALWAYS, "atbm_after_load_firmware: 0x1610102c=0x%x\n", regdata);
	regdata &= ~(0xffff0000);
	regdata |= BIT(0) | BIT(1) | (0x1 << 16);
	atbm_direct_write_reg_32(hw_priv,0x1610102c, regdata);
	atbm_direct_read_reg_32(hw_priv,0x1610102c, &regdata);
	wifi_printk(WIFI_ALWAYS, "atbm_after_load_firmware: 0x1610102c=0x%x\n", regdata);

	atbm_direct_read_reg_32(hw_priv,0x16101000, &regdata);
	regdata &= ~(BIT(8));
	atbm_direct_write_reg_32(hw_priv,0x16101000,regdata);
	atbm_direct_read_reg_32(hw_priv,0x16101000, &regdata);
	wifi_printk(WIFI_ALWAYS, "atbm_after_load_firmware: 0x16101000=0x%x\n", regdata);

	iot_printf("lmac_start ARES\n");
#endif    //ATHENA_LITE  
   hw_priv->init_done = 1;
   /*atbm receive packet form the device*/
   hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
   hw_priv->sbus_ops->sbus_memcpy_fromio(hw_priv->sbus_priv,0x2,ATBM_NULL,RX_BUFFER_SIZE);
   hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
   return 0;

}
atbm_void atbm_firmware_init_check(struct atbmwifi_common *hw_priv)
{


}

