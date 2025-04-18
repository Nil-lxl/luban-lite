#include "usbh_core.h"
#include "usbh_cdc_acm.h"
#include "usbh_hid.h"
#include "usbh_msc.h"
#include "usbh_video.h"
#include "usbh_audio.h"

#define TEST_USBH_CDC_ACM   1
#define TEST_USBH_HID       1
#define TEST_USBH_MSC       1
#define TEST_USBH_MSC_FATFS 0
#define TEST_USBH_AUDIO     0
#define TEST_USBH_VIDEO     0

#if __has_include("lwip/pbuf.h")
#define TEST_USBH_CDC_ECM 1
#define TEST_USBH_RNDIS   1
#else
#define TEST_USBH_CDC_ECM 0
#define TEST_USBH_RNDIS   0
#endif

#if TEST_USBH_CDC_ACM
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_buffer[512];

void usbh_cdc_acm_callback(void *arg, int nbytes)
{
    //struct usbh_cdc_acm *cdc_acm_class = (struct usbh_cdc_acm *)arg;

    if (nbytes > 0) {
        for (size_t i = 0; i < nbytes; i++) {
            USB_LOG_RAW("0x%02x ", cdc_buffer[i]);
        }
        USB_LOG_RAW("nbytes:%d\r\n", nbytes);
    }
}

static void usbh_cdc_acm_thread(void *argument)
{
    int ret;
    struct usbh_cdc_acm *cdc_acm_class;

    // clang-format off
find_class:
    // clang-format on
    while ((cdc_acm_class = (struct usbh_cdc_acm *)usbh_find_class_instance("/dev/ttyACM0")) == NULL) {
        goto delete;
    }

    memset(cdc_buffer, 0, 512);

    const uint8_t data1[10] = { 0x02, 0x00, 0x00, 0x00, 0x02, 0x02, 0x08, 0x14 };

    memcpy(cdc_buffer, data1, 10);
    usbh_bulk_urb_fill(&cdc_acm_class->bulkout_urb, cdc_acm_class->hport, cdc_acm_class->bulkout, cdc_buffer, 10, 3000, NULL, NULL);
    ret = usbh_submit_urb(&cdc_acm_class->bulkout_urb);
    if (ret < 0) {
        USB_LOG_RAW("bulk out error,ret:%d\r\n", ret);
        goto find_class;
    } else {
        USB_LOG_RAW("send over:%ld\r\n", cdc_acm_class->bulkout_urb.actual_length);
    }

    usbh_bulk_urb_fill(&cdc_acm_class->bulkin_urb, cdc_acm_class->hport, cdc_acm_class->bulkin, cdc_buffer, cdc_acm_class->bulkin->wMaxPacketSize, 3000, usbh_cdc_acm_callback, cdc_acm_class);
    ret = usbh_submit_urb(&cdc_acm_class->bulkin_urb);
    if (ret < 0) {
        USB_LOG_RAW("bulk in error,ret:%d\r\n", ret);
    } else {
    }
    // clang-format off
delete:
    usb_osal_thread_delete(NULL);
    // clang-format on
}
#endif

#if TEST_USBH_HID
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buffer[128];

void usbh_hid_callback(void *arg, int nbytes)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)arg;

    if (nbytes > 0) {
        for (size_t i = 0; i < nbytes; i++) {
            USB_LOG_RAW("0x%02x ", hid_buffer[i]);
        }
        USB_LOG_RAW("nbytes:%d\r\n", nbytes);
        usbh_submit_urb(&hid_class->intin_urb);
    } else if (nbytes == -USB_ERR_NAK) { /* for dwc2 */
        usbh_submit_urb(&hid_class->intin_urb);
    } else {
    }
}

