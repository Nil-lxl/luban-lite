/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  senye Liang <senye.liang@artinchip.com>
 */
#include "usbd_core.h"
#include "usbd_hid.h"
#include <rtthread.h>
#include <rtdevice.h>

#define TOUCH_MAX_POINT_NUM     5
#define TOUCH_LOGICAL_MAXNUM    4096
#define TOUCH_LOGICAL_MINNUM    0
#define TOUCH_PHYSICAL_MINNUM   0
#define TOUCHSCREEN_PACKET_SIZE (TOUCH_MAX_POINT_NUM * 10 + 4)
#define TOUCHPAD_PACKET_SIZE    (TOUCH_MAX_POINT_NUM * 10 + 5)
#define GLOBAL_REPORT_SIZE      TOUCHPAD_PACKET_SIZE
/*!< endpoint address */
#define HID_INT_EP          0x81
#define HID_INT_EP_SIZE     (GLOBAL_REPORT_SIZE)
#define HID_INT_EP_INTERVAL 10

#define USBD_VID           0x33C3
#define USBD_PID           0x6789
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_HID_CONFIG_DESC_SIZ 34
/*!< g_touch_report descriptor size */
#define HID_TOUCHSCREEN_REPORT_DESC_SIZE    (73  + 90 * TOUCH_MAX_POINT_NUM)
#define HID_TOUCHPAD_REPORT_DESC_SIZE       (196 + 92 * TOUCH_MAX_POINT_NUM)
#define HID_TOUCH_REPORT_DESC_SIZE          (73  + 90 * TOUCH_MAX_POINT_NUM)
/*!< config descriptor size offset*/
#define HID_TOUCH_REPORT_DESC_SIZE_OFFSET   43
#define HID_INT_EP_SIZE_OFFSET              49

#define FINGER_TOUCHSCREEN_REPORT_DESC(logical_max, physical_max, physical_min) \
    0x09, 0x22,                         /*USAGE (Finger) */         \
    0xa1, 0x02,                         /*COLLECTION (Logical) */   \
    0x09, 0x42,                         /*USAGE (Tip Switch)   */   \
    0x15, 0x00,                         /*LOGICAL_MINIMUM (0)  */   \
    0x25, 0x01,                         /*LOGICAL_MAXIMUM (1)  */   \
    0x75, 0x01,                         /*REPORT_SIZE (1)      */   \
    0x95, 0x01,                         /*REPORT_COUNT (1)     */   \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs) */   \
    0x95, 0x07,                         /*REPORT_COUNT (7)     */   \
    0x81, 0x03,                         /*INPUT (Cnst,Ary,Abs) */   \
    0x75, 0x08,                         /*REPORT_SIZE (8)      */   \
    0x09, 0x51,                         /*USAGE (Contact Identifier) */\
    0x95, 0x01,                         /*REPORT_COUNT (1)     */   \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs) */   \
    0x05, 0x01,                         /*USAGE_PAGE (Generic Desk..*/ \
    0x26, (uint8_t)logical_max, (uint8_t)(logical_max >> 8), /*LOGICAL_MAXIMUM (4095) */\
    0x75, 0x10,                         /*REPORT_SIZE (16)    */    \
    0x55, 0x0e,                         /*UNIT_EXPONENT (-2)      */\
    0x65, 0x11,                         /*UNIT(Inch,EngLinear)*/    \
    0x09, 0x30,                         /*USAGE (X)*/               \
    0x35, 0x00,                         /*PHYSICAL_MINIMUM (0)*/    \
    0x46, (uint8_t)physical_max, (uint8_t)(physical_max >> 8), /*PHYSICAL_MAXIMUM (1205)*/\
    0x95, 0x01,                         /*REPORT_COUNT (2) PS: T C devices*/\
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    /*PHYSICAL_MAXIMUM (906)*/\
    0x46, (uint8_t)TOUCH_Y_PHYSICAL_MAXNUM, (uint8_t)(TOUCH_Y_PHYSICAL_MAXNUM >> 8), \
    0x09, 0x31,                         /*USAGE (Y)*/               \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    0x05, 0x0d,                         /*USAGE_PAGE (Digitizers)*/ \
    0x09, 0x48,                         /*USAGE (Width)*/           \
    0x09, 0x49,                         /*USAGE (Height)*/          \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    0x95, 0x01,                         /*REPORT_COUNT (1)  */      \
    0x55, 0x0C,                         /*UNIT_EXPONENT (-4)    */  \
    0x65, 0x12,                         /*UNIT (Radians,SIROtation)*/\
    0x35, 0x00,                         /*PHYSICAL_MINIMUM (0)*/    \
    0x47, 0x6f, 0xf5, 0x00, 0x00,       /*PHYSICAL_MAXIMUM (62831)*/\
    0x15, 0x00,                         /*LOGICAL_MINIMUM (0) */    \
    0x27, 0x6f, 0xf5, 0x00, 0x00,       /*LOGICAL_MAXIMUM (62831)*/ \
    0x09, 0x3f,                         /*USAGE (Azimuth[Orientation])*/\
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    0xc0,                               /*END_COLLECTION */         \

