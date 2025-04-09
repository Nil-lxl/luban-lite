/**************************************************************************************************************
 * altobeam RTOS WSM host interface (HI) implementation
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef ATBMWIFI__WSM_H_INCLUDED
#define ATBMWIFI__WSM_H_INCLUDED

#include "atbm_hal.h"

#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/bpstruct.h"
#endif //PACK_STRUCT_BEGIN

struct atbmwifi_common;
struct atbmwifi_vif;
struct atbm_buff;

#define OUT_BH 1

#define RECOVERY_STEP1_SUCCESS 1
#define RECOVERY_STEP2_SUCCESS 2
#define RECOVERY_ERR 			-1

/* Bands */
/* Radio band 2.412 -2.484 GHz. */
#define WSM_PHY_BAND_2_4G		(0)

/* Radio band 4.9375-5.8250 GHz. */
#define WSM_PHY_BAND_5G			(1)

/* Transmit rates */
/* 1   Mbps            ERP-DSSS */
#define WSM_TRANSMIT_RATE_1		(0)

/* 2   Mbps            ERP-DSSS */
#define WSM_TRANSMIT_RATE_2		(1)

/* 5.5 Mbps            ERP-CCK, ERP-PBCC (Not supported) */
#define WSM_TRANSMIT_RATE_5		(2) 

/* 11  Mbps            ERP-CCK, ERP-PBCC (Not supported) */
 #define WSM_TRANSMIT_RATE_11		(3) 

/* 22  Mbps            ERP-PBCC (Not supported) */
/* #define WSM_TRANSMIT_RATE_22		(4) */

/* 33  Mbps            ERP-PBCC (Not supported) */
/* #define WSM_TRANSMIT_RATE_33		(5) */

/* 6   Mbps   (3 Mbps) ERP-OFDM, BPSK coding rate 1/2 */
#define WSM_TRANSMIT_RATE_6		(6)

/* 9   Mbps (4.5 Mbps) ERP-OFDM, BPSK coding rate 3/4 */
#define WSM_TRANSMIT_RATE_9		(7)

/* 12  Mbps  (6 Mbps)  ERP-OFDM, QPSK coding rate 1/2 */
#define WSM_TRANSMIT_RATE_12		(8)

/* 18  Mbps  (9 Mbps)  ERP-OFDM, QPSK coding rate 3/4 */
#define WSM_TRANSMIT_RATE_18		(9)

/* 24  Mbps (12 Mbps)  ERP-OFDM, 16QAM coding rate 1/2 */
#define WSM_TRANSMIT_RATE_24		(10)

/* 36  Mbps (18 Mbps)  ERP-OFDM, 16QAM coding rate 3/4 */
#define WSM_TRANSMIT_RATE_36		(11)

/* 48  Mbps (24 Mbps)  ERP-OFDM, 64QAM coding rate 1/2 */
#define WSM_TRANSMIT_RATE_48		(12)

/* 54  Mbps (27 Mbps)  ERP-OFDM, 64QAM coding rate 3/4 */
#define WSM_TRANSMIT_RATE_54		(13)

/* 6.5 Mbps            HT-OFDM, BPSK coding rate 1/2 */
#define WSM_TRANSMIT_RATE_HT_6		(14)

/* 13  Mbps            HT-OFDM, QPSK coding rate 1/2 */
#define WSM_TRANSMIT_RATE_HT_13		(15)

/* 19.5 Mbps           HT-OFDM, QPSK coding rate 3/4 */
#define WSM_TRANSMIT_RATE_HT_19		(16)

/* 26  Mbps            HT-OFDM, 16QAM coding rate 1/2 */
#define WSM_TRANSMIT_RATE_HT_26		(17)

/* 39  Mbps            HT-OFDM, 16QAM coding rate 3/4 */
#define WSM_TRANSMIT_RATE_HT_39		(18)

/* 52  Mbps            HT-OFDM, 64QAM coding rate 2/3 */
#define WSM_TRANSMIT_RATE_HT_52		(19)

/* 58.5 Mbps           HT-OFDM, 64QAM coding rate 3/4 */
#define WSM_TRANSMIT_RATE_HT_58		(20)

/* 65  Mbps            HT-OFDM, 64QAM coding rate 5/6 */
#define WSM_TRANSMIT_RATE_HT_65		(21)

/* Scan types */
/* Foreground scan */
#define WSM_SCAN_TYPE_FOREGROUND	(0)

/* Background scan */
#define WSM_SCAN_TYPE_BACKGROUND	(1)

/* Auto scan */
#define WSM_SCAN_TYPE_AUTO		(2)

/* Scan flags */
/* Forced background scan means if the station cannot */
/* enter the power-save mode, it shall force to perform a */
/* background scan. Only valid when ScanType is */
/* background scan. */
#define WSM_SCAN_FLAG_FORCE_BACKGROUND	(BIT(0))

/* The WLAN device scans one channel at a time so */
/* that disturbance to the data traffic is minimized. */
#define WSM_SCAN_FLAG_SPLIT_METHOD	(BIT(1))

/* Preamble Type. Long if not set. */
#define WSM_SCAN_FLAG_SHORT_PREAMBLE	(BIT(2))

/* 11n Tx Mode. Mixed if not set. */
#define WSM_SCAN_FLAG_11N_GREENFIELD	(BIT(3))

#define WSM_FLAG_MAC_INSTANCE_1	(BIT(4))
#define WSM_FLAG_JOIN_F_FORCE_JOIN       (BIT(2))            

#define WSM_FLAG_MAC_INSTANCE_0	(~(BIT(4)))

#define WSM_FLAG_START_SMARTCONFIG		 (BIT(5))

#define WSM_FLAG_AP_BEST_CHANNEL    (BIT(6))

/* Scan constraints */
/* Maximum number of channels to be scanned. */
#define WSM_SCAN_MAX_NUM_OF_CHANNELS	(14)

/* The maximum number of SSIDs that the device can scan for. */
#define WSM_SCAN_MAX_NUM_OF_SSIDS	(1)
#ifdef CONFIG_ATBMWIFI__TESTMODE
/* Transmit flags */
/* Start Expiry time from the receipt of tx request */
#define WSM_TX_FLAG_EXPIRY_TIME		(BIT(0))
#endif /*CONFIG_ATBMWIFI__TESTMODE*/

/* Power management modes */
/* 802.11 Active mode */
#define WSM_PSM_ACTIVE			(0)

/* 802.11 PS mode */
#define WSM_PSM_PS			BIT(0)

/* Fast Power Save bit */
#define WSM_PSM_FAST_PS_FLAG		BIT(7)

/* Dynamic aka Fast power save */
#define WSM_PSM_FAST_PS			(BIT(0) | BIT(7))

/* Undetermined */
/* Note : Undetermined status is reported when the */
/* NULL data frame used to advertise the PM mode to */
/* the AP at Pre or Post Background Scan is not Acknowledged */
#define WSM_PSM_UNKNOWN			BIT(1)

/* Queue IDs */
/* best effort/legacy */
#define WSM_QUEUE_BEST_EFFORT		(0)

/* background */
#define WSM_QUEUE_BACKGROUND		(1)

/* video */
#define WSM_QUEUE_VIDEO			(2)

/* voice */
#define WSM_QUEUE_VOICE			(3)

/* HT TX parameters */
/* Non-HT */
#define WSM_HT_TX_NON_HT		(0)

/* Mixed format */
#define WSM_HT_TX_MIXED			BIT(0)

/* Greenfield format */
#define WSM_HT_TX_GREENFIELD	BIT(1)
/* short gi */
#define WSM_HT_TX_SGI			BIT(2)
#define WSM_HT_TX_WIDTH_40M			(BIT(3))
#define WSM_HT_TX_LONG_PREAMBLE			(BIT(4))
/* STBC allowed */
#define WSM_HT_TX_STBC			(BIT(7))
#define WSM_HT_TX_NEED_CONFIRM	(BIT(8)) //LMAC_TX_NEED_CONFIRM
#define WSM_HT_AGGR_EN			(BIT(9))
#define WSM_HT_NEED_SEQ			(BIT(10))

/* EPTA prioirty flags for BT Coex */
/* default epta priority */
#define WSM_EPTA_PRIORITY_DEFAULT	4
/* use for normal data */
#define WSM_EPTA_PRIORITY_DATA		4
/* use for connect/disconnect/roaming*/
#define WSM_EPTA_PRIORITY_MGT		5
/* use for action frames */
#define WSM_EPTA_PRIORITY_ACTION	5
/* use for AC_VI data */
#define WSM_EPTA_PRIORITY_VIDEO		5
/* use for AC_VO data */
#define WSM_EPTA_PRIORITY_VOICE		6
/* use for EAPOL exchange */
#define WSM_EPTA_PRIORITY_EAPOL		7

/* TX status */
/* Frame was sent aggregated */
/* Only valid for WSM_SUCCESS status. */
#define WSM_TX_STATUS_AGGREGATION	(BIT(0))

/* Host should requeue this frame later. */
/* Valid only when status is WSM_REQUEUE. */
#define WSM_TX_STATUS_REQUEUE		(BIT(1))

/* Normal Ack */
#define WSM_TX_STATUS_NORMAL_ACK	(0<<2)

/* No Ack */
#define WSM_TX_STATUS_NO_ACK		(1<<2)

/* No explicit acknowledgement */
#define WSM_TX_STATUS_NO_EXPLICIT_ACK	(2<<2)

/* Block Ack */
/* Only valid for WSM_SUCCESS status. */
#define WSM_TX_STATUS_BLOCK_ACK		(3<<2)

/* RX status */
/* Unencrypted */
#define WSM_RX_STATUS_UNENCRYPTED	(0<<0)

/* WEP */
#define WSM_RX_STATUS_WEP		(1<<0)

/* TKIP */
#define WSM_RX_STATUS_TKIP		(2<<0)

/* AES */
#define WSM_RX_STATUS_AES		(3<<0)

/* WAPI */
#define WSM_RX_STATUS_WAPI		(4<<0)

/* Macro to fetch encryption subfield. */
#define WSM_RX_STATUS_ENCRYPTION(status) ((status) & 0x07)

/* Frame was part of an aggregation */
#define WSM_RX_STATUS_AGGREGATE		(BIT(3))

/* Frame was first in the aggregation */
#define WSM_RX_STATUS_AGGREGATE_FIRST	(BIT(4))

/* Frame was last in the aggregation */
#define WSM_RX_STATUS_AGGREGATE_LAST	(BIT(5))

/* Indicates a defragmented frame */
#define WSM_RX_STATUS_DEFRAGMENTED	(BIT(6))

/* Indicates a Beacon frame */
#define WSM_RX_STATUS_BEACON		(BIT(7))

/* Indicates STA bit beacon TIM field */
#define WSM_RX_STATUS_TIM		(BIT(8))

/* Indicates Beacon frame's virtual bitmap contains multicast bit */
#define WSM_RX_STATUS_MULTICAST		(BIT(9))

/* Indicates frame contains a matching SSID */
#define WSM_RX_STATUS_MATCHING_SSID	(BIT(10))

/* Indicates frame contains a matching BSSI */
#define WSM_RX_STATUS_MATCHING_BSSI	(BIT(11))

/* Indicates SMARTCONFIG*/
#define WSM_RX_STATUS_SMARTCONFIG		(BIT(12))

/* Indicates frame received during a measurement process */
#define WSM_RX_STATUS_MEASUREMENT	(BIT(13))

