#ifndef _OS_NET_TYPE_H_
#define _OS_NET_TYPE_H_
#define ATBM_ZEROSIZE		1
#define ATBM_MAX_BSS_LIST_SIZE   	2   //实际只会保存一个
#define	ATBM_IFNAMSIZ	8
#define	ATBM_IFALIASZ	256
/* Standard interface flags (netdevice->flags). */
#define	ATBM_IFF_UP		0x1		/* interface is up		*/
#define	ATBM_IFF_BROADCAST	0x2		/* broadcast address valid	*/
#define	ATBM_IFF_DEBUG	0x4		/* turn on debugging		*/
#define	ATBM_IFF_LOOPBACK	0x8		/* is a loopback net		*/
#define	ATBM_IFF_POINTOPOINT	0x10		/* interface is has p-p link	*/
#define	ATBM_IFF_NOTRAILERS	0x20		/* avoid use of trailers	*/
#define	ATBM_IFF_RUNNING	0x40		/* interface RFC2863 OPER_UP	*/
#define	ATBM_IFF_NOARP	0x80		/* no ARP protocol		*/
#define	ATBM_IFF_PROMISC	0x100		/* receive all packets		*/
#define	ATBM_IFF_ALLMULTI	0x200		/* receive all multicast packets*/

#define ATBM_IFF_MASTER	0x400		/* master of a load balancer 	*/
#define ATBM_IFF_SLAVE	0x800		/* slave of a load balancer	*/

#define ATBM_IFF_MULTICAST	0x1000		/* Supports multicast		*/

#define ATBM_IFF_PORTSEL	0x2000          /* can set media type		*/
#define ATBM_IFF_AUTOMEDIA	0x4000		/* auto media select active	*/
#define ATBM_IFF_DYNAMIC	0x8000		/* dialup device with changing addresses*/

#define ATBM_IFF_LOWER_UP	0x10000		/* driver signals L1 up		*/
#define ATBM_IFF_DORMANT	0x20000		/* driver signals dormant	*/

#define ATBM_IFF_ECHO	0x40000		/* echo sent packets		*/


#define ATBM_IFF_VOLATILE	(ATBM_IFF_LOOPBACK|ATBM_IFF_POINTOPOINT|ATBM_IFF_BROADCAST|ATBM_IFF_ECHO|\
		ATBM_IFF_MASTER|ATBM_IFF_SLAVE|ATBM_IFF_RUNNING|ATBM_IFF_LOWER_UP|ATBM_IFF_DORMANT)

/* Private (from user) interface flags (netdevice->priv_flags). */
#define ATBM_IFF_802_1Q_VLAN 0x1             /* 802.1Q VLAN device.          */
#define ATBM_IFF_EBRIDGE	0x2		/* Ethernet bridging device.	*/
#define ATBM_IFF_SLAVE_INACTIVE	0x4	/* bonding slave not the curr. active */
#define ATBM_IFF_MASTER_8023AD	0x8	/* bonding master, 802.3ad. 	*/
#define ATBM_IFF_MASTER_ALB	0x10		/* bonding master, balance-alb.	*/
#define ATBM_IFF_BONDING	0x20		/* bonding master or slave	*/
#define ATBM_IFF_SLAVE_NEEDARP 0x40		/* need ARPs for validation	*/
#define ATBM_IFF_ISATAP	0x80		/* ISATAP interface (RFC4214)	*/
#define ATBM_IFF_MASTER_ARPMON 0x100		/* bonding master, ARP mon in use */
#define ATBM_IFF_WAN_HDLC	0x200		/* WAN HDLC device		*/
#define ATBM_IFF_XMIT_DST_RELEASE 0x400	/* dev_hard_start_xmit() is allowed to
					 * release skb->dst
					 */