#define FINGER_TOUCHPAD_REPORT_DESC(logical_max, physical_max, physical_min) \
    0x09, 0x22,                         /*USAGE (Finger) */         \
    0xa1, 0x02,                         /*COLLECTION (Logical) */   \
    0x09, 0x47,                         /*USAGE (Confidence)   */   \
    0x09, 0x42,                         /*USAGE (Tip Switch)   */   \
    0x15, 0x00,                         /*LOGICAL_MINIMUM (0)  */   \
    0x25, 0x01,                         /*LOGICAL_MAXIMUM (1)  */   \
    0x75, 0x01,                         /*REPORT_SIZE (1)      */   \
    0x95, 0x02,                         /*REPORT_COUNT (2)     */   \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs) */   \
    0x95, 0x06,                         /*REPORT_COUNT (6)     */   \
    0x81, 0x03,                         /*INPUT (Cnst,Ary,Abs) */   \
    0x75, 0x08,                         /*REPORT_SIZE (8)      */   \
    0x09, 0x51,                         /*USAGE (Contact Identifier) */\
    0x95, 0x01,                         /*REPORT_COUNT (1)     */   \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs) */   \
    0x05, 0x01,                         /*USAGE_PAGE (Generic Desk..*/ \
    0x26, (uint8_t)logical_max, (uint8_t)(logical_max >> 8), /*LOGICAL_MAXIMUM (4095) */\
    0x75, 0x10,                         /*REPORT_SIZE (16)    */    \
    0x55, 0x0e,                         /*UNIT_EXPONENT (-2)      */\
    0x65, 0x11,                         /*UNIT(Inch,EngLinear)*/    \
    0x09, 0x30,                         /*USAGE (X)*/               \
    0x35, 0x00,                         /*PHYSICAL_MINIMUM (0)*/    \
    0x46, (uint8_t)physical_max, (uint8_t)(physical_max >> 8), /*PHYSICAL_MAXIMUM (1205)*/\
    0x95, 0x01,                         /*REPORT_COUNT (2) PS: T C devices*/\
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    /*PHYSICAL_MAXIMUM (906)*/\
    0x46, (uint8_t)TOUCH_Y_PHYSICAL_MAXNUM, (uint8_t)(TOUCH_Y_PHYSICAL_MAXNUM >> 8), \
    0x09, 0x31,                         /*USAGE (Y)*/               \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    0x05, 0x0d,                         /*USAGE_PAGE (Digitizers)*/ \
    0x09, 0x48,                         /*USAGE (Width)*/           \
    0x09, 0x49,                         /*USAGE (Height)*/          \
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    0x95, 0x01,                         /*REPORT_COUNT (1)  */      \
    0x55, 0x0C,                         /*UNIT_EXPONENT (-4)    */  \
    0x65, 0x12,                         /*UNIT (Radians,SIROtation)*/\
    0x35, 0x00,                         /*PHYSICAL_MINIMUM (0)*/    \
    0x47, 0x6f, 0xf5, 0x00, 0x00,       /*PHYSICAL_MAXIMUM (62831)*/\
    0x15, 0x00,                         /*LOGICAL_MINIMUM (0) */    \
    0x27, 0x6f, 0xf5, 0x00, 0x00,       /*LOGICAL_MAXIMUM (62831)*/ \
    0x09, 0x3f,                         /*USAGE (Azimuth[Orientation])*/\
    0x81, 0x02,                         /*INPUT (Data,Var,Abs)*/    \
    0xc0,                               /*END_COLLECTION */         \

/*!<global descriptor */
uint8_t hid_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_DESC_SIZ, 0x01, 0x01,
                                USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /************** Descriptor of Joystick Touch interface ****************/
    /* 09 */
    0x09,                           /* bLength: Interface Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE,  /* bDescriptorType: Interface descriptor type */
    0x00,                           /* bInterfaceNumber: Number of Interface */
    0x00,                           /* bAlternateSetting: Alternate setting */
    0x01,                           /* bNumEndpoints */
    0x03,                           /* bInterfaceClass: HID */
    0x01,                           /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x02,                           /* nInterfaceProtocol : 0=none, 1=keyboard, 2=touch */
    0,                              /* iInterface: Index of string descriptor */
    /******************** Descriptor of Joystick Touch HID ********************/
    /* 18 */
    0x09,                           /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID,        /* bDescriptorType: HID */
    0x11,                           /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                           /* bCountryCode: Hardware target country */
    0x01,                           /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                           /* bDescriptorType */
    (uint8_t)HID_TOUCH_REPORT_DESC_SIZE,     /* wItemLength: Total length of Report descriptor */
    (uint8_t)(HID_TOUCH_REPORT_DESC_SIZE >> 8),
    /******************** Descriptor of Touch endpoint ********************/
    /* 27 */
    0x07,                           /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT,   /* bDescriptorType: */
    HID_INT_EP,                     /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                           /* bmAttributes: Interrupt endpoint */
    HID_INT_EP_SIZE,                /* wMaxPacketSize: 4 Byte max */
    0x00,
    HID_INT_EP_INTERVAL,            /* bInterval: Polling Interval */
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
    // ///////////////////////////////////////
    // /// Other Speed Configuration Descriptor
    // ///////////////////////////////////////
    0x09,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED,
    WBVAL(USB_HID_CONFIG_DESC_SIZ),
    0x01,
    0x01,
    0x00,
    USB_CONFIG_BUS_POWERED,
    USBD_MAX_POWER,
#endif
    0x00
};


#define REPORTID_TOUCHPAD   0x01
#define REPORTID_MAX_COUNT  0x02
#define REPORTID_PTPHQA     0x03
#define REPORTID_FEATURE    0x04
#define REPORTID_FUNCTION_SWITCH     0x05
#define REPORTID_MOUSE      0x06

