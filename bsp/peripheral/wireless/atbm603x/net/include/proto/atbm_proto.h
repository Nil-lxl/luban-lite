/**************************************************************************************************************
 * altobeam RTOS wifi hmac source code 
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/


#ifndef ATBMWIFI_PROTO_H
#define ATBMWIFI_PROTO_H

#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/epstruct.h"
#endif
#include "atbm_hal.h"
#define ATBM_IEEE80211_FCTL_WEP      0x4000
#define ATBM_IEEE80211_QOS_DATAGRP   0x0080
#define ATBM_FCS_LEN 4
#define ATBM_IEEE80211_FCTL_VERS		0x0003
#define ATBM_IEEE80211_FCTL_FTYPE		0x000c
#define ATBM_IEEE80211_FCTL_STYPE		0x00f0
#define ATBM_IEEE80211_FCTL_TODS		0x0100
#define ATBM_IEEE80211_FCTL_FROMDS		0x0200
#define ATBM_IEEE80211_FCTL_MOREFRAGS	0x0400
#define ATBM_IEEE80211_FCTL_RETRY		0x0800
#define ATBM_IEEE80211_FCTL_PM		0x1000
#define ATBM_IEEE80211_FCTL_MOREDATA		0x2000
#define ATBM_IEEE80211_FCTL_PROTECTED	0x4000
#define ATBM_IEEE80211_FCTL_ORDER		0x8000

#define ATBM_IEEE80211_SCTL_FRAG		0x000F
#define ATBM_IEEE80211_SCTL_SEQ		0xFFF0

#define ATBM_IEEE80211_FTYPE_MGMT		0x0000
#define ATBM_IEEE80211_FTYPE_CTL		0x0004
#define ATBM_IEEE80211_FTYPE_DATA		0x0008

/* management */
#define ATBM_IEEE80211_STYPE_ASSOC_REQ	0x0000
#define ATBM_IEEE80211_STYPE_ASSOC_RESP	0x0010
#define ATBM_IEEE80211_STYPE_REASSOC_REQ	0x0020
#define ATBM_IEEE80211_STYPE_REASSOC_RESP	0x0030
#define ATBM_IEEE80211_STYPE_PROBE_REQ	0x0040
#define ATBM_IEEE80211_STYPE_PROBE_RESP	0x0050
#define ATBM_IEEE80211_STYPE_BEACON		0x0080
#define ATBM_IEEE80211_STYPE_ATIM		0x0090
#define ATBM_IEEE80211_STYPE_DISASSOC	0x00A0
#define ATBM_IEEE80211_STYPE_AUTH		0x00B0
#define ATBM_IEEE80211_STYPE_DEAUTH		0x00C0
#define ATBM_IEEE80211_STYPE_ACTION		0x00D0

/* control */
#define ATBM_IEEE80211_STYPE_BACK_REQ	0x0080
#define ATBM_IEEE80211_STYPE_BACK		0x0090
#define ATBM_IEEE80211_STYPE_PSPOLL		0x00A0
#define ATBM_IEEE80211_STYPE_RTS		0x00B0
#define ATBM_IEEE80211_STYPE_CTS		0x00C0
#define ATBM_IEEE80211_STYPE_ACK		0x00D0
#define ATBM_IEEE80211_STYPE_CFEND		0x00E0
#define ATBM_IEEE80211_STYPE_CFENDACK	0x00F0

/* data */
#define ATBM_IEEE80211_STYPE_DATA			0x0000
#define ATBM_IEEE80211_STYPE_DATA_CFACK		0x0010
#define ATBM_IEEE80211_STYPE_DATA_CFPOLL		0x0020
#define ATBM_IEEE80211_STYPE_DATA_CFACKPOLL		0x0030
#define ATBM_IEEE80211_STYPE_NULLFUNC		0x0040
#define ATBM_IEEE80211_STYPE_CFACK			0x0050
#define ATBM_IEEE80211_STYPE_CFPOLL			0x0060
#define ATBM_IEEE80211_STYPE_CFACKPOLL		0x0070
#define ATBM_IEEE80211_STYPE_QOS_DATA		0x0080
#define ATBM_IEEE80211_STYPE_QOS_DATA_CFACK		0x0090
#define ATBM_IEEE80211_STYPE_QOS_DATA_CFPOLL		0x00A0
#define ATBM_IEEE80211_STYPE_QOS_DATA_CFACKPOLL	0x00B0
#define ATBM_IEEE80211_STYPE_QOS_NULLFUNC		0x00C0
#define ATBM_IEEE80211_STYPE_QOS_CFACK		0x00D0
#define ATBM_IEEE80211_STYPE_QOS_CFPOLL		0x00E0
#define ATBM_IEEE80211_STYPE_QOS_CFACKPOLL		0x00F0


/* miscellaneous IEEE 802.11 constants */
#define ATBM_IEEE80211_MAX_FRAG_THRESHOLD	2352
#define ATBM_IEEE80211_MAX_RTS_THRESHOLD	2353
#define ATBM_IEEE80211_MAX_AID		2007
#define ATBM_IEEE80211_MAX_TIM_LEN		251
/* Maximum size for the MA-UNITDATA primitive, 802.11 standard section
   6.2.1.1.2.

   802.11e clarifies the figure in section 7.1.2. The frame body is
   up to 2304 octets long (maximum MSDU size) plus any crypt overhead. */
#define ATBM_IEEE80211_MAX_DATA_LEN		2304
/* 30 byte 4 addr hdr, 2 byte QoS, 2304 byte MSDU, 12 byte crypt, 4 byte FCS */
#define ATBM_IEEE80211_MAX_FRAME_LEN		2352

#define ATBM_IEEE80211_MAX_SSID_LEN		32


#define ATBM_IEEE80211_QOS_CTL_LEN		2
/* 1d tag mask */
#define ATBM_IEEE80211_QOS_CTL_TAG1D_MASK		0x0007
/* TID mask */
#define ATBM_IEEE80211_QOS_CTL_TID_MASK		0x0007
/* EOSP */
#define ATBM_IEEE80211_QOS_CTL_EOSP			0x0010
/* ACK policy */
#define ATBM_IEEE80211_QOS_CTL_ACK_POLICY_NORMAL	0x0000
#define ATBM_IEEE80211_QOS_CTL_ACK_POLICY_NOACK	0x0020
#define ATBM_IEEE80211_QOS_CTL_ACK_POLICY_NO_EXPL	0x0040
#define ATBM_IEEE80211_QOS_CTL_ACK_POLICY_BLOCKACK	0x0060
/* A-MSDU 802.11n */
#define ATBM_IEEE80211_QOS_CTL_A_MSDU_PRESENT	0x0080

/* U-APSD queue for WMM IEs sent by AP */
#define ATBM_IEEE80211_WMM_IE_AP_QOSINFO_UAPSD	(1<<7)
#define ATBM_IEEE80211_WMM_IE_AP_QOSINFO_PARAM_SET_CNT_MASK	0x0f

/* U-APSD queues for WMM IEs sent by STA */
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_VO	(1<<0)
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_VI	(1<<1)
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_BK	(1<<2)
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_BE	(1<<3)
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK	0x0f

/* U-APSD max SP length for WMM IEs sent by STA */
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_ALL	0x00
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_2	0x01
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_4	0x02
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_6	0x03
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_MASK	0x03
#define ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_SHIFT	5
#define ATBM_IEEE80211_DEFAULT_MAX_SP_LEN    \
	ATBM_IEEE80211_WMM_IE_STA_QOSINFO_SP_ALL
#define ATBM_IEEE80211_DEFAULT_UAPSD_QUEUES \
	 (ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_VO| \
	 ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_VI	| \
	 ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_BK	| \
	 ATBM_IEEE80211_WMM_IE_STA_QOSINFO_AC_BE)
		
/*
 * WMM Information Element (used in (Re)Association Request frames; may also be
 * used in Beacon frames)
 */
struct wmm_information_element {
	/* Element ID: 221 (0xdd); Length: 7 */
	/* required fields for WMM version 1 */
	atbm_uint8 oui[3]; /* 00:50:f2 */
	atbm_uint8 oui_type; /* 2 */
	atbm_uint8 oui_subtype; /* 0 */
	atbm_uint8 version; /* 1 for WMM version 1.0 */
	atbm_uint8 qos_info; /* AP/STA specific QoS info */
} atbm_packed  ;



#define ATBM_WMM_AC_AIFSN_MASK 0x0f
#define ATBM_WMM_AC_AIFNS_SHIFT 0
#define ATBM_WMM_AC_ACM 0x10
#define ATBM_WMM_AC_ACI_MASK 0x60
#define ATBM_WMM_AC_ACI_SHIFT 5

#define ATBM_WMM_AC_ECWMIN_MASK 0x0f
#define ATBM_WMM_AC_ECWMIN_SHIFT 0
#define ATBM_WMM_AC_ECWMAX_MASK 0xf0
#define ATBM_WMM_AC_ECWMAX_SHIFT 4

struct wmm_ac_parameter {
	atbm_uint8 aci_aifsn; /* AIFSN, ACM, ACI */
	atbm_uint8 cw; /* ECWmin, ECWmax (CW = 2^ECW - 1) */
	atbm_uint16 txop_limit;
}  atbm_packed  ;

/*
 * WMM Parameter Element (used in Beacon, Probe Response, and (Re)Association
 * Response frmaes)
 */
struct wmm_parameter_element {
	/* Element ID: 221 (0xdd); Length: 24 */
	/* required fields for WMM version 1 */
	atbm_uint8 oui[3]; /* 00:50:f2 */
	atbm_uint8 oui_type; /* 2 */
	atbm_uint8 oui_subtype; /* 1 */
	atbm_uint8 version; /* 1 for WMM version 1.0 */
	atbm_uint8 qos_info; /* AP/STA specific QoS info */
	atbm_uint8 reserved; /* 0 */
	struct wmm_ac_parameter ac[4]; /* AC_BE, AC_BK, AC_VI, AC_VO */

} atbm_packed  ;



#define ATBM_IEEE80211_HT_CTL_LEN		4
#define ATBM_IEEE80211_HDRLEN		24


#define ATBM_NUM_RX_DATA_QUEUES	16
//#define USHRT_MAX      ((atbm_uint16)(~0U))
#define ATBM_IEEE80211_ADDR_LEN 6
//#define IEEE80211_FC0_VERSION_0 0x00
#define	ATBM_IEEE80211_ADDR_COPY(dst, src)	atbm_memcpy(dst, src, IEEE80211_ADDR_LEN)

struct atbmwifi_ieee80211_hdr {
	atbm_uint16 frame_control;
	atbm_uint16 duration_id;
	atbm_uint8 addr1[6];
	atbm_uint8 addr2[6];
	atbm_uint8 addr3[6];
	atbm_uint16 seq_ctrl;
	atbm_uint8 addr4[6];
}   atbm_packed ;

struct atbmwifi_ieee80211_hdr_3addr {
	atbm_uint16 frame_control;
	atbm_uint16 duration_id;
	atbm_uint8 addr1[6];
	atbm_uint8 addr2[6];
	atbm_uint8 addr3[6];
	atbm_uint16 seq_ctrl;
}  atbm_packed ;

struct atbmwifi_ieee80211_qos_hdr {
	atbm_uint16 frame_control;
	atbm_uint16 duration_id;
	atbm_uint8 addr1[6];
	atbm_uint8 addr2[6];
	atbm_uint8 addr3[6];
	atbm_uint16 seq_ctrl;
	atbm_uint16 qos_ctrl;
}  atbm_packed ;