#define ATBM_IFF_DONT_BRIDGE 0x800		/* disallow bridging this ether dev */
#define ATBM_IFF_DISABLE_NETPOLL	0x1000	/* disable netpoll at run-time */
#define ATBM_IFF_MACVLAN_PORT	0x2000	/* device used as macvlan port */
#define ATBM_IFF_BRIDGE_PORT	0x4000		/* device used as bridge port */
#define ATBM_IFF_OVS_DATAPATH	0x8000	/* device used as Open vSwitch
					 * datapath port */
#define ATBM_IF_GET_IFACE	0x0001		/* for querying only */
#define ATBM_IF_GET_PROTO	0x0002
/* For definitions see hdlc.h */
#define ATBM_IF_IFACE_V35	0x1000		/* V.35 serial interface	*/
#define ATBM_IF_IFACE_V24	0x1001		/* V.24 serial interface	*/
#define ATBM_IF_IFACE_X21	0x1002		/* X.21 serial interface	*/
#define ATBM_IF_IFACE_T1	0x1003		/* T1 telco serial interface	*/
#define ATBM_IF_IFACE_E1	0x1004		/* E1 telco serial interface	*/
#define ATBM_IF_IFACE_SYNC_SERIAL 0x1005	/* can't be set by software	*/
#define ATBM_IF_IFACE_X21D   0x1006          /* X.21 Dual Clocking (FarSite) */
/* For definitions see hdlc.h */
#define ATBM_IF_PROTO_HDLC	0x2000		/* raw HDLC protocol		*/
#define ATBM_IF_PROTO_PPP	0x2001		/* PPP protocol			*/
#define ATBM_IF_PROTO_CISCO	0x2002		/* Cisco HDLC protocol		*/
#define ATBM_IF_PROTO_FR	0x2003		/* Frame Relay protocol		*/
#define ATBM_IF_PROTO_FR_ADD_PVC 0x2004	/*    Create FR PVC		*/
#define ATBM_IF_PROTO_FR_DEL_PVC 0x2005	/*    Delete FR PVC		*/
#define ATBM_IF_PROTO_X25	0x2006		/* X.25				*/
#define ATBM_IF_PROTO_HDLC_ETH 0x2007	/* raw HDLC, Ethernet emulation	*/
#define ATBM_IF_PROTO_FR_ADD_ETH_PVC 0x2008	/*  Create FR Ethernet-bridged PVC */
#define ATBM_IF_PROTO_FR_DEL_ETH_PVC 0x2009	/*  Delete FR Ethernet-bridged PVC */
#define ATBM_IF_PROTO_FR_PVC	0x200A		/* for reading PVC status	*/
#define ATBM_IF_PROTO_FR_ETH_PVC 0x200B
#define ATBM_IF_PROTO_RAW    0x200C          /* RAW Socket                   */
#define ATBM_ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ATBM_ETH_HLEN	14		/* Total octets in header.	 */
#define ATBM_ETH_ZLEN	60		/* Min. octets in frame sans FCS */
#define ATBM_ETH_DATA_LEN	1500		/* Max. octets in payload	 */
#define ATBM_ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */
#define ATBM_ETH_FCS_LEN	4		/* Octets in the FCS		 */

extern unsigned int lbs_debug;
/*
 *	These are the defined Ethernet Protocol ID's.
 */