#ifdef HID_TOUCHPAD_MODE
static const uint8_t hid_touchpad_report_desc[] = {
//TOUCH PAD input TLC
    0x05, 0x0d,                         //  USAGE_PAGE (Digitizers)
    0x09, 0x05,                         //  USAGE (Touch Pad)
    0xa1, 0x01,                         //  COLLECTION (Application)
    0x85, REPORTID_TOUCHPAD,            //  REPORT_ID (Touch pad)
    FINGER_TOUCHPAD_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHPAD_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHPAD_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHPAD_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHPAD_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    0x05, 0x0d,                         //  USAGE_PAGE (Digitizers)
    0x55, 0x0C,                         //  UNIT_EXPONENT (-4)
    0x66, 0x01, 0x10,                   //  UNIT (Seconds)
    0x47, 0xff, 0xff, 0x00, 0x00,       //  PHYSICAL_MAXIMUM (65535)
    0x27, 0xff, 0xff, 0x00, 0x00,       //  LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                         //  REPORT_SIZE (16)
    0x95, 0x01,                         //  REPORT_COUNT (1)
    0x05, 0x0d,                         //  USAGE_PAGE (Digitizers)
    0x09, 0x56,                         //  USAGE (Scan Time)
    0x81, 0x02,                         //  INPUT (Data,Var,Abs)
    0x09, 0x54,                         //  USAGE (Contact count)
    0x25, 0x7f,                         //  LOGICAL_MAXIMUM (127)
    0x95, 0x01,                         //  REPORT_COUNT (1)
    0x75, 0x08,                         //  REPORT_SIZE (8)
    0x81, 0x02,                         //  INPUT (Data,Var,Abs)
    0x05, 0x09,                         //  USAGE_PAGE (Button)
    0x09, 0x01,                         //  USAGE_(Button 1)
    0x09, 0x02,                         //  USAGE_(Button 2)
    0x09, 0x03,                         //  USAGE_(Button 3)
    0x25, 0x01,                         //  LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //  REPORT_SIZE (1)
    0x95, 0x03,                         //  REPORT_COUNT (3)
    0x81, 0x02,                         //  INPUT (Data,Var,Abs)
    0x95, 0x05,                         //  REPORT_COUNT (5)
    0x81, 0x03,                         //  INPUT (Cnst,Var,Abs)
    0x05, 0x0d,                         //  USAGE_PAGE (Digitizer)
    0x85, REPORTID_MAX_COUNT,           //  REPORT_ID (Feature)
    0x09, 0x55,                         //  USAGE (Contact Count Maximum)
    0x09, 0x59,                         //  USAGE (Pad TYpe)
    0x75, 0x04,                         //  REPORT_SIZE (4)
    0x95, 0x02,                         //  REPORT_COUNT (2)
    0x25, 0x0f,                         //  LOGICAL_MAXIMUM (15)
    0xb1, 0x02,                         //  FEATURE (Data,Var,Abs)
    0x06, 0x00, 0xff,                   //  USAGE_PAGE (Vendor Defined)
    0x85, REPORTID_PTPHQA,              //  REPORT_ID (PTPHQA)
    0x09, 0xC5,                         //  SAGE (Vendor Usage 0xC5)
    0x15, 0x00,                         //  OGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,                   //  OGICAL_MAXIMUM (0xff)
    0x75, 0x08,                         //  EPORT_SIZE (8)
    0x96, 0x00, 0x01,                   //  EPORT_COUNT (0x100 (256))
    0xb1, 0x02,                         //  EATURE (Data,Var,Abs)
    0xc0,                               //  ND_COLLECTION
    //CONFIG TLC
    0x05, 0x0d,                         //  SAGE_PAGE (Digitizer)
    0x09, 0x0E,                         //  SAGE (Configuration)
    0xa1, 0x01,                         //  COLLECTION (Application)
    0x85, REPORTID_FEATURE,             //  REPORT_ID (Feature)
    0x09, 0x22,                         //  USAGE (Finger)
    0xa1, 0x02,                         //  COLLECTION (logical)
    0x09, 0x52,                         //  SAGE (Input Mode)
    0x15, 0x00,                         //  OGICAL_MINIMUM (0)
    0x25, 0x0a,                         //  OGICAL_MAXIMUM (10)
    0x75, 0x08,                         //  EPORT_SIZE (8)
    0x95, 0x01,                         //  EPORT_COUNT (1)
    0xb1, 0x02,                         //  EATURE (Data,Var,Abs
    0xc0,                               //  END_COLLECTION
    0x09, 0x22,                         //  USAGE (Finger)
    0xa1, 0x00,                         //  COLLECTION (physical)
    0x85, REPORTID_FUNCTION_SWITCH,     //  REPORT_ID (Feature)
    0x09, 0x57,                         //  USAGE(Surface switch)
    0x09, 0x58,                         //  USAGE(Button switch)
    0x75, 0x01,                         //  REPORT_SIZE (1)
    0x95, 0x02,                         //  REPORT_COUNT (2)
    0x25, 0x01,                         //  LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                         //  FEATURE (Data,Var,Abs)
    0x95, 0x06,                         //  REPORT_COUNT (6)
    0xb1, 0x03,                         //  FEATURE (Cnst,Var,Abs)
    0xc0,                               //  END_COLLECTION
    0xc0,                               // END_COLLECTION
    //MOUSE TLC
    0x05, 0x01,                         //  USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                         //  USAGE (Mouse)
    0xa1, 0x01,                         //  COLLECTION (Application)
    0x85, REPORTID_MOUSE,               //  REPORT_ID (Mouse)
    0x09, 0x01,                         //  USAGE (Pointer)
    0xa1, 0x00,                         //  COLLECTION (Physical)
    0x05, 0x09,                         //  USAGE_PAGE (Button)
    0x19, 0x01,                         //  USAGE_MINIMUM (Button 1)
    0x29, 0x02,                         //  USAGE_MAXIMUM (Button 2)
    0x25, 0x01,                         //  LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //  REPORT_SIZE (1)
    0x95, 0x02,                         //  REPORT_COUNT (2)
    0x81, 0x02,                         //  INPUT (Data,Var,Abs)
    0x95, 0x06,                         //  REPORT_COUNT (6)
    0x81, 0x03,                         //  INPUT (Cnst,Var,Abs)
    0x05, 0x01,                         //  USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         //  USAGE (X)
    0x09, 0x31,                         //  USAGE (Y)
    0x75, 0x10,                         //  REPORT_SIZE (16)
    0x95, 0x02,                         //  REPORT_COUNT (2)
    0x25, 0x0a,                         //  OGICAL_MAXIMUM (10)
    0x81, 0x06,                         //  INPUT (Data,Var,Rel)
    0xc0,                               //  END_COLLECTION
    0xc0,                               //  END_COLLECTION

};
#endif
static uint8_t touchpad_dev_certification[] = {
    0xfc, 0x28, 0xfe, 0x84, 0x40, 0xcb, 0x9a, 0x87, 0x0d, 0xbe, 0x57, 0x3c, 0xb6, 0x70, 0x09, 0x88, 0x07,
    0x97, 0x2d, 0x2b, 0xe3, 0x38, 0x34, 0xb6, 0x6c, 0xed, 0xb0, 0xf7, 0xe5, 0x9c, 0xf6, 0xc2, 0x2e, 0x84,
    0x1b, 0xe8, 0xb4, 0x51, 0x78, 0x43, 0x1f, 0x28, 0x4b, 0x7c, 0x2d, 0x53, 0xaf, 0xfc, 0x47, 0x70, 0x1b,
    0x59, 0x6f, 0x74, 0x43, 0xc4, 0xf3, 0x47, 0x18, 0x53, 0x1a, 0xa2, 0xa1, 0x71, 0xc7, 0x95, 0x0e, 0x31,
    0x55, 0x21, 0xd3, 0xb5, 0x1e, 0xe9, 0x0c, 0xba, 0xec, 0xb8, 0x89, 0x19, 0x3e, 0xb3, 0xaf, 0x75, 0x81,
    0x9d, 0x53, 0xb9, 0x41, 0x57, 0xf4, 0x6d, 0x39, 0x25, 0x29, 0x7c, 0x87, 0xd9, 0xb4, 0x98, 0x45, 0x7d,
    0xa7, 0x26, 0x9c, 0x65, 0x3b, 0x85, 0x68, 0x89, 0xd7, 0x3b, 0xbd, 0xff, 0x14, 0x67, 0xf2, 0x2b, 0xf0,
    0x2a, 0x41, 0x54, 0xf0, 0xfd, 0x2c, 0x66, 0x7c, 0xf8, 0xc0, 0x8f, 0x33, 0x13, 0x03, 0xf1, 0xd3, 0xc1, 0x0b,
    0x89, 0xd9, 0x1b, 0x62, 0xcd, 0x51, 0xb7, 0x80, 0xb8, 0xaf, 0x3a, 0x10, 0xc1, 0x8a, 0x5b, 0xe8, 0x8a,
    0x56, 0xf0, 0x8c, 0xaa, 0xfa, 0x35, 0xe9, 0x42, 0xc4, 0xd8, 0x55, 0xc3, 0x38, 0xcc, 0x2b, 0x53, 0x5c,
    0x69, 0x52, 0xd5, 0xc8, 0x73, 0x02, 0x38, 0x7c, 0x73, 0xb6, 0x41, 0xe7, 0xff, 0x05, 0xd8, 0x2b, 0x79,
    0x9a, 0xe2, 0x34, 0x60, 0x8f, 0xa3, 0x32, 0x1f, 0x09, 0x78, 0x62, 0xbc, 0x80, 0xe3, 0x0f, 0xbd, 0x65,
    0x20, 0x08, 0x13, 0xc1, 0xe2, 0xee, 0x53, 0x2d, 0x86, 0x7e, 0xa7, 0x5a, 0xc5, 0xd3, 0x7d, 0x98, 0xbe,
    0x31, 0x48, 0x1f, 0xfb, 0xda, 0xaf, 0xa2, 0xa8, 0x6a, 0x89, 0xd6, 0xbf, 0xf2, 0xd3, 0x32, 0x2a, 0x9a,
    0xe4, 0xcf, 0x17, 0xb7, 0xb8, 0xf4, 0xe1, 0x33, 0x08, 0x24, 0x8b, 0xc4, 0x43, 0xa5, 0xe5, 0x24, 0xc2
};

