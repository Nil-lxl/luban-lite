/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TOUCH_H__
#define __TOUCH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>

#define TOUCH_OK                                0
#define TOUCH_ERROR                             -1

#define PIN_OUTPUT_MODE                         0x00
#define PIN_INPUT_MODE                          0x01
#define PIN_INPUT_PULLUP_MODE                   0x02
#define PIN_INPUT_PULLDOWN_MODE                 0x03
#define PIN_OUTPUT_OD_MODE                      0x04

#define IRQ_PIN_MODE_RISING                     0x00
#define IRQ_PIN_MODE_FALLING                    0x01
#define IRQ_PIN_MODE_RISING_FALLING             0x02
#define IRQ_PIN_MODE_HIGH_LEVEL                 0x03
#define IRQ_PIN_MODE_LOW_LEVEL                  0x04

#define IRQ_PIN_DISABLE                         0x0
#define IRQ_PIN_ENABLE                          0x1

#define IRQ_PIN_NONE                            -1

#define TOUCH_EVENT_NONE                        (0)   /* Touch none */
#define TOUCH_EVENT_UP                          (1)   /* Touch up event */
#define TOUCH_EVENT_DOWN                        (2)   /* Touch down event */
#define TOUCH_EVENT_MOVE                        (3)   /* Touch move event */

/* Touch control cmd types */
#define TOUCH_CTRL_GET_ID                       0
#define TOUCH_CTRL_GET_INFO                     1

#define LOW_POWER                               0x00
#define HIGH_POWER                              0x01

/* Touch ic type */
#define TOUCH_TYPE_NODE                         0
#define TOUCH_TYPE_CAPACITANCE                  1
#define TOUCH_TYPE_RESISTANCE                   2

/* Touch vendor types */
#define TOUCH_VENDOR_UNKNOWN                    0
#define TOUCH_VENDOR_GT                         1
#define TOUCH_VENDOR_FT                         2
#define TOUCH_VENDOR_ST                         3
#define TOUCH_VENDOR_CST                        4

struct touch_info {
    uint8_t         type;
    uint8_t         vendor;
    uint8_t         point_num;
    uint8_t         range_x;
    uint8_t         range_y;
};

struct touch {
    char *name;
    struct touch_ops *ops;
    struct touch_info info;
};

struct touch_data {
    uint8_t          event;                 /* The touch event of the data */
    uint8_t          track_id;              /* Track id of point */
    uint8_t          width;                 /* Point of width */
    uint16_t         x_coordinate;          /* Point of x coordinate */
    uint16_t         y_coordinate;          /* Point of y coordinate */
};

struct touch_ops {
    int     (*control)(struct touch *touch, int cmd, void *arg);
    int     (*readpoint)(struct touch *touch, void *buf, unsigned int len);
};

int touch_control(struct touch *touch, int cmd, void *arg);
int touch_readpoint(struct touch *touch, void *buf, unsigned int len);
int touch_get_i2c_chan_id(const char *name);
long touch_pin_get(const char *name);
void touch_set_pin_mode(long pin, long mode);
void touch_pin_write(long pin, long value);
void touch_register(struct touch *touch);
void touch_unregister(void);

#endif
