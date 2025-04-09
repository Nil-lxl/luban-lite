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

int atbm_reg_read_8(struct atbmwifi_common *hw_priv,
				atbm_uint16 addr, atbm_uint8 *val)
{
	atbm_uint32 bigVal;
	int ret;
	ret = atbm_reg_read_dpll(hw_priv, addr, &bigVal, sizeof(bigVal));
	*val = (atbm_uint8)bigVal;
	return ret;
}

int atbm_reg_write_8(struct atbmwifi_common *hw_priv,
				atbm_uint16 addr, atbm_uint8 val)
{
	atbm_uint32 bigVal = (atbm_uint32)val;
	return atbm_reg_write_dpll(hw_priv, addr, &bigVal, sizeof(bigVal));
}
 
int atbm_reg_read_16(struct atbmwifi_common *hw_priv,
				atbm_uint16 addr, atbm_uint16 *val)
{
	atbm_uint32 bigVal;
	int ret;
	ret = atbm_reg_read(hw_priv, addr, &bigVal, sizeof(bigVal));
	*val = (atbm_uint16)bigVal;
	return ret;
}

int atbm_reg_write_16(struct atbmwifi_common *hw_priv,
				atbm_uint16 addr, atbm_uint16 val)
{
	atbm_uint32 bigVal = (atbm_uint32)val;
	return atbm_reg_write(hw_priv, addr, &bigVal, sizeof(bigVal));
}

int atbm_reg_read_32(struct atbmwifi_common *hw_priv,
				atbm_uint16 addr, atbm_uint32 *val)
{
	return atbm_reg_read(hw_priv, addr, val, sizeof(int));
}

int atbm_reg_write_32(struct atbmwifi_common *hw_priv,
				atbm_uint16 addr, atbm_uint32 val)
{
	return atbm_reg_write(hw_priv, addr, &val, sizeof(int));
}

int atbm_apb_read(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
				atbm_void *buf, atbm_uint32 buf_len)
{
	return atbm_indirect_read(hw_priv, addr, buf, buf_len,
		ATBM_HIFREG_CONFIG_PFETCH_BIT, ATBM_HIFREG_SRAM_DPORT_REG_ID);
}

int atbm_ahb_read(struct atbmwifi_common *hw_priv, atbm_uint32 addr,
				atbm_void *buf, atbm_size_t buf_len)
{
	return atbm_indirect_read(hw_priv, addr, buf, buf_len,
		ATBM_HIFREG_CONFIG_AHB_PFETCH_BIT, ATBM_HIFREG_AHB_DPORT_REG_ID);
}

int atbm_apb_read_32(struct atbmwifi_common *hw_priv,
				atbm_uint32 addr, atbm_uint32 *val)
{
	return atbm_apb_read(hw_priv, addr, val, sizeof(int));
}

int atbm_apb_write_32(struct atbmwifi_common *hw_priv,
				atbm_uint32 addr, atbm_uint32 val)
{
	return atbm_apb_write(hw_priv, addr, &val, sizeof(val));
}

int atbm_ahb_read_32(struct atbmwifi_common *hw_priv,
				atbm_uint32 addr, atbm_uint32 *val)
{
	return atbm_ahb_read(hw_priv, addr, val, sizeof(int));
}

int atbm_ahb_write_32(struct atbmwifi_common *hw_priv,
				atbm_uint32 addr, atbm_uint32 val)
{
	int ret = atbm_ahb_write(hw_priv, addr, &val, sizeof(val));
	if((addr&WRITE_32K_ADDR_MSK)==WRITE_32K_ADDR)
	{
		atbm_mdelay(10);
	}
	return ret;
}

static int __atbm_reg_read(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
				atbm_void *buf, atbm_uint32 buf_len)
{
	atbm_uint16 addr_sdio;
	atbm_uint32 sdio_reg_addr_17bit ;

	/* Check if buffer is aligned to 4 byte boundary */
	if (ATBM_WARN_ON(((unsigned long)buf & 3) && (buf_len > 4))) {
		wifi_printk(WIFI_IF,
			   "%s: buffer is not aligned.\n", __FUNCTION__);
		return -ATBM_EINVAL;
	}

	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(0, 0, 0, addr_sdio);

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_read_sync(hw_priv->sbus_priv,
						  sdio_reg_addr_17bit,
						  buf, buf_len);
}