#ifdef HID_TOUCHSCREEN_MODE
static const uint8_t hid_touchscreen_report_desc[] = {
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x09, 0x04,                         // USAGE (Touch Screen)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, 0x01,                         // REPORT_ID (Touch)
    FINGER_TOUCHSCREEN_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHSCREEN_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHSCREEN_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHSCREEN_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    FINGER_TOUCHSCREEN_REPORT_DESC(TOUCH_LOGICAL_MAXNUM, TOUCH_X_PHYSICAL_MAXNUM, TOUCH_Y_PHYSICAL_MAXNUM)
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x55, 0x0C,                         // UNIT_EXPONENT (-4)
    0x66, 0x01, 0x10,                   // UNIT (Seconds)
    0x47, 0xff, 0xff, 0x00, 0x00,       // PHYSICAL_MAXIMUM (65535)
    0x27, 0xff, 0xff, 0x00, 0x00,       // LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x09, 0x56,                         // USAGE (Scan Time)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x09, 0x54,                         // USAGE (Contact count)
    0x25, 0x7f,                         // LOGICAL_MAXIMUM (127)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x75, 0x08,                         // REPORT_SIZE (8)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x85, REPORTID_MAX_COUNT,           // REPORT_ID (Feature)
    0x09, 0x55,                         // USAGE(Contact Count Maximum)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x25, 0x02,                         // LOGICAL_MAXIMUM (2)
    0xb1, 0x02,                         // FEATURE (Data,Var,Abs)
    0x85, 0x44,                         // REPORT_ID (Feature)
    0x06, 0x00, 0xff,                   // USAGE_PAGE (Vendor Defined)
    0x09, 0xC5,                         // USAGE (Vendor Usage 0xC5)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,                   // LOGICAL_MAXIMUM (0xff)
    0x75, 0x08,                         // REPORT_SIZE (8)
    0x96, 0x00,  0x01,                  // REPORT_COUNT (0x100 (256))
    0xb1, 0x02,                         // FEATURE (Data,Var,Abs)
    0xc0,                               // END_COLLECTION
};