/* Indicates frame received as an HT packet */
#define WSM_RX_STATUS_HT		(BIT(14))

/* Indicates frame received with STBC */
#define WSM_RX_STATUS_STBC		(BIT(15))

/* Indicates Address 1 field matches dot11StationId */
#define WSM_RX_STATUS_ADDRESS1		(BIT(16))

/* Indicates Group address present in the Address 1 field */
#define WSM_RX_STATUS_GROUP		(BIT(17))

/* Indicates Broadcast address present in the Address 1 field */
#define WSM_RX_STATUS_BROADCAST		(BIT(18))

/* Indicates group key used with encrypted frames */
#define WSM_RX_STATUS_GROUP_KEY		(BIT(19))
/*indicates rx 40M*/
#define WSM_RX_STATUS_RX_40M		(BIT(22))
/* Macro to fetch encryption key index. */
#define WSM_RX_STATUS_KEY_IDX(status)	(((status >> 20)) & 0x0F)
#define WSM_RX_STATUS_SHORT_GI      (BIT(24))

/* Frame Control field starts at Frame offset + 2 */
#define WSM_TX_2BYTES_SHIFT		(BIT(7))

/* Join mode */
/* IBSS */
#define WSM_JOIN_MODE_IBSS		(0)

/* BSS */
#define WSM_JOIN_MODE_BSS		(1)

/* PLCP preamble type */
/* For long preamble */
#define WSM_JOIN_PREAMBLE_LONG		(0)

/* For short preamble (Long for 1Mbps) */
#define WSM_JOIN_PREAMBLE_SHORT		(1)

/* For short preamble (Long for 1 and 2Mbps) */
#define WSM_JOIN_PREAMBLE_SHORT_2	(2)

/* Join flags */
/* Unsynchronized */
#define WSM_JOIN_FLAGS_UNSYNCRONIZED	BIT(0)
/* The BSS owner is a P2P GO */
#define WSM_JOIN_FLAGS_P2P_GO		BIT(1)
/* Force to join BSS with the BSSID and the
 * SSID specified without waiting for beacons. The
 * ProbeForJoin parameter is ignored. */
#define WSM_JOIN_FLAGS_FORCE		BIT(2)
/* Give probe request/response higher
 * priority over the BT traffic */
#define WSM_JOIN_FLAGS_PRIO		BIT(3)

/* Key types */
#define WSM_KEY_TYPE_WEP_DEFAULT	(0)
#define WSM_KEY_TYPE_WEP_PAIRWISE	(1)
#define WSM_KEY_TYPE_TKIP_GROUP		(2)
#define WSM_KEY_TYPE_TKIP_PAIRWISE	(3)
#define WSM_KEY_TYPE_AES_GROUP		(4)
#define WSM_KEY_TYPE_AES_PAIRWISE	(5)
#define WSM_KEY_TYPE_WAPI_GROUP		(6)
#define WSM_KEY_TYPE_WAPI_PAIRWISE	(7)
#define WSM_KEY_TYPE_IGTK_GROUP		 8

/* Key indexes */
#define ATBMWIFI_WSM_KEY_MAX_INDEX	(15)
#define WSM_KEY_MAX_INDEX		  ((ATBMWIFI__MAX_STA_IN_AP_MODE+1) +4)  // MAX_KEY_ENTRIES  

/* ACK policy */
#define WSM_ACK_POLICY_NORMAL		(0)
#define WSM_ACK_POLICY_NO_ACK		(1)

/* Start modes */
#define WSM_START_MODE_AP		(0)	/* Mini AP */
#define WSM_START_MODE_P2P_GO		(1)	/* P2P GO */
#define WSM_START_MODE_P2P_DEV		(2)	/* P2P device */

/* SetAssociationMode MIB flags */
#define WSM_ASSOCIATION_MODE_USE_PREAMBLE_TYPE		(BIT(0))
#define WSM_ASSOCIATION_MODE_USE_HT_MODE		(BIT(1))
#define WSM_ASSOCIATION_MODE_USE_BASIC_RATE_SET		(BIT(2))
#define WSM_ASSOCIATION_MODE_USE_MPDU_START_SPACING	(BIT(3))
#define WSM_ASSOCIATION_MODE_SNOOP_ASSOC_FRAMES		(BIT(4))

/* RcpiRssiThreshold MIB flags */
#define WSM_RCPI_RSSI_THRESHOLD_ENABLE	(BIT(0))
#define WSM_RCPI_RSSI_USE_RSSI		(BIT(1))
#define WSM_RCPI_RSSI_DONT_USE_UPPER	(BIT(2))
#define WSM_RCPI_RSSI_DONT_USE_LOWER	(BIT(3))

/* Update-ie constants */
#define WSM_UPDATE_IE_BEACON		(BIT(0))
#define WSM_UPDATE_IE_PROBE_RESP	(BIT(1))
#define WSM_UPDATE_IE_PROBE_REQ		(BIT(2))

/* WSM events */
/* Error */
#define WSM_EVENT_ERROR			(0)

/* BSS lost */
#define WSM_EVENT_BSS_LOST		(1)

/* BSS regained */
#define WSM_EVENT_BSS_REGAINED		(2)

/* Radar detected */
#define WSM_EVENT_RADAR_DETECTED	(3)

/* RCPI or RSSI threshold triggered */
#define WSM_EVENT_RCPI_RSSI		(4)

/* BT inactive */
#define WSM_EVENT_BT_INACTIVE		(5)

/* BT active */
#define WSM_EVENT_BT_ACTIVE		(6)

#define WSM_EVENT_PS_MODE_ERROR         (7)

#define WSM_EVENT_INACTIVITY		(9)

/* MAC Addr Filter */
#define WSM_MIB_ID_MAC_ADDR_FILTER	0x1030

/* MIB IDs */
/* 4.1  dot11StationId */
#define WSM_MIB_ID_DOT11_STATION_ID		0x0000

/* 4.2  dot11MaxtransmitMsduLifeTime */
#define WSM_MIB_ID_DOT11_MAX_TRANSMIT_LIFTIME	0x0001

/* 4.3  dot11MaxReceiveLifeTime */
#define WSM_MIB_ID_DOT11_MAX_RECEIVE_LIFETIME	0x0002

/* 4.4  dot11SlotTime */
#define WSM_MIB_ID_DOT11_SLOT_TIME		0x0003

/* 4.5  dot11GroupAddressesTable */
#define WSM_MIB_ID_DOT11_GROUP_ADDRESSES_TABLE	0x0004
#define WSM_MAX_GRP_ADDRTABLE_ENTRIES		8

/* 4.6  dot11WepDefaultKeyId */
#define WSM_MIB_ID_DOT11_WEP_DEFAULT_KEY_ID	0x0005

/* 4.7  dot11CurrentTxPowerLevel */
#define WSM_MIB_ID_DOT11_CURRENT_TX_POWER_LEVEL	0x0006

/* 4.8  dot11RTSThreshold */
#define WSM_MIB_ID_DOT11_RTS_THRESHOLD		0x0007

#define WSM_MIB_ID_DOT11_TRANSMIT_RETRY_CNT		0x0008
/* 4.9  NonErpProtection */
#define WSM_MIB_ID_NON_ERP_PROTECTION		0x1000

/* 4.10 ArpIpAddressesTable */
#define WSM_MIB_ID_ARP_IP_ADDRESSES_TABLE	0x1001
#define WSM_MAX_ARP_IP_ADDRTABLE_ENTRIES	1

/* 4.11 TemplateFrame */
#define WSM_MIB_ID_TEMPLATE_FRAME		0x1002

/* 4.12 RxFilter */
#define WSM_MIB_ID_RX_FILTER			0x1003

/* 4.13 BeaconFilterTable */
#define WSM_MIB_ID_BEACON_FILTER_TABLE		0x1004

/* 4.14 BeaconFilterEnable */
#define WSM_MIB_ID_BEACON_FILTER_ENABLE		0x1005

/* 4.15 OperationalPowerMode */
#define WSM_MIB_ID_OPERATIONAL_POWER_MODE	0x1006

/* 4.16 BeaconWakeUpPeriod */
#define WSM_MIB_ID_BEACON_WAKEUP_PERIOD		0x1007

/* 4.17 RcpiRssiThreshold */
#define WSM_MIB_ID_RCPI_RSSI_THRESHOLD		0x1009

/* 4.18 StatisticsTable */
#define WSM_MIB_ID_STATISTICS_TABLE		0x100A

/* 4.19 IbssPsConfig */
#define WSM_MIB_ID_IBSS_PS_CONFIG		0x100B

/* 4.20 CountersTable */
#define WSM_MIB_ID_COUNTERS_TABLE		0x100C

/* 4.21 BlockAckPolicy */
#define WSM_MIB_ID_BLOCK_ACK_POLICY		0x100E

/* 4.22 OverrideInternalTxRate */
#define WSM_MIB_ID_OVERRIDE_INTERNAL_TX_RATE	0x100F

/* 4.23 SetAssociationMode */
#define WSM_MIB_ID_SET_ASSOCIATION_MODE		0x1010

/* 4.24 UpdateEptaConfigData */
#define WSM_MIB_ID_UPDATE_EPTA_CONFIG_DATA	0x1011

/* 4.25 SelectCcaMethod */
#define WSM_MIB_ID_SELECT_CCA_METHOD		0x1012

/* 4.26 SetUpasdInformation */
#define WSM_MIB_ID_SET_UAPSD_INFORMATION	0x1013

/* 4.27 SetAutoCalibrationMode  WBF00004073 */
#define WSM_MIB_ID_SET_AUTO_CALIBRATION_MODE	0x1015

/* 4.28 SetTxRateRetryPolicy */
#define WSM_MIB_ID_SET_TX_RATE_RETRY_POLICY	0x1016

/* 4.29 SetHostMessageTypeFilter */
#define WSM_MIB_ID_SET_HOST_MSG_TYPE_FILTER	0x1017

/* 4.30 P2PFindInfo */
#define WSM_MIB_ID_P2P_FIND_INFO		0x1018

/* 4.31 P2PPsModeInfo */
#define WSM_MIB_ID_P2P_PS_MODE_INFO		0x1019

/* 4.32 SetEtherTypeDataFrameFilter */
#define WSM_MIB_ID_SET_ETHERTYPE_DATAFRAME_FILTER 0x101A

/* 4.33 SetUDPPortDataFrameFilter */
#define WSM_MIB_ID_SET_UDPPORT_DATAFRAME_FILTER	0x101B

/* 4.34 SetMagicDataFrameFilter */
#define WSM_MIB_ID_SET_MAGIC_DATAFRAME_FILTER	0x101C

/* This is the end of specification. */

/* 4.35 P2PDeviceInfo */
#define WSM_MIB_ID_P2P_DEVICE_INFO		0x101D

/* 4.36 SetWCDMABand */
#define WSM_MIB_ID_SET_WCDMA_BAND		0x101E

/* 4.37 GroupTxSequenceCounter */
#define WSM_MIB_ID_GRP_SEQ_COUNTER		0x101F

/* 4.38 ProtectedMgmtPolicy */
#define WSM_MIB_ID_PROTECTED_MGMT_POLICY	0x1020

/* 4.39 SetHtProtection */
#define WSM_MIB_ID_SET_HT_PROTECTION		0x1021

