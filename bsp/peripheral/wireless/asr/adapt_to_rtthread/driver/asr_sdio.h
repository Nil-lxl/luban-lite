#ifndef __ASR_SDIO_PORT_H__
#define __ASR_SDIO_PORT_H__

#include "asr_rtos_api.h"

#include <drivers/mmcsd_card.h>
typedef struct rt_sdio_function  asr_drv_sdio_t;

#define FN(x)                           (x)
#define FN0                             FN(0)
#define FN1                             FN(1)
#define FN2                             FN(2)
#define FN3                             FN(3)
#define FN4                             FN(4)
#define FN5                             FN(5)
#define FN6                             FN(6)
#define FN7                             FN(7)
#define SDIO_CCCR_IENx                  0x04    /* Function/Master Interrupt Enable */

#define CONFIG_SDC_ID 1



typedef struct {
    asr_drv_sdio_t  *func;
    int             gpio_irq;
    int             gpio_power;
    int             gpio_reset;
    unsigned int    active_power;
    unsigned int    active_reset;
}asr_sbus;


/**
 *  claim_host - exclusively claim a bus for a certain SDIO function
 *  @func: SDIO function that will be accessed
 *
 *  Claim a bus for a set of operations. The SDIO function given
 *  is used to figure out which bus is relevant.
 */
void
sdio_claim_host(asr_drv_sdio_t  *func);

/**
 *  release_host - release a bus for a certain SDIO function
 *  @func: SDIO function that was accessed
 *
 *  Release a bus, allowing others to claim the bus for their
 *  operations.
 */
void
sdio_release_host(asr_drv_sdio_t *func);

#define sdio_lock(l) sdio_claim_host(l)
#define sdio_unlock(l) sdio_release_host(l)

void asr_sdio_claim_host(void);
void asr_sdio_release_host(void);

/**
 *  sdio_align_size - pads a transfer size to a more optimal value
 *  @func: SDIO function
 *  @sz: original transfer size
 *
 *  Pads the original data size with a number of extra bytes in
 *  order to avoid controller bugs and/or performance hits
 *  (e.g. some controllers revert to PIO for certain sizes).
 *
 *  If possible, it will also adjust the size so that it can be
 *  handled in just a single request.
 *
 *  Returns the improved size, which might be unmodified.
 */
inline static unsigned int
nx_sdio_align_size(asr_drv_sdio_t *func, unsigned int sz)
{
    /* Do a first check with the controller, in case it
     * wants to increase the size up to a point where it
     * might need more than one block.
     */
    if (sz && sz & 3) {
        sz &= ~0x03;
        sz += 0x04;
    }

    /* The controller is simply incapable of transferring the size
     * we want in decent manner, so just return the original size.
     */
    return sz;
}

/*
 * asr_sdio_set_block_size - asr set the block size of an SDIO function
 * @c: asr_drv_sdio_t
 * @fn:
 * @sz: new block size
 */
int asr_sdio_set_block_size(unsigned int blksz);
int sdio_enable_func__(unsigned int func_num);
int sdio_disable_func__(unsigned int func_num);

#define asr_sdio_enable_func(fn)    sdio_enable_func__(fn)
#define asr_sdio_disable_func(fn)    sdio_disable_func__(fn)
#define asr_sdio_align_size nx_sdio_align_size

asr_drv_sdio_t *asr_sdio_detect(unsigned int id, int enable);
void asr_sdio_deinit(asr_drv_sdio_t *card);

extern void sdio_card_gpio_hisr_func(void);
int asr_sdio_claim_irq(void);
int asr_sdio_release_irq(void);

unsigned char sdio_host_enable_isr(unsigned char enable);

/**
 *  sdio_readb - read a single byte from a SDIO function
 *  @card: SDIO to access
 *  @addr: address to read
 *  @err_ret: optional status value from transfer
 *
 *  Reads a single byte from the address space of a given SDIO
 *  function. If there is a problem reading the address, 0xff
 *  is returned and @err_ret will contain the error code.
 */
unsigned char sdio_readb(asr_drv_sdio_t *card, unsigned int func_num, unsigned int addr, int *err_ret);
/**
 *  sdio_writeb - write a single byte to a SDIO function
 *  @card: SDIO to access
 *  @b: byte to write
 *  @addr: address to write to
 *  @err_ret: optional status value from transfer
 *
 *  Writes a single byte to the address space of a given SDIO
 *  function. @err_ret will contain the status of the actual
 *  transfer.
 */
void sdio_writeb(asr_drv_sdio_t *card, unsigned int func_num, unsigned char b, unsigned int addr, int  *err_ret);
/**
 *  memcpy_fromio - read a chunk of memory from a SDIO function
 *  @dst: buffer to store the data
 *  @addr: address to begin reading from
 *  @count: number of bytes to read
 *
 *  Reads from the address space of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
int asr_sdio_memcpy_fromio(asr_drv_sdio_t *card, unsigned int func_num, void *dst, unsigned int addr, int count);
/**
 *  memcpy_toio - write a chunk of memory to a SDIO function
 *  @addr: address to start writing to
 *  @src: buffer that contains the data to write
 *  @count: number of bytes to write
 *
 *  Writes to the address space of a given SDIO function. Return
 *  value indicates if the transfer succeeded or not.
 */
int asr_sdio_memcpy_toio(asr_drv_sdio_t *card, unsigned int func_num, unsigned int addr, void *src, int count);

int SDIO_CMD52(int write, unsigned int fn, unsigned int addr, unsigned char in, unsigned char*out);

int asr_sdio_dev_init(void);

unsigned char sdio_writeb_cmd52(unsigned int addr, unsigned char data);
unsigned char sdio_readb_cmd52(unsigned int addr, unsigned char *data);
unsigned char sdio_read_cmd53(unsigned int dataport, unsigned char *data, int size);
unsigned char sdio_write_cmd53(unsigned int dataport, unsigned char *data, int size);

#define sdio_bus_probe  asr_sdio_bus_probe
#define sdio_get_block_size  asr_sdio_get_block_size

int asr_sdio_bus_probe(void);

OSStatus asr_sdio_register_driver(void);

OSStatus asr_sdio_unregister_driver();

unsigned int asr_sdio_get_block_size(void);

unsigned char sdio_read_cmd53_reg(unsigned int dataport, unsigned char *data, int size);

#endif /* __ASR_SDIO_H__ */
