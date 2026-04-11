/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-16     Lenoyan      the first version
 */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sysinit/sysinit.h>
#include <syscfg/syscfg.h>
#include "os/os_mbuf.h"
#include "nimble/transport.h"
#ifdef LPKG_NIMBLE_HCI_H5
#include "nimble/transport/hci_h5.h"
#else
#include "nimble/transport/hci_h4.h"
#endif

#include <rtthread.h>
#include <rtdevice.h>

#if defined(LPKG_NIMBLE_HCI_H5)
struct hci_h5_sm g_hci_h5sm;
#elif defined(LPKG_NIMBLE_HCI_H4)
struct hci_h4_sm g_hci_h4sm;
#else
#error "Not support HCI protocol"
#endif

static rt_device_t g_serial;
static char g_msg_pool[256] = {0};
static struct rt_messagequeue g_rx_mq;

#if defined(LPKG_NIMBLE_HCI_H5)
static const uint8_t sync_req[] = { 0x01, 0x7e };
static const uint8_t sync_rsp[] = { 0x02, 0x7d };
/* Third byte may change */
static uint8_t conf_req[3] = { 0x03, 0xfc };
static const uint8_t conf_rsp[] = { 0x04, 0x7b };

static int hci_h5_uart_frame_cb(uint8_t pkt_type, void *data)
{

    switch (pkt_type) {
    case HCI_H5_LINK:
        if (!memcmp(data, sync_req, sizeof(sync_req))) {
            if (g_hci_h5sm.link_state == ACTIVE) {
                /* TODO Reset H5 */
                g_hci_h5sm.link_state = UNINIT;
            }

            hci_h5_sm_send(&g_hci_h5sm, HCI_H5_LINK, sync_rsp, sizeof(sync_rsp));
        } else if (!memcmp(data, sync_rsp, sizeof(sync_rsp))) {
            if (g_hci_h5sm.link_state == ACTIVE) {
                /* TODO Reset H5 */
                g_hci_h5sm.link_state = UNINIT;
            }

            g_hci_h5sm.link_state = INIT;
            conf_req[2] = g_hci_h5sm.tx_win & 0x07;
            hci_h5_sm_send(&g_hci_h5sm, HCI_H5_LINK, conf_req, sizeof(conf_req));
        } else if (!memcmp(data, conf_req, 2)) {
            /* The Host sends Config Response messages without a Configuration Field.*/
            hci_h5_sm_send(&g_hci_h5sm, HCI_H5_LINK, conf_rsp, sizeof(conf_rsp));
            /* Then send Config Request with Configuration Field */
            conf_req[2] = g_hci_h5sm.tx_win & 0x07;
            hci_h5_sm_send(&g_hci_h5sm, HCI_H5_LINK, conf_req, sizeof(conf_req));
        } else if (!memcmp(data, conf_rsp, 2)) {
            g_hci_h5sm.link_state = ACTIVE;
            /* Configuration field present */
            g_hci_h5sm.tx_win = (((uint8_t *)data)[2] & 0x07);
            pr_debug("Finished H5 configuration, tx_win %u\n", g_hci_h5sm.tx_win);
        } else {
            pr_err("Not handled yet %x %x\n", ((uint8_t *)data)[0], ((uint8_t *)data)[1]);
        }
        aicos_mdelay(100);
        break;
    case HCI_H5_EVT:
        return ble_transport_to_hs_evt(data);
    case HCI_H5_ACL:
        return ble_transport_to_hs_acl(data);
    default:
        assert(0);
        break;
    }
    return -1;
}
#elif defined(LPKG_NIMBLE_HCI_H4)
static int hci_h4_uart_frame_cb(uint8_t pkt_type, void *data)
{
    switch (pkt_type) {
    case HCI_H4_EVT:
        return ble_transport_to_hs_evt(data);
    case HCI_H4_ACL:
        return ble_transport_to_hs_acl(data);
    default:
        assert(0);
        break;
    }
    return -1;
}
#endif

static void rtthread_uart_rx_entry(void *parameter)
{
    rt_size_t size;
    uint8_t data[2048];
    size_t data_len;

    while (1) {
        if (!rt_mq_recv(&g_rx_mq, &size, sizeof(rt_size_t), 10000)) {
            data_len = rt_device_read(g_serial, 0, &data, size);
            if (data_len > 0) {
#if defined(LPKG_NIMBLE_HCI_H5) // H5
                hci_h5_sm_rx(&g_hci_h5sm, data, data_len);
#elif defined(LPKG_NIMBLE_HCI_H4)
                hci_h4_sm_rx(&g_hci_h4sm, data, data_len);
#endif
            }
        }
    }
}

void rtthread_uart_tx(const uint8_t *buf, size_t len)
{
    size_t remaining = len;
    size_t tx_size = 0;

    while (remaining > 0) {
        tx_size = rt_device_write(g_serial, 0, buf, remaining);
        buf += tx_size;
        remaining -= tx_size;
    }
}