/* 4.40 GPIO Command */
#define WSM_MIB_ID_GPIO_COMMAND			0x1022

/* 4.41 TSF Counter Value */
#define WSM_MIB_ID_TSF_COUNTER			0x1023

/* Test Purposes Only */
#define WSM_MIB_ID_BLOCK_ACK_INFO		0x100D

/* 4.42 UseMultiTxConfMessage */
#define WSM_MIB_USE_MULTI_TX_CONF		0x1024

/* 4.43 Keep-alive period */
#define WSM_MIB_ID_KEEP_ALIVE_PERIOD		0x1025

/* 4.44 Disable BSSID filter */
#define WSM_MIB_ID_DISABLE_BSSID_FILTER		0x1026

/* Inactivity */
#define WSM_MIB_ID_SET_INACTIVITY		0x1035

/* MAC Addr Filter */
#define WSM_MIB_ID_MAC_ADDR_FILTER		0x1030

#ifdef MCAST_FWDING
/* 4.51 Set Forwarding Offload */
#define WSM_MIB_ID_FORWARDING_OFFLOAD		0x1033
#endif

#ifdef IPV6_FILTERING
/* IpV6 Addr Filter */
/* 4.52 Neighbor solicitation IPv6 address table */
#define WSM_MIB_IP_IPV6_ADDR_FILTER		0x1032
#define WSM_MIB_ID_NS_IP_ADDRESSES_TABLE	0x1034
#define WSM_MAX_NDP_IP_ADDRTABLE_ENTRIES	1
#endif /*IPV6_FILTERING*/
#define WSM_MIB_ID_ADDR_SMU                    0x1036
#define WSM_MIB_ID_ZERO_COUNTERS_TABLE		0x1037
#define WSM_MIB_ID_DBG_PRINT_TO_HOST		0x1038
#define WSM_MIB_ID_FW_CMD					0x1039
#define WSM_MIB_ID_MAC_ADDRESS_FROM_EFUSE   0x103a
#define WSM_MIB_ID_SET_EFUSE_DATA			0x103b
#define WSM_MIB_ID_GET_DATA_FROM_EFUSE		0x103c
#define WSM_MIB_ID_SET_EFUSE_MAC			0x103d
#define WSM_MIB_ID_START_WOL				0x103e
#define WSM_MIB_ID_GET_ETF_RX_STATS			0x103f
#define WSM_MIB_ID_SET_EFUSE_DATA_ZHUANG	0x1040
#define WSM_MIB_ID_SMARTCONFIG_START		0x1041
#define WSM_MIB_ID_GET_EFUSE_CUR_STATUS		0x1042
#define WSM_MIB_ID_GET_EFUSE_FIRST_MAC      0x1045
#define WSM_MIB_ID_GET_TEMP_TX_REQ_ID       0x1046
#define WSM_MIB_ID_GET_RATE					0x1050


#define WSM_MIB_ID_GET_SIGMSTAR_256BITSEFUSE  0x1051
#define WSM_MIB_ID_SET_SIGMSTAR_256BITSEFUSE  0x1052
#define WSM_MIB_ID_GET_FIRST_BLOCK_EFUSE        0x1056
#define WSM_MIB_ID_SET_RATE_TX_POWER            0x1058
#define WSM_MIB_ID_GET_EFUSE_ALL_DATA           0x105b
#define WSM_MIB_ID_GET_CONFIG_TX_POWER			0x105c
#define WSM_MIB_ID_GET_CFO_PPM_CORRECTION_VALUE	0x105d
#define WSM_MIB_ID_SET_PHY_WRITE_REG_BIT_U32	0x105e
#define WSM_MIB_ID_GET_PHY_INIT_GLOBAL_PARA		0x105f
#define WSM_START_FIND_ID                 0x0019
#define WSM_START_FIND_RESP_ID            0x0419
#define WSM_STOP_FIND_ID					 0x001A
#define WSM_STOP_FIND_RESP_ID                0x041A

/*4.54 request buffer id*/
#define WSM_REQUEST_BUFFER_REQ_ID           0x0023
#define WSM_REQUEST_BUFFER_REQ_CNF_ID       0X0423

/*4.55 This device-to-host message returns the */
/*results for a previously queued transmit request */

#define WSM_CNF_BASE                      0x0400
#define WSM_TX_REQ_ID                     0x0404
#define WSM_GIVE_BUFFER_REQ_ID            0x0422
#define WSM_LEGACY_MULTI_TX_CNF_ID        0x041E
#define WSM_RATE_MULTI_TX_CNF_ID          0x041F

#define WSM_IND_BASE 					   0x0800
#define WSM_STARTUP_IND_ID 					0x0801
#define WSM_RECEIVE_INDICATION_ID           0x0804
#define WSM_EVENT_INDICATION_ID             0x0805
#define WSM_SCAN_COMPLETE_IND_ID 			0x0806
#define WSM_SWITCH_CHANNLE_IND_ID           0x080A
#define WSM_SET_PM_MODE_CMPL_IND_ID          0x0809
#define WSM_FIND_CMPL_IND_ID                 0x080B
#define WSM_SUSP_RESUME_TX_IND_ID            0x080C
#define WSM_DEBUG_IND_ID 					0x080E
#define WSM_DEBUG_PRINT_IND_ID				0x0810
#define WSM_MULTI_RECEIVE_INDICATION_ID		 0x0811
#define WSM_SINGLE_CHANNEL_MULTI_RECEIVE_INDICATION_ID	 0x0813
#define WSM_BLE_IND_ID	 					0x0821



/* Frame template types */
#define WSM_FRAME_TYPE_PROBE_REQUEST	(0)
#define WSM_FRAME_TYPE_BEACON		(1)
#define WSM_FRAME_TYPE_NULL		(2)
#define WSM_FRAME_TYPE_QOS_NULL		(3)
#define WSM_FRAME_TYPE_PS_POLL		(4)
#define WSM_FRAME_TYPE_PROBE_RESPONSE	(5)
#define WSM_FRAME_TYPE_ARP_REPLY        (6)

#ifdef IPV6_FILTERING
#define WSM_FRAME_TYPE_NA               (7)
#endif /*IPV6_FILTERING*/

#define WSM_FRAME_GREENFIELD		(0x80)	/* See 4.11 */

/* Status */
/* The WSM firmware has completed a request */
/* successfully. */
#define WSM_STATUS_SUCCESS              (0)

/* This is a generic failure code if other error codes do */
/* not apply. */
#define WSM_STATUS_FAILURE              (1)

/* A request contains one or more invalid parameters. */
#define WSM_INVALID_PARAMETER           (2)

/* The request cannot perform because the device is in */
/* an inappropriate mode. */
#define WSM_ACCESS_DENIED               (3)

/* The frame received includes a decryption error. */
#define WSM_STATUS_DECRYPTFAILURE       (4)

/* A MIC failure is detected in the received packets. */
#define WSM_STATUS_MICFAILURE           (5)

/* The transmit request failed due to retry limit being */
/* exceeded. */
#define WSM_STATUS_RETRY_EXCEEDED       (6)

/* The transmit request failed due to MSDU life time */
/* being exceeded. */
#define WSM_STATUS_TX_LIFETIME_EXCEEDED (7)

/* The link to the AP is lost. */
#define WSM_STATUS_LINK_LOST            (8)

/* No key was found for the encrypted frame */
#define WSM_STATUS_NO_KEY_FOUND         (9)

/* Jammer was detected when transmitting this frame */
#define WSM_STATUS_JAMMER_DETECTED      (10)

/* The message should be requeued later. */
/* This is applicable only to Transmit */
#define WSM_REQUEUE                     (11)

/* Advanced filtering options */
#define WSM_MAX_FILTER_ELEMENTS		(4)

#define WSM_FILTER_ACTION_IGNORE	(0)
#define WSM_FILTER_ACTION_FILTER_IN	(1)
#define WSM_FILTER_ACTION_FILTER_OUT	(2)

#define WSM_FILTER_PORT_TYPE_DST	(0)
#define WSM_FILTER_PORT_TYPE_SRC	(1)

struct wsm_hdr {
	atbm_uint16 len;
	atbm_uint16 id;
}atbm_packed;

struct wsm_hdr_tx {
	atbm_uint32 flag;
	
#if  ATBM_WSM_SDIO_TX_MULT
	atbm_uint16 tx_len;
	atbm_uint16 tx_id;
#endif
#if ATBM_USB_BUS	
	atbm_uint16 usb_len;
	atbm_uint16 usb_id;
#endif
	atbm_uint16 len;
	atbm_uint16 id;
}atbm_packed;

#define WSM_TX_SEQ_MAX			(7)
#define WSM_TX_SEQ(seq)			\
		((seq & WSM_TX_SEQ_MAX) << 13)
#define WSM_TX_LINK_ID_MAX		(0x03)
#define WSM_TX_LINK_ID(link_id)		\
		((link_id & WSM_TX_LINK_ID_MAX) << 6)

#define WSM_TX_IF_ID_MAX		(0x0F)
#define WSM_TX_IF_ID(if_id)		\
		((if_id & WSM_TX_IF_ID_MAX) << 6)

#define MAX_BEACON_SKIP_TIME_MS 1000
#define WSM_TX_FLAG(channel) (channel &0x1f)
#define WSM_TX_FLAG_CHECK(id) ((id &0xffff)<<16)
#ifdef FPGA_SETUP
#define WSM_CMD_LAST_CHANCE_TIMEOUT (HZ * 9 / 2)
#else
#define WSM_CMD_LAST_CHANCE_TIMEOUT (HZ * 20 / 2)
#endif
#define WSM_CMD_EXTENDED_TIMEOUT (HZ * 20 / 2)

#define WSM_RI_GET_PEER_ID_FROM_FLAGS(_f)         (((_f)&(0xF<<25)>>25))
/* ******************************************************************** */
/* WSM capcbility							*/
struct wsm_caps {
	atbm_uint16 numInpChBufs;
	atbm_uint16 sizeInpChBuf;
	atbm_uint16 hardwareId;
	atbm_uint16 hardwareSubId;
	atbm_uint32 firmwareCap;
	atbm_uint16 firmwareType;
	atbm_uint16 firmwareApiVer;
	atbm_uint16 firmwareBuildNumber;
	atbm_uint16 firmwareVersion;
	atbm_uint32 HiHwCnfBufaddr;
	atbm_uint32 firmwareReady;
	atbm_uint16 exceptionaddr;
	atbm_uint16 NumOfInterfaces;
	atbm_uint16 NumOfStations;
	atbm_uint32 NumOfHwXmitedAddr;
	atbm_uint32 firmeareExCap;
};

/* ******************************************************************** */
/* WSM commands								*/

#define WSM_CONFIGURATION_REQ_ID 0x0009
#define WSM_CONFIGURATION_RESP_ID 0x0409
struct wsm_tx_power_range {
	int min_power_level;
	int max_power_level;
	atbm_uint32 stepping;
};

/* 3.1 */
struct wsm_configuration {
	/* [in] */ atbm_uint32 dot11MaxTransmitMsduLifeTime;
	/* [in] */ atbm_uint32 dot11MaxReceiveLifeTime;
	/* [in] */ atbm_uint32 dot11RtsThreshold;
	/* [in, out] */ atbm_uint8 *dot11StationId;
	/* [in] */ atbm_uint8 *dpdData;
	/* [in] */ atbm_uint32 dpdData_size;
	/* [out] */ atbm_uint8 dot11FrequencyBandsSupported;
	/* [out] */ atbm_uint32 supportedRateMask;
	/* [out] */ struct wsm_tx_power_range txPowerRange[2];
};