static void usbh_hid_thread(void *argument)
{
    int ret;
    struct usbh_hid *hid_class;

    // clang-format off
find_class:
    // clang-format on
    while ((hid_class = (struct usbh_hid *)usbh_find_class_instance("/dev/input0")) == NULL) {
        goto delete;
    }

    usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_buffer, hid_class->intin->wMaxPacketSize, 0, usbh_hid_callback, hid_class);
    ret = usbh_submit_urb(&hid_class->intin_urb);
    if (ret < 0) {
        goto find_class;
    }
    // clang-format off
delete:
    usb_osal_thread_delete(NULL);
    // clang-format on
}
#endif

#if TEST_USBH_MSC

#if TEST_USBH_MSC_FATFS
#include "ff.h"

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_write_buffer[25 * 100];

USB_NOCACHE_RAM_SECTION FATFS fs;
USB_NOCACHE_RAM_SECTION FIL fnew;
UINT fnum;
FRESULT res_sd = 0;

int usb_msc_fatfs_test()
{
    const char *tmp_data = "cherryusb fatfs demo...\r\n";

    USB_LOG_RAW("data len:%d\r\n", strlen(tmp_data));
    for (uint32_t i = 0; i < 100; i++) {
        memcpy(&read_write_buffer[i * 25], tmp_data, strlen(tmp_data));
    }

    res_sd = f_mount(&fs, "2:", 1);
    if (res_sd != FR_OK) {
        USB_LOG_RAW("mount fail,res:%d\r\n", res_sd);
        return -1;
    }

    USB_LOG_RAW("test fatfs write\r\n");
    res_sd = f_open(&fnew, "2:test.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (res_sd == FR_OK) {
        res_sd = f_write(&fnew, read_write_buffer, sizeof(read_write_buffer), &fnum);
        if (res_sd == FR_OK) {
            USB_LOG_RAW("write success, write len:%d\n", fnum);
        } else {
            USB_LOG_RAW("write fail\r\n");
            goto unmount;
        }
        f_close(&fnew);
    } else {
        USB_LOG_RAW("open fail\r\n");
        goto unmount;
    }
    USB_LOG_RAW("test fatfs read\r\n");

    res_sd = f_open(&fnew, "2:test.txt", FA_OPEN_EXISTING | FA_READ);
    if (res_sd == FR_OK) {
        res_sd = f_read(&fnew, read_write_buffer, sizeof(read_write_buffer), &fnum);
        if (res_sd == FR_OK) {
            USB_LOG_RAW("read success, read len:%d\n", fnum);
        } else {
            USB_LOG_RAW("read fail\r\n");
            goto unmount;
        }
        f_close(&fnew);
    } else {
        USB_LOG_RAW("open fail\r\n");
        goto unmount;
    }
    f_mount(NULL, "2:", 1);
    return 0;
unmount:
    f_mount(NULL, "2:", 1);
    return -1;
}
#endif

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t partition_table[512];

#ifndef __RTTHREAD__
#if TEST_USBH_MSC
static void usbh_msc_thread(void *argument)
{
    int ret;
    struct usbh_msc *msc_class;

    // clang-format off
find_class:
    // clang-format on
    while ((msc_class = (struct usbh_msc *)usbh_find_class_instance("/dev/sda")) == NULL) {
        goto delete;
    }

#if 1
    /* get the partition table */
    ret = usbh_msc_scsi_read10(msc_class, 0, partition_table, 1);
    if (ret < 0) {
        USB_LOG_RAW("scsi_read10 error,ret:%d\r\n", ret);
        goto find_class;
    }
    for (uint32_t i = 0; i < 512; i++) {
        if (i % 16 == 0) {
            USB_LOG_RAW("\r\n");
        }
        USB_LOG_RAW("%02x ", partition_table[i]);
    }
    USB_LOG_RAW("\r\n");
#endif

#if TEST_USBH_MSC_FATFS
    usb_msc_fatfs_test();
#endif
    // clang-format off
delete:
    usb_osal_thread_delete(NULL);
    // clang-format on
}
#endif
#endif // TEST_USBH_MSC
#endif // __RTTHREAD__