#define ATBM_ETH_P_LOOP	0x0060		/* Ethernet Loopback packet	*/
#define ATBM_ETH_P_PUP	0x0200		/* Xerox PUP packet		*/
#define ATBM_ETH_P_PUPAT	0x0201		/* Xerox PUP Addr Trans packet	*/
#define ATBM_ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ATBM_ETH_P_X25	0x0805		/* CCITT X.25			*/
#define ATBM_ETH_P_ARP	0x0806		/* Address Resolution packet	*/
#define	ATBM_ETH_P_BPQ	0x08FF		/* G8BPQ AX.25 Ethernet Packet	[ NOT AN OFFICIALLY REGISTERED ID ] */
#define ATBM_ETH_P_IEEEPUP	0x0a00		/* Xerox IEEE802.3 PUP packet */
#define ATBM_ETH_P_IEEEPUPAT	0x0a01		/* Xerox IEEE802.3 PUP Addr Trans packet */
#define ATBM_ETH_P_DEC       0x6000          /* DEC Assigned proto           */
#define ATBM_ETH_P_DNA_DL    0x6001          /* DEC DNA Dump/Load            */
#define ATBM_ETH_P_DNA_RC    0x6002          /* DEC DNA Remote Console       */
#define ATBM_ETH_P_DNA_RT    0x6003          /* DEC DNA Routing              */
#define ATBM_ETH_P_LAT       0x6004          /* DEC LAT                      */
#define ATBM_ETH_P_DIAG      0x6005          /* DEC Diagnostics              */
#define ATBM_ETH_P_CUST      0x6006          /* DEC Customer use             */
#define ATBM_ETH_P_SCA       0x6007          /* DEC Systems Comms Arch       */
#define ATBM_ETH_P_TEB	0x6558		/* Trans Ether Bridging		*/
#define ATBM_ETH_P_RARP      0x8035		/* Reverse Addr Res packet	*/
#define ATBM_ETH_P_ATALK	0x809B		/* Appletalk DDP		*/
#define ATBM_ETH_P_AARP	0x80F3		/* Appletalk AARP		*/
#define ATBM_ETH_P_8021Q	0x8100          /* 802.1Q VLAN Extended Header  */
#define ATBM_ETH_P_IPX	0x8137		/* IPX over DIX			*/
#define ATBM_ETH_P_IPV6	0x86DD		/* IPv6 over bluebook		*/
#define ATBM_ETH_P_PAUSE	0x8808		/* IEEE Pause frames. See 802.3 31B */
#define ATBM_ETH_P_SLOW	0x8809		/* Slow Protocol. See 802.3ad 43B */
#define ATBM_ETH_P_WCCP	0x883E		/* Web-cache coordination protocol
				 * defined in draft-wilson-wrec-wccp-v2-00.txt */
#define ATBM_ETH_P_PPP_DISC	0x8863		/* PPPoE discovery messages     */
#define ATBM_ETH_P_PPP_SES	0x8864		/* PPPoE session messages	*/
#define ATBM_ETH_P_MPLS_UC	0x8847		/* MPLS Unicast traffic		*/
#define ATBM_ETH_P_MPLS_MC	0x8848		/* MPLS Multicast traffic	*/
#define ATBM_ETH_P_ATMMPOA	0x884c		/* MultiProtocol Over ATM	*/
#define ATBM_ETH_P_LINK_CTL	0x886c		/* HPNA, wlan link local tunnel */
#define ATBM_ETH_P_ATMFATE	0x8884		/* Frame-based ATM Transport
					 * over Ethernet
					 */
#define ATBM_ETH_P_PAE	0x888E		/* Port Access Entity (IEEE 802.1X) */
#define ATBM_ETH_P_AOE	0x88A2		/* ATA over Ethernet		*/
#define ATBM_ETH_P_TIPC	0x88CA		/* TIPC 			*/
#define ATBM_ETH_P_1588	0x88F7		/* IEEE 1588 Timesync */
#define ATBM_ETH_P_FCOE	0x8906		/* Fibre Channel over Ethernet  */
#define ATBM_ETH_P_FIP	0x8914		/* FCoE Initialization Protocol */
#define ATBM_ETH_P_EDSA	0xDADA		/* Ethertype DSA [ NOT AN OFFICIALLY REGISTERED ID ] */
/*
 *	Non DIX types. Won't clash for 1500 types.
 */