static int __atbm_reg_write(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
				const atbm_void *buf, atbm_uint32 buf_len)
{
	atbm_uint16 addr_sdio;
	atbm_uint32 sdio_reg_addr_17bit ;


	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(0, 0, 0, addr_sdio);

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_write_sync(hw_priv->sbus_priv,
						sdio_reg_addr_17bit,
						buf, buf_len);
}

static int __atbm_data_read(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
				atbm_void *buf, atbm_uint32 buf_len, int buf_id)
{
	atbm_uint16 addr_sdio;
	atbm_uint32 sdio_reg_addr_17bit ;

	/* Check if buffer is aligned to 4 byte boundary */
	if (ATBM_WARN_ON(((unsigned long)buf & 3) && (buf_len > 4))) {
		wifi_printk(WIFI_IF,
			   "%s: buffer is not aligned.\n", __FUNCTION__);
		return -ATBM_EINVAL;
	}

	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_memcpy_fromio(hw_priv->sbus_priv,
						  sdio_reg_addr_17bit,
						  buf, buf_len);
}

static int __atbm_data_write(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
				const atbm_void *buf, atbm_uint32 buf_len, int buf_id)
{
	atbm_uint16 addr_sdio;
	atbm_uint32 sdio_reg_addr_17bit ;


	/* Convert to SDIO Register Address */
	addr_sdio = SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_memcpy_toio(hw_priv->sbus_priv,
						sdio_reg_addr_17bit,
						buf, buf_len);
}

static	int __atbm_reg_read_32(struct atbmwifi_common *hw_priv,
					atbm_uint16 addr, atbm_uint32 *val)
{
	return __atbm_reg_read(hw_priv, addr, val, sizeof(val));
}

static	int __atbm_reg_write_32(struct atbmwifi_common *hw_priv,
					atbm_uint16 addr, atbm_uint32 val)
{
	return __atbm_reg_write(hw_priv, addr, &val, sizeof(val));
}