static rt_err_t serial_input_mq(rt_device_t dev, rt_size_t size)
{
    rt_err_t result;

    result = rt_mq_send(&g_rx_mq, &size, sizeof(rt_size_t));
    if (result == -RT_EFULL) {
        pr_err("message queue full!\n");
    }
    return result;
}

static int rtthread_hci_uart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    g_serial = rt_device_find(LPKG_NIMBLE_HCI_UART_DEVICE_NAME);
    if (!g_serial) {
        pr_err("find %s failed!\n", LPKG_NIMBLE_HCI_UART_DEVICE_NAME);
        return -1;
    }

    if (rt_device_control(g_serial, RT_SERIAL_GET_CONFIG, &config) != RT_EOK) {
        rt_kprintf("uart get configure parameter fail!\n");
        return -1;
    }

    config.baud_rate = BAUD_RATE_115200;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
#if defined(LPKG_NIMBLE_HCI_H5) // H5
    config.parity    = PARITY_EVEN;
#elif defined(LPKG_NIMBLE_HCI_H4)
    config.baud_rate = BAUD_RATE_1500000;
    config.parity    = PARITY_NONE;
    config.flowcontrol = RT_SERIAL_FLOWCONTROL_CTSRTS;
    config.function = RT_SERIAL_RS232_AUTO_FLOW_CTRL;
#endif

    if (rt_device_control(g_serial, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK) {
        pr_err("uart set baudrate fail!\n");
        return -1;
    }

    rt_device_set_rx_indicate(g_serial, serial_input_mq);
    rt_mq_init(&g_rx_mq, "hci_rx_mq",
               g_msg_pool,
               sizeof(rt_size_t),
               sizeof(g_msg_pool),
               RT_IPC_FLAG_FIFO);

#ifdef RT_USING_SERIAL_V2
    rt_device_open(g_serial, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
#else
    rt_device_open(g_serial, RT_DEVICE_FLAG_INT_RX);
#endif

    rt_thread_t rx_thread = rt_thread_create("hci_uart_rx", rtthread_uart_rx_entry,
                                            RT_NULL, 4096, 24, 10);
    if (rx_thread != RT_NULL) {
        rt_thread_startup(rx_thread);
    } else {
        rt_device_close(g_serial);
        return -1;
    }

    return 0;
}


int ble_transport_to_ll_cmd_impl(void *buf)
{
    uint8_t *cmd_pkt_data = (uint8_t *)buf;
    size_t pkt_len = cmd_pkt_data[2] + 3;

#if defined(LPKG_NIMBLE_HCI_H5) // H5
    if (g_hci_h5sm.link_state == ACTIVE) {
        hci_h5_sm_send(&g_hci_h5sm, HCI_H5_CMD, buf, pkt_len);
    }
#elif defined(LPKG_NIMBLE_HCI_H4)
    uint8_t indicator = 0;
    indicator = HCI_H4_CMD;
    rtthread_uart_tx(&indicator, 1);
    rtthread_uart_tx(cmd_pkt_data, pkt_len);

#endif
    ble_transport_free(buf);

    return 0;
}

int ble_transport_to_ll_acl_impl(struct os_mbuf *om)
{

#if defined(LPKG_NIMBLE_HCI_H5) // H5
    uint16_t data_len = OS_MBUF_PKTLEN(om);
    uint8_t *data = malloc(data_len);

    if (g_hci_h5sm.link_state == ACTIVE) {
        os_mbuf_copydata(om, 0, data_len, data);
        hci_h5_sm_send(&g_hci_h5sm, HCI_H5_ACL, data, data_len);
    }

    free(data);
    os_mbuf_free_chain(om);
#elif defined(LPKG_NIMBLE_HCI_H4)
    uint8_t indicator = 0;

    indicator = HCI_H4_ACL;

    rtthread_uart_tx(&indicator, 1);

    struct os_mbuf *x = om;
    while (x != NULL)
    {
        rtthread_uart_tx(x->om_data, x->om_len);
        x = SLIST_NEXT(x, om_next);
    }

    os_mbuf_free_chain(om);
#endif

    return 0;
}

static int rtthread_ble_transport_init(void)
{
    int rc;
    SYSINIT_ASSERT_ACTIVE();

    rc = rtthread_hci_uart_init();
    SYSINIT_PANIC_ASSERT(rc == 0);

#if defined(LPKG_NIMBLE_HCI_H5) // H5
    hci_h5_sm_init(&g_hci_h5sm, &hci_h5_allocs_from_ll, hci_h5_uart_frame_cb);
    hci_h5_sm_send(&g_hci_h5sm, HCI_H5_LINK, sync_req, sizeof(sync_req));
#elif defined(LPKG_NIMBLE_HCI_H4)
    hci_h4_sm_init(&g_hci_h4sm, &hci_h4_allocs_from_ll, hci_h4_uart_frame_cb);
#endif
    return 0;
}
#ifdef LPKG_NIMBLE_HCI_USING_RTT_UART
INIT_APP_EXPORT(rtthread_ble_transport_init);
#endif