#if 0
void usbh_videostreaming_parse_mjpeg(struct usbh_urb *urb, struct usbh_videostreaming *stream)
{
    struct usbh_iso_frame_packet *iso_packet;
    uint32_t num_of_iso_packets;
    uint8_t data_offset;
    uint32_t data_len;
    uint8_t header_len = 0;

    num_of_iso_packets = urb->num_of_iso_packets;
    iso_packet = urb->iso_packet;

    for (uint32_t i = 0; i < num_of_iso_packets; i++) {
        /*
            uint8_t frameIdentifier : 1U;
            uint8_t endOfFrame      : 1U;
            uint8_t presentationTimeStamp    : 1U;
            uint8_t sourceClockReference : 1U;
            uint8_t reserved             : 1U;
            uint8_t stillImage           : 1U;
            uint8_t errorBit             : 1U;
            uint8_t endOfHeader          : 1U;
        */
        if (iso_packet[i].actual_length == 0) { /* skip no data */
            continue;
        }

        header_len = iso_packet[i].transfer_buffer[0];

        if ((header_len > 12) || (header_len == 0)) { /* do not be illegal */
            while (1) {
            }
        }
        if (iso_packet[i].transfer_buffer[1] & (1 << 6)) { /* error bit, re-receive */
            stream->bufoffset = 0;
            continue;
        }

        if ((stream->bufoffset == 0) && ((iso_packet[i].transfer_buffer[header_len] != 0xff) || (iso_packet[i].transfer_buffer[header_len + 1] != 0xd8))) {
            stream->bufoffset = 0;
            continue;
        }

        data_offset = header_len;
        data_len = iso_packet[i].actual_length - header_len;

        /** do something here */

        stream->bufoffset += data_len;

        if (iso_packet[i].transfer_buffer[1] & (1 << 1)) {
            if ((iso_packet[i].transfer_buffer[iso_packet[i].actual_length - 2] != 0xff) || (iso_packet[i].transfer_buffer[iso_packet[i].actual_length - 1] != 0xd9)) {
                stream->bufoffset = 0;
                continue;
            }

            /** do something here */

            if (stream->video_one_frame_callback) {
                stream->video_one_frame_callback(stream);
            }
            stream->bufoffset = 0;
        }
    }
    /** do something here */
}

void usbh_videostreaming_parse_yuyv2(struct usbh_urb *urb, struct usbh_videostreaming *stream)
{
    struct usbh_iso_frame_packet *iso_packet;
    uint32_t num_of_iso_packets;
    uint8_t data_offset;
    uint32_t data_len;
    uint8_t header_len = 0;

    num_of_iso_packets = urb->num_of_iso_packets;
    iso_packet = urb->iso_packet;

    for (uint32_t i = 0; i < num_of_iso_packets; i++) {
        /*
            uint8_t frameIdentifier : 1U;
            uint8_t endOfFrame      : 1U;
            uint8_t presentationTimeStamp    : 1U;
            uint8_t sourceClockReference : 1U;
            uint8_t reserved             : 1U;
            uint8_t stillImage           : 1U;
            uint8_t errorBit             : 1U;
            uint8_t endOfHeader          : 1U;
        */

        if (iso_packet[i].actual_length == 0) { /* skip no data */
            continue;
        }

        header_len = iso_packet[i].transfer_buffer[0];

        if ((header_len > 12) || (header_len == 0)) { /* do not be illegal */
            while (1) {
            }
        }
        if (iso_packet[i].transfer_buffer[1] & (1 << 6)) { /* error bit, re-receive */
            stream->bufoffset = 0;
            continue;
        }

        data_offset = header_len;
        data_len = iso_packet[i].actual_length - header_len;

        /** do something here */

        stream->bufoffset += data_len;

        if (iso_packet[i].transfer_buffer[1] & (1 << 1)) {
            /** do something here */

            if (stream->video_one_frame_callback && (stream->bufoffset == stream->buflen)) {
                stream->video_one_frame_callback(stream);
            }
            stream->bufoffset = 0;
        }
    }

    /** do something here */
}
#endif