int __atbm_reg_write_dpll(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
			const atbm_void *buf, atbm_uint32 buf_len,int buf_id)
{
	atbm_uint16 addr_sdio;
	atbm_uint32 sdio_reg_addr_17bit ;

	/* Check if buffer is aligned to 4 byte boundary */
	if (ATBM_WARN_ON(((unsigned long)buf & 3) && (buf_len > 4))) {
		wifi_printk(WIFI_IF,
			   "%s: buffer is not aligned.\n", __FUNCTION__);
		return -ATBM_EINVAL;
	}

	/* Convert to SDIO Register Address */
	addr_sdio = addr;//SPI_REG_ADDR_TO_SDIO(addr);
	sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	return hw_priv->sbus_ops->sbus_write_sync(hw_priv->sbus_priv,
						  sdio_reg_addr_17bit,
						  buf, buf_len);

}
int __atbm_reg_read_dpll(struct atbmwifi_common *hw_priv, atbm_uint16 addr,
			 const atbm_void *buf, atbm_uint32 buf_len,int buf_id)
{
		atbm_uint16 addr_sdio;
		atbm_uint32 sdio_reg_addr_17bit ;

		//wifi_printk(WIFI_IF, "%x,addr,func=s\n",addr,__FUNCTION__);

		/* Convert to SDIO Register Address */
		addr_sdio = addr;//SPI_REG_ADDR_TO_SDIO(addr);
		sdio_reg_addr_17bit = SDIO_ADDR17BIT(buf_id, 0, 0, addr_sdio);
		//wifi_printk(WIFI_IF, "%x,sdio_reg_addr_17bit,func=%s\n",sdio_reg_addr_17bit,__FUNCTION__);

		ATBM_BUG_ON(!hw_priv->sbus_ops);
		return hw_priv->sbus_ops->sbus_read_sync(hw_priv->sbus_priv,
							sdio_reg_addr_17bit,
							(atbm_void*)buf, buf_len);


}
int atbm_reg_read_dpll(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	ret = __atbm_reg_read_dpll(hw_priv, addr, buf, buf_len, 0);
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int atbm_reg_write_dpll(struct atbmwifi_common *hw_priv, atbm_uint16 addr, const atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	ret = __atbm_reg_write_dpll(hw_priv, addr, buf, buf_len, 0);
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}


int atbm_reg_read(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	ret = __atbm_reg_read(hw_priv, addr, buf, buf_len);
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int atbm_reg_write(struct atbmwifi_common *hw_priv, atbm_uint16 addr, const atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	ret = __atbm_reg_write(hw_priv, addr, buf, buf_len);
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int atbm_data_read(struct atbmwifi_common *hw_priv, atbm_void *buf, atbm_uint32 buf_len)
{
	int ret, retry = 1;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	{
		int buf_id_rx = hw_priv->buf_id_rx;
		while (retry <= MAX_RETRY) {
			ret = __atbm_data_read(hw_priv,
					ATBM_HIFREG_IN_OUT_QUEUE_REG_ID, buf,
					buf_len, buf_id_rx + 1);
			if (!ret) {
				buf_id_rx = (buf_id_rx + 1) & 3;
				hw_priv->buf_id_rx = buf_id_rx;
				break;
			} else {
				retry++;
				atbm_mdelay(1);
				wifi_printk(WIFI_IF, "%s,error :[%d]\n",
						__FUNCTION__, ret);
			}
		}
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

//sdio
int atbm_data_write(struct atbmwifi_common *hw_priv, const atbm_void *buf,
			atbm_size_t buf_len)
{
	int ret, retry = 1;
	int buf_id_tx;

	ATBM_BUG_ON(!hw_priv->sbus_ops);
	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	buf_id_tx = hw_priv->buf_id_tx;
	while (retry <= MAX_RETRY) {
		ret = __atbm_data_write(hw_priv,
				ATBM_HIFREG_IN_OUT_QUEUE_REG_ID, buf,
				buf_len, buf_id_tx);

			if (!ret) {
//				buf_id_tx =  ((buf_id_tx + 1) >= hw_priv->wsm_caps_numInpChBufs)? 0: (buf_id_tx + 1) ;
				buf_id_tx = (buf_id_tx+hw_priv->buf_id_offset)&(0x3f);
				hw_priv->buf_id_tx = buf_id_tx;
				break;
			} else {
				retry++;
				atbm_mdelay(1);
			wifi_printk(WIFI_IF, "%s,%d,error :[%d]\n",
					__FUNCTION__, __LINE__, ret);
		}
	}
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}


//usb
int atbm_indirect_read(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_void *buf,
			 atbm_uint32 buf_len, atbm_uint32 prefetch, atbm_uint16 port_addr)
{
	atbm_uint32 val32 = 0;
	int i, ret;

	if ((buf_len / 2) >= 0x1000) {
		wifi_printk(WIFI_IF,
				"%s: Can't read more than 0xfff words.\n",
				__FUNCTION__);
		ATBM_WARN_ON_FUNC(1);
		return -ATBM_EINVAL;
		goto out;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
	/* Write address */
	ret = __atbm_reg_write_32(hw_priv, ATBM_HIFREG_SRAM_BASE_ADDR_REG_ID,
					addr);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
				"%s: Can't write address register.\n",
				__FUNCTION__);
		goto out;
	}

	/* Read CONFIG Register Value - We will read 32 bits */
	ret = __atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
				"%s: Can't read config register.\n",
				__FUNCTION__);
		goto out;
	}

	/* Set PREFETCH bit */
	ret = __atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,
					val32 | prefetch);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
				"%s: Can't write prefetch bit.\n",
				__FUNCTION__);
		goto out;
	}

	/* Check for PRE-FETCH bit to be cleared */
	for (i = 0; i < 20; i++) {
		ret = __atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,
					   &val32);
		if (ret < 0) {
			wifi_printk(WIFI_IF,
					"%s: Can't check prefetch bit.\n",
					__FUNCTION__);
			goto out;
		}
		if (!(val32 & prefetch))
			break;

		atbm_mdelay(i);
	}

	if (val32 & prefetch) {
		wifi_printk(WIFI_IF,
				"%s: Prefetch bit is not cleared.\n",
				__FUNCTION__);
		goto out;
	}

	/* Read data port */
	ret = __atbm_reg_read(hw_priv, port_addr, buf, buf_len);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
				"%s: Can't read data port.\n",
				__FUNCTION__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}

