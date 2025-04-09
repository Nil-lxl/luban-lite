/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/
#include "atbm_hal.h"
#include "atbm_sbus.h"
#include "atbm_os_api.h"
#include "../../include/svn_version.h"
#if (ATBM_PLATFORM==JIANRONG_RTOS_3268)
#elif (ATBM_PLATFORM==JIANRONG_RTOS_3298)
#include "sdcard.h"
#elif (ATBM_PLATFORM==AK_RTOS_300) || (ATBM_PLATFORM==AK_RTOS_37D)
#include "interrupt.h"
#endif
extern atbm_void atbm_core_release(struct atbmwifi_common *hw_priv);

#define DPLL_CLOCK 24
struct build_info{
	int ver;
	int dpll;
	char driver_info[64];
};
const char DRIVER_INFO[]={"[===SDIO-ATHENAB=="};
static int driver_build_info(atbm_void)
{
	struct build_info build;
	build.ver=SVN_VERSION;
	build.dpll=DPLL_CLOCK;
	atbm_memcpy(build.driver_info,(atbm_void*)DRIVER_INFO,sizeof(DRIVER_INFO));
	wifi_printk(WIFI_DBG_ANY,"SVN_VER=%d,DPLL_CLOCK=%d,BUILD_TIME=%s\n",build.ver,build.dpll,build.driver_info);
	return 0;
}

int atbm_sdio_suspend(struct atbmwifi_common *hw_priv)
{
	return 0;
}
int atbm_sdio_resume(struct atbmwifi_common *hw_priv)
{
	return 0;
}

static int atbm_sdio_memcpy_fromio(struct sbus_priv *self,
				     unsigned int addr,
				     void *dst, int count)
{
	return __atbm_sdio_memcpy_fromio(self->func, dst, addr, count);
}
static int atbm_sdio_memcpy_toio(struct sbus_priv *self,
				   unsigned int addr,
				   const void *src, int count)
{
	return __atbm_sdio_memcpy_toio(self->func, addr, (void *)src, count);
}
static void atbm_sdio_lock(struct sbus_priv *self)
{
	atbm_sdio_claim_host(self->func);
}
static void atbm_sdio_unlock(struct sbus_priv *self)
{
	atbm_sdio_release_host(self->func);
}
#if(ATBM_PLATFORM==JIANRONG_RTOS_3298)
extern struct mmc_host bw_mmc_host;
static int atbm_sdio_irq_handler (atbm_uint32 irq, struct atbm_sdio_func *func, void *reg)

{
	struct mmc_host *host;
    host = &bw_mmc_host;
	if(sdio_int_check(host->controller))
	{
		struct sbus_priv *self = atbm_sdio_get_drvdata(func);
		sdc_testio_low(4);
		sdio_clr_irq_pending(host);
		ATBM_BUG_ON(!self);
		if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	}
}
#elif (ATBM_PLATFORM==JIANRONG_RTOS_3268)
static void atbm_sdio_irq_handler(struct atbm_sdio_func *func)
{
	EXT_INT ext_int;
	//ENTER();
#if (SYS_CHIP_MODULE == APPO_TIGA)
	ext_int = EXT_INT2;
#elif (SYS_CHIP_MODULE == APPO_VISION)
	ext_int = EXT_INT6;
#endif
	//printf("port isr kick\n");
	if(EXT_INT_CHK_PEND(ext_int))
	{
		struct sbus_priv *self = atbm_sdio_get_drvdata(func);
       // printf("port isr ok:%x\n",self->irq_handler);
		EXT_INT_CLR_PEND(ext_int);
		ATBM_BUG_ON(!self);
		
		if (self->irq_handler)
			self->irq_handler(self->irq_priv);
		
		//printf("port isr end:%x\n");

	}

}
#else
static void atbm_sdio_irq_handler(struct atbm_sdio_func *func)
{
	struct sbus_priv *self = atbm_sdio_get_drvdata(func);

	ATBM_BUG_ON(!self);

	if (self->irq_handler)
		self->irq_handler(self->irq_priv);

}
#endif
static int atbm_sdio_irq_subscribe(struct sbus_priv *self,
				     sbus_irq_handler handler,
				     void *priv)
{
	int ret;
	unsigned long flags;

	if (!handler)
		return -ATBM_EINVAL;

	atbm_spin_lock_irqsave(&self->lock, &flags);
	self->irq_priv = priv;
	self->irq_handler = handler;
	atbm_spin_unlock_irqrestore(&self->lock, flags);
	atbm_sdio_claim_host(self->func);
	ret = atbm_sdio_claim_irq(self->func, atbm_sdio_irq_handler);
	if (ret)
		wifi_printk(WIFI_IF,"Failed to claim sdio Irq :%d\n",ret);
	atbm_sdio_release_host(self->func);
	return ret;
}