/**
 * atbmwifi_ieee80211_has_tods - check if ATBM_IEEE80211_FCTL_TODS is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE int atbmwifi_ieee80211_has_tods(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_TODS)) != 0;
}

/**
 * atbmwifi_ieee80211_has_fromds - check if ATBM_IEEE80211_FCTL_FROMDS is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE int atbmwifi_ieee80211_has_fromds(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FROMDS)) != 0;
}

/**
 * atbmwifi_ieee80211_has_a4 - check if ATBM_IEEE80211_FCTL_TODS and ATBM_IEEE80211_FCTL_FROMDS are set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE int atbmwifi_ieee80211_has_a4(atbm_uint16 fc)
{
	atbm_uint16 tmp = atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_TODS | ATBM_IEEE80211_FCTL_FROMDS);
	return (fc & tmp) == tmp;
}

/**
 * atbmwifi_ieee80211_has_morefrags - check if ATBM_IEEE80211_FCTL_MOREFRAGS is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_has_morefrags(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_MOREFRAGS)) != 0;
}

/**
 * atbmwifi_ieee80211_has_retry - check if ATBM_IEEE80211_FCTL_RETRY is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_has_retry(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_RETRY)) != 0;
}

/**
 * atbmwifi_ieee80211_has_pm - check if ATBM_IEEE80211_FCTL_PM is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_has_pm(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_PM)) != 0;
}

/**
 * atbmwifi_ieee80211_has_moredata - check if ATBM_IEEE80211_FCTL_MOREDATA is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_has_moredata(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_MOREDATA)) != 0;
}

/**
 * atbmwifi_ieee80211_has_protected - check if ATBM_IEEE80211_FCTL_PROTECTED is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_has_protected(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_PROTECTED)) != 0;
}

/**
 * atbmwifi_ieee80211_has_order - check if ATBM_IEEE80211_FCTL_ORDER is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_has_order(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_ORDER)) != 0;
}

/**
 * atbmwifi_ieee80211_is_mgmt - check if type is ATBM_IEEE80211_FTYPE_MGMT
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_mgmt(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT);
}

/**
 * atbmwifi_ieee80211_is_ctl - check if type is ATBM_IEEE80211_FTYPE_CTL
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_ctl(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL);
}

/**
 * atbmwifi_ieee80211_is_data - check if type is ATBM_IEEE80211_FTYPE_DATA
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_data(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA);
}

/**
 * atbmwifi_ieee80211_is_data_qos - check if type is ATBM_IEEE80211_FTYPE_DATA and ATBM_IEEE80211_STYPE_QOS_DATA is set
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_data_qos(atbm_uint16 fc)
{
	/*
	 * mask with QOS_DATA rather than ATBM_IEEE80211_FCTL_STYPE as we just need
	 * to check the one bit
	 */
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_STYPE_QOS_DATA)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA | ATBM_IEEE80211_STYPE_QOS_DATA);
}

/**
 * atbmwifi_ieee80211_is_data_present - check if type is ATBM_IEEE80211_FTYPE_DATA and has data
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_data_present(atbm_uint16 fc)
{
	/*
	 * mask with 0x40 and test that that bit is clear to only return ATBM_TRUE
	 * for the data-containing substypes.
	 */
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | 0x40)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA);
}

/**
 * atbmwifi_ieee80211_is_assoc_req - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_ASSOC_REQ
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_assoc_req(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_ASSOC_REQ);
}

/**
 * atbmwifi_ieee80211_is_assoc_resp - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_ASSOC_RESP
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_assoc_resp(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_ASSOC_RESP);
}

/**
 * atbmwifi_ieee80211_is_reassoc_req - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_REASSOC_REQ
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_reassoc_req(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_REASSOC_REQ);
}

/**
 * atbmwifi_ieee80211_is_reassoc_resp - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_REASSOC_RESP
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_reassoc_resp(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_REASSOC_RESP);
}

/**
 * atbmwifi_ieee80211_is_probe_req - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_PROBE_REQ
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_probe_req(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_PROBE_REQ);
}

/**
 * atbmwifi_ieee80211_is_probe_resp - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_PROBE_RESP
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_probe_resp(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_PROBE_RESP);
}

/**
 * atbmwifi_ieee80211_is_beacon - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_BEACON
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_beacon(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_BEACON);
}

/**
 * atbmwifi_ieee80211_is_atim - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_ATIM
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_atim(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_ATIM);
}

/**
 * atbmwifi_ieee80211_is_disassoc - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_DISASSOC
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_disassoc(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_DISASSOC);
}

/**
 * atbmwifi_ieee80211_is_auth - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_AUTH
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_auth(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_AUTH);
}

/**
 * atbmwifi_atbmwifi_ieee80211_is_deauth - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_DEAUTH
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_deauth(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_DEAUTH);
}

/**
 * atbmwifi_ieee80211_is_action - check if ATBM_IEEE80211_FTYPE_MGMT && ATBM_IEEE80211_STYPE_ACTION
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_action(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_MGMT | ATBM_IEEE80211_STYPE_ACTION);
}

/**
 * atbmwifi_ieee80211_is_back_req - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_BACK_REQ
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_back_req(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_BACK_REQ);
}

/**
 * atbmwifi_ieee80211_is_back - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_BACK
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_back(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_BACK);
}

/**
 * atbmwifi_ieee80211_is_pspoll - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_PSPOLL
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_pspoll(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_PSPOLL);
}

/**
 * atbmwifi_ieee80211_is_rts - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_RTS
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_rts(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_RTS);
}

/**
 * atbmwifi_ieee80211_is_cts - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_CTS
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_cts(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_CTS);
}

/**
 * atbmwifi_ieee80211_is_ack - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_ACK
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_ack(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_ACK);
}

/**
 * atbmwifi_ieee80211_is_cfend - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_CFEND
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_cfend(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_CFEND);
}

/**
 * atbmwifi_ieee80211_is_cfendack - check if ATBM_IEEE80211_FTYPE_CTL && ATBM_IEEE80211_STYPE_CFENDACK
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_cfendack(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_CTL | ATBM_IEEE80211_STYPE_CFENDACK);
}

/**
 * atbmwifi_ieee80211_is_nullfunc - check if frame is a regular (non-QoS) nullfunc frame
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_nullfunc(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA | ATBM_IEEE80211_STYPE_NULLFUNC);
}

/**
 * atbmwifi_ieee80211_is_qos_nullfunc - check if frame is a QoS nullfunc frame
 * @fc: frame control bytes in little-endian byteorder
 */
static __INLINE  int atbmwifi_ieee80211_is_qos_nullfunc(atbm_uint16 fc)
{
	return (fc & atbm_cpu_to_le16(ATBM_IEEE80211_FCTL_FTYPE | ATBM_IEEE80211_FCTL_STYPE)) ==
	       atbm_cpu_to_le16(ATBM_IEEE80211_FTYPE_DATA | ATBM_IEEE80211_STYPE_QOS_NULLFUNC);
}
#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/bpstruct.h"
#endif //PACK_STRUCT_BEGIN

struct atbmwifi_ieee80211s_hdr {
	atbm_uint8 flags;
	atbm_uint8 ttl;
	atbm_uint32 seqnum;
	atbm_uint8 eaddr1[6];
	atbm_uint8 eaddr2[6];
}  atbm_packed ;


/**
 * struct atbmwifi_ieee80211_quiet_ie
 *
 * This structure refers to "Quiet information element"
 */
struct atbmwifi_ieee80211_quiet_ie {
	atbm_uint8 count;
	atbm_uint8 period;
	atbm_uint16 duration;
	atbm_uint16 offset;
}  atbm_packed ;

/**
 * struct atbmwifi_ieee80211_msrment_ie
 *
 * This structure refers to "Measurement Request/Report information element"
 */
struct atbmwifi_ieee80211_msrment_ie {
	atbm_uint8 token;
	atbm_uint8 mode;
	atbm_uint8 type;
	atbm_uint8 request[ZEROSIZE];
}  atbm_packed ;

/**
 * struct atbmwifi_ieee80211_channel_sw_ie
 *
 * This structure refers to "Channel Switch Announcement information element"
 */
struct atbmwifi_ieee80211_channel_sw_ie {
	atbm_uint8 mode;
	atbm_uint8 new_ch_num;
	atbm_uint8 count;
} atbm_packed;
struct atbmwifi_ieee80211_ext_chansw_ie{
	atbm_uint8 mode;
	atbm_uint8 new_operaring_class;
	atbm_uint8 new_ch_num;
	atbm_uint8 count;
}atbm_packed;

struct atbmwifi_ieee80211_sec_chan_offs_ie {
	atbm_uint8 sec_chan_offs;
}atbm_packed;


struct atbmwifi_ieee80211_channel_sw_packed_ie{
	struct atbmwifi_ieee80211_channel_sw_ie *chan_sw_ie;
	struct atbmwifi_ieee80211_ext_chansw_ie *ex_chan_sw_ie;
	struct atbmwifi_ieee80211_sec_chan_offs_ie *sec_chan_offs_ie;
}atbm_packed;


/* Public action codes */
enum atbm_ieee80211_pub_actioncode {
	ATBM_WLAN_PUB_ACTION_EX_CHL_SW_ANNOUNCE = 4,
};

/**
 * struct atbmwifi_ieee80211_tim
 *
 * This structure refers to "Traffic Indication Map information element"
 */

struct atbmwifi_ieee80211_tim_ie {
	atbm_uint8	tim_ie;			/* WIFI_ELEMID_TIM */
	atbm_uint8	tim_len;
	atbm_uint8	dtim_count;		/* DTIM count */
	atbm_uint8	dtim_period;		/* DTIM period */
	atbm_uint8	tim_bitmapctl;		/* bitmap control */
	atbm_uint8	tim_vbitmap[1];		/* variable-length bitmap */
}  atbm_packed ;

#define WLAN_SA_QUERY_TR_ID_LEN 2