#define ATBM_ETH_P_802_3	0x0001		/* Dummy type for 802.3 frames  */
#define ATBM_ETH_P_AX25	0x0002		/* Dummy protocol id for AX.25  */
#define ATBM_ETH_P_ALL	0x0003		/* Every packet (be careful!!!) */
#define ATBM_ETH_P_802_2	0x0004		/* 802.2 frames 		*/
#define ATBM_ETH_P_SNAP	0x0005		/* Internal only		*/
#define ATBM_ETH_P_DDCMP     0x0006          /* DEC DDCMP: Internal only     */
#define ATBM_ETH_P_WAN_PPP   0x0007          /* Dummy type for WAN PPP frames*/
#define ATBM_ETH_P_PPP_MP    0x0008          /* Dummy type for PPP MP frames */
#define ATBM_ETH_P_LOCALTALK 0x0009		/* Localtalk pseudo type 	*/
#define ATBM_ETH_P_CAN	0x000C		/* Controller Area Network      */
#define ATBM_ETH_P_PPPTALK	0x0010		/* Dummy type for Atalk over PPP*/
#define ATBM_ETH_P_TR_802_2	0x0011		/* 802.2 frames 		*/
#define ATBM_ETH_P_MOBITEX	0x0015		/* Mobitex (kaz@cafe.net)	*/
#define ATBM_ETH_P_CONTROL	0x0016		/* Card specific control frames */
#define ATBM_ETH_P_IRDA	0x0017		/* Linux-IrDA			*/
#define ATBM_ETH_P_ECONET	0x0018		/* Acorn Econet			*/
#define ATBM_ETH_P_HDLC	0x0019		/* HDLC frames			*/
#define ATBM_ETH_P_ARCNET	0x001A		/* 1A for ArcNet :-)            */
#define ATBM_ETH_P_DSA	0x001B		/* Distributed Switch Arch.	*/
#define ATBM_ETH_P_TRAILER	0x001C		/* Trailer switch tagging	*/
#define ATBM_ETH_P_PHONET	0x00F5		/* Nokia Phonet frames          */
#define ATBM_ETH_P_IEEE802154 0x00F6		/* IEEE802.15.4 frame		*/
#define ATBM_ETH_P_CAIF	0x00F7		/* ST-Ericsson CAIF protocol	*/

struct atbmwifi_ieee8023_hdr {
	unsigned char	h_dest[ATBM_ETH_ALEN];	/* destination eth addr	*/
	unsigned char	h_source[ATBM_ETH_ALEN];	/* source ether addr	*/
	atbm_uint16		h_proto;		/* packet type ID field	*/
}atbm_packed;


#define ATBM_IPH_TOS(hdr) (atbm_ntohs((hdr)->_v_hl_tos) & 0xff)
/*
 *	This is an Ethernet frame header.
 */
struct  atbm_ip_addr {
	  atbm_uint32 addr;
}atbm_packed;
struct atbm_ip_hdr {
  /* version / header length / type of service */
  atbm_uint8 _v_hl;
  atbm_uint8 _tos;
  /* total length */
  atbm_uint16 _len;
  /* identification */
  atbm_uint16 _id;
  /* fragment offset field */
  atbm_uint16 _offset;
#define ATBM_IP_RF 0x8000        /* reserved fragment flag */
#define ATBM_IP_DF 0x4000        /* dont fragment flag */
#define ATBM_IP_MF 0x2000        /* more fragments flag */
#define ATBM_IP_OFFMASK 0x1fff   /* mask for fragmenting bits */
  /* time to live / protocol*/
 atbm_uint8 _ttl;
 atbm_uint8 _proto;
  /* checksum */
  atbm_uint16 _chksum;
  /* source and destination IP addresses */
  struct atbm_ip_addr src;
  struct atbm_ip_addr dest; 
}atbm_packed;
typedef unsigned char Boolean;


