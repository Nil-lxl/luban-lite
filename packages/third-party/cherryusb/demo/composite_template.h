/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  senye Liang <senye.liang@artinchip.com>
 */
#ifndef __COMPOSITE_TEMPLATE_H_
#define __COMPOSITE_TEMPLATE_H_



void usbd_comp_func_register(const uint8_t *desc,
                                void (*event_handler)(uint8_t event),
                                int (*usbd_comp_class_init)(uint8_t *ep_table, void *data),
                                void *data);
extern void usbd_comp_func_release(const uint8_t *desc, void *data);
extern bool usbd_composite_is_inited(void);
extern uint8_t usbd_compsite_set_dev_num(uint8_t num);
extern uint8_t usbd_compsite_get_dev_num(void);
extern uint8_t usbd_composite_func_switch(uint8_t index);
#endif