struct atbmwifi_ieee80211_mgmt {
	atbm_uint16 frame_control;
	atbm_uint16 duration;
	atbm_uint8 da[6];
	atbm_uint8 sa[6];
	atbm_uint8 bssid[6];
	atbm_uint16 seq_ctrl;
	union {
		struct {
			atbm_uint16 auth_alg;
			atbm_uint16 auth_transaction;
			atbm_uint16 status_code;
			/* possibly followed by Challenge text */
			atbm_uint8 variable[ZEROSIZE];
		}   atbm_packed auth;
		struct {
			atbm_uint16 reason_code;
		}   atbm_packed deauth;
		struct {
			atbm_uint16 capab_info;
			atbm_uint16 listen_interval;
			/* followed by SSID and Supported rates */
			atbm_uint8 variable[ZEROSIZE];
		}   atbm_packed assoc_req;
		struct {
			atbm_uint16 capab_info;
			atbm_uint16 status_code;
			atbm_uint16 aid;
			/* followed by Supported rates */
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  assoc_resp, reassoc_resp;
		struct {
			atbm_uint16 capab_info;
			atbm_uint16 listen_interval;
			atbm_uint8 current_ap[6];
			/* followed by SSID and Supported rates */
			atbm_uint8 variable[ZEROSIZE];
		}   atbm_packed reassoc_req;
		struct {
			atbm_uint16 reason_code;
		}  atbm_packed  disassoc;
		struct {
			atbm_uint64 timestamp;
			atbm_uint16 beacon_int;
			atbm_uint16 capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params, TIM */
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  beacon ;
		struct {
			/* only variable items: SSID, Supported rates */
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  probe_req;
		struct {
			atbm_uint64 timestamp;
			atbm_uint16 beacon_int;
			atbm_uint16 capab_info;
			/* followed by some of SSID, Supported rates,
			 * FH Params, DS Params, CF Params, IBSS Params */
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  probe_resp;
		struct {
			atbm_uint8 category;
			union {
				struct {
					atbm_uint8 action_code;
					atbm_uint8 dialog_token;
					atbm_uint8 status_code;
					atbm_uint8 variable[ZEROSIZE];
				}   atbm_packed wme_action;
				struct{
					atbm_uint8 action_code;
					atbm_uint8 element_id;
					atbm_uint8 length;
					struct atbmwifi_ieee80211_channel_sw_ie sw_elem;
				}atbm_packed  chan_switch;				
				struct{
					atbm_uint8	action_code;
					struct atbmwifi_ieee80211_ext_chansw_ie ext_sw_elem;
					atbm_uint8 variable[ZEROSIZE];
				}atbm_packed ext_chan_switch;
				struct{
					atbm_uint8 action_code;
					atbm_uint8 chan_width;
				}atbm_packed notify_chan_width;
				struct{
					atbm_uint8 action_code;
					atbm_uint8 dialog_token;
					atbm_uint8 element_id;
					atbm_uint8 length;
					struct atbmwifi_ieee80211_msrment_ie msr_elem;
				}   atbm_packed measurement;
				struct{
					atbm_uint8 action_code;
					atbm_uint8 dialog_token;
					atbm_uint16 capab;
					atbm_uint16 timeout;
					atbm_uint16 start_seq_num;
				}   atbm_packed addba_req;
				struct{
					atbm_uint8 action_code;
					atbm_uint8 dialog_token;
					atbm_uint16 status;
					atbm_uint16 capab;
					atbm_uint16 timeout;
				}   atbm_packed addba_resp;
				struct{
					atbm_uint8 action_code;
					atbm_uint16 params;
					atbm_uint16 reason_code;
				}  atbm_packed  delba;
				struct {
					atbm_uint8 action_code;
					atbm_uint8 variable[ZEROSIZE];
				}   atbm_packed self_prot;
				struct {
					atbm_uint8 action;
					atbm_uint8 trans_id[WLAN_SA_QUERY_TR_ID_LEN];
				}   atbm_packed sa_query;
				struct {
					atbm_uint8 action;
					atbm_uint8 smps_control;
				}   atbm_packed ht_smps;
			}atbm_packed u;
		}  atbm_packed action;
	} atbm_packed u;
}atbm_packed ;


/* mgmt header + 1 byte category code */
#define ATBM_IEEE80211_MIN_ACTION_SIZE offsetof(struct atbmwifi_ieee80211_mgmt, u.action.u)


/* Management MIC information element (IEEE 802.11w) */
struct atbmwifi_ieee80211_mmie {
	atbm_uint8 element_id;
	atbm_uint8 length;
	atbm_uint16 key_id;
	atbm_uint8 sequence_number[6];
	atbm_uint8 mic[8];
} atbm_packed;

struct atbmwifi_ieee80211_vendor_ie {
	atbm_uint8 element_id;
	atbm_uint8 len;
	atbm_uint8 oui[3];
	atbm_uint8 oui_type;
} atbm_packed;

/* Control frames */
struct atbmwifi_ieee80211_rts {
	atbm_uint16 frame_control;
	atbm_uint16 duration;
	atbm_uint8 ra[6];
	atbm_uint8 ta[6];
} atbm_packed;

struct atbmwifi_ieee80211_cts {
	atbm_uint16 frame_control;
	atbm_uint16 duration;
	atbm_uint8 ra[6];
} ;

struct atbmwifi_ieee80211_pspoll {
	atbm_uint16 frame_control;
	atbm_uint16 aid;
	atbm_uint8 bssid[6];
	atbm_uint8 ta[6];
}  atbm_packed ;

/* TDLS */

/* Link-id information element */
struct atbmwifi_ieee80211_tdls_lnkie {
	atbm_uint8 ie_type; /* Link Identifier IE */
	atbm_uint8 ie_len;
	atbm_uint8 bssid[6];
	atbm_uint8 init_sta[6];
	atbm_uint8 resp_sta[6];
}  atbm_packed ;

struct atbmwifi_ieee80211_tdls_data {
	atbm_uint8 da[6];
	atbm_uint8 sa[6];
	atbm_uint16 ether_type;
	atbm_uint8 payload_type;
	atbm_uint8 category;
	atbm_uint8 action_code;
	union {
		struct {
			atbm_uint8 dialog_token;
			atbm_uint16 capability;
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  setup_req;
		struct {
			atbm_uint16 status_code;
			atbm_uint8 dialog_token;
			atbm_uint16 capability;
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  setup_resp;
		struct {
			atbm_uint16 status_code;
			atbm_uint8 dialog_token;
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  setup_cfm;
		struct {
			atbm_uint16 reason_code;
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  teardown;
		struct {
			atbm_uint8 dialog_token;
			atbm_uint8 variable[ZEROSIZE];
		}  atbm_packed  discover_req;
	}  atbm_packed u;
}  atbm_packed ;

/**
 * struct atbmwifi_ieee80211_bar - HT Block Ack Request
 *
 * This structure refers to "HT BlockAckReq" as
 * described in 802.11n draft section 7.2.1.7.1
 */
struct atbmwifi_ieee80211_bar {
	atbm_uint16 frame_control;
	atbm_uint16 duration;
	atbm_uint8 ra[6];
	atbm_uint8 ta[6];
	atbm_uint16 control;
	atbm_uint16 start_seq_num;
} atbm_packed  ;

/* 802.11 BAR control masks */
#define ATBM_IEEE80211_BAR_CTRL_ACK_POLICY_NORMAL	0x0000
#define ATBM_IEEE80211_BAR_CTRL_MULTI_TID		0x0002
#define ATBM_IEEE80211_BAR_CTRL_CBMTID_COMPRESSED_BA	0x0004
#define ATBM_IEEE80211_BAR_CTRL_TID_INFO_MASK	0xf000
#define ATBM_IEEE80211_BAR_CTRL_TID_INFO_SHIFT	12

#define IEEE80211_HT_MCS_MASK_LEN		10

enum atbm_nl80211_channel_type {
	ATBM_NL80211_CHAN_NO_HT,
	ATBM_NL80211_CHAN_HT20,
	ATBM_NL80211_CHAN_HT40MINUS, //second low //prim channel 5~11
	ATBM_NL80211_CHAN_HT40PLUS //second plus     //prim channel 1~9
};
#define atbmwifi_chtype_is_40M(_chtype) (((_chtype)==ATBM_NL80211_CHAN_HT40MINUS)||((_chtype)==ATBM_NL80211_CHAN_HT40PLUS))

/**
 * struct atbmwifi_ieee80211_mcs_info - MCS information
 * @rx_mask: RX mask
 * @rx_highest: highest supported RX rate. If set represents
 *	the highest supported RX data rate in units of 1 Mbps.
 *	If this field is 0 this value should not be used to
 *	consider the highest RX data rate supported.
 * @tx_params: TX parameters
 */
#if 1
struct atbmwifi_ieee80211_mcs_info {
	atbm_uint8 rx_mask[IEEE80211_HT_MCS_MASK_LEN];
	atbm_uint16 rx_highest;	
	atbm_uint8 tx_params;
	atbm_uint8 reserved[3];
}  atbm_packed ;

/* 802.11n HT capability MSC set */
#define ATBM_IEEE80211_HT_MCS_RX_HIGHEST_MASK	0xFFC0//0x3ff
#define ATBM_IEEE80211_HT_MCS_TX_DEFINED		0x80//0x01
#define ATBM_IEEE80211_HT_MCS_TX_RX_DIFF		0x40//0x02
/* value 0 == 1 stream etc */
#define ATBM_IEEE80211_HT_MCS_TX_MAX_STREAMS_MASK	0x30//0x0C
#define ATBM_IEEE80211_HT_MCS_TX_MAX_STREAMS_SHIFT	4///2
#define	ATBM_IEEE80211_HT_MCS_TX_MAX_STREAMS	4
#define ATBM_IEEE80211_HT_MCS_TX_UNEQUAL_MODULATION	0x08//0x10
#else
struct atbmwifi_ieee80211_mcs_info {
	atbm_uint8 rx_mask[IEEE80211_HT_MCS_MASK_LEN];
	atbm_uint8 tx_params;
	atbm_uint8 reserved[3];	
	atbm_uint16 rx_highest;
}  atbm_packed ;

/* 802.11n HT capability MSC set */
#define ATBM_IEEE80211_HT_MCS_RX_HIGHEST_MASK	0x3ff
#define ATBM_IEEE80211_HT_MCS_TX_DEFINED		0x01
#define ATBM_IEEE80211_HT_MCS_TX_RX_DIFF		0x02
/* value 0 == 1 stream etc */
#define ATBM_IEEE80211_HT_MCS_TX_MAX_STREAMS_MASK	0x0C
#define ATBM_IEEE80211_HT_MCS_TX_MAX_STREAMS_SHIFT	2
#define	ATBM_IEEE80211_HT_MCS_TX_MAX_STREAMS	4
#define ATBM_IEEE80211_HT_MCS_TX_UNEQUAL_MODULATION	0x10

#endif

/*
 * 802.11n D5.0 20.3.5 / 20.6 says:
 * - indices 0 to 7 and 32 are single spatial stream
 * - 8 to 31 are multiple spatial streams using equal modulation
 *   [8..15 for two streams, 16..23 for three and 24..31 for four]
 * - remainder are multiple spatial streams using unequal modulation
 */
#define ATBM_IEEE80211_HT_MCS_UNEQUAL_MODULATION_START 33
#define ATBM_IEEE80211_HT_MCS_UNEQUAL_MODULATION_START_BYTE \
	(ATBM_IEEE80211_HT_MCS_UNEQUAL_MODULATION_START / 8)

/**
 * struct atbmwifi_ieee80211_ht_cap - HT capabilities
 *
 * This structure is the "HT capabilities element" as
 * described in 802.11n D5.0 7.3.2.57
 */
struct atbmwifi_ieee80211_ht_cap {
	atbm_uint16 cap_info;
	atbm_uint8 ampdu_params_info;

	/* 16 bytes MCS information */
	struct atbmwifi_ieee80211_mcs_info mcs;

