/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: duanmt@artinchip.com
 */

#include <rtdevice.h>
#include <drivers/serial.h>

#include "usbd_core.h"
#include "usbd_cdc.h"

#define DEBUG

#ifdef DEBUG
#undef USB_LOG_DBG
#define USB_LOG_DBG USB_LOG_INFO
#endif

/*!< endpoint address */
#define CDC_INT_EP  0x83
#define CDC_OUT_EP  0x02
#define CDC_IN_EP   0x81

#define USBD_VID           0x33C3
#define USBD_PID           0x7788
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define AICUPG_UART_DEV_NO 2
#define DEBUG_ENABLE_LOCAL_UART

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

/*!< global descriptor */
static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x2A,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'A', 0x00,                  /* wcChar0 */
    'r', 0x00,                  /* wcChar1 */
    't', 0x00,                  /* wcChar2 */
    'I', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    'C', 0x00,                  /* wcChar5 */
    'h', 0x00,                  /* wcChar6 */
    'i', 0x00,                  /* wcChar7 */
    'p', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'L', 0x00,                  /* wcChar10 */
    'i', 0x00,                  /* wcChar11 */
    'n', 0x00,                  /* wcChar12 */
    'k', 0x00,                  /* wcChar13 */
    'e', 0x00,                  /* wcChar14 */
    'r', 0x00,                  /* wcChar15 */
    ' ', 0x00,                  /* wcChar16 */
    '2', 0x00,                  /* wcChar16 */
    '.', 0x00,                  /* wcChar16 */
    '0', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x12,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '0', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '5', 0x00,                  /* wcChar5 */
    '2', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x02,
    0x02,
    0x01,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

#define AICUPG_BUF_SIZE         2048

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[AICUPG_BUF_SIZE] = "";
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[AICUPG_BUF_SIZE] = "";

volatile bool ep_tx_busy_flag = false;

#ifdef DEBUG_ENABLE_LOCAL_UART
#include <rtthread.h>

static rt_device_t g_aicupg_uart = NULL;
static struct cdc_line_coding g_aicupg_line = {115200, 0, 0, 8};

static struct rt_semaphore g_uart_sem;
static struct rt_ringbuffer g_uart_rb = {0};
static unsigned int g_uart_dat_sz = 0;
static uint8_t g_uart_buf[AICUPG_BUF_SIZE] = "";
static uint32_t g_acm_in_is_idle = 1;
#endif

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

void usbd_event_handler(uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            usbd_ep_start_read(CDC_OUT_EP, read_buffer, AICUPG_BUF_SIZE);
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t ep, uint32_t nbytes)
{
    // USB_LOG_RAW("Out len: %ld\n", nbytes);
#ifdef DEBUG_ENABLE_LOCAL_UART
    if (nbytes > 0) {
        int ret = 0;

        ret = rt_device_write(g_aicupg_uart, -1, (const void *)read_buffer,
                              (rt_size_t)nbytes);
        if (ret != nbytes)
            USB_LOG_INFO("rt_device_write(%d) return %d\n", (u32)nbytes, ret);
    }
#endif

    /* setup next out ep read transfer */
    usbd_ep_start_read(CDC_OUT_EP, read_buffer, AICUPG_BUF_SIZE);
}

void usbd_cdc_acm_bulk_in(uint8_t ep, uint32_t nbytes)
{
    rt_size_t len = nbytes;

#ifdef DEBUG_ENABLE_LOCAL_UART
    len = rt_ringbuffer_data_len(&g_uart_rb);
    // USB_LOG_DBG("In len: %d/%ld\n", len, nbytes);
    rt_ringbuffer_get(&g_uart_rb, write_buffer, len);

    len = rt_ringbuffer_data_len(&g_uart_rb);
    if (len == 0) {
        g_acm_in_is_idle = 1;
        return;
    }
#endif

    usbd_ep_start_write(CDC_IN_EP, write_buffer, len);
#ifdef DEBUG_ENABLE_LOCAL_UART
    g_acm_in_is_idle = 0;
#endif

    if (len && (len % CDC_MAX_MPS) == 0) {
        /* send zlp */
        usbd_ep_start_write(CDC_IN_EP, NULL, 0);
    } else {
        ep_tx_busy_flag = false;
    }
}

/*!< endpoint call back */
struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

static struct usbd_interface intf0;
static struct usbd_interface intf1;

#ifdef DEBUG_ENABLE_LOCAL_UART

rt_err_t aicupg_uart_recv_cb(rt_device_t dev, rt_size_t size)
{
    if (size > 0) {
        g_uart_dat_sz = size;
        rt_sem_release(&g_uart_sem);
    }

    return RT_EOK;
}

void aicupg_uart_recv_thread(void *argv)
{
    int ret = 0;

    aicos_msleep(3000);
    while (1) {
        rt_sem_take(&g_uart_sem, RT_WAITING_FOREVER);
        if (!g_uart_dat_sz)
            continue;

        if (g_uart_dat_sz > AICUPG_BUF_SIZE / 2) {
            pr_err("The data length %d is too bigger!\n", g_uart_dat_sz);
            g_uart_dat_sz = AICUPG_BUF_SIZE / 2;
        }

        ret = rt_device_read(g_aicupg_uart, -1, write_buffer, g_uart_dat_sz);
        if (ret > 0) {
            write_buffer[ret] = 0;
            pr_debug("Idle %d, recv %d data: |%s|\n",
                     g_acm_in_is_idle, ret, write_buffer);
            rt_ringbuffer_put_force(&g_uart_rb, write_buffer, g_uart_dat_sz);
            if (g_acm_in_is_idle)
                usbd_ep_start_write(CDC_IN_EP, write_buffer, g_uart_dat_sz);
        } else {
            if (ret < 0)
                pr_warn("read() return [%ld] %s\n",
                         rt_get_errno(), rt_strerror(rt_get_errno()));
        }
        aicos_msleep(10);
    }

    rt_sem_detach(&g_uart_sem);
    rt_device_close(g_aicupg_uart);
}

