/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xindong.hu@artinchip.com
 */

#include "usbd_core.h"
#include "../class/midi/usbd_midi.h"

#define MIDI_OUT_EP 0x02
#define MIDI_IN_EP  0x81

#define USBD_VID           0x33C3
#define USBD_PID           0x0404
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + 9 + 9 + 9 + 7 + MIDI_SIZEOF_JACK_DESC + 9 + 5 + 9 + 5)

#ifdef CONFIG_USB_HS
#define MIDI_EP_MPS 512
#else
#define MIDI_EP_MPS 64
#endif

#define MIDI_OUT_PACKET   4
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[MIDI_OUT_PACKET];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[MIDI_OUT_PACKET];

const uint8_t midi_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    // Standard AC Interface Descriptor
    0x09,
    0x04,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,
    // Class-specific AC Interface Descriptor
    0x09,
    0x24,
    0x01,
    0x00,
    0x01,
    0x09,
    0x00,
    0x01,
    0x01,
    // MIDIStreaming Interface Descriptors
    0x09,
    0x04,
    0x01,
    0x00,
    0x02,
    0x01,
    0x03,
    0x00,
    0x00,
    // Class-Specific MS Interface Header Descriptor
    0x07,
    0x24,
    0x01,
    0x00,
    0x01,
    WBVAL(65),

    MIDI_JACK_DESCRIPTOR_INIT(0x01),
    // OUT endpoint descriptor
    0x09, 0x05, MIDI_OUT_EP, 0x02, WBVAL(MIDI_EP_MPS), 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x01,

    // IN endpoint descriptor
    0x09, 0x05, MIDI_IN_EP, 0x02, WBVAL(MIDI_EP_MPS), 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x03,

    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x28,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'M', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'D', 0x00,                  /* wcChar12 */
    'I', 0x00,                  /* wcChar13 */
    ' ', 0x00,                  /* wcChar14 */
    'D', 0x00,                  /* wcChar15 */
    'E', 0x00,                  /* wcChar16 */
    'M', 0x00,                  /* wcChar17 */
    'O', 0x00,                  /* wcChar18 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '0', 0x00,                  /* wcChar9 */
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
    ///////////////////////////////////////
    /// Other Speed Configuration Descriptor
    ///////////////////////////////////////
    0x09,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED,
    WBVAL(USB_CONFIG_SIZE),
    0x02,
    0x01,
    0x00,
    0x80,
    USB_CONFIG_POWER_MA(100),

    // Standard AC Interface Descriptor
    0x09,
    0x04,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,
    // Class-specific AC Interface Descriptor
    0x09,
    0x24,
    0x01,
    0x00,
    0x01,
    0x09,
    0x00,
    0x01,
    0x01,
    // MIDIStreaming Interface Descriptors
    0x09,
    0x04,
    0x01,
    0x00,
    0x02,
    0x01,
    0x03,
    0x00,
    0x00,
    // Class-Specific MS Interface Header Descriptor
    0x07,
    0x24,
    0x01,
    0x00,
    0x01,
    WBVAL(65),
    MIDI_JACK_DESCRIPTOR_INIT(0x01),
    // OUT endpoint descriptor
    0x09, 0x05, MIDI_OUT_EP, 0x02, WBVAL(MIDI_EP_MPS), 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x01,

    // IN endpoint descriptor
    0x09, 0x05, MIDI_IN_EP, 0x02, WBVAL(MIDI_EP_MPS), 0x00, 0x00, 0x00,
    0x05, 0x25, 0x01, 0x01, 0x03,
    0x00
};

/********************************************************************************
* @function   :  usbd_midi_input_buffer
* @parameters ： data: This data is the midi streaming input to the master device
* @return:       if run success,return 0
* @Description:  This function is a customer-defined function
********************************************************************************/
int usbd_midi_input_buffer(uint8_t *data)
{
    return 0;
}

/********************************************************************************
* @function   :  usbd_ms_decoder
* @parameters ： data: This data is undecoderd,need decoder
* @return:       if run success,return 0
* @Description:  This function is a customer-defined function
********************************************************************************/
int usbd_ms_decoder(uint8_t *data)
{
    return 0;
}

void usbd_clear_buffer(void)
{
    static uint8_t reset_num = 1;
    if (reset_num++>2) {
        USB_LOG_INFO("usbd_clear_buffer\r\n");
        memset(read_buffer, 0, MIDI_OUT_PACKET);
        memset(write_buffer, 0, MIDI_OUT_PACKET);
    }
}

void usbd_event_handler(uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            usbd_clear_buffer();
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
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_midi_bulk_out(uint8_t ep, uint32_t nbytes)
{
    if (usbd_ms_decoder(read_buffer)) {
        USB_LOG_ERR("midi streaming decoder fail!\r\n");
        return;
    }
    usbd_ep_start_read(MIDI_OUT_EP, read_buffer, MIDI_OUT_PACKET);
}

void usbd_midi_bulk_in(uint8_t ep, uint32_t nbytes)
{
    if (usbd_midi_input_buffer(write_buffer)) {
        USB_LOG_ERR("midi input buffer fail!\r\n");
        return;
    }
    usbd_ep_start_write(MIDI_IN_EP, write_buffer, MIDI_OUT_PACKET);
}

void usbd_midi_open(void)
{
    usbd_ep_start_read(MIDI_OUT_EP, read_buffer, MIDI_OUT_PACKET);
}

struct usbd_interface intf0;
struct usbd_interface intf1;

struct usbd_endpoint midi_out_ep = {
    .ep_addr = MIDI_OUT_EP,
    .ep_cb = usbd_midi_bulk_out
};

struct usbd_endpoint midi_in_ep = {
    .ep_addr = MIDI_IN_EP,
    .ep_cb = usbd_midi_bulk_in
};

void midi_init(void)
{
    usbd_desc_register(midi_descriptor);
    usbd_add_interface(usbd_midi_init_intf(&intf0));
    usbd_add_interface(usbd_midi_init_intf(&intf1));
    usbd_add_endpoint(&midi_out_ep);
    usbd_add_endpoint(&midi_in_ep);

    usbd_initialize();
}

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdevice.h>

int usbd_midi_init(void)
{
    midi_init();
    return 0;
}

INIT_DEVICE_EXPORT(usbd_midi_init);
#endif