#define ATBM_WPA_CIPHER_NONE BIT(0)
#define ATBM_WPA_CIPHER_WEP40 BIT(1)
#define ATBM_WPA_CIPHER_WEP104 BIT(2)
#define ATBM_WPA_CIPHER_TKIP BIT(3)
#define ATBM_WPA_CIPHER_CCMP BIT(4)
#if CONFIG_IEEE80211W
#define ATBM_WPA_CIPHER_AES_128_CMAC BIT(5)
#endif /* CONFIG_IEEE80211W */
#define ATBM_WPA_CIPHER_GCMP BIT(6)
#ifdef CONFIG_WAPI
#define ATBM_WPA_CIPHER_SMS4 BIT(7)
#endif

#define ATBM_WPA_KEY_MGMT_IEEE8021X BIT(0)
#define ATBM_WPA_KEY_MGMT_PSK BIT(1)
#define ATBM_WPA_KEY_MGMT_NONE BIT(2)
#define ATBM_WPA_KEY_MGMT_IEEE8021X_NO_WPA BIT(3)
#define ATBM_WPA_KEY_MGMT_WPA_NONE BIT(4)
#define ATBM_WPA_KEY_MGMT_FT_IEEE8021X BIT(5)
#define ATBM_WPA_KEY_MGMT_FT_PSK BIT(6)
#define ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256 BIT(7)
#define ATBM_WPA_KEY_MGMT_PSK_SHA256 BIT(8)
#define ATBM_WPA_KEY_MGMT_WPS BIT(9)
#define ATBM_WPA_KEY_MGMT_SAE BIT(10)
#define ATBM_WPA_KEY_MGMT_FT_SAE BIT(11)
#define ATBM_WPA_KEY_MGMT_WEP BIT(12)

#define atbmwifi_wpa_key_mgmt_wpa_ieee8021x(akm)			  \
(													   \
		akm & ATBM_WPA_KEY_MGMT_IEEE8021X ||			   \
		akm & ATBM_WPA_KEY_MGMT_FT_IEEE8021X ||			   \
		akm & ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256		   \
)
#define atbmwifi_wpa_key_mgmt_wpa_psk(akm)						\
(														\
		akm & ATBM_WPA_KEY_MGMT_PSK ||						 \
		akm & ATBM_WPA_KEY_MGMT_FT_PSK ||					 \
		akm & ATBM_WPA_KEY_MGMT_PSK_SHA256					 \
)
#define atbmwifi_wpa_key_mgmt_ft(akm)							\
(														\
		akm & ATBM_WPA_KEY_MGMT_FT_PSK ||					\
		akm & ATBM_WPA_KEY_MGMT_FT_IEEE8021X				\
)
#define atbmwifi_wpa_key_mgmt_sha256(akm)					   \
(													   \
		akm & ATBM_WPA_KEY_MGMT_PSK_SHA256 ||			   \
		akm & ATBM_WPA_KEY_MGMT_IEEE8021X_SHA256			||\
		akm & ATBM_WPA_KEY_MGMT_SAE || \
		akm & ATBM_WPA_KEY_MGMT_FT_SAE \
)
#define atbmwifi_wpa_key_mgmt_sae(akm) \
(											\
		akm & ATBM_WPA_KEY_MGMT_SAE || \
		akm & ATBM_WPA_KEY_MGMT_FT_SAE \
)

#define ATBM_WPA_PROTO_WPA BIT(0)
#define ATBM_WPA_PROTO_RSN BIT(1)

#define ATBM_WPA_AUTH_ALG_OPEN BIT(0)
#define ATBM_WPA_AUTH_ALG_SHARED BIT(1)
#define ATBM_WPA_AUTH_ALG_LEAP BIT(2)
#define ATBM_WPA_AUTH_ALG_FT BIT(3)
#define ATBM_WPA_AUTH_ALG_SAE BIT(4)
#define ATBM_WPA_AUTH_ALG_FILS BIT(5)
#define ATBM_WPA_AUTH_ALG_FILS_SK_PFS BIT(6)

