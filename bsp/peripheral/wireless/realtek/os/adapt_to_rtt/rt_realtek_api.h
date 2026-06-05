/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RT_REALTEK_API_H__
#define __RT_REALTEK_API_H__

extern void wlan_recv(int idx, unsigned int len);
extern void rtw_init_macaddr(char *mac);
extern void rtw_wlan_get_mac(u8 *mac);

#if CONFIG_ENABLE_P2P
void rtw_wlan_p2p_go_started(void);
void rtw_wlan_p2p_go_stopped(void);
#endif

#endif