int atbm_apb_write(struct atbmwifi_common *hw_priv, atbm_uint32 addr, const atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;

	if ((buf_len / 2) >= 0x1000) {
		wifi_printk(WIFI_IF,
				"%s: Can't wrire more than 0xfff words.\n",
				__FUNCTION__);
		ATBM_WARN_ON_FUNC(1);
		return -ATBM_EINVAL;
	}

	hw_priv->sbus_ops->lock(hw_priv->sbus_priv);

	/* Write address */
	ret = __atbm_reg_write_32(hw_priv, ATBM_HIFREG_SRAM_BASE_ADDR_REG_ID,
					addr);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
				"%s: Can't write address register.\n",
				__FUNCTION__);
		goto out;
	}

	/* Write data port */
	ret = __atbm_reg_write(hw_priv, ATBM_HIFREG_SRAM_DPORT_REG_ID,
					buf, buf_len);
	if (ret < 0) {
		wifi_printk(WIFI_IF, "%s: Can't write data port.\n",
				__FUNCTION__);
		goto out;
	}

out:
	hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
	return ret;
}
int atbm_fw_write(struct atbmwifi_common *priv, atbm_uint32 addr, const atbm_void *buf,
						atbm_uint32 buf_len)
{
	return atbm_ahb_write(priv,  addr, buf, buf_len);		
}


int atbm_ahb_write(struct atbmwifi_common *priv, atbm_uint32 addr, const atbm_void *buf,
						atbm_uint32 buf_len)
{
	int ret;
	if (buf_len  >= 512) {
			wifi_printk(WIFI_IF,
							"%s: Can't wrire more than 0xfff words.\n",
							__FUNCTION__);
			ATBM_WARN_ON_FUNC(1);
			return -ATBM_EINVAL;
	}

	priv->sbus_ops->lock(priv->sbus_priv);

	/* Write address */
	ret = __atbm_reg_write_32(priv, ATBM_HIFREG_SRAM_BASE_ADDR_REG_ID, addr);
	if (ret < 0) {
			wifi_printk(WIFI_IF,
							"%s: Can't write address register.\n",
							__FUNCTION__);
			goto out;
	}

	/* Write data port */
	ret = __atbm_reg_write(priv, ATBM_HIFREG_AHB_DPORT_REG_ID,
									buf, buf_len);
	if (ret < 0) {
			wifi_printk(WIFI_IF, "%s: Can't write data port.\n",
							__FUNCTION__);
			goto out;
	}
out:
	priv->sbus_ops->unlock(priv->sbus_priv);

	return ret;
}

int atbm_direct_read_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val)
{
	int ret;
	atbm_uint32 val32;
	atbm_uint32 orig_config_data = 0;

	/* Checking for access mode */
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: can't read " \
			"config register.\n", __FUNCTION__);
		goto out1;
	}
	orig_config_data = val32;
	val32 |= ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT;
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s:  can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}
	ret = atbm_ahb_read_32(hw_priv,addr,val);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s:  can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}

out:
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,orig_config_data);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}
out1:
	return ret;
}


int atbm_direct_write_reg_32(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 val)
{
	int ret;
	atbm_uint32 val32;
	atbm_uint32 orig_config_data = 0;

	/* Checking for access mode */
	ret = atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: can't read " \
			"config register.\n", __FUNCTION__);
		goto out1;
	}
	orig_config_data = val32;
	val32 |= ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT;
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s:  can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}
	ret = atbm_ahb_write_32(hw_priv,addr,val);

	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s:  can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}

out:
	ret = atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,orig_config_data);
	if (ret < 0) {
		wifi_printk(WIFI_IF,
			"%s: enable_irq: can't write " \
			"config register.\n", __FUNCTION__);
		goto out;
	}
out1:
	return ret;
}
int __atbm_irq_enable(struct atbmwifi_common *priv, int enable)
{
	atbm_uint32 val32;
	int ret;

	ret = __atbm_reg_read_32(priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,"Can't read config register.\n");
		return ret;
	}

	if (enable){
		if(val32 & ATBM_HIFREG_CONF_IRQ_RDY_ENABLE)
			return ret;
		val32 |= ATBM_HIFREG_CONF_IRQ_RDY_ENABLE;

	}
	else {
		if((val32 & ATBM_HIFREG_CONF_IRQ_RDY_ENABLE)==0)
			return ret;
		val32 &= ~ATBM_HIFREG_CONF_IRQ_RDY_ENABLE;
	}
	ret = __atbm_reg_write_32(priv, ATBM_HIFREG_CONFIG_REG_ID, val32);
	if (ret < 0) {
		wifi_printk(WIFI_IF,"Can't write config register.\n");
		return ret;
	}
	return 0;
}
int atbm_reg_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint16 addr, atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;
	int retry=0;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	while (retry <= 3) {
		ret = __atbm_reg_read(hw_priv, addr, buf, buf_len);
		if(ret){
			wifi_printk(WIFI_ALWAYS,"%s\n",__func__);
			retry++;
		}else{
			break;
		}
	}
	return ret;
}