enum atbm_wpa_alg {
	ATBM_WPA_ALG_NONE,
	ATBM_WPA_ALG_WEP,
	ATBM_WPA_ALG_TKIP,
	ATBM_WPA_ALG_CCMP,
	ATBM_WPA_ALG_IGTK,
	ATBM_WPA_ALG_PMK
};
/**
 * enum wpa_cipher - Cipher suites
 */
enum atbm_wpa_cipher {
	ATBM_CIPHER_NONE,
	ATBM_CIPHER_WEP40,
	ATBM_CIPHER_TKIP,
	ATBM_CIPHER_CCMP,
	ATBM_CIPHER_WEP104,
	ATBM_CIPHER_GCMP,
#ifdef CONFIG_WAPI
	ATBM_CIPHER_SMS4
#endif
};
/**
 * enum wpa_key_mgmt - Key management suites
 */
enum atbm_wpa_key_mgmt {
	ATBM_KEY_MGMT_802_1X,
	ATBM_KEY_MGMT_PSK,
	ATBM_KEY_MGMT_NONE,
	ATBM_KEY_MGMT_802_1X_NO_WPA,
	ATBM_KEY_MGMT_WPA_NONE,
	ATBM_KEY_MGMT_FT_802_1X,
	ATBM_KEY_MGMT_FT_PSK,
	ATBM_KEY_MGMT_802_1X_SHA256,
	ATBM_KEY_MGMT_PSK_SHA256,
	ATBM_KEY_MGMT_WPS
};
/**
 * enum wpa_states - wpa_supplicant state
 *
 * These enumeration values are used to indicate the current wpa_supplicant
 * state (wpa_s->wpa_state). The current state can be retrieved with
 * wpa_supplicant_get_state() function and the state can be changed by calling
 * wpa_supplicant_set_state(). In WPA state machine (wpa.c and preauth.c), the
 * wrapper functions wpa_sm_get_state() and wpa_sm_set_state() should be used
 * to access the state variable.
 */
enum atbm_wpa_states {
	/**
	 * ATBM_WPA_DISCONNECTED - Disconnected state
	 *
	 * This state indicates that client is not associated, but is likely to
	 * start looking for an access point. This state is entered when a
	 * connection is lost.
	 */
	ATBM_WPA_DISCONNECTED,
	/**
	 * WPA_INTERFACE_DISABLED - Interface disabled
	 *
	 * This stat eis entered if the network interface is disabled, e.g.,
	 * due to rfkill. wpa_supplicant refuses any new operations that would
	 * use the radio until the interface has been enabled.
	 */
	ATBM_WPA_INTERFACE_DISABLED,

	/**
	 * WPA_INACTIVE - Inactive state (wpa_supplicant disabled)
	 *
	 * This state is entered if there are no enabled networks in the
	 * configuration. wpa_supplicant is not trying to associate with a new
	 * network and external interaction (e.g., ctrl_iface call to add or
	 * enable a network) is needed to start association.
	 */
	ATBM_WPA_INACTIVE,

	/**
	 * ATBM_WPA_SCANNING - Scanning for a network
	 *
	 * This state is entered when wpa_supplicant starts scanning for a
	 * network.
	 */
	ATBM_WPA_SCANNING,

	/**
	 * ATBM_WPA_AUTHENTICATING - Trying to authenticate with a BSS/SSID
	 *
	 * This state is entered when wpa_supplicant has found a suitable BSS
	 * to authenticate with and the driver is configured to try to
	 * authenticate with this BSS. This state is used only with drivers
	 * that use wpa_supplicant as the SME.
	 */
	ATBM_WPA_AUTHENTICATING,

	/**
	 * WPA_ASSOCIATING - Trying to associate with a BSS/SSID
	 *
	 * This state is entered when wpa_supplicant has found a suitable BSS
	 * to associate with and the driver is configured to try to associate
	 * with this BSS in ap_scan=1 mode. When using ap_scan=2 mode, this
	 * state is entered when the driver is configured to try to associate
	 * with a network using the configured SSID and security policy.
	 */
	ATBM_WPA_ASSOCIATING,

