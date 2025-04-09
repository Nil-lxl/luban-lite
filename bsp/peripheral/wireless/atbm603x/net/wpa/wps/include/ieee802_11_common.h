/*
 * IEEE 802.11 Common routines
 * Copyright (c) 2002-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef IEEE802_11_COMMON_H
#define IEEE802_11_COMMON_H


typedef enum { ParseOK = 0, ParseUnknown = 1, ParseFailed = -1 } ParseRes;

enum atbm_hostapd_hw_mode ieee80211_freq_to_chan(atbm_int32 freq, atbm_uint8 *channel);
int ieee80211_chan_to_freq(const char *country, atbm_uint8 op_class, atbm_uint8 chan);
int ieee80211_is_dfs(int freq);
int is_11b(atbm_uint8 rate);


const char * fc2str(atbm_uint16 fc);
#endif /* IEEE802_11_COMMON_H */