int wsm_configuration(struct atbmwifi_common *hw_priv,
		      struct wsm_configuration *arg,
		      int if_id);

/* 3.3 */
#define WSM_RESET_REQ_ID 0x000A
#define WSM_RESET_RESP_ID 0x040A
struct wsm_reset {
	/* [in] */ int link_id;
	/* [in] */ ATBM_BOOL reset_statistics;
};

int wsm_reset(struct atbmwifi_common *hw_priv, const struct wsm_reset *arg,
	      int if_id);

/* 3.5 */
#define WSM_READ_MIB_REQ_ID 0x0005
#define WSM_READ_MIB_RESP_ID 0x0405
int wsm_read_mib(struct atbmwifi_common *hw_priv, atbm_uint16 mibId, atbm_void *buf,
		 atbm_size_t buf_size,int if_id);

/* 3.7 */
#define WSM_WRITE_MIB_REQ_ID 0x0006
#define WSM_WRITE_MIB_RESP_ID 0x0406
int wsm_write_mib(struct atbmwifi_common *hw_priv, atbm_uint16 mibId, atbm_void *buf,
		  atbm_size_t buf_size, int if_id);

/* 3.9 */
#define WSM_START_SCAN_REQ_ID 0x0007
#define WSM_START_SCAN_RESP_ID 0x0407
struct wsm_ssid {
	atbm_uint8 ssid[32];
	atbm_uint32 length;
};

struct wsm_scan_ch {
	atbm_uint16 number;
	atbm_uint32 minChannelTime;
	atbm_uint32 maxChannelTime;
	atbm_uint32 txPowerLevel;
};

/* 3.13 */
//#define WSM_SCAN_COMPLETE_IND_ID 0x0806
struct wsm_scan_complete {
	/* WSM_STATUS_... */
	atbm_uint32 status;

	/* WSM_PSM_... */
	atbm_uint8 psm;

	/* Number of channels that the scan operation completed. */
	atbm_uint8 numChannels;
#ifdef ROAM_OFFLOAD
	atbm_uint16 reserved;
#endif /*ROAM_OFFLOAD*/
};

typedef atbm_void (*wsm_scan_complete_cb) (struct atbmwifi_common *hw_priv,
	int interface_link_id,struct wsm_scan_complete *arg);

/* 3.9 */
struct wsm_scan {
	/* WSM_PHY_BAND_... */
	/* [in] */ atbm_uint8 band;

	/* WSM_SCAN_TYPE_... */
	/* [in] */ atbm_uint8 scanType;

	/* WSM_SCAN_FLAG_... */
	/* [in] */ atbm_uint8 scanFlags;

	/* WSM_TRANSMIT_RATE_... */
	/* [in] */ atbm_uint8 maxTransmitRate;

	/* Interval period in TUs that the device shall the re- */
	/* execute the requested scan. Max value supported by the device */
	/* is 256s. */
	/* [in] */ atbm_uint32 autoScanInterval;

	/* Number of probe requests (per SSID) sent to one (1) */
	/* channel. Zero (0) means that none is send, which */
	/* means that a passive scan is to be done. Value */
	/* greater than zero (0) means that an active scan is to */
	/* be done. */
	/* [in] */ atbm_uint32 numOfProbeRequests;

	/* Number of channels to be scanned. */
	/* Maximum value is WSM_SCAN_MAX_NUM_OF_CHANNELS. */
	/* [in] */ atbm_uint8 numOfChannels;

	/* Number of SSID provided in the scan command (this */
	/* is zero (0) in broadcast scan) */
	/* The maximum number of SSIDs is WSM_SCAN_MAX_NUM_OF_SSIDS. */
	/* [in] */ atbm_uint8 numOfSSIDs;

	/* The delay time (in microseconds) period */
	/* before sending a probe-request. */
	/* [in] */ atbm_uint8 probeDelay;

	/* SSIDs to be scanned [numOfSSIDs]; */
	/* [in] */ struct wsm_ssid *ssids;

	/* Channels to be scanned [numOfChannels]; */
	/* [in] */ struct wsm_scan_ch *ch;
}atbm_packed;

int wsm_scan(struct atbmwifi_common *hw_priv, const struct wsm_scan *arg,
			int if_id);

/* 3.11 */
#define WSM_STOP_SCAN_REQ_ID 0x0008
#define WSM_STOP_SCAN_RESP_ID 0x0408
int wsm_stop_scan(struct atbmwifi_common *hw_priv, int if_id);

/* 3.14 */
struct wsm_tx_confirm {
	/* Packet identifier used in wsm_tx. */
	/* [out] */ atbm_uint32 packetID;

	/* WSM_STATUS_... */
	/* [out] */ atbm_uint32 status;

	/* WSM_TRANSMIT_RATE_... */
	/* [out] */ atbm_uint8 txedRate;

	/* The number of times the frame was transmitted */
	/* without receiving an acknowledgement. */
	/* [out] */ atbm_uint8 ackFailures;

	/* WSM_TX_STATUS_... */
	/* [out] */ atbm_uint16 flags;

	/* The total time in microseconds that the frame spent in */
	/* the WLAN device before transmission as completed. */
	/* [out] */ atbm_uint32 mediaDelay;

	/* The total time in microseconds that the frame spent in */
	/* the WLAN device before transmission was started. */
	/* [out] */ atbm_uint32 txQueueDelay;

}atbm_packed;

struct txrate_status {
	atbm_uint16 txfail[RATE_INDEX_MAX];
	atbm_uint16 txcnt[RATE_INDEX_MAX];
}atbm_packed;;

struct wsm_tx_rate_confirm {
	int if_id;
	int link_id;
	struct txrate_status txstatus;
}atbm_packed;

/* 3.15 */
typedef atbm_void (*wsm_tx_confirm_cb) (struct atbmwifi_common *hw_priv,
				   struct wsm_tx_confirm *arg);

#define WSM_TRANSMIT_REQ_MSG_ID		0x0004
/* Note that ideology of wsm_tx struct is different against the rest of
 * WSM API. wsm_hdr is /not/ a caller-adapted struct to be used as an input
 * argument for WSM call, but a prepared bytestream to be sent to firmware.
 * It is filled partly in atbmwifi_tx, partly in low-level WSM code.
 * Please pay attention once again: ideology is different.
 *
 * Legend:
 * - [in]: atbmwifi_tx must fill this field.
 * - [wsm]: the field is filled by low-level WSM.
 */
struct wsm_tx {
	/* common WSM header */
	/* [in/wsm] */ struct wsm_hdr_tx hdr;

	/* Packet identifier that meant to be used in completion. */
	/* [in] */ atbm_uint32 packetID;

	/* WSM_TRANSMIT_RATE_... */
	/* [in] */ atbm_uint8 maxTxRate;

	/* WSM_QUEUE_... */
	/* [in] */ atbm_uint8 queueId;

	/* True: another packet is pending on the host for transmission. */
	/* [wsm] */ atbm_uint8 more;

	/* Bit 0 = 0 - Start expiry time from first Tx attempt (default) */
	/* Bit 0 = 1 - Start expiry time from receipt of Tx Request */
	/* Bits 3:1  - PTA Priority */
	/* Bits 6:4  - Tx Rate Retry Policy */
	/* Bit 7 - Reserved */
	/* [in] */ atbm_uint8 flags;

	/* Should be 0. */
	/* [in] */ atbm_uint32 reserved;

	/* The elapsed time in TUs, after the initial transmission */
	/* of an MSDU, after which further attempts to transmit */
	/* the MSDU shall be terminated. Overrides the global */
	/* dot11MaxTransmitMsduLifeTime setting [optional] */
	/* Device will set the default value if this is 0. */
	/* [wsm] */ atbm_uint32 expireTime;

	/* WSM_HT_TX_... */
	/* [in] */ atbm_uint32 htTxParameters;
}atbm_packed;

/* = sizeof(generic hi hdr) + sizeof(wsm hdr) + sizeof(alignment) */
#define WSM_TX_EXTRA_HEADROOM (28+4)

/* 3.16 FW = struct WSM_HI_RX_IND_S*/
struct wsm_rx {
	/* WSM_STATUS_... */
	/* [out] */ atbm_uint32 status;

	/* Specifies the channel of the received packet. */
	/* [out] */ atbm_uint16 channelNumber;

	/* WSM_TRANSMIT_RATE_... */
	/* [out] */ atbm_uint8 rxedRate;

	/* This value is expressed in signed Q8.0 format for */
	/* RSSI and unsigned Q7.1 format for RCPI. */
	/* [out] */ atbm_uint8 rcpiRssi;

	/* WSM_RX_STATUS_... */
	/* [out] */ atbm_uint32 flags;
	/* WSM_RX_STATUS_... */
	/* [out] */ atbm_uint32 channelType;

	/* An 802.11 frame. */
	/* [out] */ //void *frame;

	/* Size of the frame */
	/* [out] */ //atbm_size_t frame_size;

	/* Link ID */
	/* [out] */ //int link_id;
	/* [out] */ //int if_id;
}atbm_packed;
struct wsm_multi_rx {
	atbm_uint16 MsgLen;
	atbm_uint16 MsgId;
	atbm_uint8  RxFrameNum;
	atbm_uint8  reserved[3];	
}atbm_packed;
/* = sizeof(generic hi hdr) + sizeof(wsm hdr) */
#define WSM_RX_EXTRA_HEADROOM (16)

typedef atbm_void (*wsm_rx_cb) (struct atbmwifi_vif *priv, struct wsm_rx *arg,
			   struct atbm_buff **skb_p);

/* 3.17 */
struct wsm_event {
	/* WSM_STATUS_... */
	/* [out] */ atbm_uint32 eventId;

	/* Indication parameters. */
	/* For error indication, this shall be a 32-bit WSM status. */
	/* For RCPI or RSSI indication, this should be an 8-bit */
	/* RCPI or RSSI value. */
	/* [out] */ atbm_uint32 eventData;
};

struct atbm_wsm_event {
	struct atbm_list_head link;
	struct wsm_event evt;
	atbm_uint8 if_id;
	atbm_uint8 reserved[3];
};

/* 3.18 - 3.22 */
/* Measurement. Skipped for now. Irrelevent. */

typedef atbm_void (*wsm_event_cb) (struct atbmwifi_common *hw_priv,
			      struct wsm_event *arg);



/* 3.23 */
#define WSM_JOIN_REQ_ID 0x000B
#define WSM_JOIN_RESP_ID 0x040B
struct wsm_join {
	/* WSM_JOIN_MODE_... */
	/* [in] */ atbm_uint8 mode;

	/* WSM_PHY_BAND_... */
	/* [in] */ atbm_uint8 band;

	/* Specifies the channel number to join. The channel */
	/* number will be mapped to an actual frequency */
	/* according to the band */
	/* [in] */ atbm_uint16 channelNumber;

	/* Specifies the BSSID of the BSS or IBSS to be joined */
	/* or the IBSS to be started. */
	/* [in] */ atbm_uint8 bssid[6];

	/* ATIM window of IBSS */
	/* When ATIM window is zero the initiated IBSS does */
	/* not support power saving. */
	/* [in] */ atbm_uint16 atimWindow;

	/* WSM_JOIN_PREAMBLE_... */
	/* [in] */ atbm_uint8 preambleType;