#endif

enum touch_mode {
    TOUCH_SCREEN = 0,   /* support for win10、win11 */
    TOUCH_PAD = 1,      /* support for win10、win11 */
};

struct hid_touch_cfg_t {  // 10 bytes
    uint8_t tip;
    uint8_t id;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t azimuth;
};

struct hid_touchscreen_report_t { // 4 + 10 * TOUCH_MAX_POINT_NUM
    uint8_t report_id;
    struct hid_touch_cfg_t touch[TOUCH_MAX_POINT_NUM];
    uint16_t scan_time;
    uint8_t count;
}__attribute__((packed));

struct hid_touchpad_report_t { // 5 + 10 * TOUCH_MAX_POINT_NUM
    uint8_t report_id;
    struct hid_touch_cfg_t touch[TOUCH_MAX_POINT_NUM];
    uint16_t scan_time;
    uint8_t count;
    uint8_t buttons1 : 1;
    uint8_t buttons2 : 1;
    uint8_t buttons3 : 1;
}__attribute__((packed));

struct hid_touch_t {
    rt_device_t dev;
    rt_sem_t sem;
    enum touch_mode mode;

    u64 base_time;
    uint8_t last_event;
    uint8_t id[TOUCH_MAX_POINT_NUM + 1];
    struct hid_touch_cfg_t finger[TOUCH_MAX_POINT_NUM];
    volatile uint8_t cur_max_point_num;
    volatile uint8_t cur_point_num;
    volatile uint8_t hid_state;

    /* hardware param */
    uint16_t hid_touch_rotate;
    struct rt_touch_data *data;
    struct rt_touch_info info;

    /* hid param */
    const uint8_t *cur_desc;
    uint8_t *touch_report;
    uint16_t int_ep_size;
    uint16_t hid_desc_len;


    // test_action_settings
    // instruct_action_settings

} g_hid_touch;

struct usbd_interface touch_intf;
static rt_thread_t  hid_touch_tid;
/*!< touch g_touch_report */
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_report[GLOBAL_REPORT_SIZE];

#define HID_STATE_IDLE 0
#define HID_STATE_BUSY 1

void usbd_hid_get_report(uint8_t intf, uint8_t report_id, uint8_t report_type,
                            uint8_t **data, uint32_t *len)
{
    USB_LOG_DBG("Get report : id %d\n", report_id);
    switch (report_id) {
        case REPORTID_MAX_COUNT:
            (*data)[0] = REPORTID_MAX_COUNT;
            (*data)[1] = TOUCH_MAX_POINT_NUM;
            *len = 2;
            break;
        case REPORTID_PTPHQA:
            (*data)[0] = REPORTID_PTPHQA; //report id
            memcpy(&((*data)[1]), touchpad_dev_certification,
                    sizeof(touchpad_dev_certification));
            *len = sizeof(touchpad_dev_certification) + 1;
            break;
    }
}

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
void usbd_comp_touch_event_handler(uint8_t event)
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

/* function ------------------------------------------------------------------*/
static struct hid_touch_t *get_hid_touch(void)
{
    return &g_hid_touch;
}

void usbd_hid_set_idle(uint8_t intf, uint8_t report_id, uint8_t duration)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    hid_touch->hid_state = HID_STATE_IDLE;
}

static void usbd_hid_int_callback(uint8_t ep, uint32_t nbytes)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    hid_touch->hid_state = HID_STATE_IDLE;
}

/*!< endpoint call back */
static struct usbd_endpoint hid_in_ep = {
    .ep_cb = usbd_hid_int_callback,
    .ep_addr = HID_INT_EP
};

void hid_touch_hw_info(void)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    USB_LOG_INFO("HID-TOUCH: info: %d %d %d %d %d\n",
                    (uint8_t)hid_touch->info.type,
                    (uint8_t)hid_touch->info.vendor,
                    (uint8_t)hid_touch->info.point_num,
                    (uint16_t)hid_touch->info.range_x,
                    (uint16_t)hid_touch->info.range_y);
}

void hid_touch_hw_data_dump(uint8_t id)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    USB_LOG_INFO("HID-TOUCH: hw: %d %d %d %d %d\n",
                    (uint8_t)hid_touch->data[id].track_id,
                    (uint16_t)hid_touch->data[id].x_coordinate,
                    (uint16_t)hid_touch->data[id].y_coordinate,
                    (uint16_t)hid_touch->data[id].timestamp,
                    (uint16_t)hid_touch->data[id].width);
}

void hid_touch_sw_data_dump(uint8_t id)
{
    struct hid_touchscreen_report_t *touch_report = NULL;
    touch_report = (struct hid_touchscreen_report_t *)g_report;

    USB_LOG_INFO("HID-TOUCH: sw: %d %d %d %d %d\n",
                    (uint8_t)touch_report->touch[id].tip,
                    (uint8_t)touch_report->touch[id].id,
                    (uint16_t)touch_report->touch[id].x,
                    (uint16_t)touch_report->touch[id].y,
                    (uint16_t)touch_report->touch[id].width);
}

