
#include <drivers/mmcsd_core.h>
#include <drivers/mmcsd_card.h>
#include "asr_sdio.h"
#include <drivers/sdio.h>
#include "asr_gpio.h"
#include "asr_rtos_api.h"
#include "asr_types.h"
#include "asr_dbg.h"
#include "uwifi_sdio.h"
#include "asr_gpio.h"
#include <aic_log.h>


#define MIN(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))

#define ASR_FIRMWARE_ADDR           0x14
#define READ_DIRECTION              0
#define WRITE_DIRECTION             1
#define SDIO_DEFAULT_BLOCK_SIZE     512

static uint32_t s_real_block_size = SDIO_DEFAULT_BLOCK_SIZE;


asr_sbus g_asr_sbus_drvdata;


extern void asr_wifi_set_init_done();


void sdio_claim_host(asr_drv_sdio_t  *func)
{
    mmcsd_host_lock(func->card->host);
}

void sdio_release_host(asr_drv_sdio_t *func)
{
    mmcsd_host_unlock(func->card->host);
}

void asr_sdio_claim_host(void)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    if (func) {
        mmcsd_host_lock(func->card->host);
    }
}

void asr_sdio_release_host(void)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    if (func) {
        mmcsd_host_unlock(func->card->host);
    }
}


unsigned int asr_sdio_get_block_size(void)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    // be_warn("currntt block size: %d", func->cur_blk_size);
    return func->cur_blk_size;
}

int asr_sdio_set_block_size(unsigned int blksz)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    func->card->host->max_blk_size = blksz;
    return sdio_set_block_size(func, blksz);
}

int sdio_enable_func__(unsigned int func_num)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    return sdio_enable_func(func->card->sdio_function[func_num]);
}

int sdio_disable_func__(unsigned int func_num)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    return sdio_disable_func(func->card->sdio_function[func_num]);
}

unsigned char sdio_readb(asr_drv_sdio_t *card, unsigned int func_num, unsigned int addr, int *err_ret)
{
    asr_drv_sdio_t *func = card;
    return sdio_io_readb(func->card->sdio_function[func_num], addr, err_ret);
}

void sdio_writeb(asr_drv_sdio_t *card, unsigned int func_num, unsigned char b, unsigned int addr, int  *err_ret)
{
    int ret;
    asr_drv_sdio_t *func = card;

    ret = sdio_io_writeb(func->card->sdio_function[func_num], addr, b);
    if (err_ret) {
        *err_ret = ret;
    }
}


static int sdio_write_firmware(unsigned int addr, void *src, int count, uint8_t func_num)
{
    uint32_t remainder = count;
    int ret = -1;
    uint32_t incr_addr = 1; // non-zero indicates increase address, 0 keep fixed
    uint32_t blocks;
    int fn_bsize = s_real_block_size;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    uint8_t *firmware_buf = asr_rtos_malloc(1024);
    if (firmware_buf == NULL) {
        return -ENOMEM;
    }

    while (remainder >= fn_bsize) {
        blocks = remainder / fn_bsize;
        count = blocks * fn_bsize;
        memcpy(firmware_buf, src, count);

        ret = asr_sdio_memcpy_toio(func, func_num, addr, src, count);
        if (ret) {
            pr_err("%s[%d] error! ret=%d\n", __FUNCTION__, __LINE__, ret);
            asr_rtos_free(firmware_buf);
            return ret;
        }
        remainder -= count;
        src = (uint8_t *)src + count;
        if (incr_addr)
            addr += count;
    }

    while (remainder > 0) {
        count = MIN(remainder, fn_bsize);

        memcpy(firmware_buf, src, count);

        ret = asr_sdio_memcpy_toio(func, func_num, addr, src, count);
        if (ret) {
            pr_err("%s[%d] error! ret=%d\n", __FUNCTION__, __LINE__, ret);
            asr_rtos_free(firmware_buf);
            return ret;
        }
        remainder -= count;
        src = (uint8_t *)src + count;
        if (incr_addr)
            addr += count;
    }
    asr_rtos_free(firmware_buf);
    return ret;
}

int asr_sdio_memcpy_fromio(asr_drv_sdio_t *card, unsigned int func_num, void *dst, unsigned int addr, int count)
{
    asr_drv_sdio_t *func = card;
    unsigned int size = count;

    size = nx_sdio_align_size(func->card->sdio_function[func_num], size);
    return sdio_io_rw_extended_block(func->card->sdio_function[func_num], 0, addr, 0, dst, size);
}

int asr_sdio_memcpy_toio(asr_drv_sdio_t *card, unsigned int func_num, unsigned int addr, void *src, int count)
{
    asr_drv_sdio_t *func = card;
    unsigned int size = count;

    size = nx_sdio_align_size(func->card->sdio_function[func_num], size);
    return sdio_io_rw_extended_block(func->card->sdio_function[func_num], 1, addr, 0, src, size);
}

