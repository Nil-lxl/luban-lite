/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  zequan liang <zequan liang@artinchip.com>
 */

int simple_serial_init(char *uart, int boaud, int data_bits, int stop_bits, int parity);
int simple_serial_send(unsigned char *data, int len);
int simple_serial_recv(unsigned char *data, int max_len);