static void atbm_sdio_irq_en(struct sbus_priv *self,atbm_uint8 en)
{

}
static int atbm_sdio_irq_unsubscribe(struct sbus_priv *self)
{
	int ret = 0;
	unsigned long flags;
	//const struct resource *irq = self->pdata->irq;

	ATBM_WARN_ON_FUNC(!self->irq_handler);
	if (!self->irq_handler)
		return 0;

	atbm_sdio_claim_host(self->func);
	ret = atbm_sdio_release_irq(self->func);
	atbm_sdio_release_host(self->func);
	atbm_spin_lock_irqsave(&self->lock, &flags);
	self->irq_priv = ATBM_NULL;
	self->irq_handler = ATBM_NULL;
	atbm_spin_unlock_irqrestore(&self->lock, flags);

	return ret;
}

static int atbm_sdio_reset(struct sbus_priv *self)
{
	int ret;
	int regdata;
	int func_num;

	return 0;
	wifi_printk(WIFI_IF,"atbm_sdio_reset++\n");
	atbm_sdio_claim_host(self->func);

	/**********************/
	wifi_printk(WIFI_IF,"SDIO_RESET++\n");
	/* SDIO Simplified Specification V2.0, 4.4 Reset for SDIO */
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_ABORT, &ret);
	if (ret)
		regdata = 0x08;
	else
		regdata |= 0x08;
	atbm_sdio_f0_writeb(self->func, regdata, ATBM_SDIO_CCCR_ABORT, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;
	atbm_mdelay(1500);
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_ABORT, &ret);
	wifi_printk(WIFI_IF,"SDIO_RESET-- 0x%x\n",regdata);

	/**********************/
	wifi_printk(WIFI_IF,"ATBM_SDIO_SPEED_EHS++\n");
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_SPEED, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;

	regdata |= ATBM_SDIO_SPEED_EHS;
	atbm_sdio_f0_writeb(self->func, regdata, ATBM_SDIO_CCCR_SPEED, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;

	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_SPEED, &ret);
	wifi_printk(WIFI_IF,"ATBM_SDIO_SPEED_EHS -- 0x%x:0x%x\n",regdata,ATBM_SDIO_SPEED_EHS);

	/**********************/
	wifi_printk(WIFI_IF,"ATBM_SDIO_BUS_WIDTH_4BIT++\n");
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_IF, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;

	//regdata |= ATBM_SDIO_BUS_WIDTH_4BIT;
	regdata = 0xff;
	atbm_sdio_f0_writeb(self->func, regdata, ATBM_SDIO_CCCR_IF, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_IF, &ret);
	wifi_printk(WIFI_IF,"ATBM_SDIO_BUS_WIDTH_4BIT -- 0x%x:0x%x\n",regdata,ATBM_SDIO_BUS_WIDTH_4BIT);
	/**********************/
	wifi_printk(WIFI_IF,"SDIO_BUS_ENABLE_FUNC++\n");
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_IOEx, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;
	regdata |= BIT(func_num);
	wifi_printk(WIFI_IF,"SDIO_BUS_ENABLE_FUNC regdata %x\n",regdata);
	atbm_sdio_f0_writeb(self->func, regdata, ATBM_SDIO_CCCR_IOEx, &ret);
	if (ATBM_WARN_ON(ret))
		goto set_func0_err;
	regdata = atbm_sdio_f0_readb(self->func, ATBM_SDIO_CCCR_IOEx, &ret);
	wifi_printk(WIFI_IF,"SDIO_BUS_ENABLE_FUNC -- 0x%x\n",regdata);
	/**********************/