static int _touch_param_transform(uint8_t i)
{
    uint16_t temp = 0;
    uint32_t x = 0, y = 0;
    struct hid_touch_t *hid_touch = get_hid_touch();
    struct hid_touchscreen_report_t *touch_report = NULL;
    touch_report = (struct hid_touchscreen_report_t *)g_report;

    // 1. touch screen rotation
    if ((hid_touch->hid_touch_rotate == 90) ||
        (hid_touch->hid_touch_rotate == 270)) {
        temp = hid_touch->finger[i].y;
        hid_touch->finger[i].y = hid_touch->finger[i].x;
        hid_touch->finger[i].x = temp;
    }

    if (hid_touch->finger[i].x > TOUCH_X_PHYSICAL_MAXNUM ||
        hid_touch->finger[i].y > TOUCH_Y_PHYSICAL_MAXNUM)
        USB_LOG_WRN("WRN 1: The hardware x/y coordinates do not match the software scaling x/y reference values.\n"
                    "You can try rotating 90 degrees or 270 degrees.\n");

    // 2. resolution scaling
    x = hid_touch->finger[i].x;
    y = hid_touch->finger[i].y;
    x = x * TOUCH_LOGICAL_MAXNUM / TOUCH_X_PHYSICAL_MAXNUM;
    y = y * TOUCH_LOGICAL_MAXNUM / TOUCH_Y_PHYSICAL_MAXNUM;
    hid_touch->finger[i].x = (uint16_t)x;
    hid_touch->finger[i].y = (uint16_t)y;

    if (hid_touch->finger[i].x > TOUCH_LOGICAL_MAXNUM ||
        hid_touch->finger[i].y > TOUCH_LOGICAL_MAXNUM)
        USB_LOG_WRN("WRN 2: The x/y values have been scaled incorrectly.\n"
                    "You can try swapping the x/y logical resolution.\n");

    // 3. flip
    #ifdef HID_TOUCH_X_FLIP
    hid_touch->finger[i].x = TOUCH_LOGICAL_MAXNUM - hid_touch->finger[i].x;
    #endif
    #ifdef HID_TOUCH_Y_FLIP
    hid_touch->finger[i].y = TOUCH_LOGICAL_MAXNUM - hid_touch->finger[i].y;
    #endif

    if (touch_report->touch[i].x < 0 || touch_report->touch[i].y < 0)
        USB_LOG_WRN("WRN 3: The x/y coordinates flip set are incorret.\n"
                    "You can try filpping the x/y coordinates.\n");

    #ifdef HID_TOUCH_DEBUG
    hid_touch_sw_data_dump(i);
    hid_touch_hw_data_dump(i);
    #endif
    return 0;
}

uint8_t hid_touch_fingers_num_update(struct rt_touch_data *data)
{
    uint8_t finger_cnt = 0;

    if (data->track_id == 0 && data->event != RT_TOUCH_EVENT_NONE)
        finger_cnt++;

    for (int i = 0; i < TOUCH_MAX_POINT_NUM; i++) {
        USB_LOG_DBG("finers id[%d] : %d\n", i, data->track_id);

        if (data->track_id != 0)
            finger_cnt++;

        data++;
    }

    USB_LOG_DBG("finger_cnt: %d\n", finger_cnt);

    return finger_cnt;
}

uint8_t _finger_alloc(uint8_t id)
{
    uint8_t ret;
    struct hid_touch_t *hid_touch = get_hid_touch();

    ret = hid_touch->cur_point_num;
    hid_touch->id[hid_touch->cur_point_num] = id;
    hid_touch->cur_point_num++;

    return ret;
}

uint8_t _finger_get(uint8_t id)
{
    struct hid_touch_t *hid_touch = get_hid_touch();

    for (int i = 0; i < hid_touch->cur_point_num; i++) {
        if (hid_touch->id[i] == id) {
            return i;
        }
    }

    return id;
}

uint8_t _finger_release(uint8_t id)
{
    struct hid_touch_t *hid_touch = get_hid_touch();

    hid_touch->cur_point_num--;
    for (int i = 0; i < hid_touch->cur_point_num; i++) {
        if (hid_touch->id[i] != id)
            continue;
        for (int j = i; j < hid_touch->cur_point_num; j++) {
            hid_touch->id[j] = hid_touch->id[j + 1];
        }
        break;
    }

    return 0;
}

static int _touch_down(uint8_t i)
{
    uint8_t index;
    struct hid_touch_t *hid_touch = get_hid_touch();

    if (hid_touch->last_event == RT_TOUCH_EVENT_DOWN ||
        hid_touch->last_event == RT_TOUCH_EVENT_MOVE)
        rt_thread_mdelay(30);

    index = _finger_alloc(hid_touch->data[i].track_id);

    hid_touch->finger[index].tip = 0;
    hid_touch->finger[index].id = hid_touch->data[i].track_id;
    hid_touch->finger[index].x = hid_touch->data[i].x_coordinate;
    hid_touch->finger[index].y = hid_touch->data[i].y_coordinate;
    hid_touch->finger[index].width = hid_touch->data[i].width;

    _touch_param_transform(index);

    return 0;
}

static int _touch_move(uint8_t i)
{
    uint8_t index;
    struct hid_touch_t *hid_touch = get_hid_touch();

    if (hid_touch->last_event == RT_TOUCH_EVENT_DOWN)
        rt_thread_mdelay(30);

    index = _finger_get(hid_touch->data[i].track_id);

    hid_touch->finger[index].tip = 3;
    hid_touch->finger[index].id = hid_touch->data[i].track_id;
    hid_touch->finger[index].x = hid_touch->data[i].x_coordinate;
    hid_touch->finger[index].y = hid_touch->data[i].y_coordinate;
    hid_touch->finger[index].width = hid_touch->data[i].width;

    _touch_param_transform(index);

    return 0;
}

