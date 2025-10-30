/*
 * hostapd / IEEE 802.11ac VHT
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of BSD license
 *
 * See README and COPYING for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "common/ieee802_11_defs.h"
#include "hostapd.h"
#include "ap_config.h"
#include "sta_info.h"
#include "beacon.h"
#include "ieee802_11.h"
#include "dfs.h"

u64 read_tsf_timer(void)
{
	return 0;
}

u8 * hostapd_eid_s1g_capabilities(struct hostapd_data *hapd, u8 *eid, struct ieee80211_s1g_capabilities *sta_cap)
{
	struct ieee80211_s1g_capabilities *cap = NULL;
	struct hostapd_hw_modes *mode = hapd->iface->current_mode;
	u8 *pos = eid;
	u16 i = 0, Supported_BW = 0, min_MPDU_Start_spacing = 0, x = 0 , y = 0;

	if (!mode)
		wpa_printf(MSG_DEBUG, "mode is not avail...\n");
	else
		wpa_printf(MSG_DEBUG, "WLAN_EID_S1G_CAPABILITIES IE is avail = %d\n", WLAN_EID_S1G_CAPABILITIES);

	*pos++ = WLAN_EID_S1G_CAPABILITIES;
	*pos++ = sizeof(struct ieee80211_s1g_capabilities);

	cap = (struct ieee80211_s1g_capabilities *) pos;
	os_memset(cap, 0, sizeof(struct ieee80211_s1g_capabilities));
	os_memcpy(&cap->s1g_capabilities_info, &hapd->iface->conf->s1g_capab[0], 10);/* Supported MCS set comes from hw */
	os_memcpy(&cap->supported_s1g_mcs_nss_set, &hapd->iface->conf->supported_s1g_mcs_nss_set[0], 5);

	if (sta_cap != NULL) {

		//CAP info setting
		x = cap->s1g_capabilities_info.capabilities_info.Supported_BW;
		y = sta_cap->s1g_capabilities_info.capabilities_info.Supported_BW;
		Supported_BW = WPA_MIN(x, y);
		x = cap->s1g_capabilities_info.capabilities_info.min_MPDU_Start_spacing;
		y = sta_cap->s1g_capabilities_info.capabilities_info.min_MPDU_Start_spacing;
		min_MPDU_Start_spacing = WPA_MIN(x, y);

		for (i = 0; i < 10; i++)
		 cap->s1g_capabilities_info.s1g_cap_info[i] &= sta_cap->s1g_capabilities_info.s1g_cap_info[i];

		cap->s1g_capabilities_info.capabilities_info.Supported_BW = Supported_BW;
		cap->s1g_capabilities_info.capabilities_info.min_MPDU_Start_spacing = min_MPDU_Start_spacing;

		//RX/TX MCS setting
		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_1SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_1SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_1SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_2SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_2SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_2SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_3SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_3SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_3SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_4SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_4SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_4SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Highest_Supported_Long_GI_Data_Rate;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Highest_Supported_Long_GI_Data_Rate;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Highest_Supported_Long_GI_Data_Rate = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Single_SS_S1G_MCS_1MHZ;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Single_SS_S1G_MCS_1MHZ;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Single_SS_S1G_MCS_1MHZ = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_1SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_1SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_1SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_2SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_2SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_2SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_3SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_3SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_3SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_4SS;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_4SS;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_4SS = WPA_MIN(x, y);

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Highest_Supported_Long_GI_Data_Rate_0_6 |
			(cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Highest_Supported_Long_GI_Data_Rate_7_8 << 7) ;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Highest_Supported_Long_GI_Data_Rate_0_6 |
			(sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Highest_Supported_Long_GI_Data_Rate_7_8 << 7) ;
		y = WPA_MIN(x, y);
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Highest_Supported_Long_GI_Data_Rate_0_6 = y & 0x7f;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Highest_Supported_Long_GI_Data_Rate_7_8 = (y >> 7) & 0x3;

		x = cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Single_SS_S1G_MCS_1MHZ;
		y = sta_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Single_SS_S1G_MCS_1MHZ;
		cap->supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Single_SS_S1G_MCS_1MHZ = WPA_MIN(x, y);
	}

	pos += sizeof(struct ieee80211_s1g_capabilities);

	return pos;
}