	atbm_uint16 extended_ht_cap_info;
	atbm_uint32 tx_BF_cap_info;
	atbm_uint8 antenna_selection_info;
}  atbm_packed ;

/* 802.11n HT capabilities masks (for cap_info) */
#define ATBM_IEEE80211_HT_CAP_LDPC_CODING		0x0001
#define ATBM_IEEE80211_HT_CAP_SUP_WIDTH_20_40	0x0002
#define ATBM_IEEE80211_HT_CAP_SM_PS			0x000C
#define		ATBM_IEEE80211_HT_CAP_SM_PS_SHIFT	2
#define ATBM_IEEE80211_HT_CAP_GRN_FLD		0x0010
#define ATBM_IEEE80211_HT_CAP_SGI_20			0x0020
#define ATBM_IEEE80211_HT_CAP_SGI_40			0x0040
#define ATBM_IEEE80211_HT_CAP_TX_STBC		0x0080
#define ATBM_IEEE80211_HT_CAP_RX_STBC		0x0300
#define		ATBM_IEEE80211_HT_CAP_RX_STBC_SHIFT	8
#define ATBM_IEEE80211_HT_CAP_DELAY_BA		0x0400
#define ATBM_IEEE80211_HT_CAP_MAX_AMSDU		0x0800
#define ATBM_IEEE80211_HT_CAP_DSSSCCK40		0x1000
#define ATBM_IEEE80211_HT_CAP_RESERVED		0x2000
#define ATBM_IEEE80211_HT_CAP_40MHZ_INTOLERANT	0x4000
#define ATBM_IEEE80211_HT_CAP_LSIG_TXOP_PROT		0x8000

/* 802.11n HT extended capabilities masks (for extended_ht_cap_info) */
#define ATBM_IEEE80211_HT_EXT_CAP_PCO		0x0001
#define ATBM_IEEE80211_HT_EXT_CAP_PCO_TIME		0x0006
#define		ATBM_IEEE80211_HT_EXT_CAP_PCO_TIME_SHIFT	1
#define ATBM_IEEE80211_HT_EXT_CAP_MCS_FB		0x0300
#define		ATBM_IEEE80211_HT_EXT_CAP_MCS_FB_SHIFT	8
#define ATBM_IEEE80211_HT_EXT_CAP_HTC_SUP		0x0400
#define ATBM_IEEE80211_HT_EXT_CAP_RD_RESPONDER	0x0800

/* 802.11n HT capability AMPDU settings (for ampdu_params_info) */
#define ATBM_IEEE80211_HT_AMPDU_PARM_FACTOR		0x03
#define ATBM_IEEE80211_HT_AMPDU_PARM_DENSITY		0x1C
#define		ATBM_IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT	2

/*
 * Maximum length of AMPDU that the STA can receive.
 * Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
 */
enum atbmwifi_ieee80211_max_ampdu_length_exp {
	ATBM_IEEE80211_HT_MAX_AMPDU_8K = 0,
	ATBM_IEEE80211_HT_MAX_AMPDU_16K = 1,
	ATBM_IEEE80211_HT_MAX_AMPDU_32K = 2,
	ATBM_IEEE80211_HT_MAX_AMPDU_64K = 3
};

#define ATBM_IEEE80211_HT_MAX_AMPDU_FACTOR 13

/* Minimum MPDU start spacing */
enum atbmwifi_ieee80211_min_mpdu_spacing {
	ATBM_IEEE80211_HT_MPDU_DENSITY_NONE = 0,	/* No restriction */
	ATBM_IEEE80211_HT_MPDU_DENSITY_0_25 = 1,	/* 1/4 usec */
	ATBM_IEEE80211_HT_MPDU_DENSITY_0_5 = 2,	/* 1/2 usec */
	ATBM_IEEE80211_HT_MPDU_DENSITY_1 = 3,	/* 1 usec */
	ATBM_IEEE80211_HT_MPDU_DENSITY_2 = 4,	/* 2 usec */
	ATBM_IEEE80211_HT_MPDU_DENSITY_4 = 5,	/* 4 usec */
	ATBM_IEEE80211_HT_MPDU_DENSITY_8 = 6,	/* 8 usec */
	ATBM_IEEE80211_HT_MPDU_DENSITY_16 = 7	/* 16 usec */
};

/**
 * struct atbmwifi_ieee80211_ht_info - HT information
 *
 * This structure is the "HT information element" as
 * described in 802.11n D5.0 7.3.2.58
 */
struct atbmwifi_ieee80211_ht_info {
	atbm_uint8 control_chan;
	atbm_uint8 ht_param;
	atbm_uint16 operation_mode;
	atbm_uint16 stbc_param;
	atbm_uint8 basic_set[16];
}  atbm_packed ;

/* for ht_param */
#define ATBM_IEEE80211_HT_PARAM_CHA_SEC_OFFSET		0x03
#define	ATBM_IEEE80211_HT_PARAM_CHA_SEC_NONE		0x00
#define	ATBM_IEEE80211_HT_PARAM_CHA_SEC_ABOVE	0x01
#define	ATBM_IEEE80211_HT_PARAM_CHA_SEC_BELOW	0x03
#define ATBM_IEEE80211_HT_PARAM_CHAN_WIDTH_ANY		0x04
#define ATBM_IEEE80211_HT_PARAM_RIFS_MODE			0x08
#define ATBM_IEEE80211_HT_PARAM_SPSMP_SUPPORT		0x10
#define ATBM_IEEE80211_HT_PARAM_SERV_INTERVAL_GRAN		0xE0

/* for operation_mode */
#define ATBM_IEEE80211_HT_OP_MODE_PROTECTION			0x0003
#define		ATBM_IEEE80211_HT_OP_MODE_PROTECTION_NONE		0
#define		ATBM_IEEE80211_HT_OP_MODE_PROTECTION_NONMEMBER	1
#define		ATBM_IEEE80211_HT_OP_MODE_PROTECTION_20MHZ		2
#define		ATBM_IEEE80211_HT_OP_MODE_PROTECTION_NONHT_MIXED	3
#define ATBM_IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT		0x0004
#define ATBM_IEEE80211_HT_OP_MODE_BURST_TX_LIMIT			0x0008
#define ATBM_IEEE80211_HT_OP_MODE_NON_HT_STA_PRSNT		0x0010

/* for stbc_param */
#define ATBM_IEEE80211_HT_STBC_PARAM_DUAL_BEACON		0x0040
#define ATBM_IEEE80211_HT_STBC_PARAM_DUAL_CTS_PROT		0x0080
#define ATBM_IEEE80211_HT_STBC_PARAM_STBC_BEACON		0x0100
#define ATBM_IEEE80211_HT_STBC_PARAM_LSIG_TXOP_FULLPROT	0x0200
#define ATBM_IEEE80211_HT_STBC_PARAM_PCO_ACTIVE		0x0400
#define ATBM_IEEE80211_HT_STBC_PARAM_PCO_PHASE		0x0800


/* block-ack parameters */
#define ATBM_IEEE80211_ADDBA_PARAM_POLICY_MASK 0x0002
#define ATBM_IEEE80211_ADDBA_PARAM_TID_MASK 0x003C
#define ATBM_IEEE80211_ADDBA_PARAM_BUF_SIZE_MASK 0xFFC0
#define ATBM_IEEE80211_DELBA_PARAM_TID_MASK 0xF000
#define ATBM_IEEE80211_DELBA_PARAM_INITIATOR_MASK 0x0800

/*
 * A-PMDU buffer sizes
 * According to IEEE802.11n spec size varies from 8K to 64K (in powers of 2)
 */
#define ATBM_IEEE80211_MIN_AMPDU_BUF 0x8
#define ATBM_IEEE80211_MAX_AMPDU_BUF 0x40


/* Spatial Multiplexing Power Save Modes (for capability) */
#define ATBM_WLAN_HT_CAP_SM_PS_STATIC	0
#define ATBM_WLAN_HT_CAP_SM_PS_DYNAMIC	1
#define ATBM_WLAN_HT_CAP_SM_PS_INVALID	2
#define ATBM_WLAN_HT_CAP_SM_PS_DISABLED	3

/* for SM power control field lower two bits */
#define ATBM_WLAN_HT_SMPS_CONTROL_DISABLED	0
#define ATBM_WLAN_HT_SMPS_CONTROL_STATIC	1
#define ATBM_WLAN_HT_SMPS_CONTROL_DYNAMIC	3

/* Authentication algorithms */
#define ATBM_WLAN_AUTH_OPEN 0
#define ATBM_WLAN_AUTH_SHARED_KEY 1
#define ATBM_WLAN_AUTH_FT 2
#define ATBM_WLAN_AUTH_SAE 3
#define ATBM_WLAN_AUTH_LEAP 128

#define ATBM_WLAN_AUTH_CHALLENGE_LEN 128

#define ATBM_WLAN_CAPABILITY_ESS		(1<<0)
#define ATBM_WLAN_CAPABILITY_IBSS		(1<<1)

/*
 * A mesh STA sets the ESS and IBSS capability bits to zero.
 * however, this holds ATBM_TRUE for p2p probe responses (in the p2p_find
 * phase) as well.
 */
#define ATBM_WLAN_CAPABILITY_IS_STA_BSS(cap)	\
	(!((cap) & (ATBM_WLAN_CAPABILITY_ESS | ATBM_WLAN_CAPABILITY_IBSS)))

#define ATBM_WLAN_CAPABILITY_CF_POLLABLE	(1<<2)
#define ATBM_WLAN_CAPABILITY_CF_POLL_REQUEST	(1<<3)
#define ATBM_WLAN_CAPABILITY_PRIVACY		(1<<4)
#define ATBM_WLAN_CAPABILITY_SHORT_PREAMBLE	(1<<5)
#define ATBM_WLAN_CAPABILITY_PBCC		(1<<6)
#define ATBM_WLAN_CAPABILITY_CHANNEL_AGILITY	(1<<7)

/* 802.11h */
#define ATBM_WLAN_CAPABILITY_SPECTRUM_MGMT	(1<<8)
#define ATBM_WLAN_CAPABILITY_QOS		(1<<9)
#define ATBM_WLAN_CAPABILITY_SHORT_SLOT_TIME	(1<<10)
#define ATBM_WLAN_CAPABILITY_DSSS_OFDM	(1<<13)
/* measurement */
#define ATBM_IEEE80211_SPCT_MSR_RPRT_MODE_LATE	(1<<0)
#define ATBM_IEEE80211_SPCT_MSR_RPRT_MODE_INCAPABLE	(1<<1)
#define ATBM_IEEE80211_SPCT_MSR_RPRT_MODE_REFUSED	(1<<2)

#define ATBM_IEEE80211_SPCT_MSR_RPRT_TYPE_BASIC	0
#define ATBM_IEEE80211_SPCT_MSR_RPRT_TYPE_CCA	1
#define ATBM_IEEE80211_SPCT_MSR_RPRT_TYPE_RPI	2


/* 802.11g ERP information element */
#define ATBM_WLAN_ERP_NON_ERP_PRESENT (1<<0)
#define ATBM_WLAN_ERP_USE_PROTECTION (1<<1)
#define ATBM_WLAN_ERP_BARKER_PREAMBLE (1<<2)

/* ATBM_WLAN_ERP_BARKER_PREAMBLE values */
enum {
	ATBM_WLAN_ERP_PREAMBLE_SHORT = 0,
	ATBM_WLAN_ERP_PREAMBLE_LONG = 1,
};

/* Status codes */
enum atbmwifi_ieee80211_statuscode {
	ATBM_WLAN_STATUS_SUCCESS_NEXTSETP = -99,
	ATBM_WLAN_STATUS_JUST_DROP = -98,
	ATBM_WLAN_STATUS_SUCCESS = 0,
	ATBM_WLAN_STATUS_UNSPECIFIED_FAILURE = 1,
	ATBM_WLAN_STATUS_CAPS_UNSUPPORTED = 10,
	ATBM_WLAN_STATUS_REASSOC_NO_ASSOC = 11,
	ATBM_WLAN_STATUS_ASSOC_DENIED_UNSPEC = 12,
	ATBM_WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG = 13,
	ATBM_WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION = 14,
	ATBM_WLAN_STATUS_CHALLENGE_FAIL = 15,
	ATBM_WLAN_STATUS_AUTH_TIMEOUT = 16,
	ATBM_WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA = 17,
	ATBM_WLAN_STATUS_ASSOC_DENIED_RATES = 18,
	/* 802.11b */
	ATBM_WLAN_STATUS_ASSOC_DENIED_NOSHORTPREAMBLE = 19,
	ATBM_WLAN_STATUS_ASSOC_DENIED_NOPBCC = 20,
	ATBM_WLAN_STATUS_ASSOC_DENIED_NOAGILITY = 21,
	/* 802.11h */
	ATBM_WLAN_STATUS_ASSOC_DENIED_NOSPECTRUM = 22,
	ATBM_WLAN_STATUS_ASSOC_REJECTED_BAD_POWER = 23,
	ATBM_WLAN_STATUS_ASSOC_REJECTED_BAD_SUPP_CHAN = 24,
	/* 802.11g */
	ATBM_WLAN_STATUS_ASSOC_DENIED_NOSHORTTIME = 25,
	ATBM_WLAN_STATUS_ASSOC_DENIED_NODSSSOFDM = 26,
	/* 802.11w */
	ATBM_WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY = 30,
	ATBM_WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION = 31,
	/* 802.11i */
	ATBM_WLAN_STATUS_INVALID_IE = 40,
	ATBM_WLAN_STATUS_INVALID_GROUP_CIPHER = 41,
	ATBM_WLAN_STATUS_INVALID_PAIRWISE_CIPHER = 42,
	ATBM_WLAN_STATUS_INVALID_AKMP = 43,
	ATBM_WLAN_STATUS_UNSUPP_RSN_VERSION = 44,
	ATBM_WLAN_STATUS_INVALID_RSN_IE_CAP = 45,
	ATBM_WLAN_STATUS_CIPHER_SUITE_REJECTED = 46,
	/* 802.11e */
	ATBM_WLAN_STATUS_UNSPECIFIED_QOS = 32,
	ATBM_WLAN_STATUS_ASSOC_DENIED_NOBANDWIDTH = 33,
	ATBM_WLAN_STATUS_ASSOC_DENIED_LOWACK = 34,
	ATBM_WLAN_STATUS_ASSOC_DENIED_UNSUPP_QOS = 35,
	ATBM_WLAN_STATUS_REQUEST_DECLINED = 37,
	ATBM_WLAN_STATUS_INVALID_QOS_PARAM = 38,
	ATBM_WLAN_STATUS_CHANGE_TSPEC = 39,
	ATBM_WLAN_STATUS_WAIT_TS_DELAY = 47,
	ATBM_WLAN_STATUS_NO_DIRECT_LINK = 48,
	ATBM_WLAN_STATUS_STA_NOT_PRESENT = 49,
	ATBM_WLAN_STATUS_STA_NOT_QSTA = 50,