int SDIO_CMD52(int write, unsigned int fn, unsigned int addr, unsigned char in, unsigned char*out)
{
    int ret;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    if (write) {
        ret = sdio_io_writeb(func->card->sdio_function[fn], addr, in);
    }
    else {
        *out = sdio_io_readb(func->card->sdio_function[fn], addr, &ret);
    }

    return ret;
}



// adapter for asr wifi driver
unsigned char sdio_writeb_cmd52(unsigned int addr, unsigned char data)
{
    int err_ret = 0;
    int sdiofnnum = 1;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    sdio_writeb(func, sdiofnnum, data, addr, &err_ret);
    return err_ret==0?1:0;;
}

unsigned char sdio_readb_cmd52(unsigned int addr, unsigned char *data)
{
    int err_ret = 0;
    int sdiofnnum = 1;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    *data = sdio_readb(func, sdiofnnum, addr, &err_ret);
    return err_ret==0?1:0;
}

static int sdio_memcpy_toio__(asr_drv_sdio_t *card, unsigned int func_num,
                        unsigned int addr, void *src, int count)
{
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;
    if((((uintptr_t)src) &3)||(count &3)){
        pr_err("ASR write Data & Len must be 4 align,dst=0x%08x,count=%d",(uintptr_t)src,count);
    }

    if (addr == ASR_FIRMWARE_ADDR) {
        return sdio_write_firmware(addr, src, count, func_num);
    }
    return sdio_io_write_multi_fifo_b(func->card->sdio_function[func_num], addr, src,  count);
}

// adapter for asr wifi driver
unsigned char sdio_read_cmd53(unsigned int dataport, unsigned char *data, int size)
{
    int ret = 0;
    int sdiofnnum = 1;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    if((((uintptr_t)data) &3)||(size &3)){
        pr_err("ASR write Data & Len must be 4 align,dst=0x%08x,count=%d",(uintptr_t)data,size);
    }

    ret = sdio_io_read_multi_fifo_b(func->card->sdio_function[sdiofnnum], dataport, data, size);
    if (ret)
    {
        pr_err("%s[%d] error! ret=%d\n", __FUNCTION__, __LINE__, ret);
    }

    return ret;
}

unsigned char sdio_write_cmd53(unsigned int dataport, unsigned char *data, int size)
{
    int ret = 0;
    int sdiofnnum = 1;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    ret = sdio_memcpy_toio__(func, sdiofnnum,dataport, data, size);
    if (ret)
    {
        pr_err("%s[%d] error! ret=%d\n", __FUNCTION__, __LINE__, ret);
    }

    return ret;
}

unsigned char sdio_host_enable_isr(unsigned char enable)
{
    if (enable)
        asr_wlan_irq_enable();
    else
        asr_wlan_irq_disable();
    return 1;
}

void asr_wlan_irq_disable(void)
{
}

void asr_wlan_irq_enable(void)
{
}

void asr_wlan_irq_enable_clr(void)
{
}

void asr_wlan_irq_subscribe(void *handle)
{
    int ret = 0;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    if (func) {
        ret = sdio_attach_irq(func, handle);
        pr_err("%s:ret=%d\n", __func__, ret);
    }
}

void asr_wlan_irq_unsubscribe(void)
{
    int ret = 0;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    if (func) {
        ret = sdio_detach_irq(func);
        pr_info("%s:ret=%d\n", __func__, ret);
    }
}

static void asr_sdio_card_hisr_func(asr_drv_sdio_t *func)
{
    return sdio_card_gpio_hisr_func();
}

int asr_sdio_claim_irq(void)
{
    int ret = -1;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    if (func) {
        ret = sdio_attach_irq(func, asr_sdio_card_hisr_func);
        pr_info("%s:ret=%d\n", __func__, ret);
    }

    return ret;
}

int asr_sdio_release_irq(void)
{
    int ret = -1;
    asr_drv_sdio_t *func = g_asr_sbus_drvdata.func;

    if (func) {
        ret = sdio_detach_irq(func);
        pr_info("%s:ret=%d\n", __func__, ret);
    }

    return ret;
}

//FIXME
static int asr_sdio_bus_detect_change(void)
{
    int ret = -1;

    if (g_asr_sbus_drvdata.func == 0) {
        return -1;
    }

    pr_debug("%s: host=%p!\n", __func__, g_asr_sbus_drvdata.func->card->host);

    mmcsd_change(g_asr_sbus_drvdata.func->card->host);

    ret = mmcsd_wait_cd_changed(1000);
    if (ret < 0) {
        pr_debug("%s: detect failed host=%p ret=%d!\n",
            __func__, g_asr_sbus_drvdata.func->card->host, ret);
        return ret;
    }

    return ret;
}