u8 * hostapd_eid_s1g_operation(struct hostapd_data *hapd, u8 *eid)
{
	struct ieee80211_s1g_operation *oper;
	u8 *pos = eid;
	wpa_printf(MSG_DEBUG, "WLAN_EID_S1G_OPERATION IE is avail=%d\n", WLAN_EID_S1G_OPERATION);

	*pos++ = WLAN_EID_S1G_OPERATION;
	*pos++ = sizeof(struct ieee80211_s1g_operation);

	oper = (struct ieee80211_s1g_operation *) pos;
	os_memset(oper, 0, sizeof(struct ieee80211_s1g_operation));

	oper->channel_width = hapd->iface->conf->s1g_oper_chwidth;
	oper->operating_class = hapd->iface->conf->s1g_operating_class;
	oper->pri_channel_number = hapd->iface->conf->s1g_pri_channel_number;
	oper->channel_center_freq = hapd->iface->conf->s1g_channel_center_freq;
	oper->basic_s1g_mcs_nss_set.b_mcs_nss_set = host_to_le16(hapd->iface->conf->basic_s1g_mcs_nss_set);

	pos += sizeof(struct ieee80211_s1g_operation);

	return pos;
}

u8 * hostapd_eid_s1g_compatibility(struct hostapd_data *hapd, u8 *eid)
{
	struct ieee80211_s1g_compatibility_ie *pcompat;
	u8 *pos = eid;
	u64 tsf_timer;
	u32 high_tsf_timer;
	u16 cap_info;

	*pos++ = WLAN_EID_S1G_BEACON_COMPATIBILITY;
	*pos++ = sizeof(struct ieee80211_s1g_compatibility_ie);

	pcompat = (struct ieee80211_s1g_compatibility_ie *) pos;
	os_memset(pcompat, 0, sizeof(struct ieee80211_s1g_compatibility_ie));
	pcompat->beacon_interval = host_to_le16(hapd->iface->conf->s1g_full_beacon_int);
	cap_info = hostapd_own_capab_info(hapd);

	/*
	 * The TSF Completion field carries the 4 most significant octets of the TSF timer at the AP at the time of
	 * generation of the element carrying the TSF Completion field.
	 * here use 0 for temporary, should be updated with tsf timer API.
	**/
	tsf_timer = read_tsf_timer();
	if (tsf_timer & 0x80000000)
		cap_info |= WLAN_CAPABILITY_TSF_ROLLOVER;
	else
		cap_info &= ~WLAN_CAPABILITY_TSF_ROLLOVER;

	high_tsf_timer = tsf_timer >> 32;
	pcompat->compatibility_info = host_to_le16(cap_info);//;hapd->iface->conf->s1g_compatibility_info);
	pcompat->tsf_completion = host_to_le32(high_tsf_timer);
	pos += sizeof(struct ieee80211_s1g_compatibility_ie);

	return pos;
}
/*FMS Descriptor element*/
u8 * hostapd_eid_s1g_fms_descriptor(struct hostapd_data *hapd, u8 *eid)
{
	return eid;
}