	ATBM_WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE = 51,
	ATBM_WLAN_STATUS_INVALID_FT_ACTION_FRAME_COUNT = 52,
	ATBM_WLAN_STATUS_INVALID_PMKID = 53,
	ATBM_WLAN_STATUS_INVALID_MDIE = 54,
	ATBM_WLAN_STATUS_INVALID_FTIE = 55,
	ATBM_WLAN_STATUS_REQUESTED_TCLAS_NOT_SUPPORTED = 56,
	ATBM_WLAN_STATUS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES = 57,
	ATBM_WLAN_STATUS_TRY_ANOTHER_BSS = 58,
	ATBM_WLAN_STATUS_GAS_ADV_PROTO_NOT_SUPPORTED = 59,
	ATBM_WLAN_STATUS_NO_OUTSTANDING_GAS_REQ = 60,
	ATBM_WLAN_STATUS_GAS_RESP_NOT_RECEIVED = 61,
	ATBM_WLAN_STATUS_STA_TIMED_OUT_WAITING_FOR_GAS_RESP = 62,
	ATBM_WLAN_STATUS_GAS_RESP_LARGER_THAN_LIMIT = 63,
	ATBM_WLAN_STATUS_REQ_REFUSED_HOME = 64,
	ATBM_WLAN_STATUS_ADV_SRV_UNREACHABLE = 65,
	ATBM_WLAN_STATUS_REQ_REFUSED_SSPN = 67,
	ATBM_WLAN_STATUS_REQ_REFUSED_UNAUTH_ACCESS = 68,
	ATBM_WLAN_STATUS_INVALID_RSNIE = 72,
	ATBM_WLAN_STATUS_U_APSD_COEX_NOT_SUPPORTED = 73,
	ATBM_WLAN_STATUS_U_APSD_COEX_MODE_NOT_SUPPORTED = 74,
	ATBM_WLAN_STATUS_BAD_INTERVAL_WITH_U_APSD_COEX = 75,

