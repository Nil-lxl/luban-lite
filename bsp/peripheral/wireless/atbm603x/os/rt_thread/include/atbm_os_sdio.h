#ifndef ATBM_OS_SDIO_H
#define ATBM_OS_SDIO_H
#include "atbm_hal.h"
#include "drivers/mmcsd_card.h"

#define atbm_sdio_func  rt_sdio_function
struct atbm_sdio_device_id{
    uint8_t id;
};
int atbm_sdio_register_init();
int atbm_sdio_register_deinit();
void atbm_sdio_claim_host(struct atbm_sdio_func *func);
void atbm_sdio_release_host(struct atbm_sdio_func *func);
atbm_int32 atbm_sdio_enable_func(struct atbm_sdio_func *func);
void atbm_sdio_disable_func(struct atbm_sdio_func *func);
void atbm_sdio_set_drvdata(struct atbm_sdio_func *func,void *priv);
void *atbm_sdio_get_drvdata(struct atbm_sdio_func *func);
int atbm_sdio_claim_irq(struct atbm_sdio_func *func,void (*irq_handle)(struct atbm_sdio_func *func));
int atbm_sdio_release_irq(struct atbm_sdio_func *func);
int __atbm_sdio_memcpy_fromio(struct atbm_sdio_func *func,void *dst,unsigned int addr,int count);
int __atbm_sdio_memcpy_toio(struct atbm_sdio_func *func,unsigned int addr,void *dst,int count);
unsigned char atbm_sdio_f0_readb(struct atbm_sdio_func *func,unsigned int addr,int *retval);
void atbm_sdio_f0_writeb(struct atbm_sdio_func *func,unsigned char regdata,unsigned int addr,int *retval);
int atbm_sdio_set_blocksize(struct atbm_sdio_func *func,int blocksize);
atbm_uint32 atbm_sdio_alignsize(struct atbm_sdio_func *func,atbm_uint32 size);
void atbm_sdio_gpioirq_en(struct atbm_sdio_func *func,atbm_uint8 en);

#endif/* ATBM_OS_SDIO_H */