	/* Specifies if a probe request should be send with the */
	/* specified SSID when joining to the network. */
	/* [in] */ atbm_uint8 probeForJoin;

	/* DTIM Period (In multiples of beacon interval) */
	/* [in] */ atbm_uint8 dtimPeriod;

	/* WSM_JOIN_FLAGS_... */
	/* [in] */ atbm_uint8 flags;

	/* Length of the SSID */
	/* [in] */ atbm_uint32 ssidLength;

	/* Specifies the SSID of the IBSS to join or start */
	/* [in] */ atbm_uint8 ssid[32];

	/* Specifies the time between TBTTs in TUs */
	/* [in] */ atbm_uint32 beaconInterval;

	/* A bit mask that defines the BSS basic rate set. */
	/* [in] */ atbm_uint32 basicRateSet;
	/*width 40M params*/
	/*[in]*/	atbm_uint32 channel_type;


	/* Minimum transmission power level in units of 0.1dBm */
	/* [out] */ int minPowerLevel;

	/* Maximum transmission power level in units of 0.1dBm */
	/* [out] */ int maxPowerLevel;
};

int wsm_join(struct atbmwifi_common *hw_priv, struct wsm_join *arg, int if_id);

/* 3.25 */
#define WSM_SET_PM_REQ_ID 0x0010
#define WSM_SET_PM_RESP_ID 0x0410
struct wsm_set_pm {
	/* WSM_PSM_... */
	/* [in] */ atbm_uint8 pmMode;

	/* in unit of 500us; 0 to use default */
	/* [in] */ atbm_uint8 fastPsmIdlePeriod;

	/* in unit of 500us; 0 to use default */
	/* [in] */ atbm_uint8 apPsmChangePeriod;

	/* in unit of 500us; 0 to disable auto-pspoll */
	/* [in] */ atbm_uint8 minAutoPsPollPeriod;
};

int wsm_set_pm(struct atbmwifi_common *hw_priv, const struct wsm_set_pm *arg,
	       int if_id);

/* 3.27 */
struct wsm_set_pm_complete {
	atbm_uint8 psm;			/* WSM_PSM_... */
};

typedef atbm_void (*wsm_set_pm_complete_cb) (struct atbmwifi_common *hw_priv,
					struct wsm_set_pm_complete *arg);

/* 3.28 */
#define WSM_SET_BSS_PARAMS_REQ_ID 0x0011
#define WSM_SET_BSS_PARAMS_RESP_ID 0x0411
struct wsm_set_bss_params {
	/* The number of lost consecutive beacons after which */
	/* the WLAN device should indicate the BSS-Lost event */
	/* to the WLAN host driver. */
	atbm_uint8 beaconLostCount;

	/* The AID received during the association process. */
	atbm_uint16 aid;

	/* The operational rate set mask */
	atbm_uint32 operationalRateSet;
};

int wsm_set_bss_params(struct atbmwifi_common *hw_priv,
		       const struct wsm_set_bss_params *arg, int if_id);

/* 3.30 */
#define WSM_ADD_KEY_REQ_ID         0x000C
#define WSM_ADD_KEY_RESP_ID        0x040C
#ifndef LINUX_OS
#pragma anon_unions
#endif
struct wsm_add_key {
	atbm_uint8 type;		/* WSM_KEY_TYPE_... */
	atbm_uint8 entryIndex;		/* Key entry index: 0 -- WSM_KEY_MAX_INDEX */
	atbm_uint16 reserved;
	union {
		struct {
			atbm_uint8 peerAddress[6];	/* MAC address of the
						 * peer station */
			atbm_uint8 reserved;
			atbm_uint8 keyLength;		/* Key length in bytes */
			atbm_uint8 keyData[16];		/* Key data */
		}  atbm_packed wepPairwiseKey;
		struct {
			atbm_uint8 keyId;		/* Unique per key identifier
						 * (0..3) */
			atbm_uint8 keyLength;		/* Key length in bytes */
			atbm_uint16 reserved;
			atbm_uint8 keyData[16];		/* Key data */
		} atbm_packed  wepGroupKey;
		struct {
			atbm_uint8 peerAddress[6];	/* MAC address of the
						 * peer station */
			atbm_uint8 reserved[2];
			atbm_uint8 tkipKeyData[16];	/* TKIP key data */
			atbm_uint8 rxMicKey[8];		/* Rx MIC key */
			atbm_uint8 txMicKey[8];		/* Tx MIC key */
		} atbm_packed tkipPairwiseKey;
		struct {
			atbm_uint8 tkipKeyData[16];	/* TKIP key data */
			atbm_uint8 rxMicKey[8];		/* Rx MIC key */
			atbm_uint8 keyId;		/* Key ID */
			atbm_uint8 reserved[3];
			atbm_uint8 rxSeqCounter[8];	/* Receive Sequence Counter */
		} atbm_packed tkipGroupKey;
		struct {
			atbm_uint8 peerAddress[6];	/* MAC address of the
						 * peer station */
			atbm_uint16 reserved;
			atbm_uint8 aesKeyData[16];	/* AES key data */
		}  atbm_packed aesPairwiseKey;
		struct {
			atbm_uint8 aesKeyData[16];	/* AES key data */
			atbm_uint8 keyId;		/* Key ID */
			atbm_uint8 reserved[3];
			atbm_uint8 rxSeqCounter[8];	/* Receive Sequence Counter */
		}  atbm_packed aesGroupKey;
		struct {
			atbm_uint8 igtKeyData[16];	/* AES key data */
			atbm_uint8 keyId;		/* Key ID */
			atbm_uint8 reserved[3];
			atbm_uint8 ipn[8];	/* Receive Sequence Counter */
		} atbm_packed igtkGroupKey;
		struct {
			atbm_uint8 peerAddress[6];	/* MAC address of the
						 * peer station */
			atbm_uint8 keyId;		/* Key ID */
			atbm_uint8 reserved;
			atbm_uint8 wapiKeyData[16];	/* WAPI key data */
			atbm_uint8 micKeyData[16];	/* MIC key data */
		} atbm_packed wapiPairwiseKey;
		struct {
			atbm_uint8 wapiKeyData[16];	/* WAPI key data */
			atbm_uint8 micKeyData[16];	/* MIC key data */
			atbm_uint8 keyId;		/* Key ID */
			atbm_uint8 reserved[3];
		}  atbm_packed wapiGroupKey;
	}atbm_packed ;
} atbm_packed;

int wsm_add_key(struct atbmwifi_common *hw_priv, const struct wsm_add_key *arg,
			int if_id);

/* 3.32 */
#define WSM_REMOVE_KEY_REQ_ID         0x000D
#define WSM_REMOVE_KEY_RESP_ID        0x040D
struct wsm_remove_key {
	/* Key entry index : 0-10 */
	atbm_uint8 entryIndex;
};
#if 0
int wsm_remove_key(struct atbmwifi_common *hw_priv,
		   const struct wsm_remove_key *arg, int if_id);
#else
int wsm_remove_key(struct atbmwifi_common *hw_priv, const struct wsm_add_key *arg,
			int if_id);

#endif
/* 3.34 */
#define WSM_QUEUE_PARAMS_REQ_ID 0x0012
#define WSM_QUEUE_PARAMS_RESP_ID 0x0412
struct wsm_set_tx_queue_params {
	/* WSM_ACK_POLICY_... */
	atbm_uint8 ackPolicy;

	/* Medium Time of TSPEC (in 32us units) allowed per */
	/* One Second Averaging Period for this queue. */
	atbm_uint16 allowedMediumTime;

	/* dot11MaxTransmitMsduLifetime to be used for the */
	/* specified queue. */
	atbm_uint32 maxTransmitLifetime;
};

struct wsm_tx_queue_params {
	/* NOTE: index is a linux queue id. */
	struct wsm_set_tx_queue_params params[4];
};

#define WSM_TX_QUEUE_SET(queue_params, queue, ack_policy, allowed_time,     \
			 max_life_time)					    \
do {									    \
	struct wsm_set_tx_queue_params *p = &(queue_params)->params[queue]; \
	p->ackPolicy = (ack_policy);				\
	p->allowedMediumTime = (allowed_time);				\
	p->maxTransmitLifetime = (max_life_time);			\
} while (0)

int wsm_set_tx_queue_params(struct atbmwifi_common *hw_priv,
			    const struct wsm_set_tx_queue_params *arg,
			    atbm_uint8 id, int if_id);

/* 3.36 */
#define WSM_EDCA_PARAMS_REQ_ID 0x0013
#define WSM_EDCA_PARAMS_RESP_ID 0x0413
struct wsm_edca_queue_params {
	/* CWmin (in slots) for the access class. */
	/* [in] */ atbm_uint16 cwMin;

	/* CWmax (in slots) for the access class. */
	/* [in] */ atbm_uint16 cwMax;

	/* AIFS (in slots) for the access class. */
	/* [in] */ atbm_uint8 aifns;

	/* TX OP Limit (in microseconds) for the access class. */
	/* [in] */ atbm_uint16 txOpLimit;

	/* dot11MaxReceiveLifetime to be used for the specified */
	/* the access class. Overrides the global */
	/* dot11MaxReceiveLifetime value */
	/* [in] */ atbm_uint32 maxReceiveLifetime;

	/* UAPSD trigger support for the access class. */
	/* [in] */ ATBM_BOOL uapsdEnable;
};

struct wsm_edca_params {
	/* NOTE: index is a linux queue id. */
	struct wsm_edca_queue_params params[4];
};




struct config_edca_params{
	atbm_uint8 wmep_acm;		/* ACM parameter */
	atbm_uint8 aifns;		/* AIFSN parameters */
	atbm_uint8 cwMin;		/* cwmin in exponential form */
	atbm_uint8 cwMax;		/* cwmax in exponential form */
	atbm_uint16 txOpLimit;	/* txopLimit */
	atbm_uint8 wmep_noackPolicy;	/* No-Ack Policy: 0=ack, 1=no-ack */
	atbm_uint8 uapsdEnable;
};


#define TXOP_UNIT 32
#define WSM_EDCA_SET(edca, queue, aifs, cw_min, cw_max, txop, life_time,\
		uapsd)	\
	do {							\
		struct wsm_edca_queue_params *p = &(edca)->params[queue]; \
		p->cwMin = (cw_min);				\
		p->cwMax = (cw_max);				\
		p->aifns = (aifs);				\
		p->txOpLimit = ((txop) * TXOP_UNIT);		\
		p->maxReceiveLifetime = (life_time);		\
		p->uapsdEnable = (uapsd);			\
	} while (0)

int wsm_set_edca_params(struct atbmwifi_common *hw_priv,
			const struct wsm_edca_params *arg, int if_id);

int wsm_set_uapsd_param(struct atbmwifi_common *hw_priv,
			const struct wsm_edca_params *arg);

/* 3.38 */
/* Set-System info. Skipped for now. Irrelevent. */

/* 3.40 */
#define WSM_SWITCH_CHANNEL_REQ_ID 0x0016
#define WSM_SWITCH_CHANNEL_RESP_ID 0x0416
struct wsm_switch_channel {
	/* 1 - means the STA shall not transmit any further */
	/* frames until the channel switch has completed */
	/* [in] */ atbm_uint8 channelMode;