	/* 802.11s */
	ATBM_WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ = 76,
	ATBM_WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED = 77,
	ATBM_WLAN_STATUS_FCG_NOT_SUPP = 78,
	ATBM_WLAN_STATUS_STA_NO_TBTT = 78,
	ATBM_WLAN_STATUS_TRANSMISSION_FAILURE = 79,
	ATBM_WLAN_STATUS_REQ_TCLAS_NOT_SUPPORTED = 80,
	ATBM_WLAN_STATUS_TCLAS_RESOURCES_EXCHAUSTED = 81,
	ATBM_WLAN_STATUS_REJECTED_WITH_SUGGESTED_BSS_TRANSITION = 82,
	ATBM_WLAN_STATUS_REJECT_WITH_SCHEDULE = 83,
	ATBM_WLAN_STATUS_REJECT_NO_WAKEUP_SPECIFIED = 84,
	ATBM_WLAN_STATUS_SUCCESS_POWER_SAVE_MODE = 85,
	ATBM_WLAN_STATUS_PENDING_ADMITTING_FST_SESSION = 86,
	ATBM_WLAN_STATUS_PERFORMING_FST_NOW = 87,
	ATBM_WLAN_STATUS_PENDING_GAP_IN_BA_WINDOW = 88,
	ATBM_WLAN_STATUS_REJECT_U_PID_SETTING = 89,
	ATBM_WLAN_STATUS_REFUSED_EXTERNAL_REASON = 92,
	ATBM_WLAN_STATUS_REFUSED_AP_OUT_OF_MEMORY = 93,
	ATBM_WLAN_STATUS_REJECTED_EMERGENCY_SERVICE_NOT_SUPPORTED = 94,
	ATBM_WLAN_STATUS_QUERY_RESP_OUTSTANDING = 95,
	ATBM_WLAN_STATUS_REJECT_DSE_BAND = 96,
	ATBM_WLAN_STATUS_TCLAS_PROCESSING_TERMINATED = 97,
	ATBM_WLAN_STATUS_TS_SCHEDULE_CONFLICT = 98,
	ATBM_WLAN_STATUS_DENIED_WITH_SUGGESTED_BAND_AND_CHANNEL = 99,
	ATBM_WLAN_STATUS_MCCAOP_RESERVATION_CONFLICT = 100,
	ATBM_WLAN_STATUS_MAF_LIMIT_EXCEEDED = 101,
	ATBM_WLAN_STATUS_MCCA_TRACK_LIMIT_EXCEEDED = 102,
	ATBM_WLAN_STATUS_DENIED_DUE_TO_SPECTRUM_MANAGEMENT = 103,
	ATBM_WLAN_STATUS_ASSOC_DENIED_NO_VHT = 104,
	ATBM_WLAN_STATUS_ENABLEMENT_DENIED = 105,
	ATBM_WLAN_STATUS_RESTRICTION_FROM_AUTHORIZED_GDB = 106,
	ATBM_WLAN_STATUS_AUTHORIZATION_DEENABLED = 107,
	ATBM_WLAN_STATUS_FILS_AUTHENTICATION_FAILURE = 112,
	ATBM_WLAN_STATUS_UNKNOWN_AUTHENTICATION_SERVER = 113,
	ATBM_WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER = 123,
};


/* Reason codes */
enum atbmwifi_ieee80211_reasoncode {
	ATBM_WLAN_REASON_UNSPECIFIED = 1,
	ATBM_WLAN_REASON_PREV_AUTH_NOT_VALID = 2,
	ATBM_WLAN_REASON_DEAUTH_LEAVING = 3,
	ATBM_WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY = 4,
	ATBM_WLAN_REASON_DISASSOC_AP_BUSY = 5,
	ATBM_WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA = 6,
	ATBM_WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA = 7,
	ATBM_WLAN_REASON_DISASSOC_STA_HAS_LEFT = 8,
	ATBM_WLAN_REASON_STA_REQ_ASSOC_WITHOUT_AUTH = 9,
	/* 802.11h */
	ATBM_WLAN_REASON_DISASSOC_BAD_POWER = 10,
	ATBM_WLAN_REASON_DISASSOC_BAD_SUPP_CHAN = 11,
	/* 802.11i */
	ATBM_WLAN_REASON_INVALID_IE = 13,
	ATBM_WLAN_REASON_MICHAEL_MIC_FAILURE = 14,
	ATBM_WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT = 15,
	ATBM_WLAN_REASON_GROUP_KEY_HANDSHAKE_TIMEOUT = 16,
	ATBM_WLAN_REASON_IE_DIFFERENT = 17,
	ATBM_WLAN_REASON_INVALID_GROUP_CIPHER = 18,
	ATBM_WLAN_REASON_INVALID_PAIRWISE_CIPHER = 19,
	ATBM_WLAN_REASON_INVALID_AKMP = 20,
	ATBM_WLAN_REASON_UNSUPP_RSN_VERSION = 21,
	ATBM_WLAN_REASON_INVALID_RSN_IE_CAP = 22,
	ATBM_WLAN_REASON_IEEE8021X_FAILED = 23,
	ATBM_WLAN_REASON_CIPHER_SUITE_REJECTED = 24,
	ATBM_WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE =  25,
	ATBM_WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED =  26,
	/* 802.11e */
	ATBM_WLAN_REASON_DISASSOC_UNSPECIFIED_QOS = 32,
	ATBM_WLAN_REASON_DISASSOC_QAP_NO_BANDWIDTH = 33,
	ATBM_WLAN_REASON_DISASSOC_LOW_ACK = 34,
	ATBM_WLAN_REASON_DISASSOC_QAP_EXCEED_TXOP = 35,
	ATBM_WLAN_REASON_QSTA_LEAVE_QBSS = 36,
	ATBM_WLAN_REASON_QSTA_NOT_USE = 37,
	ATBM_WLAN_REASON_QSTA_REQUIRE_SETUP = 38,
	ATBM_WLAN_REASON_QSTA_TIMEOUT = 39,
	ATBM_WLAN_REASON_QSTA_CIPHER_NOT_SUPP = 45,
	
};




/* Information Element IDs */
enum atbmwifi_ieee80211_eid {
	ATBM_WLAN_EID_SSID = 0,
	ATBM_WLAN_EID_SUPP_RATES = 1,
	ATBM_WLAN_EID_FH_PARAMS = 2,
	ATBM_WLAN_EID_DS_PARAMS = 3,
	ATBM_WLAN_EID_CF_PARAMS = 4,
	ATBM_WLAN_EID_TIM = 5,
	ATBM_WLAN_EID_IBSS_PARAMS = 6,
	ATBM_WLAN_EID_COUNTRY = 7,
	ATBM_WLAN_EID_HP_PARAMS = 8,
	ATBM_WLAN_EID_HP_TABLE = 9,
	ATBM_WLAN_EID_REQUEST = 10,
	ATBM_WLAN_EID_QBSS_LOAD = 11,
	ATBM_WLAN_EID_EDCA_PARAM_SET = 12,
	ATBM_WLAN_EID_TSPEC = 13,
	ATBM_WLAN_EID_TCLAS = 14,
	ATBM_WLAN_EID_SCHEDULE = 15,
	ATBM_WLAN_EID_CHALLENGE = 16,
	/* 802.11z */
	ATBM_WLAN_EID_PWR_CONSTRAINT = 32,
	ATBM_WLAN_EID_PWR_CAPABILITY = 33,
	ATBM_WLAN_EID_TPC_REQUEST = 34,
	ATBM_WLAN_EID_TPC_REPORT = 35,
	ATBM_WLAN_EID_SUPPORTED_CHANNELS = 36,
	ATBM_WLAN_EID_CHANNEL_SWITCH = 37,
	ATBM_WLAN_EID_MEASURE_REQUEST = 38,
	ATBM_WLAN_EID_MEASURE_REPORT = 39,
	ATBM_WLAN_EID_QUIET = 40,
	ATBM_WLAN_EID_IBSS_DFS = 41,
	ATBM_WLAN_EID_ERP_INFO = 42,
	ATBM_WLAN_EID_TS_DELAY = 43,
	ATBM_WLAN_EID_TCLAS_PROCESSING = 44,
	ATBM_WLAN_EID_HT_CAPABILITY = 45,
	ATBM_WLAN_EID_RSN = 48,
	ATBM_WLAN_EID_EXT_SUPP_RATES = 50,
	ATBM_WLAN_EID_AP_CHAN_REPORT = 51,
	ATBM_WLAN_EID_NEIGHBOR_REPORT = 52,
	ATBM_WLAN_EID_RCPI = 53,
	ATBM_WLAN_EID_MOBILITY_DOMAIN = 54,
	ATBM_WLAN_EID_FAST_BSS_TRANSITION = 55,
	ATBM_WLAN_EID_TIMEOUT_INTERVAL = 56,
	ATBM_WLAN_EID_RIC_DATA = 57,
	ATBM_WLAN_EID_DSE_REGISTERED_LOCATION = 58,
	ATBM_WLAN_EID_SUPPORTED_REGULATORY_CLASSES = 59,
	ATBM_WLAN_EID_EXT_CHANSWITCH_ANN = 60,
	ATBM_WLAN_EID_HT_OPERATION = 61,
	ATBM_WLAN_EID_SECONDARY_CH_OFFSET = 62,
	ATBM_WLAN_EID_BSS_AVERAGE_ACCESS_DELAY = 63,
	ATBM_WLAN_EID_ANTENNA = 64,
	ATBM_WLAN_EID_RSNI = 65,
	ATBM_WLAN_EID_MEASUREMENT_PILOT_TRANSMISSION = 66,
	ATBM_WLAN_EID_BSS_AVAILABLE_ADM_CAPA = 67,
	ATBM_WLAN_EID_BSS_AC_ACCESS_DELAY = 68, /* note: also used by WAPI */
	ATBM_WLAN_EID_TIME_ADVERTISEMENT = 69,
	ATBM_WLAN_EID_RRM_ENABLED_CAPABILITIES = 70,
	ATBM_WLAN_EID_MULTIPLE_BSSID = 71,
	ATBM_WLAN_EID_20_40_BSS_COEXISTENCE = 72,
	ATBM_WLAN_EID_20_40_BSS_INTOLERANT = 73,
	ATBM_WLAN_EID_OVERLAPPING_BSS_SCAN_PARAMS = 74,
	ATBM_WLAN_EID_RIC_DESCRIPTOR = 75,
	ATBM_WLAN_EID_MMIE = 76,
	ATBM_WLAN_EID_EVENT_REQUEST = 78,
	ATBM_WLAN_EID_EVENT_REPORT = 79,
	ATBM_WLAN_EID_DIAGNOSTIC_REQUEST = 80,
	ATBM_WLAN_EID_DIAGNOSTIC_REPORT = 81,
	ATBM_ATBM_WLAN_EID_LOCATION_PARAMETERS = 82,
	ATBM_WLAN_EID_NONTRANSMITTED_BSSID_CAPA = 83,
	ATBM_WLAN_EID_SSID_LIST = 84,
	ATBM_WLAN_EID_MULTIPLE_BSSID_INDEX = 85,
	ATBM_WLAN_EID_FMS_DESCRIPTOR = 86,
	ATBM_WLAN_EID_FMS_REQUEST = 87,
	ATBM_WLAN_EID_FMS_RESPONSE = 88,
	ATBM_WLAN_EID_QOS_TRAFFIC_CAPABILITY = 89,
	ATBM_WLAN_EID_BSS_MAX_IDLE_PERIOD = 90,
	ATBM_WLAN_EID_TFS_REQ = 91,
	ATBM_WLAN_EID_TFS_RESP = 92,
	ATBM_WLAN_EID_WNMSLEEP = 93,
	ATBM_WLAN_EID_TIM_BROADCAST_REQUEST = 94,
	ATBM_WLAN_EID_TIM_BROADCAST_RESPONSE = 95,
	ATBM_WLAN_EID_COLLOCATED_INTERFERENCE_REPORT = 96,
	ATBM_WLAN_EID_CHANNEL_USAGE = 97,
	ATBM_WLAN_EID_TIME_ZONE = 98,
	ATBM_WLAN_EID_DMS_REQUEST = 99,
	ATBM_WLAN_EID_DMS_RESPONSE = 100,
	ATBM_WLAN_EID_LINK_ID = 101,
	ATBM_WLAN_EID_WAKEUP_SCHEDULE = 102,
	ATBM_WLAN_EID_CHANNEL_SWITCH_TIMING = 104,
	ATBM_WLAN_EID_PTI_CONTROL = 105,
	ATBM_WLAN_EID_TPU_BUFFER_STATUS = 106,
	ATBM_WLAN_EID_INTERWORKING = 107,
	ATBM_WLAN_EID_ADV_PROTO = 108,
	ATBM_WLAN_EID_EXPEDITED_BANDWIDTH_REQ = 109,
	ATBM_WLAN_EID_QOS_MAP_SET = 110,
	ATBM_WLAN_EID_ROAMING_CONSORTIUM = 111,
	WLAN_EID_EMERGENCY_ALERT_ID = 112,
	ATBM_WLAN_EID_MESH_CONFIG = 113,
	ATBM_WLAN_EID_MESH_ID = 114,
	ATBM_WLAN_EID_MESH_LINK_METRIC_REPORT = 115,
	ATBM_WLAN_EID_CONGESTION_NOTIFICATION = 116,
	ATBM_WLAN_EID_PEER_MGMT = 117,
	ATBM_WLAN_EID_MESH_CHANNEL_SWITCH_PARAMETERS = 118,
	ATBM_WLAN_EID_MESH_AWAKE_WINDOW = 119,
	ATBM_WLAN_EID_BEACON_TIMING = 120,
	ATBM_WLAN_EID_MCCAOP_SETUP_REQUEST = 121,
	ATBM_WLAN_EID_MCCAOP_SETUP_REPLY = 122,
	ATBM_WLAN_EID_MCCAOP_ADVERTISEMENT = 123,
	ATBM_WLAN_EID_MCCAOP_TEARDOWN = 124,
	ATBM_WLAN_EID_GANN = 125,
	ATBM_WLAN_EID_RANN = 126,
	ATBM_WLAN_EID_EXT_CAPAB = 127,
	ATBM_WLAN_EID_PREQ = 130,
	ATBM_WLAN_EID_PREP = 131,
	ATBM_WLAN_EID_PERR = 132,
	ATBM_WLAN_EID_PXU = 137,
	ATBM_WLAN_EID_PXUC = 138,
	ATBM_WLAN_EID_AMPE = 139,
	ATBM_WLAN_EID_MIC = 140,
	ATBM_WLAN_EID_DESTINATION_URI = 141,
	ATBM_WLAN_EID_U_APSD_COEX = 142,
	ATBM_WLAN_EID_DMG_WAKEUP_SCHEDULE = 143,
	ATBM_WLAN_EID_EXTENDED_SCHEDULE = 144,
	ATBM_WLAN_EID_STA_AVAILABILITY = 145,
	ATBM_WLAN_EID_DMG_TSPEC = 146,
	ATBM_WLAN_EID_NEXT_DMG_ATI = 147,
	ATBM_WLAN_EID_DMG_CAPABILITIES = 148,
	ATBM_WLAN_EID_DMG_OPERATION = 151,
	ATBM_WLAN_EID_DMG_BSS_PARAMETER_CHANGE = 152,
	ATBM_WLAN_EID_DMG_BEAM_REFINEMENT = 153,
	ATBM_WLAN_EID_CHANNEL_MEASUREMENT_FEEDBACK = 154,
	ATBM_WLAN_EID_CCKM = 156,
	ATBM_WLAN_EID_AWAKE_WINDOW = 157,
	ATBM_WLAN_EID_MULTI_BAND = 158,
	ATBM_WLAN_EID_ADDBA_EXTENSION = 159,
	ATBM_WLAN_EID_NEXTPCP_LIST = 160,
	ATBM_WLAN_EID_PCP_HANDOVER = 161,
	ATBM_WLAN_EID_DMG_LINK_MARGIN = 162,
	ATBM_WLAN_EID_SWITCHING_STREAM = 163,
	ATBM_WLAN_EID_SESSION_TRANSITION = 164,
	ATBM_WLAN_EID_DYNAMIC_TONE_PAIRING_REPORT = 165,
	ATBM_WLAN_EID_CLUSTER_REPORT = 166,
	ATBM_WLAN_EID_REPLAY_CAPABILITIES = 167,
	ATBM_WLAN_EID_RELAY_TRANSFER_PARAM_SET = 168,
	ATBM_WLAN_EID_BEAMLINK_MAINTENANCE = 169,
	ATBM_WLAN_EID_MULTIPLE_MAC_SUBLAYERS = 170,
	ATBM_WLAN_EID_U_PID = 171,
	ATBM_WLAN_EID_DMG_LINK_ADAPTATION_ACK = 172,
	ATBM_WLAN_EID_MCCAOP_ADVERTISEMENT_OVERVIEW = 174,
	ATBM_WLAN_EID_QUIET_PERIOD_REQUEST = 175,
	ATBM_WLAN_EID_QUIET_PERIOD_RESPONSE = 177,
	ATBM_WLAN_EID_QMF_POLICY = 181,
	ATBM_WLAN_EID_ECAPC_POLICY = 182,
	ATBM_WLAN_EID_CLUSTER_TIME_OFFSET = 183,
	ATBM_WLAN_EID_INTRA_ACCESS_CATEGORY_PRIORITY = 184,
	ATBM_WLAN_EID_SCS_DESCRIPTOR = 185,
	ATBM_WLAN_EID_QLOAD_REPORT = 186,
	ATBM_WLAN_EID_HCCA_TXOP_UPDATE_COUNT = 187,
	ATBM_WLAN_EID_HIGHER_LAYER_STREAM_ID = 188,
	ATBM_WLAN_EID_GCR_GROUP_ADDRESS = 189,
	ATBM_WLAN_EID_ANTENNA_SECTOR_ID_PATTERN = 190,
	ATBM_WLAN_EID_VHT_CAP = 191,
	ATBM_WLAN_EID_VHT_OPERATION = 192,
	ATBM_WLAN_EID_VHT_EXTENDED_BSS_LOAD = 193,
	ATBM_WLAN_EID_VHT_WIDE_BW_CHSWITCH   = 194,
	ATBM_WLAN_EID_VHT_TRANSMIT_POWER_ENVELOPE = 195,
	ATBM_WLAN_EID_VHT_CHANNEL_SWITCH_WRAPPER = 196,
	ATBM_WLAN_EID_VHT_AID = 197,
	ATBM_WLAN_EID_VHT_QUIET_CHANNEL = 198,
	ATBM_WLAN_EID_VHT_OPERATING_MODE_NOTIFICATION = 199,
	ATBM_WLAN_EID_UPSIM = 200,
	ATBM_WLAN_EID_REDUCED_NEIGHBOR_REPORT = 201,
	ATBM_WLAN_EID_TVHT_OPERATION = 202,
	ATBM_WLAN_EID_DEVICE_LOCATION = 204,
	ATBM_WLAN_EID_WHITE_SPACE_MAP = 205,
	ATBM_WLAN_EID_FTM_PARAMETERS = 206,
	ATBM_WLAN_EID_VENDOR_SPECIFIC = 221,
	ATBM_WLAN_EID_CAG_NUMBER = 237,
	ATBM_WLAN_EID_AP_CSN = 239,
	ATBM_WLAN_EID_FILS_INDICATION = 240,
	ATBM_WLAN_EID_DILS = 241,
	ATBM_WLAN_EID_FRAGMENT = 242,
	ATBM_WLAN_EID_EXTENSION = 255,
};

/* Element ID Extension (EID 255) values */
#define ATBM_WLAN_EID_EXT_ASSOC_DELAY_INFO 1
#define ATBM_WLAN_EID_EXT_FILS_REQ_PARAMS 2
#define ATBM_WLAN_EID_EXT_FILS_KEY_CONFIRM 3
#define ATBM_WLAN_EID_EXT_FILS_SESSION 4
#define ATBM_WLAN_EID_EXT_FILS_HLP_CONTAINER 5
#define ATBM_WLAN_EID_EXT_FILS_IP_ADDR_ASSIGN 6
#define ATBM_WLAN_EID_EXT_KEY_DELIVERY 7
#define ATBM_WLAN_EID_EXT_FILS_WRAPPED_DATA 8
#define ATBM_WLAN_EID_EXT_FTM_SYNC_INFO 9
#define ATBM_WLAN_EID_EXT_EXTENDED_REQUEST 10
#define ATBM_WLAN_EID_EXT_ESTIMATED_SERVICE_PARAMS 11
#define ATBM_WLAN_EID_EXT_FILS_PUBLIC_KEY 12
#define ATBM_WLAN_EID_EXT_FILS_NONCE 13
#define ATBM_WLAN_EID_EXT_FUTURE_CHANNEL_GUIDANCE 14
#define ATBM_WLAN_EID_EXT_OWE_DH_PARAM 32
#define ATBM_WLAN_EID_EXT_PASSWORD_IDENTIFIER 33
#define ATBM_WLAN_EID_EXT_HE_CAPABILITIES 35
#define ATBM_WLAN_EID_EXT_HE_OPERATION 36
#define ATBM_WLAN_EID_EXT_HE_MU_EDCA_PARAMS 38
#define ATBM_WLAN_EID_EXT_SPATIAL_REUSE 39
#define ATBM_WLAN_EID_EXT_OCV_OCI 54


/* Action category code */
enum atbmwifi_ieee80211_category {
	ATBM_WLAN_CATEGORY_SPECTRUM_MGMT = 0,
	ATBM_WLAN_CATEGORY_QOS = 1,
	ATBM_WLAN_CATEGORY_DLS = 2,
	ATBM_WLAN_CATEGORY_BACK = 3,
	ATBM_WLAN_CATEGORY_PUBLIC = 4,
	ATBM_WLAN_CATEGORY_HT = 7,
	ATBM_WLAN_CATEGORY_SA_QUERY = 8,
	ATBM_WLAN_CATEGORY_PROTECTED_DUAL_OF_ACTION = 9,
	ATBM_WLAN_CATEGORY_TDLS = 12,
	ATBM_WLAN_CATEGORY_MESH_ACTION = 13,
	ATBM_WLAN_CATEGORY_MULTIHOP_ACTION = 14,
	ATBM_WLAN_CATEGORY_SELF_PROTECTED = 15,
	ATBM_WLAN_CATEGORY_WMM = 17,
	ATBM_WLAN_CATEGORY_VENDOR_SPECIFIC_PROTECTED = 126,
	ATBM_WLAN_CATEGORY_VENDOR_SPECIFIC = 127,

};

/* SPECTRUM_MGMT action code */
enum atbmwifi_ieee80211_spectrum_mgmt_actioncode {
	ATBM_WLAN_ACTION_SPCT_MSR_REQ = 0,	 
	ATBM_WLAN_ACTION_SPCT_MSR_RPRT = 1,  
	ATBM_WLAN_ACTION_SPCT_TPC_REQ = 2,	 
	ATBM_WLAN_ACTION_SPCT_TPC_RPRT = 3,  
	ATBM_WLAN_ACTION_SPCT_CHL_SWITCH = 4,
};

/* HT action codes */
enum atbmwifi_ieee80211_ht_actioncode {
	ATBM_WLAN_HT_ACTION_NOTIFY_CHANWIDTH = 0, 
	ATBM_WLAN_HT_ACTION_SMPS = 1,             
	ATBM_WLAN_HT_ACTION_PSMP = 2,             
	ATBM_WLAN_HT_ACTION_PCO_PHASE = 3,        
	ATBM_WLAN_HT_ACTION_CSI = 4,              
	ATBM_WLAN_HT_ACTION_NONCOMPRESSED_BF = 5, 
	ATBM_WLAN_HT_ACTION_COMPRESSED_BF = 6,    
	ATBM_WLAN_HT_ACTION_ASEL_IDX_FEEDBACK = 7,
};

/* Self Protected Action codes */
enum atbmwifi_ieee80211_self_protected_actioncode {
	ATBM_WLAN_SP_RESERVED = 0,
	ATBM_WLAN_SP_MESH_PEERING_OPEN = 1,
	ATBM_WLAN_SP_MESH_PEERING_CONFIRM = 2,
	ATBM_WLAN_SP_MESH_PEERING_CLOSE = 3,
	ATBM_WLAN_SP_MGK_INFORM = 4,
	ATBM_WLAN_SP_MGK_ACK = 5,
};



/* Security key length */
enum atbmwifi_ieee80211_key_len {
	ATBM_WLAN_KEY_LEN_WEP40 = 5,
	ATBM_WLAN_KEY_LEN_WEP104 = 13,
	ATBM_WLAN_KEY_LEN_CCMP = 16,
	ATBM_WLAN_KEY_LEN_TKIP = 32,
	ATBM_WLAN_KEY_LEN_AES_CMAC = 16,
	ATBM_WLAN_KEY_LEN_SMS4 = 32,
};

/* Public action codes */
enum atbmwifi_ieee80211_pub_actioncode {
	ATBM_WLAN_PUB_ACTION_TDLS_DISCOVER_RES = 14,
};


/*
 * TDLS capabililites to be enabled in the 5th byte of the
 * @WLAN_EID_EXT_CAPABILITY information element
 */
#define ATBM_WLAN_EXT_CAPA5_TDLS_ENABLED	BIT(5)
#define ATBM_WLAN_EXT_CAPA5_TDLS_PROHIBITED	BIT(6)

/* TDLS specific payload type in the LLC/SNAP header */
#define ATBM_WLAN_TDLS_SNAP_RFTYPE	0x2




/*
 * IEEE 802.11-2007 7.3.2.9 Country information element
 *
 * Minimum length is 8 octets, ie len must be evenly
 * divisible by 2
 */

/* Although the spec says 8 I'm seeing 6 in practice */
#define ATBM_IEEE80211_COUNTRY_IE_MIN_LEN	6

/* The Country String field of the element shall be 3 octets in length */
#define ATBM_IEEE80211_COUNTRY_STRING_LEN	3

/*
 * For regulatory extension stuff see IEEE 802.11-2007
 * Annex I (page 1141) and Annex J (page 1147). Also
 * review 7.3.2.9.
 *
 * When dot11RegulatoryClassesRequired is ATBM_TRUE and the
 * first_channel/reg_extension_id is >= 201 then the IE
 * compromises of the 'ext' struct represented below:
 *
 *  - Regulatory extension ID - when generating IE this just needs
 *    to be monotonically increasing for each triplet passed in
 *    the IE
 *  - Regulatory class - index into set of rules
 *  - Coverage class - index into air propagation time (Table 7-27),
 *    in microseconds, you can compute the air propagation time from
 *    the index by multiplying by 3, so index 10 yields a propagation
 *    of 10 us. Valid values are 0-31, values 32-255 are not defined
 *    yet. A value of 0 inicates air propagation of <= 1 us.
 *
 *  See also Table I.2 for Emission limit sets and table
 *  I.3 for Behavior limit sets. Table J.1 indicates how to map
 *  a reg_class to an emission limit set and behavior limit set.
 */
#define ATBM_IEEE80211_COUNTRY_EXTENSION_ID 201

/*
 *  Channels numbers in the IE must be monotonically increasing
 *  if dot11RegulatoryClassesRequired is not ATBM_TRUE.
 *
 *  If dot11RegulatoryClassesRequired is ATBM_TRUE consecutive
 *  subband triplets following a regulatory triplet shall
 *  have monotonically increasing first_channel number fields.
 *
 *  Channel numbers shall not overlap.
 *
 *  Note that max_power is signed.
 */
#ifndef LINUX_OS
#pragma anon_unions
#endif
struct atbmwifi_ieee80211_country_ie_triplet {
	union {
		struct {
			atbm_uint8 first_channel;
			atbm_uint8 num_channels;
			atbm_int8 max_power;
		}  atbm_packed  chans;
		struct {
			atbm_uint8 reg_extension_id;
			atbm_uint8 reg_class;
			atbm_uint8 coverage_class;
		}  atbm_packed  ext;
	}atbm_packed;
}  atbm_packed ;

enum atbmwifi_ieee80211_timeout_interval_type {
	ATBM_WLAN_TIMEOUT_REASSOC_DEADLINE = 1 /* 802.11r */,
	ATBM_WLAN_TIMEOUT_KEY_LIFETIME = 2 /* 802.11r */,
	ATBM_WLAN_TIMEOUT_ASSOC_COMEBACK = 3 /* 802.11w */,
};

/* BACK action code */
enum atbmwifi_ieee80211_back_actioncode {
	ATBM_WLAN_ACTION_ADDBA_REQ = 0,
	ATBM_WLAN_ACTION_ADDBA_RESP = 1,
	ATBM_WLAN_ACTION_DELBA = 2,
};

/* BACK (block-ack) parties */
enum atbmwifi_ieee80211_back_parties {
	ATBM_WLAN_BACK_RECIPIENT = 0,
	ATBM_WLAN_BACK_INITIATOR = 1,
};

/* SA Query action */
enum atbmwifi_ieee80211_sa_query_action {
	ATBM_WLAN_ACTION_SA_QUERY_REQUEST = 0,
	ATBM_WLAN_ACTION_SA_QUERY_RESPONSE = 1,
};


/* cipher suite selectors */
#define ATBM_WLAN_CIPHER_SUITE_USE_GROUP	0x000FAC00
#define ATBM_WLAN_CIPHER_SUITE_WEP40		0x000FAC01
#define ATBM_WLAN_CIPHER_SUITE_TKIP		0x000FAC02
/* reserved: 				0x000FAC03 */
#define ATBM_WLAN_CIPHER_SUITE_CCMP		0x000FAC04
#define ATBM_WLAN_CIPHER_SUITE_WEP104	0x000FAC05
#define ATBM_WLAN_CIPHER_SUITE_AES_CMAC	0x000FAC06
#define ATBM_WLAN_CIPHER_SUITE_SMS4		0x000FAC07

/* AKM suite selectors */
#define ATBM_WLAN_AKM_SUITE_8021X		0x000FAC01
#define ATBM_WLAN_AKM_SUITE_PSK		0x000FAC02
#define ATBM_WLAN_AKM_SUITE_SAE			0x000FAC08
#define ATBM_WLAN_AKM_SUITE_FT_OVER_SAE	0x000FAC09
#define ATBM_WLAN_AKM_SUITE_WAPI_PSK		0x000FAC03

#define ATBM_WLAN_MAX_KEY_LEN		32

#define ATBM_WLAN_PMKID_LEN			16

#define ATBM_WLAN_OUI_WFA			0x506f9a
#define ATBM_WLAN_OUI_TYPE_WFA_P2P		9

/*
 * WMM/802.11e Tspec Element
 */
#define ATBM_IEEE80211_WMM_IE_TSPEC_TID_MASK		0x0F
#define ATBM_IEEE80211_WMM_IE_TSPEC_TID_SHIFT	1

enum atbmwifi_ieee80211_tspec_status_code {
	ATBM_IEEE80211_TSPEC_STATUS_ADMISS_ACCEPTED = 0,
	ATBM_IEEE80211_TSPEC_STATUS_ADDTS_INVAL_PARAMS = 0x1,
};

struct atbmwifi_ieee80211_tspec_ie {
	atbm_uint8 element_id;
	atbm_uint8 len;
	atbm_uint8 oui[3];
	atbm_uint8 oui_type;
	atbm_uint8 oui_subtype;
	atbm_uint8 version;
	atbm_uint16 tsinfo;
	atbm_uint8 tsinfo_resvd;
	atbm_uint16 nominal_msdu;
	atbm_uint16 max_msdu;
	atbm_uint32 min_service_int;
	atbm_uint32 max_service_int;
	atbm_uint32 inactivity_int;
	atbm_uint32 suspension_int;
	atbm_uint32 service_start_time;
	atbm_uint32 min_data_rate;
	atbm_uint32 mean_data_rate;
	atbm_uint32 peak_data_rate;
	atbm_uint32 max_burst_size;
	atbm_uint32 delay_bound;
	atbm_uint32 min_phy_rate;
	atbm_uint16 sba;
	atbm_uint16 medium_time;
}  atbm_packed ;

#define ATBM_TIM_BASE_SIZE       			3
#define ATBM_TIM_BITCTL_MCAST       		1
#define ATBM_TIM_BITCTL_UCAST_MASK 		((atbm_uint8)~(ATBM_TIM_BITCTL_MCAST))
#define ATBM_TIM_BITCTL_UCAST_SHIFT 	1
#define ATBM_ADD_TIM_BIT     				0
#define ATBM_REMOVE_TIM_BIT     			1


#define ATBM_OUI_MICROSOFT 0x0050f2 /* Microsoft (also used in Wi-Fi specs)*/
#define ATBM_WMM_OUI_TYPE  2
#define ATBM_WMM_OUI_SUBTYPE_INFORMATION_ELEMENT 0
#define ATBM_WMM_OUI_SUBTYPE_PARAMETER_ELEMENT 1
#define ATBM_WMM_OUI_SUBTYPE_TSPEC_ELEMENT 2

#define ATBM_OUI_WFA 0x506f9a
#define ATBM_P2P_IE_VENDOR_TYPE 0x506f9a09
#define ATBM_WFD_IE_VENDOR_TYPE 0x506f9a0a
#define ATBM_WFD_OUI_TYPE 10
#define ATBM_HS20_IE_VENDOR_TYPE 0x506f9a10
#define ATBM_HS20_INDICATION_OUI_TYPE 16

#define ATBM_P2P_OUI_TYPE 9
#define ATBM_OUI_BROADCOM 0x00904c /* Broadcom (Epigram) */

#define ATBM_OUI_QCA 0x001374

#define ATBM_WPA_SELECTOR_LEN 4
#define ATBM_WPA_VERSION 1
#define ATBM_RSN_SELECTOR_LEN 4
#define ATBM_RSN_VERSION 1


/* Parsed Information Elements */
struct atbmwifi_ieee802_11_elems {
	//atbm_uint8 *ie_start;
	//atbm_size_t total_len;