#if TEST_USBH_CDC_ECM || TEST_USBH_RNDIS
#include "netif/etharp.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#endif

#ifdef __RTTHREAD__

#ifndef RT_USING_TIMER_SOFT
#error must enable RT_USING_TIMER_SOFT to support timer callback in thread
#endif

#include <rtthread.h>
#include <rtdevice.h>
#include <netif/ethernetif.h>
#include <netdev.h>

#else
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

TimerHandle_t dhcp_handle1;
TimerHandle_t dhcp_handle2;

static void dhcp_timeout(TimerHandle_t xTimer)
{
    struct netif *netif = (struct netif *)pvTimerGetTimerID(xTimer);
    struct dhcp *dhcp;

    if (netif_is_up(netif)) {
        dhcp = netif_dhcp_data(netif);

        if (dhcp && (dhcp->state == DHCP_STATE_BOUND)) {
            USB_LOG_INFO("IPv4 Address     : %s\r\n", ipaddr_ntoa(&netif->ip_addr));
            USB_LOG_INFO("IPv4 Subnet mask : %s\r\n", ipaddr_ntoa(&netif->netmask));
            USB_LOG_INFO("IPv4 Gateway     : %s\r\n\r\n", ipaddr_ntoa(&netif->gw));

            xTimerStop(xTimer, 0);
        }
    }
}
#endif
#endif

#if TEST_USBH_CDC_ECM
#include "usbh_cdc_ecm.h"

struct netif g_cdc_ecm_netif;

#ifdef __RTTHREAD__
static struct eth_device cdc_ecm_dev;

static rt_err_t rt_usbh_cdc_ecm_control(rt_device_t dev, int cmd, void *args)
{
    struct usbh_cdc_ecm *cdc_ecm_class = (struct usbh_cdc_ecm *)dev->user_data;

    switch (cmd) {
        case NIOCTL_GADDR:

            /* get mac address */
            if (args)
                rt_memcpy(args, cdc_ecm_class->mac, 6);
            else
                return -RT_ERROR;

            break;

        default:
            break;
    }

    return RT_EOK;
}

static rt_err_t rt_usbh_cdc_ecm_eth_tx(rt_device_t dev, struct pbuf *p)
{
    return usbh_cdc_ecm_linkoutput(NULL, p);
}
#endif

static err_t usbh_cdc_ecm_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_cdc_ecm_linkoutput;
    return ERR_OK;
}

