#include "atbm_os_sdio.h"
#include "atbm_hal.h"
#include <drivers/sdio.h>
#include <drivers/mmcsd_core.h>

#define __ATBM_SDIO_ALIGNSIZE(___blks,___size)  ((___size)+(___blks)-((___size)%(___blks)))
#define ATBM_SDIO_ALIGNSIZE(_blks,_size)        (((_size)%(_blks)==0)?(_size):__ATBM_SDIO_ALIGNSIZE(_blks,_size))

static struct rt_sdio_driver  atmbwifi_driver;

extern int atbm_sdio_probe(struct atbm_sdio_func *func,const struct atbm_sdio_device_id *id);
extern int atbm_sdio_disconnect(struct atbm_sdio_func *func);

static struct rt_sdio_device_id atbm_sdio_ids[] = {
        {1,0x007a,0x6011},
};
void atbm_sdio_claim_host(struct atbm_sdio_func  *func)
{
    mmcsd_host_lock(func->card->host);
}
void atbm_sdio_release_host(struct atbm_sdio_func *func)
{
    mmcsd_host_unlock(func->card->host);
}

void atbm_sdio_host_enable_irq(int enable)
{
}
int atbm_sdio_claim_irq(struct atbm_sdio_func *func,rt_sdio_irq_handler_t *handle)
{
    return sdio_attach_irq(func,handle);
}
int atbm_sdio_release_irq(struct atbm_sdio_func *func)
{
    return sdio_detach_irq(func);
}
int __atbm_sdio_memcpy_fromio(struct atbm_sdio_func *func,void *dst,unsigned int addr,int count)
{
    int ret;
    if((((unsigned int)dst) &3)||(count &3)){
        wifi_printk(WIFI_ALWAYS,"Read Data & Len must be 4 align,dst=0x%08x,count=%d",(unsigned int )dst,count);
    }
    switch(count){
    case    sizeof(atbm_uint16):
            *(atbm_uint16 *)dst=sdio_io_readw(func,addr,&ret);
            return ret;
    case    sizeof(atbm_uint32):
            *(atbm_uint32 *)dst=sdio_io_readl(func,addr,&ret);
            return ret;
    default:
        break;
    }
    ret=sdio_io_read_multi_incr_b(func,addr,dst,count);
  if(ret!=0){
       wifi_printk(WIFI_ALWAYS," __atbm_sdio_memcpy_fromio ret =%d\n",ret);
  }

    return ret;
}

int __atbm_sdio_memcpy_toio(struct atbm_sdio_func *func,unsigned int addr,void *dst,int count)
{
    int ret;
    if((((unsigned int)dst) &3)||(count &3)){
        wifi_printk(WIFI_ALWAYS,"Write Data & Len must be 4 align,dst=0x%08x,count=%d",(unsigned int )dst,count);
    }
    switch(count){
    case    sizeof(atbm_uint16):
            return sdio_io_writew(func,*(atbm_uint16 *)dst,addr);
    case    sizeof(atbm_uint32):
            return sdio_io_writel(func,*(atbm_uint32 *)dst,addr);
    default:
        break;
    }
    ret=sdio_io_write_multi_incr_b(func,addr,dst,count);
  if(ret!=0){
        wifi_printk(WIFI_ALWAYS," __atbm_sdio_memcpy_toio ret =%d\n",ret);
  }

    return ret;
}

unsigned char atbm_sdio_f0_readb(struct atbm_sdio_func *func,unsigned int addr,int *retval)
{
    struct atbm_sdio_func *func0;
    func0 = func->card->sdio_function[0];
    return sdio_io_readb(func0,addr,retval);

}

void atbm_sdio_f0_writeb(struct atbm_sdio_func *func,unsigned char regdata,unsigned int addr,int *retval)
{
    struct atbm_sdio_func *func0;

    func0 = func->card->sdio_function[0];
    *retval=sdio_io_writeb(func0,addr,regdata);
}

atbm_uint32 atbm_sdio_alignsize(struct atbm_sdio_func *func,atbm_uint32 size)
{
    atbm_uint32 block_size = func->cur_blk_size;
    atbm_uint32 alignsize = size;
    if((block_size == 0)||(size == 0)){
        rt_kprintf("atbm_sdio_alignsize err\n");
        return alignsize;
    }

    alignsize = ATBM_SDIO_ALIGNSIZE(block_size,size);
    return alignsize;
}

int atbm_sdio_register(struct rt_sdio_driver *sdio_driver)
{
    return sdio_register_driver(sdio_driver);
}

void atbm_sdio_deregister(struct rt_sdio_driver *sdio_driver)
{
    sdio_unregister_driver(sdio_driver);
}
void atbm_sdio_set_drvdata(struct atbm_sdio_func *func,void *priv)
{
    sdio_set_drvdata(func,priv);
}

void *atbm_sdio_get_drvdata(struct atbm_sdio_func *func)
{
    return sdio_get_drvdata(func);
}

atbm_int32 atbm_sdio_enable_func(struct atbm_sdio_func *func)
{
    return sdio_enable_func(func);
}

void atbm_sdio_disable_func(struct atbm_sdio_func *func)
{
    sdio_disable_func(func);
}

int  atbm_sdio_set_blocksize(struct atbm_sdio_func *func,int blocksize)
{
    func->card->host->max_blk_size=blocksize;
    return sdio_set_block_size(func,blocksize);
}
extern void atbm_wifi_set_init_done();
int _atbm_sdio_probe(struct rt_mmcsd_card *card)
{
    if(atbm_sdio_probe(card->sdio_function[1],NULL)){
        return -1;
    }
    return 0;
}
int _atbm_sdio_disconnect(struct rt_mmcsd_card *card)
{
    return atbm_sdio_disconnect(card->sdio_function[1]);
}
int atbm_sdio_register_init()
{
    int ret =0;
    atmbwifi_driver.name="atbm6031";
    atmbwifi_driver.id  = atbm_sdio_ids;
    atmbwifi_driver.probe      = _atbm_sdio_probe;
    atmbwifi_driver.remove     = _atbm_sdio_disconnect;
    wifi_printk(WIFI_ALWAYS, "atbm_sdio_register_init\r\n");
    ret = atbm_sdio_register(&atmbwifi_driver);
    if (ret){
        wifi_printk(WIFI_DBG_ERROR,"atbmwifi sdio driver register error\n");
    }
    return ret;
}

int atbm_sdio_register_deinit()
{
    atbm_sdio_deregister(&atmbwifi_driver);
    return 0;
}