	/* pointers to IEs */
	atbm_uint8 *ssid;
	atbm_uint8 *supp_rates;
	//atbm_uint8 *fh_params;
	atbm_uint8 *ds_params;
	//atbm_uint8 *cf_params;
	struct atbmwifi_ieee80211_tim_ie *tim;
	//atbm_uint8 *ibss_params;
	atbm_uint8 *challenge;
	atbm_uint8 *wpa;
	atbm_uint8 *rsn;
	atbm_uint8 *erp_info;
	atbm_uint8 *ext_supp_rates;
	atbm_uint8 *wmm_info;
	atbm_uint8 *wmm_param;
#if CONFIG_WPS
	atbm_uint8 *wps_ie;
	atbm_uint8 *wps_ie2;
#endif
#if CONFIG_P2P
	atbm_uint8 *p2p_ie;
	atbm_uint8 *pref_freq_list;
#ifdef CONFIG_WIFI_DISPLAY
	const atbm_uint8 *wfd;
#endif
#endif
	struct atbmwifi_ieee80211_ht_cap *ht_cap_elem;
	struct atbmwifi_ieee80211_ht_info *ht_info_elem;
	//atbm_uint8 *peering;
	//atbm_uint8 *preq;
	//atbm_uint8 *prep;
	//atbm_uint8 *perr;
	atbm_uint8 *ch_switch_elem;
	atbm_uint8 *country_elem;
	atbm_uint8 *extended_ch_switch_elem;
	atbm_uint8 * secondary_ch_elem;
	//atbm_uint8 *pwr_constr_elem;
	//atbm_uint8 *quiet_elem;	/* first quite element */
	atbm_uint8 *timeout_int;