int atbm_reg_write_unlock(struct atbmwifi_common *hw_priv, atbm_uint16 addr, const atbm_void *buf,
			atbm_uint32 buf_len)
{
	int ret;
	int retry=0;
	ATBM_BUG_ON(!hw_priv->sbus_ops);
	while (retry <= 3) {
		ret = __atbm_reg_write(hw_priv, addr, buf, buf_len);
		if(ret){
			wifi_printk(WIFI_ALWAYS,"%s\n",__func__);
			retry++;
		}else{
			break;
		}
	}
	return ret;
}
int atbm_direct_write_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 val)
{
    int ret;
	atbm_uint32 val32;
	atbm_uint32 orig_config_data = 0;

	/* Checking for access mode */
	ret = atbm_reg_read_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s: can't read " \
			"config register.\n", __func__);
		goto out1;
	}
	orig_config_data = val32;
	val32 |= ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT;
	ret = atbm_reg_write_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s:  can't write " \
			"config register.\n", __func__);
		goto out;
	}
	ret = atbm_ahb_write_unlock_32(hw_priv,addr,val);

	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s:  can't write " \
			"config register.\n", __func__);
		goto out;
	}

out:
	ret = atbm_reg_write_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,orig_config_data);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s: enable_irq: can't write " \
			"config register.\n", __func__);
		goto out;
	}
out1:
	return ret;
}
int atbm_direct_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr, atbm_uint32 *val)
{
    int ret;
	atbm_uint32 val32;
	atbm_uint32 orig_config_data = 0;

	/* Checking for access mode */
	ret = atbm_reg_read_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s: can't read " \
			"config register.\n", __func__);
		goto out1;
	}
	orig_config_data = val32;
	val32 |= ATBM_HIFREG_CONFIG_ACCESS_MODE_BIT;
	ret = atbm_reg_write_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,val32);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s:  can't write " \
			"config register.\n", __func__);
		goto out;
	}
	ret = atbm_ahb_read_unlock_32(hw_priv,addr,val);
	//printk("val=%x\n",val);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s:  can't write " \
			"config register.\n", __func__);
		goto out;
	}

out:
	ret = atbm_reg_write_unlock_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,orig_config_data);
	if (ret < 0) {
		wifi_printk(WIFI_ALWAYS,
			"%s: enable_irq: can't write " \
			"config register.\n", __func__);
		goto out;
	}
