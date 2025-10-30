/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
/* Sdio addr is 4*spi_addr */
#ifndef ATBM_USB_HWIO_H
#define ATBM_USB_HWIO_H
int atbm_reg_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
		    void *buf, atbm_uint32 buf_len);
int atbm_reg_write_unlock(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
		     const void *buf, atbm_uint32 buf_len);
int atbm_direct_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val);
int atbm_direct_write_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 val);
int atbm_ahb_write_unlock(struct atbmwifi_common *priv, atbm_uint32 addr, const void *buf,
                     atbm_uint32 buf_len);
int atbm_indirect_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr, void *buf,
			 atbm_uint32 buf_len, atbm_uint32 prefetch, atbm_uint16 port_addr);

int atbm_reg_write_16(struct atbmwifi_common *hw_priv,
			atbm_uint16 addr, atbm_uint16 val);

#define SPI_REG_ADDR_TO_SDIO(spi_reg_addr) ((spi_reg_addr) << 2)
	
#define SDIO_ADDR17BIT(buf_id, mpf, rfu, reg_id_ofs) \
					((((buf_id)    & 0x3F) << 6) \
					| (((rfu)		 & 1) << 5) \
					| (((reg_id_ofs) & 0x1F) << 0))	
#define MAX_RETRY		9
int atbm_fw_write(struct atbmwifi_common *priv, atbm_uint32 addr, const atbm_void *buf,
						atbm_uint32 buf_len);
static inline int atbm_ahb_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
				  void *buf, atbm_size_t buf_len)
{
	return atbm_indirect_read_unlock(hw_priv, addr, buf, buf_len,
		ATBM_HIFREG_CONFIG_AHB_PFETCH_BIT, ATBM_HIFREG_AHB_DPORT_REG_ID);
}
static inline int atbm_reg_read_unlock_32(struct atbmwifi_common *hw_priv,
				     atbm_uint16 addr, atbm_uint32 *val)
{
	return atbm_reg_read_unlock(hw_priv, addr, val, sizeof(int));
}
static inline int atbm_reg_write_unlock_32(struct atbmwifi_common *hw_priv,
				      atbm_uint16 addr, atbm_uint32 val)
{
	return atbm_reg_write_unlock(hw_priv, addr, &val, sizeof(int));
}
static inline int atbm_ahb_read_unlock_32(struct atbmwifi_common *hw_priv,
					 atbm_uint32 addr, atbm_uint32 *val)
{
	return atbm_ahb_read_unlock(hw_priv, addr, val, sizeof(int));
}
int atbm_data_force_write(struct atbmwifi_common *hw_priv, const void *buf,
                        int buf_len);
static inline int atbm_ahb_write_unlock_32(struct atbmwifi_common *hw_priv,
					  atbm_uint32 addr, atbm_uint32 val)
{
	int ret;
	int retry = 0;
	while(retry<=3){
		ret = atbm_ahb_write_unlock(hw_priv, addr, &val, sizeof(val));
		if(ret){
			wifi_printk(WIFI_ALWAYS,"%s\n",__func__);
		}else{
			break;
		}
	}
	if((addr&WRITE_32K_ADDR_MSK)==WRITE_32K_ADDR)
	{
		atbm_mdelay(10);
	}
	return ret;
}
#endif