/* RAW support*/
u8 * hostapd_eid_s1g_rps(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	struct ieee80211_s1g_rps_ie *ptr = NULL;

	if(hapd->iface->conf->s1g_capab[6] & S1G_CAP_INFO_RAW_OPERATION_SUPPORT)
	{
		*pos++ = WLAN_EID_S1G_RPS;
		*pos++ = sizeof(struct ieee80211_s1g_rps_ie);

		ptr = (struct ieee80211_s1g_rps_ie *)pos;

		/**
		  * Bit 0 (Bit 2 of RAW Control subfield): Paged STA
		  * Bit 1(Bit 3 of RAW Control subfield): RA Frame
		  */
		ptr->control = 0x04;
		/*FormatIndication(1bit) + CrossSlotBoundary(1bit) + SlotDurationCount(y) + NumberofSlots((14 -y)bit)*/
		/*here is example of 7 slots, DSLOT = 500 + CSLOT × 120 = (500 + 256*120) microseconds*/
		ptr->slot_definition = host_to_le16(0xe3fc);//0b1110,0011,1111,1100;
		/*	since RAW Control has no indications for RAW Start Time, RAW Group, Channel Indication, Periodic Operation Parameters */
		/*	RAW Control, RAW Slot Definition, RAW Start Time, RAW Group, Channel Indication, Periodic Operation Parameters */
		pos += sizeof(struct ieee80211_s1g_rps_ie);
	}

	return pos;
}


u8 * hostapd_eid_s1g_relay(struct hostapd_data *hapd, u8 *eid)
{
	return eid;
}

u8 * hostapd_eid_s1g_page_slice(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	le32 slice_control_tmp;
	struct ieee80211_s1g_page_slice_ie *ptr = NULL;
	union {
		u32 control;
		u32 index : 		2,
			slice_len : 	5,
			slice_count : 	5,
			block_offset :	5,
			TIM_offset :	4,
			Reserved :		11;
	}slice_control;

	if(hapd->iface->conf->s1g_capab[6] & S1G_CAP_INFO_PAGE_SLICING_SUPPORT)
	{
		*pos++ = WLAN_EID_S1G_PAGE_SLICE;
		*pos++ = sizeof(struct ieee80211_s1g_page_slice_ie);
		ptr = (struct ieee80211_s1g_page_slice_ie *)pos;

		/*The Page Period field indicates the number of beacon intervals between successive beacons that carry the
		  Page Slice element for the associated page.*/
		ptr->period = 10;
		slice_control.index = 3;
		slice_control.slice_len = 4;
		slice_control.block_offset = 5;
		slice_control.TIM_offset = 6;
		slice_control_tmp = host_to_le32(slice_control.control & 0x00ffffff);
		ptr->slice_control[0] = slice_control_tmp & 0xff;
		ptr->slice_control[1] = (slice_control_tmp >> 8) & 0xff;
		ptr->slice_control[2] = (slice_control_tmp >> 16) & 0xff;
		pos += sizeof(struct ieee80211_s1g_page_slice_ie);
	}

	return pos;
}