	/* Number of TBTTs until channel switch occurs. */
	/* 0 - indicates switch shall occur at any time */
	/* 1 - occurs immediately before the next TBTT */
	/* [in] */ atbm_uint8 channelSwitchCount;

	/* The new channel number to switch to. */
	/* Note this is defined as per section 2.7. */
	/* [in] */ atbm_uint16 newChannelNumber;
};

int wsm_switch_channel(struct atbmwifi_common *hw_priv,
		       const struct wsm_switch_channel *arg, int if_id);

typedef atbm_void (*wsm_channel_switch_cb) (struct atbmwifi_common *hw_priv);

#define WSM_START_REQ_ID 0x0017
#define WSM_START_RESP_ID 0x0417
struct wsm_start {
	/* WSM_START_MODE_... */
	/* [in] */ atbm_uint8 mode;

	/* WSM_PHY_BAND_... */
	/* [in] */ atbm_uint8 band;

	/* Channel number */
	/* [in] */ atbm_uint16 channelNumber;

	/* Client Traffic window in units of TU */
	/* Valid only when mode == ..._P2P */
	/* [in] */ atbm_uint32 CTWindow;

	/* Interval between two consecutive */
	/* beacon transmissions in TU. */
	/* [in] */ atbm_uint32 beaconInterval;

	/* DTIM period in terms of beacon intervals */
	/* [in] */ atbm_uint8 DTIMPeriod;

	/* WSM_JOIN_PREAMBLE_... */
	/* [in] */ atbm_uint8 preambleType;

	/* The delay time (in microseconds) period */
	/* before sending a probe-request. */
	/* [in] */ atbm_uint8 probeDelay;

	/* Length of the SSID */
	/* [in] */ atbm_uint8 ssidLength;

	/* SSID of the BSS or P2P_GO to be started now. */
	/* [in] */ atbm_uint8 ssid[32];

	/* The basic supported rates for the MiniAP. */
	/* [in] */ atbm_uint32 basicRateSet;
	/* [in] */ 	atbm_uint32	channel_type;
};

int wsm_start(struct atbmwifi_common *hw_priv, const struct wsm_start *arg,
		int if_id);

#define WSM_BEACON_TRANSMIT_REQ_ID 0x0018
#define WSM_BEACON_TRANSMIT_RESP_ID 0x0418
#if 0
struct wsm_beacon_transmit {
	/* 1: enable; 0: disable */
	/* [in] */ atbm_uint8 enableBeaconing;
};

int wsm_beacon_transmit(struct atbmwifi_common *hw_priv,
			const struct wsm_beacon_transmit *arg,
			int if_id);
#endif

int wsm_start_find(struct atbmwifi_common *hw_priv, int if_id);

int wsm_stop_find(struct atbmwifi_common *hw_priv, int if_id);

typedef atbm_void (*wsm_find_complete_cb) (struct atbmwifi_common *hw_priv,
				      atbm_uint32 status);

struct wsm_suspend_resume {
	/* See 3.52 */
	/* Link ID */
	/* [out] */ int link_id;
	/* Stop sending further Tx requests down to device for this link */
	/* [out] */ ATBM_BOOL stop;
	/* Transmit multicast Frames */
	/* [out] */ ATBM_BOOL multicast;
	/* The AC on which Tx to be suspended /resumed. */
	/* This is applicable only for U-APSD */
	/* WSM_QUEUE_... */
	/* [out] */ int queue;
	/* [out] */ int if_id;
};

typedef atbm_void (*wsm_suspend_resume_cb) (struct atbmwifi_vif *priv,
				       struct wsm_suspend_resume *arg);

/* 3.54 Update-IE request. */
#define WSM_UPDATE_IE_REQ_ID     0x001B
#define WSM_UPDATE_IE_RESP_ID    0x041B
struct wsm_update_ie {
	/* WSM_UPDATE_IE_... */
	/* [in] */ atbm_uint16 what;
	/* [in] */ atbm_uint16 count;
	/* [in] */ atbm_uint8 *ies;
	/* [in] */ atbm_size_t length;
};

int wsm_update_ie(struct atbmwifi_common *hw_priv,
		  const struct wsm_update_ie *arg, int if_id);

/* 3.56 */
struct wsm_map_link {
	/* MAC address of the remote device */
	/* [in] */ atbm_uint8 mac_addr[6];
	/* [in] */ atbm_uint8 unmap;
	/* [in] */ atbm_uint8 link_id;
};
#define WSM_MAP_LINK_REQ_ID       0x001C
#define WSM_MAP_LINK_RESP_ID       0x041C
int wsm_map_link(struct atbmwifi_common *hw_priv, const struct wsm_map_link *arg,
		int if_id);

struct wsm_cbc {
	wsm_scan_complete_cb scan_complete;
	wsm_event_cb event;
	wsm_set_pm_complete_cb set_pm_complete;
	wsm_channel_switch_cb channel_switch;
	wsm_find_complete_cb find_complete;
	wsm_suspend_resume_cb suspend_resume;
};
#ifdef MCAST_FWDING

/* 3.65	Give Buffer Request */
int wsm_init_release_buffer_request(struct atbmwifi_common *priv, atbm_uint8 index);

/* 3.67	Request Buffer Request */
int wsm_request_buffer_request(struct atbmwifi_vif *priv,
                                atbm_uint8 *arg);
#endif
/* ******************************************************************** */
/* MIB shortcats							*/
struct wsm_rcpi_rssi_threshold {
	atbm_uint8 rssiRcpiMode;	/* WSM_RCPI_RSSI_... */
	atbm_uint8 lowerThreshold;
	atbm_uint8 upperThreshold;
	atbm_uint8 rollingAverageCount;
};

struct wsm_counters_table {
	atbm_uint32 countPlcpErrors;
	atbm_uint32 countFcsErrors;
	atbm_uint32 countTxPackets;
	atbm_uint32 countRxPackets;
	atbm_uint32 countRxPacketErrors;
	atbm_uint32 countRtsSuccess;
	atbm_uint32 countRtsFailures;
	atbm_uint32 countRxFramesSuccess;
	atbm_uint32 countRxDecryptionFailures;
	atbm_uint32 countRxMicFailures;
	atbm_uint32 countRxNoKeyFailures;
	atbm_uint32 countTxMulticastFrames;
	atbm_uint32 countTxFramesSuccess;
	atbm_uint32 countTxFrameFailures;
	atbm_uint32 countTxFramesRetried;
	atbm_uint32 countTxFramesMultiRetried;
	atbm_uint32 countRxFrameDuplicates;
	atbm_uint32 countAckFailures;
	atbm_uint32 countRxMulticastFrames;
	atbm_uint32 countRxCMACICVErrors;
	atbm_uint32 countRxCMACReplays;
	atbm_uint32 countRxMgmtCCMPReplays;
	atbm_uint32 countRxBIPMICErrors;
};

struct efuse_headr{
	atbm_uint8 specific;
	atbm_uint8 version;
	atbm_uint8 dcxo_trim;
	atbm_uint8 delta_gain1;
	atbm_uint8 delta_gain2;
	atbm_uint8 delta_gain3;
	atbm_uint8 Tj_room;
	atbm_uint8 topref_ctrl_bias_res_trim;
	atbm_uint8 PowerSupplySel;
	atbm_uint8 mac[6];
};

//wsm_efuse_change_data_cmd  return value
#define LMC_STATUS_CODE__EFUSE_VERSION_CHANGE	96
#define LMC_STATUS_CODE__EFUSE_FIRST_WRITE 		97
#define LMC_STATUS_CODE__EFUSE_PARSE_FAILED 	98
#define LMC_STATUS_CODE__EFUSE_FULL 			99

#define WSM_HI_EFUSE_CHANGE_DATA_REQ_ID 0x0019
#define WSM_HI_EFUSE_CHANGE_DATA_CNF_ID 0x0419


int wsm_get_SIGMSTAR_256BITSEFUSE(struct atbmwifi_common *hw_priv, atbm_uint8 *efuse, atbm_int32 len);
int wsm_set_SIGMSTAR_256BITSEFUSE(struct atbmwifi_common *hw_priv, atbm_uint8 *efuse, atbm_int32 len);
int wsm_get_efuse_data(struct atbmwifi_common *hw_priv, atbm_void *efuse, int len);

struct wsm_rx_filter {
	ATBM_BOOL promiscuous;
	ATBM_BOOL bssid;
	ATBM_BOOL fcs;
	ATBM_BOOL probeResponder;
	ATBM_BOOL keepalive;
};
int wsm_set_probe_responder(struct atbmwifi_vif *priv, ATBM_BOOL enable);
int wsm_set_keepalive_filter(struct atbmwifi_vif *priv, ATBM_BOOL enable);

#define WSM_BEACON_FILTER_IE_HAS_CHANGED	BIT(0)
#define WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT	BIT(1)
#define WSM_BEACON_FILTER_IE_HAS_APPEARED	BIT(2)

struct wsm_beacon_filter_table_entry {
	atbm_uint8	ieId;
	atbm_uint8	actionFlags;
	atbm_uint8	oui[3];
	atbm_uint8	matchData[3];
} ;

struct wsm_beacon_filter_table {
	atbm_uint32 numOfIEs;
	struct wsm_beacon_filter_table_entry entry[10];
} ;

#define WSM_BEACON_FILTER_ENABLE	BIT(0) /* Enable/disable beacon filtering */
#define WSM_BEACON_FILTER_AUTO_ERP	BIT(1) /* If 1 FW will handle ERP IE changes internally */

struct wsm_beacon_filter_control {
	int enabled;
	int bcn_count;
};

enum wsm_power_mode {
	wsm_power_mode_active = 0,
	wsm_power_mode_doze = 1,
	wsm_power_mode_quiescent = 2,
};

struct wsm_operational_mode {
	enum wsm_power_mode power_mode;
	int disableMoreFlagUsage;
	int performAntDiversity;
};

struct wsm_inactivity {
	atbm_uint8 max_inactivity;
	atbm_uint8 min_inactivity;
};
struct wsm_template_frame {
	atbm_uint8 frame_type;
	atbm_uint8 rate;
	ATBM_BOOL disable;
	struct atbm_buff *skb;
};

struct wsm_protected_mgmt_policy {
	ATBM_BOOL protectedMgmtEnable;
	ATBM_BOOL unprotectedMgmtFramesAllowed;
	ATBM_BOOL encryptionForAuthFrame;
};
struct wsm_association_mode {
	atbm_uint8 flags;		/* WSM_ASSOCIATION_MODE_... */
	atbm_uint8 preambleType;	/* WSM_JOIN_PREAMBLE_... */
	atbm_uint8 greenfieldMode;	/* 1 for greenfield */
	atbm_uint8 mpduStartSpacing;
	atbm_uint32 basicRateSet;
};

struct wsm_set_tx_rate_retry_policy_header {
	atbm_uint8 numTxRatePolicies;
	atbm_uint8 reserved[3];
} ;

struct wsm_set_tx_rate_retry_policy_policy {
	atbm_uint8 policyIndex;
	atbm_uint8 shortRetryCount;
	atbm_uint8 longRetryCount;
	atbm_uint8 policyFlags;
	atbm_uint8 rateRecoveryCount;
	atbm_uint8 reserved[3];
	atbm_uint8 rateCountIndices[12];
} ;

