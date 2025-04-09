/*
 * IEEE 802.11 Common routines
 * Copyright (c) 2002-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "atbm_hal.h"

 enum atbm_hostapd_hw_mode ieee80211_freq_to_chan(atbm_int32 freq, atbm_uint8 *channel)
{
	enum atbm_hostapd_hw_mode mode = ATBM_NUM_HOSTAPD_MODES;

	if (freq >= 2412 && freq <= 2472) {
		mode = ATBM_HOSTAPD_MODE_IEEE80211G;
		*channel = (freq - 2407) / 5;
	} else if (freq == 2484) {
		mode = ATBM_HOSTAPD_MODE_IEEE80211B;
		*channel = 14;
	} else if (freq >= 4900 && freq < 5000) {
		mode = ATBM_HOSTAPD_MODE_IEEE80211A;
		*channel = (freq - 4000) / 5;
	} else if (freq >= 5000 && freq < 5900) {
		mode = ATBM_HOSTAPD_MODE_IEEE80211A;
		*channel = (freq - 5000) / 5;
	}

	return mode;
}


static const char *us_op_class_cc[] = {
	"US", "CA", NULL
};

static const char *eu_op_class_cc[] = {
	"AL", "AM", "AT", "AZ", "BA", "BE", "BG", "BY", "CH", "CY", "CZ", "DE",
	"DK", "EE", "EL", "ES", "FI", "FR", "GE", "HR", "HU", "IE", "IS", "IT",
	"LI", "LT", "LU", "LV", "MD", "ME", "MK", "MT", "NL", "NO", "PL", "PT",
	"RO", "RS", "RU", "SE", "SI", "SK", "TR", "UA", "UK", NULL
};

static const char *jp_op_class_cc[] = {
	"JP", NULL
};

static const char *cn_op_class_cc[] = {
	"CN", "CA", NULL
};


 static atbm_int32 country_match(const char *cc[], const char *country)
{
	atbm_int32 i;

	if (country == NULL)
		return 0;
	for (i = 0; cc[i]; i++) {
		if (cc[i][0] == country[0] && cc[i][1] == country[1])
			return 1;
	}

	return 0;
}


 static atbm_int32 ieee80211_chan_to_freq_us(atbm_uint8 op_class, atbm_uint8 chan)
{
	switch (op_class) {
	case 12: /* channels 1..11 */
	case 32: /* channels 1..7; 40 MHz */
	case 33: /* channels 5..11; 40 MHz */
		if (chan < 1 || chan > 11)
			return -1;
		return 2407 + 5 * chan;
	case 1: /* channels 36,40,44,48 */
	case 2: /* channels 52,56,60,64; dfs */
	case 22: /* channels 36,44; 40 MHz */
	case 23: /* channels 52,60; 40 MHz */
	case 27: /* channels 40,48; 40 MHz */
	case 28: /* channels 56,64; 40 MHz */
		if (chan < 36 || chan > 64)
			return -1;
		return 5000 + 5 * chan;
	case 4: /* channels 100-144 */
	case 24: /* channels 100-140; 40 MHz */
		if (chan < 100 || chan > 144)
			return -1;
		return 5000 + 5 * chan;
	case 3: /* channels 149,153,157,161 */
	case 25: /* channels 149,157; 40 MHz */
	case 26: /* channels 149,157; 40 MHz */
	case 30: /* channels 153,161; 40 MHz */
	case 31: /* channels 153,161; 40 MHz */
		if (chan < 149 || chan > 161)
			return -1;
		return 5000 + 5 * chan;
	case 34: /* 60 GHz band, channels 1..3 */
		if (chan < 1 || chan > 3)
			return -1;
		return 56160 + 2160 * chan;
	}
	return -1;
}


 static atbm_int32 ieee80211_chan_to_freq_eu(atbm_uint8 op_class, atbm_uint8 chan)
{
	switch (op_class) {
	case 4: /* channels 1..13 */
	case 11: /* channels 1..9; 40 MHz */
	case 12: /* channels 5..13; 40 MHz */
		if (chan < 1 || chan > 13)
			return -1;
		return 2407 + 5 * chan;
	case 1: /* channels 36,40,44,48 */
	case 2: /* channels 52,56,60,64; dfs */
	case 5: /* channels 36,44; 40 MHz */
	case 6: /* channels 52,60; 40 MHz */
	case 8: /* channels 40,48; 40 MHz */
	case 9: /* channels 56,64; 40 MHz */
		if (chan < 36 || chan > 64)
			return -1;
		return 5000 + 5 * chan;
	case 3: /* channels 100-140 */
	case 7: /* channels 100-132; 40 MHz */
	case 10: /* channels 104-136; 40 MHz */
	case 16: /* channels 100-140 */
		if (chan < 100 || chan > 140)
			return -1;
		return 5000 + 5 * chan;
	case 17: /* channels 149,153,157,161,165,169 */
		if (chan < 149 || chan > 169)
			return -1;
		return 5000 + 5 * chan;
	case 18: /* 60 GHz band, channels 1..4 */
		if (chan < 1 || chan > 4)
			return -1;
		return 56160 + 2160 * chan;
	}
	return -1;
}


 static atbm_int32 ieee80211_chan_to_freq_jp(atbm_uint8 op_class, atbm_uint8 chan)
{
	switch (op_class) {
	case 30: /* channels 1..13 */
	case 56: /* channels 1..9; 40 MHz */
	case 57: /* channels 5..13; 40 MHz */
		if (chan < 1 || chan > 13)
			return -1;
		return 2407 + 5 * chan;
	case 31: /* channel 14 */
		if (chan != 14)
			return -1;
		return 2414 + 5 * chan;
	case 1: /* channels 34,38,42,46(old) or 36,40,44,48 */
	case 32: /* channels 52,56,60,64 */
	case 33: /* channels 52,56,60,64 */
	case 36: /* channels 36,44; 40 MHz */
	case 37: /* channels 52,60; 40 MHz */
	case 38: /* channels 52,60; 40 MHz */
	case 41: /* channels 40,48; 40 MHz */
	case 42: /* channels 56,64; 40 MHz */
	case 43: /* channels 56,64; 40 MHz */
		if (chan < 34 || chan > 64)
			return -1;
		return 5000 + 5 * chan;
	case 34: /* channels 100-140 */
	case 35: /* channels 100-140 */
	case 39: /* channels 100-132; 40 MHz */
	case 40: /* channels 100-132; 40 MHz */
	case 44: /* channels 104-136; 40 MHz */
	case 45: /* channels 104-136; 40 MHz */
	case 58: /* channels 100-140 */
		if (chan < 100 || chan > 140)
			return -1;
		return 5000 + 5 * chan;
	case 59: /* 60 GHz band, channels 1..4 */
		if (chan < 1 || chan > 3)
			return -1;
		return 56160 + 2160 * chan;
	}
	return -1;
}


 static atbm_int32 ieee80211_chan_to_freq_cn(atbm_uint8 op_class, atbm_uint8 chan)
{
	switch (op_class) {
	case 7: /* channels 1..13 */
	case 8: /* channels 1..9; 40 MHz */
	case 9: /* channels 5..13; 40 MHz */
		if (chan < 1 || chan > 13)
			return -1;
		return 2407 + 5 * chan;
	case 1: /* channels 36,40,44,48 */
	case 2: /* channels 52,56,60,64; dfs */
	case 4: /* channels 36,44; 40 MHz */
	case 5: /* channels 52,60; 40 MHz */
		if (chan < 36 || chan > 64)
			return -1;
		return 5000 + 5 * chan;
	case 3: /* channels 149,153,157,161,165 */
	case 6: /* channels 149,157; 40 MHz */
		if (chan < 149 || chan > 165)
			return -1;
		return 5000 + 5 * chan;
	}
	return -1;
}


 static atbm_int32 ieee80211_chan_to_freq_global(atbm_uint8 op_class, atbm_uint8 chan)
{
	/* Table E-4 in IEEE Std 802.11-2012 - Global operating classes */
	switch (op_class) {
	case 81:
		/* channels 1..13 */
		if (chan < 1 || chan > 13)
			return -1;
		return 2407 + 5 * chan;
	case 82:
		/* channel 14 */
		if (chan != 14)
			return -1;
		return 2414 + 5 * chan;
	case 83: /* channels 1..9; 40 MHz */
	case 84: /* channels 5..13; 40 MHz */
		if (chan < 1 || chan > 13)
			return -1;
		return 2407 + 5 * chan;
	case 115: /* channels 36,40,44,48; indoor only */
	case 116: /* channels 36,44; 40 MHz; indoor only */
	case 117: /* channels 40,48; 40 MHz; indoor only */
	case 118: /* channels 52,56,60,64; dfs */
	case 119: /* channels 52,60; 40 MHz; dfs */
	case 120: /* channels 56,64; 40 MHz; dfs */
		if (chan < 36 || chan > 64)
			return -1;
		return 5000 + 5 * chan;
	case 121: /* channels 100-140 */
	case 122: /* channels 100-142; 40 MHz */
	case 123: /* channels 104-136; 40 MHz */
		if (chan < 100 || chan > 140)
			return -1;
		return 5000 + 5 * chan;
	case 124: /* channels 149,153,157,161 */
	case 125: /* channels 149,153,157,161,165,169 */
	case 126: /* channels 149,157; 40 MHz */
	case 127: /* channels 153,161; 40 MHz */
		if (chan < 149 || chan > 161)
			return -1;
		return 5000 + 5 * chan;
	case 128: /* center freqs 42, 58, 106, 122, 138, 155; 80 MHz */
	case 130: /* center freqs 42, 58, 106, 122, 138, 155; 80 MHz */
		if (chan < 36 || chan > 161)
			return -1;
		return 5000 + 5 * chan;
	case 129: /* center freqs 50, 114; 160 MHz */
		if (chan < 50 || chan > 114)
			return -1;
		return 5000 + 5 * chan;
	case 180: /* 60 GHz band, channels 1..4 */
		if (chan < 1 || chan > 4)
			return -1;
		return 56160 + 2160 * chan;
	}
	return -1;
}