u8 * hostapd_eid_s1g_sector_operation(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_aid_req(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}


u8 * hostapd_eid_s1g_aid_resp(struct hostapd_data *hapd, u16 aid, u8 *eid)
{
	u8 *pos = eid;
	struct ieee80211_s1g_aid_resp_ie *ptr;

	*pos++ = WLAN_EID_S1G_AID_RESPONSE;
	*pos++ = sizeof(struct ieee80211_s1g_aid_resp_ie);
	ptr = (struct ieee80211_s1g_aid_resp_ie *)pos;

	ptr->aid = host_to_le16(aid | BIT(14) | BIT(15));
	ptr->switch_count = hapd->iface->conf->s1g_aid_switch_count; //for now 2 beacon intervals for AID switch. should be figured in hostapd_s1g.conf.
	ptr->unscaled_interval = host_to_le16(hapd->iface->conf->s1g_unscaled_interval);
	ptr->unified_scaling_factor = host_to_le16(hapd->iface->conf->s1g_unified_scaling_factor);
	pos += sizeof(struct ieee80211_s1g_aid_resp_ie);

	return pos;
}

u8 * hostapd_eid_s1g_short_beacon_interval(struct hostapd_data *hapd, u8 *eid)
{

	struct ieee80211_s1g_short_beacon_interval_ie *ptr;
	u8 *pos = eid;

	*pos++ = WLAN_EID_S1G_SHORT_BEACON_INTERVAL;
	*pos++ = sizeof(struct ieee80211_s1g_short_beacon_interval_ie);

	ptr = (struct ieee80211_s1g_short_beacon_interval_ie *)pos;

	ptr->short_beacon_interval = host_to_le16(hapd->iconf->beacon_int);;
	pos += sizeof(struct ieee80211_s1g_short_beacon_interval_ie);

	return pos;
}

u8 * hostapd_eid_s1g_change_sequence(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_twt(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_auth_control(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_tsf_timer_accuracy(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_reachable_address(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_relay_activation(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_relay_discovery(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_AID_announ(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_pv1_probe_resp_option(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_el_operation(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_sectorized_grp_id_list(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_header_compression(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_sst_operation(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

u8 * hostapd_eid_s1g_sst_selective(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}


u8 * hostapd_eid_s1g_mad(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	return pos;
}

#if 0
u8 * hostapd_eid_s1g_vendor(struct hostapd_data *hapd, u8 *eid)
{
		struct ieee80211_vendor_ie *ptr;
		ptr->element_id = IE_VENDOR_SPECIFIC;
		ptr->len = 4;
	/*oui should be first 3 bytes of MAC address*/
		os_memcpy(ptr->oui, hapd->conf->BSSID, 3);
		ptr->oui_type = VENDOR_S1G_TYPE;

		return sizeof(struct ieee80211_vendor_ie);
}
#endif

static int check_valid_s1g_mcs(struct hostapd_hw_modes *mode,
			       const u8 *sta_s1g_capab)
{
	const struct ieee80211_s1g_capabilities *s1g_cap;
	struct ieee80211_s1g_capabilities ap_s1g_cap;
	u32 sta_rx_mcs_set, ap_tx_mcs_set;
	int i;

	if (!mode)
		return 1;

	/*
	 * Disable s1g caps for STAs for which there is not even a single
	 * allowed MCS in any supported number of streams, i.e., STA is
	 * advertising 3 (not supported) as s1g MCS rates for all supported
	 * stream cases.
	 */
	os_memcpy(&ap_s1g_cap.supported_s1g_mcs_nss_set, mode->s1g_mcs_set,
		  sizeof(ap_s1g_cap.supported_s1g_mcs_nss_set));
	s1g_cap = (const struct ieee80211_s1g_capabilities *) sta_s1g_capab;

	/* AP Tx MCS map vs. STA Rx MCS map */
	ap_tx_mcs_set = ap_s1g_cap.supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Single_SS_S1G_MCS_1MHZ |
					(ap_s1g_cap.supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_1SS >> 2) |
					(ap_s1g_cap.supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_2SS >> 4) |
					(ap_s1g_cap.supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_3SS >> 6) |
					(ap_s1g_cap.supported_s1g_mcs_nss_set.mcs_nss_set.Tx_Max_S1G_MCS_4SS >> 8);
	sta_rx_mcs_set = s1g_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Single_SS_S1G_MCS_1MHZ |
					(s1g_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_1SS >> 2) |
					(s1g_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_2SS >> 4) |
					(s1g_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_3SS >> 6) |
					(s1g_cap->supported_s1g_mcs_nss_set.mcs_nss_set.Rx_Max_S1G_MCS_4SS >> 8);


	for (i = 0; i < S1G_RX_NSS_MAX_STREAMS + 1; i++) {
		if (((ap_tx_mcs_set >> (i * 2)) & 0x3) == 3)
			continue;
		if (((sta_rx_mcs_set >> (i * 2)) & 0x3) == 3)
			continue;

		return 1;
	}
	return 1;
}

/* to be done*/
u8 * hostapd_eid_s1g_wb_chsw_wrapper(struct hostapd_data *hapd, u8 *eid)
{
	#if 0
	u8 bw, chan1, chan2 = 0;
	int freq1;

	if (!hapd->cs_freq_params.channel ||
	    !hapd->cs_freq_params.s1g_enabled)
		return eid;

	/* bandwidth: 0: 40, 1: 80, 2: 160, 3: 80+80 */
	switch (hapd->cs_freq_params.bandwidth) {
	/*S1G Bandwidth FROM 1M TO 16M ---To Be Done*/
	case 1:
		bw = 0;
		break;
	case 2:
		bw = 0;
		break;
	case 4:
		bw = 0;
		break;
	case 8:
		bw = 0;
		break;
	case 16:
		bw = 0;
		break;
	case 40:
		bw = 0;
		break;
	case 80:
		/* check if it's 80+80 */
		if (!hapd->cs_freq_params.center_freq2)
			bw = 1;
		else
			bw = 3;
		break;
	case 160:
		bw = 2;
		break;
	default:
		/* not valid VHT bandwidth or not in CSA */
		return eid;
	}

	freq1 = hapd->cs_freq_params.center_freq1 ?
		hapd->cs_freq_params.center_freq1 :
		hapd->cs_freq_params.freq;
	if (ieee80211_freq_to_chan(freq1, &chan1) !=
	    HOSTAPD_MODE_IEEE80211A)
		return eid;

	if (hapd->cs_freq_params.center_freq2 &&
	    ieee80211_freq_to_chan(hapd->cs_freq_params.center_freq2,
				   &chan2) != HOSTAPD_MODE_IEEE80211A)
		return eid;

	*eid++ = WLAN_EID_s1g_CHANNEL_SWITCH_WRAPPER;
	*eid++ = 5; /* Length of Channel Switch Wrapper */
	*eid++ = WLAN_EID_s1g_WIDE_BW_CHSWITCH;
	*eid++ = 3; /* Length of Wide Bandwidth Channel Switch element */
	*eid++ = bw; /* New Channel Width */
	*eid++ = chan1; /* New Channel Center Frequency Segment 0 */
	*eid++ = chan2; /* New Channel Center Frequency Segment 1 */

	return eid;
	#endif
	return eid;
}


u8 * hostapd_eid_s1g_txpower_envelope(struct hostapd_data *hapd, u8 *eid)
{
#if 0
	struct hostapd_iface *iface = hapd->iface;
	struct hostapd_config *iconf = iface->conf;
	struct hostapd_hw_modes *mode = iface->current_mode;
	struct hostapd_channel_data *chan;
	int dfs, i;
	u8 channel, tx_pwr_count, local_pwr_constraint;
	int max_tx_power;
	u8 tx_pwr;

	if (!mode)
		return eid;

	if (ieee80211_freq_to_chan(iface->freq, &channel) == NUM_HOSTAPD_MODES)
		return eid;

	for (i = 0; i < mode->num_channels; i++) {
		if (mode->channels[i].freq == iface->freq)
			break;
	}
	if (i == mode->num_channels)
		return eid;

	switch (iface->conf->s1g_oper_chwidth) {
	case s1g_CHANWIDTH_USE_HT:
		if (iconf->secondary_channel == 0) {
			/* Max Transmit Power count = 0 (20 MHz) */
			tx_pwr_count = 0;
		} else {
			/* Max Transmit Power count = 1 (20, 40 MHz) */
			tx_pwr_count = 1;
		}
		break;
	case s1g_CHANWIDTH_80MHZ:
		/* Max Transmit Power count = 2 (20, 40, and 80 MHz) */
		tx_pwr_count = 2;
		break;
	case s1g_CHANWIDTH_80P80MHZ:
	case s1g_CHANWIDTH_160MHZ:
		/* Max Transmit Power count = 3 (20, 40, 80, 160/80+80 MHz) */
		tx_pwr_count = 3;
		break;
	default:
		return eid;
	}

	/*
	 * Below local_pwr_constraint logic is referred from
	 * hostapd_eid_pwr_constraint.
	 *
	 * Check if DFS is required by regulatory.
	 */
	dfs = hostapd_is_dfs_required(hapd->iface);
	if (dfs < 0)
		dfs = 0;

	/*
	 * In order to meet regulations when TPC is not implemented using
	 * a transmit power that is below the legal maximum (including any
	 * mitigation factor) should help. In this case, indicate 3 dB below
	 * maximum allowed transmit power.
	 */
	if (hapd->iconf->local_pwr_constraint == -1)
		local_pwr_constraint = (dfs == 0) ? 0 : 3;
	else
		local_pwr_constraint = hapd->iconf->local_pwr_constraint;

	/*
	 * A STA that is not an AP shall use a transmit power less than or
	 * equal to the local maximum transmit power level for the channel.
	 * The local maximum transmit power can be calculated from the formula:
	 * local max TX pwr = max TX pwr - local pwr constraint
	 * Where max TX pwr is maximum transmit power level specified for
	 * channel in Country element and local pwr constraint is specified
	 * for channel in this Power Constraint element.
	 */
	chan = &mode->channels[i];
	max_tx_power = chan->max_tx_power - local_pwr_constraint;

	/*
	 * Local Maximum Transmit power is encoded as two's complement
	 * with a 0.5 dB step.
	 */
	max_tx_power *= 2; /* in 0.5 dB steps */
	if (max_tx_power > 127) {
		/* 63.5 has special meaning of 63.5 dBm or higher */
		max_tx_power = 127;
	}
	if (max_tx_power < -128)
		max_tx_power = -128;
	if (max_tx_power < 0)
		tx_pwr = 0x80 + max_tx_power + 128;
	else
		tx_pwr = max_tx_power;

	*eid++ = WLAN_EID_s1g_TRANSMIT_POWER_ENVELOPE;
	*eid++ = 2 + tx_pwr_count;

	/*
	 * Max Transmit Power count and
	 * Max Transmit Power units = 0 (EIRP)
	 */
	*eid++ = tx_pwr_count;

	for (i = 0; i <= tx_pwr_count; i++)
		*eid++ = tx_pwr;
#endif
	return eid;
}


u16 copy_sta_s1g_capab(struct hostapd_data *hapd, struct sta_info *sta,
		       const u8 *s1g_capab)
{
	/* Disable VHT caps for STAs associated to no-VHT BSSes. */

	if(!s1g_capab) wpa_printf(MSG_DEBUG,"copy_sta_s1g_capab-->s1g_capab is NULL....");
	if (!s1g_capab ||
	    hapd->conf->disable_11ah ||
	    !check_valid_s1g_mcs(hapd->iface->current_mode, s1g_capab)) {

		sta->flags &= ~WLAN_STA_S1G;
		//sta->flags |= WLAN_STA_S1G; // force it....
		os_free(sta->s1g_capabilities);
		sta->s1g_capabilities = NULL;
		wpa_printf(MSG_DEBUG,"copy_sta_s1g_capab-->!s1g_capab....");
		return WLAN_STATUS_SUCCESS;
	}

	if (sta->s1g_capabilities == NULL) {
		sta->s1g_capabilities =
			os_zalloc(sizeof(struct ieee80211_s1g_capabilities));
		if (sta->s1g_capabilities == NULL){
			wpa_printf(MSG_DEBUG,"copy_sta_s1g_capab-->WLAN_STATUS_UNSPECIFIED_FAILURE");
			return WLAN_STATUS_UNSPECIFIED_FAILURE;
		}
	}

	sta->flags |= WLAN_STA_S1G;
	wpa_printf(MSG_DEBUG,"copy_sta_s1g_capab--> sta->flags = %x, WLAN_STA_S1G=%x\n", sta->flags, WLAN_STA_S1G);
	os_memcpy(sta->s1g_capabilities, s1g_capab,
		  sizeof(struct ieee80211_s1g_capabilities));

	return WLAN_STATUS_SUCCESS;
}

#if 0
//to be done
u16 copy_sta_vendor_s1g(struct hostapd_data *hapd, struct sta_info *sta,
			const u8 *ie, size_t len)
{
	const u8 *s1g_capab;
	unsigned int s1g_capab_len;

	if (!ie || len < 5 + 2 + sizeof(struct ieee80211_s1g_capabilities) ||
	    hapd->conf->disable_11ah)
		goto no_capab;

	/* The VHT Capabilities element embedded in vendor VHT */
	s1g_capab = ie + 5;
	if (s1gcapab[0] != WLAN_EID_S1G_CAP)
		goto no_capab;
	s1g_capab_len = s1g_capab[1];
	if (s1g_capab_len < sizeof(struct ieee80211_s1g_capabilities) ||
	    (int) s1g_capab_len > ie + len - s1g_capab - 2)
		goto no_capab;
	s1g_capab += 2;

	if (sta->s1g_capabilities == NULL) {
		sta->s1g_capabilities =
			os_zalloc(sizeof(struct ieee80211_s1g_capabilities));
		if (sta->s1g_capabilities == NULL)
			return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	sta->flags |= WLAN_STA_s1g | WLAN_STA_VENDOR_s1g;
	os_memcpy(sta->s1g_capabilities, s1g_capab,
		  sizeof(struct ieee80211_s1g_capabilities));
	return WLAN_STATUS_SUCCESS;

no_capab:
	sta->flags &= ~WLAN_STA_VENDOR_s1g;
	return WLAN_STATUS_SUCCESS;
}

#endif

u16 set_sta_s1g_opmode(struct hostapd_data *hapd, struct sta_info *sta,
		       const u8 *s1g_oper_notif)
{
	if (!s1g_oper_notif) {
		sta->flags &= ~WLAN_STA_S1G_OPMODE_ENABLED;
		return WLAN_STATUS_SUCCESS;
	}

	sta->flags |= WLAN_STA_S1G_OPMODE_ENABLED;
	sta->s1g_opmode = *s1g_oper_notif;
	return WLAN_STATUS_SUCCESS;
}

u8 * hostapd_eid_vendor_s1g(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;

	if (!hapd->iface->current_mode)
		return eid;

	*pos++ = WLAN_EID_VENDOR_SPECIFIC;
	*pos++ = (5 +		/* The Vendor OUI, type and subtype */
		  2 + sizeof(struct ieee80211_s1g_capabilities) +
		  2 + sizeof(struct ieee80211_s1g_operation));

	WPA_PUT_BE32(pos, (OUI_SUSHIMAC11AH << 8) | VENDOR_S1G_TYPE);
	pos += 4;
	*pos++ = VENDOR_S1G_SUBTYPE;
//	pos = hostapd_eid_s1g_capabilities(hapd, pos, NULL);
//	pos = hostapd_eid_s1g_operation(hapd, pos);

	return pos;
}

//Need to modify this later according to AH draft 8.0
int ieee802_11_rx_s1g_action_ap(struct hostapd_data *hapd,
				const struct ieee80211_mgmt *mgmt, size_t len)
{
	u8 action;
	const u8 *payload;
	size_t plen;

	if (len < IEEE80211_HDRLEN + 2)
		return -1;

	payload = ((const u8 *) mgmt) + IEEE80211_HDRLEN + 1;
	action = *payload++;
	plen = len - IEEE80211_HDRLEN - 2;

	switch (action) {
		case WLAN_UNPROTECTED_S1G_ACTION_AID_SW_REQ:
		case WLAN_UNPROTECTED_S1G_ACTION_AID_SW_RESP:
		case WLAN_UNPROTECTED_S1G_ACTION_SYNC_CTRL:
		case WLAN_UNPROTECTED_S1G_ACTION_STA_INFO_ANN:
		case WLAN_UNPROTECTED_S1G_ACTION_TWT_SETUP:
		case WLAN_UNPROTECTED_S1G_ACTION_TWT_TEARDOWN:
		case WLAN_UNPROTECTED_S1G_ACTION_EDCA_PARM_SET:
		case WLAN_UNPROTECTED_S1G_ACTION_EL_OPERATION:
		case WLAN_UNPROTECTED_S1G_ACTION_SECTORIZED_GRP_ID_LIST:
		case WLAN_UNPROTECTED_S1G_ACTION_SECTOR_ID_FEEDBACK:
		case WLAN_UNPROTECTED_S1G_ACTION_TWT_INFO:
			return 0;
	}

	wpa_printf(MSG_DEBUG, "Unsupported WLAN_UNPROTECTED_S1G_ACTION %u from " MACSTR,
		   action, MAC2STR(mgmt->sa));
	return -1;
}

/* to be done*/
#if 1
void hostapd_get_s1g_capab(struct hostapd_data *hapd,
			   struct ieee80211_s1g_capabilities *s1g_cap,
			   struct ieee80211_s1g_capabilities *neg_s1g_cap)
{
	//u32 cap, own_cap, sym_caps;

	if (s1g_cap == NULL)
		return;
	os_memcpy(neg_s1g_cap, s1g_cap, sizeof(*neg_s1g_cap));
#if 0
	cap = le_to_host32(neg_s1g_cap->s1g_capabilities_info);
	own_cap = hapd->iconf->s1g_capab;

	/* mask out symmetric VHT capabilities we don't support */
	sym_caps = s1g_CAP_SHORT_GI_80 | s1g_CAP_SHORT_GI_160;
	cap &= ~sym_caps | (own_cap & sym_caps);

	/* mask out beamformer/beamformee caps if not supported */
	if (!(own_cap & s1g_CAP_SU_BEAMFORMER_CAPABLE))
		cap &= ~(s1g_CAP_SU_BEAMFORMEE_CAPABLE |
			 s1g_CAP_BEAMFORMEE_STS_MAX);

	if (!(own_cap & s1g_CAP_SU_BEAMFORMEE_CAPABLE))
		cap &= ~(s1g_CAP_SU_BEAMFORMER_CAPABLE |
			 s1g_CAP_SOUNDING_DIMENSION_MAX);

	if (!(own_cap & s1g_CAP_MU_BEAMFORMER_CAPABLE))
		cap &= ~s1g_CAP_MU_BEAMFORMEE_CAPABLE;

	if (!(own_cap & s1g_CAP_MU_BEAMFORMEE_CAPABLE))
		cap &= ~s1g_CAP_MU_BEAMFORMER_CAPABLE;

	/* mask channel widths we don't support */
	switch (own_cap & s1g_CAP_SUPP_CHAN_WIDTH_MASK) {
	case s1g_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ:
		break;
	case s1g_CAP_SUPP_CHAN_WIDTH_160MHZ:
		if (cap & s1g_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ) {
			cap &= ~s1g_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ;
			cap |= s1g_CAP_SUPP_CHAN_WIDTH_160MHZ;
		}
		break;
	default:
		cap &= ~s1g_CAP_SUPP_CHAN_WIDTH_MASK;
		break;
	}

	if (!(cap & s1g_CAP_SUPP_CHAN_WIDTH_MASK))
		cap &= ~s1g_CAP_SHORT_GI_160;

	/*
	 * if we don't support RX STBC, mask out TX STBC in the STA's HT caps
	 * if we don't support TX STBC, mask out RX STBC in the STA's HT caps
	 */
	if (!(own_cap & s1g_CAP_RXSTBC_MASK))
		cap &= ~s1g_CAP_TXSTBC;
	if (!(own_cap & s1g_CAP_TXSTBC))
		cap &= ~s1g_CAP_RXSTBC_MASK;

	neg_s1g_cap->s1g_capabilities_info = host_to_le32(cap);
	#endif
}
#endif