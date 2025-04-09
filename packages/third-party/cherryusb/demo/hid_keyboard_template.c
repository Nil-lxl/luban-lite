/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbd_core.h"
#include "usbd_hid.h"

#define USBD_VID           0x33C3
#define USBD_PID           0x6788
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define HID_INT_EP          0x81
#define HID_INT_EP_SIZE     8
#define HID_INT_EP_INTERVAL 2

#define USB_HID_CONFIG_DESC_SIZ       34
#define HID_KEYBOARD_REPORT_DESC_SIZE 63

static const uint8_t hid_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_DESC_SIZ, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /************** Descriptor of Joystick Mouse interface ****************/
    /* 09 */
    0x09,                          /* bLength: Interface Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */
    0x00,                          /* bInterfaceNumber: Number of Interface */
    0x00,                          /* bAlternateSetting: Alternate setting */
    0x01,                          /* bNumEndpoints */
    0x03,                          /* bInterfaceClass: HID */
    0x01,                          /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x01,                          /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,                             /* iInterface: Index of string descriptor */
    /******************** Descriptor of Joystick Mouse HID ********************/
    /* 18 */
    0x09,                    /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x11,                    /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                          /* bCountryCode: Hardware target country */
    0x01,                          /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                          /* bDescriptorType */
    HID_KEYBOARD_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Mouse endpoint ********************/
    /* 27 */
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HID_INT_EP,                   /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    HID_INT_EP_SIZE,              /* wMaxPacketSize: 4 Byte max */
    0x00,
    HID_INT_EP_INTERVAL, /* bInterval: Polling Interval */
    /* 34 */
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
    0x26,                       /* bLength */
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
    'H', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'D', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
    ///////////////////////////////////////
    /// Other Speed Configuration Descriptor
    ///////////////////////////////////////
    0x09,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED,
    WBVAL(USB_HID_CONFIG_DESC_SIZ),
    0x01,
    0x01,
    0x00,
    USB_CONFIG_BUS_POWERED,
    USB_CONFIG_POWER_MA(USBD_MAX_POWER),
#endif
    0x00
};

#if 0
/* USB HID device Configuration Descriptor */
static uint8_t hid_desc[9] __ALIGN_END = {
    /* 18 */
    0x09,                    /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x11,                    /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                          /* bCountryCode: Hardware target country */
    0x01,                          /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                          /* bDescriptorType */
    HID_KEYBOARD_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */
    0x00,
};
#endif

static const uint8_t hid_keyboard_report_desc[HID_KEYBOARD_REPORT_DESC_SIZE] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0xe0, // USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7, // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x08, // REPORT_COUNT (8)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x03, // INPUT (Cnst,Var,Abs)
    0x95, 0x05, // REPORT_COUNT (5)
    0x75, 0x01, // REPORT_SIZE (1)
    0x05, 0x08, // USAGE_PAGE (LEDs)
    0x19, 0x01, // USAGE_MINIMUM (Num Lock)
    0x29, 0x05, // USAGE_MAXIMUM (Kana)
    0x91, 0x02, // OUTPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x03, // REPORT_SIZE (3)
    0x91, 0x03, // OUTPUT (Cnst,Var,Abs)
    0x95, 0x06, // REPORT_COUNT (6)
    0x75, 0x08, // REPORT_SIZE (8)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0xFF, // LOGICAL_MAXIMUM (255)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0x00, // USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65, // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00, // INPUT (Data,Ary,Abs)
    0xc0        // END_COLLECTION
};