void usbh_cdc_ecm_run(struct usbh_cdc_ecm *cdc_ecm_class)
{
#ifdef __RTTHREAD__
    struct netdev *netdev;

    memset(&cdc_ecm_dev, 0, sizeof(struct eth_device));

    cdc_ecm_dev.parent.control = rt_usbh_cdc_ecm_control;
    cdc_ecm_dev.eth_rx = NULL;
    cdc_ecm_dev.eth_tx = rt_usbh_cdc_ecm_eth_tx;
    cdc_ecm_dev.parent.user_data = cdc_ecm_class;

    eth_device_init(&cdc_ecm_dev, "u0");
    eth_device_linkchange(&cdc_ecm_dev, RT_TRUE);

    usb_osal_thread_create("usbh_cdc_ecm_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_cdc_ecm_rx_thread, cdc_ecm_dev.netif);
#else
    struct netif *netif = &g_cdc_ecm_netif;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, cdc_ecm_class->mac, 6);

    IP4_ADDR(&cdc_ecm_class->ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&cdc_ecm_class->netmask, 0, 0, 0, 0);
    IP4_ADDR(&cdc_ecm_class->gateway, 0, 0, 0, 0);

    netif = netif_add(netif, &cdc_ecm_class->ipaddr, &cdc_ecm_class->netmask, &cdc_ecm_class->gateway, NULL, usbh_cdc_ecm_if_init, tcpip_input);
    netif_set_default(netif);
    while (!netif_is_up(netif)) {
    }

    dhcp_handle1 = xTimerCreate((const char *)"dhcp1", (TickType_t)200, (UBaseType_t)pdTRUE, (void *const)netif, (TimerCallbackFunction_t)dhcp_timeout);
    if (dhcp_handle1 == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        while (1) {
        }
    }

    usb_osal_thread_create("usbh_cdc_ecm_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_cdc_ecm_rx_thread, netif);
#if LWIP_DHCP
    dhcp_start(netif);
    xTimerStart(dhcp_handle1, 0);
#endif
#endif
}

void usbh_cdc_ecm_stop(struct usbh_cdc_ecm *cdc_ecm_class)
{
#ifdef __RTTHREAD__
    eth_device_deinit(&cdc_ecm_dev);
#else
    struct netif *netif = &g_cdc_ecm_netif;
#if LWIP_DHCP
    dhcp_stop(netif);
    dhcp_cleanup(netif);
#endif
    netif_set_down(netif);
    netif_remove(netif);
#endif
}
#endif

#if TEST_USBH_RNDIS
#include "usbh_rndis.h"

struct netif g_rndis_netif;

#ifdef __RTTHREAD__

static struct eth_device rndis_dev;

static rt_timer_t keep_timer = RT_NULL;

static void rndis_dev_keepalive_timeout(void *parameter)
{
    struct usbh_rndis *rndis_class = (struct usbh_rndis *)parameter;
    usbh_rndis_keepalive(rndis_class);
}

static void timer_init(struct usbh_rndis *rndis_class)
{
    keep_timer = rt_timer_create("keep",
                                 rndis_dev_keepalive_timeout,
                                 rndis_class,
                                 5000,
                                 RT_TIMER_FLAG_PERIODIC |
                                     RT_TIMER_FLAG_SOFT_TIMER);

    rt_timer_start(keep_timer);
}

static rt_err_t rt_usbh_rndis_control(rt_device_t dev, int cmd, void *args)
{
    struct usbh_rndis *rndis_class = (struct usbh_rndis *)dev->user_data;

    switch (cmd) {
        case NIOCTL_GADDR:

            /* get mac address */
            if (args)
                rt_memcpy(args, rndis_class->mac, 6);
            else
                return -RT_ERROR;

            break;

        default:
            break;
    }

    return RT_EOK;
}

static rt_err_t rt_usbh_rndis_eth_tx(rt_device_t dev, struct pbuf *p)
{
    return usbh_rndis_linkoutput(NULL, p);
}

#else
TimerHandle_t timer_handle;

static void rndis_dev_keepalive_timeout(TimerHandle_t xTimer)
{
    struct usbh_rndis *rndis_class = (struct usbh_rndis *)pvTimerGetTimerID(xTimer);
    usbh_rndis_keepalive(rndis_class);
}

void timer_init(struct usbh_rndis *rndis_class)
{
    timer_handle = xTimerCreate((const char *)NULL, (TickType_t)5000, (UBaseType_t)pdTRUE, (void *const)rndis_class, (TimerCallbackFunction_t)rndis_dev_keepalive_timeout);
    if (NULL != timer_handle) {
        xTimerStart(timer_handle, 0);
    } else {
        USB_LOG_ERR("timer creation failed! \r\n");
        for (;;) {
           {};
        }
    }
}
#endif

static err_t usbh_rndis_if_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->output = etharp_output;
    netif->linkoutput = usbh_rndis_linkoutput;
    return ERR_OK;
}