out1:
	return ret;
}
int atbm_ahb_write_unlock(struct atbmwifi_common *priv, atbm_uint32 addr, const void *buf,
                        atbm_uint32 buf_len)
{
        int ret;
		int retry=0;
		//printk(KERN_ERR "%s: addr %x\n",__func__,addr);
        if (buf_len  >= 512) {
                wifi_printk(WIFI_ALWAYS,
                                "%s: Can't wrire more than 0xfff words.\n",
                                __func__);
                ATBM_WARN_ON_FUNC(1);
				wifi_printk(WIFI_ALWAYS, "%s:EXIT (1) \n",__func__);
                return -1;
        }
        /* Write address */
		while(retry<=3){
	        ret = __atbm_reg_write_32(priv, ATBM_HIFREG_SRAM_BASE_ADDR_REG_ID, addr);
	        if (ret < 0) {
	                wifi_printk(WIFI_ALWAYS,
	                                "%s: Can't write address register.\n",
	                                __func__);
					retry++;
	        }else{
				break;
			}
		}
		retry=0;
        /* Write data port */
		while(retry<=3){
	        ret = __atbm_reg_write(priv, ATBM_HIFREG_AHB_DPORT_REG_ID,
	                                        buf, buf_len);
	        if (ret < 0) {
	                wifi_printk(WIFI_ALWAYS, "%s: Can't write data port.\n",
	                                __func__);
					retry++;
	        }else{
				break;
			}
		}
//out:
        return ret;
}
int atbm_indirect_read_unlock(struct atbmwifi_common *hw_priv, atbm_uint32 addr, void *buf,
			 atbm_uint32 buf_len, atbm_uint32 prefetch, atbm_uint16 port_addr)
{
	atbm_uint32 val32 = 0;
	int i, ret;
	int retry=0;
	if ((buf_len / 2) >= 0x1000) {
		wifi_printk(WIFI_ALWAYS,
				"%s: Can't read more than 0xfff words.\n",
				__func__);
		ATBM_WARN_ON_FUNC(1);
		return -1;
		goto out;
	}
	/* Write address */
	
	while(retry<=3){
		ret = __atbm_reg_write_32(hw_priv, ATBM_HIFREG_SRAM_BASE_ADDR_REG_ID,
					    addr);
			if (ret < 0) {
				wifi_printk(WIFI_ALWAYS,
						"%s: Can't write address register.\n",
						__func__);
				retry++;
			}else{
				break;
			}
	}

	/* Read CONFIG Register Value - We will read 32 bits */
	retry=0;
	while(retry<=3){
		ret = __atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID, &val32);
		if (ret <0) {
			wifi_printk(WIFI_ALWAYS,
					"%s: Can't read config register.\n",
					__func__);
			retry++;
		}else{
			break;
		}
	}

	/* Set PREFETCH bit */
	retry=0;
	while(retry<=3){
	ret = __atbm_reg_write_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,
						val32 | prefetch);
		if (ret < 0) {
			wifi_printk(WIFI_ALWAYS,
					"%s: Can't write prefetch bit.\n",
					__func__);
			retry++;
		}else{
			break;
		}
	}

	/* Check for PRE-FETCH bit to be cleared */
	for (i = 0; i < 20; i++) {
		ret = __atbm_reg_read_32(hw_priv, ATBM_HIFREG_CONFIG_REG_ID,
					   &val32);
		if (ret < 0) {
			wifi_printk(WIFI_ALWAYS,
					"%s: Can't check prefetch bit.\n",
					__func__);
			atbm_mdelay(i);
			continue;
		}
		if (!(val32 & prefetch))
			break;

		atbm_mdelay(i);
	}

	if (val32 & prefetch) {
		wifi_printk(WIFI_ALWAYS,
				"%s: Prefetch bit is not cleared.\n",
				__func__);
		goto out;
	}

	/* Read data port */
	retry=0;
	while(retry<=3){
		ret = __atbm_reg_read(hw_priv, port_addr, buf, buf_len);
		if (ret < 0) {
			wifi_printk(WIFI_ALWAYS,
					"%s: Can't read data port.\n",
					__func__);
			retry++;
		}else{
			break;
		}
	}
out:
	return ret;
}

#if (PROJ_TYPE>=ARES_B)
//just ARESB have this function
//used this function to clear sdio rtl bug register
// if not do this sdio direct mode (wr/read reigster) will not work
int atbm_data_force_write(struct atbmwifi_common *hw_priv, const void *buf,
                        int buf_len)
{
        int ret, retry = 1;
        int buf_id_tx;
        ATBM_BUG_ON(!hw_priv->sbus_ops);
        hw_priv->sbus_ops->lock(hw_priv->sbus_priv);
        buf_id_tx = ((hw_priv->buf_id_tx-1)&0x3f)+64;
		wifi_printk(WIFI_ALWAYS,"buf_id_tx =%d %s\n",buf_id_tx,__func__);
        while (retry <= MAX_RETRY) {
                ret = __atbm_data_write(hw_priv,
                                ATBM_HIFREG_IN_OUT_QUEUE_REG_ID, buf,
                                buf_len, buf_id_tx);

                if (!ret) {
                        buf_id_tx =  ((buf_id_tx + hw_priv->buf_id_offset)&0x3f);
                        hw_priv->buf_id_tx = buf_id_tx;
                        break;
                } else {
                        retry++;
                        atbm_mdelay(1000);
                       
                }
        }
        hw_priv->sbus_ops->unlock(hw_priv->sbus_priv);
        return ret;
}
#endif //#if (PROJ_TYPE>=ARES_B)