struct wsm_set_tx_rate_retry_policy {
	struct wsm_set_tx_rate_retry_policy_header hdr;
	struct wsm_set_tx_rate_retry_policy_policy tbl[8];
} ;
/* 4.32 SetEtherTypeDataFrameFilter */
struct wsm_ether_type_filter_hdr {
	atbm_uint8 nrFilters;		/* Up to WSM_MAX_FILTER_ELEMENTS */
	atbm_uint8 reserved[3];
} ;

struct wsm_ether_type_filter {
	atbm_uint8 filterAction;	/* WSM_FILTER_ACTION_XXX */
	atbm_uint8 reserved;
	atbm_uint16 etherType;	/* Type of ethernet frame */
} ;


/* 4.33 SetUDPPortDataFrameFilter */
struct wsm_udp_port_filter_hdr {
	atbm_uint8 nrFilters;		/* Up to WSM_MAX_FILTER_ELEMENTS */
	atbm_uint8 reserved[3];
} ;

struct wsm_udp_port_filter {
	atbm_uint8 filterAction;	/* WSM_FILTER_ACTION_XXX */
	atbm_uint8 portType;		/* WSM_FILTER_PORT_TYPE_XXX */
	atbm_uint16 udpPort;		/* Port number */
} ;

/* Undocumented MIBs: */
/* 4.35 P2PDeviceInfo */
#define D11_MAX_SSID_LEN		(32)

struct wsm_p2p_device_type {
	atbm_uint16 categoryId;
	atbm_uint8 oui[4];
	atbm_uint16 subCategoryId;
} ;

struct wsm_p2p_device_info {
	struct wsm_p2p_device_type primaryDevice;
	atbm_uint8 reserved1[3];
	atbm_uint8 devNameSize;
	atbm_uint8 localDevName[D11_MAX_SSID_LEN];
	atbm_uint8 reserved2[3];
	atbm_uint8 numSecDevSupported;
	struct wsm_p2p_device_type secondaryDevices[ZEROSIZE];
} ;

/* 4.36 SetWCDMABand - WO */
struct wsm_cdma_band {
	atbm_uint8 WCDMA_Band;
	atbm_uint8 reserved[3];
} ;

/* 4.37 GroupTxSequenceCounter - RO */
struct wsm_group_tx_seq {
	atbm_uint32 bits_47_16;
	atbm_uint16 bits_15_00;
	atbm_uint16 reserved;
} ;

/* 4.39 SetHtProtection - WO */
#define WSM_DUAL_CTS_PROT_ENB		(1 << 0)
#define WSM_NON_GREENFIELD_STA		PRESENT(1 << 1)
#define WSM_HT_PROT_MODE__NO_PROT	(0 << 2)
#define WSM_HT_PROT_MODE__NON_MEMBER	(1 << 2)
#define WSM_HT_PROT_MODE__20_MHZ	(2 << 2)
#define WSM_HT_PROT_MODE__NON_HT_MIXED	(3 << 2)
#define WSM_LSIG_TXOP_PROT_FULL		(1 << 4)
#define WSM_LARGE_L_LENGTH_PROT		(1 << 5)

struct wsm_ht_protection {
	atbm_uint32 flags;
} ;

/* 4.40 GPIO Command - R/W */
#define WSM_GPIO_COMMAND_SETUP	0
#define WSM_GPIO_COMMAND_READ	1
#define WSM_GPIO_COMMAND_WRITE	2
#define WSM_GPIO_COMMAND_RESET	3
#define WSM_GPIO_ALL_PINS	0xFF

struct wsm_gpio_command {
	atbm_uint8 GPIO_Command;
	atbm_uint8 pin;
	atbm_uint16 config;
} ;

/* 4.41 TSFCounter - RO */
struct wsm_tsf_counter {
	atbm_uint64 TSF_Counter;
} ;

/* 4.43 Keep alive period */
struct wsm_keep_alive_period {
	atbm_uint16 keepAlivePeriod;
	atbm_uint8 reserved[2];
} ;

/* BSSID filtering */
struct wsm_set_bssid_filtering {
	atbm_uint8 filter;
	atbm_uint8 reserved[3];
} ;


/* Multicat filtering - 4.5 */
struct wsm_multicast_filter {
	atbm_uint32 enable;
	atbm_uint32 numOfAddresses;
	atbm_uint8 macAddress[WSM_MAX_GRP_ADDRTABLE_ENTRIES][ATBM_ETH_ALEN];
} ;

/* Mac Addr Filter Info */
struct wsm_mac_addr_info {
	atbm_uint8 filter_mode;
	atbm_uint8 address_mode;
	atbm_uint8 MacAddr[6];
} ;

/* Mac Addr Filter */
struct wsm_mac_addr_filter {
	atbm_uint8 numfilter;
	atbm_uint8 action_mode;
	atbm_uint8 Reserved[2];
	struct wsm_mac_addr_info macaddrfilter[ZEROSIZE];
} ;

/* Broadcast Addr Filter */
struct wsm_broadcast_addr_filter {
	atbm_uint8 action_mode;
	atbm_uint8 nummacaddr;
	atbm_uint8 filter_mode;
	atbm_uint8 address_mode;
	atbm_uint8 MacAddr[6];
} ;


/* ARP IPv4 filtering - 4.10 */
struct wsm_arp_ipv4_filter {
	atbm_uint32 enable;
	atbm_uint32 ipv4Address[WSM_MAX_ARP_IP_ADDRTABLE_ENTRIES];
} ;

#ifdef IPV6_FILTERING
/* NDP IPv6 filtering */
struct wsm_ndp_ipv6_filter {
	atbm_uint32 enable;
	struct in6_addr ipv6Address[WSM_MAX_NDP_IP_ADDRTABLE_ENTRIES];
} ;
/* IPV6 Addr Filter Info */
struct wsm_ip6_addr_info {
	atbm_uint8 filter_mode;
	atbm_uint8 address_mode;
	atbm_uint8 Reserved[2];
	atbm_uint8 ipv6[16];
};

/* IPV6 Addr Filter */
struct wsm_ipv6_filter {
	atbm_uint8 numfilter;
	atbm_uint8 action_mode;
	atbm_uint8 Reserved[2];
	struct wsm_ip6_addr_info ipv6filter[0];
} ;
#endif /*IPV6_FILTERING*/

/* P2P Power Save Mode Info - 4.31 */
struct wsm_p2p_ps_modeinfo {
	atbm_uint8	oppPsCTWindow;
	atbm_uint8	count;
	atbm_uint8	reserved;
	atbm_uint8	dtimCount;
	atbm_uint32	duration;
	atbm_uint32	interval;
	atbm_uint32	startTime;
} ;


/* 4.26 SetUpasdInformation */
struct wsm_uapsd_info {
	atbm_uint16 uapsdFlags;
	atbm_uint16 minAutoTriggerInterval;
	atbm_uint16 maxAutoTriggerInterval;
	atbm_uint16 autoTriggerStep;
};
/* 4.22 OverrideInternalTxRate */
struct wsm_override_internal_txrate {
	atbm_uint8 internalTxRate;
	atbm_uint8 nonErpInternalTxRate;
	atbm_uint8 reserved[2];
} ;


#ifdef MCAST_FWDING
/* 4.51 SetForwardingOffload */
struct wsm_forwarding_offload {
	atbm_uint8 fwenable;
	atbm_uint8 flags;
	atbm_uint8 reserved[2];
} ;

#endif
/* ******************************************************************** */
/* WSM TX port control							*/

atbm_void wsm_lock_tx(struct atbmwifi_common *hw_priv);
atbm_void wsm_vif_lock_tx(struct atbmwifi_vif *priv);
atbm_void wsm_lock_tx_async(struct atbmwifi_common *hw_priv);
ATBM_BOOL wsm_flush_tx(struct atbmwifi_common *hw_priv);
ATBM_BOOL wsm_vif_flush_tx(struct atbmwifi_vif *priv);
atbm_void wsm_unlock_tx(struct atbmwifi_common *hw_priv);

/* ******************************************************************** */
/* WSM / BH API								*/

int wsm_handle_exception(struct atbmwifi_common *hw_priv, atbm_uint8 * data, atbm_uint32 len);
int wsm_handle_rx(struct atbmwifi_common *hw_priv, int id, struct wsm_hdr *wsm,
		  struct atbm_buff **skb_p);

/* ******************************************************************** */
/* wsm_buf API								*/

struct wsm_buf {
	atbm_uint8 *begin;
	atbm_uint8 *data;
	atbm_uint8 *end;
};

atbm_void wsm_buf_init(struct wsm_buf *buf);
atbm_void wsm_buf_deinit(struct wsm_buf *buf);

/* ******************************************************************** */
/* wsm_cmd API								*/

struct wsm_cmd {
	atbm_spinlock_t lock;
	int done;
	atbm_uint8 *ptr;
	atbm_uint32 len;
	atbm_void *arg;
	int ret;
	atbm_uint16 cmd;
};

/* ******************************************************************** */
/* WSM TX buffer access							*/

int wsm_get_tx(struct atbmwifi_common *hw_priv, atbm_uint8 **data,
	       atbm_size_t *tx_len, int *burst, int *vif_selected);
int wsm_txed(struct atbmwifi_common *hw_priv, atbm_uint8 *data);
int wsm_set_uapsd_info(struct atbmwifi_common *hw_priv,
				     struct wsm_uapsd_info *arg,
				     int if_id);
int wsm_set_rcpi_rssi_threshold(struct atbmwifi_common *hw_priv,
					struct wsm_rcpi_rssi_threshold *arg,
					int if_id);
int wsm_set_rx_filter(struct atbmwifi_common *hw_priv,
				    const struct wsm_rx_filter *arg,
				    int if_id);
int wsm_set_beacon_filter_table(struct atbmwifi_common *hw_priv,
					struct wsm_beacon_filter_table *ft,
					int if_id);
int wsm_beacon_filter_control(struct atbmwifi_common *hw_priv,
					struct wsm_beacon_filter_control *arg,
					int if_id);
int wsm_set_bssid_filtering(struct atbmwifi_common *hw_priv,
					  ATBM_BOOL enabled, int if_id);

int wsm_set_operational_mode(struct atbmwifi_common *hw_priv,
					const struct wsm_operational_mode *arg,
					int if_id);
int wsm_use_multi_tx_conf(struct atbmwifi_common *hw_priv,
					ATBM_BOOL enabled, int if_id);
atbm_uint8 wsm_queue_id_to_wsm(atbm_uint8 queueId);

int wsm_set_tx_rate_retry_policy(struct atbmwifi_common *hw_priv,
				struct wsm_set_tx_rate_retry_policy *arg,
				int if_id);
int wsm_set_block_ack_policy(struct atbmwifi_common *hw_priv,
					   atbm_uint8 blockAckTxTidPolicy,
					   atbm_uint8 blockAckRxTidPolicy,
					   int if_id);
int wsm_set_template_frame(struct atbmwifi_common *hw_priv,
					 struct wsm_template_frame *arg,
					 int if_id);
int wsm_set_beacon_wakeup_period(struct atbmwifi_common *hw_priv,
					       unsigned dtim_interval,
					       unsigned listen_interval,
					       int if_id);
int wsm_keep_alive_period(struct atbmwifi_common *hw_priv,
					int period, int if_id);

int wsm_set_association_mode(struct atbmwifi_common *hw_priv,
					   struct wsm_association_mode *arg,
					   int if_id);
/*
*rx ba session start
*/
#define WSM_RX_BA_SESSION_REQ_ID 0x0014
#define WSM_RX_BA_SESSION_RESP_ID	0x0414

