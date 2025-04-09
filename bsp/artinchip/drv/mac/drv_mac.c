
/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/err.h"
#include "lwip/ethip6.h"
#include <string.h>
#include <aic_core.h>
#include <netif/ethernetif.h>
#include "aic_mac.h"
#include "aic_phy.h"

#include "aicmac_macaddr.h"

extern unsigned long mac_base[MAX_ETH_MAC_PORT];
extern aicmac_dma_desc_ctl_t dctl[MAX_ETH_MAC_PORT];
extern unsigned long mac_irq[MAX_ETH_MAC_PORT];
extern aicmac_config_t mac_config[MAX_ETH_MAC_PORT];
extern aic_phy_device_t phy_device[MAX_ETH_MAC_PORT];

typedef struct _aicmac_rt
{
    /* inherit from ethernet device */
    struct eth_device parent;

    /* Speed */
    uint32_t    speed;
    /* Duplex Mode */
    uint32_t    mode;
    uint32_t    port;
    /* interface address info, hw address */
    rt_uint8_t  dev_addr[6];
}aicmac_rt_t;

#if defined(AIC_DEV_GMAC0_MACADDR) | defined(AIC_DEV_GMAC1_MACADDR)
static int hexchar2int(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    return 0;
}

static void hex2bin(unsigned char *bin, char *hex, int binlength)
{
    int i = 0;

    if ((strlen(hex) > binlength*2) || (strlen(hex)%2))
        return;

    for (i = 0; i < strlen(hex); i += 2) {
        bin[i / 2] = (char)((hexchar2int(hex[i]) << 4)
            | hexchar2int(hex[i + 1]));
    }
}
#endif

static void aicmac_rt_phy_poll_thread(void *para)
{
    aicmac_rt_t *mac = (aicmac_rt_t *)para;
    int port = mac->port;
    aic_phy_device_t *phydev = &phy_device[port];
    int old_link = 0;
    int err;

    while(1) {
        /* Sleep */
        aicos_msleep(2000);

        /* Pool phy satus */
        err = aicphy_read_status(port);
        if (err) {
            pr_err("%s fail.\n", __func__);
            continue;
        }

        /* Phy link status not change */
        if (old_link == phydev->link)
            continue;
        old_link = phydev->link;

        /* Phy link status change: DOWN -> UP */
        if (phydev->link) {
            pr_info(" Port %d link UP! %s mode: speed %dM, %s duplex, flow control %s.\n",
                    (int)port,
                    (phydev->autoneg ? "autoneg" : "fixed"),
                    phydev->speed,
                    (phydev->duplex ? "full" : "half"),
                    (phydev->pause ? "on" : "off"));

            /* Config mac base on autoneg result */
            if (phydev->autoneg) {
                aicmac_set_mac_speed(port, phydev->speed);
                aicmac_set_mac_duplex(port, phydev->duplex);
                aicmac_set_mac_pause(port, phydev->pause);
            }

            /* Enable MAC and DMA transmission and reception */
            aicmac_start(port);

            eth_device_linkchange(&mac->parent, RT_TRUE);
        /* Phy link status change: UP -> DOWN */
        } else {
            pr_info(" Port %d link DOWN!\n", (int)port);

            /* Disable MAC and DMA transmission and reception */
            aicmac_stop(port);

            eth_device_linkchange(&mac->parent, RT_FALSE);
        }
    }
}

static int aicmac_rt_phy_init(aicmac_rt_t *mac)
{
    int port = mac->port;
    aicmac_config_t *config = &mac_config[port];
    aic_phy_device_t *phydev = &phy_device[port];
    phydev->autoneg = config->autonegotiation;

    //aicphy_sw_reset(port);

    aicphy_read_abilities(port);

    aicphy_config_aneg(port);

    /* create the task that handles the ETH_MAC */
    aicos_thread_create("eth_phy_poll", 4096,
                        RT_LWIP_TCPTHREAD_PRIORITY-1, aicmac_rt_phy_poll_thread, mac);

    return ETH_SUCCESS;
}

static irqreturn_t aicmac_rt_interrupt(int irq_num, void *data)
{
    aicmac_rt_t *mac = (aicmac_rt_t *)data;

    pr_debug("%s\n", __func__);

    /* Frame received */
    if (aicmac_get_dma_int_status(mac->port, ETH_DMAINTSTS_RI) == SET) {
        /* Clear pending bit */
        aicmac_clear_dma_int_pending(mac->port, ETH_DMAINTSTS_RI);

        eth_device_ready(&(mac->parent));
    }

    /* Clear the interrupt flags. */
    /* Clear the Eth DMA Rx IT pending bits */
    aicmac_clear_dma_int_pending(mac->port, ETH_DMAINTSTS_NIS);

    return IRQ_HANDLED;
}