int asr_sdio_bus_probe(void)
{
    int ret = -1;

    pr_debug("%s: detect card start!\n", __func__);

    ret = asr_sdio_bus_detect_change();
    if (ret < 0) {
        return ret;
    }
    if (ret == MMCSD_HOST_PLUGED) {
        pr_debug("%s: detect sdio card ret=%d!\n", __func__, ret);
        return 0;
    }

    ret = asr_sdio_bus_detect_change();
    if (ret < 0) {
        return ret;
    }
    if (ret == MMCSD_HOST_PLUGED) {
        pr_debug("%s: detect card success!\n", __func__);
        ret = 0;
    } else {
        pr_debug("%s: detect card fail ret=%d!\n", __func__, ret);
    }

    return ret;
}

int asr_sdio_dev_init(void)
{
    //int ret = 0;

    memset(&g_asr_sbus_drvdata, 0, sizeof(asr_sbus));

    //获取gpio_power引脚
    //asr_gpio_init();

#if 0
    ret = asr_wlan_gpio_reset();
    if (ret < 0) {
        return ret;
    }

    ret = asr_sdio_bus_probe();
    if (ret < 0) {
        pr_debug("%s: asr_sdio_bus_probe failed ret=%d!", __func__, ret);
        //return ret;
    }
#endif
    return 0;
}

unsigned char sdio_read_cmd53_reg(unsigned int dataport, unsigned char *data, int size)
{
    return sdio_read_cmd53(dataport, data, size);
}

rt_int32_t asr_sdio_probe(struct rt_mmcsd_card *card)
{
    int ret;
    struct rt_sdio_function *func = card->sdio_function[1];

    g_asr_sbus_drvdata.func = func;

    //func->card->host->ops->enable_sdio_irq(func->card->host, 1);

    pr_debug("%s: host=%p!\n", __func__, g_asr_sbus_drvdata.func->card->host);

    asr_sdio_set_block_size(SDIO_BLOCK_SIZE_DLD);
    sdio_set_drvdata(func, &g_asr_sbus_drvdata);

    sdio_claim_host(func);
    ret = sdio_enable_func__(func->num);
    if (ret) {
        sdio_disable_func__(func->num);
        sdio_release_host(func);
        return kGeneralErr;
    }
    sdio_release_host(func);

    // asr_wifi_set_init_done();

    pr_debug("%s[%d]blk_size=%d,freq_max=%d,flags=0x%X,(%X,%X)\n",
        __func__, __LINE__, func->cur_blk_size,card->host->freq_max,card->host->flags,
        card->cis.manufacturer, card->cis.product);

    uwifi_platform_init();

    ret = asr_wlan_init();
    if (ret) {
        pr_debug("%s: failed ret=%d!", __func__, ret);
        return kGeneralErr;
    }

    pr_debug("%s: probe success!\n", __func__);

    return kNoErr;
}

rt_int32_t asr_sdio_remove(struct rt_mmcsd_card *card)
{
    struct rt_sdio_function *func = card->sdio_function[1];

    pr_info("ASR sdio deinit called\n");

    uwifi_platform_deinit_api();

    sdio_claim_host(func);
    sdio_disable_func__(func->num);
    sdio_set_drvdata(func, NULL);
    sdio_release_host(func);

    return kNoErr;
}

static struct rt_sdio_device_id asr_sdio_id[] = {
	{
		.func_code	= 1,
		.manufacturer = 0x424c,
		.product = 0x6006,
	},
    {}
};

static struct rt_sdio_driver asr_wifi_driver = {
    .name = "ASR5825S",
    .probe = asr_sdio_probe,
    .remove = asr_sdio_remove,
    .id = asr_sdio_id,
};

OSStatus asr_sdio_register_driver(void)
{
    int ret;

    ret = sdio_register_driver(&asr_wifi_driver);
    if (ret) {
        pr_err("ASR wifi driver register fail!! ret=%d\n", ret);
        return kGeneralErr;
    }
    return kNoErr;
}

OSStatus asr_sdio_unregister_driver()
{
    int ret;

	ret = sdio_unregister_driver(&asr_wifi_driver);
    if (ret) {
        pr_err("ASR wifi driver unregister fail!!\n");
        return kGeneralErr;
    }
    return kNoErr;
}

int asr_sdio_register_driver_my(void)
{
    int ret = 0;

    asr_gpio_init();

    asr_wlan_set_gpio_reset_pin(0);
    asr_msleep(10);
    asr_wlan_set_gpio_reset_pin(1);
    asr_msleep(10);

    ret = sdio_register_driver(&asr_wifi_driver);
    if (ret) {
        pr_err("ASR wifi driver probe fail,ret=%x,wait card probe.\n", ret);
        return ret;
    }
    return ret;
}
//INIT_COMPONENT_EXPORT(asr_sdio_register_driver_my);