#define WSM_RX_BA_SESSION_MODE__DISABLE 0
#define WSM_RX_BA_SESSION_MODE__ENABLE	1
#define WSM_RX_BA_SESSION_MODE__UPDATE	3
struct wsm_rx_ba_session{
	atbm_uint8 mode;
	atbm_uint8 win_size;
	atbm_uint8 tid;
	atbm_uint8 rxtx;
	atbm_uint8 TA[ATBM_ETH_ALEN];
	atbm_uint16 ssn;
	atbm_uint16 timeout;
	atbm_uint8 hw_token;
	atbm_uint8 resv2;
};
int wsm_req_rx_ba_session(struct atbmwifi_common *hw_priv,struct wsm_rx_ba_session *arg,int if_id);


#define WSM_SET_CHANTYPE_ID			(0x24)
#define WSM_SET_CHANTYPE_RESP_ID		(0x424)
#define WSM_SET_CHANTYPE_FLAGS__CHANNUM_CHANGE		(0)
#define WSM_SET_CHANTYPE_FLAGS__CHANTYPE_CHANGE		(1)
#define WSM_SET_CHANTYPE_FLAGS__TOOL_SET_FORCE		(2)
#define WSM_SET_CHANTYPE_FLAGS__CCA_LEVEL_CHANGE	(3)
#define WSM_SET_CHANTYPE_PRB_TPC					(5)
#define WSM_SET_CHANTYPE_FLAGS__ETF_GREEDFILED		(6)
#define WSM_SET_CHANTYPE_FLAGS__ETF_TEST_START		(7)
//see doc /* ´ø¿íÑ¡ÔñËã·¨.doc*/
struct wsm_set_chantype
{
	atbm_uint8		band;			//0:2.4G,1:5G
	atbm_uint8		flag;			//no use
	atbm_uint16		channelNumber;	// channel number
	atbm_uint32		channelType;	// channel type
};

#define WSM_GET_CCA_ID			(0x1F)
#define WSM_GET_CCA_RESP_ID		(0x41F)
#define WSM_GET_CCA_FLAGS__START_CCA			(BIT(0))
#define WSM_GET_CCA_FLAGS__STOP_CCA				(BIT(1))
#define WSM_GET_CCA_FLAGS__SET_INIT				(BIT(2))
struct wsm_get_cca_req
{
	atbm_uint32		flags;
	atbm_uint32		rx_phy_enable_num_req;
};
struct wsm_get_cca_resp
{
	atbm_uint32		status;
	atbm_uint32	    rx_phy_enable_num_cnf;//
	atbm_uint32	    pri_channel_idle_cnt;//priv channel idle time (util 50us)
	atbm_uint32	    pri_snd_channel_idle_cnt;//priv channel and snd channel at the same time  idle time (util 50us)
};
#define WSM_SEND_CHTYPE_CHG_REQUEST__FLAGS_SEN_40M	(BIT(1))
#define WSM_SEND_CHTYPE_CHG_REQUEST__FLAGS_SEN_20M	(BIT(0))
struct wsm_req_chtype_change
{
	atbm_uint8 MacAddr[6];
	atbm_uint16 flags;
};
struct wsm_req_chtype_change_ind
{
	atbm_uint32 status;
};



#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/epstruct.h"
#endif

#define WSM_SEND_CHTYPE_CHG_REQUEST_RESP_ID	(0x425)
#define WSM_SEND_CHTYPE_CHG_REQUEST_ID			(0x25)
#define WSM_SEND_CHTYPE_CHG_REQUEST_IND_ID		(0x825)

typedef struct WSM_REGVAL_S {
	atbm_uint32 addr;
	atbm_uint32 val;
} wsm_regval;

typedef struct WSM_REGVAL_BIT_S {
	atbm_uint32 addr;
	atbm_uint16 endBit;
	atbm_uint16 startBit;
	atbm_uint32 data;
} wsm_regval_bit;

typedef struct wsm_phy_init_global_para_s {
	atbm_uint32 wifi_chip_id;
	atbm_uint8 rx_outer_lna_flag;
	atbm_uint8 set_adaptive_flag;
	atbm_uint16 stationId0[3];
	atbm_uint16 stationId1[3];
	atbm_uint8 rfSubtypeDefine;
	atbm_uint8 aresAy;
	atbm_uint8 aresAzlc;
	atbm_uint8 hwChipVersionAresM;
	atbm_uint8 hwChipVersionAresB;
	atbm_uint8 hwChipVersionMar;
	atbm_uint8 customerPlatform;
	atbm_uint8 sigmastarNvrIpc;
	atbm_uint8 specialCut;
	atbm_uint8 aresm6012B;
	atbm_uint8 chipType;
	atbm_uint8 chipAresB;
	atbm_uint8 chipFeatureType;
	atbm_uint8 richwaveOuterpaSwType;
	atbm_uint8 kxcomtechOuterpaSwType;
	atbm_uint8 qorvo4219OuterpaType;
	atbm_uint8 richwaveOuterpaType;
	atbm_uint8 reserve[7];
	atbm_uint32 compileMacro;
} wsm_phy_init_global_para;

int wsm_phy_write_reg_bit_u32(struct atbmwifi_common *hw_priv, const wsm_regval_bit *pTable,
	atbm_uint32 tableSize);
int wsm_phy_init_get_global_flag(struct atbmwifi_common *hw_priv,
	wsm_phy_init_global_para *para_resp);


#define WSM_TXRX_DATA_TEST_REQUEST_ID (0x21)
#define WSM_TXRX_DATA_TEST_RESPONSE_ID (0x421)

#define WSM_BLE_MSG_REQ_ID 		0x0029
#define WSM_BLE_MSG_RESP_ID 	0x0429


#define BLE_MSG_COEXIST_START 			0x00
#define BLE_MSG_COEXIST_STOP 			0x01
#define BLE_MSG_SET_ADV_DATA 			0x02
#define BLE_MSG_ADV_RESP_MODE_START 	0x03
#define BLE_MSG_SET_RESP_DATA 			0x04
#define BLE_MSG_MAX_ID 					0x04



struct wsm_ble_msg {
	atbm_uint32 status;
	atbm_uint8 ble_id;
	atbm_uint8 reserved[3];
};

struct wsm_ble_msg_coex_start {
	atbm_uint32 status;
	atbm_uint8 ble_id;
	atbm_uint8 reserved[3];
	atbm_uint32 interval;
	atbm_uint32 coex_flag;
	atbm_uint32 scan_win;
	atbm_uint32 chan_flag;
};

struct wsm_ble_msg_adv_data {
	atbm_uint32 status;
	atbm_uint8 ble_id;
	atbm_uint8 reserved[3];
	atbm_uint8 mac[6];
	atbm_uint8 adv_data_len;
	atbm_uint8 adv_data[31];
};

struct wsm_ble_msg_adv_resp_start {
	atbm_uint32 status;
	atbm_uint8 ble_id;
	atbm_uint8 reserved[3];
	atbm_uint32 interval;
};

struct wsm_ble_msg_resp_data {
	atbm_uint32 status;
	atbm_uint8 ble_id;
	atbm_uint8 reserved[3];
	atbm_uint8 resp_data_len;
	atbm_uint8 resp_data[31];
};

struct wsm_ble_adv_rpt{
	atbm_uint8 mac[6];
	atbm_uint8 adv_data_len;
	atbm_uint8 adv_data[31];
};

struct wsm_ble_conn_rpt{
	atbm_uint8 init_addr[6];
	atbm_uint8 adv_addr[6];
};

struct wsm_ble_rpt {
	union{
		struct wsm_ble_adv_rpt adv_rpt;
		struct wsm_ble_conn_rpt conn_rpt;
	}u;
};



int wsm_ble_msg_coexist_start(struct atbmwifi_common *hw_priv, const struct wsm_ble_msg_coex_start *arg, int if_id);
int wsm_ble_msg_coexist_stop(struct atbmwifi_common *hw_priv, const struct wsm_ble_msg *arg, int if_id);
int wsm_ble_msg_set_adv_data(struct atbmwifi_common *hw_priv, const struct wsm_ble_msg_adv_data *arg, int if_id);
int wsm_ble_indication(struct atbmwifi_common *hw_priv, struct wsm_buf *buf, int if_id);


#define WSM_SEND_CHTYPE_CHG_REQUEST_RESP_ID	(0x425)
#define WSM_SEND_CHTYPE_CHG_REQUEST_ID			(0x25)
#define WSM_SEND_CHTYPE_CHG_REQUEST_IND_ID		(0x825)


#define HI_MSG_ID_BLE_BIT			BIT(8)
#define HI_MSG_ID_BLE_BASE			(0x800+HI_MSG_ID_BLE_BIT)
#define HI_MSG_ID_BLE_BASE_CNF		(HI_MSG_ID_BLE_BASE | WSM_CNF_BASE)
#define HI_MSG_ID_BLE_EVENT			(HI_MSG_ID_BLE_BASE+0x03)
#define HI_MSG_ID_BLE_ACK			(HI_MSG_ID_BLE_BASE+0x04)


int wsm_set_chantype_func(struct atbmwifi_common *hw_priv,
				    struct wsm_set_chantype *arg,int if_id);
int wsm_get_cca(struct atbmwifi_common *hw_priv,struct wsm_get_cca_req *arg,
				struct wsm_get_cca_resp *cca_res,
				int if_id);
int wsm_get_cca_confirm(struct atbmwifi_common *hw_priv,
                                struct wsm_get_cca_resp *arg, struct wsm_buf *buf);
extern int wsm_req_chtype_change_func(struct atbmwifi_common *hw_priv,
											struct wsm_req_chtype_change *arg,int if_id);
int wsm_req_chtype_indication(struct atbmwifi_common *hw_priv,
					 struct wsm_buf *buf);

atbm_void atbm_get_mac_address(struct atbmwifi_common *hw_priv);
int wsm_set_inactivity(struct atbmwifi_common *hw_priv,
					const struct wsm_inactivity *arg,
					int if_id);
int wsm_release_vif_tx_buffer(struct atbmwifi_common *hw_priv, int if_id,
				int count);
atbm_void wsm_alloc_tx_buffer(struct atbmwifi_common *hw_priv);
int wsm_release_tx_buffer(struct atbmwifi_common *hw_priv, int count);
atbm_void wsm_alloc_tx_buffer_nolock(struct atbmwifi_common *hw_priv);
int wsm_release_tx_buffer_nolock(struct atbmwifi_common *hw_priv, int count);
int wsm_stop_tx(struct atbmwifi_common *hw_priv, struct atbmwifi_vif *priv );
int wsm_start_tx(struct atbmwifi_common *hw_priv, struct atbmwifi_vif *priv );
int  wsm_sync_channle_process(struct atbmwifi_common *hw_priv,int type) ;
int wsm_set_protected_mgmt_policy(struct atbmwifi_common *hw_priv,
			      struct wsm_protected_mgmt_policy *arg,
			      int if_id);
int wsm_check_txrx_data(struct atbmwifi_common *hw_priv,		void *arg,	struct wsm_buf *buf);
int wsm_txrx_data_test(struct atbmwifi_common *hw_priv,
				   int len,int if_id);
int wsm_ble_xmit(struct atbmwifi_common *hw_priv,atbm_uint8 *xmit,atbm_size_t len);

#endif /* ATBMWIFI__WSM_H_INCLUDED */