static rt_err_t aicmac_rt_init(rt_device_t dev)
{
    aicmac_rt_t *mac = dev->user_data;
    int port = mac->port;

    /* Mac init */
    aicmac_init(port);

    /* initialize MAC address 0 in ethernet MAC */
    aicmac_set_mac_addr(port, 0, mac->dev_addr);

    /* Initialize Tx Descriptors list: Chain Mode */
    aicmac_dma_tx_desc_init(port);
    /* Initialize Rx Descriptors list: Chain Mode  */
    aicmac_dma_rx_desc_init(port);

    /* Enable Ethernet Rx interrrupt */
    aicmac_set_dma_rx_desc_int(port, ENABLE);

    aicos_request_irq(mac_irq[port],
                      (irq_handler_t)aicmac_rt_interrupt, 0, NULL, mac);

    /* Phy init */
    aicmac_rt_phy_init(mac);

    return 0;
}

static rt_err_t aicmac_rt_tx(rt_device_t dev, struct pbuf *p)
{
    aicmac_rt_t *mac = (aicmac_rt_t *)dev->user_data;
    struct pbuf *q;
    aicmac_dma_desc_t *pdesc;
    int ret = ERR_OK;
    u8 *buffer;
    uint16_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;


    pr_debug("%s\n", __func__);

    if (p == NULL) {
        pr_err("%s invalid parameter.\n", __func__);
        return ERR_MEM;
    }

    // aicos_mutex_take(mac->eth_tx_mutex, AICOS_WAIT_FOREVER);

    pdesc = dctl[mac->port].tx_desc_p;
    /* before read: invalid cache */
    aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

    buffer = (u8 *)(unsigned long)(pdesc->buff1_addr);
    bufferoffset = 0;

    for (q = p; q != NULL; q = q->next) {
        if ((pdesc->control & ETH_DMATxDesc_OWN) != (u32)RESET) {
            pr_err("%s no enough desc for transmit.(len = %u)\n", __func__, q->len);
            ret = ERR_MEM;
            goto error;
        }

        /* Get bytes in current lwIP buffer  */
        byteslefttocopy = q->len;
        payloadoffset = 0;

        /* Check if the length of data to copy is bigger than Tx buffer size*/
        while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE) {
            /* Copy data to Tx buffer*/
            memcpy((u8_t *)((u8_t *)buffer + bufferoffset),
                   (u8_t *)((u8_t *)q->payload + payloadoffset),
                   (ETH_TX_BUF_SIZE - bufferoffset));
            /* after write: flush cache */
            aicmac_dcache_clean((uintptr_t)((u8_t *)buffer + bufferoffset),
                                (ETH_TX_BUF_SIZE - bufferoffset));

            /* Point to next descriptor */
            pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
            /* before read: invalid cache */
            aicmac_dcache_invalid((uintptr_t)pdesc, sizeof(aicmac_dma_desc_t));

            /* Check if the buffer is available */
            if ((pdesc->control & ETH_DMATxDesc_OWN) != (u32)RESET) {
                pr_err("%s no enough desc for transmit.(len = %u)\n", __func__, q->len);
                ret = ERR_MEM;
                goto error;
            }

            buffer = (u8 *)(unsigned long)(pdesc->buff1_addr);

            byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
            payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
            framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
            bufferoffset = 0;
        }

        /* Copy the remaining bytes */
        memcpy((u8_t *)((u8_t *)buffer + bufferoffset),
               (u8_t *)((u8_t *)q->payload + payloadoffset), byteslefttocopy);
        /* after write: flush cache */
        aicmac_dcache_clean((uintptr_t)((u8_t *)buffer + bufferoffset),
                            byteslefttocopy);
        bufferoffset = bufferoffset + byteslefttocopy;
        framelength = framelength + byteslefttocopy;
    }

    /* Prepare transmit descriptors to give to DMA*/
    aicmac_submit_tx_frame(mac->port, framelength);

error:
    /* Give semaphore and exit */
    // aicos_mutex_give(mac->eth_tx_mutex);

    return ret;
}

static struct pbuf * aicmac_rt_rx(rt_device_t dev)
{
    aicmac_rt_t *mac = dev->user_data;
    struct pbuf *p = NULL;
    struct pbuf *q = NULL;
    u32_t len;
    aicmac_frame_t frame = { 0, 0, 0, 0 };
    aicmac_dma_desc_t *pdesc = NULL;
    u8 *buffer;
    uint32_t bufferoffset = 0;
    uint32_t payloadoffset = 0;
    uint32_t byteslefttocopy = 0;

    /* get received frame */
    frame = aicmac_get_rx_frame_interrupt(mac->port);

    if ((NULL == frame.descriptor) || (NULL == frame.last_desc))
        return NULL;

    /* Obtain the size of the packet and put it into the "len" variable. */
    len = frame.length;
    buffer = (u8 *)(unsigned long)frame.buffer;

    if (len > 0) {
        /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }

    if (p != NULL) {
        pdesc = frame.descriptor;
        bufferoffset = 0;
        for (q = p; q != NULL; q = q->next) {
            byteslefttocopy = q->len;
            payloadoffset = 0;

            /* Check if the length of bytes to copy in current pbuf is bigger than Rx buffer size*/
            while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE) {
                aicmac_gdma_sync();
                /* before read: invalid cache */
                aicmac_dcache_invalid((uintptr_t)((u8_t *)buffer + bufferoffset),
                                      (ETH_RX_BUF_SIZE - bufferoffset));
                /* Copy data to pbuf*/
                memcpy((u8_t *)((u8_t *)q->payload + payloadoffset),
                       (u8_t *)((u8_t *)buffer + bufferoffset),
                       (ETH_RX_BUF_SIZE - bufferoffset));

                /* Point to next descriptor */
                pdesc = (aicmac_dma_desc_t *)(unsigned long)(pdesc->buff2_addr);
                buffer = (unsigned char *)(unsigned long)(pdesc->buff1_addr);

                byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
                payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
                bufferoffset = 0;
            }

            aicmac_gdma_sync();
            /* before read: invalid cache */
            aicmac_dcache_invalid((uintptr_t)((u8_t *)buffer + bufferoffset),
                                  byteslefttocopy);
            /* Copy remaining data in pbuf */
            memcpy((u8_t *)((u8_t *)q->payload + payloadoffset),
                   (u8_t *)((u8_t *)buffer + bufferoffset), byteslefttocopy);
            bufferoffset += byteslefttocopy;
        }
    }

    /* Point to next descriptor */
    dctl[mac->port].rx_desc_received_p = (aicmac_dma_desc_t *)(unsigned long)(frame.last_desc->buff2_addr);
    dctl[mac->port].rx_frame_info.seg_cnt = 0;

    aicmac_release_rx_frame(mac->port);
    return p;
}

static rt_err_t aicmac_rt_open(rt_device_t dev, rt_uint16_t oflag)
{
    LOG_D("emac open");
    return RT_EOK;
}

static rt_err_t aicmac_rt_close(rt_device_t dev)
{
    LOG_D("emac close");
    return RT_EOK;
}

static rt_size_t aicmac_rt_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    LOG_D("emac read");
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static rt_size_t aicmac_rt_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    LOG_D("emac write");
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static rt_err_t aicmac_rt_control(rt_device_t dev, int cmd, void *args)
{
    aicmac_rt_t *mac = dev->user_data;
    switch (cmd)
    {
        case NIOCTL_GADDR:

            /* get mac address */
            if (args) rt_memcpy(args, mac->dev_addr, 6);
            else return -RT_ERROR;

            break;

        default :
            break;
    }

    return RT_EOK;
}

static aicmac_rt_t aicmac_device[MAX_ETH_MAC_PORT];
static const struct rt_device_ops aicmac_rt_ops = {
    aicmac_rt_init,
    aicmac_rt_open,
    aicmac_rt_close,
    aicmac_rt_read,
    aicmac_rt_write,
    aicmac_rt_control
};
/* Register the EMAC device */
static int aicmac_rt_register(int port, char *name)
{
    int state;

    aicmac_device[port].port = port;

    aicmac_get_macaddr_from_chipid(port, aicmac_device[port].dev_addr);

#ifdef AIC_DEV_GMAC0_MACADDR
    if (port == 0) {
        hex2bin(aicmac_device[port].dev_addr, AIC_DEV_GMAC0_MACADDR, 6);
    }
#endif

#ifdef AIC_DEV_GMAC1_MACADDR
    if (port == 1) {
        hex2bin(aicmac_device[port].dev_addr, AIC_DEV_GMAC1_MACADDR, 6);
    }
#endif

#ifdef RT_USING_DEVICE_OPS
    aicmac_device[port].parent.parent.ops = &aicmac_rt_ops;
#else
    aicmac_device[port].parent.parent.init      = aicmac_rt_init;
    aicmac_device[port].parent.parent.open      = aicmac_rt_open;
    aicmac_device[port].parent.parent.close     = aicmac_rt_close;
    aicmac_device[port].parent.parent.read      = aicmac_rt_read;
    aicmac_device[port].parent.parent.write     = aicmac_rt_write;
    aicmac_device[port].parent.parent.control   = aicmac_rt_control;
#endif
    aicmac_device[port].parent.parent.user_data = &aicmac_device[port];

    aicmac_device[port].parent.eth_rx = aicmac_rt_rx;
    aicmac_device[port].parent.eth_tx = aicmac_rt_tx;

    /* register eth device */
    state = eth_device_init(&(aicmac_device[port].parent), name);

    if (RT_EOK == state)
    {
        LOG_D("aicmac device[%d] init success", port);
    }
    else
    {
        LOG_E("aicmac device[%d] init faild: %d", port, state);
    }

    return state;
}

static int aicmac_rt_probe()
{
    int state = 0;
#ifdef AIC_USING_GMAC0
    state = aicmac_rt_register(0, "e0");
    if (RT_EOK == state)
        LOG_D("aicmac device[%d] init success", 0);
    else
        LOG_E("aicmac device[%d] init faild: %d", 0, state);
#endif

#ifdef AIC_USING_GMAC1
    state = aicmac_rt_register(1, "e1");
    if (RT_EOK == state)
        LOG_D("aicmac device[%d] init success", 1);
    else
        LOG_E("aicmac device[%d] init faild: %d", 1, state);
#endif
    return state;
}

INIT_DEVICE_EXPORT(aicmac_rt_probe);