static int _touch_up(uint8_t i)
{
    struct hid_touch_t *hid_touch = get_hid_touch();

    _finger_release(hid_touch->data[i].track_id);

    return 0;
}

 uint16_t _touch_get_scan_time(uint8_t ctrl)
{
    uint16_t scan_time;
    u64 time_us;
    struct hid_touch_t *hid_touch = get_hid_touch();

    time_us = aic_get_time_us();

    time_us = time_us - hid_touch->base_time;
    scan_time = (uint16_t)(time_us / 100);  /* 100us units */

    USB_LOG_DBG("base_time:%lld, scan_time:%d .\n",
                    hid_touch->base_time, scan_time);

    return scan_time;
}

static uint8_t *hid_touchscreen_data(void)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    struct hid_touchscreen_report_t *touchscreen = NULL;
    touchscreen = (struct hid_touchscreen_report_t *)g_report;

    memset(g_report, 0, GLOBAL_REPORT_SIZE);

    touchscreen->report_id = 0x01;
    // touchscreen->scan_time = _touch_get_scan_time(0);
    touchscreen->scan_time = hid_touch->data->timestamp;

    if (hid_touch->cur_point_num > hid_touch->cur_max_point_num)
        hid_touch->cur_point_num = hid_touch->cur_max_point_num;

    touchscreen->count = hid_touch->cur_point_num;

    for (int i = 0; i < hid_touch->cur_point_num; i++)
        memcpy(&touchscreen->touch[i], &hid_touch->finger[i],
                sizeof(struct hid_touch_cfg_t));

    return (uint8_t *)touchscreen;
}

static uint8_t *hid_touchpad_data(void)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    struct hid_touchpad_report_t *touchpad_report = NULL;
    touchpad_report = (struct hid_touchpad_report_t *)g_report;

    memset(g_report, 0, GLOBAL_REPORT_SIZE);

    touchpad_report->report_id = 0x01;
    // touchpad_report->scan_time = _touch_get_scan_time(0);
    touchpad_report->scan_time = 0xf;
    touchpad_report->count = hid_touch->cur_point_num;

    for (int i = 0; i < hid_touch->cur_point_num; i++)
        memcpy(&touchpad_report->touch[i], &hid_touch->finger[i],
                sizeof(struct hid_touch_cfg_t));

    touchpad_report->buttons1 = 0;  //todo 1bit
    touchpad_report->buttons2 = 0;  //todo 1bit
    touchpad_report->buttons3 = 0;  //todo 1bit

    return (uint8_t *)touchpad_report;

}

static void hid_touch_send_data(void)
{
    int ret = 0;
    uint8_t *report;
    struct hid_touch_t *hid_touch = get_hid_touch();

    switch (hid_touch->mode) {
        case TOUCH_SCREEN:
            report = hid_touchscreen_data();
            break;
        case TOUCH_PAD:
            report = hid_touchpad_data();
            break;
        default:
            goto __err;
    }

    if (report == NULL)
        goto __err;

    if (hid_touch->hid_state == HID_STATE_IDLE) {
        hid_touch->hid_state = HID_STATE_BUSY;
        ret = usbd_ep_start_write(hid_in_ep.ep_addr, report,
                                    hid_touch->int_ep_size);
        #ifdef HID_TOUCH_DEBUG
        USB_LOG_RAW("HID-TOUCH: Current effective finger count: %#lx \n",
                (long)hid_touch->cur_point_num);
        #endif

        if (ret < 0)
            return;
    }

    if (hid_touch->cur_point_num == 0)
        hid_touch->base_time = aic_get_time_us();

    return;

__err:
    USB_LOG_ERR("HID-TOUCH: No haraware data !\n");
    return;
}

static int hid_touch_event_processing(uint8_t index, uint8_t event)
{
    switch(event) {
        case RT_TOUCH_EVENT_MOVE:
            USB_LOG_DBG("HID-TOUCH: RT_TOUCH_EVENT_MOVE\n");
            _touch_move(index);
            break;
        case RT_TOUCH_EVENT_DOWN:
            USB_LOG_DBG("HID-TOUCH: RT_TOUCH_EVENT_DOWN\n");
            _touch_down(index);
            break;
        case RT_TOUCH_EVENT_UP:
            USB_LOG_DBG("HID-TOUCH: RT_TOUCH_EVENT_UP\n");
            _touch_up(index);
            break;
        default:
            return -1;
    }

    return 0;
}