/* ASCII */
const uint32_t hid_keyboard_map[128] = {
    [0] = HID_KBD_USAGE_NONE, [1] = HID_KBD_USAGE_NONE, [2] = HID_KBD_USAGE_NONE, [3] = HID_KBD_USAGE_NONE,
    [4] = HID_KBD_USAGE_NONE, [5] = HID_KBD_USAGE_NONE, [6] = HID_KBD_USAGE_NONE, [7] = HID_KBD_USAGE_NONE,
    [8] = HID_KBD_USAGE_NONE, [9] = HID_KBD_USAGE_NONE, [10] = HID_KBD_USAGE_NONE, [11] = HID_KBD_USAGE_NONE,
    [12] = HID_KBD_USAGE_NONE,
    [13] = HID_KBD_USAGE_ENTER, // Enter
    [14] = HID_KBD_USAGE_NONE, [15] = HID_KBD_USAGE_NONE, [16] = HID_KBD_USAGE_NONE, [17] = HID_KBD_USAGE_NONE,
    [18] = HID_KBD_USAGE_NONE, [19] = HID_KBD_USAGE_NONE, [20] = HID_KBD_USAGE_NONE, [21] = HID_KBD_USAGE_NONE,
    [22] = HID_KBD_USAGE_NONE, [23] = HID_KBD_USAGE_NONE, [24] = HID_KBD_USAGE_NONE, [25] = HID_KBD_USAGE_NONE,
    [26] = HID_KBD_USAGE_NONE, [27] = HID_KBD_USAGE_NONE, [28] = HID_KBD_USAGE_NONE, [29] = HID_KBD_USAGE_NONE,
    [30] = HID_KBD_USAGE_NONE, [31] = HID_KBD_USAGE_NONE, [127] = HID_KBD_USAGE_NONE,

    ['0'] = HID_KBD_USAGE_0, ['1'] = HID_KBD_USAGE_1, ['2'] = HID_KBD_USAGE_1 + 1,
    ['3'] = HID_KBD_USAGE_1 + 2, ['4'] = HID_KBD_USAGE_1 + 3, ['5'] = HID_KBD_USAGE_1 + 4,
    ['6'] = HID_KBD_USAGE_1 + 5, ['7'] = HID_KBD_USAGE_1 + 6, ['8'] = HID_KBD_USAGE_1 + 7,
    ['9'] = HID_KBD_USAGE_1 + 8,

    ['A'] = (HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_A),       ['B'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 1)),
    ['C'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 2)), ['D'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 3)),
    ['E'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 4)), ['F'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 5)),
    ['G'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 6)), ['H'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 7)),
    ['I'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 8)), ['J'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 9)),
    ['K'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 10)), ['L'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 11)),
    ['M'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 12)), ['N'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 13)),
    ['O'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 14)), ['P'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 15)),
    ['Q'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 16)), ['R'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 17)),
    ['S'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 18)), ['T'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 19)),
    ['U'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 20)), ['V'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 21)),
    ['W'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 22)), ['X'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 23)),
    ['Y'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 24)), ['Z'] = (HID_KBD_USAGE_POSTFAIL << 16 | (HID_KBD_USAGE_A + 25)),

    ['a'] = HID_KBD_USAGE_A, ['b'] = HID_KBD_USAGE_A + 1, ['c'] = HID_KBD_USAGE_A + 2,
    ['d'] = HID_KBD_USAGE_A + 3, ['e'] = HID_KBD_USAGE_A + 4, ['f'] = HID_KBD_USAGE_A + 5,
    ['g'] = HID_KBD_USAGE_A + 6, ['h'] = HID_KBD_USAGE_A + 7, ['i'] = HID_KBD_USAGE_A + 8,
    ['j'] = HID_KBD_USAGE_A + 9, ['k'] = HID_KBD_USAGE_A + 10, ['l'] = HID_KBD_USAGE_A + 11,
    ['m'] = HID_KBD_USAGE_A + 12, ['n'] = HID_KBD_USAGE_A + 13, ['o'] = HID_KBD_USAGE_A + 14,
    ['p'] = HID_KBD_USAGE_A + 15, ['q'] = HID_KBD_USAGE_A + 16, ['r'] = HID_KBD_USAGE_A + 17,
    ['s'] = HID_KBD_USAGE_A + 18, ['t'] = HID_KBD_USAGE_A + 19, ['u'] = HID_KBD_USAGE_A + 20,
    ['v'] = HID_KBD_USAGE_A + 21, ['w'] = HID_KBD_USAGE_A + 22, ['x'] = HID_KBD_USAGE_A + 23,
    ['y'] = HID_KBD_USAGE_A + 24, ['z'] = HID_KBD_USAGE_A + 25,

    [' '] = HID_KBD_USAGE_SPACE,
    ['\t'] = HID_KBD_USAGE_TAB,
    ['!'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_EXCLAM,
    ['"'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_DQUOUTE,
    ['#'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_POUND,
    ['$'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_DOLLAR,
    ['%'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_PERCENT,
    ['&'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_AMPERSAND,
    ['\''] = HID_KBD_USAGE_DQUOUTE,
    ['('] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_LPAREN,
    [')'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_RPAREN,
    ['*'] = HID_KBD_USAGE_KPDMUL,
    ['+'] = HID_KBD_USAGE_KPDPLUS,
    [','] = HID_KBD_USAGE_COMMON,
    ['-'] = HID_KBD_USAGE_HYPHEN,
    ['.'] = HID_KBD_USAGE_PERIOD,
    ['/'] = HID_KBD_USAGE_KPDDIV,
    [':'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_COLON,
    [';'] = HID_KBD_USAGE_COLON,
    ['<'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_LT,
    ['='] = HID_KBD_USAGE_EQUAL,
    ['>'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_GT,
    ['?'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_QUESTION,
    ['@'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_AT,
    ['['] = HID_KBD_USAGE_LBRACKET,
    ['\\'] = HID_KBD_USAGE_BSLASH,
    [']'] = HID_KBD_USAGE_RBRACKET,
    ['^'] = HID_KBD_USAGE_KPDEXP,
    ['_'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_UNDERSCORE,
    ['`'] = HID_KBD_USAGE_NONUSPOUND,
    ['{'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_LBRACKET,
    ['|'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_BSLASH,
    ['}'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_RBRACKET,
    ['~'] = HID_KBD_USAGE_POSTFAIL << 16 | HID_KBD_USAGE_TILDE,
};

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
void usbd_comp_keyboard_event_handler(uint8_t event)
#else
void usbd_event_handler(uint8_t event)
#endif
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
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

#define HID_STATE_IDLE 0
#define HID_STATE_BUSY 1

/*!< hid state ! Data can be sent only when state is idle  */
static volatile uint8_t hid_state = HID_STATE_IDLE;
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[64];

void usbd_hid_int_callback(uint8_t ep, uint32_t nbytes)
{
    if (hid_state == HID_STATE_BUSY)
        hid_state = HID_STATE_IDLE;
}

static struct usbd_endpoint hid_in_ep = {
    .ep_cb = usbd_hid_int_callback,
    .ep_addr = HID_INT_EP
};

static struct usbd_interface intf0;

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
int usbd_comp_keyboard_init(uint8_t *ep_table, void *data)
{
    hid_in_ep.ep_addr = ep_table[0];
    usbd_add_interface(usbd_hid_init_intf(&intf0, hid_keyboard_report_desc, HID_KEYBOARD_REPORT_DESC_SIZE));
    usbd_add_endpoint(&hid_in_ep);
    return 0;
}
#endif

void hid_keyboard_init(void)
{

#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_desc_register(hid_descriptor);
    usbd_add_interface(usbd_hid_init_intf(&intf0, hid_keyboard_report_desc, HID_KEYBOARD_REPORT_DESC_SIZE));
    usbd_add_endpoint(&hid_in_ep);

    usbd_initialize();
#else
    usbd_comp_func_register(hid_descriptor,
                            usbd_comp_keyboard_event_handler,
                            usbd_comp_keyboard_init, "keyboard");
#endif

}

int usbd_keyboard_putchar(const char str)
{
    uint8_t index = (uint8_t)str;

    if (index > 128 || index < 0) {
        return -index;
    }

    USB_LOG_DBG("[%d]: %c -> %#x\n", index, str, hid_keyboard_map[index]);

    /* keydown */
    write_buffer[0] = (uint8_t)(hid_keyboard_map[index] >> 16);
    write_buffer[1] = 0x00;
    write_buffer[2] = (uint8_t)hid_keyboard_map[index];

    int ret = usbd_ep_start_write(hid_in_ep.ep_addr, write_buffer, 8);
    if (ret < 0) {
        USB_LOG_WRN("USB send data failed. %d\n", ret);
        return ret;
    }

    hid_state = HID_STATE_BUSY;
    while (hid_state == HID_STATE_BUSY) {
    }

    /* keyup */
    memset(write_buffer, 0, 8);
    ret = usbd_ep_start_write(hid_in_ep.ep_addr, write_buffer, 8);
    if (ret < 0) {
        USB_LOG_WRN("USB send data failed. %d\n", ret);
        return ret;
    }

    hid_state = HID_STATE_BUSY;
    while (hid_state == HID_STATE_BUSY) {
    }

    return 0;
}

int usbd_keyboard_putnchar(const char *str, int n)
{
    int i, ret;

    for (i = 0; i < n; i++) {
        ret = usbd_keyboard_putchar(str[i]);
        if (ret < 0) {
            USB_LOG_WRN("This character cannot printed! index: %d\n", i);
        }
    }

    return n;
}

void hid_keyboard_test(void)
{
    for (int i = 0; i < 128; i++) {
        usbd_keyboard_putchar((char)i);
    }
}

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdevice.h>

int usbd_hid_keyboard_init(void)
{
    hid_keyboard_init();
    return 0;
}
INIT_DEVICE_EXPORT(usbd_hid_keyboard_init);

#if defined (RT_USING_FINSH)
#include <finsh.h>

int test_usbd_hid_keyboard(int argc, char **argv)
{
    if (argc > 1) {
        usbd_keyboard_putnchar(argv[1], sizeof(argv[1]));
    } else {
        hid_keyboard_test();
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(test_usbd_hid_keyboard, test_usbd_hid_keyboard, test usb device hid keyboard);
#endif
#endif