/**
 * ieee80211_chan_to_freq - Convert channel info to frequency
 * @country: Country code, if known; otherwise, global operating class is used
 * @op_class: Operating class
 * @chan: Channel number
 * Returns: Frequency in MHz or -1 if the specified channel is unknown
 */
 atbm_int32 ieee80211_chan_to_freq(const char *country, atbm_uint8 op_class, atbm_uint8 chan)
{
	atbm_int32 freq;

	if (country_match(us_op_class_cc, country)) {
		freq = ieee80211_chan_to_freq_us(op_class, chan);
		if (freq > 0)
			return freq;
	}

	if (country_match(eu_op_class_cc, country)) {
		freq = ieee80211_chan_to_freq_eu(op_class, chan);
		if (freq > 0)
			return freq;
	}

	if (country_match(jp_op_class_cc, country)) {
		freq = ieee80211_chan_to_freq_jp(op_class, chan);
		if (freq > 0)
			return freq;
	}

	if (country_match(cn_op_class_cc, country)) {
		freq = ieee80211_chan_to_freq_cn(op_class, chan);
		if (freq > 0)
			return freq;
	}

	return ieee80211_chan_to_freq_global(op_class, chan);
}


 atbm_int32 ieee80211_is_dfs(atbm_int32 freq)
{
	/* TODO: this could be more accurate to better cover all domains */
	return (freq >= 5260 && freq <= 5320) || (freq >= 5500 && freq <= 5700);
}


 atbm_int32 is_11b(atbm_uint8 rate)
{
	return rate == 0x02 || rate == 0x04 || rate == 0x0b || rate == 0x16;
}