void usbh_rndis_run(struct usbh_rndis *rndis_class)
{
#ifdef __RTTHREAD__
    struct netdev *netdev;

    memset(&rndis_dev, 0, sizeof(struct eth_device));

    rndis_dev.parent.control = rt_usbh_rndis_control;
    rndis_dev.eth_rx = NULL;
    rndis_dev.eth_tx = rt_usbh_rndis_eth_tx;
    rndis_dev.parent.user_data = rndis_class;

    eth_device_init(&rndis_dev, "u1");
    eth_device_linkchange(&rndis_dev, RT_TRUE);

    usb_osal_thread_create("usbh_rndis_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_rndis_rx_thread, rndis_dev.netif);
    timer_init(rndis_class);
#else
    struct netif *netif = &g_rndis_netif;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, rndis_class->mac, 6);

    IP4_ADDR(&rndis_class->ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&rndis_class->netmask, 0, 0, 0, 0);
    IP4_ADDR(&rndis_class->gateway, 0, 0, 0, 0);

    netif = netif_add(netif, &rndis_class->ipaddr, &rndis_class->netmask, &rndis_class->gateway, NULL, usbh_rndis_if_init, tcpip_input);
    netif_set_default(netif);
    while (!netif_is_up(netif)) {
    }

    dhcp_handle2 = xTimerCreate((const char *)"dhcp2", (TickType_t)200, (UBaseType_t)pdTRUE, (void *const)netif, (TimerCallbackFunction_t)dhcp_timeout);
    if (dhcp_handle2 == NULL) {
        USB_LOG_ERR("timer creation failed! \r\n");
        while (1) {
        }
    }

    usb_osal_thread_create("usbh_rndis_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_rndis_rx_thread, netif);
    timer_init(rndis_class);

#if LWIP_DHCP
    dhcp_start(netif);
    xTimerStart(dhcp_handle2, 0);
#endif
#endif
}

void usbh_rndis_stop(struct usbh_rndis *rndis_class)
{
#ifdef __RTTHREAD__
    eth_device_deinit(&rndis_dev);
    rt_timer_stop(keep_timer);
    rt_timer_delete(keep_timer);
#else
    struct netif *netif = &g_rndis_netif;
#if LWIP_DHCP
    dhcp_stop(netif);
    dhcp_cleanup(netif);
#endif
    netif_set_down(netif);
    netif_remove(netif);
    xTimerStop(timer_handle, 0);
    xTimerDelete(timer_handle, 0);
#endif
}
#endif

void usbh_cdc_acm_run(struct usbh_cdc_acm *cdc_acm_class)
{
#if TEST_USBH_CDC_ACM
    usb_osal_thread_create("usbh_cdc", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_cdc_acm_thread, NULL);
#endif
}

void usbh_cdc_acm_stop(struct usbh_cdc_acm *cdc_acm_class)
{
}

void usbh_hid_run(struct usbh_hid *hid_class)
{
#if TEST_USBH_HID
    usb_osal_thread_create("usbh_hid", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_hid_thread, NULL);
#endif
}

void usbh_hid_stop(struct usbh_hid *hid_class)
{
}

#ifndef __RTTHREAD__
void usbh_msc_run(struct usbh_msc *msc_class)
{
#if TEST_USBH_MSC
    usb_osal_thread_create("usbh_msc", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_msc_thread, NULL);
#endif
}

void usbh_msc_stop(struct usbh_msc *msc_class)
{
}
#endif

void usbh_audio_run(struct usbh_audio *audio_class)
{
#if TEST_USBH_AUDIO
#error "if you want to use iso, please contact with me"
    usb_osal_thread_create("usbh_audio", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_audio_thread, NULL);
#endif
}

void usbh_audio_stop(struct usbh_audio *audio_class)
{
}

void usbh_video_run(struct usbh_video *video_class)
{
#if TEST_USBH_VIDEO
#error "if you want to use iso, please contact with me"
    usb_osal_thread_create("usbh_video", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_video_thread, NULL);
#endif
}

void usbh_video_stop(struct usbh_video *video_class)
{
}

void usbh_class_test(void)
{
#ifdef __RTTHREAD__
    /* do nothing */
#else
#if TEST_USBH_CDC_ECM || TEST_USBH_RNDIS
    /* Initialize the LwIP stack */
    tcpip_init(NULL, NULL);
#endif
#endif
}