static void hid_touch_thread(void *parameter)
{
    int ret;
    struct hid_touch_t *hid_touch = get_hid_touch();

    hid_touch->data = (struct rt_touch_data *)rt_malloc(
                        sizeof(struct rt_touch_data) *
                        hid_touch->info.point_num);

    while(1) {
        rt_sem_take(hid_touch->sem, RT_WAITING_FOREVER);
        if (rt_device_read(hid_touch->dev, 0, hid_touch->data, hid_touch->info.point_num)
                            == hid_touch->info.point_num) {

            hid_touch->cur_max_point_num = hid_touch_fingers_num_update(hid_touch->data);

            for (rt_uint8_t i = 0; i < TOUCH_MAX_POINT_NUM; i++) {
                if (i > hid_touch->info.point_num)
                    break;

                if (hid_touch->mode == TOUCH_SCREEN || TOUCH_PAD)
                    ret = hid_touch_event_processing(i, hid_touch->data[i].event);

                if (ret < 0)
                    continue;

                hid_touch->last_event = hid_touch->data[i].event;
            }
            hid_touch_send_data();
        }
        rt_device_control(hid_touch->dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
    }
}

static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    rt_device_control(hid_touch->dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    rt_sem_release(hid_touch->sem);
    return 0;
}

static int hid_touch_hw_init(char *dev_name)
{
    int ret = 0;
    uint8_t id[10];
    struct hid_touch_t *hid_touch = get_hid_touch();

    hid_touch->dev = rt_device_find(dev_name);
    if (hid_touch->dev == RT_NULL) {
        USB_LOG_ERR("HID-TOUCH: can't find device:%s\n", (char *)hid_touch->dev);
        return -1;
    }

    rt_device_close(hid_touch->dev);
#ifdef HID_TOUCH_INT_MODE
    ret = rt_device_open(hid_touch->dev, RT_DEVICE_FLAG_INT_RX);
#else
    ret = rt_device_open(hid_touch->dev, RT_DEVICE_FLAG_RDONLY);
#endif
    if (ret != RT_EOK) {
        USB_LOG_ERR("HID-TOUCH: open device failed!");
        return -1;
    }

    rt_device_control(hid_touch->dev, RT_TOUCH_CTRL_GET_ID, id);
    rt_device_control(hid_touch->dev, RT_TOUCH_CTRL_GET_INFO, &hid_touch->info);
    hid_touch_hw_info();

    rt_device_set_rx_indicate(hid_touch->dev, rx_callback);
    hid_touch->sem = rt_sem_create("hid_touch_sem", 0, RT_IPC_FLAG_FIFO);
    if (hid_touch->sem == RT_NULL) {
        USB_LOG_ERR("HID-TOUCH: create dynamic semaphore failed.\n");
        return -1;
    }

    hid_touch_tid = rt_thread_create("hid_touch", hid_touch_thread,
                                     RT_NULL, 1024 * 4,
                                     RT_THREAD_PRIORITY_MAX - 2, 20);

    if (hid_touch_tid != RT_NULL)
        rt_thread_startup(hid_touch_tid);

    return 0;
}

static int _set_desc_report_size(uint16_t size)
{
    hid_descriptor[HID_TOUCH_REPORT_DESC_SIZE_OFFSET] = (uint8_t)(size);
    hid_descriptor[HID_TOUCH_REPORT_DESC_SIZE_OFFSET + 1] = (uint8_t)(size >> 8);

    return 0;
}

static int _set_desc_intep_size(uint16_t size)
{
    hid_descriptor[HID_INT_EP_SIZE_OFFSET] = size;

    return 0;
}

static int hid_touch_mode_switch(void)
{
    struct hid_touch_t *hid_touch = get_hid_touch();

#ifdef HID_TOUCHPAD_MODE
    hid_touch->mode = TOUCH_PAD;
    hid_touch->cur_desc = hid_touchpad_report_desc;
    hid_touch->int_ep_size = TOUCHPAD_PACKET_SIZE;
    hid_touch->hid_desc_len = HID_TOUCHPAD_REPORT_DESC_SIZE;
#endif
#ifdef HID_TOUCHSCREEN_MODE
    hid_touch->mode = TOUCH_SCREEN;
    hid_touch->cur_desc = hid_touchscreen_report_desc;
    hid_touch->int_ep_size = TOUCHSCREEN_PACKET_SIZE;
    hid_touch->hid_desc_len = HID_TOUCHSCREEN_REPORT_DESC_SIZE;
#endif

    _set_desc_intep_size(hid_touch->int_ep_size);
    _set_desc_report_size(hid_touch->hid_desc_len);

#ifdef HID_TOUCH_DEBUG
    USB_LOG_INFO("HID-touch: report size :%d \n", hid_touch->hid_desc_len);
    USB_LOG_INFO("HID-touch: INT ep size :%d \n", hid_touch->int_ep_size);
#endif
    return 0;
}

#ifdef LPKG_CHERRYUSB_DEVICE_COMPOSITE
#include "composite_template.h"
int usbd_comp_touch_init(uint8_t *ep_table, void *data)
{
    struct hid_touch_t *hid_touch = get_hid_touch();
    hid_in_ep.ep_addr = ep_table[0];

    usbd_add_interface(usbd_hid_init_intf(&touch_intf,
                                            hid_touch->cur_desc,
                                            hid_touch->hid_desc_len));
    usbd_add_endpoint(&hid_in_ep);
    return 0;
}
#endif

static void hid_touch_init(void)
{
    /*!< init touch g_touch_report data */
    struct hid_touch_t *hid_touch = get_hid_touch();

    memset(hid_touch, 0, sizeof(struct hid_touch_t));
    hid_touch->hid_touch_rotate = AIC_HID_ROTATE_DEGREE;
    hid_touch_hw_init(HID_TOUCH_NAME);
    hid_touch_mode_switch();

#ifndef LPKG_CHERRYUSB_DEVICE_COMPOSITE
    usbd_desc_register(hid_descriptor);
    usbd_add_interface(usbd_hid_init_intf(&touch_intf,
                                            hid_touch->cur_desc,
                                            hid_touch->hid_desc_len));
    usbd_add_endpoint(&hid_in_ep);
    usbd_initialize();
#else
    usbd_comp_func_register(hid_descriptor,
                            usbd_comp_touch_event_handler,
                            usbd_comp_touch_init, "touch");
#endif
}

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdevice.h>

int usbd_hid_touch_init(void)
{
    hid_touch_init();
    return 0;
}
#if !defined(LPKG_CHERRYUSB_DYNAMIC_REGISTRATION_MODE)
INIT_APP_EXPORT(usbd_hid_touch_init);
#endif

#endif
