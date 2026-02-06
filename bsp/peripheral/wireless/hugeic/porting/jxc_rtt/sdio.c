#include "os_porting.h"
#include "drivers/mmcsd_card.h"
#include "drivers/mmcsd_core.h"
#include "drivers/sdio.h"
#include "os_sdio.h"
#include "plat_gpio.h"

#define HGIC_VENDOR_ID  (0xA012)
#define HGIC_WLAN_4001  (0x4001)
#define HGIC_WLAN_4002  (0x4002)
#define HGIC_WLAN_4102  (0x4102)
#define HGIC_WLAN_4103  (0x4103)
#define HGIC_WLAN_4104  (0x4104)
#define HGIC_WLAN_8400  (0x8400)
#define HGIC_WLAN_8410  (0x8410)
#define HGIC_WLAN_8000  (0x8000)


#define SDIO_DEVICE(vend,dev) \
    .manufacturer = (vend), .product = (dev)

//typedef rt_sdio_irq_handler_t sdio_irq_handler_t;

const struct rt_sdio_device_id hgic_sdio_wdev_ids[] = {
    {SDIO_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_4002)},
    {SDIO_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_4104)},
    {SDIO_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8400)},
    {SDIO_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8410)},
    {SDIO_DEVICE(HGIC_VENDOR_ID, HGIC_WLAN_8000)},
    { /* end: all zeroes */             },
};

unsigned char *malloc_sdio_rxbuf(unsigned int buff_len,unsigned int flags)
{
    return rt_malloc(buff_len);//maybe align
}

void free_sdio_rxbuf(unsigned char *rxbuf)
{
    if(rxbuf) {
        rt_free(rxbuf);
    }
}

unsigned char sys_sdio_readb(sdio_func_t *func, unsigned long addr, int *err)
{
    return sdio_io_readb(func,  (rt_uint32_t)addr, (rt_int32_t *)err);
}

unsigned char sys_sdio_writeb(sdio_func_t *func, unsigned char b, unsigned long addr, int *err)
{
    int ret = 0;
    ret = sdio_io_writeb(func, (rt_uint32_t)addr, b);
    if (err) {
        *err = ret;
    }
    return 0;
}

static int hgic_sdio_readb(sdio_func_t *func, unsigned int addr, int *err)
{
    unsigned char  val   = 0;
    int retry = 2;

    val = sys_sdio_readb(func, addr, err);

    while (*err && retry-- > 0) {
        val = sys_sdio_readb(func, addr, err);
    }
    return val;
}

static void hgic_sdio_writeb(sdio_func_t *func, unsigned char b, unsigned int addr, int *err)
{
    int retry = 4;
    do {
        sys_sdio_writeb(func, b, addr, err);
    } while (*err && retry-- > 0);
}

int hgic_sdio_abort(sdio_func_t *func)
{
    int err_ret = 0;
    struct rt_sdio_function func0;
    rt_memcpy(&func0, func, sizeof(func0));
    func0.num = 0;
    hgic_sdio_writeb(&func0, 1, 6, &err_ret);
    return err_ret;
}

int hgic_sdio_read_cccr(sdio_func_t *func, unsigned char *pending)
{
    int ret = 0;
    struct rt_sdio_function func0;
    rt_memcpy(&func0, func, sizeof(func0));
    func0.num = 0;
    *pending = hgic_sdio_readb(&func0, 0x5, &ret);
    return ret;
}

long sys_sdio_memcpy_fromio(sdio_func_t *func, unsigned char *dest, unsigned long addr, int count)
{
    return sdio_io_read_multi_incr_b(func, addr, dest, count);
}

long sys_sdio_memcpy_toio(sdio_func_t *func, unsigned long addr, unsigned char *src, int count)
{
    return sdio_io_write_multi_incr_b(func, addr, src, count);
}

void sys_sdio_claim_host(sdio_func_t *func)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    mmcsd_host_lock(sdio_func->card->host);
}

void sys_sdio_release_host(sdio_func_t *func)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    mmcsd_host_unlock(sdio_func->card->host);
}

long sys_sdio_set_block_size(sdio_func_t *func, int bsize)
{
    long ret  = 0;
    ret = sdio_set_block_size(func,bsize);
    if (ret) {
        hgic_err("Set sdio blocksize failed:%d,blocksize:%d\n", ret, bsize);
    } else {
        hgic_dbg("Set sdio blocksize:%d\n", bsize);
    }
    return ret;
}

long sys_sdio_enable_func(sdio_func_t *func)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    long ret = 0;
    ret = sdio_enable_func(sdio_func);
    if (ret) {
        hgic_dbg("enable: function%d failed!\n", sdio_func->num);
    } else {
        hgic_dbg("sys_sdio_enable_func: function%d enabled.\n", sdio_func->num);
    }
    return ret;
}

long sys_sdio_disable_func(sdio_func_t *func)
{
    return sdio_disable_func(func);
}

long sys_sdio_claim_irq(sdio_func_t *func, sdio_irq_handler_t irqhdl)
{
    return sdio_attach_irq(func, irqhdl);
}

long sys_sdio_release_irq(sdio_func_t *func)
{
    return sdio_detach_irq(func);
}

void *sys_sdio_get_drvdata(sdio_func_t *func)
{
    return (void *)sdio_get_drvdata(func);
}

long sys_sdio_set_drvdata(sdio_func_t *func, void *data)
{
    sdio_set_drvdata(func, data);
    return 0;
}