	/**
	 * ATBM_WPA_ASSOCIATED - Association completed
	 *
	 * This state is entered when the driver reports that association has
	 * been successfully completed with an AP. If IEEE 802.1X is used
	 * (with or without WPA/WPA2), wpa_supplicant remains in this state
	 * until the IEEE 802.1X/EAPOL authentication has been completed.
	 */
	ATBM_WPA_ASSOCIATED,

	/**
	 * ATBM_WPA_4WAY_HANDSHAKE - WPA 4-Way Key Handshake in progress
	 *
	 * This state is entered when WPA/WPA2 4-Way Handshake is started. In
	 * case of WPA-PSK, this happens when receiving the first EAPOL-Key
	 * frame after association. In case of WPA-EAP, this state is entered
	 * when the IEEE 802.1X/EAPOL authentication has been completed.
	 */
	ATBM_WPA_4WAY_HANDSHAKE,

	/**
	 * ATBM_WPA_GROUP_HANDSHAKE - WPA Group Key Handshake in progress
	 *
	 * This state is entered when 4-Way Key Handshake has been completed
	 * (i.e., when the supplicant sends out message 4/4) and when Group
	 * Key rekeying is started by the AP (i.e., when supplicant receives
	 * message 1/2).
	 */
	ATBM_WPA_GROUP_HANDSHAKE,

	/**
	 * ATBM_WPA_COMPLETED - All authentication completed
	 *
	 * This state is entered when the full authentication process is
	 * completed. In case of WPA2, this happens when the 4-Way Handshake is
	 * successfully completed. With WPA, this state is entered after the
	 * Group Key Handshake; with IEEE 802.1X (non-WPA) connection is
	 * completed after dynamic keys are received (or if not used, after
	 * the EAP authentication has been completed). With static WEP keys and
	 * plaintext connections, this state is entered when an association
	 * has been completed.
	 *
	 * This state indicates that the supplicant has completed its
	 * processing for the association phase and that data connection is
	 * fully configured.
	 */
	ATBM_WPA_COMPLETED
};

#define ATBM_MLME_SETPROTECTION_PROTECT_TYPE_NONE 0
#define ATBM_MLME_SETPROTECTION_PROTECT_TYPE_RX 1
#define ATBM_MLME_SETPROTECTION_PROTECT_TYPE_TX 2
#define ATBM_MLME_SETPROTECTION_PROTECT_TYPE_RX_TX 3

#define ATBM_MLME_SETPROTECTION_KEY_TYPE_GROUP 0
#define ATBM_MLME_SETPROTECTION_KEY_TYPE_PAIRWISE 1


/**
 * enum mfp_options - Management frame protection (IEEE 802.11w) options
 */
enum atbm_mfp_options {
	ATBM_NO_MGMT_FRAME_PROTECTION = 0,
	ATBM_MGMT_FRAME_PROTECTION_OPTIONAL = 1,
	ATBM_MGMT_FRAME_PROTECTION_REQUIRED = 2
};

/**
 * enum hostapd_hw_mode - Hardware mode
 */
enum atbm_hostapd_hw_mode {
	ATBM_HOSTAPD_MODE_IEEE80211B,
	ATBM_HOSTAPD_MODE_IEEE80211G,
	ATBM_HOSTAPD_MODE_IEEE80211A,
	ATBM_NUM_HOSTAPD_MODES
};

atbm_uint16 atbm_htons(atbm_uint16 x);
atbm_uint16 atbm_ntohs(atbm_uint16 x);
atbm_uint32 atbm_htonl(atbm_uint32 x);
atbm_uint32 atbm_ntohl(atbm_uint32 x);

int atbmwifi_hexstr2bin(atbm_uint8 *buf, const char *hex, atbm_size_t len);

#endif /*_OS_NET_TYPE_H_*/