set_func0_err:
	atbm_sdio_set_block_size(self,ATBM_SDIO_BLOCK_SIZE);
	/* Restore the WLAN function number */
	atbm_sdio_release_host(self->func);
	return 0;
}
static atbm_uint32 atbm_sdio_align_size(struct sbus_priv *self, atbm_uint32 size)
{
	atbm_uint32 aligned = atbm_sdio_alignsize(self->func, size);
	return aligned;
}
int atbm_sdio_set_block_size(struct sbus_priv *self, atbm_uint32 size)
{
	return atbm_sdio_set_blocksize(self->func, size);
}
static int atbm_sdio_pm(struct sbus_priv *self, ATBM_BOOL  suspend)
{
	int ret = 0;
	return ret;
}
/* Probe Function to be called by SDIO stack when device is discovered */
int atbm_sdio_probe(struct atbm_sdio_func *func,
			      const struct atbm_sdio_device_id *id)
{
	struct sbus_priv *self;
	int ret;

	wifi_printk(WIFI_IF,"Probe called\n");
	
	//atbm_atomic_set(&g_wtd.wtd_probe, 0);
#ifdef LINUX_OS
	func->card->quirks|=MMC_QUIRK_LENIENT_FN0;
	func->card->quirks |= MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
#endif

	self = (struct sbus_priv *)atbm_kzalloc(sizeof(*self), GFP_KERNEL);
	if (!self) {
		wifi_printk(WIFI_DBG_ERROR, "Can't allocate SDIO sbus_priv.");
		return -1;
	}
	atbm_spin_lock_init(&self->lock);
	self->func = func;
	//self->wtd = &g_wtd;
	atbm_sdio_set_drvdata(func, self);
	atbm_sdio_claim_host(func);
	ret=atbm_sdio_enable_func(func);
	if(ret){
		atbm_sdio_disable_func(func);
		atbm_sdio_release_host(func);
		atbm_kfree(self);
		return -1;
	}
	atbm_sdio_release_host(func);
	ret=Atbmwifi_halEntry(self);
	if (ret) {
		atbm_kfree(self);
		atbm_sdio_claim_host(func);
		atbm_sdio_disable_func(func);
		atbm_sdio_release_host(func);
		atbm_sdio_set_drvdata(func, NULL);
		//atbm_atomic_set(&g_wtd.wtd_probe, -1);
	}
	else {
		//atbm_atomic_set(&g_wtd.wtd_probe, 1);
		wifi_printk(WIFI_IF,"[atbm_wtd]:set wtd_probe = 1\n");
	}
	return ret;
}
void atbm_sdio_disconnect(struct atbm_sdio_func *func)
{
	struct sbus_priv *self = atbm_sdio_get_drvdata(func);
	wifi_printk(WIFI_IF,"atbm_sdio_disconnect");
	if (self) {
		//atbm_atomic_set(&g_wtd.wtd_probe, 0);
		if (self->core) {
			atbm_core_release(self->core);
			self->core->sbus_ops->irq_unsubscribe(self->core->sbus_priv);
			//self->core = ATBM_NULL;
		}
		atbm_sdio_claim_host(func);
		/*
		*	reset sdio
		*/
		{
			int ret;
			int regdata;
			/**********************/
//			wifi_printk(WIFI_IF,"[%s]:SDIO_RESET++\n",dev_name(&func->card->host->class_dev));
			/* SDIO Simplified Specification V2.0, 4.4 Reset for SDIO */
			regdata = atbm_sdio_f0_readb(func, ATBM_SDIO_CCCR_ABORT, &ret);
			if (ret)
				regdata = 0x08;
			else
				regdata |= 0x08;
			atbm_sdio_f0_writeb(func, regdata, ATBM_SDIO_CCCR_ABORT, &ret);
			ATBM_WARN_ON_FUNC(ret);
			atbm_mdelay(50);
			regdata = atbm_sdio_f0_readb(func, ATBM_SDIO_CCCR_ABORT, &ret);
//			wifi_printk(WIFI_IF,"[%s]:SDIO_RESET-- 0x%x\n",dev_name(&func->card->host->class_dev),regdata);

			/**********************/
		}
		if (self->core) {
			self->core = ATBM_NULL;
		}
		atbm_sdio_disable_func(func);
		atbm_sdio_release_host(func);
		atbm_sdio_set_drvdata(func, ATBM_NULL);
		atbm_kfree(self);
	}
}
struct sbus_ops atbm_sdio_sbus_ops;
static int  atbm_sdio_init(atbm_void)
{
	atbm_sdio_sbus_ops.sbus_memcpy_fromio	= atbm_sdio_memcpy_fromio;
	atbm_sdio_sbus_ops.sbus_memcpy_toio	= atbm_sdio_memcpy_toio;
	atbm_sdio_sbus_ops.sbus_read_sync		= atbm_sdio_memcpy_fromio;
	atbm_sdio_sbus_ops.sbus_write_sync	= atbm_sdio_memcpy_toio;
	atbm_sdio_sbus_ops.lock				= atbm_sdio_lock;
	atbm_sdio_sbus_ops.unlock				= atbm_sdio_unlock;
	atbm_sdio_sbus_ops.reset				= atbm_sdio_reset;
	atbm_sdio_sbus_ops.align_size			= atbm_sdio_align_size;
	atbm_sdio_sbus_ops.power_mgmt			= atbm_sdio_pm;
	atbm_sdio_sbus_ops.set_block_size		= atbm_sdio_set_block_size;
	atbm_sdio_sbus_ops.irq_unsubscribe	= atbm_sdio_irq_unsubscribe;
	atbm_sdio_sbus_ops.irq_subscribe	= atbm_sdio_irq_subscribe;
	atbm_sdio_sbus_ops.abort	= ATBM_NULL;
	atbm_sdio_sbus_ops.sdio_irq_en = atbm_sdio_irq_en;

	wifi_printk(WIFI_IF, "atbm_sdio_init\n");
	driver_build_info();

	return atbm_sdio_register_init();
}
static atbm_void  atbm_sdio_exit(atbm_void)
{
	atbm_sdio_register_deinit();
	wifi_printk(WIFI_IF,"atbm_usb_exit:atbm_sdio_exit\n");
}
atbm_void atbm_sdio_module_init(atbm_void)
{
	wifi_printk(WIFI_IF, "atbm_sdio_module_init\n");
	atbm_init_firmware();
	wifi_printk(WIFI_ALWAYS,"[Wifi] Enter %s \n", __func__);
	atbm_sdio_init();
}
atbm_void atbm_sdio_module_exit(atbm_void)
{
	wifi_printk(WIFI_IF,"atbm_sdio_module_exit\n");
	atbm_sdio_exit();
	atbm_release_firmware();
	return ;
}