/*
long sys_sdio_register_driver(sdio_driver_t *driver)
{
    return sdio_register_driver((struct rt_sdio_driver *)driver)
}


void sys_sdio_unregister_driver(sdio_driver_t *driver)
{
    ;
}

unsigned short hgic_get_func_rca(sdio_func_t *func)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    ASSERT(sdio_func);
    ASSERT(sdio_func->card);
    return sdio_func->card->rca;
}
*/

unsigned long sys_sdio_get_func_num(sdio_func_t *func)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    return sdio_func->num;
}

void *sys_sdio_get_func_dev(sdio_func_t *func)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    return (void *)sdio_func->card;
}

unsigned short sys_sdio_get_device_id(sdio_device_id_t *id)
{
    struct rt_sdio_device_id *devid = (struct rt_sdio_device_id *)id;
    return devid->product;
}

unsigned short sys_sdio_get_vendor_id(sdio_device_id_t *id)
{
    struct rt_sdio_device_id *devid = (struct rt_sdio_device_id *)id;
    return devid->manufacturer;
}

void hgic_mmc_set_blk_size(sdio_func_t *func, unsigned int blk_size)
{
    sdio_set_block_size(func, blk_size);
}


void hgic_mmc_set_clock(sdio_func_t *func, unsigned int hz)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    ASSERT(sdio_func);
    ASSERT(sdio_func->card);
    ASSERT(sdio_func->card->host);

    mmcsd_set_clock(sdio_func->card->host, hz);
}

void hgic_mmc_set_timing(sdio_func_t *func, unsigned int timing)
{
    // do nothing
}

void hgic_mmc_set_bus_width(sdio_func_t *func, unsigned int width)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    ASSERT(sdio_func);
    ASSERT(sdio_func->card);
    ASSERT(sdio_func->card->host);

    mmcsd_set_bus_width(sdio_func->card->host, width);
}

#if 0
long hgic_mmc_io_rw_direct(sdio_func_t *func, int write, unsigned fn, unsigned addr, unsigned char in, unsigned char *out)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    ASSERT(out)
    ASSERT(sdio_func);
    ASSERT(sdio_func->card);


    //return sdio_io_rw_direct(sdio_func->card, write, fn, addr, write?&in:out, 0);

    return sdio_io_rw_direct(sdio_func->card, write, fn, addr, out, out);
}
#endif

long hgic_mmc_send_cmd(sdio_func_t *func, void *cmd, int retries)
{
    struct rt_sdio_function *sdio_func = (struct rt_sdio_function *)func;
    struct rt_mmcsd_cmd *command = (struct rt_mmcsd_cmd *)cmd;
    int err = 0;

    ASSERT(sdio_func);
    ASSERT(sdio_func->card);
    ASSERT(sdio_func->card->host);
    ASSERT(command);

    err = mmcsd_send_cmd(sdio_func->card->host, command, retries);
    if (err) {
        hgic_err("cmd:%d Error\n",command->cmd_code);
        return err;
    }
    return 0;
}

unsigned long hgic_sdio_get_max_clock(sdio_func_t *func)
{
    return 50000000;
}

typedef int (*hgic_probe)(void *dev, void *bus);
extern int hgic_sdio_probe(sdio_func_t *func, const sdio_device_id_t *id);
extern void hgic_sdio_remove(sdio_func_t *func);
extern void hgic_sdio_set_probe_hdl(hgic_probe probe);
extern unsigned int hgic_sdio_set_max_pkt(unsigned int max_pkt);

static rt_int32_t hgic_sdio_probe_rtt(struct rt_mmcsd_card *card)
{
    rt_int32_t ret = 0;
    sdio_func_t *func = card->sdio_function[1];
    int i = 0;
    struct rt_sdio_device_id *id = NULL;

    for(i = 0; i < (sizeof(hgic_sdio_wdev_ids)/sizeof(hgic_sdio_wdev_ids[0]));i++) {
        if((card->sdio_function[1]->manufacturer == hgic_sdio_wdev_ids[i].manufacturer)
            && (card->sdio_function[1]->product == hgic_sdio_wdev_ids[i].product)) {
            id = &hgic_sdio_wdev_ids[i];
            break;
        }
    }
    if(!id) {
        hgic_err("Can not find vendor:0x%x,device 0x%x\n",card->sdio_function[1]->manufacturer,
            card->sdio_function[1]->product);
        return -1;
    }
    ret = hgic_sdio_probe(func, (void *)id);
    if (ret) {
        hgic_err("sdio probe faile!\n");                
    }
    return ret;
}

static rt_int32_t hgic_sdio_remove_rtt(struct rt_mmcsd_card *card)
{
    sdio_func_t *func = card->sdio_function[1];

    hgic_sdio_remove(func);
    return 0;
}


static struct rt_sdio_driver hgic_sdio_driver = {
    .name     = "hgic_sdio_wlan",
    //.id = hgic_sdio_wdev_ids,
    .probe    = hgic_sdio_probe_rtt,
    .remove   = hgic_sdio_remove_rtt,
};

int hgic_sdio_init(hgic_probe probe, unsigned int max_pkt)
{
    int ret;
    hgic_sdio_set_probe_hdl(probe);
    hgic_sdio_set_max_pkt(max_pkt);

    hgic_sdio_driver.id = &hgic_sdio_wdev_ids[4];

    ret = sdio_register_driver(&hgic_sdio_driver);
    if (ret) {
        hgic_err("hgicf sdio register driver failed! ret = %d\n", ret);
        return;
    }

    return ret;
}

void hgic_sdio_exit(void)
{
    sdio_unregister_driver(&hgic_sdio_driver);
}