#endif

int cdc_acm_init(void)
{
#ifdef DEBUG_ENABLE_LOCAL_UART
    rt_thread_t thid = NULL;
    char name[8] = "";
    int ret = 0;

    snprintf(name, 8, "uart%d", AICUPG_UART_DEV_NO);
    pr_info("Link %s to ACM ...\n", name);
    g_aicupg_uart = rt_device_find(name);
    if (!g_aicupg_uart) {
        pr_err("Failed to find %s\n", name);
        return -1;
    }
    ret = rt_device_open(g_aicupg_uart,
                         RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (ret) {
        pr_err("Failed to open %s\n", name);
        return -1;
    }
    rt_device_set_rx_indicate(g_aicupg_uart, aicupg_uart_recv_cb);

    rt_ringbuffer_init(&g_uart_rb, g_uart_buf, AICUPG_BUF_SIZE);
    rt_sem_init(&g_uart_sem, name, 0, RT_IPC_FLAG_FIFO);

    thid = rt_thread_create(name, aicupg_uart_recv_thread,
                            RT_NULL, 1024*2, 0, 10);
    if (thid != RT_NULL) {
        rt_thread_startup(thid);
    } else {
        pr_err("Failed to create thread %s\n", name);
        rt_device_close(g_aicupg_uart);
        return -RT_ERROR;
    }
#endif

    usbd_desc_register(cdc_descriptor);
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf0));
    usbd_add_interface(usbd_cdc_acm_init_intf(&intf1));
    usbd_add_endpoint(&cdc_out_ep);
    usbd_add_endpoint(&cdc_in_ep);
    usbd_initialize();
    return 0;
}
INIT_DEVICE_EXPORT(cdc_acm_init);

volatile uint8_t dtr_enable = 0;

void usbd_cdc_acm_set_dtr(uint8_t intf, bool dtr)
{
    if (dtr) {
        USB_LOG_INFO("intf: %d, dtr: %d\n", intf, dtr);
        dtr_enable = 1;
    } else {
        dtr_enable = 0;
    }
}

void cdc_acm_data_send_with_dtr_test(void)
{
    USB_LOG_INFO("dtr_enable: %d, busy_flag: %d\n", dtr_enable, ep_tx_busy_flag);
    if (dtr_enable) {
        ep_tx_busy_flag = true;
        usbd_ep_start_write(CDC_IN_EP, write_buffer, AICUPG_BUF_SIZE);
        while (ep_tx_busy_flag) {
        }
    }
}

#ifdef DEBUG_ENABLE_LOCAL_UART

#pragma GCC optimize ("O0")
static int baudrate_is_valid(u32 rate)
{
    int i;
    u32 rates[] = {BAUD_RATE_115200,
                   BAUD_RATE_230400,
                   BAUD_RATE_460800,
                   BAUD_RATE_921600,
                   BAUD_RATE_2000000,
                   BAUD_RATE_3000000};

    for (i = 0; i < ARRAY_SIZE(rates); i++) {
        if (rate == rates[i])
            return 1;
    }

    return 0;
}

rt_err_t aic_uart_configure(u32 index, struct serial_configure *cfg);

void usbd_cdc_acm_set_line_coding(uint8_t intf,
                                  struct cdc_line_coding *line_coding)
{
    struct serial_configure cfg = {0};
    int ret = 0;

    if (!g_aicupg_uart)
        return;

    if (!memcmp(line_coding, &g_aicupg_line, sizeof(struct cdc_line_coding))) {
        USB_LOG_DBG("%s() No change\n", __func__);
        return;
    }

    if (!baudrate_is_valid(line_coding->dwDTERate)) {
        pr_warn("Invalid baudrate %d\n", (u32)line_coding->dwDTERate);
        return;
    }
    cfg.baud_rate = line_coding->dwDTERate;

    if (line_coding->bDataBits < DATA_BITS_5
        || line_coding->bDataBits > DATA_BITS_8) {
        pr_warn("Invalid data bits %d\n", line_coding->bDataBits);
        return;
    }
    cfg.data_bits = line_coding->bDataBits;
    cfg.stop_bits = 1;

    USB_LOG_INFO("Change UART: Baud rate %d, Bits %d, Parity %d\n",
            (u32)line_coding->dwDTERate, line_coding->bDataBits,
            line_coding->bParityType);

    ret = aic_uart_configure(AICUPG_UART_DEV_NO, &cfg);
    if (ret < 0) {
        pr_err("Failed to config UART.  ret = -%d\n", -ret);
        return;
    }
    memcpy(&g_aicupg_line, line_coding, sizeof(struct cdc_line_coding));
}

void usbd_cdc_acm_get_line_coding(uint8_t intf,
                                  struct cdc_line_coding *line_coding)
{
    if (!g_aicupg_uart)
        return;

    memcpy(line_coding, &g_aicupg_line, sizeof(struct cdc_line_coding));
}

#endif