	/* length of them, respectively */
	atbm_uint8 ssid_len;
	atbm_uint8 supp_rates_len;
	//atbm_uint8 fh_params_len;
	atbm_uint8 ds_params_len;
	//atbm_uint8 cf_params_len;
	atbm_uint8 tim_len;
	atbm_uint8 challenge_len;
	atbm_uint8 wpa_len;
	atbm_uint8 rsn_len;
	atbm_uint8 erp_info_len;
	atbm_uint8 ext_supp_rates_len;
	atbm_uint8 wmm_info_len;
	atbm_uint8 wmm_param_len;
	//atbm_uint8 p2p_ie_len;
#if CONFIG_WPS
	atbm_uint8 wps_ie_len;
	atbm_uint8 wps_ie_len2;
#endif
#if CONFIG_P2P
	atbm_uint8 p2p_ie_len;
	atbm_uint8 pref_freq_list_len;
#endif
	//atbm_uint8 bcm_ie_len;
	//atbm_uint8 peering_len;
	//atbm_uint8 preq_len;
	//atbm_uint8 prep_len;
	//atbm_uint8 perr_len;
	atbm_uint8 ch_switch_elem_len;
	atbm_uint8 country_elem_len;	
	atbm_uint8 extended_ch_switch_elem_len;
	atbm_uint8 secondary_ch_elem_len;
	//atbm_uint8 pwr_constr_elem_len;
	//atbm_uint8 quiet_elem_len;
	//atbm_uint8 num_of_quiet_elem;	/* can be more the one */
	atbm_uint8 timeout_int_len;

};

enum ieee80211_ble_msg_type {
	BLE_MSG_TYPE_UNKOWN,
	BLE_MSG_TYPE_ACK,
	BLE_MSG_TYPE_EVT,
};
struct ieee80211_ble_status {
	enum ieee80211_ble_msg_type msg_type;
};

enum ieee80211_ble_thread_action{
	THREAD_ACTION_WAKEUP,
	THREAD_ACTION_SHOULD_SUSPEND,
	THREAD_ACTION_SUSPENED,
	THREAD_ACTION_SHOULD_STOP,
};

struct ieee80211_ble_thread
{
	const char *name;
	atbm_thread_internal_t *thread;
	unsigned long			flags;
	unsigned long           wakeup_period;
	int (*thread_fn)(struct ieee80211_ble_thread *thread);
	int (*period_handle)(struct ieee80211_ble_thread *thread);
	struct atbmwifi_common	*hw_priv;
	atbm_os_wait_queue_head_t wq;
	int prio;
};

struct ieee80211_ble_local{
	ATBM_BOOL ble_started;
	atbm_spinlock_t  ble_spin_lock;
	atbm_mutex	ble_mutex_lock;
	int  (*ble_recv_callback)(struct atbmwifi_common, atbm_uint8* buff, atbm_size_t buff_size, enum ieee80211_ble_msg_type msg_type);
	struct atbm_buff_head ble_xmit_queue;
	struct atbm_buff_head ble_recv_queue;
	struct ieee80211_ble_thread recv_thread;
	struct ieee80211_ble_thread xmit_thread;
};

struct  ieee80211_ble_buff{
	struct atbm_buff *skb;
	atbm_uint8 mem[0] __attribute__((__aligned__(64)));
};

/**
 * atbmwifi_ieee80211_get_qos_ctl - get pointer to qos control bytes
 * @hdr: the frame
 *
 * The qos ctrl bytes come after the frame_control, duration, seq_num
 * and 3 or 4 addresses of length ATBM_ETH_ALEN.
 * 3 addr: 2 + 2 + 2 + 3*6 = 24
 * 4 addr: 2 + 2 + 2 + 4*6 = 30
 */
static __INLINE atbm_uint8 *atbmwifi_ieee80211_get_qos_ctl(struct atbmwifi_ieee80211_hdr *hdr)
{
	if (atbmwifi_ieee80211_has_a4(hdr->frame_control))
		return (atbm_uint8 *)hdr + 30;
	else
		return (atbm_uint8 *)hdr + 24;
}

/**
 * atbmwifi_ieee80211_get_SA - get pointer to SA
 * @hdr: the frame
 *
 * Given an 802.11 frame, this function returns the offset
 * to the source address (SA). It does not verify that the
 * header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
static __INLINE atbm_uint8 *atbmwifi_ieee80211_get_SA(struct atbmwifi_ieee80211_hdr *hdr)
{
	if (atbmwifi_ieee80211_has_a4(hdr->frame_control))
		return hdr->addr4;
	if (atbmwifi_ieee80211_has_fromds(hdr->frame_control))
		return hdr->addr3;
	return hdr->addr2;
}

/**
 * atbmwifi_ieee80211_get_DA - get pointer to DA
 * @hdr: the frame
 *
 * Given an 802.11 frame, this function returns the offset
 * to the destination address (DA). It does not verify that
 * the header is long enough to contain the address, and the
 * header must be long enough to contain the frame control
 * field.
 */
static __INLINE atbm_uint8 *atbmwifi_ieee80211_get_DA(struct atbmwifi_ieee80211_hdr *hdr)
{
	if (atbmwifi_ieee80211_has_tods(hdr->frame_control))
		return hdr->addr3;
	else
		return hdr->addr1;
}

/**
 * atbmwifi_ieee80211_is_robust_mgmt_frame - check if frame is a robust management frame
 * @hdr: the frame (buffer must include at least the first octet of payload)
 */
static __INLINE ATBM_BOOL atbmwifi_ieee80211_is_robust_mgmt_frame(struct atbmwifi_ieee80211_hdr *hdr)
{
	if (atbmwifi_ieee80211_is_disassoc(hdr->frame_control) ||
	    atbmwifi_ieee80211_is_deauth(hdr->frame_control))
		return ATBM_TRUE;

	if (atbmwifi_ieee80211_is_action(hdr->frame_control)) {
		atbm_uint8 *category;

		/*
		 * Action frames, excluding Public Action frames, are Robust
		 * Management Frames. However, if we are looking at a Protected
		 * frame, skip the check since the data may be encrypted and
		 * the frame has already been found to be a Robust Management
		 * Frame (by the other end).
		 */
		if (atbmwifi_ieee80211_has_protected(hdr->frame_control))
			return ATBM_TRUE;
		category = ((atbm_uint8 *) hdr) + 24;
		return *category != ATBM_WLAN_CATEGORY_PUBLIC &&
			*category != ATBM_WLAN_CATEGORY_HT &&
			*category != ATBM_WLAN_CATEGORY_SELF_PROTECTED &&
			*category != ATBM_WLAN_CATEGORY_VENDOR_SPECIFIC;
	}

	return ATBM_FALSE;
}

/**
 * atbmwifi_ieee80211_fhss_chan_to_freq - get channel frequency
 * @channel: the FHSS channel
 *
 * Convert IEEE802.11 FHSS channel to frequency (MHz)
 * Ref IEEE 802.11-2007 section 14.6
 */
static __INLINE int atbmwifi_ieee80211_fhss_chan_to_freq(int channel)
{
	if ((channel > 1) && (channel < 96))
		return channel + 2400;
	else
		return -1;
}

/**
 * atbmwifi_ieee80211_freq_to_fhss_chan - get channel
 * @freq: the channels frequency
 *
 * Convert frequency (MHz) to IEEE802.11 FHSS channel
 * Ref IEEE 802.11-2007 section 14.6
 */
static __INLINE int atbmwifi_ieee80211_freq_to_fhss_chan(int freq)
{
	if ((freq > 2401) && (freq < 2496))
		return freq - 2400;
	else
		return -1;
}

/**
 * atbmwifi_ieee80211_dsss_chan_to_freq - get channel center frequency
 * @channel: the DSSS channel
 *
 * Convert IEEE802.11 DSSS channel to the center frequency (MHz).
 * Ref IEEE 802.11-2007 section 15.6
 */
static __INLINE int atbmwifi_ieee80211_dsss_chan_to_freq(int channel)
{
	if ((channel > 0) && (channel < 14))
		return 2407 + (channel * 5);
	else if (channel == 14)
		return 2484;
	else
		return -1;
}

/**
 * atbmwifi_ieee80211_freq_to_dsss_chan - get channel
 * @freq: the frequency
 *
 * Convert frequency (MHz) to IEEE802.11 DSSS channel
 * Ref IEEE 802.11-2007 section 15.6
 *
 * This routine selects the channel with the closest center frequency.
 */
static __INLINE int atbmwifi_ieee80211_freq_to_dsss_chan(int freq)
{
	if ((freq >= 2410) && (freq < 2475))
		return (freq - 2405) / 5;
	else if ((freq >= 2482) && (freq < 2487))
		return 14;
	else
		return -1;
}

/* Convert IEEE802.11 HR DSSS channel to frequency (MHz) and back
 * Ref IEEE 802.11-2007 section 18.4.6.2
 *
 * The channels and frequencies are the same as those defined for DSSS
 */
#define atbmwifi_ieee80211_hr_chan_to_freq(chan) atbmwifi_ieee80211_dsss_chan_to_freq(chan)
#define atbmwifi_ieee80211_freq_to_hr_chan(freq) atbmwifi_ieee80211_freq_to_dsss_chan(freq)

/* Convert IEEE802.11 ERP channel to frequency (MHz) and back
 * Ref IEEE 802.11-2007 section 19.4.2
 */
#define atbmwifi_ieee80211_erp_chan_to_freq(chan) atbmwifi_ieee80211_hr_chan_to_freq(chan)
#define atbmwifi_ieee80211_freq_to_erp_chan(freq) atbmwifi_ieee80211_freq_to_hr_chan(freq)

/**
 * atbmwifi_ieee80211_ofdm_chan_to_freq - get channel center frequency
 * @s_freq: starting frequency == (dotChannelStartingFactor/2) MHz
 * @channel: the OFDM channel
 *
 * Convert IEEE802.11 OFDM channel to center frequency (MHz)
 * Ref IEEE 802.11-2007 section 17.3.8.3.2
 */
static __INLINE  int atbmwifi_ieee80211_ofdm_chan_to_freq(int s_freq, int channel)
{
	if ((channel > 0) && (channel <= 200) &&
	    (s_freq >= 4000))
		return s_freq + (channel * 5);
	else
		return -1;
}

/**
 * atbmwifi_ieee80211_freq_to_ofdm_channel - get channel
 * @s_freq: starting frequency == (dotChannelStartingFactor/2) MHz
 * @freq: the frequency
 *
 * Convert frequency (MHz) to IEEE802.11 OFDM channel
 * Ref IEEE 802.11-2007 section 17.3.8.3.2
 *
 * This routine selects the channel with the closest center frequency.
 */
static __INLINE int atbmwifi_ieee80211_freq_to_ofdm_chan(int s_freq, int freq)
{
	if ((freq > (s_freq + 2)) && (freq <= (s_freq + 1202)) &&
	    (s_freq >= 4000))
		return (freq + 2 - s_freq) / 5;
	else
		return -1;
}

/**
 * atbmwifi_ieee80211_tu_to_usec - convert time units (TU) to microseconds
 * @tu: the TUs
 */
static __INLINE unsigned long atbmwifi_ieee80211_tu_to_usec(unsigned long tu)
{
	return 1024 * tu;
}


/**
 * atbmwifi_ieee80211_is_public_action - check if frame is a public action frame
 * @hdr: the frame
 * @len: length of the frame
 */
static __INLINE ATBM_BOOL atbmwifi_ieee80211_is_public_action(struct atbmwifi_ieee80211_hdr *hdr,
					      atbm_size_t len)
{
	struct atbmwifi_ieee80211_mgmt *mgmt = (atbm_void *)hdr;

	if (len < ATBM_IEEE80211_MIN_ACTION_SIZE)
		return ATBM_FALSE;
	if (!atbmwifi_ieee80211_is_action(hdr->frame_control))
		return ATBM_FALSE;
	return mgmt->u.action.category == ATBM_WLAN_CATEGORY_PUBLIC;
}


#endif //ATBMWIFI_PROTO_H
